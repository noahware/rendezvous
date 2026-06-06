#pragma once
#include "../util/math.hpp"

namespace rv
{
	class texture;
	class font;

	struct color
	{
		float r = 0.f, g = 0.f, b = 0.f, a = 1.f;

		constexpr color() noexcept = default;

		constexpr color(const float r, const float g, const float b, const float a = 255.f) noexcept
				:	r(r > 1.f ? r / 255.f : r),
					g(g > 1.f ? g / 255.f : g),
					b(b > 1.f ? b / 255.f : b),
					a(a > 1.f ? a / 255.f : a) { }
	};

	struct position : vector_2d<float>
	{

	};

	struct ndc_position : vector_2d<float>
	{

	};

	struct texture_position : vector_2d<float>
	{

	};

	struct rect
	{
		position min, max;

		bool operator==(const rect& other) const
		{
			return min == other.min && max == other.max;
		}

		bool operator!=(const rect& other) const
		{
			return !(*this == other);
		}
	};

	// per-corner radii for a rounded rect: top-left, top-right, bottom-right, bottom-left. lives in
	// the render layer so both the renderer draw calls and the gui style can share one type.
	struct corner_radii
	{
		float tl = 0.f;
		float tr = 0.f;
		float br = 0.f;
		float bl = 0.f;

		[[nodiscard]] constexpr float max() const noexcept
		{
			const float a = tl > tr ? tl : tr;
			const float b = br > bl ? br : bl;

			return a > b ? a : b;
		}

		[[nodiscard]] static constexpr corner_radii uniform(const float r) noexcept
		{
			return { r, r, r, r };
		}
	};
}
