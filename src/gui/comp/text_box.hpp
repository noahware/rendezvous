#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../text_metrics.hpp"
#include "../../input/input.hpp"
#include "../../util/string.hpp"
#include "text_edit.hpp"

namespace rv
{
	class text_box final : public element
	{
	public:
		text_box(const element_size size, shared_ptr_t<gui_font> font, shared_ptr_t<input> input) noexcept
				:	element(size), font_(cstd::move(font)), input_(cstd::move(input))
		{
			init_defaults();
			te_init(stb_, !multiline_);
		}

		text_box& text(const string_view_t value)
		{
			set_text(value);

			return *this;
		}

		[[nodiscard]] const string_t& text() const noexcept
		{
			return text_;
		}

		text_box& bind(string_t* ptr)
		{
			bound_ = ptr;

			if (bound_)
			{
				set_text(*bound_);
			}

			return *this;
		}

		text_box& on_change(function_t<void(const string_t&)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		text_box& on_submit(function_t<void(const string_t&)> callback)
		{
			on_submit_ = cstd::move(callback);

			return *this;
		}

		text_box& multiline(const bool enabled)
		{
			multiline_ = enabled;
			te_init(stb_, !multiline_);

			return *this;
		}

		text_box& selection_color(const color col) noexcept
		{
			selection_color_ = col;

			return *this;
		}

		text_box& caret_color(const color col) noexcept
		{
			caret_color_ = col;

			return *this;
		}

		[[nodiscard]] bool focusable() const noexcept override
		{
			return true;
		}

		bool on_mouse_click() override
		{
			refresh_font_context();

			const position local = local_text_pos(input_->mouse_pos());
			te_click(buf_, stb_, local.x, local.y);

			dragging_ = true;
			blink_ = 0.f;

			return true;
		}

		void update(const float dt) override
		{
			refresh_font_context();

			if (hovered_)
			{
				input_->set_cursor(cursor_type::text_input);
			}

			if (!is_focused())
			{
				dragging_ = false;

				// reflect external changes while we don't own the keyboard
				if (bound_ && *bound_ != text_)
				{
					set_text(*bound_);
				}
			}
			else
			{
				bool edited = false;

				for (const cstd::uint32_t cp : input_->typed_chars())
				{
					te_key(buf_, stb_, cp);
					edited = true;
				}

				edited |= handle_command_keys();

				if (dragging_)
				{
					if (input_->is_mouse_down(0))
					{
						const position local = local_text_pos(input_->mouse_pos());
						te_drag(buf_, stb_, local.x, local.y);
					}
					else
					{
						dragging_ = false;
					}
				}

				if (edited)
				{
					sync_text_out();
					blink_ = 0.f;
				}

				update_scroll();
				blink_ += dt;
			}

			element::update(dt);
		}

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override
		{
			if (multiline_ || !font_)
			{
				return { 0.f, 0.f };
			}

			const float scale = current_scale();

			return { 0.f, font_->line_height() * scale };
		}

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			if (!font_)
			{
				return;
			}

			renderer.push_clip_rect(min, max);

			const float scale = current_scale();
			const float line_h = font_->line_height() * scale;
			const float font_size = style_.font_size.value_or(0.f);

			// Single-line inputs vertically center their text; multiline starts at the top. Center on
			// the glyph block (ascent..descent) rather than the full line height so line_gap doesn't
			// sit the text visually high (same approach as the checkbox label).
			float origin_y = min.y - scroll_.y;

			if (!multiline_)
			{
				const float glyph_h = (font_->ascent() - font_->descent()) * scale;
				origin_y = min.y + ((max.y - min.y) - glyph_h) * 0.5f;
			}

			const position origin = { min.x - scroll_.x, origin_y };

			const int len = static_cast<int>(buf_.chars.size());
			const bool has_selection = stb_.select_start != stb_.select_end;
			const int sel_lo = (stb_.select_start < stb_.select_end) ? stb_.select_start : stb_.select_end;
			const int sel_hi = (stb_.select_start < stb_.select_end) ? stb_.select_end : stb_.select_start;

			int i = 0;
			float row_y = origin.y;
			string_t line_utf8;

			while (i < len)
			{
				StbTexteditRow row;
				rv_te_layout_row(&row, const_cast<text_edit_buffer*>(&buf_), i);

				if (row.num_chars <= 0)
				{
					break;
				}

				const int row_end = i + row.num_chars;

				// selection highlight for this row
				if (has_selection && sel_hi > i && sel_lo < row_end)
				{
					const int hl_lo = (sel_lo > i) ? sel_lo : i;
					const int hl_hi = (sel_hi < row_end) ? sel_hi : row_end;
					const float x_lo = origin.x + x_offset_in_row(i, hl_lo);
					const float x_hi = origin.x + x_offset_in_row(i, hl_hi);

					renderer.draw_rect_filled({ x_lo, row_y }, { x_hi, row_y + line_h }, selection_color_);
				}

				// build the row's UTF-8 text, skipping a trailing newline
				line_utf8.clear();

				for (int k = i; k < row_end; ++k)
				{
					const char32_t ch = buf_.chars[static_cast<cstd::size_t>(k)];

					if (ch == U'\n')
					{
						continue;
					}

					encode_utf8(ch, line_utf8);
				}

				if (!line_utf8.empty())
				{
					renderer.draw_text(*font_, { origin.x, row_y }, line_utf8, visual_text_color_, font_size);
				}

				i = row_end;
				row_y += line_h;
			}

			// caret
			if (is_focused() && caret_visible())
			{
				float caret_x = 0.f;
				int caret_row = 0;
				caret_metrics(stb_.cursor, caret_x, caret_row);

				const float cx = origin.x + caret_x;
				const float cy = origin.y + static_cast<float>(caret_row) * line_h;

				renderer.draw_rect_filled({ cx, cy }, { cx + 1.5f, cy + line_h }, caret_color_);
			}

			renderer.pop_clip_rect();
		}

	private:
		void init_defaults() noexcept
		{
			style_.background_color = color{ 0.12f, 0.12f, 0.15f, 1.f };
			style_.rounding = 6.f;
			style_.border_color = color{ 0.3f, 0.3f, 0.36f, 1.f };
			style_.border_width = border_vector{ 1.f, 1.f, 1.f, 1.f };
			style_.text_color = color{ 1.f, 1.f, 1.f, 1.f };
			style_.padding = border_vector{ 6.f, 8.f, 6.f, 8.f };
			style_.transition_speed = 12.f;
		}

		[[nodiscard]] float current_scale() const noexcept
		{
			const float font_size = style_.font_size.value_or(0.f);

			return (font_size > 0.f && font_) ? font_size / font_->baked_size() : 1.f;
		}

		void refresh_font_context() noexcept
		{
			buf_.font = font_;
			buf_.scale = current_scale();
			buf_.line_height = font_ ? font_->line_height() * buf_.scale : 0.f;
			buf_.single_line = !multiline_;
			buf_.wrap_width = multiline_ ? content_width() : 0.f;
		}

		[[nodiscard]] float content_width() const noexcept
		{
			const auto pad = style_.padding.value_or(border_vector{});
			const auto brd = style_.border_width.value_or(border_vector{});

			return computed_size_.x - pad.left - pad.right - brd.left - brd.right;
		}

		// Convert an absolute mouse position into text-local coordinates,
		// accounting for padding/border insets and current scroll.
		[[nodiscard]] position local_text_pos(const position mouse) const noexcept
		{
			const auto pad = style_.padding.value_or(border_vector{});
			const auto brd = style_.border_width.value_or(border_vector{});
			const float origin_x = computed_pos_.x + pad.left + brd.left;
			const float origin_y = computed_pos_.y + pad.top + brd.top;

			return { mouse.x - origin_x + scroll_.x, mouse.y - origin_y + scroll_.y };
		}

		void set_text(const string_view_t value)
		{
			text_ = string_t(value);
			mark_layout_dirty();
			buf_.chars.clear();

			const char* s = text_.data();
			const char* end = s + text_.size();

			while (s < end)
			{
				buf_.chars.push_back(static_cast<char32_t>(decode_utf8(s, end)));
			}

			const int len = static_cast<int>(buf_.chars.size());

			if (stb_.cursor > len)
			{
				stb_.cursor = len;
			}

			stb_.select_start = stb_.select_end = stb_.cursor;
		}

		void sync_text_out()
		{
			string_t rebuilt;
			rebuilt.reserve(buf_.chars.size());

			for (const char32_t ch : buf_.chars)
			{
				encode_utf8(ch, rebuilt);
			}

			if (rebuilt == text_)
			{
				return;
			}

			text_ = cstd::move(rebuilt);
			mark_layout_dirty();

			if (bound_)
			{
				*bound_ = text_;
			}

			if (on_change_)
			{
				on_change_(text_);
			}
		}

		[[nodiscard]] bool caret_visible() const noexcept
		{
			return cstd::fmodf(blink_, 1.f) < 0.5f;
		}

		bool handle_command_keys()
		{
			const bool shift = input_->is_key_down(key::shift);
			const bool ctrl = input_->is_key_down(key::control);
			const cstd::uint32_t shift_bit = shift ? STB_TEXTEDIT_K_SHIFT : 0u;
			bool edited = false;

			auto send = [&](const cstd::uint32_t cmd)
			{
				te_key(buf_, stb_, cmd | shift_bit);
			};

			if (input_->is_key_pressed(key::left))   { send(STB_TEXTEDIT_K_LEFT);  edited = true; }
			if (input_->is_key_pressed(key::right))  { send(STB_TEXTEDIT_K_RIGHT); edited = true; }
			if (input_->is_key_pressed(key::up))     { send(STB_TEXTEDIT_K_UP);    edited = true; }
			if (input_->is_key_pressed(key::down))   { send(STB_TEXTEDIT_K_DOWN);  edited = true; }

			if (input_->is_key_pressed(key::home))
			{
				send(ctrl ? STB_TEXTEDIT_K_TEXTSTART : STB_TEXTEDIT_K_LINESTART);
				edited = true;
			}

			if (input_->is_key_pressed(key::end))
			{
				send(ctrl ? STB_TEXTEDIT_K_TEXTEND : STB_TEXTEDIT_K_LINEEND);
				edited = true;
			}

			if (input_->is_key_pressed(key::del))
			{
				te_key(buf_, stb_, STB_TEXTEDIT_K_DELETE);
				edited = true;
			}

			if (input_->is_key_pressed(key::backspace))
			{
				te_key(buf_, stb_, STB_TEXTEDIT_K_BACKSPACE);
				edited = true;
			}

			if (input_->is_key_pressed(key::enter))
			{
				if (multiline_)
				{
					te_key(buf_, stb_, U'\n');
					edited = true;
				}
				else if (on_submit_)
				{
					on_submit_(text_);
				}
			}

			if (ctrl)
			{
				if (input_->is_key_pressed(key::a)) { select_all(); edited = true; }
				if (input_->is_key_pressed(key::c)) { copy_selection(); }
				if (input_->is_key_pressed(key::x)) { if (copy_selection()) { delete_selection(); edited = true; } }
				if (input_->is_key_pressed(key::v)) { if (paste_clipboard()) { edited = true; } }
			}

			return edited;
		}

		// Selects the entire buffer and parks the caret at the end.
		void select_all() noexcept
		{
			stb_.select_start = 0;
			stb_.select_end = stb_.cursor = static_cast<int>(buf_.chars.size());
		}

		// Encodes the current selection to UTF-8 and copies it to the clipboard.
		// Returns false (and does nothing) when there is no selection.
		bool copy_selection()
		{
			if (stb_.select_start == stb_.select_end)
			{
				return false;
			}

			const int lo = (stb_.select_start < stb_.select_end) ? stb_.select_start : stb_.select_end;
			const int hi = (stb_.select_start < stb_.select_end) ? stb_.select_end : stb_.select_start;

			string_t out;

			for (int i = lo; i < hi; ++i)
			{
				encode_utf8(buf_.chars[static_cast<cstd::size_t>(i)], out);
			}

			input_->set_clipboard_text(out);
			return true;
		}

		// Removes the active selection in one edit step.
		void delete_selection()
		{
			if (stb_.select_start != stb_.select_end)
			{
				te_key(buf_, stb_, STB_TEXTEDIT_K_BACKSPACE);
			}
		}

		// Inserts the clipboard contents at the caret, replacing any selection.
		// Returns false when nothing was inserted.
		bool paste_clipboard()
		{
			const string_t clip = input_->get_clipboard_text();

			if (clip.empty())
			{
				return false;
			}

			const char* s = clip.data();
			const char* end = s + clip.size();
			bool inserted = false;

			while (s < end)
			{
				const cstd::uint32_t cp = decode_utf8(s, end);

				// Single-line fields never accept line breaks.
				if (!multiline_ && (cp == U'\n' || cp == U'\r'))
				{
					continue;
				}

				te_key(buf_, stb_, cp);
				inserted = true;
			}

			return inserted;
		}

		// Pixel x-offset of character index `target` within the row starting at `row_start`.
		[[nodiscard]] float x_offset_in_row(const int row_start, const int target) const noexcept
		{
			float x = 0.f;
			cstd::uint32_t prev = 0;

			for (int k = row_start; k < target; ++k)
			{
				const char32_t ch = buf_.chars[static_cast<cstd::size_t>(k)];

				if (ch == U'\n')
				{
					break;
				}

				if (font_)
				{
					x += glyph_step(*font_, prev, static_cast<cstd::uint32_t>(ch), buf_.scale);
				}

				prev = static_cast<cstd::uint32_t>(ch);
			}

			return x;
		}

		// Walk layout rows to find which row the cursor is on plus its x-offset.
		void caret_metrics(const int cursor, float& out_x, int& out_row) const noexcept
		{
			const int len = static_cast<int>(buf_.chars.size());
			int i = 0;
			int row = 0;

			while (i < len)
			{
				StbTexteditRow r;
				rv_te_layout_row(&r, const_cast<text_edit_buffer*>(&buf_), i);

				if (r.num_chars <= 0)
				{
					break;
				}

				const int row_end = i + r.num_chars;
				const bool ends_newline = buf_.chars[static_cast<cstd::size_t>(row_end - 1)] == U'\n';

				// cursor within this row, or sitting at its end. If the row ends in a
				// newline, cursor == row_end belongs to the following line (even when
				// that line is the empty last one), so fall through to the next row.
				if (cursor < row_end || (cursor == row_end && !ends_newline))
				{
					out_x = x_offset_in_row(i, cursor);
					out_row = row;
					return;
				}

				i = row_end;
				++row;
			}

			out_x = 0.f;
			out_row = row;
		}

		void update_scroll() noexcept
		{
			if (!font_)
			{
				return;
			}

			const float line_h = font_->line_height() * buf_.scale;

			float caret_x = 0.f;
			int caret_row = 0;
			caret_metrics(stb_.cursor, caret_x, caret_row);

			if (!multiline_)
			{
				const float view_w = content_width();

				if (caret_x - scroll_.x > view_w)
				{
					scroll_.x = caret_x - view_w;
				}

				if (caret_x - scroll_.x < 0.f)
				{
					scroll_.x = caret_x;
				}

				if (scroll_.x < 0.f)
				{
					scroll_.x = 0.f;
				}

				scroll_.y = 0.f;
			}
			else
			{
				const auto pad = style_.padding.value_or(border_vector{});
				const auto brd = style_.border_width.value_or(border_vector{});
				const float view_h = computed_size_.y - pad.top - pad.bottom - brd.top - brd.bottom;
				const float caret_y = static_cast<float>(caret_row) * line_h;

				if (caret_y + line_h - scroll_.y > view_h)
				{
					scroll_.y = caret_y + line_h - view_h;
				}

				if (caret_y - scroll_.y < 0.f)
				{
					scroll_.y = caret_y;
				}

				if (scroll_.y < 0.f)
				{
					scroll_.y = 0.f;
				}

				scroll_.x = 0.f;
			}
		}

		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;

		text_edit_buffer buf_;
		STB_TexteditState stb_ = {};
		string_t text_;
		string_t* bound_ = nullptr;

		bool multiline_ = false;
		bool dragging_ = false;
		float blink_ = 0.f;
		position scroll_ = { 0.f, 0.f };

		color selection_color_ = { 0.26f, 0.45f, 0.78f, 0.6f };
		color caret_color_ = { 1.f, 1.f, 1.f, 1.f };

		function_t<void(const string_t&)> on_change_;
		function_t<void(const string_t&)> on_submit_;
	};
}
