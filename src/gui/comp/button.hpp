#pragma once
#include "../element.hpp"
#include "../../log/log.hpp"

namespace rv
{
	class button final : public element
	{
	public:
		button() noexcept = default;

		explicit button(const struct position position, const vector_2d<float> size, shared_ptr_t<element> parent = { }) noexcept
				:	element(position, size, cstd::move(parent)) { }

		void on_mouse_click() override
		{
			LOG_INFO("click");
		}

	protected:
		void render_self(gui_renderer& renderer, const rv::position position) const override
		{
			const auto min = position + position_;
			const auto max = min + size_;

			constexpr color col = { 1.f, 1.f, 1.f, 1.f };

			renderer.draw_rect_filled({ min.x, min.y }, { max.x, max.y }, col);
		}
	};
}
