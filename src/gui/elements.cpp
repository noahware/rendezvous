#include "gui.hpp"

namespace rv
{

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

	void element::update(float dt)
	{
		for (auto& anim : animations_)
		{
			anim.update(dt);
		}

		animations_.erase(
			std::remove_if(animations_.begin(), animations_.end(), [](const animation_state& a)
			{
				return a.is_finished() && a.get_fill_mode() == fill_mode::none;
			}),
			animations_.end()
		);

		if (style_.background_color || style_.text_color || style_.rounding || style_.border_color)
		{
			const color target_bg = style_.background_color.value_or(color{ 0.f, 0.f, 0.f, 0.f });
			const color target_tc = style_.text_color.value_or(color{ 1.f, 1.f, 1.f, 1.f });
			const float target_rd = style_.rounding.value_or(0.f);
			const color target_bc = style_.border_color.value_or(color{ 0.f, 0.f, 0.f, 0.f });

			const float tspeed = style_.transition_speed.value_or(0.f);

			if (!transitions_initialized_ || tspeed <= 0.f)
			{
				visual_bg_ = target_bg;
				visual_text_color_ = target_tc;
				visual_rounding_ = target_rd;
				visual_border_color_ = target_bc;
				transitions_initialized_ = true;
			}
			else
			{
				const float factor = cstd::fminf(tspeed * dt, 1.f);
				visual_bg_ = lerp_color(visual_bg_, target_bg, factor);
				visual_text_color_ = lerp_color(visual_text_color_, target_tc, factor);
				visual_rounding_ = lerp(visual_rounding_, target_rd, factor);
				visual_border_color_ = lerp_color(visual_border_color_, target_bc, factor);

				auto close = [](const float a, const float b) noexcept
				{
					return cstd::fabsf(a - b) < 0.001f;
				};

				auto color_close = [&](const color& a, const color& b) noexcept
				{
					return close(a.r, b.r) && close(a.g, b.g) && close(a.b, b.b) && close(a.a, b.a);
				};

				if (color_close(visual_bg_, target_bg))
				{
					visual_bg_ = target_bg;
				}

				if (color_close(visual_text_color_, target_tc))
				{
					visual_text_color_ = target_tc;
				}

				if (close(visual_rounding_, target_rd))
				{
					visual_rounding_ = target_rd;
				}

				if (color_close(visual_border_color_, target_bc))
				{
					visual_border_color_ = target_bc;
				}
			}
		}
	}

	float element::compute_main_content_size(const element_style& defaults, const bool vertical) const noexcept
	{
		const auto insets = compute_insets(style_, defaults);
		const float gap = resolve_gap(style_, defaults, vertical);
		float content_size = 0.f;
		cstd::size_t visible_count = 0;

		for (const auto& child : children_)
		{
			if (!child->is_visible())
			{
				continue;
			}

			const auto child_margin = child->style().margin.value_or(
				defaults.margin.value_or(border_vector{}));

			const float child_main = vertical
				? (child->computed_size().y + child_margin.top + child_margin.bottom)
				: (child->computed_size().x + child_margin.left + child_margin.right);

			content_size += child_main;
			++visible_count;
		}

		// add gaps between visible children
		if (visible_count > 1)
		{
			content_size += gap * static_cast<float>(visible_count - 1);
		}

		// add insets (padding + border)
		const float total_insets = vertical
			? (insets.top + insets.bottom)
			: (insets.left + insets.right);
		content_size += total_insets;

		return content_size;
	}

	void element::apply_scroll(const float delta, const element_style& defaults) noexcept
	{
		const auto dir = style_.direction.value_or(
			defaults.direction.value_or(layout_direction::horizontal)
		);
		const bool vertical = rv::is_vertical(dir);

		const float content_size = compute_main_content_size(defaults, vertical);

		const float viewport = vertical ? computed_size_.y : computed_size_.x;
		const float max_scroll = cstd::fmaxf(0.f, content_size - viewport);
		const float scroll_speed = 30.f;

		if (vertical)
		{
			scroll_offset_.y += delta * scroll_speed;
			scroll_offset_.y = cstd::fmaxf(-max_scroll, cstd::fminf(0.f, scroll_offset_.y));
		}
		else
		{
			scroll_offset_.x += delta * scroll_speed;
			scroll_offset_.x = cstd::fmaxf(-max_scroll, cstd::fminf(0.f, scroll_offset_.x));
		}
	}

	optional_t<keyframe_props> element::animated_props() const noexcept
	{
		if (animations_.empty())
		{
			return {};
		}

		// merge all active animations - later ones win per property, offset accumulates
		keyframe_props merged;
		bool has_any = false;

		for (const auto& anim : animations_)
		{
			if (anim.is_finished() && anim.get_fill_mode() == fill_mode::none)
			{
				continue;
			}

			const auto props = anim.current_props();
			has_any = true;

			if (props.col)
			{
				merged.col = props.col;
			}

			if (props.opacity)
			{
				merged.opacity = props.opacity;
			}

			if (props.rounding)
			{
				merged.rounding = props.rounding;
			}

			// offset is additive - stacked slides combine
			if (props.offset)
			{
				if (merged.offset)
				{
					merged.offset->x += props.offset->x;
					merged.offset->y += props.offset->y;
				}
				else
				{
					merged.offset = props.offset;
				}
			}
		}

		if (!has_any)
		{
			return {};
		}

		return merged;
	}
}
