#pragma once
#include "position.hpp"
#include "../util/types.hpp"

// Colour-space helpers used by the colour picker. Conversions build the result color by
// direct field assignment rather than the color(r, g, b, a) constructor, which divides any
// component > 1.0 by 255 (see position.hpp), the same reason lerp_color avoids it.

namespace rv
{
	// Hue/saturation/value/alpha, each in [0, 1]. The picker stores this as its canonical
	// state because RGB -> HSV is lossy at the greyscale edge (hue is undefined when sat == 0),
	// which would make the hue cursor jump while editing.
	struct hsva
	{
		float h = 0.f, s = 0.f, v = 0.f, a = 1.f;
	};

	[[nodiscard]] inline float clamp01(const float x) noexcept
	{
		return cstd::fmaxf(0.f, cstd::fminf(1.f, x));
	}

	[[nodiscard]] inline color hsv_to_rgb(const float h, const float s, const float v, const float a = 1.f) noexcept
	{
		color out;
		out.a = clamp01(a);

		const float sat = clamp01(s);
		const float val = clamp01(v);

		if (sat <= 0.f)
		{
			out.r = val;
			out.g = val;
			out.b = val;

			return out;
		}

		float hue = cstd::fmodf(h, 1.f);

		if (hue < 0.f)
		{
			hue += 1.f;
		}

		hue *= 6.f;

		const cstd::int32_t sector = static_cast<cstd::int32_t>(cstd::floorf(hue));
		const float f = hue - static_cast<float>(sector);

		const float p = val * (1.f - sat);
		const float q = val * (1.f - sat * f);
		const float t = val * (1.f - sat * (1.f - f));

		switch (sector % 6)
		{
		case 0:  out.r = val; out.g = t;   out.b = p;   break;
		case 1:  out.r = q;   out.g = val; out.b = p;   break;
		case 2:  out.r = p;   out.g = val; out.b = t;   break;
		case 3:  out.r = p;   out.g = q;   out.b = val; break;
		case 4:  out.r = t;   out.g = p;   out.b = val; break;
		default: out.r = val; out.g = p;   out.b = q;   break;
		}

		return out;
	}

	[[nodiscard]] inline hsva rgb_to_hsv(const color& c) noexcept
	{
		const float r = clamp01(c.r);
		const float g = clamp01(c.g);
		const float b = clamp01(c.b);

		const float mx = cstd::fmaxf(r, cstd::fmaxf(g, b));
		const float mn = cstd::fminf(r, cstd::fminf(g, b));
		const float delta = mx - mn;

		hsva out;
		out.v = mx;
		out.s = (mx <= 0.f) ? 0.f : (delta / mx);
		out.a = clamp01(c.a);

		if (delta <= 0.f)
		{
			out.h = 0.f;

			return out;
		}

		if (mx == r)
		{
			out.h = (g - b) / delta;
		}
		else if (mx == g)
		{
			out.h = 2.f + (b - r) / delta;
		}
		else
		{
			out.h = 4.f + (r - g) / delta;
		}

		out.h /= 6.f;

		if (out.h < 0.f)
		{
			out.h += 1.f;
		}

		return out;
	}

	// Returns -1 for a non-hex character so callers can reject malformed input.
	[[nodiscard]] inline cstd::int32_t hex_nibble(const char c) noexcept
	{
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'a' && c <= 'f') return c - 'a' + 10;
		if (c >= 'A' && c <= 'F') return c - 'A' + 10;

		return -1;
	}

	// Accepts #RGB, #RRGGBB, #RRGGBBAA (the leading # is optional). Components are written
	// directly so the color constructor's >1.0 normalisation never runs.
	[[nodiscard]] inline optional_t<color> color_from_hex(string_view_t text) noexcept
	{
		if (!text.empty() && text.front() == '#')
		{
			text.remove_prefix(1);
		}

		const cstd::size_t len = text.size();

		if (len != 3 && len != 6 && len != 8)
		{
			return {};
		}

		array_t<cstd::int32_t, 8> nibbles = { };

		for (cstd::size_t i = 0; i < len; ++i)
		{
			const cstd::int32_t n = hex_nibble(text[i]);

			if (n < 0)
			{
				return {};
			}

			nibbles[i] = n;
		}

		color out;

		if (len == 3)
		{
			// shorthand: each nibble is duplicated (e.g. f80 -> ff8800)
			out.r = static_cast<float>(nibbles[0] * 17) / 255.f;
			out.g = static_cast<float>(nibbles[1] * 17) / 255.f;
			out.b = static_cast<float>(nibbles[2] * 17) / 255.f;
			out.a = 1.f;

			return out;
		}

		out.r = static_cast<float>(nibbles[0] * 16 + nibbles[1]) / 255.f;
		out.g = static_cast<float>(nibbles[2] * 16 + nibbles[3]) / 255.f;
		out.b = static_cast<float>(nibbles[4] * 16 + nibbles[5]) / 255.f;
		out.a = (len == 8) ? static_cast<float>(nibbles[6] * 16 + nibbles[7]) / 255.f : 1.f;

		return out;
	}

	[[nodiscard]] inline string_t hex_from_color(const color& c, const bool with_alpha) noexcept
	{
		const auto to_byte = [](const float v) noexcept
		{
			return static_cast<cstd::int32_t>(cstd::roundf(clamp01(v) * 255.f));
		};

		if (with_alpha)
		{
			return cstd::format("#{:02X}{:02X}{:02X}{:02X}",
				to_byte(c.r), to_byte(c.g), to_byte(c.b), to_byte(c.a));
		}

		return cstd::format("#{:02X}{:02X}{:02X}", to_byte(c.r), to_byte(c.g), to_byte(c.b));
	}
}
