#include "gui.hpp"

namespace rv
{
	void layout(element& el, const vector_2d<float> available, const element_style& defaults)
	{
		const auto& size = el.declared_size();
		const auto& style = el.style();
		const auto dir = style.direction.value_or(
			defaults.direction.value_or(layout_direction::horizontal)
		);
		const bool vertical = rv::is_vertical(dir);
		const float gap = resolve_gap(style, defaults, vertical);
		const float cross_gap = resolve_gap(style, defaults, !vertical);

		// resolve own size
		const optional_t<float> resolved_w = resolve_size(size.width, available.x);
		const optional_t<float> resolved_h = resolve_size(size.height, available.y);

		// compute insets (padding + border)
		const auto insets = compute_insets(style, defaults);
		const float insets_h = insets.left + insets.right;
		const float insets_v = insets.top + insets.bottom;

		// child available = resolved size (or parent available) minus insets
		const vector_2d<float> child_available = {
			cstd::fmaxf(0.f, resolved_w.value_or(available.x) - insets_h),
			cstd::fmaxf(0.f, resolved_h.value_or(available.y) - insets_v)
		};

		// alignment for stretch
		const auto al = style.align.value_or(defaults.align.value_or(alignment::start));

		// wrap mode
		const auto wrap_v = style.wrap.value_or(defaults.wrap.value_or(wrap_mode::no_wrap));
		const bool wrapping = wrap_v != wrap_mode::no_wrap;

		// separate flow children from absolute children
		vector_t<shared_ptr_t<element>> flow_children;
		vector_t<shared_ptr_t<element>> abs_children;

		for (const auto& child : el.children())
		{
			if (!child->is_visible())
			{
				continue;
			}

			const auto child_pos = child->style().position.value_or(position_type::relative);

			if (child_pos == position_type::absolute)
			{
				abs_children.push_back(child);
			}
			else
			{
				flow_children.push_back(child);
			}
		}

		const cstd::size_t visible_count = flow_children.size();


		struct flex_item
		{
			shared_ptr_t<element> child;
			float base_size = 0.f;
			float final_size = 0.f;
			float grow = 0.f;
			float shrink = 0.f;
			float main_margins = 0.f;
			float cross_margins = 0.f;
			bool frozen = false;
		};

		vector_t<flex_item> items;
		items.reserve(visible_count);

		for (const auto& child : flow_children)
		{
			flex_item item;
			item.child = child;

			const auto& child_style = child->style();
			const auto child_margin = child_style.margin.value_or(
				defaults.margin.value_or(border_vector{})
			);

			item.main_margins = vertical
				? child_margin.top + child_margin.bottom
				: child_margin.left + child_margin.right;

			item.cross_margins = vertical
				? child_margin.left + child_margin.right
				: child_margin.top + child_margin.bottom;

			// determine flex properties
			const styled_size& child_main_sv = vertical
				? child->declared_size().height
				: child->declared_size().width;

			// backward compat: fill mode = flex_grow=1, flex_shrink=1, flex_basis=0
			const bool is_fill = child_main_sv.mode == size_mode::fill
				&& !child_style.flex_grow.has_value()
				&& !child_style.flex_shrink.has_value()
				&& !child_style.flex_basis.has_value();

			if (is_fill)
			{
				item.grow = 1.f;
				item.shrink = 1.f;
				item.base_size = 0.f;
			}
			else
			{
				item.grow = child_style.flex_grow.value_or(0.f);
				item.shrink = child_style.flex_shrink.value_or(1.f);

				// determine base size from flex_basis or declared size
				const auto& basis = child_style.flex_basis;
				const float main_available = vertical ? child_available.y : child_available.x;

				if (basis.has_value())
				{
					const auto resolved = resolve_size(*basis, main_available);

					if (resolved.has_value())
					{
						item.base_size = *resolved;
					}
					else
					{
						// auto basis - layout to get natural size
						vector_2d<float> margin_adjusted = child_available;

						if (vertical)
						{
							margin_adjusted.x -= item.cross_margins;
						}
						else
						{
							margin_adjusted.y -= item.cross_margins;
						}

						layout(*child, margin_adjusted, defaults);
						item.base_size = vertical ? child->computed_size().y : child->computed_size().x;
					}
				}
				else if (child_main_sv.mode == size_mode::fill)
				{
					// fill with explicit flex properties
					item.base_size = 0.f;
				}
				else if (child_main_sv.mode == size_mode::auto_v)
				{
					// auto - layout to get natural size
					vector_2d<float> margin_adjusted = child_available;

					if (vertical)
					{
						margin_adjusted.x -= item.cross_margins;
					}
					else
					{
						margin_adjusted.y -= item.cross_margins;
					}

					layout(*child, margin_adjusted, defaults);
					item.base_size = vertical ? child->computed_size().y : child->computed_size().x;
				}
				else
				{
					// px or percent - resolve normally
					const auto resolved = resolve_size(child_main_sv, main_available);
					item.base_size = resolved.value_or(0.f);
				}
			}

			item.final_size = item.base_size;
			items.push_back(cstd::move(item));
		}

		const float total_main = vertical ? child_available.y : child_available.x;
		const float total_cross = vertical ? child_available.x : child_available.y;

		vector_t<flex_line> lines;

		if (wrapping && !items.empty())
		{
			float line_main = 0.f;
			cstd::size_t line_start = 0;
			cstd::size_t line_count = 0;

			for (cstd::size_t i = 0; i < items.size(); ++i)
			{
				const float item_main = items[i].base_size + items[i].main_margins;
				const float gap_before = (line_count > 0) ? gap : 0.f;

				if (line_count > 0 && line_main + gap_before + item_main > total_main)
				{
					// start new line
					lines.push_back({ line_start, line_count, line_main, 0.f });
					line_start = i;
					line_count = 1;
					line_main = item_main;
				}
				else
				{
					line_main += gap_before + item_main;
					line_count++;
				}
			}

			if (line_count > 0)
			{
				lines.push_back({ line_start, line_count, line_main, 0.f });
			}
		}
		else
		{
			float used_main = 0.f;

			for (const auto& item : items)
			{
				used_main += item.base_size + item.main_margins;
			}

			if (items.size() > 1)
			{
				used_main += gap * static_cast<float>(items.size() - 1);
			}

			lines.push_back({ 0, items.size(), used_main, 0.f });
		}

		for (auto& line : lines)
		{
			// reset frozen state
			for (cstd::size_t i = line.start_index; i < line.start_index + line.count; ++i)
			{
				items[i].frozen = false;
			}

			// compute used main for this line
			float line_used = 0.f;

			for (cstd::size_t i = line.start_index; i < line.start_index + line.count; ++i)
			{
				line_used += items[i].base_size + items[i].main_margins;
			}

			if (line.count > 1)
			{
				line_used += gap * static_cast<float>(line.count - 1);
			}

			const float line_free = total_main - line_used;

			if (line_free > 0.f)
			{
				// grow with freeze loop
				bool changed = true;

				while (changed)
				{
					changed = false;
					float total_grow = 0.f;
					float frozen_main = 0.f;

					for (cstd::size_t i = line.start_index; i < line.start_index + line.count; ++i)
					{
						if (items[i].frozen)
						{
							frozen_main += items[i].final_size + items[i].main_margins;
						}
						else
						{
							total_grow += items[i].grow;
							frozen_main += items[i].base_size + items[i].main_margins;
						}
					}

					if (total_grow <= 0.f)
					{
						break;
					}

					const float gap_space = line.count > 1
						? gap * static_cast<float>(line.count - 1) : 0.f;
					const float distributable = cstd::fmaxf(0.f, total_main - frozen_main - gap_space);

					for (cstd::size_t i = line.start_index; i < line.start_index + line.count; ++i)
					{
						if (items[i].frozen || items[i].grow <= 0.f)
						{
							continue;
						}

						const float share = distributable * (items[i].grow / total_grow);
						items[i].final_size = items[i].base_size + share;

						// apply min/max constraints
						const float main_avail = vertical ? available.y : available.x;
						const auto& cs = items[i].child->style();
						const auto& min_sv = vertical ? cs.min_height : cs.min_width;
						const auto& max_sv = vertical ? cs.max_height : cs.max_width;

						const float clamped = clamp_size(items[i].final_size, min_sv, max_sv, main_avail);

						if (clamped != items[i].final_size)
						{
							items[i].final_size = clamped;
							items[i].frozen = true;
							changed = true;
						}
					}
				}
			}
			else if (line_free < 0.f)
			{
				// shrink with freeze loop
				bool changed = true;

				while (changed)
				{
					changed = false;
					float total_shrink_scaled = 0.f;
					float frozen_main = 0.f;

					for (cstd::size_t i = line.start_index; i < line.start_index + line.count; ++i)
					{
						if (items[i].frozen)
						{
							frozen_main += items[i].final_size + items[i].main_margins;
						}
						else
						{
							total_shrink_scaled += items[i].shrink * items[i].base_size;
							frozen_main += items[i].base_size + items[i].main_margins;
						}
					}

					if (total_shrink_scaled <= 0.f)
					{
						break;
					}

					const float gap_space = line.count > 1
						? gap * static_cast<float>(line.count - 1) : 0.f;
					const float overflow_amt = cstd::fmaxf(0.f, frozen_main + gap_space - total_main);

					for (cstd::size_t i = line.start_index; i < line.start_index + line.count; ++i)
					{
						if (items[i].frozen || items[i].shrink <= 0.f)
						{
							continue;
						}

						const float ratio = (items[i].shrink * items[i].base_size) / total_shrink_scaled;
						items[i].final_size = cstd::fmaxf(0.f, items[i].base_size - overflow_amt * ratio);

						// apply min/max constraints
						const float main_avail = vertical ? available.y : available.x;
						const auto& cs = items[i].child->style();
						const auto& min_sv = vertical ? cs.min_height : cs.min_width;
						const auto& max_sv = vertical ? cs.max_height : cs.max_width;

						const float clamped = clamp_size(items[i].final_size, min_sv, max_sv, main_avail);

						if (clamped != items[i].final_size)
						{
							items[i].final_size = clamped;
							items[i].frozen = true;
							changed = true;
						}
					}
				}
			}
		}

		float max_cross = 0.f;

		for (auto& line : lines)
		{
			float line_cross = 0.f;

			for (cstd::size_t i = line.start_index; i < line.start_index + line.count; ++i)
			{
				auto& item = items[i];

				vector_2d<float> child_avail = child_available;

				if (vertical)
				{
					child_avail.y = item.final_size;
					child_avail.x -= item.cross_margins;
				}
				else
				{
					child_avail.x = item.final_size;
					child_avail.y -= item.cross_margins;
				}

				layout(*item.child, child_avail, defaults);

				const float child_cross = vertical
					? item.child->computed_size().x
					: item.child->computed_size().y;

				line_cross = cstd::fmaxf(line_cross, child_cross + item.cross_margins);
			}

			line.cross_size = line_cross;
			max_cross = cstd::fmaxf(max_cross, line_cross);

			// recompute line.main_size from final child sizes
			float line_main_final = 0.f;

			for (cstd::size_t i = line.start_index; i < line.start_index + line.count; ++i)
			{
				const float child_main = vertical
					? items[i].child->computed_size().y
					: items[i].child->computed_size().x;

				line_main_final += child_main + items[i].main_margins;
			}

			if (line.count > 1)
			{
				line_main_final += gap * static_cast<float>(line.count - 1);
			}

			line.main_size = line_main_final;
		}

		if (wrapping && lines.size() > 1)
		{
			const auto ac = style.align_content_v.value_or(
				defaults.align_content_v.value_or(align_content::flex_start)
			);

			if (ac == align_content::stretch)
			{
				float total_lines_cross = 0.f;

				for (const auto& line : lines)
				{
					total_lines_cross += line.cross_size;
				}

				const float cross_gap_total = lines.size() > 1
					? cross_gap * static_cast<float>(lines.size() - 1) : 0.f;
				const float remaining_cross = total_cross - total_lines_cross - cross_gap_total;

				if (remaining_cross > 0.f)
				{
					const float extra_per_line = remaining_cross / static_cast<float>(lines.size());

					for (auto& line : lines)
					{
						line.cross_size += extra_per_line;
					}
				}
			}
		}

		for (auto& line : lines)
		{
			for (cstd::size_t i = line.start_index; i < line.start_index + line.count; ++i)
			{
				auto& item = items[i];
				const auto child_al = item.child->style().align_self.value_or(al);

				if (child_al == alignment::stretch)
				{
					const styled_size& cross_sv = vertical
						? item.child->declared_size().width
						: item.child->declared_size().height;

					if (cross_sv.mode == size_mode::auto_v || cross_sv.mode == size_mode::fill)
					{
						// when wrapping with multiple lines, stretch to line's cross size
						// otherwise stretch to full available cross
						const float stretch_target = (wrapping && lines.size() > 1)
							? line.cross_size
							: (vertical ? child_available.x : child_available.y);

						const float stretch_cross = cstd::fmaxf(0.f, stretch_target - item.cross_margins);

						vector_2d<float> new_size = item.child->computed_size();

						if (vertical)
						{
							new_size.x = stretch_cross;
						}
						else
						{
							new_size.y = stretch_cross;
						}

						item.child->set_computed_size(new_size);
					}
				}
			}
		}

		float final_w, final_h;

		float own_cross;

		if (wrapping && lines.size() > 1)
		{
			own_cross = 0.f;

			for (const auto& line : lines)
			{
				own_cross += line.cross_size;
			}

			if (lines.size() > 1)
			{
				own_cross += cross_gap * static_cast<float>(lines.size() - 1);
			}
		}
		else
		{
			own_cross = max_cross;
		}

		// max line main for auto sizing
		float max_line_main = 0.f;

		for (const auto& line : lines)
		{
			max_line_main = cstd::fmaxf(max_line_main, line.main_size);
		}

		if (resolved_w.has_value())
		{
			final_w = resolved_w.value();
		}
		else if (size.width.mode == size_mode::fill)
		{
			final_w = available.x;
		}
		else
		{
			final_w = (vertical ? own_cross : max_line_main) + insets_h;
		}

		if (resolved_h.has_value())
		{
			final_h = resolved_h.value();
		}
		else if (size.height.mode == size_mode::fill)
		{
			final_h = available.y;
		}
		else
		{
			final_h = (vertical ? max_line_main : own_cross) + insets_v;
		}

		// content-based sizing for leaf elements (e.g. text)
		if (flow_children.empty())
		{
			const auto content = el.content_size(child_available);

			if (content.x > 0.f || content.y > 0.f)
			{
				if (size.width.mode == size_mode::auto_v && !resolved_w.has_value())
				{
					final_w = content.x + insets_h;
				}

				if (size.height.mode == size_mode::auto_v && !resolved_h.has_value())
				{
					final_h = content.y + insets_v;
				}
			}
		}

		// apply min/max constraints
		final_w = clamp_size(final_w, style.min_width, style.max_width, available.x);
		final_h = clamp_size(final_h, style.min_height, style.max_height, available.y);

		// apply aspect ratio
		if (style.aspect_ratio.has_value())
		{
			const float ar = *style.aspect_ratio;

			if (size.height.mode == size_mode::auto_v && size.width.mode != size_mode::auto_v)
			{
				final_h = final_w / ar;
				final_h = clamp_size(final_h, style.min_height, style.max_height, available.y);
			}
			else if (size.width.mode == size_mode::auto_v && size.height.mode != size_mode::auto_v)
			{
				final_w = final_h * ar;
				final_w = clamp_size(final_w, style.min_width, style.max_width, available.x);
			}
		}

		el.set_computed_size({ final_w, final_h });

		el.set_flex_lines(cstd::move(lines));

		for (const auto& child : abs_children)
		{
			const auto& cs = child->style();
			const auto child_margin = cs.margin.value_or(
				defaults.margin.value_or(border_vector{})
			);

			const float content_w = final_w - insets_h;
			const float content_h = final_h - insets_v;

			// if both insets specified on an axis, derive size
			const auto left_v = resolve_size(cs.inset_left.value_or(styled_size::auto_v()), content_w);
			const auto right_v = resolve_size(cs.inset_right.value_or(styled_size::auto_v()), content_w);
			const auto top_v = resolve_size(cs.inset_top.value_or(styled_size::auto_v()), content_h);
			const auto bottom_v = resolve_size(cs.inset_bottom.value_or(styled_size::auto_v()), content_h);

			vector_2d<float> abs_available = { content_w, content_h };

			if (left_v.has_value() && right_v.has_value())
			{
				abs_available.x = cstd::fmaxf(0.f, content_w - *left_v - *right_v
					- child_margin.left - child_margin.right);
			}

			if (top_v.has_value() && bottom_v.has_value())
			{
				abs_available.y = cstd::fmaxf(0.f, content_h - *top_v - *bottom_v
					- child_margin.top - child_margin.bottom);
			}

			layout(*child, abs_available, defaults);
		}
	}
}
