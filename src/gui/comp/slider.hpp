#pragma once
#include "../element.hpp"
#include "../../input/input.hpp"

namespace rv
{
	template <class T = float>
	class slider : public element
	{
	public:
		slider(const element_size size, shared_ptr_t<input> input) noexcept
				:	element(size), input_(cstd::move(input))
		{
			init_slider_defaults();
		}

		slider& on_change(function_t<void(T)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		slider& value(const T v)
		{
			target_value_ = clamp_value(v);
			visual_t_ = normalize(target_value_);

			return *this;
		}

		slider& range(const T min_v, const T max_v)
		{
			min_value_ = min_v;
			max_value_ = max_v;
			target_value_ = clamp_value(target_value_);
			visual_t_ = normalize(target_value_);

			return *this;
		}

		[[nodiscard]] T value() const noexcept
		{
			return target_value_;
		}

		[[nodiscard]] T min_value() const noexcept
		{
			return min_value_;
		}

		[[nodiscard]] T max_value() const noexcept
		{
			return max_value_;
		}

		slider& bind(T* ptr) noexcept
		{
			bound_ = ptr;

			if (bound_)
			{
				target_value_ = clamp_value(*bound_);
				visual_t_ = normalize(target_value_);
			}

			return *this;
		}

		slider& track_color(const color col) noexcept
		{
			track_color_ = col;

			return *this;
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

		slider& thumb_size(const float w, const float h) noexcept
		{
			thumb_width_ = w;
			thumb_height_ = h;

			return *this;
		}

		slider& thumb_rounding(const float r) noexcept
		{
			thumb_rounding_ = r;

			return *this;
		}

		slider& extend_track(const bool v = true) noexcept
		{
			extend_track_ = v;

			if (v)
			{
				saved_padding_ = style_.padding.value_or(border_vector{});
				saved_rounding_ = style_.rounding.value_or(0.f);

				style_.background_color.reset();
				style_.padding = border_vector{ 0.f, 0.f, 0.f, 0.f };
				style_.rounding.reset();
			}

			return *this;
		}

		slider& background_color(const color c, const element_state state = element_state::normal) noexcept
		{
			element::background_color(c, state);

			if (extend_track_ && state == element_state::normal)
			{
				style_.padding = saved_padding_;
				style_.rounding = saved_rounding_;
			}

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

			if (bound_ && !dragging_)
			{
				const T ext = clamp_value(*bound_);

				if (ext != target_value_)
				{
					target_value_ = ext;
				}
			}

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

			const float target_t = normalize(target_value_);
			const float diff = target_t - visual_t_;

			if (diff != 0.f)
			{
				const float speed = style_.transition_speed.value_or(12.f);
				const float factor = cstd::fminf(speed * dt, 1.f);
				visual_t_ += diff * factor;

				if (cstd::fabsf(diff) < 0.001f)
				{
					visual_t_ = target_t;
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

		[[nodiscard]] float thumb_half_width() const noexcept
		{
			return thumb_width_ > 0.f ? thumb_width_ * 0.5f : thumb_radius_;
		}

		void draw_thumb(gui_renderer& renderer, const float cx, const float cy, const color col) const
		{
			if (thumb_width_ > 0.f)
			{
				const float hw = thumb_width_ * 0.5f;
				const float hh = (thumb_height_ > 0.f ? thumb_height_ : thumb_width_) * 0.5f;
				renderer.draw_rect_filled({ cx - hw, cy - hh }, { cx + hw, cy + hh }, col, thumb_rounding_);
			}
			else
			{
				renderer.draw_circle_filled({ cx, cy }, thumb_radius_, col);
			}
		}

		[[nodiscard]] float normalize(const T v) const noexcept
		{
			if (max_value_ == min_value_)
			{
				return 0.f;
			}

			return static_cast<float>(v - min_value_) / static_cast<float>(max_value_ - min_value_);
		}

		[[nodiscard]] T denormalize(const float t) const noexcept
		{
			if constexpr (cstd::is_floating_point_v<T>)
			{
				return min_value_ + static_cast<T>(t) * (max_value_ - min_value_);
			}
			else
			{
				return static_cast<T>(static_cast<float>(min_value_) + t * static_cast<float>(max_value_ - min_value_) + 0.5f);
			}
		}

		[[nodiscard]] T clamp_value(const T v) const noexcept
		{
			if (v < min_value_) return min_value_;
			if (v > max_value_) return max_value_;
			return v;
		}

		void update_value_from_mouse()
		{
			const position mouse_pos = input_->mouse_pos();
			const float hw = thumb_half_width();
			const float track_x = visual_pos().x + hw;
			const float track_width = computed_size_.x - hw * 2.f;

			if (track_width <= 0.f)
			{
				return;
			}

			const float raw = (mouse_pos.x - track_x) / track_width;
			const float clamped = cstd::fmaxf(0.f, cstd::fminf(1.f, raw));
			const T new_value = denormalize(clamped);

			if (new_value != target_value_)
			{
				target_value_ = new_value;

				if (bound_)
				{
					*bound_ = target_value_;
				}

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
			const color track_col = track_color_;
			const float hw = thumb_half_width();

			const float track_inset = extend_track_ ? 0.f : hw;
			const position track_min = { min.x + track_inset, center_y - track_height * 0.5f };
			const position track_max = { max.x - track_inset, center_y + track_height * 0.5f };

			renderer.draw_rect_filled(track_min, track_max, track_col, track_height * 0.5f);

			const float thumb_min_x = min.x + hw;
			const float thumb_range = (max.x - hw) - thumb_min_x;
			const float thumb_x = thumb_min_x + thumb_range * visual_t_;

			if (visual_t_ > 0.f)
			{
				const position fill_max = { thumb_x, track_max.y };
				renderer.draw_rect_filled(track_min, fill_max, fill_color_, track_height * 0.5f);
			}

			const color thumb_col = (hovered_ || dragging_) ? thumb_color_active_ : thumb_color_;
			draw_thumb(renderer, thumb_x, center_y, thumb_col);
		}

		shared_ptr_t<input> input_;
		function_t<void(T)> on_change_;

		T min_value_ = static_cast<T>(0);
		T max_value_ = static_cast<T>(1);
		T target_value_ = static_cast<T>(0);
		float visual_t_ = 0.f;
		bool dragging_ = false;
		T* bound_ = nullptr;

		float thumb_radius_ = 6.f;
		float thumb_width_ = 0.f;
		float thumb_height_ = 0.f;
		float thumb_rounding_ = 0.f;
		color track_color_ = { 0.3f, 0.3f, 0.3f, 1.f };
		color fill_color_ = { 0.4f, 0.7f, 1.f, 1.f };
		color thumb_color_ = { 1.f, 1.f, 1.f, 1.f };
		color thumb_color_active_ = { 0.9f, 0.9f, 0.9f, 1.f };
		bool extend_track_ = false;
		border_vector saved_padding_;
		float saved_rounding_ = 0.f;
	};

	template <class T = float>
	class range_slider : public slider<T>
	{
		using base = slider<T>;

	public:
		range_slider(const element_size size, shared_ptr_t<input> input) noexcept
				:	base(size, cstd::move(input), true) { }

		range_slider& on_range_change(function_t<void(T, T)> callback)
		{
			on_range_change_ = cstd::move(callback);

			return *this;
		}

		range_slider& bind(T* low, T* high) noexcept
		{
			bound_low_ = low;
			bound_high_ = high;

			if (bound_low_)
			{
				target_low_ = base::clamp_value(*bound_low_);
				visual_low_t_ = base::normalize(target_low_);
			}

			if (bound_high_)
			{
				target_high_ = base::clamp_value(*bound_high_);
				visual_high_t_ = base::normalize(target_high_);
			}

			return *this;
		}

		range_slider& values(const T low, const T high)
		{
			target_low_ = base::clamp_value(low);
			target_high_ = base::clamp_value(high);
			visual_low_t_ = base::normalize(target_low_);
			visual_high_t_ = base::normalize(target_high_);

			return *this;
		}

		[[nodiscard]] T low() const noexcept
		{
			return target_low_;
		}

		[[nodiscard]] T high() const noexcept
		{
			return target_high_;
		}

		bool on_mouse_click() override
		{
			base::dragging_ = true;

			const position mouse_pos = base::input_->mouse_pos();
			const float hw = base::thumb_half_width();
			const float track_x = base::visual_pos().x + hw;
			const float track_width = base::computed_size_.x - hw * 2.f;

			if (track_width <= 0.f)
			{
				return true;
			}

			const float click_t = (mouse_pos.x - track_x) / track_width;
			const float dist_low = cstd::fabsf(click_t - visual_low_t_);
			const float dist_high = cstd::fabsf(click_t - visual_high_t_);

			active_ = (dist_low <= dist_high) ? active_thumb::low : active_thumb::high;

			update_range_from_mouse();

			return true;
		}

		void update(const float dt) override
		{
			element::update(dt);

			if (!base::dragging_)
			{
				if (bound_low_)
				{
					const T ext = base::clamp_value(*bound_low_);

					if (ext != target_low_)
					{
						target_low_ = ext;
					}
				}

				if (bound_high_)
				{
					const T ext = base::clamp_value(*bound_high_);

					if (ext != target_high_)
					{
						target_high_ = ext;
					}
				}
			}

			if (base::dragging_)
			{
				if (!base::input_->is_mouse_down(0))
				{
					base::dragging_ = false;
					active_ = active_thumb::none;
				}
				else
				{
					update_range_from_mouse();
				}
			}

			const float speed = base::style_.transition_speed.value_or(12.f);
			const float factor = cstd::fminf(speed * dt, 1.f);

			const float target_low_t = base::normalize(target_low_);
			const float target_high_t = base::normalize(target_high_);

			const float diff_low = target_low_t - visual_low_t_;
			const float diff_high = target_high_t - visual_high_t_;

			if (diff_low != 0.f)
			{
				visual_low_t_ += diff_low * factor;

				if (cstd::fabsf(diff_low) < 0.001f)
				{
					visual_low_t_ = target_low_t;
				}
			}

			if (diff_high != 0.f)
			{
				visual_high_t_ += diff_high * factor;

				if (cstd::fabsf(diff_high) < 0.001f)
				{
					visual_high_t_ = target_high_t;
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
			const position mouse_pos = base::input_->mouse_pos();
			const float hw = base::thumb_half_width();
			const float track_x = base::visual_pos().x + hw;
			const float track_width = base::computed_size_.x - hw * 2.f;

			if (track_width <= 0.f)
			{
				return;
			}

			const float raw = (mouse_pos.x - track_x) / track_width;
			const float clamped = cstd::fmaxf(0.f, cstd::fminf(1.f, raw));
			const T new_value = base::denormalize(clamped);

			bool changed = false;

			if (active_ == active_thumb::low)
			{
				const T capped = (new_value < target_high_) ? new_value : target_high_;

				if (capped != target_low_)
				{
					target_low_ = capped;
					changed = true;
				}
			}
			else if (active_ == active_thumb::high)
			{
				const T capped = (new_value > target_low_) ? new_value : target_low_;

				if (capped != target_high_)
				{
					target_high_ = capped;
					changed = true;
				}
			}

			if (changed)
			{
				if (bound_low_)
				{
					*bound_low_ = target_low_;
				}

				if (bound_high_)
				{
					*bound_high_ = target_high_;
				}

				if (on_range_change_)
				{
					on_range_change_(target_low_, target_high_);
				}
			}
		}

		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			const float height = max.y - min.y;
			const float center_y = min.y + height * 0.5f;
			const float track_height = 4.f;
			const color track_col = base::track_color_;
			const float hw = base::thumb_half_width();

			const float track_inset = base::extend_track_ ? 0.f : hw;
			const position track_min = { min.x + track_inset, center_y - track_height * 0.5f };
			const position track_max = { max.x - track_inset, center_y + track_height * 0.5f };

			renderer.draw_rect_filled(track_min, track_max, track_col, track_height * 0.5f);

			const float thumb_min_x = min.x + hw;
			const float thumb_range = (max.x - hw) - thumb_min_x;
			const float low_x = thumb_min_x + thumb_range * visual_low_t_;
			const float high_x = thumb_min_x + thumb_range * visual_high_t_;

			if (visual_high_t_ > visual_low_t_)
			{
				const position fill_min = { low_x, track_min.y };
				const position fill_max = { high_x, track_max.y };
				renderer.draw_rect_filled(fill_min, fill_max, base::fill_color_, track_height * 0.5f);
			}

			const color low_col = (base::hovered_ || (base::dragging_ && active_ == active_thumb::low))
				? base::thumb_color_active_ : base::thumb_color_;
			const color high_col = (base::hovered_ || (base::dragging_ && active_ == active_thumb::high))
				? base::thumb_color_active_ : base::thumb_color_;

			base::draw_thumb(renderer, low_x, center_y, low_col);
			base::draw_thumb(renderer, high_x, center_y, high_col);
		}

		function_t<void(T, T)> on_range_change_;

		T target_low_ = static_cast<T>(0);
		T target_high_ = static_cast<T>(1);
		float visual_low_t_ = 0.f;
		float visual_high_t_ = 1.f;

		T* bound_low_ = nullptr;
		T* bound_high_ = nullptr;

		active_thumb active_ = active_thumb::none;
	};
}
