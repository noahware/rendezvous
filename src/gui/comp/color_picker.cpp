#include "color_picker.hpp"

namespace rv
{
	color_picker_popup::regions color_picker_popup::layout(const position min, const position max) noexcept
	{
		const float l = min.x + pad;
		const float t = min.y + pad;

		const rect sv = { { l, t }, { l + sv_w, t + sv_h } };
		const rect hue = { { sv.max.x + gap, t }, { sv.max.x + gap + bar_w, t + sv_h } };

		const float ay = sv.max.y + gap;
		const rect alpha = { { l, ay }, { hue.max.x, ay + alpha_h } };

		const float hy = ay + alpha_h + gap;
		const rect hex = { { l, hy }, { hue.max.x, hy + hex_h } };

		return { sv, hue, alpha, hex };
	}

	bool color_picker_popup::on_mouse_click()
	{
		if (!state_)
		{
			return true;
		}

		const position m = input_ ? input_->mouse_pos() : position{ };
		const regions r = layout(visual_pos_, { visual_pos_.x + computed_size_.x, visual_pos_.y + computed_size_.y });

		if (contains_point(r.sv, m))
		{
			active_ = region::sv;
		}
		else if (contains_point(r.hue, m))
		{
			active_ = region::hue;
		}
		else if (contains_point(r.alpha, m))
		{
			active_ = region::alpha;
		}
		else
		{
			active_ = region::none;

			return true;
		}

		dragging_ = true;
		update_from_mouse(r, m);

		return true;
	}

	void color_picker_popup::update(const float dt)
	{
		if (dragging_)
		{
			if (!input_ || !input_->is_mouse_down(0))
			{
				dragging_ = false;
				active_ = region::none;
			}
			else
			{
				const regions r = layout(visual_pos_, { visual_pos_.x + computed_size_.x, visual_pos_.y + computed_size_.y });
				update_from_mouse(r, input_->mouse_pos());
			}
		}

		if (hovered_ && input_)
		{
			input_->set_cursor(cursor_type::hand);
		}

		element::update(dt);
	}

	void color_picker_popup::render_self(gui_renderer& renderer, const position min, const position max) const
	{
		if (!state_)
		{
			return;
		}

		// render_self receives the inner (post-border) box; recompute from the element box so
		// the drawn regions match the input hit-testing in on_mouse_click/update.
		const regions r = layout(visual_pos_, { visual_pos_.x + computed_size_.x, visual_pos_.y + computed_size_.y });

		draw_sv_square(renderer, r.sv);
		draw_hue_strip(renderer, r.hue);
		draw_alpha_bar(renderer, r.alpha);
		draw_cursors(renderer, r);
	}

	void color_picker_popup::update_from_mouse(const regions& r, const position m) noexcept
	{
		switch (active_)
		{
		case region::sv:
		{
			const float w = r.sv.max.x - r.sv.min.x;
			const float h = r.sv.max.y - r.sv.min.y;
			state_->s = (w > 0.f) ? clamp01((m.x - r.sv.min.x) / w) : 0.f;
			state_->v = (h > 0.f) ? clamp01(1.f - (m.y - r.sv.min.y) / h) : 0.f;
			break;
		}
		case region::hue:
		{
			const float h = r.hue.max.y - r.hue.min.y;
			state_->h = (h > 0.f) ? clamp01((m.y - r.hue.min.y) / h) : 0.f;
			break;
		}
		case region::alpha:
		{
			const float w = r.alpha.max.x - r.alpha.min.x;
			state_->a = (w > 0.f) ? clamp01((m.x - r.alpha.min.x) / w) : 0.f;
			break;
		}
		default:
			return;
		}

		if (on_edit_)
		{
			on_edit_();
		}
	}

	void color_picker_popup::draw_sv_square(gui_renderer& renderer, const rect& r) const
	{
		const position mn = floor_pos(r.min);
		const position mx = floor_pos(r.max);

		const color hue_col = hsv_to_rgb(state_->h, 1.f, 1.f);
		const color white = rgb(1.f, 1.f, 1.f);

		// white -> hue across x
		renderer.draw_rect_filled_multi_color(mn, mx, white, hue_col, hue_col, white, region_round, rounding_flags_all);

		// transparent -> black down y, multiplied over the above
		const color clear = rgb(0.f, 0.f, 0.f, 0.f);
		const color black = rgb(0.f, 0.f, 0.f, 1.f);
		renderer.draw_rect_filled_multi_color(mn, mx, clear, clear, black, black, region_round, rounding_flags_all);
	}

	void color_picker_popup::draw_hue_strip(gui_renderer& renderer, const rect& r) const
	{
		constexpr cstd::int32_t segments = 6;

		const float x0 = cstd::floorf(r.min.x);
		const float x1 = cstd::floorf(r.max.x);
		const float top_y = cstd::floorf(r.min.y);
		const float height = cstd::floorf(r.max.y) - top_y;

		for (cstd::int32_t i = 0; i < segments; ++i)
		{
			const color top = hsv_to_rgb(static_cast<float>(i) / static_cast<float>(segments), 1.f, 1.f);
			const color bottom = hsv_to_rgb(static_cast<float>(i + 1) / static_cast<float>(segments), 1.f, 1.f);

			// floor each boundary so adjacent segments share an exact pixel edge (no seam)
			const float sy0 = top_y + cstd::floorf(height * static_cast<float>(i) / static_cast<float>(segments));
			const float sy1 = top_y + cstd::floorf(height * static_cast<float>(i + 1) / static_cast<float>(segments));

			// round only the outer corners so the strip reads as one rounded bar
			cstd::uint32_t flag_bits = 0;

			if (i == 0)
			{
				flag_bits |= rounding_flags_top_left | rounding_flags_top_right;
			}

			if (i == segments - 1)
			{
				flag_bits |= rounding_flags_bottom_left | rounding_flags_bottom_right;
			}

			const auto flags = static_cast<rounding_flags>(flag_bits);
			const float round = flag_bits != 0 ? region_round : 0.f;

			renderer.draw_rect_filled_multi_color({ x0, sy0 }, { x1, sy1 }, top, top, bottom, bottom, round, flags);
		}
	}

	void color_picker_popup::draw_alpha_bar(gui_renderer& renderer, const rect& r) const
	{
		const position mn = floor_pos(r.min);
		const position mx = floor_pos(r.max);

		// the bar is a checkerboard + gradient composite, so round it with a clip rect rather
		// than per-rect rounding
		renderer.push_clip_rect(mn, mx, region_round);

		draw_checker(renderer, mn, mx);

		const color opaque = hsv_to_rgb(state_->h, state_->s, state_->v, 1.f);
		color clear = opaque;
		clear.a = 0.f;

		renderer.draw_rect_filled_multi_color(mn, mx, clear, opaque, opaque, clear, 0.f, rounding_flags_none);

		renderer.pop_clip_rect();
	}

	void color_picker_popup::draw_cursors(gui_renderer& renderer, const regions& r) const
	{
		// saturation/value point
		const float sx = r.sv.min.x + state_->s * (r.sv.max.x - r.sv.min.x);
		const float sy = r.sv.min.y + (1.f - state_->v) * (r.sv.max.y - r.sv.min.y);
		renderer.draw_circle({ sx, sy }, 6.f, rgb(0.f, 0.f, 0.f, 0.6f), 2.f, 16);
		renderer.draw_circle({ sx, sy }, 5.f, rgb(1.f, 1.f, 1.f, 1.f), 2.f, 16);

		// hue marker
		const float hy = r.hue.min.y + state_->h * (r.hue.max.y - r.hue.min.y);
		renderer.draw_rect({ r.hue.min.x - 2.f, hy - 2.f }, { r.hue.max.x + 2.f, hy + 2.f }, rgb(1.f, 1.f, 1.f, 1.f), 2.f, 2.f);

		// alpha marker
		const float ax = r.alpha.min.x + state_->a * (r.alpha.max.x - r.alpha.min.x);
		renderer.draw_rect({ ax - 2.f, r.alpha.min.y - 2.f }, { ax + 2.f, r.alpha.max.y + 2.f }, rgb(1.f, 1.f, 1.f, 1.f), 2.f, 2.f);
	}

	void color_picker_popup::draw_checker(gui_renderer& renderer, const position min, const position max)
	{
		constexpr float tile = 6.f;
		const color light = { 0.55f, 0.55f, 0.55f, 1.f };
		const color dark = { 0.35f, 0.35f, 0.35f, 1.f };

		renderer.draw_rect_filled(min, max, dark, 0.f);

		cstd::int32_t row = 0;

		for (float y = min.y; y < max.y; y += tile, ++row)
		{
			const float y1 = cstd::floorf(y);
			const float y2 = cstd::floorf(cstd::fminf(y + tile, max.y));
			cstd::int32_t col = row;

			for (float x = min.x; x < max.x; x += tile, ++col)
			{
				if ((col & 1) == 0)
				{
					continue;
				}

				const float x1 = cstd::floorf(x);
				const float x2 = cstd::floorf(cstd::fminf(x + tile, max.x));
				renderer.draw_rect_filled({ x1, y1 }, { x2, y2 }, light, 0.f);
			}
		}
	}

	void color_picker::update(const float dt)
	{
		ensure_hex_field();
		configure_popup_once();

		// reflect external writes to the bound colour while the popup is closed
		if (bound_ && !open_)
		{
			const color cur = current_color();
			const color ext = *bound_;

			if (ext.r != cur.r || ext.g != cur.g || ext.b != cur.b || ext.a != cur.a)
			{
				hsva_ = rgb_to_hsv(ext);
			}
		}

		// close when a click lands outside both the swatch and the popup (focus-independent,
		// so dragging inside the popup never closes it). update_all runs before process_events,
		// so this sees the same click edge as the popup's own on_mouse_click.
		if (open_ && input_ && input_->is_mouse_clicked(0))
		{
			const position m = input_->mouse_pos();

			// hit-test against the drawn (visual) rect, not computed_pos_, so the swatch still
			// registers when the picker lives inside a scrolled/offset container (e.g. a tab).
			const position vp = visual_pos();
			const bool over_swatch = m.x >= vp.x && m.x <= vp.x + computed_size_.x
			                      && m.y >= vp.y && m.y <= vp.y + computed_size_.y;

			if (!over_swatch && !popup_contains(m))
			{
				open_ = false;
			}
		}

		// Toggle popup visibility only on change, and relayout once when it does, so the popup
		// gets positioned when shown without forcing a full-tree relayout every frame.
		if (open_ != popup_shown_)
		{
			popup_shown_ = open_;
			popup_->set_visible(open_);
			mark_layout_dirty();
		}

		// Mirror the colour into the hex field, but never mid-drag. text_box::set_text marks
		// the layout dirty, and a full-tree relayout every frame during a drag halves the frame
		// rate; the field snaps to the final value when the drag releases.
		if (open_ && hex_ && !hex_->is_focused() && !popup_->is_dragging())
		{
			const string_t want = hex_from_color(current_color(), true);

			if (hex_->text() != want)
			{
				hex_->text(want);
			}
		}

		if (hovered_ && input_)
		{
			input_->set_cursor(cursor_type::hand);
		}

		element::update(dt);
	}

	void color_picker::render_self(gui_renderer& renderer, const position min, const position max) const
	{
		const float rounding = style_.rounding.value_or(4.f);
		const color cur = current_color();

		// An opaque colour hides the checkerboard, so skip it (and the clip-rect cost) and just
		// fill the rounded swatch, this is the common case and runs every frame.
		if (cur.a < 0.999f)
		{
			renderer.push_clip_rect(min, max, rounding);
			draw_swatch_background(renderer, min, max);
			renderer.draw_rect_filled(min, max, cur, 0.f);
			renderer.pop_clip_rect();
		}
		else
		{
			renderer.draw_rect_filled(min, max, cur, rounding);
		}

		renderer.draw_rect(min, max, swatch_border_, 1.f, rounding);
	}

	void color_picker::commit()
	{
		const color c = current_color();

		if (bound_)
		{
			*bound_ = c;
		}

		if (on_change_)
		{
			on_change_(c);
		}
	}

	void color_picker::ensure_hex_field()
	{
		if (hex_)
		{
			return;
		}

		const auto g = gui_.lock();

		if (!g)
		{
			return;
		}

		hex_ = g->make_child<text_box>(popup_, element_size{ styled_size::px(0.f), styled_size::px(0.f) },
			font_, input_);
		hex_->positioning(position_type::absolute);
		hex_->text_size(13.f);
		hex_->background_color({ 0.1f, 0.1f, 0.12f, 1.f }).rounding(4.f)
			.border_color({ 0.3f, 0.3f, 0.36f, 1.f }).border_width(1.f)
			.padding({ 4.f, 6.f, 4.f, 6.f }).text_color({ 0.92f, 0.92f, 0.96f, 1.f });
		hex_->on_submit([this](const string_t& s) { apply_hex(s); });
	}

	void color_picker::apply_hex(const string_t& s)
	{
		const auto parsed = color_from_hex(s);

		if (!parsed)
		{
			return;
		}

		hsva_ = rgb_to_hsv(*parsed);
		commit();
	}

	void color_picker::configure_popup_once()
	{
		if (popup_configured_ || !hex_ || computed_size_.y <= 0.f)
		{
			return;
		}

		const auto pad = style_.padding.value_or(border_vector{ });
		const auto brd = style_.border_width.value_or(border_vector{ });
		const float inset_l = pad.left + brd.left;
		const float inset_t = pad.top + brd.top;

		popup_->set_declared_size(element_size{
			styled_size::px(color_picker_popup::width()),
			styled_size::px(color_picker_popup::height())
		});
		popup_->inset_left(styled_size::px(-inset_l));
		popup_->inset_top(styled_size::px(computed_size_.y - inset_t));

		// position the hex field inside the popup's hex region (popup has no padding, so the
		// region offsets from the popup origin are the child's insets)
		const color_picker_popup::regions r = color_picker_popup::layout(
			position{ 0.f, 0.f },
			position{ color_picker_popup::width(), color_picker_popup::height() });

		hex_->set_declared_size(element_size{
			styled_size::px(r.hex.max.x - r.hex.min.x),
			styled_size::px(r.hex.max.y - r.hex.min.y)
		});
		hex_->inset_left(styled_size::px(r.hex.min.x));
		hex_->inset_top(styled_size::px(r.hex.min.y));

		popup_configured_ = true;
	}

	void color_picker::draw_swatch_background(gui_renderer& renderer, const position min, const position max) const
	{
		// checkerboard so partial alpha reads as transparency
		constexpr float tile = 6.f;
		const color light = { 0.55f, 0.55f, 0.55f, 1.f };
		const color dark = { 0.35f, 0.35f, 0.35f, 1.f };

		renderer.draw_rect_filled(min, max, dark, 0.f);

		cstd::int32_t row = 0;

		for (float y = min.y; y < max.y; y += tile, ++row)
		{
			const float y1 = cstd::floorf(y);
			const float y2 = cstd::floorf(cstd::fminf(y + tile, max.y));
			cstd::int32_t col = row;

			for (float x = min.x; x < max.x; x += tile, ++col)
			{
				if ((col & 1) == 0)
				{
					continue;
				}

				const float x1 = cstd::floorf(x);
				const float x2 = cstd::floorf(cstd::fminf(x + tile, max.x));
				renderer.draw_rect_filled({ x1, y1 }, { x2, y2 }, light, 0.f);
			}
		}
	}
}
