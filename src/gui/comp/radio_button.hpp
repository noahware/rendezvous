#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../text_metrics.hpp"
#include "../../util/string.hpp"

namespace rv
{
	class radio_button final : public element
	{
	public:
		radio_button() noexcept = default;

		explicit radio_button(const element_size size) noexcept
			: element(size)
		{
			init_defaults();
		}

		radio_button(const element_size size, shared_ptr_t<gui_font> font) noexcept
			: element(size), font_(cstd::move(font))
		{
			init_defaults();
		}

		radio_button& label(const string_view_t text)
		{
			label_ = string_t(text);
			mark_layout_dirty();

			return *this;
		}

		radio_button& value(const cstd::int32_t v) noexcept
		{
			value_ = v;

			return *this;
		}

		[[nodiscard]] bool is_selected() const noexcept
		{
			return selected_;
		}

		radio_button& select() noexcept
		{
			if (bound_)
			{
				*bound_ = value_;
			}

			selected_ = true;

			if (on_change_)
			{
				on_change_(value_);
			}

			return *this;
		}

		radio_button& on_change(function_t<void(cstd::int32_t)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		radio_button& bind(cstd::int32_t* ptr) noexcept
		{
			bound_ = ptr;

			if (bound_)
			{
				selected_ = (*bound_ == value_);
				visual_t_ = selected_ ? 1.f : 0.f;
			}

			return *this;
		}

		radio_button& circle_color(const color col) noexcept
		{
			circle_color_ = col;

			return *this;
		}

		radio_button& dot_color(const color col) noexcept
		{
			dot_color_ = col;

			return *this;
		}

		radio_button& hover_color(const color col) noexcept
		{
			hover_color_ = col;

			return *this;
		}

		radio_button& pressed_color(const color col) noexcept
		{
			pressed_color_ = col;

			return *this;
		}

		radio_button& gap(const float px) noexcept
		{
			gap_ = px;
			mark_layout_dirty();

			return *this;
		}

		bool on_mouse_click() override
		{
			if (bound_)
			{
				*bound_ = value_;
			}

			selected_ = true;

			if (on_change_)
			{
				on_change_(value_);
			}

			return true;
		}

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override;

		void update(const float dt) override;

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override;

	private:
		void init_defaults() noexcept
		{
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

		cstd::int32_t value_ = 0;
		cstd::int32_t* bound_ = nullptr;
		bool selected_ = false;
		float visual_t_ = 0.f;
		float gap_ = 8.f;

		color circle_color_{ 0.15f, 0.15f, 0.18f, 1.f };
		color hover_color_{ 0.22f, 0.22f, 0.26f, 1.f };
		color pressed_color_{ 0.1f, 0.1f, 0.12f, 1.f };
		color dot_color_{ 0.4f, 0.7f, 1.f, 1.f };
		color border_color_{ 0.4f, 0.4f, 0.45f, 1.f };
		float border_width_ = 1.5f;

		transition<color> circle_fill_;

		function_t<void(cstd::int32_t)> on_change_;
	};
}
