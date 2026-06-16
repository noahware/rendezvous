#include "panel.hpp"

namespace rv
{
	bool panel::on_mouse_click()
	{
		if (!input_)
		{
			return false;
		}

		if (auto_portrait_focus_ && z_index() < z_index_popup)
		{
			if (const auto g = gui_.lock())
			{
				g->show_portrait_focus(shared_from_this(), portrait_focus_blur_, portrait_focus_color_);
			}
		}

		// raise above sibling panels for focus-to-front behaviour.
		// skip when in portrait focus (z >= popup layer) to avoid dropping it back into
		// the panel band where the blur background would cover it.
		if (z_index() < z_index_popup)
		{
			if (const auto g = gui_.lock())
			{
				z_index(g->raise_panel());
			}
		}

		const position mouse = input_->mouse_pos();
		const position p_min = visual_pos();
		const position p_max = { p_min.x + computed_size_.x, p_min.y + computed_size_.y };

		if (resizable_)
		{
			const resize_edge edge = detect_resize_edge(mouse, p_min, p_max);

			if (edge != resize_edge::none)
			{
				resizing_ = true;
				active_edge_ = edge;
				resize_anchor_ = mouse;
				resize_start_size_ = computed_size_;

				const float left = style_.inset_left.value_or(styled_size::px(0)).value;
				const float top = style_.inset_top.value_or(styled_size::px(0)).value;
				resize_start_pos_ = { left, top };

				return true;
			}
		}

		if (draggable_)
		{
			if (mouse.x >= p_min.x && mouse.x <= p_max.x &&
			    mouse.y >= p_min.y && mouse.y <= p_max.y)
			{
				dragging_ = true;
				drag_anchor_ = mouse;

				return true;
			}
		}

		// a panel is an opaque window: consume the click so it doesn't fall through to whatever
		// sits behind it (the raise above has already taken effect).
		return true;
	}

	void panel::update(const float dt)
	{
		element::update(dt);

		if (!input_)
		{
			return;
		}

		const position mouse = input_->mouse_pos();
		const bool mouse_down = input_->is_mouse_down(0);

		if (dragging_)
		{
			if (!mouse_down)
			{
				dragging_ = false;
			}
			else
			{
				const float dx = mouse.x - drag_anchor_.x;
				const float dy = mouse.y - drag_anchor_.y;

				const float current_left = style_.inset_left.value_or(styled_size::px(0)).value;
				const float current_top = style_.inset_top.value_or(styled_size::px(0)).value;

				style_.inset_left = styled_size::px(current_left + dx);
				style_.inset_top = styled_size::px(current_top + dy);

				mark_layout_dirty();

				drag_anchor_ = mouse;
			}
		}

		if (resizing_)
		{
			if (!mouse_down)
			{
				resizing_ = false;
				active_edge_ = resize_edge::none;
			}
			else
			{
				apply_resize(mouse);
			}
		}

		if (!dragging_ && !resizing_ && resizable_)
		{
			const position p_min = visual_pos();
			const position p_max = { p_min.x + computed_size_.x, p_min.y + computed_size_.y };
			const resize_edge edge = detect_resize_edge(mouse, p_min, p_max);

			update_cursor(edge);
		}

		if (scrollable_)
		{
			style_.overflow = overflow_mode::scroll;
		}
		else
		{
			style_.overflow = overflow_mode::hidden;
		}
	}

	void panel::init_panel_defaults() noexcept
	{
		style_.background_color = color{ 0.08f, 0.08f, 0.10f, 0.95f };
		style_.rounding = 8.f;
		style_.border_color = color{ 0.25f, 0.25f, 0.30f, 1.f };
		style_.border_width = border_vector{ 1.f, 1.f, 1.f, 1.f };
		style_.transition_speed = 12.f;
		style_.direction = layout_direction::vertical;
		style_.position = position_type::absolute;
		style_.overflow = overflow_mode::hidden;
		style_.padding = border_vector{ 0.f, 0.f, 0.f, 0.f };

		// float panels in the panel overlay layer so they sit above inline content and can be
		// raised above one another on click (see on_mouse_click).
		z_index(z_index_panel);
	}

	resize_edge panel::detect_resize_edge(const position mouse, const position p_min,
	                                      const position p_max) const noexcept
	{
		const bool hovered = mouse.x >= p_min.x && mouse.x <= p_max.x &&
		                     mouse.y >= p_min.y && mouse.y <= p_max.y;

		if (!hovered)
		{
			const bool near = mouse.x >= p_min.x - resize_border_ &&
			                  mouse.x <= p_max.x + resize_border_ &&
			                  mouse.y >= p_min.y - resize_border_ &&
			                  mouse.y <= p_max.y + resize_border_;

			if (!near)
			{
				return resize_edge::none;
			}
		}

		const bool on_left   = mouse.x >= p_min.x - resize_border_ && mouse.x <= p_min.x + resize_border_;
		const bool on_right  = mouse.x >= p_max.x - resize_border_ && mouse.x <= p_max.x + resize_border_;
		const bool on_top    = mouse.y >= p_min.y - resize_border_ && mouse.y <= p_min.y + resize_border_;
		const bool on_bottom = mouse.y >= p_max.y - resize_border_ && mouse.y <= p_max.y + resize_border_;

		if (on_top && on_left && !lock_width_ && !lock_height_)     return resize_edge::top_left;
		if (on_top && on_right && !lock_width_ && !lock_height_)    return resize_edge::top_right;
		if (on_bottom && on_left && !lock_width_ && !lock_height_)  return resize_edge::bottom_left;
		if (on_bottom && on_right && !lock_width_ && !lock_height_) return resize_edge::bottom_right;
		if (on_top && !lock_height_)    return resize_edge::top;
		if (on_bottom && !lock_height_) return resize_edge::bottom;
		if (on_left && !lock_width_)    return resize_edge::left;
		if (on_right && !lock_width_)   return resize_edge::right;

		return resize_edge::none;
	}

	void panel::update_cursor(const resize_edge edge) const noexcept
	{
		if (!input_)
		{
			return;
		}

		switch (edge)
		{
		case resize_edge::top:
		case resize_edge::bottom:
			input_->set_cursor(cursor_type::resize_ns);
			break;
		case resize_edge::left:
		case resize_edge::right:
			input_->set_cursor(cursor_type::resize_ew);
			break;
		case resize_edge::top_left:
		case resize_edge::bottom_right:
			input_->set_cursor(cursor_type::resize_nwse);
			break;
		case resize_edge::top_right:
		case resize_edge::bottom_left:
			input_->set_cursor(cursor_type::resize_nesw);
			break;
		default:
			break;
		}
	}

	void panel::apply_resize(const position mouse) noexcept
	{
		const float dx = mouse.x - resize_anchor_.x;
		const float dy = mouse.y - resize_anchor_.y;

		float new_w = resize_start_size_.x;
		float new_h = resize_start_size_.y;
		float new_x = resize_start_pos_.x;
		float new_y = resize_start_pos_.y;

		if (has_flag(active_edge_, resize_edge::right) && !lock_width_)
		{
			new_w = resize_start_size_.x + dx;
		}

		if (has_flag(active_edge_, resize_edge::bottom) && !lock_height_)
		{
			new_h = resize_start_size_.y + dy;
		}

		if (has_flag(active_edge_, resize_edge::left) && !lock_width_)
		{
			new_w = resize_start_size_.x - dx;
			new_x = resize_start_pos_.x + dx;
		}

		if (has_flag(active_edge_, resize_edge::top) && !lock_height_)
		{
			new_h = resize_start_size_.y - dy;
			new_y = resize_start_pos_.y + dy;
		}

		if (new_w < min_panel_w_)
		{
			if (has_flag(active_edge_, resize_edge::left))
			{
				new_x -= (min_panel_w_ - new_w);
			}

			new_w = min_panel_w_;
		}

		if (max_panel_w_ > 0.f && new_w > max_panel_w_)
		{
			if (has_flag(active_edge_, resize_edge::left))
			{
				new_x -= (max_panel_w_ - new_w);
			}

			new_w = max_panel_w_;
		}

		if (new_h < min_panel_h_)
		{
			if (has_flag(active_edge_, resize_edge::top))
			{
				new_y -= (min_panel_h_ - new_h);
			}

			new_h = min_panel_h_;
		}

		if (max_panel_h_ > 0.f && new_h > max_panel_h_)
		{
			if (has_flag(active_edge_, resize_edge::top))
			{
				new_y -= (max_panel_h_ - new_h);
			}

			new_h = max_panel_h_;
		}

		style_.size = element_size{ styled_size::px(new_w), styled_size::px(new_h) };
		style_.inset_left = styled_size::px(new_x);
		style_.inset_top = styled_size::px(new_y);

		mark_layout_dirty();
	}
}
