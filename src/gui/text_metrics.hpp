#pragma once
#include "gui.hpp"
#include "../util/types.hpp"
#include "../util/string.hpp"

namespace rv
{
	// per-glyph pen advance: glyph advance + kerning vs the previous glyph, plus letter_spacing.
	// letter_spacing is added per glyph (matching draw_text), so measurement and drawing agree.
	[[nodiscard]] inline float glyph_step(const gui_font& font, const cstd::uint32_t prev,
	                                      const cstd::uint32_t cp, const float scale,
	                                      const float letter_spacing = 0.f) noexcept
	{
		float adv = font.glyph_advance(cp) * scale;

		if (prev != 0)
		{
			adv += font.kerning(prev, cp) * scale;
		}

		return adv + letter_spacing;
	}

	// total advance width of a UTF-8 string at the given scale (plus optional letter_spacing). shared
	// by widgets that measure label/content widths, replacing per-widget UTF-8 measurement loops.
	[[nodiscard]] inline float measure_text_width(const gui_font& font, const string_view_t text,
	                                              const float scale, const float letter_spacing = 0.f) noexcept
	{
		float width = 0.f;
		cstd::uint32_t prev = 0;

		const char* s = text.data();
		const char* const end = s + text.size();

		while (s < end)
		{
			const cstd::uint32_t cp = decode_utf8(s, end);
			width += glyph_step(font, prev, cp, scale, letter_spacing);
			prev = cp;
		}

		return width;
	}
}
