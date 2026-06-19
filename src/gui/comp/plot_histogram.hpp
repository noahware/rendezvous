#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "../text_metrics.hpp"
#include "../../input/input.hpp"
#include "../../util/string.hpp"

namespace rv
{
	class plot_histogram final : public element
	{
	public:
		plot_histogram(const element_size size, shared_ptr_t<gui_font> font, shared_ptr_t<input> input) noexcept
				:	element(size), font_(cstd::move(font)), input_(cstd::move(input))
		{
			init_defaults();
		}

		plot_histogram& data(const span_t<const float> values);

		plot_histogram& push_value(const float v);

		plot_histogram& push_values(const span_t<const float> values)
		{
			for (const float v : values)
			{
				push_value(v);
			}

			return *this;
		}

		plot_histogram& capacity(const cstd::size_t n) noexcept
		{
			capacity_ = n == 0 ? 1 : n;
			unbounded_ = false;
			streaming_ = true;

			return *this;
		}

		plot_histogram& unbounded(const bool on = true) noexcept
		{
			unbounded_ = on;
			streaming_ = true;

			return *this;
		}

		plot_histogram& bind(const float* ptr) noexcept
		{
			bound_ = ptr;
			streaming_ = true;

			return *this;
		}

		plot_histogram& view_window(const cstd::size_t samples) noexcept;

		plot_histogram& autoscale(const bool on = true) noexcept
		{
			autoscale_ = on;

			return *this;
		}

		plot_histogram& range(const float lo, const float hi) noexcept
		{
			scale_min_ = lo;
			scale_max_ = hi;
			autoscale_ = false;

			return *this;
		}

		plot_histogram& overlay(const string_view_t text)
		{
			overlay_ = string_t(text);

			return *this;
		}

		plot_histogram& bar_color(const color c) noexcept
		{
			bar_color_ = c;

			return *this;
		}

		plot_histogram& bar_gap(const float gap) noexcept
		{
			bar_gap_ = gap;

			return *this;
		}

		plot_histogram& bar_rounding(const float r) noexcept
		{
			bar_rounding_ = r;

			return *this;
		}

		plot_histogram& show_readout(const bool on) noexcept
		{
			show_readout_ = on;

			return *this;
		}

		plot_histogram& interactive(const bool on) noexcept
		{
			interactive_ = on;

			return *this;
		}

		plot_histogram& show_axis(const bool on) noexcept
		{
			show_axis_ = on;

			return *this;
		}

		plot_histogram& show_grid(const bool on) noexcept
		{
			show_grid_ = on;

			return *this;
		}

		plot_histogram& axis_ticks(const cstd::int32_t n) noexcept
		{
			axis_ticks_ = n > 0 ? n : 1;

			return *this;
		}

		plot_histogram& axis_color(const color c) noexcept
		{
			axis_color_ = c;

			return *this;
		}

		plot_histogram& grid_color(const color c) noexcept
		{
			grid_color_ = c;

			return *this;
		}

		plot_histogram& axis_font_size(const float px) noexcept
		{
			axis_font_size_ = px;

			return *this;
		}

		bool on_mouse_click() override;

		void update(const float dt) override;

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override;

	private:
		void init_defaults() noexcept;

		void reset_view() noexcept;

		[[nodiscard]] static float clampf(const float v, const float lo, const float hi) noexcept
		{
			return cstd::fmaxf(lo, cstd::fminf(hi, v));
		}

		void apply_zoom(const float scroll, const float w, const float fcount) noexcept;

		[[nodiscard]] float sample(const cstd::size_t i) const noexcept
		{
			return data_[(offset_ + i) % data_.size()];
		}

		void resolve_scale(const cstd::size_t i0, const cstd::size_t i1, float& lo, float& hi) const noexcept;

		[[nodiscard]] float measure_text(const string_view_t s, const float scale) const noexcept;

		[[nodiscard]] static string_t format_sample(const cstd::size_t idx, const float v)
		{
			return cstd::format("{}: {:.3f}", idx, v);
		}

		void draw_overlay(gui_renderer& renderer, const position min) const;

		static float nice_number(float x, bool round_up) noexcept;
		void compute_axis_range(float data_lo, float data_hi, float& axis_lo, float& axis_hi, float& tick_step) const noexcept;
		static string_t format_axis_value(float v, float tick_step);
		float compute_axis_margin(float axis_lo, float axis_hi, float tick_step) const noexcept;
		void draw_axis(gui_renderer& renderer, position element_min, position plot_min, position plot_max, float axis_lo, float axis_hi, float tick_step) const;

		vector_t<float> data_;
		cstd::size_t capacity_ = 128;
		cstd::size_t offset_ = 0;
		cstd::size_t configured_window_ = 0;
		bool streaming_ = false;
		bool unbounded_ = false;
		const float* bound_ = nullptr;
		bool autoscale_ = true;
		bool interactive_ = true;
		bool fit_all_ = true;
		bool follow_live_ = true;
		bool dragging_ = false;
		bool show_readout_ = true;
		float view_offset_ = 0.f;
		float view_span_ = 0.f;
		float last_mouse_x_ = 0.f;
		float scale_min_ = 0.f;
		float scale_max_ = 1.f;
		float bar_gap_ = 1.f;
		float bar_rounding_ = 0.f;
		float readout_size_ = 13.f;
		string_t overlay_;
		color bar_color_{ 0.4f, 0.7f, 1.f, 0.85f };
		color readout_bg_{ 0.f, 0.f, 0.f, 0.85f };
		color readout_text_{ 1.f, 1.f, 1.f, 1.f };
		color overlay_color_{ 0.7f, 0.7f, 0.75f, 1.f };
		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;

		bool show_axis_ = false;
		bool show_grid_ = false;
		cstd::int32_t axis_ticks_ = 5;
		float axis_font_size_ = 14.f;
		color axis_color_{ 0.5f, 0.5f, 0.55f, 1.f };
		color grid_color_{ 0.2f, 0.2f, 0.24f, 0.5f };
	};
}
