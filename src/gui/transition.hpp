#pragma once
#include "../util/types.hpp"
#include "animation.hpp"      // lerp, lerp_color
#include "element_types.hpp"  // color, corner_radii

// A small reusable "value eases toward a target" helper, shared by element's per-state visual
// transitions and by widgets that animate their own visual channels (e.g. the checkbox box color
// or the slider thumb position). Replaces the hand-rolled lerp + snap + initialized dance that was
// duplicated across widgets. Header-only (produces no translation unit).
namespace rv
{
	// customization points: per-type interpolation and closeness test.
	[[nodiscard]] inline float transition_lerp(const float a, const float b, const float t) noexcept
	{
		return lerp(a, b, t);
	}

	[[nodiscard]] inline color transition_lerp(const color& a, const color& b, const float t) noexcept
	{
		return lerp_color(a, b, t);
	}

	[[nodiscard]] inline corner_radii transition_lerp(const corner_radii& a, const corner_radii& b, const float t) noexcept
	{
		return { lerp(a.tl, b.tl, t), lerp(a.tr, b.tr, t), lerp(a.br, b.br, t), lerp(a.bl, b.bl, t) };
	}

	[[nodiscard]] inline bool transition_close(const float a, const float b) noexcept
	{
		return cstd::fabsf(a - b) < 0.001f;
	}

	[[nodiscard]] inline bool transition_close(const color& a, const color& b) noexcept
	{
		return transition_close(a.r, b.r) && transition_close(a.g, b.g)
			&& transition_close(a.b, b.b) && transition_close(a.a, b.a);
	}

	[[nodiscard]] inline bool transition_close(const corner_radii& a, const corner_radii& b) noexcept
	{
		return transition_close(a.tl, b.tl) && transition_close(a.tr, b.tr)
			&& transition_close(a.br, b.br) && transition_close(a.bl, b.bl);
	}

	// `current` eases toward `target` at `speed` units/sec; snaps when within epsilon, and snaps
	// immediately when speed <= 0 or before the first step.
	template <class T>
	struct transition
	{
		T current{};
		bool initialized = false;

		void reset(const T& value) noexcept
		{
			current = value;
			initialized = true;
		}

		const T& step(const T& target, const float speed, const float dt) noexcept
		{
			if (!initialized || speed <= 0.f)
			{
				current = target;
				initialized = true;

				return current;
			}

			current = transition_lerp(current, target, cstd::fminf(speed * dt, 1.f));

			if (transition_close(current, target))
			{
				current = target;
			}

			return current;
		}
	};
}
