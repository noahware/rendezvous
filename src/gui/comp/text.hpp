#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../text_metrics.hpp"
#include "../../util/string.hpp"

namespace rv
{
	class text_element final : public element
	{
	public:
		text_element() noexcept = default;

		explicit text_element(const element_size size, shared_ptr_t<gui_font> font) noexcept
			: element(size), font_(cstd::move(font)) { }

		text_element& content(const string_view_t text)
		{
			text_ = string_t(text);
			wrap_cache_valid_ = false;
			mark_layout_dirty();
			return *this;
		}

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override;

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override;

	private:
		[[nodiscard]] float single_line_width(const float scale) const noexcept;

		[[nodiscard]] float measure_line(const string_view_t line, const float scale) const noexcept;

		// first line truncated to fit max_width with a trailing "..." appended when it overflows.
		[[nodiscard]] string_t ellipsize(const float max_width, const float scale, const float letter_spacing) const noexcept;

		[[nodiscard]] vector_t<string_view_t> wrap_text(const float max_width, const float scale) const noexcept;

		[[nodiscard]] float resolved_scale() const noexcept
		{
			return style_.font_size.value_or(0.f) > 0.f ? style_.font_size.value_or(0.f) / font_->baked_size() : 1.f;
		}

		void update_wrap_cache(const float width, const float scale) const noexcept;

		shared_ptr_t<gui_font> font_;
		string_t text_;

		mutable vector_t<string_view_t> cached_wrapped_;
		mutable float cached_wrap_width_ = 0.f;
		mutable float cached_wrap_scale_ = 0.f;
		mutable bool wrap_cache_valid_ = false;
	};
}
