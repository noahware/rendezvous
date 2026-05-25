#pragma once
#include "element.hpp"

namespace rv
{
	[[nodiscard]] inline optional_t<float> resolve_size(const styled_size& sv, const float available_px)
	{
		switch (sv.mode)
		{
			case size_mode::px:      return sv.value;
			case size_mode::percent: return available_px * (sv.value / 100.f);
			case size_mode::fill:    return {};
			case size_mode::auto_v:  return {};
		}

		return {};
	}

	[[nodiscard]] inline float clamp_size(const float value, const optional_t<styled_size>& min_sv,
	                                      const optional_t<styled_size>& max_sv, const float available)
	{
		float result = value;

		if (min_sv.has_value())
		{
			const auto min_v = resolve_size(*min_sv, available);

			if (min_v.has_value())
			{
				result = cstd::fmaxf(result, *min_v);
			}
		}

		if (max_sv.has_value())
		{
			const auto max_v = resolve_size(*max_sv, available);

			if (max_v.has_value())
			{
				result = cstd::fminf(result, *max_v);
			}
		}

		return result;
	}

	// implemented in elements.cpp
	void layout(element& el, vector_2d<float> available, const element_style& defaults);
}
