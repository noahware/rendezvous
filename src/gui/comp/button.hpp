#pragma once
#include "../element.hpp"
#include "../../log/log.hpp"

namespace rv
{
	class button final : public element
	{
	public:
		button() noexcept = default;

		explicit button(const element_size size) noexcept
				:	element(size) { }

		void on_mouse_click() override
		{
			LOG_INFO("click");
		}

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			constexpr color col = { 1.f, 1.f, 1.f, 1.f };

			renderer.draw_rect_filled(min, max, col);
		}
	};
}
