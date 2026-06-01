#include "gui.hpp"

namespace rv
{
	void compute_justify(const justify_content jc, const float remaining,
	                     const cstd::size_t count, const float base_gap,
	                     float& out_offset, float& out_gap) noexcept
	{
		out_offset = 0.f;
		out_gap = base_gap;

		switch (jc)
		{
		case justify_content::start:
			break;
		case justify_content::center:
			out_offset = remaining * 0.5f;
			break;
		case justify_content::end:
			out_offset = remaining;
			break;
		case justify_content::space_between:
			if (count > 1)
			{
				out_gap = base_gap + remaining / static_cast<float>(count - 1);
			}
			break;
		case justify_content::space_around:
			if (count > 0)
			{
				const float space = remaining / static_cast<float>(count);
				out_offset = space * 0.5f;
				out_gap = base_gap + space;
			}
			break;
		case justify_content::space_evenly:
			if (count > 0)
			{
				const float space = remaining / static_cast<float>(count + 1);
				out_offset = space;
				out_gap = base_gap + space;
			}
			break;
		}
	}

	void position_line_children(const span_t<const shared_ptr_t<element>> children,
	                            const bool effective_reversed,
	                            const bool vertical,
	                            const float available_main,
	                            const float line_cross_size,
	                            const justify_content jc,
	                            const alignment al,
	                            const float gap,
	                            const position base_cursor,
	                            const float cross_cursor_offset,
	                            const element_style& defaults)
	{
		// compute main-axis usage
		float total_main_used = 0.f;
		const cstd::size_t count = children.size();

		for (const auto& child : children)
		{
			const auto cm = child->style().margin.value_or(
				defaults.margin.value_or(border_vector{})
			);

			total_main_used += vertical
				? child->computed_size().y + cm.top + cm.bottom
				: child->computed_size().x + cm.left + cm.right;
		}

		if (count > 1)
		{
			total_main_used += gap * static_cast<float>(count - 1);
		}

		const float remaining = cstd::fmaxf(0.f, available_main - total_main_used);

		float main_offset = 0.f;
		float effective_gap = gap;
		compute_justify(jc, remaining, count, gap, main_offset, effective_gap);

		// build iteration order
		vector_t<cstd::size_t> order;
		order.reserve(count);

		for (cstd::size_t i = 0; i < count; ++i)
		{
			order.push_back(effective_reversed ? (count - 1 - i) : i);
		}

		// position children
		float main_cursor = main_offset;
		bool first = true;

		for (const auto idx : order)
		{
			const auto& child = children[idx];

			if (!first)
			{
				main_cursor += effective_gap;
			}

			first = false;

			const auto child_margin = child->style().margin.value_or(
				defaults.margin.value_or(border_vector{})
			);

			const float child_cross = vertical
				? child->computed_size().x
				: child->computed_size().y;

			const float cross_margins = vertical
				? child_margin.left + child_margin.right
				: child_margin.top + child_margin.bottom;

			// cross-axis alignment within line
			const auto child_al = child->style().align_self.value_or(al);
			float cross_offset = 0.f;

			switch (child_al)
			{
			case alignment::start:
				break;
			case alignment::center:
				cross_offset = (line_cross_size - child_cross - cross_margins) * 0.5f;
				break;
			case alignment::end:
				cross_offset = line_cross_size - child_cross - cross_margins;
				break;
			case alignment::stretch:
				break;
			}

			position child_pos = base_cursor;

			if (vertical)
			{
				child_pos.y += main_cursor + child_margin.top;
				child_pos.x += cross_cursor_offset + cross_offset + child_margin.left;
			}
			else
			{
				child_pos.x += main_cursor + child_margin.left;
				child_pos.y += cross_cursor_offset + cross_offset + child_margin.top;
			}

			resolve_positions(*child, child_pos, defaults);

			// advance main cursor
			main_cursor += vertical
				? child->computed_size().y + child_margin.top + child_margin.bottom
				: child->computed_size().x + child_margin.left + child_margin.right;
		}
	}

	void resolve_positions(element& el, const position cursor, const element_style& defaults)
	{
		el.set_computed_pos(cursor);

		if (!el.is_visible())
		{
			return;
		}

		const auto& style = el.style();
		const auto dir = style.direction.value_or(
			defaults.direction.value_or(layout_direction::horizontal)
		);
		const bool vertical = rv::is_vertical(dir);
		const bool reversed = rv::is_reversed(dir);

		// RTL flips horizontal to horizontal_reverse
		const auto tdir = style.dir.value_or(defaults.dir.value_or(text_direction::ltr));
		const bool effective_reversed = (tdir == text_direction::rtl && !vertical) ? !reversed : reversed;

		const auto al = style.align.value_or(defaults.align.value_or(alignment::start));
		const auto jc = style.justify.value_or(defaults.justify.value_or(justify_content::start));
		const float gap = resolve_gap(style, defaults, vertical);
		const float cross_gap = resolve_gap(style, defaults, !vertical);

		// compute insets (padding + border)
		const auto insets = compute_insets(style, defaults);

		const float available_main = (vertical ? el.computed_size().y : el.computed_size().x)
			- (vertical ? insets.top + insets.bottom : insets.left + insets.right);
		const float available_cross = (vertical ? el.computed_size().x : el.computed_size().y)
			- (vertical ? insets.left + insets.right : insets.top + insets.bottom);

		// collect visible flow children (skip absolute) - in original order
		vector_t<shared_ptr_t<element>> flow_children;
		vector_t<shared_ptr_t<element>> abs_children;

		for (const auto& child : el.children())
		{
			if (!child->is_visible())
			{
				continue;
			}

			const auto child_pos_type = child->style().position.value_or(position_type::relative);

			if (child_pos_type == position_type::absolute)
			{
				abs_children.push_back(child);
			}
			else
			{
				flow_children.push_back(child);
			}
		}

		// base position = cursor + insets
		const position base = { cursor.x + insets.left, cursor.y + insets.top };

		const auto& flex_lines_ref = el.flex_lines();
		const auto wrap_v = style.wrap.value_or(defaults.wrap.value_or(wrap_mode::no_wrap));
		const bool multi_line = wrap_v != wrap_mode::no_wrap && flex_lines_ref.size() > 1;

		if (multi_line)
		{
			// ── multi-line wrap positioning ──
			const cstd::size_t line_count = flex_lines_ref.size();

			// compute total lines cross size
			float total_lines_cross = 0.f;

			for (const auto& line : flex_lines_ref)
			{
				total_lines_cross += line.cross_size;
			}

			if (line_count > 1)
			{
				total_lines_cross += cross_gap * static_cast<float>(line_count - 1);
			}

			const float cross_remaining = cstd::fmaxf(0.f, available_cross - total_lines_cross);

			// align-content distribution
			const auto ac = style.align_content_v.value_or(
				defaults.align_content_v.value_or(align_content::flex_start)
			);

			float cross_start_offset = 0.f;
			float effective_cross_gap = cross_gap;

			switch (ac)
			{
			case align_content::flex_start:
				break;
			case align_content::flex_end:
				cross_start_offset = cross_remaining;
				break;
			case align_content::center:
				cross_start_offset = cross_remaining * 0.5f;
				break;
			case align_content::stretch:
				// stretch already handled in layout()
				break;
			case align_content::space_between:
				if (line_count > 1)
				{
					effective_cross_gap = cross_gap + cross_remaining / static_cast<float>(line_count - 1);
				}
				break;
			case align_content::space_around:
				if (line_count > 0)
				{
					const float space = cross_remaining / static_cast<float>(line_count);
					cross_start_offset = space * 0.5f;
					effective_cross_gap = cross_gap + space;
				}
				break;
			case align_content::space_evenly:
				if (line_count > 0)
				{
					const float space = cross_remaining / static_cast<float>(line_count + 1);
					cross_start_offset = space;
					effective_cross_gap = cross_gap + space;
				}
				break;
			}

			const bool wrap_reversed = wrap_v == wrap_mode::wrap_reverse;

			// build line order
			vector_t<cstd::size_t> line_order;
			line_order.reserve(line_count);

			for (cstd::size_t i = 0; i < line_count; ++i)
			{
				line_order.push_back(wrap_reversed ? (line_count - 1 - i) : i);
			}

			// position each line
			float cross_cursor = cross_start_offset;

			for (cstd::size_t li = 0; li < line_order.size(); ++li)
			{
				if (li > 0)
				{
					cross_cursor += effective_cross_gap;
				}

				const auto& line = flex_lines_ref[line_order[li]];

				// get children for this line from flow_children using line indices
				// (flex_lines store indices into the original flow order)
				vector_t<shared_ptr_t<element>> line_children;

				for (cstd::size_t i = line.start_index; i < line.start_index + line.count; ++i)
				{
					if (i < flow_children.size())
					{
						line_children.push_back(flow_children[i]);
					}
				}

				position_line_children(
					line_children,
					effective_reversed, vertical,
					available_main, line.cross_size,
					jc, al, gap,
					base, cross_cursor,
					defaults
				);

				cross_cursor += line.cross_size;
			}
		}
		else
		{
			// ── single-line positioning (original path) ──

			// if reversed, reverse flow children order
			if (effective_reversed)
			{
				std::reverse(flow_children.begin(), flow_children.end());
			}

			// count visible flow children and compute total main-axis usage
			float total_main = 0.f;
			const cstd::size_t visible_count = flow_children.size();

			for (const auto& child : flow_children)
			{
				const auto child_margin = child->style().margin.value_or(
					defaults.margin.value_or(border_vector{})
				);

				const float child_main = vertical
					? child->computed_size().y + child_margin.top + child_margin.bottom
					: child->computed_size().x + child_margin.left + child_margin.right;

				total_main += child_main;
			}

			if (visible_count > 1)
			{
				total_main += gap * static_cast<float>(visible_count - 1);
			}

			const float remaining = cstd::fmaxf(0.f, available_main - total_main);

			// justify: compute main-axis offset and effective gap
			float main_offset = 0.f;
			float effective_gap = gap;
			compute_justify(jc, remaining, visible_count, gap, main_offset, effective_gap);

			position child_cursor = base;

			if (vertical)
			{
				child_cursor.y += main_offset;
			}
			else
			{
				child_cursor.x += main_offset;
			}

			bool first_visible = true;

			for (const auto& child : flow_children)
			{
				if (!first_visible)
				{
					if (vertical)
					{
						child_cursor.y += effective_gap;
					}
					else
					{
						child_cursor.x += effective_gap;
					}
				}

				first_visible = false;

				const auto child_margin = child->style().margin.value_or(
					defaults.margin.value_or(border_vector{})
				);

				// apply margin offset
				position child_pos = child_cursor;
				child_pos.x += child_margin.left;
				child_pos.y += child_margin.top;

				// alignment: cross-axis offset (with align_self override)
				const auto child_al = child->style().align_self.value_or(al);

				const float child_cross = vertical
					? child->computed_size().x
					: child->computed_size().y;

				const float cross_margins = vertical
					? child_margin.left + child_margin.right
					: child_margin.top + child_margin.bottom;

				float cross_offset = 0.f;

				switch (child_al)
				{
				case alignment::start:
					break;
				case alignment::center:
					cross_offset = (available_cross - child_cross - cross_margins) * 0.5f;
					break;
				case alignment::end:
					cross_offset = available_cross - child_cross - cross_margins;
					break;
				case alignment::stretch:
					// stretch is handled in layout() - here just treat as start
					break;
				}

				if (vertical)
				{
					child_pos.x += cross_offset;
				}
				else
				{
					child_pos.y += cross_offset;
				}

				resolve_positions(*child, child_pos, defaults);

				// advance cursor along main axis
				if (vertical)
				{
					child_cursor.y += child->computed_size().y + child_margin.top + child_margin.bottom;
				}
				else
				{
					child_cursor.x += child->computed_size().x + child_margin.left + child_margin.right;
				}
			}
		}

		// position absolute children
		for (const auto& child : abs_children)
		{
			const auto child_margin = child->style().margin.value_or(
				defaults.margin.value_or(border_vector{})
			);
			const auto& cs = child->style();

			const auto resolve_inset = [](const optional_t<styled_size>& sv, const float avail) -> optional_t<float>
			{
				if (!sv.has_value())
				{
					return {};
				}

				switch (sv->mode)
				{
				case size_mode::px:      return sv->value;
				case size_mode::percent: return avail * (sv->value / 100.f);
				default:                 return {};
				}
			};

			const float content_w = el.computed_size().x - insets.left - insets.right;
			const float content_h = el.computed_size().y - insets.top - insets.bottom;

			const auto left_v = resolve_inset(cs.inset_left, content_w);
			const auto right_v = resolve_inset(cs.inset_right, content_w);
			const auto top_v = resolve_inset(cs.inset_top, content_h);
			const auto bottom_v = resolve_inset(cs.inset_bottom, content_h);

			float child_x = cursor.x + insets.left;
			float child_y = cursor.y + insets.top;

			if (left_v.has_value())
			{
				child_x += *left_v + child_margin.left;
			}
			else if (right_v.has_value())
			{
				child_x += content_w - *right_v - child->computed_size().x - child_margin.right;
			}
			else
			{
				child_x += child_margin.left;
			}

			if (top_v.has_value())
			{
				child_y += *top_v + child_margin.top;
			}
			else if (bottom_v.has_value())
			{
				child_y += content_h - *bottom_v - child->computed_size().y - child_margin.bottom;
			}
			else
			{
				child_y += child_margin.top;
			}

			resolve_positions(*child, position{ child_x, child_y }, defaults);
		}
	}

	void update_all(element& el, const float dt, const position current_offset)
	{
		if (!el.is_visible())
		{
			return;
		}

		position new_offset = current_offset;
		const auto anim = el.animated_props();
		if (anim && anim->offset)
		{
			new_offset.x += anim->offset->x;
			new_offset.y += anim->offset->y;
		}

		el.set_visual_pos({ el.computed_pos().x + new_offset.x, el.computed_pos().y + new_offset.y });

		el.update(dt);

		position child_offset = new_offset;
		if (el.style().overflow.value_or(overflow_mode::visible) == overflow_mode::scroll)
		{
			child_offset.x += el.scroll_offset().x;
			child_offset.y += el.scroll_offset().y;
		}

		for (const auto& child : el.children())
		{
			update_all(*child, dt, child_offset);
		}
	}
}
