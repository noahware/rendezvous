#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../text_metrics.hpp"
#include "../../input/input.hpp"
#include "../../util/string.hpp"
#include <charconv>

namespace rv
{
	class plot_lines final : public element
	{
	public:
		plot_lines(const element_size size, shared_ptr_t<gui_font> font, shared_ptr_t<input> input) noexcept
				:	element(size), font_(cstd::move(font)), input_(cstd::move(input))
		{
			init_defaults();
		}

		plot_lines& data(const span_t<const float> values)
		{
			data_.assign(values.begin(), values.end());
			offset_ = 0;
			streaming_ = false;

			return *this;
		}

		plot_lines& push_value(const float v)
		{
			streaming_ = true;

			if (capacity_ == 0)
			{
				capacity_ = 128;
			}

			if (data_.size() < capacity_)
			{
				data_.push_back(v);
			}
			else
			{
				data_[offset_] = v;
				offset_ = (offset_ + 1) % capacity_;
			}

			return *this;
		}

		plot_lines& push_values(const span_t<const float> values)
		{
			for (const float v : values)
			{
				push_value(v);
			}

			return *this;
		}

		plot_lines& capacity(const cstd::size_t n) noexcept
		{
			capacity_ = n;
			streaming_ = true;

			return *this;
		}

		plot_lines& autoscale(const bool on = true) noexcept
		{
			autoscale_ = on;

			return *this;
		}

		plot_lines& range(const float lo, const float hi) noexcept
		{
			scale_min_ = lo;
			scale_max_ = hi;
			autoscale_ = false;

			return *this;
		}

		plot_lines& overlay(const string_view_t text)
		{
			overlay_ = string_t(text);

			return *this;
		}

		plot_lines& line_color(const color c) noexcept
		{
			line_color_ = c;

			return *this;
		}

		plot_lines& fill(const bool on) noexcept
		{
			fill_ = on;

			return *this;
		}

		plot_lines& fill_color(const color c) noexcept
		{
			fill_color_ = c;

			return *this;
		}

		plot_lines& line_thickness(const float t) noexcept
		{
			line_thickness_ = t;

			return *this;
		}

	protected:
		void init_defaults() noexcept
		{
			style_.background_color = color{ 0.1f, 0.1f, 0.13f, 1.f };
			style_.border_color = color{ 0.28f, 0.28f, 0.34f, 1.f };
			style_.border_width = border_vector{ 1.f, 1.f, 1.f, 1.f };
			style_.rounding = 6.f;
		}

		[[nodiscard]] float sample(const cstd::size_t i) const noexcept
		{
			return data_[(offset_ + i) % data_.size()];
		}

		void resolve_scale(float& lo, float& hi) const noexcept
		{
			if (!autoscale_)
			{
				lo = scale_min_;
				hi = scale_max_;
			}
			else
			{
				lo = sample(0);
				hi = lo;

				for (cstd::size_t i = 1; i < data_.size(); ++i)
				{
					const float v = sample(i);
					lo = cstd::fminf(lo, v);
					hi = cstd::fmaxf(hi, v);
				}
			}

			if (hi <= lo)
			{
				hi = lo + 1.f;
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

		[[nodiscard]] static string_t format_sample(const cstd::size_t idx, const float v)
		{
			char buf[64];
			char* p = buf;
			char* const last = buf + sizeof(buf);

			p = std::to_chars(p, last, idx).ptr;
			*p++ = ':';
			*p++ = ' ';
			p = std::to_chars(p, last, v, std::chars_format::fixed, 3).ptr;

			return string_t(buf, p);
		}

		void render_self(gui_renderer& renderer, const position min, const position max) const override
		{
			const cstd::size_t count = data_.size();

			if (count == 0)
			{
				draw_overlay(renderer, min);
				return;
			}

			const float w = max.x - min.x;
			const float h = max.y - min.y;

			float lo = 0.f;
			float hi = 1.f;
			resolve_scale(lo, hi);

			const float inv_range = 1.f / (hi - lo);
			const float step = count > 1 ? w / static_cast<float>(count - 1) : 0.f;

			const auto point_at = [&](const cstd::size_t i) noexcept -> position
			{
				const float t = (sample(i) - lo) * inv_range;
				const float cl = cstd::fmaxf(0.f, cstd::fminf(1.f, t));
				return { min.x + step * static_cast<float>(i), max.y - cl * h };
			};

			if (fill_ && count > 1)
			{
				for (cstd::size_t i = 0; i < count; ++i)
				{
					renderer.add_path_point(point_at(i));
				}

				renderer.add_path_point({ max.x, max.y });
				renderer.add_path_point({ min.x, max.y });
				renderer.draw_filled_path(fill_color_);
			}

			if (count > 1)
			{
				for (cstd::size_t i = 0; i < count; ++i)
				{
					renderer.add_path_point(point_at(i));
				}

				renderer.draw_lined_path(line_color_, line_thickness_, false, 1.f,
				                         cap_style::round, join_style::miter);
			}
			else
			{
				renderer.draw_circle_filled(point_at(0), line_thickness_ + 1.f, line_color_);
			}

			draw_probe(renderer, min, max, count, point_at);
			draw_overlay(renderer, min);
		}

		template <class Fn>
		void draw_probe(gui_renderer& renderer, const position min, const position max,
		                const cstd::size_t count, Fn&& point_at) const
		{
			if (!hovered_ || !input_)
			{
				return;
			}

			const float w = max.x - min.x;

			if (w <= 0.f)
			{
				return;
			}

			const position mouse = input_->mouse_pos();
			const float t = (mouse.x - min.x) / w;
			const float clamped = cstd::fmaxf(0.f, cstd::fminf(1.f, t));

			cstd::size_t idx = static_cast<cstd::size_t>(clamped * static_cast<float>(count - 1) + 0.5f);

			if (idx >= count)
			{
				idx = count - 1;
			}

			const position pt = point_at(idx);

			renderer.draw_line({ pt.x, min.y }, { pt.x, max.y }, probe_color_, 1.f);
			renderer.draw_circle_filled(pt, 3.5f, line_color_);

			const float scale = readout_size_ / font_->baked_size();
			const float line_h = font_->line_height() * scale;
			const string_t label = format_sample(idx, sample(idx));
			const float tw = measure_text(label, scale);

			const float pad = 5.f;
			float bx = pt.x + 10.f;
			float by = pt.y - line_h - pad * 2.f;

			if (bx + tw + pad * 2.f > max.x)
			{
				bx = pt.x - 10.f - tw - pad * 2.f;
			}

			if (by < min.y)
			{
				by = pt.y + 10.f;
			}

			const position box_min = { bx, by };
			const position box_max = { bx + tw + pad * 2.f, by + line_h + pad * 2.f };

			renderer.draw_rect_filled(box_min, box_max, readout_bg_, 4.f);
			renderer.draw_text(*font_, { bx + pad, by + pad }, label, readout_text_, readout_size_);
		}

		void draw_overlay(gui_renderer& renderer, const position min) const
		{
			if (overlay_.empty())
			{
				return;
			}

			renderer.draw_text(*font_, { min.x + 4.f, min.y + 3.f }, overlay_, overlay_color_, readout_size_);
		}

		vector_t<float> data_;
		cstd::size_t capacity_ = 128;
		cstd::size_t offset_ = 0;
		bool streaming_ = false;
		bool autoscale_ = true;
		bool fill_ = true;
		float scale_min_ = 0.f;
		float scale_max_ = 1.f;
		float line_thickness_ = 1.5f;
		float readout_size_ = 13.f;
		string_t overlay_;
		color line_color_{ 0.4f, 0.7f, 1.f, 1.f };
		color fill_color_{ 0.4f, 0.7f, 1.f, 0.18f };
		color probe_color_{ 1.f, 1.f, 1.f, 0.35f };
		color readout_bg_{ 0.f, 0.f, 0.f, 0.85f };
		color readout_text_{ 1.f, 1.f, 1.f, 1.f };
		color overlay_color_{ 0.7f, 0.7f, 0.75f, 1.f };
		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;
	};
}
