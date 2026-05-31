#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../../input/input.hpp"
#include "../../util/string.hpp"

namespace rv
{
	constexpr float combo_row_pad_v = 4.f;
	constexpr float combo_row_pad_h = 10.f;

	class combo_popup final : public element
	{
	public:
		explicit combo_popup(const element_size size) noexcept
				:	element(size)
		{
			style_.background_color = color{ 0.15f, 0.15f, 0.18f, 1.f };
			style_.rounding = 6.f;
			style_.text_color = color{ 1.f, 1.f, 1.f, 1.f };
			// no padding/border so the content box matches the element box; this keeps
			// per-row hit-testing (update) aligned with row rendering (render_self).
			style_.shadow_color = color{ 0.f, 0.f, 0.f, 0.5f };
			style_.shadow_blur = 12.f;

			topmost(true);
		}

		void configure(shared_ptr_t<gui_font> font, shared_ptr_t<input> input,
		               const vector_t<string_t>* options, const int* selected,
		               function_t<void(int)> on_select) noexcept
		{
			font_ = cstd::move(font);
			input_ = cstd::move(input);
			options_ = options;
			selected_ = selected;
			on_select_ = cstd::move(on_select);
		}

		bool on_mouse_click() override
		{
			if (on_select_ && hovered_row_ >= 0)
			{
				on_select_(hovered_row_);
			}

			return true;
		}

		void update(const float dt) override
		{
			row_height_ = row_height();

			const int count = options_ ? static_cast<int>(options_->size()) : 0;

			if (hovered_ && input_ && row_height_ > 0.f && count > 0)
			{
				const float local_y = input_->mouse_pos().y - computed_pos_.y;
				int row = static_cast<int>(local_y / row_height_);

				if (row < 0)
				{
					row = 0;
				}

				if (row >= count)
				{
					row = count - 1;
				}

				hovered_row_ = row;
				input_->set_cursor(cursor_type::hand);
			}
			else
			{
				hovered_row_ = -1;
			}

			element::update(dt);
		}

		[[nodiscard]] float row_height() const noexcept
		{
			if (!font_)
			{
				return 0.f;
			}

			return font_->line_height() * current_scale() + 2.f * combo_row_pad_v;
		}

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			if (!font_ || !options_)
			{
				return;
			}

			renderer.push_clip_rect(min, max);

			const float scale = current_scale();
			const float line_h = font_->line_height() * scale;
			const float font_size = style_.font_size.value_or(0.f);
			const float rh = row_height_ > 0.f ? row_height_ : (line_h + 2.f * combo_row_pad_v);
			const color text_col = style_.text_color.value_or(color{ 1.f, 1.f, 1.f, 1.f });

			// match the popup's corner rounding on the first/last row highlights so a
			// highlighted edge row doesn't paint square corners over the rounded popup.
			const float rounding = style_.rounding.value_or(0.f);
			const cstd::size_t last = options_->size() - 1;

			for (cstd::size_t i = 0; i < options_->size(); ++i)
			{
				const float top = min.y + static_cast<float>(i) * rh;
				const position row_min = { min.x, top };
				const position row_max = { max.x, top + rh };

				cstd::uint32_t flag_bits = 0;

				if (i == 0)
				{
					flag_bits |= rounding_flags_top_left | rounding_flags_top_right;
				}

				if (i == last)
				{
					flag_bits |= rounding_flags_bottom_left | rounding_flags_bottom_right;
				}

				const auto flags = static_cast<rounding_flags>(flag_bits);
				const float row_round = flag_bits != 0 ? rounding : 0.f;

				if (selected_ && static_cast<int>(i) == *selected_)
				{
					renderer.draw_rect_filled(row_min, row_max, row_selected_color_, row_round, flags);
				}

				if (static_cast<int>(i) == hovered_row_)
				{
					renderer.draw_rect_filled(row_min, row_max, row_hover_color_, row_round, flags);
				}

				const float text_y = top + (rh - line_h) * 0.5f;
				renderer.draw_text(*font_, { min.x + combo_row_pad_h, text_y },
				                   (*options_)[i], text_col, font_size);
			}

			renderer.pop_clip_rect();
		}

	private:
		[[nodiscard]] float current_scale() const noexcept
		{
			const float font_size = style_.font_size.value_or(0.f);

			return (font_size > 0.f && font_) ? font_size / font_->baked_size() : 1.f;
		}

		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;
		const vector_t<string_t>* options_ = nullptr;
		const int* selected_ = nullptr;
		function_t<void(int)> on_select_;

		int hovered_row_ = -1;
		float row_height_ = 0.f;

		color row_hover_color_ = { 0.26f, 0.45f, 0.78f, 0.55f };
		color row_selected_color_ = { 0.3f, 0.3f, 0.36f, 1.f };
	};

	// The combo box header/control. Click to open the dropdown list, click an option to
	// select, click elsewhere to close (focus-out). Index-based selection mirroring the
	// slider/checkbox bind pattern.
	class combo_box final : public element
	{
	public:
		combo_box(const element_size size, shared_ptr_t<gui_font> font, shared_ptr_t<input> input) noexcept
				:	element(size), font_(cstd::move(font)), input_(cstd::move(input))
		{
			init_defaults();

			// create the popup once, as a child of this combo (plain tree-order rendering).
			popup_ = make_child<combo_popup>(element_size{ styled_size::px(0.f), styled_size::px(0.f) });
			popup_->positioning(position_type::absolute);
			popup_->set_visible(false);
			popup_->configure(font_, input_, &options_, &selected_,
				[this](const int i)
				{
					commit(i);
					open_ = false;
				});
		}

		combo_box& options(vector_t<string_t> opts)
		{
			options_ = cstd::move(opts);

			if (selected_ >= static_cast<int>(options_.size()))
			{
				selected_ = options_.empty() ? -1 : static_cast<int>(options_.size()) - 1;
			}

			return *this;
		}

		combo_box& selected(const int index) noexcept
		{
			selected_ = index;

			if (bound_)
			{
				*bound_ = selected_;
			}

			return *this;
		}

		combo_box& bind(int* ptr) noexcept
		{
			bound_ = ptr;

			if (bound_)
			{
				selected_ = *bound_;
			}

			return *this;
		}

		combo_box& on_change(function_t<void(int)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		combo_box& placeholder(const string_view_t text)
		{
			placeholder_ = string_t(text);

			return *this;
		}

		[[nodiscard]] int selected() const noexcept
		{
			return selected_;
		}

		[[nodiscard]] bool focusable() const noexcept override
		{
			return true;
		}

		bool on_mouse_click() override
		{
			open_ = !open_;

			return true;
		}

		void update(const float dt) override
		{
			// reflect external changes while the list is closed
			if (bound_ && !open_ && *bound_ != selected_)
			{
				selected_ = *bound_;
			}

			// clicking an option (the popup child, not the combo) clears combo focus
			if (!is_focused())
			{
				open_ = false;
			}

			popup_->set_visible(open_);

			// keep the popup positioned flush below the header and sized to its contents
			const auto pad = style_.padding.value_or(border_vector{});
			const auto brd = style_.border_width.value_or(border_vector{});
			const float inset_l = pad.left + brd.left;
			const float inset_t = pad.top + brd.top;

			const int count = static_cast<int>(options_.size());
			const int rows = count < max_visible_rows ? count : max_visible_rows;
			const float rh = popup_->row_height();

			popup_->text_size(style_.font_size.value_or(0.f));
			popup_->set_declared_size(element_size{
				styled_size::px(computed_size_.x),
				styled_size::px(static_cast<float>(rows) * rh)
			});
			popup_->inset_left(styled_size::px(-inset_l));
			popup_->inset_top(styled_size::px(computed_size_.y - inset_t));

			if (hovered_ && input_)
			{
				input_->set_cursor(cursor_type::hand);
			}

			element::update(dt);
		}

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override
		{
			if (!font_)
			{
				return { 0.f, 0.f };
			}

			return { 0.f, font_->line_height() * current_scale() };
		}

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			if (!font_)
			{
				return;
			}

			const float scale = current_scale();
			const float line_h = font_->line_height() * scale;
			const float font_size = style_.font_size.value_or(0.f);
			const float text_y = min.y + ((max.y - min.y) - line_h) * 0.5f;

			const bool has_selection = selected_ >= 0 && selected_ < static_cast<int>(options_.size());
			const string_t& label = has_selection ? options_[static_cast<cstd::size_t>(selected_)] : placeholder_;

			color text_col = visual_text_color_;

			if (!has_selection)
			{
				text_col.a *= 0.5f;
			}

			renderer.push_clip_rect(min, max);
			renderer.draw_text(*font_, { min.x, text_y }, label, text_col, font_size);
			renderer.pop_clip_rect();

			// dropdown indicator caret near the right edge
			renderer.draw_text(*font_, { max.x - 14.f, text_y }, "v", visual_text_color_, font_size);
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

		void commit(int index)
		{
			if (options_.empty())
			{
				selected_ = -1;

				return;
			}

			if (index < 0)
			{
				index = 0;
			}

			if (index >= static_cast<int>(options_.size()))
			{
				index = static_cast<int>(options_.size()) - 1;
			}

			selected_ = index;

			if (bound_)
			{
				*bound_ = selected_;
			}

			if (on_change_)
			{
				on_change_(selected_);
			}
		}

		static constexpr int max_visible_rows = 8;

		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;
		shared_ptr_t<combo_popup> popup_;

		vector_t<string_t> options_;
		int selected_ = -1;
		int* bound_ = nullptr;
		bool open_ = false;
		string_t placeholder_ = "Select...";

		function_t<void(int)> on_change_;
	};
}
