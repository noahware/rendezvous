#pragma once
#include "../element.hpp"
#include "../gui.hpp"
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
			return *this;
		}

		text_element& text_color(const color col) noexcept
		{
			color_ = col;
			return *this;
		}

		text_element& text_size(const float size) noexcept
		{
			font_size_ = size;
			return *this;
		}

		text_element& text_alignment(const text_align align) noexcept
		{
			align_ = align;
			return *this;
		}

		[[nodiscard]] vector_2d<float> content_size(const vector_2d<float> available) const noexcept override
		{
			if (!font_ || text_.empty())
			{
				return { 0.f, 0.f };
			}

			const float scale = font_size_ > 0.f ? font_size_ / font_->baked_size() : 1.f;
			const float line_h = font_->line_height() * scale;
			const bool has_width_constraint = (declared_size().width.mode != size_mode::auto_v);

			if (!has_width_constraint)
			{
				const float w = single_line_width(scale);
				return { w, line_h };
			}

			const auto wrapped = wrap_text(available.x, scale);
			float max_w = 0.f;

			for (const auto& line : wrapped)
			{
				max_w = cstd::fmaxf(max_w, measure_line(line, scale));
			}

			return { max_w, line_h * static_cast<float>(wrapped.size()) };
		}

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			if (!font_ || text_.empty())
			{
				return;
			}

			const float scale = font_size_ > 0.f ? font_size_ / font_->baked_size() : 1.f;
			const float line_h = font_->line_height() * scale;
			const float box_w = max.x - min.x;

			const auto wrapped = wrap_text(box_w, scale);

			renderer.push_clip_rect(min, max);

			float y = min.y;

			for (const auto& line : wrapped)
			{
				float x = min.x;

				if (align_ == text_align::center)
				{
					const float lw = measure_line(line, scale);
					x += (box_w - lw) * 0.5f;
				}
				else if (align_ == text_align::right)
				{
					const float lw = measure_line(line, scale);
					x += box_w - lw;
				}

				renderer.draw_text(*font_, { x, y }, line, color_, font_size_);
				y += line_h;
			}

			renderer.pop_clip_rect();
		}

	private:
		[[nodiscard]] float single_line_width(const float scale) const noexcept
		{
			float width = 0.f;
			cstd::uint32_t prev_cp = 0;

			const char* s = text_.data();
			const char* end = s + text_.size();

			while (s < end)
			{
				const cstd::uint32_t cp = decode_utf8(s, end);

				if (cp == '\n' || cp == '\r')
				{
					prev_cp = 0;
					continue;
				}

				if (prev_cp != 0)
				{
					width += font_->kerning(prev_cp, cp) * scale;
				}

				width += font_->glyph_advance(cp) * scale;
				prev_cp = cp;
			}

			return width;
		}

		[[nodiscard]] float measure_line(const string_view_t line, const float scale) const noexcept
		{
			float width = 0.f;
			cstd::uint32_t prev_cp = 0;

			const char* s = line.data();
			const char* end = s + line.size();

			while (s < end)
			{
				const cstd::uint32_t cp = decode_utf8(s, end);

				if (prev_cp != 0)
				{
					width += font_->kerning(prev_cp, cp) * scale;
				}

				width += font_->glyph_advance(cp) * scale;
				prev_cp = cp;
			}

			return width;
		}

		[[nodiscard]] vector_t<string_view_t> wrap_text(const float max_width, const float scale) const noexcept
		{
			vector_t<string_view_t> lines;

			if (text_.empty() || max_width <= 0.f)
			{
				return lines;
			}

			const char* text_start = text_.data();
			const char* text_end = text_start + text_.size();

			const char* line_start = text_start;
			const char* s = text_start;
			float current_width = 0.f;
			cstd::uint32_t prev_cp = 0;

			const char* last_break = nullptr;
			float width_at_break = 0.f;

			while (s < text_end)
			{
				const char* char_start = s;
				const cstd::uint32_t cp = decode_utf8(s, text_end);

				// forced line break
				if (cp == '\n')
				{
					lines.emplace_back(line_start, static_cast<cstd::size_t>(char_start - line_start));
					line_start = s;
					current_width = 0.f;
					prev_cp = 0;
					last_break = nullptr;
					continue;
				}

				if (cp == '\r')
				{
					prev_cp = 0;
					continue;
				}

				// record break opportunity at spaces
				if (cp == ' ' || cp == '\t')
				{
					last_break = s; // position AFTER the space
					width_at_break = current_width;
				}

				// accumulate width
				if (prev_cp != 0)
				{
					current_width += font_->kerning(prev_cp, cp) * scale;
				}

				current_width += font_->glyph_advance(cp) * scale;
				prev_cp = cp;

				// check overflow
				if (current_width > max_width && char_start != line_start)
				{
					if (last_break)
					{
						// break at last space
						const cstd::size_t len = static_cast<cstd::size_t>(last_break - line_start);

						// trim trailing space from line
						const char* line_end = last_break;
						while (line_end > line_start && *(line_end - 1) == ' ')
						{
							--line_end;
						}

						lines.emplace_back(line_start, static_cast<cstd::size_t>(line_end - line_start));
						line_start = last_break;
						current_width = current_width - width_at_break;
						last_break = nullptr;
						width_at_break = 0.f;

						// re-measure from last_break to current position for correct kerning
						// (simplified: current_width approximation is acceptable for now)
					}
					else
					{
						// no break point - force break at current char
						lines.emplace_back(line_start, static_cast<cstd::size_t>(char_start - line_start));
						line_start = char_start;
						current_width = font_->glyph_advance(cp) * scale;
						prev_cp = cp;
						last_break = nullptr;
						width_at_break = 0.f;
					}
				}
			}

			// emit final line
			if (line_start < text_end)
			{
				lines.emplace_back(line_start, static_cast<cstd::size_t>(text_end - line_start));
			}

			return lines;
		}

		shared_ptr_t<gui_font> font_;
		string_t text_;
		color color_ = { 1.f, 1.f, 1.f, 1.f };
		float font_size_ = 0.f;
		text_align align_ = text_align::left;
	};
}
