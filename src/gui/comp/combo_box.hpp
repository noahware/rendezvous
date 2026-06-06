#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../../input/input.hpp"
#include "../../util/string.hpp"

namespace rv
{
	constexpr float combo_row_pad_v = 4.f;
	constexpr float combo_row_pad_h = 10.f;

	class combo_popup final : public element
	{
	public:
		explicit combo_popup(const element_size size) noexcept
				:	element(size)
		{
			style_.background_color = color{ 0.15f, 0.15f, 0.18f, 1.f };
			style_.rounding = 6.f;
			style_.text_color = color{ 1.f, 1.f, 1.f, 1.f };
			// no padding/border so the content box matches the element box; this keeps
			// per-row hit-testing (update) aligned with row rendering (render_self).
			style_.shadow_color = color{ 0.f, 0.f, 0.f, 0.5f };
			style_.shadow_blur = 12.f;

			z_index(z_index_popup); // dropdown sits on the popup layer, above any floating panels
		}

		void configure(shared_ptr_t<gui_font> font, shared_ptr_t<input> input,
		               const vector_t<string_t>* options, const cstd::int32_t* selected,
		               function_t<void(cstd::int32_t)> on_select) noexcept;

		bool on_mouse_click() override;

		void update(const float dt) override;

		[[nodiscard]] float row_height() const noexcept;

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override;

	private:
		[[nodiscard]] float current_scale() const noexcept
		{
			const float font_size = style_.font_size.value_or(0.f);

			return (font_size > 0.f && font_) ? font_size / font_->baked_size() : 1.f;
		}

		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;
		const vector_t<string_t>* options_ = nullptr;
		const cstd::int32_t* selected_ = nullptr;
		function_t<void(cstd::int32_t)> on_select_;

		cstd::int32_t hovered_row_ = -1;
		float row_height_ = 0.f;

		color row_hover_color_ = { 0.26f, 0.45f, 0.78f, 0.55f };
		color row_selected_color_ = { 0.3f, 0.3f, 0.36f, 1.f };
	};

	// The combo box header/control. Click to open the dropdown list, click an option to
	// select, click elsewhere to close (focus-out). Index-based selection mirroring the
	// slider/checkbox bind pattern.
	class combo_box final : public element
	{
	public:
		combo_box(const element_size size, shared_ptr_t<gui_font> font, shared_ptr_t<input> input) noexcept;

		combo_box& options(vector_t<string_t> opts);

		combo_box& selected(const cstd::int32_t index) noexcept
		{
			selected_ = index;

			if (bound_)
			{
				*bound_ = selected_;
			}

			return *this;
		}

		combo_box& bind(cstd::int32_t* ptr) noexcept
		{
			bound_ = ptr;

			if (bound_)
			{
				selected_ = *bound_;
			}

			return *this;
		}

		combo_box& on_change(function_t<void(cstd::int32_t)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		combo_box& placeholder(const string_view_t text)
		{
			placeholder_ = string_t(text);

			return *this;
		}

		[[nodiscard]] cstd::int32_t selected() const noexcept
		{
			return selected_;
		}

		[[nodiscard]] bool focusable() const noexcept override
		{
			return true;
		}

		bool on_mouse_click() override
		{
			open_ = !open_;

			return true;
		}

		void update(const float dt) override;

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override;

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override;

	private:
		void init_defaults() noexcept;

		[[nodiscard]] float current_scale() const noexcept
		{
			const float font_size = style_.font_size.value_or(0.f);

			return (font_size > 0.f && font_) ? font_size / font_->baked_size() : 1.f;
		}

		void commit(cstd::int32_t index);

		static constexpr cstd::int32_t max_visible_rows = 8;

		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;
		shared_ptr_t<combo_popup> popup_;

		vector_t<string_t> options_;
		cstd::int32_t selected_ = -1;
		cstd::int32_t* bound_ = nullptr;
		bool open_ = false;
		string_t placeholder_ = "Select...";

		function_t<void(cstd::int32_t)> on_change_;
	};
}
