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

			bool click_handled = false;
			bool enter_handled = false;

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
			}
		}

		unique_ptr_t<gui_renderer> renderer_;
		shared_ptr_t<input> input_;
		element_tree tree_;
		element_style default_style_;
	};
}
