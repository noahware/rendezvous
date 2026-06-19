#pragma once
#include "../render/position.hpp"
#include "../util/hash.hpp"
#include "../util/string.hpp"
#include "../util/types.hpp"
#include "styled_size.hpp"
#include "animation.hpp"
#include "element_types.hpp"
#include "transition.hpp"

namespace rv
{
	class gui_renderer;

	enum class key : cstd::int32_t;
	class gui;
	class gui_font;
	class gui_texture;
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
	class tabs;
	class image;
	class separator;
	class progress_bar;
	class radio_button;
	class collapsing_header;
	class plot_histogram;
	class multi_combo_box;
	template <class T> class slider;
	template <class T> class range_slider;

	#define RV_WIDGET_FACTORY_DECLS                                                                 \
		button& add_button(string_view_t text = {});                                                  \
		checkbox& add_checkbox(string_view_t label = {});                                             \
		text_element& add_label(string_view_t text = {});                                             \
		text_element& add_label(string_view_t text, shared_ptr_t<gui_font> font);                      \
		text_box& add_text_input(string_view_t text = {});                                            \
		text_box& add_text_area(string_view_t text = {});                                             \
		slider<float>& add_slider(float mn = 0.f, float mx = 1.f, float v = 0.f);                 \
		range_slider<float>& add_range_slider(float mn = 0.f, float mx = 1.f, float lo = 0.f, float hi = 1.f); \
		combo_box& add_combo_box(vector_t<string_t> options = {});                                \
		color_picker& add_color_picker(color initial = {});                                       \
		panel& add_panel();                                                                       \
		plot_lines& add_plot_lines();                                                             \
		plot_lines& add_plot_var(string_view_t label, const float* value);                            \
		value_inspector& add_inspector();                                                         \
		element& add_row();                                                                       \
		element& add_column();                                                                    \
		element& add_container(string_view_t title = {});                                            \
		key_bind& add_key_bind(key initial_key = {});                                             \
		tabs& add_tabs();                                                                         \
		image& add_image(shared_ptr_t<gui_texture> tex = {});                                     \
		separator& add_separator();                                                               \
		progress_bar& add_progress_bar(float value = 0.f);                                        \
		radio_button& add_radio_button(string_view_t label = {}, cstd::int32_t value = 0);            \
		collapsing_header& add_collapsing_header(string_view_t label = {});                           \
		plot_histogram& add_plot_histogram();                                             \
	multi_combo_box& add_multi_combo_box(vector_t<string_t> options = {});

	template <class T, class ...Args>
	[[nodiscard]] shared_ptr_t<T> make_element(Args&&... args)
	{
		return cstd::make_shared<T>(cstd::forward<Args>(args)...);
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
			auto child = make_element<T>(cstd::forward<Args>(args)...);

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
			if (hovered != hovered_) { settled_ = false; }
			hovered_ = hovered;
		}

		[[nodiscard]] bool is_pressed() const noexcept
		{
			return pressed_;
		}

		void set_pressed(const bool pressed) noexcept
		{
			if (pressed != pressed_) { settled_ = false; }
			pressed_ = pressed;
		}

		[[nodiscard]] bool is_focused() const noexcept
		{
			return focused_;
		}

		void set_focused(const bool focused) noexcept
		{
			if (focused != focused_) { settled_ = false; }
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

		element& smooth_scroll(const bool smooth) noexcept
		{
			style_.smooth_scroll = smooth;

			return *this;
		}

		element& dir(const text_direction d) noexcept
		{
			style_.dir = d;
			mark_layout_dirty();

			return *this;
		}

		// state == normal writes the base style; any other state writes that state's override block,
		// resolved later by resolve_visual(). existing single-arg callers keep working via the default.
		element& background_color(const color c, const element_state state = element_state::normal) noexcept
		{
			if (state == element_state::normal)
			{
				style_.background_color = c;
			}
			else
			{
				ensure_state(state).background_color = c;
			}

			mark_unsettled();
			return *this;
		}

		element& background_gradient(const color start, const color end,
		                             const gradient_direction dir = gradient_direction::to_bottom) noexcept
		{
			style_.background_gradient = gradient{ start, end, dir };

			return *this;
		}

		element& background_gradient(const gradient& g) noexcept
		{
			style_.background_gradient = g;

			return *this;
		}

		element& clear_background_gradient() noexcept
		{
			style_.background_gradient = {};

			return *this;
		}

		// CSS-like background image, painted over background_color and behind this element's content.
		// Defaults to object-fit: cover. The texture is shared-owned, so it outlives the element.
		element& background_image(shared_ptr_t<gui_texture> tex, const image_fit fit = image_fit::cover,
		                          const color tint = { 1.f, 1.f, 1.f, 1.f }) noexcept
		{
			style_.background_image = cstd::move(tex);
			style_.background_image_fit = fit;
			style_.background_image_tint = tint;

			return *this;
		}

		element& text_color(const color c, const element_state state = element_state::normal) noexcept
		{
			if (state == element_state::normal)
			{
				style_.text_color = c;
			}
			else
			{
				ensure_state(state).text_color = c;
			}

			mark_unsettled();
			return *this;
		}

		element& rounding(const float r) noexcept
		{
			style_.rounding = r;
			style_.radii.reset(); // uniform shorthand wins; drop any per-corner override
			mark_unsettled();

			return *this;
		}

		element& border_color(const color c, const element_state state = element_state::normal) noexcept
		{
			if (state == element_state::normal)
			{
				style_.border_color = c;
			}
			else
			{
				ensure_state(state).border_color = c;
			}

			mark_unsettled();
			return *this;
		}

		element& shadow(const color col, const float blur = 15.f, const float spread = 0.f) noexcept
		{
			style_.shadow_color = col;
			style_.shadow_blur = blur;
			style_.shadow_spread = spread;

			return *this;
		}

		element& shadow_gradient(const color start, const color end,
		                         const gradient_direction dir = gradient_direction::to_bottom,
		                         const float blur = 15.f, const float spread = 0.f) noexcept
		{
			style_.shadow_gradient = gradient{ start, end, dir };
			style_.shadow_blur = blur;
			style_.shadow_spread = spread;

			return *this;
		}

		element& clear_shadow_gradient() noexcept
		{
			style_.shadow_gradient = {};

			return *this;
		}

		element& backdrop_blur(const float sigma) noexcept
		{
			style_.backdrop_blur = sigma;
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

		// element-level opacity in [0,1]; multiplied through the whole subtree at render time.
		element& opacity(const float o, const element_state state = element_state::normal) noexcept
		{
			if (state == element_state::normal)
			{
				style_.opacity = o;
			}
			else
			{
				ensure_state(state).opacity = o;
			}

			mark_unsettled();
			return *this;
		}

		// per-corner radii overload; the float rounding() above is the uniform shorthand.
		element& rounding(const corner_radii radii) noexcept
		{
			style_.radii = radii;
			style_.rounding.reset(); // per-corner wins; drop any uniform override
			mark_unsettled();

			return *this;
		}

		// non-interactive + visually dimmed; a disabled subtree receives no hover/press/focus/click.
		element& disabled(const bool v = true) noexcept
		{
			disabled_ = v;
			mark_unsettled();

			return *this;
		}

		[[nodiscard]] bool is_disabled() const noexcept
		{
			return disabled_;
		}

		// deferred-overlay draw order: any non-zero z lifts this element into the final overlay pass,
		// where overlays render and hit-test sorted by z (higher on top). z == 0 keeps it inline.
		// see the z_index_panel / z_index_popup layer constants.
		element& z_index(const cstd::int32_t z) noexcept
		{
			z_index_ = z;

			return *this;
		}

		[[nodiscard]] cstd::int32_t z_index() const noexcept
		{
			return z_index_;
		}

		element& scale(const float s) noexcept
		{
			style_.scale = s;

			return *this;
		}

		element& translate(const position t) noexcept
		{
			style_.translate = t;

			return *this;
		}

		element& cursor(const cursor_type c) noexcept
		{
			style_.cursor = c;

			return *this;
		}

		element& line_height(const float mult) noexcept
		{
			style_.line_height = mult;
			mark_layout_dirty();

			return *this;
		}

		element& letter_spacing(const float px) noexcept
		{
			style_.letter_spacing = px;
			mark_layout_dirty();

			return *this;
		}

		element& decoration(const text_decoration d) noexcept
		{
			style_.decoration = d;

			return *this;
		}

		element& text_ellipsis(const bool on = true) noexcept
		{
			style_.text_ellipsis = on;

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

		// Show only the child at `index`; every other child gets display:none (visible(false),
		// removed from layout and not drawn). This is the web idiom for switching views: toggle
		// `display` on sibling <div>s. It's the native "one child at a time", pair it with
		// buttons whose on_click calls show_only(i), like onclick handlers / React setState.
		element& show_only(const cstd::int32_t index) noexcept
		{
			for (cstd::int32_t i = 0; i < static_cast<cstd::int32_t>(children_.size()); ++i)
			{
				children_[i]->visible(i == index);
			}

			return *this;
		}

		// shorthand: lift this element onto the popup overlay layer (above panels), or back inline.
		// "topmost" now simply means a non-zero z-index; see z_index() and the layer constants.
		element& topmost(const bool v = true) noexcept
		{
			z_index_ = v ? z_index_popup : 0;

			return *this;
		}

		// an element is deferred to the overlay pass whenever it carries a non-zero z-index.
		[[nodiscard]] bool is_topmost() const noexcept
		{
			return z_index_ != 0;
		}

		element& animate(keyframe_sequence seq, animation_options opts)
		{
			animations_.emplace_back(cstd::move(seq), cstd::move(opts));
			mark_unsettled();

			return *this;
		}

		element& stop_animations() noexcept
		{
			animations_.clear();

			return *this;
		}

		[[nodiscard]] optional_t<keyframe_props> animated_props() const noexcept;

		// the per-frame merged animation props, computed once in update() and reused by render() and
		// update_all(), so the keyframe interpolation isn't repeated 2-3x per animated element.
		[[nodiscard]] const optional_t<keyframe_props>& animated_props_cached() const noexcept
		{
			return cached_anim_props_;
		}

		// resolve the effective background/text/border colors + opacity for the current interaction
		// state: start from the base style, then overlay the focus, hover, active (pressed) and
		// disabled override blocks in increasing priority. a disabled element with no explicit
		// disabled opacity is dimmed by default. this is the single source of per-state styling that
		// replaces the per-widget hover/press color swaps.
		[[nodiscard]] resolved_visual resolve_visual() const noexcept
		{
			resolved_visual r;
			r.bg      = style_.background_color.value_or(r.bg);
			r.text    = style_.text_color.value_or(r.text);
			r.border  = style_.border_color.value_or(r.border);
			r.opacity = style_.opacity.value_or(r.opacity);

			const auto apply = [&r](const optional_t<state_style>& s) noexcept
			{
				if (!s)
				{
					return;
				}

				if (s->background_color) { r.bg = *s->background_color; }
				if (s->text_color)       { r.text = *s->text_color; }
				if (s->border_color)     { r.border = *s->border_color; }
				if (s->opacity)          { r.opacity = *s->opacity; }
			};

			if (focused_) { apply(style_.focus); }
			if (hovered_) { apply(style_.hover); }
			if (pressed_) { apply(style_.active); }

			if (disabled_)
			{
				apply(style_.disabled_style);

				if (!style_.disabled_style || !style_.disabled_style->opacity)
				{
					r.opacity *= 0.45f;
				}
			}

			return r;
		}

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

		// clears the "settled" flag so element::update() re-resolves + re-eases next frame. called by
		// every setter that changes a resolve_visual input, and by subclasses that mutate style_
		// directly at runtime (e.g. key_bind) so the idle-frame skip can't freeze a stale color.
		void mark_unsettled() noexcept
		{
			settled_ = false;
		}

		// lazily materialize the per-state override block for `state` (never called with normal).
		state_style& ensure_state(const element_state state) noexcept
		{
			optional_t<state_style>* slot = &style_.hover;

			switch (state)
			{
			case element_state::pressed:  slot = &style_.active; break;
			case element_state::focused:  slot = &style_.focus; break;
			case element_state::disabled: slot = &style_.disabled_style; break;
			default: break; // hovered uses the default slot; normal never reaches here
			}

			if (!*slot)
			{
				*slot = state_style{};
			}

			return **slot;
		}

		vector_2d<float> computed_size_ = { };
		position computed_pos_ = { };
		position visual_pos_ = { };
		element_style style_;
		bool hovered_ = false;
		bool pressed_ = false;
		bool focused_ = false;
		bool visible_ = true;
		bool disabled_ = false;
		cstd::int32_t z_index_ = 0;
		string_t tooltip_;
		vector_t<animation_state> animations_;
		optional_t<keyframe_props> cached_anim_props_;

		color visual_bg_ = { 0.f, 0.f, 0.f, 0.f };
		color visual_text_color_ = { 1.f, 1.f, 1.f, 1.f };
		corner_radii visual_radii_ = { };
		color visual_border_color_ = { 0.f, 0.f, 0.f, 0.f };
		float visual_opacity_ = 1.f;
		bool transitions_initialized_ = false;
		// true once the visual_* channels have eased onto their resolved target with nothing
		// animating; while true, element::update() skips the resolve + lerp work. Every setter that
		// feeds resolve_visual (state + style) and animate() clear it via mark_unsettled().
		bool settled_ = false;

		vector_t<flex_line> flex_lines_;
		position scroll_offset_ = { 0.f, 0.f };
		position target_scroll_offset_ = { 0.f, 0.f };

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
