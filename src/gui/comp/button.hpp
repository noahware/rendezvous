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

		button& on_click(function_t<void()> callback)
		{
			on_click_ = cstd::move(callback);

			return *this;
		}

		bool on_mouse_click() override
		{
			if (on_click_)
			{
				on_click_();
			}

			return true;
		}

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			color col = hovered_
				? color{ 0.8f, 0.8f, 0.8f, 1.f }
				: color{ 1.f, 1.f, 1.f, 1.f };

			float round = 0.f;

			if (const auto anim = animated_props())
			{
				if (anim->col)
				{
					col = *anim->col;
				}

				if (anim->opacity)
				{
					col.a *= *anim->opacity;
				}

				if (anim->rounding)
				{
					round = *anim->rounding;
				}
			}

			renderer.draw_rect_filled(min, max, col, round);
		}

		function_t<void()> on_click_;
	};
}
