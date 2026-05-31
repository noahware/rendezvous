#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../../input/input.hpp"
#include "../../input/key_names.hpp"
#include "../../util/string.hpp"

namespace rv
{
	class key_bind final : public element
	{
	public:
		key_bind() noexcept = default;

		explicit key_bind(const element_size size) noexcept
			: element(size)
		{
			init_defaults();
		}

		key_bind(const element_size size, shared_ptr_t<gui_font> font, shared_ptr_t<input> input) noexcept
			: element(size), font_(cstd::move(font)), input_(cstd::move(input))
		{
			init_defaults();
		}

		key_bind& value(const key k) noexcept
		{
			key_code_ = k;

			if (bound_)
			{
				*bound_ = key_code_;
			}

			return *this;
		}

		[[nodiscard]] key current_key() const noexcept
		{
			return key_code_;
		}

		key_bind& bind(key* ptr) noexcept
		{
			bound_ = ptr;

			if (bound_)
			{
				key_code_ = *bound_;
			}

			return *this;
		}

		key_bind& on_change(function_t<void(key)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		key_bind& hover_color(const color col) noexcept
		{
			hover_color_ = col;

			return *this;
		}

		key_bind& pressed_color(const color col) noexcept
		{
			pressed_color_ = col;

			return *this;
		}

		key_bind& listening_color(const color col) noexcept
		{
			listening_color_ = col;

			return *this;
		}

		key_bind& listening_border_color(const color col) noexcept
		{
			listening_border_ = col;

			return *this;
		}

		[[nodiscard]] bool focusable() const noexcept override
		{
			return true;
		}

		bool on_mouse_click() override
		{
			if (listening_)
			{
				commit(key::mouse_left);
			}
			else
			{
				prev_key_code_ = key_code_;
				listening_ = true;
			}

			return true;
		}

		void update(const float dt) override
		{
			if (hovered_)
			{
				input_->set_cursor(cursor_type::hand);
			}

			if (!listening_)
			{
				if (bound_ && *bound_ != key_code_)
				{
					key_code_ = *bound_;
				}
			}
			else if (!is_focused())
			{
				cancel();
			}
			else
			{
				scan_keys();
			}

			const auto resting_bg = style_.background_color;
			const auto resting_border = style_.border_color;

			if (listening_)
			{
				style_.background_color = listening_color_;
				style_.border_color = listening_border_;
			}
			else if (pressed_)
			{
				style_.background_color = pressed_color_;
			}
			else if (hovered_)
			{
				style_.background_color = hover_color_;
			}

			element::update(dt);

			style_.background_color = resting_bg;
			style_.border_color = resting_border;
		}

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override
		{
			if (!font_)
			{
				return { 0.f, 0.f };
			}

			const float scale = current_scale();
			const float line_h = font_->line_height() * scale;

			return { 0.f, line_h };
		}

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			if (!font_)
			{
				return;
			}

			const string_view_t display = listening_ ? "..." : key_display_name(key_code_);

			const float scale = current_scale();
			const float text_w = measure_text_width(display, scale);
			const float line_h = font_->line_height() * scale;

			const float box_w = max.x - min.x;
			const float box_h = max.y - min.y;

			const float x = min.x + (box_w - text_w) * 0.5f;
			const float glyph_h = (font_->ascent() - font_->descent()) * scale;
			const float y = min.y + (box_h - glyph_h) * 0.5f;

			renderer.draw_text(*font_, { x, y }, display, visual_text_color_, style_.font_size.value_or(0.f));
		}

	private:
		void init_defaults() noexcept
		{
			style_.background_color = color{ 0.12f, 0.12f, 0.15f, 1.f };
			style_.rounding = 6.f;
			style_.border_color = color{ 0.3f, 0.3f, 0.36f, 1.f };
			style_.border_width = border_vector{ 1.f, 1.f, 1.f, 1.f };
			style_.text_color = color{ 1.f, 1.f, 1.f, 1.f };
			style_.padding = border_vector{ 6.f, 10.f, 6.f, 10.f };
			style_.transition_speed = 12.f;
		}

		[[nodiscard]] float current_scale() const noexcept
		{
			const float font_size = style_.font_size.value_or(0.f);

			return (font_size > 0.f && font_) ? font_size / font_->baked_size() : 1.f;
		}

		[[nodiscard]] float measure_text_width(const string_view_t text, const float scale) const noexcept
		{
			float width = 0.f;
			cstd::uint32_t prev_cp = 0;

			const char* s = text.data();
			const char* end = s + text.size();

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

		void commit(const key new_key)
		{
			key_code_ = new_key;
			listening_ = false;

			if (bound_)
			{
				*bound_ = key_code_;
			}

			if (on_change_)
			{
				on_change_(key_code_);
			}
		}

		void cancel() noexcept
		{
			key_code_ = prev_key_code_;
			listening_ = false;
		}

		void scan_keys()
		{
			// keyboard scan
			for (cstd::int32_t i = 0; i < input_state::key_count; ++i)
			{
				if (!input_->is_key_pressed(i))
				{
					continue;
				}

				// skip mouse button VK codes — those are handled below via the mouse arrays
				if (i >= 0x01 && i <= 0x06)
				{
					continue;
				}

				if (i == static_cast<cstd::int32_t>(key::escape))
				{
					cancel();
					return;
				}

				if (i == static_cast<cstd::int32_t>(key::del) ||
					i == static_cast<cstd::int32_t>(key::backspace))
				{
					commit(key::none);
					return;
				}

				commit(static_cast<key>(i));
				return;
			}

			// mouse button scan (left click is handled via on_mouse_click)
			constexpr key mouse_buttons[] = { key::mouse_right, key::mouse_middle, key::mouse_4, key::mouse_5 };
			constexpr input::button_type mouse_indices[] = { 1, 2, 3, 4 };

			for (cstd::int32_t b = 0; b < 4; ++b)
			{
				if (input_->is_mouse_clicked(mouse_indices[b]))
				{
					commit(mouse_buttons[b]);
					return;
				}
			}
		}

		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;

		key key_code_ = key::none;
		key prev_key_code_ = key::none;
		bool listening_ = false;

		key* bound_ = nullptr;
		function_t<void(key)> on_change_;

		color hover_color_ = { 0.22f, 0.22f, 0.26f, 1.f };
		color pressed_color_ = { 0.1f, 0.1f, 0.12f, 1.f };
		color listening_color_ = { 0.15f, 0.20f, 0.30f, 1.f };
		color listening_border_ = { 0.35f, 0.55f, 0.85f, 1.f };
	};
}
