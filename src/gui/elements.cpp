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

	void element::render(gui_renderer& renderer, const element_style& defaults,
	                     const position offset, vector_t<deferred_render>* overlays) const
	{
		if (!visible_)
		{
			return;
		}

		position total_offset = offset;
		const auto anim = animated_props();

		if (anim && anim->offset)
		{
			total_offset.x += anim->offset->x;
			total_offset.y += anim->offset->y;
		}

		const position min = { computed_pos_.x + total_offset.x, computed_pos_.y + total_offset.y };
		const position max = { min.x + computed_size_.x, min.y + computed_size_.y };

		color effective_bg = visual_bg_;
		float effective_rounding = visual_rounding_;

		if (anim)
		{
			if (anim->col)
			{
				effective_bg = *anim->col;
			}

			if (anim->opacity)
			{
				effective_bg.a *= *anim->opacity;
			}

			if (anim->rounding)
			{
				effective_rounding = *anim->rounding;
			}
		}

		if (effective_bg.a > 0.001f)
		{
			const color shadow_col = style_.shadow_color.value_or(color{0.f, 0.f, 0.f, 0.f});
			if (shadow_col.a > 0.001f)
			{
				const float shadow_blur = style_.shadow_blur.value_or(15.f);
				const float shadow_spread = style_.shadow_spread.value_or(0.f);
				renderer.draw_shadow_rect(min, max, shadow_col, effective_rounding, shadow_blur, shadow_spread);
			}

			renderer.draw_rect_filled(min, max, effective_bg, effective_rounding);
		}

		const auto insets = compute_insets(style_, defaults);
		const position content_min = { min.x + insets.left, min.y + insets.top };
		const position content_max = { max.x - insets.right, max.y - insets.bottom };

		render_self(renderer, content_min, content_max);

		if (visual_border_color_.a > 0.001f)
		{
			const auto bw = style_.border_width.value_or(border_vector{});
			const float thickness = cstd::fmaxf(
				cstd::fmaxf(bw.top, bw.bottom),
				cstd::fmaxf(bw.left, bw.right)
			);

			if (thickness > 0.f)
			{
				renderer.draw_rect(min, max, visual_border_color_, thickness, effective_rounding);
			}
		}

		const auto ov = style_.overflow.value_or(defaults.overflow.value_or(overflow_mode::visible));
		const bool clip = (ov == overflow_mode::hidden || ov == overflow_mode::scroll);

		if (clip)
		{
			renderer.push_clip_rect(min, max, effective_rounding);
		}

		position child_offset = total_offset;

		if (ov == overflow_mode::scroll)
		{
			child_offset.x += scroll_offset_.x;
			child_offset.y += scroll_offset_.y;
		}

		for (const auto& child : children_)
		{
			// topmost children are deferred to the gui's final overlay pass so they draw above
			// everything; capture the offset they have here so the deferred draw lands correctly.
			if (overlays && child->is_topmost() && child->is_visible())
			{
				overlays->push_back({ child.get(), child_offset });

				continue;
			}

			child->render(renderer, defaults, child_offset, overlays);
		}

		if (clip)
		{
			renderer.pop_clip_rect();
		}

		if (ov == overflow_mode::scroll && style_.show_scrollbar.value_or(true))
		{
			const auto dir = style_.direction.value_or(defaults.direction.value_or(layout_direction::horizontal));
			const bool vertical = rv::is_vertical(dir);
			const float content_size = compute_main_content_size(defaults, vertical);
			const float viewport = vertical ? computed_size_.y : computed_size_.x;

			if (content_size > viewport)
			{
				const float scrollbar_thickness = 4.f;
				const float scrollbar_margin = 4.f;
				const float scroll_track_size = viewport - 2.f * scrollbar_margin;

				const float scroll_ratio = viewport / content_size;
				const float thumb_size = cstd::fmaxf(20.f, scroll_track_size * scroll_ratio);
				const float max_scroll = content_size - viewport;
				const float scroll_pos = vertical ? -scroll_offset_.y : -scroll_offset_.x;
				const float scroll_percent = max_scroll > 0.f ? scroll_pos / max_scroll : 0.f;
				const float thumb_pos = scroll_percent * (scroll_track_size - thumb_size);

				const color track_col = color{0.15f, 0.15f, 0.15f, 0.4f};
				const color thumb_col = color{0.4f, 0.4f, 0.4f, 0.6f};

				if (vertical)
				{
					const position track_min = { max.x - scrollbar_thickness - scrollbar_margin, min.y + scrollbar_margin };
					const position track_max = { max.x - scrollbar_margin, max.y - scrollbar_margin };
					renderer.draw_rect_filled(track_min, track_max, track_col, scrollbar_thickness * 0.5f);

					const position thumb_min = { track_min.x, min.y + scrollbar_margin + thumb_pos };
					const position thumb_max = { track_max.x, thumb_min.y + thumb_size };
					renderer.draw_rect_filled(thumb_min, thumb_max, thumb_col, scrollbar_thickness * 0.5f);
				}
				else
				{
					const position track_min = { min.x + scrollbar_margin, max.y - scrollbar_thickness - scrollbar_margin };
					const position track_max = { max.x - scrollbar_margin, max.y - scrollbar_margin };
					renderer.draw_rect_filled(track_min, track_max, track_col, scrollbar_thickness * 0.5f);

					const position thumb_min = { min.x + scrollbar_margin + thumb_pos, track_min.y };
					const position thumb_max = { thumb_min.x + thumb_size, track_max.y };
					renderer.draw_rect_filled(thumb_min, thumb_max, thumb_col, scrollbar_thickness * 0.5f);
				}
			}
		}
	}
}
