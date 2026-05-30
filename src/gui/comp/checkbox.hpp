#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../../util/string.hpp"

namespace rv
{
	class checkbox final : public element
	{
	public:
		checkbox() noexcept = default;

		explicit checkbox(const element_size size) noexcept
			: element(size)
		{
			init_defaults();
		}

		checkbox(const element_size size, shared_ptr_t<gui_font> font) noexcept
			: element(size), font_(cstd::move(font))
		{
			init_defaults();
		}

		checkbox& label(const string_view_t text)
		{
			label_ = string_t(text);
			mark_layout_dirty();

			return *this;
		}

		checkbox& checked(const bool state) noexcept
		{
			checked_ = state;
			visual_t_ = state ? 1.f : 0.f;

			if (bound_)
			{
				*bound_ = checked_;
			}

			return *this;
		}

		[[nodiscard]] bool is_checked() const noexcept
		{
			return checked_;
		}

		checkbox& toggle() noexcept
		{
			checked_ = !checked_;

			if (bound_)
			{
				*bound_ = checked_;
			}

			if (on_change_)
			{
				on_change_(checked_);
			}

			return *this;
		}

		checkbox& on_change(function_t<void(bool)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		checkbox& hover_color(const color col) noexcept
		{
			hover_color_ = col;

			return *this;
		}

		checkbox& pressed_color(const color col) noexcept
		{
			pressed_color_ = col;

			return *this;
		}

		checkbox& box_color(const color col) noexcept
		{
			box_color_ = col;

			return *this;
		}

		checkbox& check_color(const color col) noexcept
		{
			check_color_ = col;

			return *this;
		}

		checkbox& gap(const float px) noexcept
		{
			gap_ = px;
			mark_layout_dirty();

			return *this;
		}

		checkbox& bind(bool* ptr) noexcept
		{
			bound_ = ptr;

			if (bound_)
			{
				checked_ = *bound_;
				visual_t_ = checked_ ? 1.f : 0.f;
			}

			return *this;
		}

		bool on_mouse_click() override
		{
			toggle();

			return true;
		}

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override
		{
			if (!font_ || label_.empty())
			{
				return { 0.f, 0.f };
			}

			const float scale = resolved_scale();
			const float line_h = font_->line_height() * scale;

			return { line_h + gap_ + measure_label_width(scale), line_h };
		}

		void update(const float dt) override
		{
			if (bound_ && *bound_ != checked_)
			{
				checked_ = *bound_;
			}

			// element::update animates visual_text_color_ from style_.text_color,
			// which we use for the label. the box fill is animated separately
			// below since the element itself draws no background.
			element::update(dt);

			const color target_fill = pressed_ ? pressed_color_ : (hovered_ ? hover_color_ : box_color_);

			if (!box_initialized_)
			{
				visual_box_color_ = target_fill;
				box_initialized_ = true;
			}
			else
			{
				const float speed = style_.transition_speed.value_or(12.f);
				const float factor = cstd::fminf(speed * dt, 1.f);
				visual_box_color_ = lerp_color(visual_box_color_, target_fill, factor);
			}

			const float target_t = checked_ ? 1.f : 0.f;
			const float diff = target_t - visual_t_;

			if (diff != 0.f)
			{
				const float speed = style_.transition_speed.value_or(12.f);
				const float factor = cstd::fminf(speed * dt, 1.f);
				visual_t_ += diff * factor;

				if (cstd::fabsf(diff) < 0.001f)
				{
					visual_t_ = target_t;
				}
			}
		}

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			const float h = max.y - min.y;

			// the box is a square on the left; with no label it fills the element.
			const float box_side = label_.empty() ? cstd::fminf(max.x - min.x, h) : h;
			const position box_min = { min.x, min.y + (h - box_side) * 0.5f };
			const position box_max = { box_min.x + box_side, box_min.y + box_side };

			const float rounding = style_.rounding.value_or(0.f);

			renderer.draw_rect_filled(box_min, box_max, visual_box_color_, rounding);

			if (border_color_.a > 0.001f && border_width_ > 0.f)
			{
				renderer.draw_rect(box_min, box_max, border_color_, border_width_, rounding);
			}

			if (visual_t_ > 0.f)
			{
				const float cx = (box_min.x + box_max.x) * 0.5f;
				const float cy = (box_min.y + box_max.y) * 0.5f;
				const float half = (box_side * 0.5f) * visual_t_;
				const float inner_rounding = rounding * visual_t_;

				renderer.draw_rect_filled(
					{ cx - half, cy - half },
					{ cx + half, cy + half },
					check_color_,
					inner_rounding
				);
			}

			if (font_ && !label_.empty())
			{
				const float scale = resolved_scale();

				// draw_text positions pos.y at the top of the line box, so center
				// the glyph block (ascent..descent) against the box rather than the
				// full line height — line_gap would otherwise push the text high.
				const float glyph_h = (font_->ascent() - font_->descent()) * scale;
				const float x = box_max.x + gap_;
				const float y = min.y + (h - glyph_h) * 0.5f;

				renderer.draw_text(*font_, { x, y }, label_, visual_text_color_, style_.font_size.value_or(0.f));
			}
		}

	private:
		void init_defaults() noexcept
		{
			style_.rounding = 4.f;
			style_.text_color = color{ 0.85f, 0.85f, 0.9f, 1.f };
			style_.transition_speed = 12.f;
		}

		[[nodiscard]] float resolved_scale() const noexcept
		{
			return (font_ && style_.font_size.value_or(0.f) > 0.f)
				? style_.font_size.value_or(0.f) / font_->baked_size()
				: 1.f;
		}

		[[nodiscard]] float measure_label_width(const float scale) const noexcept
		{
			float width = 0.f;
			cstd::uint32_t prev_cp = 0;

			const char* s = label_.data();
			const char* end = s + label_.size();

			while (s < end)
			{
				const cstd::uint32_t cp = decode_utf8(s, end);

				if (prev_cp != 0)
				{
					width += font_->kerning(prev_cp, cp) * scale;
				}

				width += font_->glyph_advance(cp) * scale;
				prev_cp = cp;
			}

			return width;
		}

		shared_ptr_t<gui_font> font_;
		string_t label_;

		bool checked_ = false;
		float visual_t_ = 0.f;
		float gap_ = 8.f;

		color box_color_ = { 0.15f, 0.15f, 0.18f, 1.f };
		color hover_color_ = { 0.22f, 0.22f, 0.26f, 1.f };
		color pressed_color_ = { 0.1f, 0.1f, 0.12f, 1.f };
		color check_color_ = { 0.4f, 0.7f, 1.f, 1.f };
		color border_color_ = { 0.4f, 0.4f, 0.45f, 1.f };
		float border_width_ = 1.5f;

		color visual_box_color_ = { 0.15f, 0.15f, 0.18f, 1.f };
		bool box_initialized_ = false;

		function_t<void(bool)> on_change_;
		bool* bound_ = nullptr;
	};
}
