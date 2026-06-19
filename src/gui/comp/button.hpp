#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../text_metrics.hpp"
#include "../../util/string.hpp"

namespace rv
{
	class button final : public element
	{
	public:
		button() noexcept = default;

		explicit button(const element_size size) noexcept
			: element(size)
		{
			init_defaults();
		}

		button(const element_size size, shared_ptr_t<gui_font> font) noexcept
			: element(size), font_(cstd::move(font))
		{
			init_defaults();
		}

		button& text(const string_view_t text)
		{
			text_ = string_t(text);

			return *this;
		}

		button& text_alignment(const text_align align) noexcept
		{
			text_align_ = align;

			return *this;
		}

		// optional accent stripe on the left edge (e.g. the active item in a sidebar nav)
		button& accent(const color col, const float thickness = 3.f) noexcept
		{
			accent_color_ = col;
			accent_thickness_ = thickness;

			return *this;
		}

		// left inset for a left-aligned label, so it clears the accent / reads as a nav indent
		button& label_indent(const float px) noexcept
		{
			label_indent_ = px;

			return *this;
		}

		// kept for API compatibility; both now route into the generic per-state styling system.
		button& hover_color(const color col) noexcept
		{
			background_color(col, element_state::hovered);

			return *this;
		}

		button& pressed_color(const color col) noexcept
		{
			background_color(col, element_state::pressed);

			return *this;
		}

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

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override
		{
			if (!font_ || text_.empty())
			{
				return { 0.f, 0.f };
			}

			const float scale = style_.font_size.value_or(0.f) > 0.f ? style_.font_size.value_or(0.f) / font_->baked_size() : 1.f;
			const float line_h = font_->line_height() * scale;
			const float text_w = measure_text_width(*font_, text_, scale);

			return { text_w + line_h * 0.8f, line_h + line_h * 0.4f };
		}

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			// accent stripe sits at the (content-box) left edge; nav buttons carry no padding,
			// so that is also the button's outer edge.
			if (accent_thickness_ > 0.f && accent_color_.a > 0.001f)
			{
				renderer.draw_rect_filled(min, { min.x + accent_thickness_, max.y }, accent_color_, 0.f);
			}

			if (!font_ || text_.empty())
			{
				return;
			}

			const float scale = style_.font_size.value_or(0.f) > 0.f ? style_.font_size.value_or(0.f) / font_->baked_size() : 1.f;
			const float line_h = font_->line_height() * scale;
			const float text_w = measure_text_width(*font_, text_, scale);

			const float box_w = max.x - min.x;
			const float box_h = max.y - min.y;

			float x;

			switch (text_align_)
			{
			case text_align::left:
				x = min.x + label_indent_;
				break;
			case text_align::right:
				x = max.x - text_w - label_indent_;
				break;
			default:
				x = min.x + (box_w - text_w) * 0.5f;
				break;
			}

			const float y = min.y + (box_h - line_h) * 0.5f;

			renderer.draw_text(*font_, { x, y }, text_, visual_text_color_, style_.font_size.value_or(0.f));
		}

	private:
		void init_defaults() noexcept
		{
			style_.background_color = color{ 0.18f, 0.18f, 0.22f, 1.f };
			style_.hover  = state_style{ .background_color = color{ 0.28f, 0.28f, 0.34f, 1.f } };
			style_.active = state_style{ .background_color = color{ 0.12f, 0.12f, 0.15f, 1.f } };
			style_.rounding = 6.f;
			style_.text_color = color{ 1.f, 1.f, 1.f, 1.f };
			style_.transition_speed = 12.f;
		}

		shared_ptr_t<gui_font> font_;
		string_t text_;

		function_t<void()> on_click_;

		text_align text_align_ = text_align::center;
		color accent_color_ = { 0.f, 0.f, 0.f, 0.f };
		float accent_thickness_ = 0.f;
		float label_indent_ = 0.f;
	};
}
