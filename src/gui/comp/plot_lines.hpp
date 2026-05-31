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
			reset_view();

			return *this;
		}

		plot_lines& push_value(const float v)
		{
			streaming_ = true;

			if (unbounded_)
			{
				data_.push_back(v);
			}
			else if (data_.size() < capacity_)
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
			capacity_ = n == 0 ? 1 : n;
			unbounded_ = false;
			streaming_ = true;

			return *this;
		}

		plot_lines& unbounded(const bool on = true) noexcept
		{
			unbounded_ = on;
			streaming_ = true;

			return *this;
		}

		plot_lines& view_window(const cstd::size_t samples) noexcept
		{
			configured_window_ = samples;

			if (samples == 0)
			{
				fit_all_ = true;
			}
			else
			{
				fit_all_ = false;
				follow_live_ = true;
				view_span_ = static_cast<float>(samples);
			}

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

		plot_lines& interactive(const bool on) noexcept
		{
			interactive_ = on;

			return *this;
		}

		bool on_mouse_click() override
		{
			if (interactive_ && input_)
			{
				dragging_ = true;
				last_mouse_x_ = input_->mouse_pos().x;
			}

			return interactive_;
		}

		void update(const float dt) override
		{
			element::update(dt);

			const cstd::size_t count = data_.size();

			if (count < 2)
			{
				dragging_ = false;
				return;
			}

			const float fcount = static_cast<float>(count);
			const float w = computed_size_.x;

			if (fit_all_)
			{
				view_span_ = fcount;
				view_offset_ = 0.f;
			}
			else
			{
				view_span_ = clampf(view_span_, 2.f, fcount);

				if (follow_live_)
				{
					view_offset_ = fcount - view_span_;
				}
			}

			if (interactive_ && input_ && hovered_ && w > 0.f)
			{
				apply_zoom(input_->scroll_delta(), w, fcount);
			}

			if (dragging_)
			{
				if (!input_ || !input_->is_mouse_down(0))
				{
					dragging_ = false;
				}
				else if (!fit_all_ && w > 0.f)
				{
					const float mx = input_->mouse_pos().x;
					view_offset_ -= (mx - last_mouse_x_) * ((view_span_ - 1.f) / w);
					last_mouse_x_ = mx;
					follow_live_ = false;
				}
				else if (input_)
				{
					last_mouse_x_ = input_->mouse_pos().x;
				}
			}

			if (!fit_all_)
			{
				const float max_off = cstd::fmaxf(0.f, fcount - view_span_);
				view_offset_ = clampf(view_offset_, 0.f, max_off);
				follow_live_ = (view_offset_ + view_span_ >= fcount - 0.5f);
			}
		}

	protected:
		void init_defaults() noexcept
		{
			style_.background_color = color{ 0.1f, 0.1f, 0.13f, 1.f };
			style_.border_color = color{ 0.28f, 0.28f, 0.34f, 1.f };
			style_.border_width = border_vector{ 1.f, 1.f, 1.f, 1.f };
			style_.rounding = 6.f;
		}

		void reset_view() noexcept
		{
			follow_live_ = true;
			view_offset_ = 0.f;
			dragging_ = false;

			if (configured_window_ > 0)
			{
				fit_all_ = false;
				view_span_ = static_cast<float>(configured_window_);
			}
			else
			{
				fit_all_ = true;
				view_span_ = 0.f;
			}
		}

		[[nodiscard]] static float clampf(const float v, const float lo, const float hi) noexcept
		{
			return cstd::fmaxf(lo, cstd::fminf(hi, v));
		}

		void apply_zoom(const float scroll, const float w, const float fcount) noexcept
		{
			if (scroll == 0.f)
			{
				return;
			}

			const float fx = clampf((input_->mouse_pos().x - visual_pos().x) / w, 0.f, 1.f);
			const float cursor = view_offset_ + fx * (view_span_ - 1.f);
			const float factor = clampf(1.f - 0.15f * scroll, 0.2f, 2.f);
			const float new_span = clampf(view_span_ * factor, 2.f, fcount);

			if (new_span >= fcount)
			{
				fit_all_ = true;
			}
			else
			{
				fit_all_ = false;
				view_span_ = new_span;
				view_offset_ = cursor - fx * (new_span - 1.f);
			}
		}

		[[nodiscard]] float sample(const cstd::size_t i) const noexcept
		{
			return data_[(offset_ + i) % data_.size()];
		}

		void resolve_scale(const cstd::size_t i0, const cstd::size_t i1, float& lo, float& hi) const noexcept
		{
			if (!autoscale_)
			{
				lo = scale_min_;
				hi = scale_max_;
			}
			else
			{
				lo = sample(i0);
				hi = lo;

				for (cstd::size_t i = i0 + 1; i <= i1; ++i)
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
			renderer.push_clip_rect(min, max, style_.rounding.value_or(0.f));

			const cstd::size_t count = data_.size();

			if (count == 0)
			{
				draw_overlay(renderer, min);
				renderer.pop_clip_rect();
				return;
			}

			const float w = max.x - min.x;
			const float h = max.y - min.y;
			const float fcount = static_cast<float>(count);

			const float vo = (!fit_all_ && count >= 2) ? view_offset_ : 0.f;
			const float vs = (!fit_all_ && count >= 2) ? view_span_ : fcount;
			const float denom = vs > 1.f ? vs - 1.f : 1.f;

			cstd::size_t i0 = 0;
			cstd::size_t i1 = count - 1;

			if (!fit_all_ && count >= 2)
			{
				i0 = static_cast<cstd::size_t>(cstd::fmaxf(0.f, vo));
				const float last_f = vo + vs - 1.f;
				i1 = static_cast<cstd::size_t>(cstd::fmaxf(0.f, last_f)) + 1;
				i1 = i1 > count - 1 ? count - 1 : i1;
				i0 = i0 > count - 1 ? count - 1 : i0;
				i1 = i1 < i0 ? i0 : i1;
			}

			float lo = 0.f;
			float hi = 1.f;
			resolve_scale(i0, i1, lo, hi);
			const float inv_range = 1.f / (hi - lo);

			const auto point_at = [&](const cstd::size_t i) noexcept -> position
			{
				const float cl = clampf((sample(i) - lo) * inv_range, 0.f, 1.f);
				return { min.x + (static_cast<float>(i) - vo) / denom * w, max.y - cl * h };
			};

			const cstd::size_t vis = i1 - i0 + 1;
			const cstd::size_t max_pts = static_cast<cstd::size_t>(cstd::fmaxf(2.f, w));
			const cstd::size_t stride = vis > max_pts ? vis / max_pts : 1;

			if (i1 > i0)
			{
				if (fill_)
				{
					emit_path(renderer, point_at, i0, i1, stride);
					renderer.add_path_point({ point_at(i1).x, max.y });
					renderer.add_path_point({ point_at(i0).x, max.y });
					renderer.draw_filled_path(fill_color_);
				}

				emit_path(renderer, point_at, i0, i1, stride);
				renderer.draw_lined_path(line_color_, line_thickness_, false, 1.f,
				                         cap_style::round, join_style::miter);
			}
			else
			{
				renderer.draw_circle_filled(point_at(i0), line_thickness_ + 1.f, line_color_);
			}

			draw_probe(renderer, min, max, count, vo, denom, i0, i1, point_at);
			draw_overlay(renderer, min);

			renderer.pop_clip_rect();
		}

		template <class Fn>
		static void emit_path(gui_renderer& renderer, Fn&& point_at, const cstd::size_t i0,
		                      const cstd::size_t i1, const cstd::size_t stride)
		{
			for (cstd::size_t i = i0; i <= i1; i += stride)
			{
				renderer.add_path_point(point_at(i));
			}

			if ((i1 - i0) % stride != 0)
			{
				renderer.add_path_point(point_at(i1));
			}
		}

		template <class Fn>
		void draw_probe(gui_renderer& renderer, const position min, const position max,
		                const cstd::size_t count, const float vo, const float denom,
		                const cstd::size_t i0, const cstd::size_t i1, Fn&& point_at) const
		{
			if (!hovered_ || dragging_ || !input_)
			{
				return;
			}

			const float w = max.x - min.x;

			if (w <= 0.f)
			{
				return;
			}

			const float fx = clampf((input_->mouse_pos().x - min.x) / w, 0.f, 1.f);
			cstd::size_t idx = static_cast<cstd::size_t>(vo + fx * denom + 0.5f);

			idx = idx < i0 ? i0 : idx;
			idx = idx > i1 ? i1 : idx;
			idx = idx > count - 1 ? count - 1 : idx;

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
		cstd::size_t configured_window_ = 0;
		bool streaming_ = false;
		bool unbounded_ = false;
		bool autoscale_ = true;
		bool fill_ = true;
		bool interactive_ = true;
		bool fit_all_ = true;
		bool follow_live_ = true;
		bool dragging_ = false;
		float view_offset_ = 0.f;
		float view_span_ = 0.f;
		float last_mouse_x_ = 0.f;
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
