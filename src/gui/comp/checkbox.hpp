#pragma once
#include "../element.hpp"

namespace rv
{
	class checkbox final : public element
	{
	public:
		checkbox() noexcept = default;

		explicit checkbox(const element_size size) noexcept
			: element(size)
		{
			init_defaults();
		}

		checkbox& checked(const bool state) noexcept
		{
			checked_ = state;
			visual_t_ = state ? 1.f : 0.f;

			if (bound_)
			{
				*bound_ = checked_;
			}

			return *this;
		}

		[[nodiscard]] bool is_checked() const noexcept
		{
			return checked_;
		}

		checkbox& on_change(function_t<void(bool)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		checkbox& hover_color(const color col) noexcept
		{
			hover_color_ = col;

			return *this;
		}

		checkbox& pressed_color(const color col) noexcept
		{
			pressed_color_ = col;

			return *this;
		}

		checkbox& bind(bool* ptr) noexcept
		{
			bound_ = ptr;

			if (bound_)
			{
				checked_ = *bound_;
				visual_t_ = checked_ ? 1.f : 0.f;
			}

			return *this;
		}

		bool on_mouse_click() override
		{
			checked_ = !checked_;

			if (bound_)
			{
				*bound_ = checked_;
			}

			if (on_change_)
			{
				on_change_(checked_);
			}

			return true;
		}

		void update(const float dt) override
		{
			if (bound_ && *bound_ != checked_)
			{
				checked_ = *bound_;
			}

			const auto resting = style_.background_color;

			if (pressed_)
			{
				style_.background_color = pressed_color_;
			}
			else if (hovered_)
			{
				style_.background_color = hover_color_;
			}

			element::update(dt);

			style_.background_color = resting;

			const float target_t = checked_ ? 1.f : 0.f;
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
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			if (visual_t_ <= 0.f)
			{
				return;
			}

			const float w = max.x - min.x;
			const float h = max.y - min.y;
			const float cx = min.x + w * 0.5f;
			const float cy = min.y + h * 0.5f;

			const float half_w = (w * 0.5f) * visual_t_;
			const float half_h = (h * 0.5f) * visual_t_;

			const float inner_rounding = style_.rounding.value_or(0.f) * visual_t_;

			renderer.draw_rect_filled(
				{ cx - half_w, cy - half_h },
				{ cx + half_w, cy + half_h },
				visual_text_color_,
				inner_rounding
			);
		}

	private:
		void init_defaults() noexcept
		{
			style_.background_color = color{ 0.15f, 0.15f, 0.18f, 1.f };
			style_.rounding = 4.f;
			style_.border_color = color{ 0.4f, 0.4f, 0.45f, 1.f };
			style_.border_width = border_vector{ 1.5f, 1.5f, 1.5f, 1.5f };
			style_.text_color = color{ 0.4f, 0.7f, 1.f, 1.f };
			style_.transition_speed = 12.f;
		}

		bool checked_ = false;
		float visual_t_ = 0.f;

		color hover_color_ = { 0.22f, 0.22f, 0.26f, 1.f };
		color pressed_color_ = { 0.1f, 0.1f, 0.12f, 1.f };

		function_t<void(bool)> on_change_;
		bool* bound_ = nullptr;
	};
}
