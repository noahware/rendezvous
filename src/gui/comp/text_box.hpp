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

		bool on_mouse_click() override;

		void update(const float dt) override;

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override;

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override;

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

		void refresh_font_context() noexcept;

		[[nodiscard]] float content_width() const noexcept
		{
			const auto pad = style_.padding.value_or(border_vector{});
			const auto brd = style_.border_width.value_or(border_vector{});

			return computed_size_.x - pad.left - pad.right - brd.left - brd.right;
		}

		// Convert an absolute mouse position into text-local coordinates,
		// accounting for padding/border insets and current scroll.
		[[nodiscard]] position local_text_pos(const position mouse) const noexcept;

		void set_text(string_view_t value);

		void sync_text_out();

		[[nodiscard]] bool caret_visible() const noexcept
		{
			return cstd::fmodf(blink_, 1.f) < 0.5f;
		}

		bool handle_command_keys();

		// Selects the entire buffer and parks the caret at the end.
		void select_all() noexcept
		{
			stb_.select_start = 0;
			stb_.select_end = stb_.cursor = static_cast<cstd::int32_t>(buf_.chars.size());
		}

		// Encodes the current selection to UTF-8 and copies it to the clipboard.
		// Returns false (and does nothing) when there is no selection.
		bool copy_selection();

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
		bool paste_clipboard();

		// Pixel x-offset of character index `target` within the row starting at `row_start`.
		[[nodiscard]] float x_offset_in_row(const cstd::int32_t row_start, const cstd::int32_t target) const noexcept;

		// Walk layout rows to find which row the cursor is on plus its x-offset.
		void caret_metrics(const cstd::int32_t cursor, float& out_x, cstd::int32_t& out_row) const noexcept;

		void update_scroll() noexcept;

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
