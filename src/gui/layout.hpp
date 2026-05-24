#pragma once
#include "element.hpp"

namespace rv
{
	inline void layout(element& el, const vector_2d<float> available)
	{
		const auto& size = el.declared_size();
		const auto& style = el.style();
		const float gap = style.gap.value_or(0.f);
		const bool is_vertical = (style.direction == layout_direction::vertical);

		const auto resolve_axis = [](const styled_size& sv, const float available_px) -> optional_t<float>
		{
			switch (sv.mode)
			{
				case size_mode::px:      return sv.value;
				case size_mode::percent: return available_px * (sv.value / 100.f);
				case size_mode::fill:    return {};
				case size_mode::auto_v:  return {};
			}

			return {};
		};

		const optional_t<float> resolved_w = resolve_axis(size.width, available.x);
		const optional_t<float> resolved_h = resolve_axis(size.height, available.y);

		const vector_2d<float> child_available = {
			resolved_w.value_or(available.x),
			resolved_h.value_or(available.y)
		};

		float used_main = 0.f;
		cstd::uint32_t fill_count = 0;
		float max_cross = 0.f;

		for (const auto& child : el.children())
		{
			const styled_size& child_main_sv = is_vertical
				? child->declared_size().height
				: child->declared_size().width;

			if (child_main_sv.mode == size_mode::fill)
			{
				fill_count++;
				continue;
			}

			layout(*child, child_available);

			const vector_2d<float> cs = child->computed_size();
			const float child_main = is_vertical ? cs.y : cs.x;
			const float child_cross = is_vertical ? cs.x : cs.y;

			used_main += child_main;
			max_cross = cstd::fmaxf(max_cross, child_cross);
		}

		const auto child_count = el.children().size();
		if (child_count > 1)
			used_main += gap * static_cast<float>(child_count - 1);

		if (fill_count > 0)
		{
			const float total_main = is_vertical ? child_available.y : child_available.x;
			const float remaining = cstd::fmaxf(0.f, total_main - used_main);
			const float fill_size = remaining / static_cast<float>(fill_count);

			for (const auto& child : el.children())
			{
				const styled_size& child_main_sv = is_vertical
					? child->declared_size().height
					: child->declared_size().width;

				if (child_main_sv.mode != size_mode::fill)
					continue;

				vector_2d<float> fill_available = child_available;
				if (is_vertical)
					fill_available.y = fill_size;
				else
					fill_available.x = fill_size;

				layout(*child, fill_available);

				const vector_2d<float> cs = child->computed_size();
				const float child_cross = is_vertical ? cs.x : cs.y;
				max_cross = cstd::fmaxf(max_cross, child_cross);
			}
		}

		float final_w, final_h;

		if (resolved_w.has_value())
			final_w = resolved_w.value();
		else if (size.width.mode == size_mode::fill)
			final_w = available.x;
		else
			final_w = is_vertical ? max_cross : used_main;

		if (resolved_h.has_value())
			final_h = resolved_h.value();
		else if (size.height.mode == size_mode::fill)
			final_h = available.y;
		else
			final_h = is_vertical ? used_main : max_cross;

		el.set_computed_size({ final_w, final_h });
	}
}
