#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../text_metrics.hpp"
#include "../../input/input.hpp"
#include "../../util/string.hpp"

namespace rv
{
	class collapsing_header final : public element
	{
	public:
		collapsing_header() noexcept = default;

		collapsing_header(element_size size, shared_ptr_t<gui_font> font, shared_ptr_t<input> input) noexcept;

		collapsing_header& label(const string_view_t text)
		{
			label_ = string_t(text);

			return *this;
		}

		collapsing_header& expanded(const bool on) noexcept
		{
			expanded_ = on;
			arrow_angle_ = on ? 90.f : 0.f;

			if (content_)
			{
				content_->visible(expanded_);
				mark_layout_dirty();
			}

			return *this;
		}

		[[nodiscard]] bool is_expanded() const noexcept
		{
			return expanded_;
		}

		collapsing_header& on_toggle(function_t<void(bool)> callback)
		{
			on_toggle_ = cstd::move(callback);

			return *this;
		}

		collapsing_header& header_color(const color col) noexcept
		{
			header_color_ = col;

			return *this;
		}

		collapsing_header& header_hover_color(const color col) noexcept
		{
			header_hover_color_ = col;

			return *this;
		}

		collapsing_header& arrow_color(const color col) noexcept
		{
			arrow_color_ = col;

			return *this;
		}

		[[nodiscard]] element& content() noexcept
		{
			return *content_;
		}

		bool on_mouse_click() override;

		void update(const float dt) override;

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override;

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override;

	private:
		void init_defaults() noexcept
		{
			style_.direction = layout_direction::vertical;
			style_.text_color = color{ 0.9f, 0.9f, 0.95f, 1.f };
			style_.transition_speed = 12.f;
			style_.rounding = 6.f;
			style_.padding = border_vector{ header_height_, 0.f, 0.f, 0.f };
		}

		void toggle();

		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;
		shared_ptr_t<element> content_;

		string_t label_;
		bool expanded_ = true;
		float arrow_angle_ = 90.f;
		float visual_expand_t_ = 1.f;
		bool header_hovered_ = false;

		static constexpr float header_height_ = 32.f;
		static constexpr float arrow_size_ = 6.f;
		static constexpr float arrow_indent_ = 14.f;
		static constexpr float text_indent_ = 28.f;

		color header_color_{ 0.15f, 0.15f, 0.18f, 1.f };
		color header_hover_color_{ 0.2f, 0.2f, 0.24f, 1.f };
		color arrow_color_{ 0.6f, 0.6f, 0.65f, 1.f };
		transition<color> header_fill_;

		function_t<void(bool)> on_toggle_;
	};
}
