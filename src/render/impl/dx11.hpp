#pragma once
#include "../render.hpp"
#include "../texture.hpp"

#if defined(_WIN32)
#include <d3d11.h>

namespace rv
{
	class renderer;

	template <class T>
	class dx11_object
	{
	public:
		dx11_object() noexcept = default;

		explicit dx11_object(T* value_ptr) noexcept
				:	value_(value_ptr) { }

		dx11_object(const dx11_object&) = delete;
		dx11_object& operator=(const dx11_object&) = delete;

		dx11_object(dx11_object&& other) noexcept
				:	value_(other.value_)
		{
			other.value_ = nullptr;
		}

		dx11_object& operator=(dx11_object&& other) noexcept
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

		~dx11_object()
		{
			release();
		}

	protected:
		T* value_ = nullptr;
	};

	class dx11_texture : public texture
	{
	public:
		explicit dx11_texture(renderer* const renderer, dx11_object<ID3D11Texture2D> texture_2d,
		                      dx11_object<ID3D11ShaderResourceView> shader_resource,
		                      const cstd::uint32_t width = 0, const cstd::uint32_t height = 0)
				:	texture(renderer, width, height),
					texture_2d_(cstd::move(texture_2d)),
					shader_resource_(cstd::move(shader_resource)) { }

		[[nodiscard]] ID3D11ShaderResourceView* shader_resource() const noexcept
		{
			return shader_resource_.value();
		}

		[[nodiscard]] ID3D11Texture2D* texture_2d() const noexcept
		{
			return texture_2d_.value();
		}

	protected:
		dx11_object<ID3D11Texture2D> texture_2d_;
		dx11_object<ID3D11ShaderResourceView> shader_resource_;
	};

	class dx11_render_target : public dx11_texture
	{
	public:
		explicit dx11_render_target(renderer* const renderer, dx11_object<ID3D11Texture2D> texture_2d,
		                            dx11_object<ID3D11ShaderResourceView> shader_resource,
		                            dx11_object<ID3D11RenderTargetView> rtv,
		                            const cstd::uint32_t width, const cstd::uint32_t height)
				:	dx11_texture(renderer, cstd::move(texture_2d), cstd::move(shader_resource), width, height),
					rtv_(cstd::move(rtv)) { }

		[[nodiscard]] ID3D11RenderTargetView* render_target_view() const noexcept
		{
			return rtv_.value();
		}

	private:
		dx11_object<ID3D11RenderTargetView> rtv_;
	};

	class dx11_renderer : public renderer
	{
	public:
		explicit dx11_renderer(ID3D11Device* device, ID3D11DeviceContext* context) noexcept;
		explicit dx11_renderer(IDXGISwapChain* swap_chain) noexcept;

		void end_frame() noexcept override;
		shared_ptr_t<texture> create_texture(span_t<const cstd::uint8_t> buffer, cstd::uint32_t width, cstd::uint32_t height) override;
		shared_ptr_t<texture> create_texture_from_srv(void* srv) override;

	protected:
		bool create_sampler() noexcept;

		bool create_buffer(cstd::size_t vertex_count);
		bool try_widen_buffer();
		
		bool create_index_buffer(cstd::size_t index_count);
		bool try_widen_index_buffer();

		bool init_backend() noexcept override;
		void begin_frame_backend(vector_2d<float> display_size) noexcept override;
		void flush_pending_vertices() noexcept override;

		shared_ptr_t<texture> create_render_target(cstd::uint32_t width, cstd::uint32_t height) override;
		void capture_backbuffer_region(const shared_ptr_t<texture>& dst, cstd::uint32_t x, cstd::uint32_t y, cstd::uint32_t w, cstd::uint32_t h) override;
		void set_render_target(const shared_ptr_t<texture>& target) override;
		void reset_render_target() override;

		dx11_object<ID3D11Device> device_;
		dx11_object<ID3D11DeviceContext> context_;

		dx11_object<ID3D11SamplerState> sampler_state_;
		dx11_object<ID3D11PixelShader> pixel_shader_;
		dx11_object<ID3D11PixelShader> shadow_pixel_shader_;
		dx11_object<ID3D11PixelShader> rect_pixel_shader_;
		dx11_object<ID3D11PixelShader> image_pixel_shader_;
		dx11_object<ID3D11PixelShader> text_shadow_pixel_shader_;
		dx11_object<ID3D11PixelShader> blur_pixel_shader_;
		dx11_object<ID3D11VertexShader> vertex_shader_;
		dx11_object<ID3D11InputLayout> input_layout_;
		dx11_object<ID3D11RasterizerState> rasterizer_state_;
		dx11_object<ID3D11BlendState> blend_state_;
		dx11_object<ID3D11Buffer> buffer_;
		dx11_object<ID3D11Buffer> index_buffer_;
		dx11_object<ID3D11Buffer> clip_cbuffer_;

		dx11_object<ID3D11RenderTargetView> saved_rtv_;
		D3D11_VIEWPORT saved_viewport_ = {};
	};
}

#endif // _WIN32
