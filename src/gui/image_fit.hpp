#pragma once
#include "../render/position.hpp"
#include "../util/types.hpp"

// CSS object-fit semantics shared by the image widget (comp/image.hpp) and the element
// background_image style field. Pure geometry, no texture/renderer dependency.
namespace rv
{
	enum class image_fit : cstd::uint8_t
	{
		fill,    // stretch to fill the box exactly (may distort): CSS object-fit default for <img>
		contain, // scale to fit fully inside the box, preserving aspect (letterbox)
		cover    // scale to cover the box, preserving aspect (center-crop)
	};

	struct image_fit_result
	{
		position draw_min;
		position draw_max;
		position uv_min;
		position uv_max;
	};

	// Resolve the draw rect + UV rect for a texture of native size (tw, th) placed in box [min, max].
	// fill/contain show the whole texture (uv 0..1); contain shrinks the draw rect (letterbox).
	// cover keeps the full box and crops via the UVs. Degenerate inputs fall back to fill.
	[[nodiscard]] inline image_fit_result compute_image_fit(const position min, const position max,
	                                                        const float tw, const float th, const image_fit fit) noexcept
	{
		const position uv0 = { 0.f, 0.f };
		const position uv1 = { 1.f, 1.f };

		const float box_w = max.x - min.x;
		const float box_h = max.y - min.y;

		if (fit == image_fit::fill || tw <= 0.f || th <= 0.f || box_w <= 0.f || box_h <= 0.f)
		{
			return { min, max, uv0, uv1 };
		}

		const float box_aspect = box_w / box_h;
		const float tex_aspect = tw / th;

		if (fit == image_fit::contain)
		{
			// shrink the draw rect to the texture's aspect ratio, centered in the box.
			float w = box_w;
			float h = box_h;

			if (tex_aspect > box_aspect)
			{
				h = box_w / tex_aspect; // wider than the box: bound by width
			}
			else
			{
				w = box_h * tex_aspect; // taller than the box: bound by height
			}

			const float cx = (min.x + max.x) * 0.5f;
			const float cy = (min.y + max.y) * 0.5f;

			const position draw_min = { cx - w * 0.5f, cy - h * 0.5f };
			const position draw_max = { cx + w * 0.5f, cy + h * 0.5f };

			return { draw_min, draw_max, uv0, uv1 };
		}

		// cover: fill the whole box and crop the overflowing axis via the UVs.
		float u = 1.f;
		float v = 1.f;

		if (tex_aspect > box_aspect)
		{
			u = box_aspect / tex_aspect; // crop left/right
		}
		else
		{
			v = tex_aspect / box_aspect; // crop top/bottom
		}

		const float u_off = (1.f - u) * 0.5f;
		const float v_off = (1.f - v) * 0.5f;

		const position crop_min = { u_off, v_off };
		const position crop_max = { u_off + u, v_off + v };

		return { min, max, crop_min, crop_max };
	}
}
