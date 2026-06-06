#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../image_fit.hpp"

namespace rv
{
	// A bitmap image element. Build a gui_texture via gui->load_texture("path.png") (or wrap a
	// renderer texture in a gui_texture_impl) and hand it in. Draws with CSS object-fit semantics;
	// with both size axes left auto, the element sizes itself to the texture's native pixel dimensions.
	class image final : public element
	{
	public:
		image() noexcept = default;

		explicit image(const element_size size) noexcept
				:	element(size) { }

		image(const element_size size, shared_ptr_t<gui_texture> tex) noexcept
				:	element(size), texture_(cstd::move(tex)) { }

		image& texture(shared_ptr_t<gui_texture> tex) noexcept
		{
			texture_ = cstd::move(tex);
			mark_layout_dirty(); // native content size may have changed

			return *this;
		}

		image& tint(const color c) noexcept
		{
			tint_ = c;

			return *this;
		}

		image& fit(const image_fit f) noexcept
		{
			fit_ = f;

			return *this;
		}

		// Native pixel size, so an auto-sized image element shrink-wraps the texture. Returns zero
		// when there's no texture or its dimensions are unknown (e.g. adopted from an external SRV).
		[[nodiscard]] vector_2d<float> content_size([[maybe_unused]] const vector_2d<float> available) const noexcept override
		{
			if (!texture_ || texture_->width() == 0 || texture_->height() == 0)
			{
				return { 0.f, 0.f };
			}

			return { static_cast<float>(texture_->width()), static_cast<float>(texture_->height()) };
		}

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			if (!texture_)
			{
				return;
			}

			const float tw = static_cast<float>(texture_->width());
			const float th = static_cast<float>(texture_->height());

			const image_fit_result f = compute_image_fit(min, max, tw, th, fit_);

			renderer.draw_image_rounded(texture_, f.draw_min, f.draw_max, style_.rounding.value_or(0.f),
			                            rounding_flags_all, f.uv_min, f.uv_max, tint_);
		}

	private:
		shared_ptr_t<gui_texture> texture_;
		color tint_ = { 1.f, 1.f, 1.f, 1.f };
		image_fit fit_ = image_fit::fill;
	};
}
