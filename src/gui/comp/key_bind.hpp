#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../../input/input.hpp"
#include "../../input/key_names.hpp"
#include "../../util/string.hpp"

namespace rv
{
	class key_bind final : public element
	{
	public:
		key_bind() noexcept = default;

		explicit key_bind(const element_size size) noexcept
			: element(size)
		{
			init_defaults();
		}

		key_bind(const element_size size, shared_ptr_t<gui_font> font, shared_ptr_t<input> input) noexcept
			: element(size), font_(cstd::move(font)), input_(cstd::move(input))
		{
			init_defaults();
		}

		key_bind& value(const key k) noexcept
		{
			key_code_ = k;

			if (bound_)
			{
				*bound_ = key_code_;
			}

			return *this;
		}

		[[nodiscard]] key current_key() const noexcept
		{
			return key_code_;
		}

		key_bind& bind(key* ptr) noexcept
		{
			bound_ = ptr;

			if (bound_)
			{
				key_code_ = *bound_;
			}

			return *this;
		}

		key_bind& on_change(function_t<void(key)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		key_bind& hover_color(const color col) noexcept
		{
			hover_color_ = col;

			return *this;
		}

		key_bind& pressed_color(const color col) noexcept
		{
			pressed_color_ = col;

			return *this;
		}

		key_bind& listening_color(const color col) noexcept
		{
			listening_color_ = col;

			return *this;
		}

		key_bind& listening_border_color(const color col) noexcept
		{
			listening_border_ = col;

			return *this;
		}

		[[nodiscard]] bool focusable() const noexcept override
		{
			return true;
		}

		bool on_mouse_click() override;

		void update(const float dt) override;

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override;

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override;

	private:
		void init_defaults() noexcept
		{
			style_.background_color = color{ 0.12f, 0.12f, 0.15f, 1.f };
			style_.rounding = 6.f;
			style_.border_color = color{ 0.3f, 0.3f, 0.36f, 1.f };
			style_.border_width = border_vector{ 1.f, 1.f, 1.f, 1.f };
			style_.text_color = color{ 1.f, 1.f, 1.f, 1.f };
			style_.padding = border_vector{ 6.f, 10.f, 6.f, 10.f };
			style_.transition_speed = 12.f;
		}

		[[nodiscard]] float current_scale() const noexcept
		{
			const float font_size = style_.font_size.value_or(0.f);

			return (font_size > 0.f && font_) ? font_size / font_->baked_size() : 1.f;
		}

		[[nodiscard]] float measure_text_width(const string_view_t text, const float scale) const noexcept;

		void commit(const key new_key);

		void cancel() noexcept
		{
			key_code_ = prev_key_code_;
			listening_ = false;
		}

		void scan_keys();

		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;

		key key_code_ = key::none;
		key prev_key_code_ = key::none;
		bool listening_ = false;

		key* bound_ = nullptr;
		function_t<void(key)> on_change_;

		color hover_color_ = { 0.22f, 0.22f, 0.26f, 1.f };
		color pressed_color_ = { 0.1f, 0.1f, 0.12f, 1.f };
		color listening_color_ = { 0.15f, 0.20f, 0.30f, 1.f };
		color listening_border_ = { 0.35f, 0.55f, 0.85f, 1.f };
	};
}
