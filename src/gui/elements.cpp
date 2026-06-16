#include "gui.hpp"
#include "image_fit.hpp"

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
		const auto& anim = animated_props_cached();

		if (anim && anim->offset)
		{
			total_offset.x += anim->offset->x;
			total_offset.y += anim->offset->y;
		}

		// css transform: translate is just an extra offset applied to this element and its subtree.
		if (style_.translate)
		{
			total_offset.x += style_.translate->x;
			total_offset.y += style_.translate->y;
		}

		const position min = { computed_pos_.x + total_offset.x, computed_pos_.y + total_offset.y };
		const position max = { min.x + computed_size_.x, min.y + computed_size_.y };

		// opacity/scale edit this element's whole emitted vertex range after the fact; only pay for
		// capturing the range start (and the edits at the end of this function) when one is actually
		// active. the common path (fully opaque, unscaled) stays free of that work.
		float effective_opacity = visual_opacity_;

		if (anim && anim->opacity)
		{
			effective_opacity *= *anim->opacity;
		}

		const bool needs_scale = style_.scale.has_value() && *style_.scale != 1.f;
		const bool needs_range = effective_opacity < 0.999f || needs_scale;
		const cstd::size_t vertex_start = needs_range ? renderer.vertex_count() : 0;

		color effective_bg = visual_bg_;
		corner_radii effective_radii = visual_radii_;

		if (anim)
		{
			if (anim->col)
			{
				effective_bg = *anim->col;
			}

			if (anim->rounding)
			{
				effective_radii = corner_radii::uniform(*anim->rounding);
			}
		}

		const float blur_sigma = style_.backdrop_blur.value_or(0.f);
		if (blur_sigma > 0.f)
		{
			renderer.execute_backdrop_blur(min, max, blur_sigma, effective_radii.max());
		}

		const bool has_gradient = style_.background_gradient.has_value();
		if (effective_bg.a > 0.001f || has_gradient)
		{
			const float shadow_blur = style_.shadow_blur.value_or(15.f);
			const float shadow_spread = style_.shadow_spread.value_or(0.f);

			if (style_.shadow_gradient)
			{
				color stl, str, sbr, sbl;
				gradient_to_corners(*style_.shadow_gradient, stl, str, sbr, sbl);
				renderer.draw_shadow_rect_multi_color(min, max, stl, str, sbr, sbl, effective_radii.max(), shadow_blur, shadow_spread);
			}
			else
			{
				const color shadow_col = style_.shadow_color.value_or(color{0.f, 0.f, 0.f, 0.f});
				if (shadow_col.a > 0.001f)
				{
					renderer.draw_shadow_rect(min, max, shadow_col, effective_radii.max(), shadow_blur, shadow_spread);
				}
			}

			if (has_gradient)
			{
				color tl, tr, br, bl;
				gradient_to_corners(*style_.background_gradient, tl, tr, br, bl);
				renderer.draw_rect_filled_multi_color(min, max, tl, tr, br, bl, effective_radii);
			}
			else
			{
				renderer.draw_rect_filled(min, max, effective_bg, effective_radii);
			}
		}

		// background image sits over the fill and under the element's own content (render_self),
		// independent of background_color so an image-only element still paints.
		if (style_.background_image && *style_.background_image)
		{
			const color img_tint = style_.background_image_tint.value_or(color{ 1.f, 1.f, 1.f, 1.f });

			const auto& bg_tex = *style_.background_image;
			const image_fit_result fit = compute_image_fit(min, max,
				static_cast<float>(bg_tex->width()), static_cast<float>(bg_tex->height()),
				style_.background_image_fit.value_or(image_fit::cover));

			renderer.draw_image_rounded(bg_tex, fit.draw_min, fit.draw_max, effective_radii.max(),
			                            rounding_flags_all, fit.uv_min, fit.uv_max, img_tint);
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
				renderer.draw_rect(min, max, visual_border_color_, thickness, effective_radii);
			}
		}

		const auto ov = style_.overflow.value_or(defaults.overflow.value_or(overflow_mode::visible));
		const bool clip = (ov == overflow_mode::hidden || ov == overflow_mode::scroll);

		if (clip)
		{
			renderer.push_clip_rect(min, max, effective_radii.max());
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

		// apply the opacity/scale edits to [vertex_start, vertex_end) captured above. opacity
		// multiplies through so nested opacities compose; scale is about the element centre.
		if (needs_range)
		{
			const cstd::size_t vertex_end = renderer.vertex_count();

			if (effective_opacity < 0.999f)
			{
				renderer.modify_alpha(vertex_start, vertex_end, effective_opacity);
			}

			if (needs_scale)
			{
				const position centre = { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
				renderer.modify_scale(vertex_start, vertex_end, centre, *style_.scale);
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

		// snapshot the merged animation props once per frame, after advancing/pruning animations, so
		// render() and update_all() reuse it instead of each recomputing the keyframe interpolation.
		cached_anim_props_ = animated_props();

		const auto g = gui_.lock();
		const bool smooth = style_.smooth_scroll.value_or(g ? g->default_style().smooth_scroll.value_or(false) : false);

		if (smooth)
		{
			const float lerp_speed = 20.f;
			const float factor = cstd::fminf(lerp_speed * dt, 1.f);
			if (cstd::fabsf(scroll_offset_.x - target_scroll_offset_.x) > 0.05f)
			{
				scroll_offset_.x = lerp(scroll_offset_.x, target_scroll_offset_.x, factor);
			}
			else
			{
				scroll_offset_.x = target_scroll_offset_.x;
			}

			if (cstd::fabsf(scroll_offset_.y - target_scroll_offset_.y) > 0.05f)
			{
				scroll_offset_.y = lerp(scroll_offset_.y, target_scroll_offset_.y, factor);
			}
			else
			{
				scroll_offset_.y = target_scroll_offset_.y;
			}
		}

		// fast path: elements with no colour/visual styling or per-state overrides (plain layout
		// containers, rows, columns) keep their default visual_* values; skip the resolve + lerps.
		const bool has_visual = style_.background_color || style_.text_color || style_.border_color
			|| style_.rounding || style_.radii || style_.opacity
			|| style_.hover || style_.active || style_.focus || style_.disabled_style || disabled_;

		if (settled_ && animations_.empty() && (!smooth || scroll_offset_ == target_scroll_offset_))
		{
			return;
		}

		if (!has_visual)
		{
			settled_ = (scroll_offset_ == target_scroll_offset_);
			return;
		}

		// once eased onto the target with nothing animating and no resolve_visual input changed
		// (state setters, style setters and animate() all clear settled_), the resolve + lerp below
		// would be an exact no-op. skipping it is what keeps a large static UI cheap per frame.

		// resolve the per-state target (base style + active hover/press/focus/disabled overrides),
		// then ease the visual_* values toward it. this single path replaces every widget's bespoke
		// hover/press color swap.
		const float tspeed = style_.transition_speed.value_or(0.f);
		const resolved_visual target = resolve_visual();

		const float uniform = style_.rounding.value_or(0.f);
		const corner_radii target_radii = style_.radii.value_or(
			corner_radii{ uniform, uniform, uniform, uniform });

		if (!transitions_initialized_ || tspeed <= 0.f)
		{
			visual_bg_ = target.bg;
			visual_text_color_ = target.text;
			visual_border_color_ = target.border;
			visual_radii_ = target_radii;
			visual_opacity_ = target.opacity;
			transitions_initialized_ = true;
			settled_ = animations_.empty() && (!smooth || scroll_offset_ == target_scroll_offset_);

			return;
		}

		const float factor = cstd::fminf(tspeed * dt, 1.f);

		visual_bg_ = lerp_color(visual_bg_, target.bg, factor);
		visual_text_color_ = lerp_color(visual_text_color_, target.text, factor);
		visual_border_color_ = lerp_color(visual_border_color_, target.border, factor);
		visual_radii_ = transition_lerp(visual_radii_, target_radii, factor);
		visual_opacity_ = lerp(visual_opacity_, target.opacity, factor);

		const auto color_close = [](const color& a, const color& b) noexcept
		{
			return transition_close(a, b);
		};

		// snap each channel that has effectively reached its target; we are settled once every
		// channel has snapped and nothing is animating (an animating element keeps re-resolving).
		bool all_settled = animations_.empty() && (!smooth || scroll_offset_ == target_scroll_offset_);

		if (color_close(visual_bg_, target.bg)) { visual_bg_ = target.bg; } else { all_settled = false; }
		if (color_close(visual_text_color_, target.text)) { visual_text_color_ = target.text; } else { all_settled = false; }
		if (color_close(visual_border_color_, target.border)) { visual_border_color_ = target.border; } else { all_settled = false; }
		if (transition_close(visual_radii_, target_radii)) { visual_radii_ = target_radii; } else { all_settled = false; }
		if (transition_close(visual_opacity_, target.opacity)) { visual_opacity_ = target.opacity; } else { all_settled = false; }

		settled_ = all_settled;
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

		const bool smooth = style_.smooth_scroll.value_or(defaults.smooth_scroll.value_or(false));

		if (vertical)
		{
			if (smooth)
			{
				target_scroll_offset_.y += delta * scroll_speed;
				target_scroll_offset_.y = cstd::fmaxf(-max_scroll, cstd::fminf(0.f, target_scroll_offset_.y));
			}
			else
			{
				scroll_offset_.y += delta * scroll_speed;
				scroll_offset_.y = cstd::fmaxf(-max_scroll, cstd::fminf(0.f, scroll_offset_.y));
				target_scroll_offset_.y = scroll_offset_.y;
			}
		}
		else
		{
			if (smooth)
			{
				target_scroll_offset_.x += delta * scroll_speed;
				target_scroll_offset_.x = cstd::fmaxf(-max_scroll, cstd::fminf(0.f, target_scroll_offset_.x));
			}
			else
			{
				scroll_offset_.x += delta * scroll_speed;
				scroll_offset_.x = cstd::fmaxf(-max_scroll, cstd::fminf(0.f, scroll_offset_.x));
				target_scroll_offset_.x = scroll_offset_.x;
			}
		}

		mark_unsettled();
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
