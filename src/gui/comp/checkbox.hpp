#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../text_metrics.hpp"
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

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override;

		void update(const float dt) override;

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override;

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

		// box fill eases toward its per-state target (box/hover/pressed) via the shared helper.
		transition<color> box_fill_;

		function_t<void(bool)> on_change_;
		bool* bound_ = nullptr;
	};
}
