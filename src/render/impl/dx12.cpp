#include "dx12.hpp"
#include "../shaders.hpp"

namespace
{
	struct alignas(16) clip_cbuffer_data
	{
		float clip_min[2];
		float clip_max[2];
		float clip_radii[4];
		float clip_enabled;
		float padding[3];
	};

	constexpr cstd::uint32_t initial_srv_heap_capacity = 64;

	bool create_upload_buffer(ID3D12Device* device, cstd::size_t size, ID3D12Resource** out, void** mapped = nullptr)
	{
		D3D12_HEAP_PROPERTIES heap_props = { };
		heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC desc = { };
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = size;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		const HRESULT hr = device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE,
			&desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(out));

		if (FAILED(hr))
		{
			return false;
		}

		if (mapped)
		{
			D3D12_RANGE read_range = { 0, 0 };
			(*out)->Map(0, &read_range, mapped);
		}

		return true;
	}
}

rv::dx12_renderer::dx12_renderer(ID3D12Device* const device, ID3D12CommandQueue* const command_queue) noexcept
	:	device_(device),
		command_queue_(command_queue)
{
	device_->AddRef();
	command_queue_->AddRef();
}

bool rv::dx12_renderer::init_backend() noexcept
{
	if (!device_ || !command_queue_)
	{
		return false;
	}

	for (cstd::uint32_t i = 0; i < frame_count; ++i)
	{
		if (FAILED(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(command_allocators_[i].release_and_get()))))
		{
			return false;
		}
	}

	if (FAILED(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		command_allocators_[0].value(), nullptr, IID_PPV_ARGS(command_list_.release_and_get()))))
	{
		return false;
	}

	command_list_->Close();

	if (!create_root_signature())
	{
		return false;
	}

	const span_t<const cstd::byte> vs_code(d3d12_vertex_shader.data(), d3d12_vertex_shader.size());
	const span_t<const cstd::byte> ps_code(d3d12_pixel_shader.data(), d3d12_pixel_shader.size());
	const span_t<const cstd::byte> shadow_ps_code(d3d12_shadow_pixel_shader.data(), d3d12_shadow_pixel_shader.size());
	const span_t<const cstd::byte> rect_ps_code(d3d12_rect_pixel_shader.data(), d3d12_rect_pixel_shader.size());
	const span_t<const cstd::byte> image_ps_code(d3d12_image_pixel_shader.data(), d3d12_image_pixel_shader.size());
	const span_t<const cstd::byte> text_shadow_ps_code(d3d12_text_shadow_pixel_shader.data(), d3d12_text_shadow_pixel_shader.size());

	if (!create_pipeline_state(vs_code, ps_code, default_pso_.release_and_get()) ||
		!create_pipeline_state(vs_code, shadow_ps_code, shadow_pso_.release_and_get()) ||
		!create_pipeline_state(vs_code, rect_ps_code, rect_pso_.release_and_get()) ||
		!create_pipeline_state(vs_code, image_ps_code, image_pso_.release_and_get()) ||
		!create_pipeline_state(vs_code, text_shadow_ps_code, text_shadow_pso_.release_and_get()))
	{
		return false;
	}

	srv_heap_capacity_ = initial_srv_heap_capacity;

	D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = { };
	srv_heap_desc.NumDescriptors = srv_heap_capacity_;
	srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	if (FAILED(device_->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(srv_heap_.release_and_get()))))
	{
		return false;
	}

	srv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	if (FAILED(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.release_and_get()))))
	{
		return false;
	}

	fence_value_ = 1;
	fence_event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);

	if (!fence_event_)
	{
		return false;
	}

	return true;
}

bool rv::dx12_renderer::create_root_signature() noexcept
{
	dx12_object<ID3D12RootSignature> rs;

	if (FAILED(device_->CreateRootSignature(0, d3d12_vertex_shader.data(), d3d12_vertex_shader.size(),
		IID_PPV_ARGS(root_signature_.release_and_get()))))
	{
		return false;
	}

	return true;
}

bool rv::dx12_renderer::create_pipeline_state(const span_t<const cstd::byte>& vs_bytecode,
                                               const span_t<const cstd::byte>& ps_bytecode,
                                               ID3D12PipelineState** out_pso) noexcept
{
	static constexpr D3D12_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,         0, offsetof(vertex, pos),         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, offsetof(vertex, col),         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,          0, offsetof(vertex, uv),          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, offsetof(vertex, custom_data), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, offsetof(vertex, custom_data) + 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = { };
	pso_desc.pRootSignature = root_signature_.value();
	pso_desc.VS.pShaderBytecode = vs_bytecode.data();
	pso_desc.VS.BytecodeLength = vs_bytecode.size();
	pso_desc.PS.pShaderBytecode = ps_bytecode.data();
	pso_desc.PS.BytecodeLength = ps_bytecode.size();

	pso_desc.BlendState.AlphaToCoverageEnable = FALSE;
	pso_desc.BlendState.IndependentBlendEnable = FALSE;

	auto& rt_blend = pso_desc.BlendState.RenderTarget[0];
	rt_blend.BlendEnable = TRUE;
	rt_blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rt_blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rt_blend.BlendOp = D3D12_BLEND_OP_ADD;
	rt_blend.SrcBlendAlpha = D3D12_BLEND_ONE;
	rt_blend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	rt_blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rt_blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	pso_desc.RasterizerState.DepthClipEnable = TRUE;
	pso_desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	pso_desc.DepthStencilState.DepthEnable = FALSE;
	pso_desc.DepthStencilState.StencilEnable = FALSE;

	pso_desc.InputLayout.pInputElementDescs = layout;
	pso_desc.InputLayout.NumElements = _countof(layout);

	pso_desc.SampleMask = UINT_MAX;
	pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pso_desc.NumRenderTargets = 1;
	pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pso_desc.SampleDesc.Count = 1;

	return SUCCEEDED(device_->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(out_pso)));
}

bool rv::dx12_renderer::create_buffer(const cstd::size_t vertex_count)
{
	const cstd::size_t size = sizeof(vertex) * vertex_count;

	for (cstd::uint32_t i = 0; i < frame_count; ++i)
	{
		if (!create_upload_buffer(device_.value(), size, vertex_upload_[i].release_and_get(), &vertex_mapped_[i]))
		{
			return false;
		}
	}

	buffer_vertex_count_ = vertex_count;
	return true;
}

bool rv::dx12_renderer::try_widen_buffer()
{
	if (pending_vertices_.size() <= buffer_vertex_count_)
	{
		return true;
	}

	constexpr cstd::size_t additional_vertices = 256;
	return create_buffer(pending_vertices_.size() + additional_vertices);
}

bool rv::dx12_renderer::create_index_buffer(const cstd::size_t index_count)
{
	const cstd::size_t size = sizeof(cstd::uint32_t) * index_count;

	for (cstd::uint32_t i = 0; i < frame_count; ++i)
	{
		if (!create_upload_buffer(device_.value(), size, index_upload_[i].release_and_get(), &index_mapped_[i]))
		{
			return false;
		}
	}

	buffer_index_count_ = index_count;
	return true;
}

bool rv::dx12_renderer::try_widen_index_buffer()
{
	if (pending_indices_.size() <= buffer_index_count_)
	{
		return true;
	}

	constexpr cstd::size_t additional_indices = 512;
	return create_index_buffer(pending_indices_.size() + additional_indices);
}

void rv::dx12_renderer::begin_frame_backend(const vector_2d<float> display_size) noexcept
{
	const cstd::uint64_t wait_value = fence_values_[frame_index_];
	if (wait_value != 0 && fence_->GetCompletedValue() < wait_value)
	{
		fence_->SetEventOnCompletion(wait_value, fence_event_);
		WaitForSingleObjectEx(fence_event_, INFINITE, FALSE);
	}

	command_allocators_[frame_index_]->Reset();
	command_list_->Reset(command_allocators_[frame_index_].value(), default_pso_.value());

	ID3D12DescriptorHeap* heaps[] = { srv_heap_.value() };
	command_list_->SetDescriptorHeaps(1, heaps);

	command_list_->SetGraphicsRootSignature(root_signature_.value());
	command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D12_VIEWPORT viewport = { };
	viewport.Width = display_size.x;
	viewport.Height = display_size.y;
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;

	command_list_->RSSetViewports(1, &viewport);
}

void rv::dx12_renderer::end_frame() noexcept
{
	flush_pending_vertices();

	command_list_->Close();

	ID3D12CommandList* lists[] = { command_list_.value() };
	command_queue_->ExecuteCommandLists(1, lists);

	const cstd::uint64_t current_fence = fence_value_;
	command_queue_->Signal(fence_.value(), current_fence);
	fence_values_[frame_index_] = current_fence;
	fence_value_++;

	frame_index_ = (frame_index_ + 1) % frame_count;
}

void rv::dx12_renderer::flush_pending_vertices() noexcept
{
	if (pending_vertices_.empty() || pending_indices_.empty())
	{
		return;
	}

	if (!try_widen_buffer() || !try_widen_index_buffer())
	{
		pending_vertices_.clear();
		pending_indices_.clear();
		pending_batches_.clear();
		return;
	}

	const cstd::uint32_t fi = frame_index_;

	cstd::memcpy(vertex_mapped_[fi], pending_vertices_.data(), pending_vertices_.size() * sizeof(vertex));
	cstd::memcpy(index_mapped_[fi], pending_indices_.data(), pending_indices_.size() * sizeof(cstd::uint32_t));

	D3D12_VERTEX_BUFFER_VIEW vbv = { };
	vbv.BufferLocation = vertex_upload_[fi]->GetGPUVirtualAddress();
	vbv.SizeInBytes = static_cast<cstd::uint32_t>(pending_vertices_.size() * sizeof(vertex));
	vbv.StrideInBytes = sizeof(vertex);

	command_list_->IASetVertexBuffers(0, 1, &vbv);

	D3D12_INDEX_BUFFER_VIEW ibv = { };
	ibv.BufferLocation = index_upload_[fi]->GetGPUVirtualAddress();
	ibv.SizeInBytes = static_cast<cstd::uint32_t>(pending_indices_.size() * sizeof(cstd::uint32_t));
	ibv.Format = DXGI_FORMAT_R32_UINT;

	command_list_->IASetIndexBuffer(&ibv);

	for (const auto& batch : pending_batches_)
	{
		const auto texture = std::static_pointer_cast<dx12_texture>(batch.texture);

		if (batch.shader == shader_type::shadow_shader)
		{
			command_list_->SetPipelineState(shadow_pso_.value());
		}
		else if (batch.shader == shader_type::rect_shader)
		{
			command_list_->SetPipelineState(rect_pso_.value());
		}
		else if (batch.shader == shader_type::image_shader)
		{
			command_list_->SetPipelineState(image_pso_.value());
		}
		else if (batch.shader == shader_type::text_shadow_shader)
		{
			command_list_->SetPipelineState(text_shadow_pso_.value());
		}
		else
		{
			command_list_->SetPipelineState(default_pso_.value());
		}

		D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_handle = srv_heap_->GetGPUDescriptorHandleForHeapStart();
		srv_gpu_handle.ptr += static_cast<cstd::uint64_t>(texture->srv_index()) * srv_descriptor_size_;
		command_list_->SetGraphicsRootDescriptorTable(1, srv_gpu_handle);

		{
			clip_cbuffer_data cb_data = { };

			if (batch.clip_rect.has_value())
			{
				const auto& clip = batch.clip_rect.value();

				cb_data.clip_min[0] = clip.bounds.min.x;
				cb_data.clip_min[1] = clip.bounds.min.y;
				cb_data.clip_max[0] = clip.bounds.max.x;
				cb_data.clip_max[1] = clip.bounds.max.y;
				cb_data.clip_enabled = 1.f;

				const float r = clip.rounding;
				cb_data.clip_radii[0] = (clip.flags & rounding_flags_top_right) ? r : 0.f;
				cb_data.clip_radii[1] = (clip.flags & rounding_flags_bottom_right) ? r : 0.f;
				cb_data.clip_radii[2] = (clip.flags & rounding_flags_bottom_left) ? r : 0.f;
				cb_data.clip_radii[3] = (clip.flags & rounding_flags_top_left) ? r : 0.f;
			}

			command_list_->SetGraphicsRoot32BitConstants(0, sizeof(cb_data) / 4, &cb_data, 0);
		}

		D3D12_RECT scissor;

		if (batch.clip_rect.has_value())
		{
			scissor.left = static_cast<LONG>(batch.clip_rect->bounds.min.x) - 1;
			scissor.top = static_cast<LONG>(batch.clip_rect->bounds.min.y) - 1;
			scissor.right = static_cast<LONG>(batch.clip_rect->bounds.max.x) + 1;
			scissor.bottom = static_cast<LONG>(batch.clip_rect->bounds.max.y) + 1;
		}
		else
		{
			scissor.left = 0;
			scissor.top = 0;
			scissor.right = static_cast<LONG>(state_.display_size.x);
			scissor.bottom = static_cast<LONG>(state_.display_size.y);
		}

		command_list_->RSSetScissorRects(1, &scissor);

		command_list_->DrawIndexedInstanced(batch.index_count, 1, batch.index_offset, batch.vertex_offset, 0);
	}

	pending_vertices_.clear();
	pending_indices_.clear();
	pending_batches_.clear();
}

shared_ptr_t<rv::texture> rv::dx12_renderer::create_texture(const span_t<const cstd::uint8_t> buffer,
                                                             const cstd::uint32_t width, const cstd::uint32_t height)
{
	if (buffer.empty() || !width || !height)
	{
		return { };
	}

	D3D12_HEAP_PROPERTIES default_heap = { };
	default_heap.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC tex_desc = { };
	tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	tex_desc.Width = width;
	tex_desc.Height = height;
	tex_desc.DepthOrArraySize = 1;
	tex_desc.MipLevels = 1;
	tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	tex_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	dx12_object<ID3D12Resource> texture_resource;

	if (FAILED(device_->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE,
		&tex_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(texture_resource.release_and_get()))))
	{
		return nullptr;
	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = { };
	cstd::uint64_t total_bytes = 0;
	device_->GetCopyableFootprints(&tex_desc, 0, 1, 0, &footprint, nullptr, nullptr, &total_bytes);

	dx12_object<ID3D12Resource> upload_resource;

	if (!create_upload_buffer(device_.value(), total_bytes, upload_resource.release_and_get()))
	{
		return nullptr;
	}

	void* mapped = nullptr;
	D3D12_RANGE read_range = { 0, 0 };
	upload_resource->Map(0, &read_range, &mapped);

	const cstd::uint8_t* src = buffer.data();
	auto* dst = static_cast<cstd::uint8_t*>(mapped) + footprint.Offset;
	const cstd::uint32_t src_row_pitch = width * 4;

	for (cstd::uint32_t y = 0; y < height; ++y)
	{
		cstd::memcpy(dst + y * footprint.Footprint.RowPitch, src + y * src_row_pitch, src_row_pitch);
	}

	upload_resource->Unmap(0, nullptr);

	wait_for_gpu();

	command_allocators_[0]->Reset();

	dx12_object<ID3D12GraphicsCommandList> upload_cmd;

	if (FAILED(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		command_allocators_[0].value(), nullptr, IID_PPV_ARGS(upload_cmd.release_and_get()))))
	{
		return nullptr;
	}

	D3D12_TEXTURE_COPY_LOCATION dst_loc = { };
	dst_loc.pResource = texture_resource.value();
	dst_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst_loc.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION src_loc = { };
	src_loc.pResource = upload_resource.value();
	src_loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src_loc.PlacedFootprint = footprint;

	upload_cmd->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, nullptr);

	D3D12_RESOURCE_BARRIER barrier = { };
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = texture_resource.value();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	upload_cmd->ResourceBarrier(1, &barrier);

	upload_cmd->Close();

	ID3D12CommandList* cmd_lists[] = { upload_cmd.value() };
	command_queue_->ExecuteCommandLists(1, cmd_lists);

	wait_for_gpu();

	const cstd::uint32_t srv_idx = allocate_srv();

	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = srv_heap_->GetCPUDescriptorHandleForHeapStart();
	cpu_handle.ptr += static_cast<cstd::uint64_t>(srv_idx) * srv_descriptor_size_;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = { };
	srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.Texture2D.MipLevels = 1;

	device_->CreateShaderResourceView(texture_resource.value(), &srv_desc, cpu_handle);

	return cstd::make_shared<dx12_texture>(this, cstd::move(texture_resource), srv_idx);
}

shared_ptr_t<rv::texture> rv::dx12_renderer::create_texture_from_srv(void* raw_srv)
{
	if (!raw_srv)
	{
		return { };
	}

	dx12_object<ID3D12Resource> empty_resource;
	return cstd::make_shared<dx12_texture>(this, cstd::move(empty_resource), 0);
}

cstd::uint32_t rv::dx12_renderer::allocate_srv() noexcept
{
	return srv_heap_count_++;
}

void rv::dx12_renderer::wait_for_gpu() noexcept
{
	const cstd::uint64_t current_fence = fence_value_;
	command_queue_->Signal(fence_.value(), current_fence);
	fence_value_++;

	if (fence_->GetCompletedValue() < current_fence)
	{
		fence_->SetEventOnCompletion(current_fence, fence_event_);
		WaitForSingleObjectEx(fence_event_, INFINITE, FALSE);
	}
}

void rv::dx12_renderer::move_to_next_frame() noexcept
{
	const cstd::uint64_t current_fence = fence_value_;
	command_queue_->Signal(fence_.value(), current_fence);
	fence_values_[frame_index_] = current_fence;
	fence_value_++;

	frame_index_ = (frame_index_ + 1) % frame_count;

	const cstd::uint64_t wait_value = fence_values_[frame_index_];
	if (wait_value != 0 && fence_->GetCompletedValue() < wait_value)
	{
		fence_->SetEventOnCompletion(wait_value, fence_event_);
		WaitForSingleObjectEx(fence_event_, INFINITE, FALSE);
	}
}
