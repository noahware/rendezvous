#include "multi_combo_box.hpp"
#include <algorithm>

namespace rv
{
	void multi_combo_popup::configure(shared_ptr_t<gui_font> font, shared_ptr_t<input> input,
	               const vector_t<string_t>* options,
	               const vector_t<cstd::int32_t>* selected_indices,
	               function_t<void(cstd::int32_t)> on_toggle) noexcept
	{
		font_ = cstd::move(font);
		input_ = cstd::move(input);
		options_ = options;
		selected_indices_ = selected_indices;
		on_toggle_ = cstd::move(on_toggle);
	}

	bool multi_combo_popup::on_mouse_click()
	{
		if (on_toggle_ && hovered_row_ >= 0)
		{
			on_toggle_(hovered_row_);
		}

		return true;
	}

	void multi_combo_popup::update(const float dt)
	{
		row_height_ = row_height();

		const cstd::int32_t count = options_ ? static_cast<cstd::int32_t>(options_->size()) : 0;

		if (hovered_ && input_ && row_height_ > 0.f && count > 0)
		{
			const float local_y = input_->mouse_pos().y - visual_pos().y;
			cstd::int32_t row = static_cast<cstd::int32_t>(local_y / row_height_);

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

	float multi_combo_popup::row_height() const noexcept
	{
		if (!font_)
		{
			return 0.f;
		}

		return font_->line_height() * current_scale() + 2.f * combo_row_pad_v;
	}

	bool multi_combo_popup::is_index_selected(const cstd::int32_t index) const noexcept
	{
		if (!selected_indices_)
		{
			return false;
		}

		const auto it = std::lower_bound(selected_indices_->begin(), selected_indices_->end(), index);

		return it != selected_indices_->end() && *it == index;
	}

	void multi_combo_popup::render_self(gui_renderer& renderer, const position min, const position max) const
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

		const float rounding = style_.rounding.value_or(0.f);
		const cstd::size_t last = options_->size() - 1;

		const float check_y_offset = (rh - multi_combo_check_size) * 0.5f;
		const float text_x = min.x + combo_row_pad_h + multi_combo_check_size + multi_combo_check_gap;

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

			const bool selected = is_index_selected(static_cast<cstd::int32_t>(i));

			if (selected)
			{
				renderer.draw_rect_filled(row_min, row_max, row_selected_color_, row_round, flags);
			}

			if (static_cast<cstd::int32_t>(i) == hovered_row_)
			{
				renderer.draw_rect_filled(row_min, row_max, row_hover_color_, row_round, flags);
			}

			const float check_x = min.x + combo_row_pad_h;
			const float check_y = top + check_y_offset;

			if (selected)
			{
				renderer.draw_rect_filled(
					{ check_x, check_y },
					{ check_x + multi_combo_check_size, check_y + multi_combo_check_size },
					check_color_, 2.f);
			}
			else
			{
				renderer.draw_rect(
					{ check_x, check_y },
					{ check_x + multi_combo_check_size, check_y + multi_combo_check_size },
					color{ 0.4f, 0.4f, 0.45f, 1.f }, 1.f, 2.f);
			}

			const float text_y = top + (rh - line_h) * 0.5f;
			renderer.draw_text(*font_, { text_x, text_y }, (*options_)[i], text_col, font_size);
		}

		renderer.pop_clip_rect();
	}

	multi_combo_box::multi_combo_box(const element_size size, shared_ptr_t<gui_font> font, shared_ptr_t<input> input) noexcept
			:	element(size), font_(cstd::move(font)), input_(cstd::move(input))
	{
		init_defaults();

		popup_ = make_child<multi_combo_popup>(element_size{ styled_size::px(0.f), styled_size::px(0.f) });
		popup_->positioning(position_type::absolute);
		popup_->set_visible(false);
		popup_->configure(font_, input_, &options_, &selected_indices_,
			[this](const cstd::int32_t i)
			{
				toggle(i);
			});
	}

	multi_combo_box& multi_combo_box::options(vector_t<string_t> opts)
	{
		options_ = cstd::move(opts);

		const auto limit = static_cast<cstd::int32_t>(options_.size());
		vector_t<cstd::int32_t> clamped;

		for (const auto idx : selected_indices_)
		{
			if (idx >= 0 && idx < limit)
			{
				clamped.push_back(idx);
			}
		}

		selected_indices_ = cstd::move(clamped);

		return *this;
	}

	multi_combo_box& multi_combo_box::selected(vector_t<cstd::int32_t> indices)
	{
		std::sort(indices.begin(), indices.end());
		indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
		selected_indices_ = cstd::move(indices);

		if (bound_)
		{
			*bound_ = selected_indices_;
		}

		return *this;
	}

	multi_combo_box& multi_combo_box::select_all()
	{
		selected_indices_.clear();

		for (cstd::int32_t i = 0; i < static_cast<cstd::int32_t>(options_.size()); ++i)
		{
			selected_indices_.push_back(i);
		}

		if (bound_)
		{
			*bound_ = selected_indices_;
		}

		return *this;
	}

	bool multi_combo_box::is_selected(const cstd::int32_t index) const noexcept
	{
		const auto it = std::lower_bound(selected_indices_.begin(), selected_indices_.end(), index);

		return it != selected_indices_.end() && *it == index;
	}

	void multi_combo_box::update(const float dt)
	{
		if (bound_ && !open_ && *bound_ != selected_indices_)
		{
			selected_indices_ = *bound_;
		}

		popup_->set_visible(open_);

		const auto pad = style_.padding.value_or(border_vector{});
		const auto brd = style_.border_width.value_or(border_vector{});
		const float inset_l = pad.left + brd.left;
		const float inset_t = pad.top + brd.top;

		const cstd::int32_t count = static_cast<cstd::int32_t>(options_.size());
		const cstd::int32_t rows = count < max_visible_rows ? count : max_visible_rows;
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

	vector_2d<float> multi_combo_box::content_size(const vector_2d<float> available) const noexcept
	{
		if (!font_)
		{
			return { 0.f, 0.f };
		}

		return { 0.f, font_->line_height() * current_scale() };
	}

	void multi_combo_box::render_self(gui_renderer& renderer, const position min, const position max) const
	{
		if (!font_)
		{
			return;
		}

		const float scale = current_scale();
		const float line_h = font_->line_height() * scale;
		const float font_size = style_.font_size.value_or(0.f);
		const float text_y = min.y + ((max.y - min.y) - line_h) * 0.5f;

		const string_t display = build_display_text();
		const bool has_selection = !display.empty();
		const string_t& label = has_selection ? display : placeholder_;

		color text_col = visual_text_color_;

		if (!has_selection)
		{
			text_col.a *= 0.5f;
		}

		renderer.push_clip_rect(min, { max.x - 30.f, max.y });
		renderer.draw_text(*font_, { min.x, text_y }, label, text_col, font_size);
		renderer.pop_clip_rect();

		const float x_center = max.x - 8.f;
		const float y_center = (min.y + max.y) * 0.5f;

		if (open_)
		{
			renderer.add_path_point({ x_center - 4.f, y_center + 2.f });
			renderer.add_path_point({ x_center, y_center - 2.f });
			renderer.add_path_point({ x_center + 4.f, y_center + 2.f });
		}
		else
		{
			renderer.add_path_point({ x_center - 4.f, y_center - 2.f });
			renderer.add_path_point({ x_center, y_center + 2.f });
			renderer.add_path_point({ x_center + 4.f, y_center - 2.f });
		}

		renderer.draw_lined_path(visual_text_color_, 1.5f, false, 1.f, cap_style::round, join_style::miter);
	}

	void multi_combo_box::init_defaults() noexcept
	{
		style_.background_color = color{ 0.12f, 0.12f, 0.15f, 1.f };
		style_.rounding = 6.f;
		style_.border_color = color{ 0.3f, 0.3f, 0.36f, 1.f };
		style_.border_width = border_vector{ 1.f, 1.f, 1.f, 1.f };
		style_.text_color = color{ 1.f, 1.f, 1.f, 1.f };
		style_.padding = border_vector{ 6.f, 8.f, 6.f, 8.f };
		style_.transition_speed = 12.f;
	}

	void multi_combo_box::toggle(const cstd::int32_t index)
	{
		if (index < 0 || index >= static_cast<cstd::int32_t>(options_.size()))
		{
			return;
		}

		const auto it = std::lower_bound(selected_indices_.begin(), selected_indices_.end(), index);

		if (it != selected_indices_.end() && *it == index)
		{
			selected_indices_.erase(it);
		}
		else
		{
			selected_indices_.insert(it, index);
		}

		if (bound_)
		{
			*bound_ = selected_indices_;
		}

		if (on_change_)
		{
			on_change_(selected_indices_);
		}
	}

	string_t multi_combo_box::build_display_text() const
	{
		if (selected_indices_.empty())
		{
			return {};
		}

		string_t joined;

		for (cstd::size_t i = 0; i < selected_indices_.size(); ++i)
		{
			const auto idx = static_cast<cstd::size_t>(selected_indices_[i]);

			if (idx < options_.size())
			{
				if (!joined.empty())
				{
					joined += ", ";
				}

				joined += options_[idx];
			}
		}

		if (!font_)
		{
			return joined;
		}

		const float scale = current_scale();
		const auto pad = style_.padding.value_or(border_vector{});
		const float available = computed_size_.x - pad.left - pad.right - 30.f;
		const float width = measure_text_width(*font_, joined, scale);

		if (width <= available)
		{
			return joined;
		}

		return cstd::format("{} selected", selected_indices_.size());
	}
}
