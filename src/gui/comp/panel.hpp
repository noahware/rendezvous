#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../../input/input.hpp"
#include "../../util/string.hpp"

namespace rv
{
	enum class resize_edge : cstd::uint8_t
	{
		none = 0,
		top = 1 << 0,
		right = 1 << 1,
		bottom = 1 << 2,
		left = 1 << 3,
		top_left = top | left,
		top_right = top | right,
		bottom_left = bottom | left,
		bottom_right = bottom | right
	};

	[[nodiscard]] inline resize_edge operator|(const resize_edge a, const resize_edge b) noexcept
	{
		return static_cast<resize_edge>(static_cast<cstd::uint8_t>(a) | static_cast<cstd::uint8_t>(b));
	}

	[[nodiscard]] inline bool has_flag(const resize_edge value, const resize_edge flag) noexcept
	{
		return (static_cast<cstd::uint8_t>(value) & static_cast<cstd::uint8_t>(flag)) != 0;
	}

	class panel : public element
	{
	public:
		panel() noexcept = default;

		explicit panel(const element_size size, shared_ptr_t<input> input) noexcept
			: element(size), input_(cstd::move(input))
		{
			init_panel_defaults();
		}

		panel& draggable(const bool v) noexcept
		{
			draggable_ = v;

			return *this;
		}

		[[nodiscard]] bool is_dragging() const noexcept
		{
			return dragging_;
		}

		panel& resizable(const bool v) noexcept
		{
			resizable_ = v;

			return *this;
		}

		panel& resize_border(const float width) noexcept
		{
			resize_border_ = width;

			return *this;
		}

		panel& min_panel_size(const float w, const float h) noexcept
		{
			min_panel_w_ = w;
			min_panel_h_ = h;

			return *this;
		}

		panel& max_panel_size(const float w, const float h) noexcept
		{
			max_panel_w_ = w;
			max_panel_h_ = h;

			return *this;
		}

		panel& lock_width(const bool v) noexcept
		{
			lock_width_ = v;

			return *this;
		}

		panel& lock_height(const bool v) noexcept
		{
			lock_height_ = v;

			return *this;
		}

		panel& scrollable(const bool v) noexcept
		{
			scrollable_ = v;
			show_scrollbar(v);

			return *this;
		}

		panel& show_scrollbar(const bool v) noexcept
		{
			element::show_scrollbar(v);

			return *this;
		}

		bool on_mouse_click() override
		{
			if (!input_)
			{
				return false;
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

			return false;
		}

		void update(const float dt) override
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

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
		}

	private:
		void init_panel_defaults() noexcept
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
		}

		[[nodiscard]] resize_edge detect_resize_edge(const position mouse, const position p_min,
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

		void update_cursor(const resize_edge edge) const noexcept
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

		void apply_resize(const position mouse) noexcept
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
		}

		shared_ptr_t<input> input_;

		bool draggable_ = false;
		bool dragging_ = false;
		position drag_anchor_ = {};

		bool resizable_ = false;
		bool resizing_ = false;
		float resize_border_ = 6.f;
		resize_edge active_edge_ = resize_edge::none;
		position resize_anchor_ = {};
		vector_2d<float> resize_start_size_ = {};
		position resize_start_pos_ = {};

		float min_panel_w_ = 100.f;
		float min_panel_h_ = 50.f;
		float max_panel_w_ = 0.f;
		float max_panel_h_ = 0.f;
		bool lock_width_ = false;
		bool lock_height_ = false;

		bool scrollable_ = false;
	};
}
