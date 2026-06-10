#include "radio_button.hpp"

namespace rv
{
	vector_2d<float> radio_button::content_size(const vector_2d<float> available) const noexcept
	{
		if (!font_ || label_.empty())
		{
			return { 0.f, 0.f };
		}

		const float scale = resolved_scale();
		const float line_h = font_->line_height() * scale;

		return { line_h + gap_ + measure_text_width(*font_, label_, scale), line_h };
	}

	void radio_button::update(const float dt)
	{
		if (bound_)
		{
			selected_ = (*bound_ == value_);
		}

		element::update(dt);

		const color target_fill = pressed_ ? pressed_color_ : (hovered_ ? hover_color_ : circle_color_);
		circle_fill_.step(target_fill, style_.transition_speed.value_or(12.f), dt);

		const float target_t = selected_ ? 1.f : 0.f;
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

	void radio_button::render_self(gui_renderer& renderer, const position min, const position max) const
	{
		const float h = max.y - min.y;
		const float diameter = label_.empty() ? cstd::fminf(max.x - min.x, h) : h;
		const float radius = diameter * 0.5f;
		const float cx = min.x + radius;
		const float cy = min.y + h * 0.5f;

		renderer.draw_circle_filled({ cx, cy }, radius, circle_fill_.current);

		if (border_color_.a > 0.001f && border_width_ > 0.f)
		{
			renderer.draw_circle({ cx, cy }, radius, border_color_, border_width_);
		}

		if (visual_t_ > 0.f)
		{
			const float dot_radius = radius * 0.45f * visual_t_;
			renderer.draw_circle_filled({ cx, cy }, dot_radius, dot_color_);
		}

		if (font_ && !label_.empty())
		{
			const float scale = resolved_scale();
			const float glyph_h = (font_->ascent() - font_->descent()) * scale;
			const float x = min.x + diameter + gap_;
			const float y = min.y + (h - glyph_h) * 0.5f;

			renderer.draw_text(*font_, { x, y }, label_, visual_text_color_, style_.font_size.value_or(0.f));
		}
	}
}
