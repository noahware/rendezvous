#pragma once
#include "../element.hpp"

namespace rv
{
	class separator final : public element
	{
	public:
		separator() noexcept = default;

		explicit separator(const element_size size) noexcept
			: element(size)
		{
		}

		separator& thickness(const float px) noexcept
		{
			thickness_ = px;
			mark_layout_dirty();

			return *this;
		}

		separator& line_color(const color col) noexcept
		{
			line_color_ = col;

			return *this;
		}

		separator& inset(const float px) noexcept
		{
			inset_ = px;

			return *this;
		}

		separator& vertical(const bool v) noexcept
		{
			vertical_ = v;
			mark_layout_dirty();

			return *this;
		}

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override
		{
			if (vertical_.has_value() && *vertical_)
			{
				return { thickness_, 0.f };
			}

			return { 0.f, thickness_ };
		}

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			const bool vert = vertical_.has_value() && *vertical_;

			if (vert)
			{
				const float cx = (min.x + max.x) * 0.5f;
				renderer.draw_rect_filled(
					{ cx - thickness_ * 0.5f, min.y + inset_ },
					{ cx + thickness_ * 0.5f, max.y - inset_ },
					line_color_);
			}
			else
			{
				const float cy = (min.y + max.y) * 0.5f;
				renderer.draw_rect_filled(
					{ min.x + inset_, cy - thickness_ * 0.5f },
					{ max.x - inset_, cy + thickness_ * 0.5f },
					line_color_);
			}
		}

	private:
		float thickness_ = 1.f;
		float inset_ = 0.f;
		color line_color_{ 0.3f, 0.3f, 0.35f, 1.f };
		optional_t<bool> vertical_;
	};
}
