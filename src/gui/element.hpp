#pragma once
#include "../render/position.hpp"
#include "../util/hash.hpp"
#include "../util/types.hpp"
#include "styled_size.hpp"
#include "animation.hpp"
#include "element_types.hpp"

namespace rv
{
	class gui_renderer;

	enum class key : cstd::int32_t;
	class gui;
	class button;
	class checkbox;
	class text_element;
	class text_box;
	class combo_box;
	class color_picker;
	class panel;
	class plot_lines;
	class value_inspector;
	class key_bind;
	template <class T> class slider;
	template <class T> class range_slider;

	#define RV_WIDGET_FACTORY_DECLS                                                                 \
		button& add_button(string_view_t text = {});                                              \
		checkbox& add_checkbox(string_view_t label = {});                                         \
		text_element& add_label(string_view_t text = {});                                         \
		text_box& add_text_input(string_view_t text = {});                                        \
		text_box& add_text_area(string_view_t text = {});                                         \
		slider<float>& add_slider(float mn = 0.f, float mx = 1.f, float v = 0.f);                 \
		range_slider<float>& add_range_slider(float mn = 0.f, float mx = 1.f, float lo = 0.f, float hi = 1.f); \
		combo_box& add_combo_box(vector_t<string_t> options = {});                                \
		color_picker& add_color_picker(color initial = {});                                       \
		panel& add_panel();                                                                       \
		plot_lines& add_plot_lines();                                                             \
		plot_lines& add_plot_var(string_view_t label, const float* value);                        \
		value_inspector& add_inspector();                                                         \
		element& add_row();                                                                       \
		element& add_column();                                                                    \
		element& add_container(string_view_t title = {});                                    \
		key_bind& add_key_bind(key initial_key = {});

	template <class T, class ...Args>
	[[nodiscard]] shared_ptr_t<T> make_element(Args&&... args)
	{
		return cstd::make_shared<T>(args...);
	}

	class element : public enable_shared_from_this_t<element>
	{
	public:
		virtual ~element() = default;
		element() noexcept = default;

		explicit element(const element_size size) noexcept
				:	style_{ .size = size } { }

		RV_WIDGET_FACTORY_DECLS

		virtual void update(float dt);

		virtual bool on_mouse_click()
		{
			return false;
		}

		virtual bool on_mouse_enter()
		{
			return false;
		}

		virtual bool on_mouse_exit()
		{
			return false;
		}

		[[nodiscard]] span_t<shared_ptr_t<element>> children() noexcept
		{
			return children_;
		}

		[[nodiscard]] span_t<const shared_ptr_t<element>> children() const noexcept
		{
			return children_;
		}

		void set_layout_dirty_ptr(bool* ptr) noexcept
		{
			layout_dirty_ptr_ = ptr;

			for (const auto& child : children_)
			{
				child->set_layout_dirty_ptr(ptr);
			}
		}

		void add_child(shared_ptr_t<element> child)
		{
			if (layout_dirty_ptr_)
			{
				child->set_layout_dirty_ptr(layout_dirty_ptr_);
			}

			if (const auto g = gui_.lock())
			{
				child->set_gui(g);
			}

			children_.push_back(cstd::move(child));
			mark_layout_dirty();
		}

		void set_gui(const shared_ptr_t<gui>& g)
		{
			gui_ = g;

			for (const auto& child : children_)
			{
				child->set_gui(g);
			}
		}

		template <class T, class ...Args>
		shared_ptr_t<T> make_child(Args&&... args)
		{
			auto child = make_element<T>(args...);

			children_.push_back(child);

			return child;
		}

		[[nodiscard]] const element_size& declared_size() const noexcept
		{
			return style_.size;
		}

		void set_declared_size(const element_size size) noexcept
		{
			style_.size = size;
			mark_layout_dirty();
		}

		[[nodiscard]] vector_2d<float> computed_size() const noexcept
		{
			return computed_size_;
		}

		void set_computed_size(const vector_2d<float> size) noexcept
		{
			computed_size_ = size;
		}

		[[nodiscard]] virtual vector_2d<float> content_size(const vector_2d<float> available) const noexcept
		{
			return { 0.f, 0.f };
		}

		[[nodiscard]] position computed_pos() const noexcept
		{
			return computed_pos_;
		}

		void set_computed_pos(const position pos) noexcept
		{
			computed_pos_ = pos;
		}

		[[nodiscard]] position visual_pos() const noexcept
		{
			return visual_pos_;
		}

		void set_visual_pos(const position pos) noexcept
		{
			visual_pos_ = pos;
		}

		[[nodiscard]] bool is_hovered() const noexcept
		{
			return hovered_;
		}

		void set_hovered(const bool hovered) noexcept
		{
			hovered_ = hovered;
		}

		[[nodiscard]] bool is_pressed() const noexcept
		{
			return pressed_;
		}

		void set_pressed(const bool pressed) noexcept
		{
			pressed_ = pressed;
		}

		[[nodiscard]] bool is_focused() const noexcept
		{
			return focused_;
		}

		void set_focused(const bool focused) noexcept
		{
			focused_ = focused;
		}

		// Widgets that own the keyboard when clicked (e.g. text_box) override this.
		[[nodiscard]] virtual bool focusable() const noexcept
		{
			return false;
		}

		[[nodiscard]] bool is_visible() const noexcept
		{
			return visible_;
		}

		void set_visible(const bool visible) noexcept
		{
			visible_ = visible;
		}

		[[nodiscard]] bool contains(const position point) const noexcept
		{
			return point.x >= computed_pos_.x
				&& point.y >= computed_pos_.y
				&& point.x <= computed_pos_.x + computed_size_.x
				&& point.y <= computed_pos_.y + computed_size_.y;
		}

		[[nodiscard]] element_style& style() noexcept
		{
			return style_;
		}

		[[nodiscard]] const element_style& style() const noexcept
		{
			return style_;
		}

		element& gap(const optional_t<float> gap) noexcept
		{
			style_.gap = gap;
			mark_layout_dirty();

			return *this;
		}

		element& direction(const layout_direction direction) noexcept
		{
			style_.direction = direction;
			mark_layout_dirty();

			return *this;
		}

		element& align(const alignment align) noexcept
		{
			style_.align = align;
			mark_layout_dirty();

			return *this;
		}

		element& justify(const justify_content justify) noexcept
		{
			style_.justify = justify;
			mark_layout_dirty();

			return *this;
		}

		element& margin(const border_vector margin) noexcept
		{
			style_.margin = margin;
			mark_layout_dirty();

			return *this;
		}

		element& margin(const float all) noexcept
		{
			style_.margin = border_vector{ all, all, all, all };
			mark_layout_dirty();

			return *this;
		}

		element& padding(const border_vector pad) noexcept
		{
			style_.padding = pad;
			mark_layout_dirty();

			return *this;
		}

		element& padding(const float all) noexcept
		{
			style_.padding = border_vector{ all, all, all, all };
			mark_layout_dirty();

			return *this;
		}

		element& border_width(const border_vector bw) noexcept
		{
			style_.border_width = bw;
			mark_layout_dirty();

			return *this;
		}

		element& border_width(const float all) noexcept
		{
			style_.border_width = border_vector{ all, all, all, all };
			mark_layout_dirty();

			return *this;
		}

		element& row_gap(const float g) noexcept
		{
			style_.row_gap = g;
			mark_layout_dirty();

			return *this;
		}

		element& column_gap(const float g) noexcept
		{
			style_.column_gap = g;
			mark_layout_dirty();

			return *this;
		}

		element& align_self(const alignment a) noexcept
		{
			style_.align_self = a;
			mark_layout_dirty();

			return *this;
		}

		element& min_width(const styled_size s) noexcept
		{
			style_.min_width = s;
			mark_layout_dirty();

			return *this;
		}

		element& max_width(const styled_size s) noexcept
		{
			style_.max_width = s;
			mark_layout_dirty();

			return *this;
		}

		element& min_height(const styled_size s) noexcept
		{
			style_.min_height = s;
			mark_layout_dirty();

			return *this;
		}

		element& max_height(const styled_size s) noexcept
		{
			style_.max_height = s;
			mark_layout_dirty();

			return *this;
		}

		element& aspect_ratio(const float r) noexcept
		{
			style_.aspect_ratio = r;
			mark_layout_dirty();

			return *this;
		}

		element& flex_grow(const float g) noexcept
		{
			style_.flex_grow = g;
			mark_layout_dirty();

			return *this;
		}

		element& flex_shrink(const float s) noexcept
		{
			style_.flex_shrink = s;
			mark_layout_dirty();

			return *this;
		}

		element& flex_basis(const styled_size b) noexcept
		{
			style_.flex_basis = b;
			mark_layout_dirty();

			return *this;
		}

		element& flex(const float grow, const float shrink, const styled_size basis) noexcept
		{
			style_.flex_grow = grow;
			style_.flex_shrink = shrink;
			style_.flex_basis = basis;
			mark_layout_dirty();

			return *this;
		}

		element& positioning(const position_type p) noexcept
		{
			style_.position = p;
			mark_layout_dirty();

			return *this;
		}

		element& inset_top(const styled_size s) noexcept
		{
			style_.inset_top = s;
			mark_layout_dirty();

			return *this;
		}

		element& inset_right(const styled_size s) noexcept
		{
			style_.inset_right = s;
			mark_layout_dirty();

			return *this;
		}

		element& inset_bottom(const styled_size s) noexcept
		{
			style_.inset_bottom = s;
			mark_layout_dirty();

			return *this;
		}

		element& inset_left(const styled_size s) noexcept
		{
			style_.inset_left = s;
			mark_layout_dirty();

			return *this;
		}

		element& wrap(const wrap_mode w) noexcept
		{
			style_.wrap = w;
			mark_layout_dirty();

			return *this;
		}

		element& align_content(const align_content ac) noexcept
		{
			style_.align_content_v = ac;
			mark_layout_dirty();

			return *this;
		}

		element& overflow(const overflow_mode o) noexcept
		{
			style_.overflow = o;
			mark_layout_dirty();

			return *this;
		}

		element& show_scrollbar(const bool show) noexcept
		{
			style_.show_scrollbar = show;

			return *this;
		}

		element& dir(const text_direction d) noexcept
		{
			style_.dir = d;
			mark_layout_dirty();

			return *this;
		}

		element& background_color(const color c) noexcept
		{
			style_.background_color = c;

			return *this;
		}

		element& text_color(const color c) noexcept
		{
			style_.text_color = c;

			return *this;
		}

		element& rounding(const float r) noexcept
		{
			style_.rounding = r;

			return *this;
		}

		element& border_color(const color c) noexcept
		{
			style_.border_color = c;

			return *this;
		}

		element& shadow(const color col, const float blur = 15.f, const float spread = 0.f) noexcept
		{
			style_.shadow_color = col;
			style_.shadow_blur = blur;
			style_.shadow_spread = spread;

			return *this;
		}

		element& transition_speed(const float speed) noexcept
		{
			style_.transition_speed = speed;

			return *this;
		}

		element& text_size(const float size) noexcept
		{
			style_.font_size = size;
			mark_layout_dirty();

			return *this;
		}

		element& text_alignment(const text_align align) noexcept
		{
			style_.text_alignment = align;

			return *this;
		}

		element& tooltip(const string_view_t text)
		{
			tooltip_ = string_t(text);

			return *this;
		}

		[[nodiscard]] const string_t& tooltip() const noexcept
		{
			return tooltip_;
		}

		[[nodiscard]] bool has_tooltip() const noexcept
		{
			return !tooltip_.empty();
		}

		void set_flex_lines(vector_t<flex_line> lines) noexcept
		{
			flex_lines_ = cstd::move(lines);
		}

		[[nodiscard]] span_t<const flex_line> flex_lines() const noexcept
		{
			return flex_lines_;
		}

		[[nodiscard]] float compute_main_content_size(const element_style& defaults, const bool vertical) const noexcept;

		[[nodiscard]] position scroll_offset() const noexcept
		{
			return scroll_offset_;
		}

		void apply_scroll(const float delta, const element_style& defaults) noexcept;

		element& visible(const bool visible) noexcept
		{
			visible_ = visible;
			mark_layout_dirty();

			return *this;
		}

		// Topmost elements (e.g. popups) are deferred to a final overlay pass so they draw above,
		// and are hit-tested before, everything else — regardless of their position in the tree.
		element& topmost(const bool v = true) noexcept
		{
			topmost_ = v;

			return *this;
		}

		[[nodiscard]] bool is_topmost() const noexcept
		{
			return topmost_;
		}

		element& animate(keyframe_sequence seq, animation_options opts)
		{
			animations_.emplace_back(cstd::move(seq), cstd::move(opts));

			return *this;
		}

		element& stop_animations() noexcept
		{
			animations_.clear();

			return *this;
		}

		[[nodiscard]] optional_t<keyframe_props> animated_props() const noexcept;

		// defined in gui.hpp after gui_renderer is complete. `overlays`, when non-null, collects
		// topmost children to be drawn in a final pass instead of inline; null means render inline.
		void render(gui_renderer& renderer, const element_style& defaults,
		            const position offset = { 0.f, 0.f },
		            vector_t<deferred_render>* overlays = nullptr) const;

	protected:
		virtual void render_self(gui_renderer& renderer, position min, position max) const
		{

		}



		void mark_layout_dirty() noexcept
		{
			if (layout_dirty_ptr_)
			{
				*layout_dirty_ptr_ = true;
			}
		}

		vector_2d<float> computed_size_ = { };
		position computed_pos_ = { };
		position visual_pos_ = { };
		element_style style_;
		bool hovered_ = false;
		bool pressed_ = false;
		bool focused_ = false;
		bool visible_ = true;
		bool topmost_ = false;
		string_t tooltip_;
		vector_t<animation_state> animations_;

		color visual_bg_ = { 0.f, 0.f, 0.f, 0.f };
		color visual_text_color_ = { 1.f, 1.f, 1.f, 1.f };
		float visual_rounding_ = 0.f;
		color visual_border_color_ = { 0.f, 0.f, 0.f, 0.f };
		bool transitions_initialized_ = false;

		vector_t<flex_line> flex_lines_;
		position scroll_offset_ = { 0.f, 0.f };

		vector_t<shared_ptr_t<element>> children_ = { };
		bool* layout_dirty_ptr_ = nullptr;
		weak_ptr_t<gui> gui_;
	};

	// implemented in elements.cpp
	void compute_justify(justify_content jc, float remaining,
	                     cstd::size_t count, float base_gap,
	                     float& out_offset, float& out_gap) noexcept;

	void position_line_children(span_t<const shared_ptr_t<element>> children,
	                            bool effective_reversed,
	                            bool vertical,
	                            float available_main,
	                            float line_cross_size,
	                            justify_content jc,
	                            alignment al,
	                            float gap,
	                            position base_cursor,
	                            float cross_cursor_offset,
	                            const element_style& defaults);

	void resolve_positions(element& el, position cursor, const element_style& defaults);

	void update_all(element& el, float dt, position current_offset = { 0.f, 0.f });
}

#include "element_tree.hpp"
