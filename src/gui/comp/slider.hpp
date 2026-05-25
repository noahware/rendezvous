#pragma once
#include "../element.hpp"
#include "../../input/input.hpp"

namespace rv
{
	class slider : public element
	{
	public:
		slider(const element_size size, shared_ptr_t<input> input) noexcept
				:	element(size), input_(cstd::move(input))
		{
			init_slider_defaults();
		}

		slider& on_change(function_t<void(float)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		slider& value(const float v)
		{
			target_value_ = v;
			visual_value_ = v;

			return *this;
		}

		[[nodiscard]] float value() const noexcept
		{
			return target_value_;
		}

		slider& fill_color(const color col) noexcept
		{
			fill_color_ = col;

			return *this;
		}

		slider& thumb_color(const color col) noexcept
		{
			thumb_color_ = col;

			return *this;
		}

		slider& thumb_color_active(const color col) noexcept
		{
			thumb_color_active_ = col;

			return *this;
		}

		slider& thumb_radius(const float r) noexcept
		{
			thumb_radius_ = r;

			return *this;
		}

		bool on_mouse_click() override
		{
			dragging_ = true;

			update_value_from_mouse();

			return true;
		}

		void update(const float dt) override
		{
			element::update(dt);

			if (dragging_)
			{
				if (!input_->is_mouse_down(0))
				{
					dragging_ = false;
				}
				else
				{
					update_value_from_mouse();
				}
			}

			const float diff = target_value_ - visual_value_;

			if (diff != 0.f)
			{
				const float speed = style_.transition_speed.value_or(12.f);
				const float factor = cstd::fminf(speed * dt, 1.f);
				visual_value_ += diff * factor;

				if (cstd::fabsf(diff) < 0.001f)
				{
					visual_value_ = target_value_;
				}
			}
		}

	protected:
		slider(const element_size size, shared_ptr_t<input> input, bool) noexcept
				:	element(size), input_(cstd::move(input))
		{
			init_slider_defaults();
		}

		void init_slider_defaults() noexcept
		{
			style_.background_color = color{ 0.3f, 0.3f, 0.3f, 1.f };
			style_.transition_speed = 12.f;
		}

		void update_value_from_mouse()
		{
			const position mouse_pos = input_->mouse_pos();
			const float track_x = computed_pos_.x + thumb_radius_;
			const float track_width = computed_size_.x - thumb_radius_ * 2.f;

			if (track_width <= 0.f)
			{
				return;
			}

			const float raw = (mouse_pos.x - track_x) / track_width;
			const float clamped = cstd::fmaxf(0.f, cstd::fminf(1.f, raw));

			if (clamped != target_value_)
			{
				target_value_ = clamped;

				if (on_change_)
				{
					on_change_(target_value_);
				}
			}
		}

		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			const float height = max.y - min.y;
			const float center_y = min.y + height * 0.5f;
			const float track_height = 4.f;
			const color track_col = style_.background_color.value_or(color{ 0.3f, 0.3f, 0.3f, 1.f });

			const position track_min = { min.x + thumb_radius_, center_y - track_height * 0.5f };
			const position track_max = { max.x - thumb_radius_, center_y + track_height * 0.5f };

			renderer.draw_rect_filled(track_min, track_max, track_col, track_height * 0.5f);

			const float track_width = track_max.x - track_min.x;
			const position fill_max = { track_min.x + track_width * visual_value_, track_max.y };

			if (visual_value_ > 0.f)
			{
				renderer.draw_rect_filled(track_min, fill_max, fill_color_, track_height * 0.5f);
			}

			const float thumb_x = track_min.x + track_width * visual_value_;
			const position thumb_pos = { thumb_x, center_y };
			const color thumb_col = (hovered_ || dragging_) ? thumb_color_active_ : thumb_color_;

			renderer.draw_circle_filled(thumb_pos, thumb_radius_, thumb_col);
		}

		shared_ptr_t<input> input_;
		function_t<void(float)> on_change_;

		float target_value_ = 0.f;
		float visual_value_ = 0.f;
		bool dragging_ = false;

		float thumb_radius_ = 8.f;
		color fill_color_ = { 0.4f, 0.7f, 1.f, 1.f };
		color thumb_color_ = { 1.f, 1.f, 1.f, 1.f };
		color thumb_color_active_ = { 0.9f, 0.9f, 0.9f, 1.f };
	};

	class range_slider : public slider
	{
	public:
		range_slider(const element_size size, shared_ptr_t<input> input) noexcept
				:	slider(size, cstd::move(input), true) { }

		range_slider& on_range_change(function_t<void(float, float)> callback)
		{
			on_range_change_ = cstd::move(callback);

			return *this;
		}

		range_slider& values(const float low, const float high)
		{
			target_low_ = low;
			target_high_ = high;
			visual_low_ = low;
			visual_high_ = high;

			return *this;
		}

		[[nodiscard]] float low() const noexcept
		{
			return target_low_;
		}

		[[nodiscard]] float high() const noexcept
		{
			return target_high_;
		}

		bool on_mouse_click() override
		{
			dragging_ = true;

			const position mouse_pos = input_->mouse_pos();
			const float track_x = computed_pos_.x + thumb_radius_;
			const float track_width = computed_size_.x - thumb_radius_ * 2.f;

			if (track_width <= 0.f)
			{
				return true;
			}

			const float click_t = (mouse_pos.x - track_x) / track_width;
			const float dist_low = cstd::fabsf(click_t - target_low_);
			const float dist_high = cstd::fabsf(click_t - target_high_);

			active_ = (dist_low <= dist_high) ? active_thumb::low : active_thumb::high;

			update_range_from_mouse();

			return true;
		}

		void update(const float dt) override
		{
			element::update(dt);

			if (dragging_)
			{
				if (!input_->is_mouse_down(0))
				{
					dragging_ = false;
					active_ = active_thumb::none;
				}
				else
				{
					update_range_from_mouse();
				}
			}

			const float speed = style_.transition_speed.value_or(12.f);
			const float factor = cstd::fminf(speed * dt, 1.f);

			const float diff_low = target_low_ - visual_low_;
			const float diff_high = target_high_ - visual_high_;

			if (diff_low != 0.f)
			{
				visual_low_ += diff_low * factor;

				if (cstd::fabsf(diff_low) < 0.001f)
				{
					visual_low_ = target_low_;
				}
			}

			if (diff_high != 0.f)
			{
				visual_high_ += diff_high * factor;

				if (cstd::fabsf(diff_high) < 0.001f)
				{
					visual_high_ = target_high_;
				}
			}
		}

	protected:
		enum class active_thumb : cstd::uint8_t
		{
			none,
			low,
			high
		};

		void update_range_from_mouse()
		{
			const position mouse_pos = input_->mouse_pos();
			const float track_x = computed_pos_.x + thumb_radius_;
			const float track_width = computed_size_.x - thumb_radius_ * 2.f;

			if (track_width <= 0.f)
			{
				return;
			}

			const float raw = (mouse_pos.x - track_x) / track_width;
			const float clamped = cstd::fmaxf(0.f, cstd::fminf(1.f, raw));

			bool changed = false;

			if (active_ == active_thumb::low)
			{
				const float new_low = cstd::fminf(clamped, target_high_);

				if (new_low != target_low_)
				{
					target_low_ = new_low;
					changed = true;
				}
			}
			else if (active_ == active_thumb::high)
			{
				const float new_high = cstd::fmaxf(clamped, target_low_);

				if (new_high != target_high_)
				{
					target_high_ = new_high;
					changed = true;
				}
			}

			if (changed && on_range_change_)
			{
				on_range_change_(target_low_, target_high_);
			}
		}

		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			const float height = max.y - min.y;
			const float center_y = min.y + height * 0.5f;
			const float track_height = 4.f;
			const color track_col = style_.background_color.value_or(color{ 0.3f, 0.3f, 0.3f, 1.f });

			const position track_min = { min.x + thumb_radius_, center_y - track_height * 0.5f };
			const position track_max = { max.x - thumb_radius_, center_y + track_height * 0.5f };

			renderer.draw_rect_filled(track_min, track_max, track_col, track_height * 0.5f);

			const float track_width = track_max.x - track_min.x;
			const position fill_min = { track_min.x + track_width * visual_low_, track_min.y };
			const position fill_max = { track_min.x + track_width * visual_high_, track_max.y };

			if (visual_high_ > visual_low_)
			{
				renderer.draw_rect_filled(fill_min, fill_max, fill_color_, track_height * 0.5f);
			}

			const float low_x = track_min.x + track_width * visual_low_;
			const float high_x = track_min.x + track_width * visual_high_;

			const color low_col = (hovered_ || (dragging_ && active_ == active_thumb::low))
				? thumb_color_active_ : thumb_color_;
			const color high_col = (hovered_ || (dragging_ && active_ == active_thumb::high))
				? thumb_color_active_ : thumb_color_;

			renderer.draw_circle_filled(position{ low_x, center_y }, thumb_radius_, low_col);
			renderer.draw_circle_filled(position{ high_x, center_y }, thumb_radius_, high_col);
		}

		function_t<void(float, float)> on_range_change_;

		float target_low_ = 0.25f;
		float target_high_ = 0.75f;
		float visual_low_ = 0.25f;
		float visual_high_ = 0.75f;

		active_thumb active_ = active_thumb::none;
	};
}
