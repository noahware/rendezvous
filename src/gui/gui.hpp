#pragma once
#include "../render/render.hpp"

namespace rv
{
	class gui_renderer
	{
	public:
		virtual void draw_rect(position min, position max, color col, float thickness = 1.f, float rounding = 0.f) noexcept = 0;
		virtual void draw_rect_filled(position min, position max, color col, float rounding = 0.f, rounding_flags flags = rounding_flags_all) noexcept = 0;
	};

	class gui_renderer_impl : public gui_renderer
	{
	public:
		explicit gui_renderer_impl(shared_ptr_t<renderer> renderer)
				:	renderer_(cstd::move(renderer)) { }

		void draw_rect(const position min, const position max, const color col, const float thickness,
		               const float rounding) noexcept override
		{
			return renderer_->draw_rect(min, max, col, thickness, rounding);
		}

		void draw_rect_filled(const position min, const position max, const color col, const float rounding,
		                      const rounding_flags flags) noexcept override
		{
			return renderer_->draw_rect_filled(min, max, col, rounding, flags);
		}

	protected:
		shared_ptr_t<renderer> renderer_;
	};

	class gui
	{
	public:
		explicit gui(unique_ptr_t<gui_renderer> renderer)
				:	renderer_(cstd::move(renderer)) { }

		void draw_button(const position min, const position max, const color col, const float rounding) noexcept
		{
			renderer_->draw_rect_filled(min, max, col, rounding);
		}

	protected:
		unique_ptr_t<gui_renderer> renderer_;
	};
}
