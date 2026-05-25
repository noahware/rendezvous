#pragma once
#include "../render/render.hpp"
#include "../render/texture.hpp"
#include "../input/input.hpp"
#include "layout.hpp"

namespace rv
{
	class gui_font
	{
	public:
		virtual ~gui_font() = default;
		[[nodiscard]] virtual float glyph_advance(cstd::uint32_t codepoint) const noexcept = 0;
		[[nodiscard]] virtual float kerning(cstd::uint32_t left, cstd::uint32_t right) const noexcept = 0;
		[[nodiscard]] virtual float line_height() const noexcept = 0;
		[[nodiscard]] virtual float baked_size() const noexcept = 0;
	};

	class gui_font_impl : public gui_font
	{
	public:
		explicit gui_font_impl(const font& f) noexcept
			: font_(f) { }

		[[nodiscard]] float glyph_advance(const cstd::uint32_t codepoint) const noexcept override
		{
			return font_.glyph(codepoint).advance;
		}

		[[nodiscard]] float kerning(const cstd::uint32_t left, const cstd::uint32_t right) const noexcept override
		{
			return font_.kerning(left, right);
		}

		[[nodiscard]] float line_height() const noexcept override
		{
			return font_.line_height();
		}

		[[nodiscard]] float baked_size() const noexcept override
		{
			return font_.baked_size();
		}

		[[nodiscard]] const font& underlying() const noexcept
		{
			return font_;
		}

	protected:
		const font& font_;
	};

	class gui_renderer
	{
	public:
		virtual ~gui_renderer() = default;
		virtual void draw_rect(position min, position max, color col, float thickness = 1.f, float rounding = 0.f) noexcept = 0;
		virtual void draw_rect_filled(position min, position max, color col, float rounding = 0.f, rounding_flags flags = rounding_flags_all) noexcept = 0;
		virtual void draw_circle_filled(position pos, float radius, color col) noexcept = 0;
		virtual void draw_shadow_rect(position min, position max, color col, float rounding = 0.f,
		                              float shadow_blur = 15.f, float shadow_spread = 0.f,
		                              rounding_flags flags = rounding_flags_all,
		                              bool cut_background = false) noexcept = 0;
		virtual void draw_text(const gui_font& font, position pos, string_view_t text, color col, float size = 0.f) noexcept = 0;
		virtual void push_clip_rect(position min, position max) noexcept = 0;
		virtual void pop_clip_rect() noexcept = 0;
		virtual float delta_time() const noexcept = 0;
	};

	class gui_renderer_impl : public gui_renderer
	{
	public:
		explicit gui_renderer_impl(shared_ptr_t<renderer> renderer)
				:	renderer_(cstd::move(renderer)) { }

		void draw_rect(const position min, const position max, const color col, const float thickness,
		               const float rounding) noexcept override
		{
			return renderer_->draw_rect(min, max, col, thickness, rounding);
		}

		void draw_rect_filled(const position min, const position max, const color col, const float rounding,
		                      const rounding_flags flags) noexcept override
		{
			return renderer_->draw_rect_filled(min, max, col, rounding, flags);
		}

		void draw_circle_filled(const position pos, const float radius, const color col) noexcept override
		{
			return renderer_->draw_circle_filled(pos, radius, col);
		}

		void draw_shadow_rect(const position min, const position max, const color col, const float rounding,
		                      const float shadow_blur, const float shadow_spread,
		                      const rounding_flags flags, const bool cut_background) noexcept override
		{
			return renderer_->draw_shadow_rect(min, max, col, rounding, shadow_blur, shadow_spread, flags, cut_background);
		}

		void draw_text(const gui_font& gf, const position pos, const string_view_t text,
		               const color col, const float size) noexcept override
		{
			const auto* impl = static_cast<const gui_font_impl*>(&gf);
			renderer_->draw_text(impl->underlying(), pos, text, col, size);
		}

		void push_clip_rect(const position min, const position max) noexcept override
		{
			renderer_->push_clip_rect(min, max);
		}

		void pop_clip_rect() noexcept override
		{
			renderer_->pop_clip_rect();
		}

		float delta_time() const noexcept override
		{
			return renderer_->state().delta_time;
		}

	protected:
		shared_ptr_t<renderer> renderer_;
	};

	class gui
	{
	public:
		explicit gui(unique_ptr_t<gui_renderer> renderer, shared_ptr_t<input> input)
				:	renderer_(cstd::move(renderer)), input_(cstd::move(input)) { }

		void render(const vector_2d<float> display_size)
		{
			const auto root = tree_.root();

			layout(*root, display_size, default_style_);
			resolve_positions(*root, position{ 0.f, 0.f }, default_style_);
			update_all(*root, renderer_->delta_time());
			process_events();
			root->render(*renderer_, default_style_);
		}

		[[nodiscard]] shared_ptr_t<element> root() const noexcept
		{
			return tree_.root();
		}

		[[nodiscard]] element_style& default_style() noexcept
		{
			return default_style_;
		}

		[[nodiscard]] const element_style& default_style() const noexcept
		{
			return default_style_;
		}

		[[nodiscard]] const shared_ptr_t<input>& get_input() const noexcept
		{
			return input_;
		}

		template <class T, class ...Args>
		shared_ptr_t<T> make_child(const shared_ptr_t<element>& parent, Args&&... args)
		{
			return tree_.make_child<T>(parent, args...);
		}

	protected:
		void process_events()
		{
			if (!input_)
			{
				return;
			}

			const position mouse_pos = input_->mouse_pos();
			const bool mouse_clicked = input_->is_mouse_clicked(0);
			const bool mouse_down = input_->is_mouse_down(0);
			const float scroll_delta = input_->scroll_delta();

			bool click_handled = false;
			bool enter_handled = false;

			shared_ptr_t<element> scroll_target;

			for (auto& [id, el] : tree_)
			{
				if (!el->is_visible())
				{
					continue;
				}

				const bool hovering = el->contains(mouse_pos);

				if (!click_handled && mouse_clicked && hovering)
				{
					click_handled = el->on_mouse_click();
				}

				if (!enter_handled && hovering && !el->is_hovered())
				{
					el->set_hovered(true);
					enter_handled = el->on_mouse_enter();
				}

				if (!hovering && el->is_hovered())
				{
					el->set_hovered(false);
					el->on_mouse_exit();
				}

				el->set_pressed(hovering && mouse_down);

				if (hovering && el->style().overflow.value_or(overflow_mode::visible) == overflow_mode::scroll)
				{
					scroll_target = el;
				}
			}

			// apply scroll to deepest scrollable
			if (scroll_target && scroll_delta != 0.f)
			{
				scroll_target->apply_scroll(scroll_delta, default_style_);
			}
		}

		unique_ptr_t<gui_renderer> renderer_;
		shared_ptr_t<input> input_;
		element_tree tree_;
		element_style default_style_;
	};
}
