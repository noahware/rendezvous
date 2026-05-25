#pragma once
#include "../element.hpp"
#include "../gui.hpp"
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

		button& hover_color(const color col) noexcept
		{
			hover_color_ = col;

			return *this;
		}

		button& pressed_color(const color col) noexcept
		{
			pressed_color_ = col;

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

		void update(const float dt) override
		{
			const auto resting = style_.background_color;

			if (pressed_)
			{
				style_.background_color = pressed_color_;
			}
			else if (hovered_)
			{
				style_.background_color = hover_color_;
			}

			element::update(dt);

			style_.background_color = resting;
		}

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override
		{
			if (!font_ || text_.empty())
			{
				return { 0.f, 0.f };
			}

			const float scale = style_.font_size.value_or(0.f) > 0.f ? style_.font_size.value_or(0.f) / font_->baked_size() : 1.f;
			const float line_h = font_->line_height() * scale;
			const float text_w = measure_text_width(scale);

			return { text_w + line_h * 0.8f, line_h + line_h * 0.4f };
		}

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			if (!font_ || text_.empty())
			{
				return;
			}

			const float scale = style_.font_size.value_or(0.f) > 0.f ? style_.font_size.value_or(0.f) / font_->baked_size() : 1.f;
			const float line_h = font_->line_height() * scale;
			const float text_w = measure_text_width(scale);

			const float box_w = max.x - min.x;
			const float box_h = max.y - min.y;

			const float x = min.x + (box_w - text_w) * 0.5f;
			const float y = min.y + (box_h - line_h) * 0.5f;

			renderer.draw_text(*font_, { x, y }, text_, visual_text_color_, style_.font_size.value_or(0.f));
		}

	private:
		void init_defaults() noexcept
		{
			style_.background_color = color{ 0.18f, 0.18f, 0.22f, 1.f };
			style_.rounding = 6.f;
			style_.text_color = color{ 1.f, 1.f, 1.f, 1.f };
			style_.transition_speed = 12.f;
		}

		[[nodiscard]] float measure_text_width(const float scale) const noexcept
		{
			float width = 0.f;
			cstd::uint32_t prev_cp = 0;

			const char* s = text_.data();
			const char* end = s + text_.size();

			while (s < end)
			{
				const cstd::uint32_t cp = decode_utf8(s, end);

				if (prev_cp != 0)
				{
					width += font_->kerning(prev_cp, cp) * scale;
				}

				width += font_->glyph_advance(cp) * scale;
				prev_cp = cp;
			}

			return width;
		}

		shared_ptr_t<gui_font> font_;
		string_t text_;

		color hover_color_ = { 0.28f, 0.28f, 0.34f, 1.f };
		color pressed_color_ = { 0.12f, 0.12f, 0.15f, 1.f };

		function_t<void()> on_click_;
	};
}
