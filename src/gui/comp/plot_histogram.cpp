#include "plot_histogram.hpp"

namespace rv
{
	plot_histogram& plot_histogram::data(const span_t<const float> values)
	{
		data_.assign(values.begin(), values.end());
		offset_ = 0;
		streaming_ = false;
		reset_view();

		return *this;
	}

	plot_histogram& plot_histogram::push_value(const float v)
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

	plot_histogram& plot_histogram::view_window(const cstd::size_t samples) noexcept
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

	bool plot_histogram::on_mouse_click()
	{
		if (interactive_ && input_)
		{
			dragging_ = true;
			last_mouse_x_ = input_->mouse_pos().x;
		}

		return interactive_;
	}

	void plot_histogram::update(const float dt)
	{
		element::update(dt);

		if (bound_)
		{
			push_value(*bound_);
		}

		const cstd::size_t count = data_.size();

		if (count < 1)
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
			view_span_ = clampf(view_span_, 1.f, fcount);

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
				view_offset_ -= (mx - last_mouse_x_) * (view_span_ / w);
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

	void plot_histogram::init_defaults() noexcept
	{
		style_.background_color = color{ 0.1f, 0.1f, 0.13f, 1.f };
		style_.border_color = color{ 0.28f, 0.28f, 0.34f, 1.f };
		style_.border_width = border_vector{ 1.f, 1.f, 1.f, 1.f };
		style_.rounding = 6.f;
	}

	void plot_histogram::reset_view() noexcept
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

	void plot_histogram::apply_zoom(const float scroll, const float w, const float fcount) noexcept
	{
		if (scroll == 0.f)
		{
			return;
		}

		const float fx = clampf((input_->mouse_pos().x - visual_pos().x) / w, 0.f, 1.f);
		const float cursor = view_offset_ + fx * view_span_;
		const float factor = clampf(1.f - 0.15f * scroll, 0.2f, 2.f);
		const float new_span = clampf(view_span_ * factor, 1.f, fcount);

		if (new_span >= fcount)
		{
			fit_all_ = true;
		}
		else
		{
			fit_all_ = false;
			view_span_ = new_span;
			view_offset_ = cursor - fx * new_span;
		}
	}

	void plot_histogram::resolve_scale(const cstd::size_t i0, const cstd::size_t i1, float& lo, float& hi) const noexcept
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

	float plot_histogram::measure_text(const string_view_t s, const float scale) const noexcept
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

	void plot_histogram::render_self(gui_renderer& renderer, const position min, const position max) const
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

		const float vo = (!fit_all_ && count >= 1) ? view_offset_ : 0.f;
		const float vs = (!fit_all_ && count >= 1) ? view_span_ : fcount;

		cstd::size_t i0 = 0;
		cstd::size_t i1 = count - 1;

		if (!fit_all_ && count >= 1)
		{
			i0 = static_cast<cstd::size_t>(cstd::fmaxf(0.f, vo));
			const float last_f = vo + vs - 1.f;
			i1 = static_cast<cstd::size_t>(cstd::fmaxf(0.f, last_f));
			i1 = i1 > count - 1 ? count - 1 : i1;
			i0 = i0 > count - 1 ? count - 1 : i0;
			i1 = i1 < i0 ? i0 : i1;
		}

		float lo = 0.f;
		float hi = 1.f;
		resolve_scale(i0, i1, lo, hi);
		const float inv_range = 1.f / (hi - lo);

		const cstd::size_t vis_count = i1 - i0 + 1;
		const float total_gaps = vis_count > 1 ? bar_gap_ * static_cast<float>(vis_count - 1) : 0.f;
		const float bar_w = vis_count > 0 ? (w - total_gaps) / static_cast<float>(vis_count) : w;

		cstd::size_t hover_idx = static_cast<cstd::size_t>(-1);

		for (cstd::size_t i = 0; i < vis_count; ++i)
		{
			const cstd::size_t data_idx = i0 + i;
			const float val = clampf((sample(data_idx) - lo) * inv_range, 0.f, 1.f);
			const float bar_h = val * h;

			const float x0 = min.x + static_cast<float>(i) * (bar_w + bar_gap_);
			const float x1 = x0 + bar_w;
			const float y0 = max.y - bar_h;

			if (bar_h > 0.5f)
			{
				if (bar_rounding_ > 0.f)
				{
					const bool full = val >= 0.99f;
					const auto flags = full
						? rounding_flags_all
						: static_cast<rounding_flags>(rounding_flags_top_left | rounding_flags_top_right);

					renderer.draw_rect_filled({ x0, y0 }, { x1, max.y }, bar_color_, bar_rounding_, flags);
				}
				else
				{
					renderer.draw_rect_filled({ x0, y0 }, { x1, max.y }, bar_color_);
				}
			}

			if (hovered_ && !dragging_ && input_)
			{
				const float mx = input_->mouse_pos().x;

				if (mx >= x0 && mx < x1)
				{
					hover_idx = data_idx;
				}
			}
		}

		if (show_readout_ && hover_idx != static_cast<cstd::size_t>(-1) && font_)
		{
			const float scale = readout_size_ / font_->baked_size();
			const float line_h = font_->line_height() * scale;
			const string_t label = format_sample(hover_idx, sample(hover_idx));
			const float tw = measure_text(label, scale);

			const float pad = 5.f;
			const float mx = input_->mouse_pos().x;
			const float my = input_->mouse_pos().y;

			float bx = mx + 10.f;
			float by = my - line_h - pad * 2.f;

			if (bx + tw + pad * 2.f > max.x)
			{
				bx = mx - 10.f - tw - pad * 2.f;
			}

			if (by < min.y)
			{
				by = my + 10.f;
			}

			const position box_min = { bx, by };
			const position box_max = { bx + tw + pad * 2.f, by + line_h + pad * 2.f };

			renderer.draw_rect_filled(box_min, box_max, readout_bg_, 4.f);
			renderer.draw_text(*font_, { bx + pad, by + pad }, label, readout_text_, readout_size_);
		}

		draw_overlay(renderer, min);

		renderer.pop_clip_rect();
	}

	void plot_histogram::draw_overlay(gui_renderer& renderer, const position min) const
	{
		if (overlay_.empty())
		{
			return;
		}

		renderer.draw_text(*font_, { min.x + 4.f, min.y + 3.f }, overlay_, overlay_color_, readout_size_);
	}
}
