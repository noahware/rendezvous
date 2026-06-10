#include "progress_bar.hpp"

namespace rv
{
	void progress_bar::update(const float dt)
	{
		if (bound_ && *bound_ != value_)
		{
			const float old = value_;
			value_ = clamp01(*bound_);

			if (on_change_ && old != value_)
			{
				on_change_(value_);
			}
		}

		element::update(dt);

		const float diff = value_ - visual_value_;

		if (diff != 0.f)
		{
			const float speed = style_.transition_speed.value_or(12.f);
			visual_value_ += diff * cstd::fminf(speed * dt, 1.f);

			if (cstd::fabsf(diff) < 0.001f)
			{
				visual_value_ = value_;
			}
		}
	}

	void progress_bar::render_self(gui_renderer& renderer, const position min, const position max) const
	{
		const float rounding = style_.rounding.value_or(0.f);
		const float w = max.x - min.x;

		renderer.draw_rect_filled(min, max, track_color_, rounding);

		const float fill_w = w * visual_value_;

		if (fill_w > 0.5f)
		{
			const position fill_max = { min.x + fill_w, max.y };

			const bool nearly_full = visual_value_ >= 0.99f;
			const auto flags = nearly_full
				? rounding_flags_all
				: static_cast<rounding_flags>(rounding_flags_top_left | rounding_flags_bottom_left);

			const float fill_r = cstd::fminf(rounding, fill_w * 0.5f);
			renderer.draw_rect_filled(min, fill_max, fill_color_, fill_r, flags);
		}

		if (show_text_ && font_)
		{
			const string_t text = text_format_
				? text_format_(value_)
				: cstd::format("{:.0f}%", value_ * 100.f);

			const float h = max.y - min.y;
			const float max_text_h = h * 0.7f;
			const float raw_glyph_h = font_->ascent() - font_->descent();
			const float fit_size = max_text_h / raw_glyph_h * font_->baked_size();

			const float font_size = (style_.font_size.value_or(0.f) > 0.f)
				? cstd::fminf(style_.font_size.value_or(0.f), fit_size)
				: fit_size;

			const float scale = font_size / font_->baked_size();
			const float tw = measure_text_width(*font_, text, scale);
			const float glyph_h = raw_glyph_h * scale;

			const float tx = min.x + (w - tw) * 0.5f;
			const float ty = min.y + (h - glyph_h) * 0.5f;

			renderer.draw_text(*font_, { tx, ty }, text, visual_text_color_, font_size);
		}
	}
}
