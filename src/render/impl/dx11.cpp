#include "dx11.hpp"
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
}

rv::dx11_renderer::dx11_renderer(ID3D11Device* const device, ID3D11DeviceContext* const context) noexcept
		:	device_(device),
			context_(context)
{
	device_->AddRef();
	context_->AddRef();
}

rv::dx11_renderer::dx11_renderer(IDXGISwapChain* swap_chain) noexcept
{
	if (swap_chain->GetDevice(IID_PPV_ARGS(device_.release_and_get())) == S_OK)
	{
		device_->GetImmediateContext(context_.release_and_get());
	}
}

bool rv::dx11_renderer::init_backend() noexcept
{
	if (!device_ || !context_)
	{
		return false;
	}

	if (FAILED(device_->CreateVertexShader(d3d11_vertex_shader.data(), d3d11_vertex_shader.size(), nullptr, vertex_shader_.release_and_get())) ||
		FAILED(device_->CreatePixelShader(d3d11_pixel_shader.data(), d3d11_pixel_shader.size(), nullptr, pixel_shader_.release_and_get())) ||
		FAILED(device_->CreatePixelShader(d3d11_shadow_pixel_shader.data(), d3d11_shadow_pixel_shader.size(), nullptr, shadow_pixel_shader_.release_and_get())) ||
		FAILED(device_->CreatePixelShader(d3d11_rect_pixel_shader.data(), d3d11_rect_pixel_shader.size(), nullptr, rect_pixel_shader_.release_and_get())) ||
		FAILED(device_->CreatePixelShader(d3d11_image_pixel_shader.data(), d3d11_image_pixel_shader.size(), nullptr, image_pixel_shader_.release_and_get()))) {
		return false;
	}

	static constexpr array_t<D3D11_INPUT_ELEMENT_DESC, 5> layout =
	{
		D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, offsetof(vertex, pos),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		D3D11_INPUT_ELEMENT_DESC{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(vertex, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(vertex, uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(vertex, custom_data), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(vertex, custom_data) + 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	if (FAILED(device_->CreateInputLayout(layout.data(), static_cast<cstd::uint32_t>(layout.size()), d3d11_vertex_shader.data(), d3d11_vertex_shader.size(), input_layout_.release_and_get())))
	{
		return false;
	}

	D3D11_BLEND_DESC blend_desc = { };

	auto& render_target = blend_desc.RenderTarget[0];

	render_target.BlendEnable = true;
	render_target.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	render_target.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	render_target.BlendOp = D3D11_BLEND_OP_ADD;
	render_target.SrcBlendAlpha = D3D11_BLEND_ONE;
	render_target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	render_target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	render_target.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (FAILED(device_->CreateBlendState(&blend_desc, blend_state_.release_and_get())))
	{
		return false;
	}

	D3D11_RASTERIZER_DESC rasterizer_desc = { };
	rasterizer_desc.FillMode = D3D11_FILL_SOLID;
	rasterizer_desc.CullMode = D3D11_CULL_BACK;
	rasterizer_desc.ScissorEnable = TRUE;
	rasterizer_desc.DepthClipEnable = TRUE;

	if (FAILED(device_->CreateRasterizerState(&rasterizer_desc, rasterizer_state_.release_and_get())))
	{
		return false;
	}

	D3D11_BUFFER_DESC cb_desc = { };
	cb_desc.ByteWidth = sizeof(clip_cbuffer_data);
	cb_desc.Usage = D3D11_USAGE_DYNAMIC;
	cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	if (FAILED(device_->CreateBuffer(&cb_desc, nullptr, clip_cbuffer_.release_and_get())))
	{
		return false;
	}

	return create_sampler();
}

bool rv::dx11_renderer::create_buffer(const cstd::size_t vertex_count)
{
	D3D11_BUFFER_DESC buffer_desc = { };

	buffer_desc.ByteWidth = sizeof(vertex) * static_cast<cstd::uint32_t>(vertex_count);
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	if (FAILED(device_->CreateBuffer(&buffer_desc, nullptr, buffer_.release_and_get())))
	{
		return false;
	}

	buffer_vertex_count_ = vertex_count;

	return true;
}

bool rv::dx11_renderer::try_widen_buffer()
{
	if (pending_vertices_.size() <= buffer_vertex_count_)
	{
		return true;
	}

	constexpr cstd::size_t additional_vertices = 256;

	return create_buffer(pending_vertices_.size() + additional_vertices);
}

bool rv::dx11_renderer::create_index_buffer(const cstd::size_t index_count)
{
	D3D11_BUFFER_DESC buffer_desc = { };

	buffer_desc.ByteWidth = sizeof(cstd::uint32_t) * static_cast<cstd::uint32_t>(index_count);
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	if (FAILED(device_->CreateBuffer(&buffer_desc, nullptr, index_buffer_.release_and_get())))
	{
		return false;
	}

	buffer_index_count_ = index_count;

	return true;
}

bool rv::dx11_renderer::try_widen_index_buffer()
{
	if (pending_indices_.size() <= buffer_index_count_)
	{
		return true;
	}

	constexpr cstd::size_t additional_indices = 512;

	return create_index_buffer(pending_indices_.size() + additional_indices);
}

void rv::dx11_renderer::begin_frame_backend(const vector_2d<float> display_size) noexcept
{
	D3D11_VIEWPORT viewport = { };

	viewport.Width = display_size.x;
	viewport.Height = display_size.y;
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;

	context_->RSSetViewports(1, &viewport);
	context_->RSSetState(rasterizer_state_.value());
	context_->IASetInputLayout(input_layout_.value());

	context_->VSSetShader(vertex_shader_.value(), nullptr, 0);
	context_->PSSetShader(pixel_shader_.value(), nullptr, 0);
	context_->GSSetShader(nullptr, nullptr, 0);
	context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11SamplerState* sampler = sampler_state_.value();
	constexpr array_t<float, 4> blend_factor = { };

	context_->OMSetBlendState(blend_state_.value(), blend_factor.data(), 0xffffffff);
	context_->PSSetSamplers(0, 1, &sampler);
}

void rv::dx11_renderer::end_frame() noexcept
{
	flush_pending_vertices();
}

bool rv::dx11_renderer::create_sampler() noexcept
{
	D3D11_SAMPLER_DESC desc = { };

	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	desc.MinLOD = 0.f;
	desc.MaxLOD = D3D11_FLOAT32_MAX;

	return SUCCEEDED(device_->CreateSamplerState(&desc, sampler_state_.release_and_get()));
}

void rv::dx11_renderer::flush_pending_vertices() noexcept 
{
	if (pending_vertices_.empty() || pending_indices_.empty()) 
	{
		return;
	}

	D3D11_MAPPED_SUBRESOURCE resource = { };

	if (!try_widen_buffer() || FAILED(context_->Map(buffer_.value(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource))) 
	{
		pending_vertices_.clear();
		pending_indices_.clear();
		return;
	}

	cstd::memcpy(resource.pData, pending_vertices_.data(), pending_vertices_.size() * sizeof(vertex));
	context_->Unmap(buffer_.value(), 0);

	D3D11_MAPPED_SUBRESOURCE idx_resource = { };

	if (!try_widen_index_buffer() || FAILED(context_->Map(index_buffer_.value(), 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource))) 
	{
		pending_vertices_.clear();
		pending_indices_.clear();
		return;
	}

	cstd::memcpy(idx_resource.pData, pending_indices_.data(), pending_indices_.size() * sizeof(cstd::uint32_t));
	context_->Unmap(index_buffer_.value(), 0);

	constexpr cstd::uint32_t strides = sizeof(vertex);
	constexpr cstd::uint32_t offsets = 0;

	ID3D11Buffer* buffer = buffer_.value();
	context_->IASetVertexBuffers(0, 1, &buffer, &strides, &offsets);

	ID3D11Buffer* idx_buffer = index_buffer_.value();
	context_->IASetIndexBuffer(idx_buffer, DXGI_FORMAT_R32_UINT, 0);

	for (const auto& batch : pending_batches_) 
	{
		const auto texture = std::static_pointer_cast<dx11_texture>(batch.texture);
		const auto shader = texture->shader_resource();

		if (batch.shader == shader_type::shadow_shader) 
		{
			context_->PSSetShader(shadow_pixel_shader_.value(), nullptr, 0);
		}
		else if (batch.shader == shader_type::rect_shader) 
		{
			context_->PSSetShader(rect_pixel_shader_.value(), nullptr, 0);
		}
		else if (batch.shader == shader_type::image_shader)
		{
			context_->PSSetShader(image_pixel_shader_.value(), nullptr, 0);
		}
		else 
		{
			context_->PSSetShader(pixel_shader_.value(), nullptr, 0);
		}

		context_->PSSetShaderResources(0, 1, &shader);

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

			D3D11_MAPPED_SUBRESOURCE cb_resource = { };
			if (SUCCEEDED(context_->Map(clip_cbuffer_.value(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cb_resource)))
			{
				cstd::memcpy(cb_resource.pData, &cb_data, sizeof(cb_data));
				context_->Unmap(clip_cbuffer_.value(), 0);
			}

			ID3D11Buffer* cb = clip_cbuffer_.value();
			context_->PSSetConstantBuffers(0, 1, &cb);
		}

		if (batch.clip_rect.has_value())
		{
			D3D11_RECT rect;
			rect.left = static_cast<LONG>(batch.clip_rect->bounds.min.x) - 1;
			rect.top = static_cast<LONG>(batch.clip_rect->bounds.min.y) - 1;
			rect.right = static_cast<LONG>(batch.clip_rect->bounds.max.x) + 1;
			rect.bottom = static_cast<LONG>(batch.clip_rect->bounds.max.y) + 1;
			context_->RSSetScissorRects(1, &rect);
		}
		else
		{
			D3D11_RECT rect;
			rect.left = 0;
			rect.top = 0;
			rect.right = static_cast<LONG>(state_.display_size.x);
			rect.bottom = static_cast<LONG>(state_.display_size.y);
			context_->RSSetScissorRects(1, &rect);
		}

		context_->DrawIndexed(batch.index_count, batch.index_offset, batch.vertex_offset);
	}

	pending_vertices_.clear();
	pending_indices_.clear();
	pending_batches_.clear();
}

shared_ptr_t<rv::texture> rv::dx11_renderer::create_texture(const span_t<const cstd::uint8_t> buffer,
                                                            const cstd::uint32_t width, const cstd::uint32_t height)
{
	if (buffer.empty() || !width || !height)
	{
		return { };
	}

	D3D11_TEXTURE2D_DESC desc = { };

	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA data = { };

	data.pSysMem = buffer.data();
	data.SysMemPitch = width * 4;

	dx11_object<ID3D11Texture2D> texture_2d = { };
	dx11_object<ID3D11ShaderResourceView> shader_resource = { };

	if (FAILED(device_->CreateTexture2D(&desc, &data, texture_2d.release_and_get())) ||
		FAILED(device_->CreateShaderResourceView(texture_2d.value(), nullptr, shader_resource.release_and_get())))
	{
		return nullptr;
	}

	return cstd::make_shared<dx11_texture>(this, cstd::move(texture_2d), cstd::move(shader_resource));
}

shared_ptr_t<rv::texture> rv::dx11_renderer::create_texture_from_srv(void* raw_srv)
{
	auto* srv = static_cast<ID3D11ShaderResourceView*>(raw_srv);
	if (!srv)
	{
		return { };
	}

	srv->AddRef();

	dx11_object<ID3D11Texture2D> empty_texture = { };
	dx11_object<ID3D11ShaderResourceView> shader_resource = { };

	*shader_resource.release_and_get() = srv;

	return cstd::make_shared<dx11_texture>(this, cstd::move(empty_texture), cstd::move(shader_resource));
}

