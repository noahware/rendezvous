#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../../input/input.hpp"
#include "../../util/string.hpp"

namespace rv
{
	enum class resize_edge : cstd::uint8_t
	{
		none = 0,
		top = 1 << 0,
		right = 1 << 1,
		bottom = 1 << 2,
		left = 1 << 3,
		top_left = top | left,
		top_right = top | right,
		bottom_left = bottom | left,
		bottom_right = bottom | right
	};

	[[nodiscard]] inline resize_edge operator|(const resize_edge a, const resize_edge b) noexcept
	{
		return static_cast<resize_edge>(static_cast<cstd::uint8_t>(a) | static_cast<cstd::uint8_t>(b));
	}

	[[nodiscard]] inline bool has_flag(const resize_edge value, const resize_edge flag) noexcept
	{
		return (static_cast<cstd::uint8_t>(value) & static_cast<cstd::uint8_t>(flag)) != 0;
	}

	class panel : public element
	{
	public:
		panel() noexcept = default;

		explicit panel(const element_size size, shared_ptr_t<input> input) noexcept
			: element(size), input_(cstd::move(input))
		{
			init_panel_defaults();
		}

		panel& draggable(const bool v) noexcept
		{
			draggable_ = v;

			return *this;
		}

		[[nodiscard]] bool is_dragging() const noexcept
		{
			return dragging_;
		}

		panel& resizable(const bool v) noexcept
		{
			resizable_ = v;

			return *this;
		}

		panel& resize_border(const float width) noexcept
		{
			resize_border_ = width;

			return *this;
		}

		panel& min_panel_size(const float w, const float h) noexcept
		{
			min_panel_w_ = w;
			min_panel_h_ = h;

			return *this;
		}

		panel& max_panel_size(const float w, const float h) noexcept
		{
			max_panel_w_ = w;
			max_panel_h_ = h;

			return *this;
		}

		panel& lock_width(const bool v) noexcept
		{
			lock_width_ = v;

			return *this;
		}

		panel& lock_height(const bool v) noexcept
		{
			lock_height_ = v;

			return *this;
		}

		panel& scrollable(const bool v) noexcept
		{
			scrollable_ = v;
			show_scrollbar(v);

			return *this;
		}

		panel& show_scrollbar(const bool v) noexcept
		{
			element::show_scrollbar(v);

			return *this;
		}

		panel& smooth_scroll(const bool v) noexcept
		{
			element::smooth_scroll(v);

			return *this;
		}

		panel& portrait_focus(const bool show)
		{
			if (const auto g = gui_.lock())
			{
				if (show)
				{
					g->show_portrait_focus(shared_from_this(), portrait_focus_blur_, portrait_focus_color_);
				}
				else
				{
					g->hide_portrait_focus();
				}
			}

			return *this;
		}

		panel& portrait_focus_blur(const float sigma) noexcept
		{
			portrait_focus_blur_ = sigma;

			return *this;
		}

		panel& portrait_focus_color(const color c) noexcept
		{
			portrait_focus_color_ = c;

			return *this;
		}

		panel& auto_portrait_focus(const bool v) noexcept
		{
			auto_portrait_focus_ = v;

			return *this;
		}

		bool on_mouse_click() override;

		void update(const float dt) override;

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
		}

	private:
		void init_panel_defaults() noexcept;

		[[nodiscard]] resize_edge detect_resize_edge(const position mouse, const position p_min,
		                                             const position p_max) const noexcept;

		void update_cursor(const resize_edge edge) const noexcept;

		void apply_resize(const position mouse) noexcept;

		shared_ptr_t<input> input_;

		bool draggable_ = false;
		bool dragging_ = false;
		position drag_anchor_ = {};

		bool resizable_ = false;
		bool resizing_ = false;
		float resize_border_ = 6.f;
		resize_edge active_edge_ = resize_edge::none;
		position resize_anchor_ = {};
		vector_2d<float> resize_start_size_ = {};
		position resize_start_pos_ = {};

		float min_panel_w_ = 100.f;
		float min_panel_h_ = 50.f;
		float max_panel_w_ = 0.f;
		float max_panel_h_ = 0.f;
		bool lock_width_ = false;
		bool lock_height_ = false;

		bool scrollable_ = false;

		float portrait_focus_blur_ = 8.f;
		color portrait_focus_color_ = { 0, 0, 0, 0.3f };
		bool auto_portrait_focus_ = false;
	};
}
