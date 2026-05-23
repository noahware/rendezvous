#pragma once
#include "../render/render.hpp"

namespace rv
{
	class gui_renderer
	{
	public:
		virtual void draw_rect(position min, position max, color col, float thickness = 1.f, float rounding = 0.f) noexcept = 0;
	};

	class gui_renderer_impl : public gui_renderer
	{
	public:
		void draw_rect(const position min, const position max, const color col, const float thickness, const float rounding) noexcept override
		{
			return renderer_->draw_rect(min, max, col, thickness, rounding);
		}

	protected:
		shared_ptr_t<renderer> renderer_;
	};

	class gui
	{
	public:


	protected:
		unique_ptr_t<gui_renderer> renderer_;
	};
}
