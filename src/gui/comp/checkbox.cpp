#include "checkbox.hpp"

namespace rv
{
	vector_2d<float> checkbox::content_size(const vector_2d<float> available) const noexcept
	{
		if (!font_ || label_.empty())
		{
			return { 0.f, 0.f };
		}

		const float scale = resolved_scale();
		const float line_h = font_->line_height() * scale;

		return { line_h + gap_ + measure_text_width(*font_, label_, scale), line_h };
	}

	void checkbox::update(const float dt)
	{
		if (bound_ && *bound_ != checked_)
		{
			checked_ = *bound_;
		}

		// element::update animates visual_text_color_ from style_.text_color,
		// which we use for the label. the box fill is animated separately
		// below since the element itself draws no background.
		element::update(dt);

		const color target_fill = pressed_ ? pressed_color_ : (hovered_ ? hover_color_ : box_color_);
		box_fill_.step(target_fill, style_.transition_speed.value_or(12.f), dt);

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

	void checkbox::render_self(gui_renderer& renderer, const position min, const position max) const
	{
		const float h = max.y - min.y;

		// the box is a square on the left; with no label it fills the element.
		const float box_side = label_.empty() ? cstd::fminf(max.x - min.x, h) : h;
		const position box_min = { min.x, min.y + (h - box_side) * 0.5f };
		const position box_max = { box_min.x + box_side, box_min.y + box_side };

		const float rounding = style_.rounding.value_or(0.f);

		renderer.draw_rect_filled(box_min, box_max, box_fill_.current, rounding);

		if (border_color_.a > 0.001f && border_width_ > 0.f)
		{
			renderer.draw_rect(box_min, box_max, border_color_, border_width_, rounding);
		}

		if (visual_t_ > 0.f)
		{
			const float cx = (box_min.x + box_max.x) * 0.5f;
			const float cy = (box_min.y + box_max.y) * 0.5f;
			const float half = (box_side * 0.5f) * visual_t_;
			const float inner_rounding = rounding * visual_t_;

			renderer.draw_rect_filled(
				{ cx - half, cy - half },
				{ cx + half, cy + half },
				check_color_,
				inner_rounding
			);
		}

		if (font_ && !label_.empty())
		{
			const float scale = resolved_scale();

			// draw_text positions pos.y at the top of the line box, so center
			// the glyph block (ascent..descent) against the box rather than the
			// full line height, line_gap would otherwise push the text high.
			const float glyph_h = (font_->ascent() - font_->descent()) * scale;
			const float x = box_max.x + gap_;
			const float y = min.y + (h - glyph_h) * 0.5f;

			renderer.draw_text(*font_, { x, y }, label_, visual_text_color_, style_.font_size.value_or(0.f));
		}
	}

}
