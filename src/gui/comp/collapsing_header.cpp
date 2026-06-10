#include "collapsing_header.hpp"

namespace rv
{
	collapsing_header::collapsing_header(const element_size size, shared_ptr_t<gui_font> font,
	                                     shared_ptr_t<input> input) noexcept
		: element(size), font_(cstd::move(font)), input_(cstd::move(input))
	{
		init_defaults();

		content_ = make_child<element>(element_size{ styled_size::fill(), styled_size::auto_v() });
		content_->direction(layout_direction::vertical).gap(8.f).padding({ 8.f, 8.f, 8.f, 8.f });
		content_->visible(expanded_);
	}

	bool collapsing_header::on_mouse_click()
	{
		if (!input_)
		{
			return false;
		}

		const float header_bottom = visual_pos().y + header_height_;

		if (input_->mouse_pos().y <= header_bottom)
		{
			toggle();
			return true;
		}

		return false;
	}

	void collapsing_header::toggle()
	{
		expanded_ = !expanded_;

		if (content_)
		{
			content_->stop_animations();

			if (expanded_)
			{
				content_->visible(true);
				mark_layout_dirty();

				keyframe_props start;
				start.opacity = 0.f;
				keyframe_props end;
				end.opacity = 1.f;

				animation_options opts;
				opts.duration = 0.25f;
				opts.ease = easing::ease_out_cubic;
				opts.fill = fill_mode::forwards;

				content_->animate(
					keyframe_sequence{}.add(0.f, start).add(1.f, end),
					opts
				);
			}
			else
			{
				content_->visible(false);
				mark_layout_dirty();
			}
		}

		if (on_toggle_)
		{
			on_toggle_(expanded_);
		}
	}

	void collapsing_header::update(const float dt)
	{
		element::update(dt);

		const float speed = style_.transition_speed.value_or(12.f);

		const float target_angle = expanded_ ? 90.f : 0.f;
		const float angle_diff = target_angle - arrow_angle_;

		if (cstd::fabsf(angle_diff) > 0.01f)
		{
			arrow_angle_ += angle_diff * cstd::fminf(speed * dt, 1.f);
		}
		else
		{
			arrow_angle_ = target_angle;
		}

		const float expand_target = expanded_ ? 1.f : 0.f;
		const float expand_diff = expand_target - visual_expand_t_;

		if (cstd::fabsf(expand_diff) > 0.001f)
		{
			visual_expand_t_ += expand_diff * cstd::fminf(speed * dt, 1.f);
		}
		else
		{
			visual_expand_t_ = expand_target;
		}

		header_hovered_ = false;

		if (hovered_ && input_)
		{
			header_hovered_ = input_->mouse_pos().y <= visual_pos().y + header_height_;
		}

		const color target_fill = header_hovered_ ? header_hover_color_ : header_color_;
		header_fill_.step(target_fill, speed, dt);
	}

	vector_2d<float> collapsing_header::content_size(const vector_2d<float> available) const noexcept
	{
		return { 0.f, 0.f };
	}

	void collapsing_header::render_self(gui_renderer& renderer, const position min, const position max) const
	{
		const float rounding = style_.rounding.value_or(0.f);
		const position hdr_min = visual_pos();
		const float hdr_right = hdr_min.x + computed_size_.x;
		const float hdr_bottom = hdr_min.y + header_height_;

		const float bottom_r = rounding * (1.f - visual_expand_t_);
		const corner_radii radii = { rounding, rounding, bottom_r, bottom_r };

		renderer.draw_rect_filled(hdr_min, { hdr_right, hdr_bottom }, header_fill_.current, radii);

		const float arrow_cx = hdr_min.x + arrow_indent_;
		const float arrow_cy = hdr_min.y + header_height_ * 0.5f;
		const float half = arrow_size_;

		constexpr float pi = 3.14159265f;
		const float rad = arrow_angle_ * pi / 180.f;
		const float cos_a = cstd::cosf(rad);
		const float sin_a = cstd::sinf(rad);

		const auto rotate = [&](const float lx, const float ly) -> position
		{
			return { arrow_cx + lx * cos_a - ly * sin_a,
			         arrow_cy + lx * sin_a + ly * cos_a };
		};

		renderer.add_path_point(rotate(half, 0.f));
		renderer.add_path_point(rotate(-half * 0.5f, -half * 0.7f));
		renderer.add_path_point(rotate(-half * 0.5f, half * 0.7f));
		renderer.draw_filled_path(arrow_color_);

		if (font_ && !label_.empty())
		{
			const float scale = (style_.font_size.value_or(0.f) > 0.f)
				? style_.font_size.value_or(0.f) / font_->baked_size()
				: 1.f;

			const float glyph_h = (font_->ascent() - font_->descent()) * scale;
			const float tx = hdr_min.x + text_indent_;
			const float ty = hdr_min.y + (header_height_ - glyph_h) * 0.5f;

			renderer.draw_text(*font_, { tx, ty }, label_, visual_text_color_,
			                   style_.font_size.value_or(0.f));
		}
	}
}
