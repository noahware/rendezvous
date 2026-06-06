#include "text.hpp"

namespace rv
{
	vector_2d<float> text_element::content_size(const vector_2d<float> available) const noexcept
	{
		if (!font_ || text_.empty())
		{
			return { 0.f, 0.f };
		}

		const float scale = resolved_scale();
		const float line_h = font_->line_height() * scale * style_.line_height.value_or(1.f);
		const bool has_width_constraint = (declared_size().width.mode != size_mode::auto_v);

		if (!has_width_constraint)
		{
			const float w = single_line_width(scale);
			return { w, line_h };
		}

		// ellipsized text stays on one line; it fills the width constraint and is one line tall.
		if (style_.text_ellipsis.value_or(false))
		{
			return { available.x, line_h };
		}

		update_wrap_cache(available.x, scale);
		float max_w = 0.f;

		for (const auto& line : cached_wrapped_)
		{
			max_w = cstd::fmaxf(max_w, measure_line(line, scale));
		}

		return { max_w, line_h * static_cast<float>(cached_wrapped_.size()) };
	}

	void text_element::render_self(gui_renderer& renderer, const position min, const position max) const
	{
		if (!font_ || text_.empty())
		{
			return;
		}

		const float scale = resolved_scale();
		const float ls = style_.letter_spacing.value_or(0.f);
		const float font_px = style_.font_size.value_or(0.f);
		const float line_h = font_->line_height() * scale * style_.line_height.value_or(1.f);
		const float box_w = max.x - min.x;

		// underline / strikethrough drawn as a thin filled rect from the text metrics.
		const auto draw_decoration = [&](const float x, const float y, const float line_w)
		{
			const text_decoration deco = style_.decoration.value_or(text_decoration::none);

			if (deco == text_decoration::none || line_w <= 0.f)
			{
				return;
			}

			const float thickness = cstd::fmaxf(1.f, scale);

			// underline sits just below the baseline; strikethrough crosses the x-height.
			const float dy = (deco == text_decoration::underline)
				? y + font_->ascent() * scale + thickness
				: y + font_->ascent() * scale * 0.65f;

			renderer.draw_rect_filled({ x, dy }, { x + line_w, dy + thickness }, visual_text_color_);
		};

		const auto aligned_x = [&](const float line_w)
		{
			const text_align al = style_.text_alignment.value_or(text_align::left);

			if (al == text_align::center) { return min.x + (box_w - line_w) * 0.5f; }
			if (al == text_align::right)  { return min.x + box_w - line_w; }

			return min.x;
		};

		// auto width: a single unconstrained line.
		if (declared_size().width.mode == size_mode::auto_v)
		{
			renderer.draw_text(*font_, { min.x, min.y }, text_, visual_text_color_, font_px, ls);
			draw_decoration(min.x, min.y, single_line_width(scale));
			return;
		}

		// constrained width with ellipsis: one line truncated to fit, suffixed with "...".
		if (style_.text_ellipsis.value_or(false))
		{
			const string_t clipped = ellipsize(box_w, scale, ls);
			const float lw = measure_line(clipped, scale);
			const float x = aligned_x(lw);

			renderer.push_clip_rect(min, max);
			renderer.draw_text(*font_, { x, min.y }, clipped, visual_text_color_, font_px, ls);
			draw_decoration(x, min.y, lw);
			renderer.pop_clip_rect();
			return;
		}

		// constrained width: wrap into lines.
		update_wrap_cache(box_w, scale);

		renderer.push_clip_rect(min, max);

		float y = min.y;

		for (const auto& line : cached_wrapped_)
		{
			const float lw = measure_line(line, scale);
			const float x = aligned_x(lw);

			renderer.draw_text(*font_, { x, y }, line, visual_text_color_, font_px, ls);
			draw_decoration(x, y, lw);
			y += line_h;
		}

		renderer.pop_clip_rect();
	}

	float text_element::single_line_width(const float scale) const noexcept
	{
		float width = 0.f;
		cstd::uint32_t prev_cp = 0;

		const char* s = text_.data();
		const char* end = s + text_.size();

		while (s < end)
		{
			const cstd::uint32_t cp = decode_utf8(s, end);

			if (cp == '\n' || cp == '\r')
			{
				prev_cp = 0;
				continue;
			}

			width += glyph_step(*font_, prev_cp, cp, scale, style_.letter_spacing.value_or(0.f));
			prev_cp = cp;
		}

		return width;
	}

	float text_element::measure_line(const string_view_t line, const float scale) const noexcept
	{
		float width = 0.f;
		cstd::uint32_t prev_cp = 0;

		const char* s = line.data();
		const char* end = s + line.size();

		while (s < end)
		{
			const cstd::uint32_t cp = decode_utf8(s, end);

			width += glyph_step(*font_, prev_cp, cp, scale, style_.letter_spacing.value_or(0.f));
			prev_cp = cp;
		}

		return width;
	}

	vector_t<string_view_t> text_element::wrap_text(const float max_width, const float scale) const noexcept
	{
		vector_t<string_view_t> lines;

		if (text_.empty() || max_width <= 0.f)
		{
			return lines;
		}

		const char* text_start = text_.data();
		const char* text_end = text_start + text_.size();

		const char* line_start = text_start;
		const char* s = text_start;
		float current_width = 0.f;
		cstd::uint32_t prev_cp = 0;

		const char* last_break = nullptr;
		float width_at_break = 0.f;

		while (s < text_end)
		{
			const char* char_start = s;
			const cstd::uint32_t cp = decode_utf8(s, text_end);

			// forced line break
			if (cp == '\n')
			{
				lines.emplace_back(line_start, static_cast<cstd::size_t>(char_start - line_start));
				line_start = s;
				current_width = 0.f;
				prev_cp = 0;
				last_break = nullptr;
				continue;
			}

			if (cp == '\r')
			{
				prev_cp = 0;
				continue;
			}

			// record break opportunity at spaces
			if (cp == ' ' || cp == '\t')
			{
				last_break = s; // position AFTER the space
				width_at_break = current_width;
			}

			// accumulate width
			current_width += glyph_step(*font_, prev_cp, cp, scale, style_.letter_spacing.value_or(0.f));
			prev_cp = cp;

			// check overflow
			if (current_width > max_width && char_start != line_start)
			{
				if (last_break)
				{
					// break at last space
					const cstd::size_t len = static_cast<cstd::size_t>(last_break - line_start);

					// trim trailing space from line
					const char* line_end = last_break;
					while (line_end > line_start && *(line_end - 1) == ' ')
					{
						--line_end;
					}

					lines.emplace_back(line_start, static_cast<cstd::size_t>(line_end - line_start));
					line_start = last_break;
					current_width = current_width - width_at_break;
					last_break = nullptr;
					width_at_break = 0.f;

					// re-measure from last_break to current position for correct kerning
					// (simplified: current_width approximation is acceptable for now)
				}
				else
				{
					// no break point - force break at current char
					lines.emplace_back(line_start, static_cast<cstd::size_t>(char_start - line_start));
					line_start = char_start;
					current_width = glyph_step(*font_, 0, cp, scale, style_.letter_spacing.value_or(0.f));
					prev_cp = cp;
					last_break = nullptr;
					width_at_break = 0.f;
				}
			}
		}

		// emit final line
		if (line_start < text_end)
		{
			lines.emplace_back(line_start, static_cast<cstd::size_t>(text_end - line_start));
		}

		return lines;
	}

	void text_element::update_wrap_cache(const float width, const float scale) const noexcept
	{
		if (wrap_cache_valid_ && width == cached_wrap_width_ && scale == cached_wrap_scale_)
		{
			return;
		}

		cached_wrapped_ = wrap_text(width, scale);
		cached_wrap_width_ = width;
		cached_wrap_scale_ = scale;
		wrap_cache_valid_ = true;
	}

	string_t text_element::ellipsize(const float max_width, const float scale, const float letter_spacing) const noexcept
	{
		const char* const s = text_.data();
		const char* const text_end = s + text_.size();

		// only the first line participates in single-line ellipsis.
		const char* line_end = s;
		while (line_end < text_end && *line_end != '\n' && *line_end != '\r')
		{
			++line_end;
		}

		const bool whole_text_one_line = (line_end == text_end);

		// width of the first line as-is.
		float line_w = 0.f;
		{
			cstd::uint32_t prev = 0;
			const char* p = s;

			while (p < line_end)
			{
				const cstd::uint32_t cp = decode_utf8(p, line_end);
				line_w += glyph_step(*font_, prev, cp, scale, letter_spacing);
				prev = cp;
			}
		}

		// it fits and there is nothing after it: no truncation needed.
		if (line_w <= max_width && whole_text_one_line)
		{
			return string_t(s, static_cast<cstd::size_t>(line_end - s));
		}

		// width of the "..." suffix (ASCII dots stay inside the typical baked glyph range).
		float ell_w = 0.f;
		{
			cstd::uint32_t prev = 0;

			for (const char* p = "..."; *p != '\0'; ++p)
			{
				const cstd::uint32_t cp = static_cast<cstd::uint32_t>(static_cast<cstd::uint8_t>(*p));
				ell_w += glyph_step(*font_, prev, cp, scale, letter_spacing);
				prev = cp;
			}
		}

		// accumulate glyphs until prefix + ellipsis would overflow the box.
		float width = 0.f;
		cstd::uint32_t prev_cp = 0;
		const char* fit_end = s;
		const char* cursor = s;

		while (cursor < line_end)
		{
			const cstd::uint32_t cp = decode_utf8(cursor, line_end);
			const float step = glyph_step(*font_, prev_cp, cp, scale, letter_spacing);

			if (width + step + ell_w > max_width)
			{
				break;
			}

			width += step;
			prev_cp = cp;
			fit_end = cursor;
		}

		string_t out(s, static_cast<cstd::size_t>(fit_end - s));
		out += "...";

		return out;
	}
}
