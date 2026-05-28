#pragma once
#include "../render.hpp"
#include "../texture.hpp"
#include <d3d12.h>
#include <dxgi1_4.h>

namespace rv
{
	class renderer;

	constexpr cstd::uint32_t frame_count = 2;

	template <class T>
	class dx12_object
	{
	public:
		dx12_object() noexcept = default;

		explicit dx12_object(T* value_ptr) noexcept
				:	value_(value_ptr) { }

		dx12_object(const dx12_object&) = delete;
		dx12_object& operator=(const dx12_object&) = delete;

		dx12_object(dx12_object&& other) noexcept
				:	value_(other.value_)
		{
			other.value_ = nullptr;
		}

		dx12_object& operator=(dx12_object&& other) noexcept
		{
			if (this != &other)
			{
				release();

				value_ = other.value_;
				other.value_ = nullptr;
			}

			return *this;
		}

		[[nodiscard]] T* value() const noexcept
		{
			return value_;
		}

		[[nodiscard]] T* operator->() const noexcept
		{
			return value_;
		}

		explicit operator bool() const noexcept
		{
			return value_ != nullptr;
		}

		void release()
		{
			if (value_)
			{
				value_->Release();

				value_ = nullptr;
			}
		}

		[[nodiscard]] T** release_and_get() noexcept
		{
			release();

			return &value_;
		}

		~dx12_object()
		{
			release();
		}

	protected:
		T* value_ = nullptr;
	};

	class dx12_texture : public texture
	{
	public:
		explicit dx12_texture(renderer* const renderer, dx12_object<ID3D12Resource> resource,
		                      cstd::uint32_t srv_index)
				:	texture(renderer),
					resource_(cstd::move(resource)),
					srv_index_(srv_index) { }

		[[nodiscard]] cstd::uint32_t srv_index() const noexcept
		{
			return srv_index_;
		}

		[[nodiscard]] ID3D12Resource* resource() const noexcept
		{
			return resource_.value();
		}

	protected:
		dx12_object<ID3D12Resource> resource_;
		cstd::uint32_t srv_index_ = 0;
	};

	class dx12_renderer : public renderer
	{
	public:
		explicit dx12_renderer(ID3D12Device* device, ID3D12CommandQueue* command_queue) noexcept;

		[[nodiscard]] ID3D12GraphicsCommandList* command_list() const noexcept
		{
			return command_list_.value();
		}

		void end_frame() noexcept override;
		shared_ptr_t<texture> create_texture(span_t<const cstd::uint8_t> buffer, cstd::uint32_t width, cstd::uint32_t height) override;
		shared_ptr_t<texture> create_texture_from_srv(void* srv) override;

	protected:
		bool create_buffer(cstd::size_t vertex_count);
		bool try_widen_buffer();
		
		bool create_index_buffer(cstd::size_t index_count);
		bool try_widen_index_buffer();

		bool init_backend() noexcept override;
		void begin_frame_backend(vector_2d<float> display_size) noexcept override;
		void flush_pending_vertices() noexcept override;

		bool create_root_signature() noexcept;
		bool create_pipeline_state(const span_t<const cstd::byte>& vs_bytecode,
		                           const span_t<const cstd::byte>& ps_bytecode,
		                           ID3D12PipelineState** out_pso) noexcept;

		void wait_for_gpu() noexcept;
		void move_to_next_frame() noexcept;

		cstd::uint32_t allocate_srv() noexcept;

		dx12_object<ID3D12Device> device_;
		dx12_object<ID3D12CommandQueue> command_queue_;
		dx12_object<ID3D12CommandAllocator> command_allocators_[frame_count];
		dx12_object<ID3D12GraphicsCommandList> command_list_;

		dx12_object<ID3D12RootSignature> root_signature_;

		dx12_object<ID3D12PipelineState> default_pso_;
		dx12_object<ID3D12PipelineState> shadow_pso_;
		dx12_object<ID3D12PipelineState> rect_pso_;
		dx12_object<ID3D12PipelineState> image_pso_;
		dx12_object<ID3D12PipelineState> text_shadow_pso_;

		dx12_object<ID3D12DescriptorHeap> srv_heap_;
		cstd::uint32_t srv_heap_capacity_ = 0;
		cstd::uint32_t srv_heap_count_ = 0;
		cstd::uint32_t srv_descriptor_size_ = 0;

		dx12_object<ID3D12Resource> vertex_upload_[frame_count];
		dx12_object<ID3D12Resource> index_upload_[frame_count];
		void* vertex_mapped_[frame_count] = { };
		void* index_mapped_[frame_count] = { };

		dx12_object<ID3D12Fence> fence_;
		cstd::uint64_t fence_value_ = 0;
		cstd::uint64_t fence_values_[frame_count] = { };
		HANDLE fence_event_ = nullptr;

		cstd::uint32_t frame_index_ = 0;
	};
}
