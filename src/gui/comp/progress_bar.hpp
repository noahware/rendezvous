#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../text_metrics.hpp"
#include "../../util/string.hpp"

namespace rv
{
	class progress_bar final : public element
	{
	public:
		progress_bar() noexcept = default;

		explicit progress_bar(const element_size size) noexcept
			: element(size)
		{
			init_defaults();
		}

		progress_bar(const element_size size, shared_ptr_t<gui_font> font) noexcept
			: element(size), font_(cstd::move(font))
		{
			init_defaults();
		}

		progress_bar& value(const float v) noexcept
		{
			value_ = clamp01(v);

			if (bound_)
			{
				*bound_ = value_;
			}

			return *this;
		}

		[[nodiscard]] float get_value() const noexcept
		{
			return value_;
		}

		progress_bar& bind(float* ptr) noexcept
		{
			bound_ = ptr;

			if (bound_)
			{
				value_ = clamp01(*bound_);
			}

			return *this;
		}

		progress_bar& on_change(function_t<void(float)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		progress_bar& fill_color(const color col) noexcept
		{
			fill_color_ = col;

			return *this;
		}

		progress_bar& track_color(const color col) noexcept
		{
			track_color_ = col;

			return *this;
		}

		progress_bar& show_text(const bool on) noexcept
		{
			show_text_ = on;

			return *this;
		}

		progress_bar& text_format(function_t<string_t(float)> fmt)
		{
			text_format_ = cstd::move(fmt);

			return *this;
		}

		void update(const float dt) override;

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override;

	private:
		void init_defaults() noexcept
		{
			style_.rounding = 10.f;
			style_.transition_speed = 12.f;
		}

		[[nodiscard]] static float clamp01(const float v) noexcept
		{
			return cstd::fmaxf(0.f, cstd::fminf(1.f, v));
		}

		shared_ptr_t<gui_font> font_;

		float value_ = 0.f;
		float visual_value_ = 0.f;
		float* bound_ = nullptr;

		color fill_color_{ 0.4f, 0.7f, 1.f, 1.f };
		color track_color_{ 0.15f, 0.15f, 0.18f, 1.f };

		bool show_text_ = false;
		function_t<string_t(float)> text_format_;
		function_t<void(float)> on_change_;
	};
}
