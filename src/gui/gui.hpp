#pragma once
#include "../render/render.hpp"
#include "../render/texture.hpp"
#include "../input/input.hpp"
#include "../util/string.hpp"
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
		virtual void push_clip_rect(position min, position max, float rounding = 0.f, rounding_flags flags = rounding_flags_all) noexcept = 0;
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

		void push_clip_rect(const position min, const position max, const float rounding = 0.f, const rounding_flags flags = rounding_flags_all) noexcept override
		{
			renderer_->push_clip_rect(min, max, rounding, flags);
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
				:	renderer_(cstd::move(renderer)), input_(cstd::move(input))
		{
			tree_.set_layout_dirty_ptr(&layout_dirty_);
		}

		void render(const vector_2d<float> display_size)
		{
			const auto root = tree_.root();

			const bool size_changed = display_size.x != last_display_size_.x
				|| display_size.y != last_display_size_.y;

			if (layout_dirty_ || size_changed)
			{
				layout(*root, display_size, default_style_);
				resolve_positions(*root, position{ 0.f, 0.f }, default_style_);
				layout_dirty_ = false;
				last_display_size_ = display_size;
			}

			update_all(*root, renderer_->delta_time());
			process_events();

			// advance the hover-to-show timer; reset whenever the hovered target changes.
			// tooltip_hover_ holds this frame's live element, so deref stays safe.
			const float dt = renderer_->delta_time();

			if (tooltip_hover_ != tooltip_target_)
			{
				tooltip_target_ = tooltip_hover_;
				tooltip_timer_ = 0.f;
			}
			else if (tooltip_target_)
			{
				tooltip_timer_ += dt;
			}

			root->render(*renderer_, default_style_);

			// drawn last and unclipped, so the tooltip always paints over the tree.
			if (tooltip_target_ && tooltip_timer_ >= tooltip_delay && font_ && input_)
			{
				draw_tooltip(tooltip_target_->tooltip(), input_->mouse_pos(), display_size);
			}
		}

		void set_font(shared_ptr_t<gui_font> font) noexcept
		{
			font_ = cstd::move(font);
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
		void process_events_recursive(const shared_ptr_t<element>& el, const position& mouse_pos, const bool mouse_clicked, const bool mouse_down, bool& click_handled, bool& enter_handled, shared_ptr_t<element>& scroll_target, element*& clicked, element*& tooltip_hover)
		{
			if (!el->is_visible())
			{
				return;
			}

			const auto& children = el->children();
			for (auto it = children.rbegin(); it != children.rend(); ++it)
			{
				process_events_recursive(*it, mouse_pos, mouse_clicked, mouse_down, click_handled, enter_handled, scroll_target, clicked, tooltip_hover);
			}

			const position visual_min = el->visual_pos();
			const position visual_max = { visual_min.x + el->computed_size().x, visual_min.y + el->computed_size().y };
			const bool hovering = mouse_pos.x >= visual_min.x && mouse_pos.x <= visual_max.x &&
			                      mouse_pos.y >= visual_min.y && mouse_pos.y <= visual_max.y;

			// children are walked first (topmost-first), so the first hovered element
			// with a tooltip is the deepest/topmost one under the cursor.
			if (hovering && !tooltip_hover && el->has_tooltip())
			{
				tooltip_hover = el.get();
			}

			if (!click_handled && mouse_clicked && hovering)
			{
				click_handled = el->on_mouse_click();

				if (click_handled)
				{
					clicked = el.get();
				}
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
				if (!scroll_target)
				{
					scroll_target = el;
				}
			}
		}

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
			element* clicked = nullptr;
			element* tooltip_hover = nullptr;

			if (tree_.root())
			{
				process_events_recursive(tree_.root(), mouse_pos, mouse_clicked, mouse_down, click_handled, enter_handled, scroll_target, clicked, tooltip_hover);
			}

			tooltip_hover_ = tooltip_hover;

			// focus only changes on a click, so it persists across frames otherwise.
			// the consuming element keeps focus if focusable; everyone else is cleared
			// (clicking a button or empty space defocuses).
			if (mouse_clicked)
			{
				const bool keep = clicked && clicked->focusable();

				for (auto& [id, el] : tree_)
				{
					el->set_focused(keep && el.get() == clicked);
				}
			}

			// apply scroll to deepest scrollable
			if (scroll_target && scroll_delta != 0.f)
			{
				scroll_target->apply_scroll(scroll_delta, default_style_);
			}
		}

		// Floating hint box near the cursor. Supports embedded newlines and clamps to the
		// display so it never spills off-screen.
		void draw_tooltip(const string_t& text, const position mouse, const vector_2d<float> display_size) const
		{
			if (text.empty() || !font_)
			{
				return;
			}

			const float baked = font_->baked_size();
			const float scale = (tooltip_font_size > 0.f && baked > 0.f) ? tooltip_font_size / baked : 1.f;
			const float line_h = font_->line_height() * scale;

			// measure the widest line and count lines in one pass
			float max_w = 0.f;
			float cur_w = 0.f;
			int line_count = 1;
			cstd::uint32_t prev = 0;

			const char* s = text.data();
			const char* const end = s + text.size();

			while (s < end)
			{
				const cstd::uint32_t cp = decode_utf8(s, end);

				if (cp == U'\n')
				{
					max_w = cstd::fmaxf(max_w, cur_w);
					cur_w = 0.f;
					prev = 0;
					++line_count;

					continue;
				}

				float adv = font_->glyph_advance(cp) * scale;

				if (prev != 0)
				{
					adv += font_->kerning(prev, cp) * scale;
				}

				cur_w += adv;
				prev = cp;
			}

			max_w = cstd::fmaxf(max_w, cur_w);

			constexpr float pad_x = 8.f;
			constexpr float pad_y = 5.f;
			const float box_w = max_w + pad_x * 2.f;
			const float box_h = line_h * static_cast<float>(line_count) + pad_y * 2.f;

			position min = { mouse.x + 14.f, mouse.y + 18.f };

			if (min.x + box_w > display_size.x)
			{
				min.x = cstd::fmaxf(0.f, display_size.x - box_w);
			}

			if (min.y + box_h > display_size.y)
			{
				min.y = cstd::fmaxf(0.f, mouse.y - box_h - 4.f);
			}

			const position max = { min.x + box_w, min.y + box_h };

			renderer_->draw_shadow_rect(min, max, color{ 0.f, 0.f, 0.f, 0.45f }, 4.f, 10.f, 0.f, rounding_flags_all, false);
			renderer_->draw_rect_filled(min, max, color{ 0.1f, 0.1f, 0.12f, 0.97f }, 4.f, rounding_flags_all);
			renderer_->draw_rect(min, max, color{ 0.32f, 0.32f, 0.38f, 1.f }, 1.f, 4.f);

			// draw each line, splitting on newlines
			position pen = { min.x + pad_x, min.y + pad_y };
			cstd::size_t line_start = 0;

			for (cstd::size_t i = 0; i <= text.size(); ++i)
			{
				if (i == text.size() || text[i] == '\n')
				{
					const string_view_t line{ text.data() + line_start, i - line_start };

					if (!line.empty())
					{
						renderer_->draw_text(*font_, pen, line, color{ 0.95f, 0.95f, 0.97f, 1.f }, tooltip_font_size);
					}

					pen.y += line_h;
					line_start = i + 1;
				}
			}
		}

		static constexpr float tooltip_delay = 0.4f;
		static constexpr float tooltip_font_size = 14.f;

		unique_ptr_t<gui_renderer> renderer_;
		shared_ptr_t<input> input_;
		element_tree tree_;
		element_style default_style_;

		shared_ptr_t<gui_font> font_;
		element* tooltip_target_ = nullptr;
		element* tooltip_hover_ = nullptr;
		float tooltip_timer_ = 0.f;

		bool layout_dirty_ = true;
		vector_2d<float> last_display_size_ = { };
	};
}
