#pragma once
#include "../render/position.hpp"
#include "../util/types.hpp"
#include "styled_size.hpp"
#include "image_fit.hpp"

// Layout enums, style description, and the small layout helper functions shared by
// element and the layout engine. Extracted from element.hpp for cohesion.
namespace rv
{
	class gui_texture;

	// forward-declared; the real cursor_type lives in input.hpp (the gui -> input dependency is
	// allowed). an opaque scoped-enum decl with an explicit underlying type is a complete type,
	// which is all optional_t<cursor_type> needs for storage.
	enum class cursor_type : cstd::uint8_t;

	// conventional z-index layers for the deferred overlay pass (higher draws / hit-tests on top).
	// any non-zero z-index lifts an element out of the inline tree into the overlay pass.
	constexpr cstd::int32_t z_index_panel = 100;   // floating / draggable panels
	constexpr cstd::int32_t z_index_popup = 1000;  // dropdowns, pickers, menus; always above panels

	enum class layout_direction : cstd::uint8_t
	{
		vertical,
		horizontal,
		vertical_reverse,
		horizontal_reverse
	};

	[[nodiscard]] inline bool is_vertical(const layout_direction d) noexcept
	{
		return d == layout_direction::vertical || d == layout_direction::vertical_reverse;
	}

	[[nodiscard]] inline bool is_reversed(const layout_direction d) noexcept
	{
		return d == layout_direction::vertical_reverse || d == layout_direction::horizontal_reverse;
	}

	enum class alignment : cstd::uint8_t
	{
		start,
		center,
		end,
		stretch
	};

	enum class justify_content : cstd::uint8_t
	{
		start,
		center,
		end,
		space_between,
		space_around,
		space_evenly
	};

	enum class position_type : cstd::uint8_t
	{
		relative,
		absolute,
		static_v
	};

	enum class wrap_mode : cstd::uint8_t
	{
		no_wrap,
		wrap,
		wrap_reverse
	};

	enum class align_content : cstd::uint8_t
	{
		flex_start,
		flex_end,
		center,
		stretch,
		space_between,
		space_around,
		space_evenly
	};

	enum class overflow_mode : cstd::uint8_t
	{
		visible,
		hidden,
		scroll
	};

	enum class text_align : cstd::uint8_t
	{
		left,
		center,
		right
	};

	enum class text_direction : cstd::uint8_t
	{
		ltr,
		rtl
	};

	enum class gradient_direction : cstd::uint8_t
	{
		to_right,
		to_left,
		to_bottom,
		to_top,
		to_bottom_right,
		to_bottom_left,
		to_top_right,
		to_top_left
	};

	struct gradient
	{
		color start;
		color end;
		gradient_direction direction = gradient_direction::to_bottom;
	};

	inline void gradient_to_corners(const gradient& g, color& tl, color& tr, color& br, color& bl) noexcept
	{
		const color mid =
		{
			(g.start.r + g.end.r) * 0.5f,
			(g.start.g + g.end.g) * 0.5f,
			(g.start.b + g.end.b) * 0.5f,
			(g.start.a + g.end.a) * 0.5f
		};

		switch (g.direction)
		{
		case gradient_direction::to_right:
			tl = g.start; tr = g.end; br = g.end; bl = g.start;
			break;
		case gradient_direction::to_left:
			tl = g.end; tr = g.start; br = g.start; bl = g.end;
			break;
		case gradient_direction::to_bottom:
			tl = g.start; tr = g.start; br = g.end; bl = g.end;
			break;
		case gradient_direction::to_top:
			tl = g.end; tr = g.end; br = g.start; bl = g.start;
			break;
		case gradient_direction::to_bottom_right:
			tl = g.start; tr = mid; br = g.end; bl = mid;
			break;
		case gradient_direction::to_bottom_left:
			tl = mid; tr = g.start; br = mid; bl = g.end;
			break;
		case gradient_direction::to_top_right:
			tl = mid; tr = g.end; br = mid; bl = g.start;
			break;
		case gradient_direction::to_top_left:
			tl = g.end; tr = mid; br = g.start; bl = mid;
			break;
		}
	}

	// interaction state used to select per-state style overrides. `normal` is the base style.
	enum class element_state : cstd::uint8_t
	{
		normal,
		hovered,
		pressed,
		focused,
		disabled
	};

	enum class text_decoration : cstd::uint8_t
	{
		none,
		underline,
		line_through
	};

	// the subset of element_style that can vary by interaction state. only the fields that are set
	// override the base style while that state is active (CSS-cascade semantics).
	struct state_style
	{
		optional_t<color> background_color;
		optional_t<color> text_color;
		optional_t<color> border_color;
		optional_t<float> opacity;
	};

	// the concrete colors + opacity an element resolves to for its current interaction state, after
	// overlaying the active state_style blocks on top of the base style (see element::resolve_visual).
	struct resolved_visual
	{
		color bg = { 0.f, 0.f, 0.f, 0.f };
		color text = { 1.f, 1.f, 1.f, 1.f };
		color border = { 0.f, 0.f, 0.f, 0.f };
		float opacity = 1.f;
	};

	struct border_vector
	{
		float top = 0.f;
		float right = 0.f;
		float bottom = 0.f;
		float left = 0.f;
	};

	struct flex_line
	{
		cstd::size_t start_index = 0;
		cstd::size_t count = 0;
		float main_size = 0.f;
		float cross_size = 0.f;
	};

	// A topmost element deferred out of the normal render pass, captured with the accumulated
	// offset it had at that point so it can be redrawn last (on top of everything).
	struct deferred_render
	{
		const class element* el = nullptr;
		position offset;
	};

	struct element_style
	{
		element_size size;
		optional_t<float> gap;
		optional_t<float> row_gap;
		optional_t<float> column_gap;
		optional_t<layout_direction> direction;
		optional_t<alignment> align;
		optional_t<alignment> align_self;
		optional_t<justify_content> justify;
		optional_t<border_vector> margin;
		optional_t<border_vector> padding;
		optional_t<border_vector> border_width;
		optional_t<styled_size> min_width;
		optional_t<styled_size> max_width;
		optional_t<styled_size> min_height;
		optional_t<styled_size> max_height;
		optional_t<float> aspect_ratio;
		optional_t<float> flex_grow;
		optional_t<float> flex_shrink;
		optional_t<styled_size> flex_basis;
		optional_t<position_type> position;
		optional_t<styled_size> inset_top;
		optional_t<styled_size> inset_right;
		optional_t<styled_size> inset_bottom;
		optional_t<styled_size> inset_left;
		optional_t<wrap_mode> wrap;
		optional_t<align_content> align_content_v;
		optional_t<overflow_mode> overflow;
		optional_t<bool> show_scrollbar;
		optional_t<bool> smooth_scroll;
		optional_t<text_direction> dir;
		optional_t<color> background_color;
		optional_t<color> text_color;
		optional_t<float> rounding;
		optional_t<color> border_color;
		optional_t<float> font_size;
		optional_t<text_align> text_alignment;
		optional_t<float> transition_speed;
		optional_t<color> shadow_color;
		optional_t<float> shadow_blur;
		optional_t<float> shadow_spread;
		optional_t<shared_ptr_t<gui_texture>> background_image;
		optional_t<color> background_image_tint;
		optional_t<image_fit> background_image_fit;

		// per-interaction-state overrides, resolved by element::resolve_visual().
		optional_t<state_style> hover;
		optional_t<state_style> active;
		optional_t<state_style> focus;
		optional_t<state_style> disabled_style;

		// element-level opacity, multiplied through the whole subtree at render time.
		optional_t<float> opacity;

		// per-corner radii; when unset, the uniform `rounding` value applies to all corners.
		optional_t<corner_radii> radii;

		// CPU-side transforms applied to this element's emitted vertex range at render time.
		// note: `rv::position` must be qualified here because the `position` member above (the CSS
		// position-type field) shadows the rv::position type name inside this struct's scope.
		optional_t<float> scale;
		optional_t<rv::position> translate;

		// requested mouse cursor while this element is hovered.
		optional_t<cursor_type> cursor;

		// text styling.
		optional_t<float> line_height;     // line-advance multiplier (1.0 == font default)
		optional_t<float> letter_spacing;  // extra px inserted between glyphs
		optional_t<text_decoration> decoration;
		optional_t<bool> text_ellipsis;    // truncate overflowing single-line text with an ellipsis
		optional_t<float> backdrop_blur;
		optional_t<gradient> background_gradient;
		optional_t<gradient> shadow_gradient;
	};

	// helper to resolve gap for the correct axis
	[[nodiscard]] inline float resolve_gap(const element_style& style, const element_style& defaults,
	                                        const bool vertical) noexcept
	{
		if (vertical)
		{
			return style.row_gap.value_or(
				defaults.row_gap.value_or(
					style.gap.value_or(defaults.gap.value_or(0.f))
				)
			);
		}

		return style.column_gap.value_or(
			defaults.column_gap.value_or(
				style.gap.value_or(defaults.gap.value_or(0.f))
			)
		);
	}

	// helper to compute total insets (padding + border_width)
	[[nodiscard]] inline border_vector compute_insets(const element_style& style,
	                                                  const element_style& defaults) noexcept
	{
		const auto pad = style.padding.value_or(defaults.padding.value_or(border_vector{}));
		const auto brd = style.border_width.value_or(defaults.border_width.value_or(border_vector{}));

		return {
			pad.top + brd.top,
			pad.right + brd.right,
			pad.bottom + brd.bottom,
			pad.left + brd.left
		};
	}
}
