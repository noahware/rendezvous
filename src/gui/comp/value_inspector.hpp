#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../text_metrics.hpp"
#include "../../util/string.hpp"

namespace rv
{
	class value_inspector final : public element
	{
	public:
		value_inspector(const element_size size, shared_ptr_t<gui_font> font) noexcept
				:	element(size), font_(cstd::move(font))
		{
			init_defaults();
		}

		template <class T>
		value_inspector& watch(const string_view_t name, const T* ptr)
		{
			entries_.push_back({ string_t(name), [ptr]() -> string_t { return format_value(*ptr); } });
			mark_layout_dirty();

			return *this;
		}

		value_inspector& watch(const string_view_t name, function_t<string_t()> getter)
		{
			entries_.push_back({ string_t(name), cstd::move(getter) });
			mark_layout_dirty();

			return *this;
		}

		value_inspector& row_height(const float h) noexcept
		{
			row_height_ = h;
			mark_layout_dirty();

			return *this;
		}

		value_inspector& text_size(const float s) noexcept
		{
			text_size_ = s;
			mark_layout_dirty();

			return *this;
		}

		value_inspector& label_color(const color c) noexcept
		{
			label_color_ = c;

			return *this;
		}

		value_inspector& value_color(const color c) noexcept
		{
			value_color_ = c;

			return *this;
		}

		value_inspector& zebra(const bool on) noexcept
		{
			zebra_ = on;

			return *this;
		}

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float>) const noexcept override
		{
			if (!font_ || entries_.empty())
			{
				return { 0.f, 0.f };
			}

			return { 0.f, static_cast<float>(entries_.size()) * resolved_row_height() };
		}

	protected:
		void init_defaults() noexcept
		{
			style_.background_color = color{ 0.1f, 0.1f, 0.13f, 1.f };
			style_.border_color = color{ 0.28f, 0.28f, 0.34f, 1.f };
			style_.border_width = border_vector{ 1.f, 1.f, 1.f, 1.f };
			style_.rounding = 6.f;
			style_.padding = border_vector{ 6.f, 8.f, 6.f, 8.f };
		}

		[[nodiscard]] float resolved_scale() const noexcept
		{
			const float size = text_size_ > 0.f ? text_size_ : 14.f;
			return size / font_->baked_size();
		}

		[[nodiscard]] float resolved_row_height() const noexcept
		{
			if (row_height_ > 0.f)
			{
				return row_height_;
			}

			return font_->line_height() * resolved_scale() + 6.f;
		}

		template <class T>
		[[nodiscard]] static string_t format_value(const T& v)
		{
			if constexpr (cstd::is_same_v<T, bool>)
			{
				return v ? string_t("true") : string_t("false");
			}
			else if constexpr (cstd::is_floating_point_v<T>)
			{
				return cstd::format("{:.3f}", v);
			}
			else if constexpr (cstd::is_integral_v<T>)
			{
				return cstd::format("{}", v);
			}
			else
			{
				return string_t(v);
			}
		}

		[[nodiscard]] float measure_text(const string_view_t s, const float scale) const noexcept
		{
			float width = 0.f;
			cstd::uint32_t prev = 0;

			const char* p = s.data();
			const char* end = p + s.size();

			while (p < end)
			{
				const cstd::uint32_t cp = decode_utf8(p, end);
				width += glyph_step(*font_, prev, cp, scale);
				prev = cp;
			}

			return width;
		}

		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			if (!font_ || entries_.empty())
			{
				return;
			}

			renderer.push_clip_rect(min, max, style_.rounding.value_or(0.f));

			const float scale = resolved_scale();
			const float row_h = resolved_row_height();
			const float line_h = font_->line_height() * scale;
			const float text_y_offset = (row_h - line_h) * 0.5f;
			const float draw_size = text_size_ > 0.f ? text_size_ : 14.f;

			for (cstd::size_t i = 0; i < entries_.size(); ++i)
			{
				const auto& entry = entries_[i];
				const float row_top = min.y + static_cast<float>(i) * row_h;

				if (zebra_ && (i & 1u) != 0u)
				{
					renderer.draw_rect_filled({ min.x, row_top }, { max.x, row_top + row_h }, zebra_color_);
				}

				const float text_y = row_top + text_y_offset;
				renderer.draw_text(*font_, { min.x, text_y }, entry.name, label_color_, draw_size);

				const string_t value = entry.get ? entry.get() : string_t();
				const float vw = measure_text(value, scale);
				renderer.draw_text(*font_, { max.x - vw, text_y }, value, value_color_, draw_size);
			}

			renderer.pop_clip_rect();
		}

		struct entry
		{
			string_t name;
			function_t<string_t()> get;
		};

		vector_t<entry> entries_;
		shared_ptr_t<gui_font> font_;
		float row_height_ = 0.f;
		float text_size_ = 0.f;
		bool zebra_ = true;
		color label_color_{ 0.65f, 0.65f, 0.72f, 1.f };
		color value_color_{ 0.9f, 0.95f, 1.f, 1.f };
		color zebra_color_{ 1.f, 1.f, 1.f, 0.04f };
	};
}
