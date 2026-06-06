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
