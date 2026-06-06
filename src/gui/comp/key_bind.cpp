#include "key_bind.hpp"



namespace rv

{

	bool key_bind::on_mouse_click()

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



	void key_bind::update(const float dt)

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



		// key_bind drives its colour target by swapping style_ around element::update, so it must
		// never be frozen by the settled fast-path; force a re-resolve every frame.
		mark_unsettled();
		element::update(dt);



		style_.background_color = resting_bg;

		style_.border_color = resting_border;

	}



	vector_2d<float> key_bind::content_size(const vector_2d<float> available) const noexcept

	{

		if (!font_)

		{

			return { 0.f, 0.f };

		}



		const float scale = current_scale();

		const float line_h = font_->line_height() * scale;

		const float text_w = measure_text_width(listening_ ? "..." : key_display_name(key_code_), scale);



		return { text_w, line_h };

	}



	void key_bind::render_self(gui_renderer& renderer, const position min, const position max) const

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



	float key_bind::measure_text_width(const string_view_t text, const float scale) const noexcept

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



	void key_bind::commit(const key new_key)

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



	void key_bind::scan_keys()

	{

		// keyboard scan

		for (cstd::int32_t i = 0; i < input_state::key_count; ++i)

		{

			if (!input_->is_key_pressed(i))

			{

				continue;

			}



			// skip mouse button VK codes, those are handled below via the mouse arrays

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

}

