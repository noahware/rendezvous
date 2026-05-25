#pragma once
#include "../render/render.hpp"
#include "../input/input.hpp"
#include "layout.hpp"

namespace rv
{
	class gui_renderer
	{
	public:
		virtual void draw_rect(position min, position max, color col, float thickness = 1.f, float rounding = 0.f) noexcept = 0;
		virtual void draw_rect_filled(position min, position max, color col, float rounding = 0.f, rounding_flags flags = rounding_flags_all) noexcept = 0;
		virtual void draw_circle_filled(position pos, float radius, color col) noexcept = 0;
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

	// element::render() defined here because gui_renderer must be complete
	inline void element::render(gui_renderer& renderer, const element_style& defaults,
	                            const position offset) const
	{
		if (!visible_)
		{
			return;
		}

		position total_offset = offset;
		const auto anim = animated_props();

		if (anim && anim->offset)
		{
			total_offset.x += anim->offset->x;
			total_offset.y += anim->offset->y;
		}

		const position min = { computed_pos_.x + total_offset.x, computed_pos_.y + total_offset.y };
		const position max = { min.x + computed_size_.x, min.y + computed_size_.y };

		render_self(renderer, min, max);

		const auto ov = style_.overflow.value_or(defaults.overflow.value_or(overflow_mode::visible));
		const bool clip = (ov == overflow_mode::hidden || ov == overflow_mode::scroll);

		if (clip)
		{
			renderer.push_clip_rect(min, max);
		}

		position child_offset = total_offset;

		if (ov == overflow_mode::scroll)
		{
			child_offset.x += scroll_offset_.x;
			child_offset.y += scroll_offset_.y;
		}

		for (const auto& child : children_)
		{
			child->render(renderer, defaults, child_offset);
		}

		if (clip)
		{
			renderer.pop_clip_rect();
		}
	}

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
			const float scroll_delta = input_->scroll_delta();

			bool click_handled = false;
			bool enter_handled = false;

			// find deepest scrollable element for scroll input
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

				// track scrollable elements under mouse
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
