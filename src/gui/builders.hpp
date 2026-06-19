#pragma once
#include "element.hpp"
#include "gui.hpp"
#include "comp/button.hpp"
#include "comp/checkbox.hpp"
#include "comp/text.hpp"
#include "comp/text_box.hpp"
#include "comp/combo_box.hpp"
#include "comp/color_picker.hpp"
#include "comp/slider.hpp"
#include "comp/panel.hpp"
#include "comp/plot_lines.hpp"
#include "comp/value_inspector.hpp"
#include "comp/key_bind.hpp"
#include "comp/tabs.hpp"
#include "comp/image.hpp"
#include "comp/separator.hpp"
#include "comp/progress_bar.hpp"
#include "comp/radio_button.hpp"
#include "comp/collapsing_header.hpp"
#include "comp/multi_combo_box.hpp"
#include "comp/plot_histogram.hpp"

// Single convenience include that pulls in every widget and defines the add_* factory methods
// for both `element` and `gui`. Include this (instead of the individual comp headers) to build
// UIs with the concise factory API, e.g. gui->add_button("Save") or container.add_checkbox(...).

namespace rv
{
	inline constexpr float default_title_size = 18.f;

	// Text size and gap come from the gui's configurable default_style (set via
	// gui->default_style()), falling back to sensible values when the user left them unset.
	[[nodiscard]] inline float default_text_size(const gui& g) noexcept { return g.default_style().font_size.value_or(16.f); }
	[[nodiscard]] inline float default_gap(const gui& g) noexcept { return g.default_style().gap.value_or(8.f); }

	[[nodiscard]] inline element_size default_button_size() noexcept    { return { styled_size::auto_v(), styled_size::px(34.f) }; }
	[[nodiscard]] inline element_size default_label_size() noexcept     { return { styled_size::auto_v(), styled_size::auto_v() }; }
	[[nodiscard]] inline element_size default_input_size() noexcept     { return { styled_size::px(260.f), styled_size::auto_v() }; }
	[[nodiscard]] inline element_size default_area_size() noexcept      { return { styled_size::px(360.f), styled_size::px(120.f) }; }
	[[nodiscard]] inline element_size default_slider_size() noexcept    { return { styled_size::fill(), styled_size::px(28.f) }; }
	[[nodiscard]] inline element_size default_combo_size() noexcept     { return { styled_size::fill(), styled_size::px(32.f) }; }
	[[nodiscard]] inline element_size default_color_picker_size() noexcept { return { styled_size::px(64.f), styled_size::px(28.f) }; }
	[[nodiscard]] inline element_size default_panel_size() noexcept     { return { styled_size::px(320.f), styled_size::px(220.f) }; }
	[[nodiscard]] inline element_size default_plot_size() noexcept      { return { styled_size::fill(), styled_size::px(120.f) }; }
	[[nodiscard]] inline element_size default_inspector_size() noexcept { return { styled_size::fill(), styled_size::auto_v() }; }
	[[nodiscard]] inline element_size default_key_bind_size() noexcept  { return { styled_size::px(120.f), styled_size::px(32.f) }; }
	[[nodiscard]] inline element_size default_tabs_size() noexcept      { return { styled_size::fill(), styled_size::px(240.f) }; }
	[[nodiscard]] inline element_size default_image_size() noexcept     { return { styled_size::auto_v(), styled_size::auto_v() }; }
	[[nodiscard]] inline element_size default_row_size() noexcept       { return { styled_size::fill(), styled_size::auto_v() }; }
	[[nodiscard]] inline element_size default_column_size() noexcept    { return { styled_size::auto_v(), styled_size::auto_v() }; }
	[[nodiscard]] inline element_size default_container_size() noexcept { return { styled_size::fill(), styled_size::auto_v() }; }
	[[nodiscard]] inline element_size default_separator_size() noexcept { return { styled_size::fill(), styled_size::px(1.f) }; }
	[[nodiscard]] inline element_size default_progress_bar_size() noexcept { return { styled_size::fill(), styled_size::px(20.f) }; }
	[[nodiscard]] inline element_size default_radio_size() noexcept { return { styled_size::auto_v(), styled_size::auto_v() }; }
	[[nodiscard]] inline element_size default_collapsing_header_size() noexcept { return { styled_size::fill(), styled_size::auto_v() }; }
	[[nodiscard]] inline element_size default_histogram_size() noexcept { return { styled_size::fill(), styled_size::px(120.f) }; }
	[[nodiscard]] inline element_size default_multi_combo_size() noexcept { return { styled_size::fill(), styled_size::px(32.f) }; }

	[[nodiscard]] inline border_vector default_slider_padding() noexcept { return { 0.f, 10.f, 0.f, 10.f }; }
	[[nodiscard]] inline float default_slider_rounding() noexcept { return 14.f; }
	[[nodiscard]] inline color default_panel_shadow() noexcept { return { 0.f, 0.f, 0.f, 0.6f }; }

	// Group-box styling for add_container(): a subtle surface with padding and a border.
	inline void style_as_container(element& e, const gui& g)
	{
		e.direction(layout_direction::vertical).gap(default_gap(g)).padding(12.f)
			.background_color({ 0.13f, 0.13f, 0.16f, 1.f }).rounding(8.f)
			.border_color({ 0.28f, 0.28f, 0.34f, 1.f }).border_width(1.f);
	}

	// ---- factory definitions --------------------------------------------------------------------
	// One macro with the bodies, expanded once for `element` and once for `gui`. The only thing
	// that differs between the two is GUI (the owning gui) and PARENT (the element to attach to).
	#define RV_WIDGET_FACTORY_DEFS(CLS, GUI, PARENT)                                                  \
		inline button& CLS::add_button(const string_view_t text) {                                        \
			auto g = GUI; auto w = g->make_child<button>(PARENT, default_button_size(), g->font());     \
			w->text_size(default_text_size(*g)); if (!text.empty()) w->text(text); return *w; }        \
		inline checkbox& CLS::add_checkbox(const string_view_t label) {                                    \
			auto g = GUI; auto w = g->make_child<checkbox>(PARENT, default_label_size(), g->font());     \
			w->text_size(default_text_size(*g)); if (!label.empty()) w->label(label); return *w; }      \
		inline text_element& CLS::add_label(const string_view_t text) {                                    \
			auto g = GUI; auto w = g->make_child<text_element>(PARENT, default_label_size(), g->font()); \
			w->text_size(default_text_size(*g)); if (!text.empty()) w->content(text); return *w; }      \
		inline text_element& CLS::add_label(const string_view_t text, shared_ptr_t<gui_font> font) {       \
			auto g = GUI; auto w = g->make_child<text_element>(PARENT, default_label_size(), font ? font : g->font()); \
			w->text_size(default_text_size(*g)); if (!text.empty()) w->content(text); return *w; }      \
		inline text_box& CLS::add_text_input(const string_view_t text) {                                   \
			auto g = GUI; auto w = g->make_child<text_box>(PARENT, default_input_size(), g->font(), g->get_input()); \
			w->text_size(default_text_size(*g)); if (!text.empty()) w->text(text); return *w; }        \
		inline text_box& CLS::add_text_area(const string_view_t text) {                                    \
			auto g = GUI; auto w = g->make_child<text_box>(PARENT, default_area_size(), g->font(), g->get_input()); \
			w->multiline(true).text_size(default_text_size(*g)); if (!text.empty()) w->text(text); return *w; } \
		inline slider<float>& CLS::add_slider(const float mn, const float mx, const float v) {         \
			auto g = GUI; auto w = g->make_child<slider<float>>(PARENT, default_slider_size(), g->get_input()); \
			w->range(mn, mx).value(v);                                                                 \
			w->padding(default_slider_padding()).rounding(default_slider_rounding()); return *w; }      \
		inline range_slider<float>& CLS::add_range_slider(const float mn, const float mx, const float lo, const float hi) { \
			auto g = GUI; auto w = g->make_child<range_slider<float>>(PARENT, default_slider_size(), g->get_input()); \
			w->range(mn, mx); w->values(lo, hi);                                                       \
			w->padding(default_slider_padding()).rounding(default_slider_rounding()); return *w; }      \
		inline combo_box& CLS::add_combo_box(vector_t<string_t> options) {                             \
			auto g = GUI; auto w = g->make_child<combo_box>(PARENT, default_combo_size(), g->font(), g->get_input()); \
			w->text_size(default_text_size(*g)); if (!options.empty()) w->options(cstd::move(options)); return *w; } \
		inline color_picker& CLS::add_color_picker(const color initial) {                              \
			auto g = GUI; auto w = g->make_child<color_picker>(PARENT, default_color_picker_size(), g->font(), g->get_input()); \
			w->value(initial); return *w; }                                                            \
		inline panel& CLS::add_panel() {                                                               \
			auto g = GUI; auto w = g->make_child<panel>(PARENT, default_panel_size(), g->get_input());   \
			w->draggable(true).resizable(true); w->shadow(default_panel_shadow(), 10.f); return *w; }   \
		inline plot_lines& CLS::add_plot_lines() {                                                     \
			auto g = GUI; auto w = g->make_child<plot_lines>(PARENT, default_plot_size(), g->font(), g->get_input()); \
			w->autoscale(); return *w; }                                                               \
		inline plot_lines& CLS::add_plot_var(const string_view_t label, const float* value) {              \
			auto g = GUI; auto w = g->make_child<plot_lines>(PARENT, default_plot_size(), g->font(), g->get_input()); \
			w->capacity(300).bind(value).overlay(label); return *w; }                                  \
		inline value_inspector& CLS::add_inspector() {                                                 \
			auto g = GUI; auto w = g->make_child<value_inspector>(PARENT, default_inspector_size(), g->font()); \
			w->text_size(default_text_size(*g)); return *w; }                                          \
		inline element& CLS::add_row() {                                                               \
			auto g = GUI; auto w = g->make_child<element>(PARENT, default_row_size());                  \
			w->direction(layout_direction::horizontal).gap(default_gap(*g)); return *w; }               \
		inline element& CLS::add_column() {                                                            \
			auto g = GUI; auto w = g->make_child<element>(PARENT, default_column_size());               \
			w->direction(layout_direction::vertical).gap(default_gap(*g)); return *w; }                 \
		inline element& CLS::add_container(const string_view_t title) {                                    \
			auto g = GUI; auto w = g->make_child<element>(PARENT, default_container_size());            \
			style_as_container(*w, *g);                                                                 \
			if (!title.empty()) { auto t = g->make_child<text_element>(w, default_label_size(), g->font()); t->content(title).text_size(default_title_size); } \
			return *w; }                                                                               \
		inline key_bind& CLS::add_key_bind(const key initial_key) {                                    \
			auto g = GUI; auto w = g->make_child<key_bind>(PARENT, default_key_bind_size(), g->font(), g->get_input()); \
			w->text_size(default_text_size(*g)); if (initial_key != key{}) w->value(initial_key); return *w; } \
		inline tabs& CLS::add_tabs() {                                                                 \
			auto g = GUI; auto w = g->make_child<tabs>(PARENT, default_tabs_size(), g->font(), g->get_input()); \
			w->tab_text_size(default_text_size(*g)); return *w; }                                      \
		inline image& CLS::add_image(const shared_ptr_t<gui_texture> tex) {                            \
			auto g = GUI; auto w = g->make_child<image>(PARENT, default_image_size());                 \
			if (tex) w->texture(tex); return *w; }                                                     \
		inline separator& CLS::add_separator() {                                                       \
			auto g = GUI; auto w = g->make_child<separator>(PARENT, default_separator_size());          \
			return *w; }                                                                               \
		inline progress_bar& CLS::add_progress_bar(const float value) {                                \
			auto g = GUI; auto w = g->make_child<progress_bar>(PARENT, default_progress_bar_size(), g->font()); \
			w->value(value); return *w; }                                                              \
		inline radio_button& CLS::add_radio_button(const string_view_t label, const cstd::int32_t value) { \
			auto g = GUI; auto w = g->make_child<radio_button>(PARENT, default_radio_size(), g->font()); \
			w->text_size(default_text_size(*g)); w->value(value);                                      \
			if (!label.empty()) w->label(label); return *w; }                                          \
		inline collapsing_header& CLS::add_collapsing_header(const string_view_t label) {                  \
			auto g = GUI; auto w = g->make_child<collapsing_header>(PARENT, default_collapsing_header_size(), g->font(), g->get_input()); \
			w->text_size(default_text_size(*g));                                                       \
			if (!label.empty()) w->label(label); return *w; }                                          \
		inline plot_histogram& CLS::add_plot_histogram() {                                             \
			auto g = GUI; auto w = g->make_child<plot_histogram>(PARENT, default_histogram_size(), g->font(), g->get_input()); \
			w->autoscale(); return *w; }                                                              \
		inline multi_combo_box& CLS::add_multi_combo_box(vector_t<string_t> options) {                \
			auto g = GUI; auto w = g->make_child<multi_combo_box>(PARENT, default_multi_combo_size(), g->font(), g->get_input()); \
			w->text_size(default_text_size(*g)); if (!options.empty()) w->options(cstd::move(options)); return *w; }

	// element: lock the weak gui back-ref (kept alive for the call); gui: just itself.
	RV_WIDGET_FACTORY_DEFS(element, gui_.lock(), shared_from_this())
	RV_WIDGET_FACTORY_DEFS(gui, this, tree_.root())

	#undef RV_WIDGET_FACTORY_DEFS
}
