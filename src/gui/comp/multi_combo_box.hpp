#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../text_metrics.hpp"
#include "../../input/input.hpp"
#include "../../util/string.hpp"
#include "combo_box.hpp"

namespace rv
{
	constexpr float multi_combo_check_size = 12.f;
	constexpr float multi_combo_check_gap = 6.f;

	class multi_combo_popup final : public element
	{
	public:
		explicit multi_combo_popup(const element_size size) noexcept
				:	element(size)
		{
			style_.background_color = color{ 0.15f, 0.15f, 0.18f, 1.f };
			style_.rounding = 6.f;
			style_.text_color = color{ 1.f, 1.f, 1.f, 1.f };
			style_.shadow_color = color{ 0.f, 0.f, 0.f, 0.5f };
			style_.shadow_blur = 12.f;

			z_index(z_index_popup);
		}

		void configure(shared_ptr_t<gui_font> font, shared_ptr_t<input> input,
		               const vector_t<string_t>* options,
		               const vector_t<cstd::int32_t>* selected_indices,
		               function_t<void(cstd::int32_t)> on_toggle) noexcept;

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

		[[nodiscard]] bool is_index_selected(cstd::int32_t index) const noexcept;

		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;
		const vector_t<string_t>* options_ = nullptr;
		const vector_t<cstd::int32_t>* selected_indices_ = nullptr;
		function_t<void(cstd::int32_t)> on_toggle_;

		cstd::int32_t hovered_row_ = -1;
		float row_height_ = 0.f;

		color row_hover_color_ = { 0.26f, 0.45f, 0.78f, 0.55f };
		color row_selected_color_ = { 0.3f, 0.3f, 0.36f, 1.f };
		color check_color_ = { 0.4f, 0.7f, 1.f, 1.f };
	};

	class multi_combo_box final : public element
	{
	public:
		multi_combo_box(const element_size size, shared_ptr_t<gui_font> font, shared_ptr_t<input> input) noexcept;

		multi_combo_box& options(vector_t<string_t> opts);

		multi_combo_box& selected(vector_t<cstd::int32_t> indices);

		multi_combo_box& select_all();

		multi_combo_box& deselect_all() noexcept
		{
			selected_indices_.clear();

			if (bound_)
			{
				*bound_ = selected_indices_;
			}

			return *this;
		}

		multi_combo_box& bind(vector_t<cstd::int32_t>* ptr) noexcept
		{
			bound_ = ptr;

			if (bound_)
			{
				selected_indices_ = *bound_;
			}

			return *this;
		}

		multi_combo_box& on_change(function_t<void(const vector_t<cstd::int32_t>&)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		multi_combo_box& placeholder(const string_view_t text)
		{
			placeholder_ = string_t(text);

			return *this;
		}

		[[nodiscard]] const vector_t<cstd::int32_t>& selected_indices() const noexcept
		{
			return selected_indices_;
		}

		[[nodiscard]] cstd::int32_t selected_count() const noexcept
		{
			return static_cast<cstd::int32_t>(selected_indices_.size());
		}

		[[nodiscard]] bool is_selected(cstd::int32_t index) const noexcept;

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

		void toggle(cstd::int32_t index);

		[[nodiscard]] string_t build_display_text() const;

		static constexpr cstd::int32_t max_visible_rows = 8;

		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;
		shared_ptr_t<multi_combo_popup> popup_;

		vector_t<string_t> options_;
		vector_t<cstd::int32_t> selected_indices_;
		vector_t<cstd::int32_t>* bound_ = nullptr;
		bool open_ = false;
		string_t placeholder_ = "Select...";

		function_t<void(const vector_t<cstd::int32_t>&)> on_change_;
	};
}
