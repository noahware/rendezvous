#pragma once
#include "gui.hpp"
#include "../util/types.hpp"

namespace rv
{
	[[nodiscard]] inline float glyph_step(const gui_font& font, const cstd::uint32_t prev,
	                                      const cstd::uint32_t cp, const float scale) noexcept
	{
		float adv = font.glyph_advance(cp) * scale;

		if (prev != 0)
		{
			adv += font.kerning(prev, cp) * scale;
		}

		return adv;
	}
}
