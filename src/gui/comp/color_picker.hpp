#pragma once
#include "../element.hpp"
#include "../gui.hpp"
#include "text_box.hpp"
#include "../../input/input.hpp"
#include "../../render/color_convert.hpp"

namespace rv
{
	// The editing surface that opens below the swatch. Draws the saturation/value square, the
	// hue strip and the alpha bar (the hex field is an absolute text_box child owned/positioned
	// by the color_picker). State is shared with the picker through an hsva pointer; every drag
	// writes that state and fires on_edit_ so the picker can commit + notify. Modelled on
	// combo_popup: an absolute, initially-hidden child with no padding so its content box equals
	// its element box, keeping hit-testing aligned with rendering.
	class color_picker_popup final : public element
	{
	public:
		struct regions
		{
			rect sv, hue, alpha, hex;
		};

		explicit color_picker_popup(const element_size size) noexcept
				:	element(size)
		{
			style_.background_color = color{ 0.13f, 0.13f, 0.16f, 1.f };
			style_.rounding = 6.f;
			style_.border_color = color{ 0.3f, 0.3f, 0.36f, 1.f };
			style_.border_width = border_vector{ 1.f, 1.f, 1.f, 1.f };
			style_.shadow_color = color{ 0.f, 0.f, 0.f, 0.5f };
			style_.shadow_blur = 14.f;

			z_index(z_index_popup); // picker popup sits on the popup layer, above any floating panels
		}

		void configure(shared_ptr_t<input> input, hsva* state, function_t<void()> on_edit) noexcept
		{
			input_ = cstd::move(input);
			state_ = state;
			on_edit_ = cstd::move(on_edit);
		}

		[[nodiscard]] bool is_dragging() const noexcept
		{
			return dragging_;
		}

		[[nodiscard]] static constexpr float width() noexcept
		{
			return pad * 2.f + sv_w + gap + bar_w;
		}

		[[nodiscard]] static constexpr float height() noexcept
		{
			return pad * 2.f + sv_h + gap + alpha_h + gap + hex_h;
		}

		// Sub-rects laid out inside the box [min, max]. Used by both rendering and input so the
		// two never drift, and by the picker to position the hex field.
		[[nodiscard]] static regions layout(const position min, const position max) noexcept;

		bool on_mouse_click() override;

		void update(const float dt) override;

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override;

	private:
		enum class region : cstd::uint8_t
		{
			none,
			sv,
			hue,
			alpha
		};

		[[nodiscard]] static bool contains_point(const rect& r, const position p) noexcept
		{
			return p.x >= r.min.x && p.x <= r.max.x && p.y >= r.min.y && p.y <= r.max.y;
		}

		[[nodiscard]] static color rgb(const float r, const float g, const float b, const float a = 1.f) noexcept
		{
			// direct fields to dodge the color(r,g,b,a) >1.0 normalisation
			color c;
			c.r = r;
			c.g = g;
			c.b = b;
			c.a = a;

			return c;
		}

		// Snap to whole pixels so the SDF-rounded corners stay crisp and the checker tiles don't
		// leave sub-pixel seams between them.
		[[nodiscard]] static position floor_pos(const position p) noexcept
		{
			return { cstd::floorf(p.x), cstd::floorf(p.y) };
		}

		void update_from_mouse(const regions& r, const position m) noexcept;

		void draw_sv_square(gui_renderer& renderer, const rect& r) const;

		void draw_hue_strip(gui_renderer& renderer, const rect& r) const;

		void draw_alpha_bar(gui_renderer& renderer, const rect& r) const;

		void draw_cursors(gui_renderer& renderer, const regions& r) const;

		static void draw_checker(gui_renderer& renderer, const position min, const position max);

		static constexpr float pad = 10.f;
		static constexpr float gap = 8.f;
		static constexpr float bar_w = 16.f;
		static constexpr float alpha_h = 16.f;
		static constexpr float hex_h = 26.f;
		static constexpr float sv_w = 170.f;
		static constexpr float sv_h = 140.f;
		static constexpr float region_round = 4.f;

		shared_ptr_t<input> input_;
		hsva* state_ = nullptr;
		function_t<void()> on_edit_;

		region active_ = region::none;
		bool dragging_ = false;
	};

	// Click the swatch to open the popup; drag the square/strip/bar or type a hex value to edit.
	// Two-way binding mirrors slider/combo_box. Canonical state is hsva_ (see color_convert.hpp).
	class color_picker final : public element
	{
	public:
		color_picker(const element_size size, shared_ptr_t<gui_font> font, shared_ptr_t<input> input) noexcept
				:	element(size), font_(cstd::move(font)), input_(cstd::move(input))
		{
			init_defaults();

			popup_ = make_child<color_picker_popup>(element_size{ styled_size::px(0.f), styled_size::px(0.f) });
			popup_->positioning(position_type::absolute);
			popup_->set_visible(false);
			popup_->configure(input_, &hsva_, [this] { commit(); });
		}

		color_picker& value(const color c) noexcept
		{
			hsva_ = rgb_to_hsv(c);

			if (bound_)
			{
				*bound_ = c;
			}

			return *this;
		}

		[[nodiscard]] color value() const noexcept
		{
			return current_color();
		}

		color_picker& bind(color* ptr) noexcept
		{
			bound_ = ptr;

			if (bound_)
			{
				hsva_ = rgb_to_hsv(*bound_);
			}

			return *this;
		}

		color_picker& on_change(function_t<void(color)> callback)
		{
			on_change_ = cstd::move(callback);

			return *this;
		}

		bool on_mouse_click() override
		{
			open_ = !open_;

			return true;
		}

		void update(const float dt) override;

	protected:
		void render_self(gui_renderer& renderer, const position min, const position max) const override;

	private:
		void init_defaults() noexcept
		{
			style_.rounding = 4.f;
			style_.transition_speed = 12.f;
		}

		[[nodiscard]] color current_color() const noexcept
		{
			return hsv_to_rgb(hsva_.h, hsva_.s, hsva_.v, hsva_.a);
		}

		[[nodiscard]] bool popup_contains(const position p) const noexcept
		{
			const position pp = popup_->visual_pos();
			const vector_2d<float> ps = popup_->computed_size();

			return open_ && p.x >= pp.x && p.x <= pp.x + ps.x && p.y >= pp.y && p.y <= pp.y + ps.y;
		}

		void commit();

		// The hex field must take keyboard focus, which only the gui-tree registration path grants
		// (element::make_child does not register). gui_ is set after construction, so create it on
		// the first update once the back-reference resolves.
		void ensure_hex_field();

		void apply_hex(const string_t& s);

		// The popup is a fixed size at a fixed offset below the swatch, so its geometry only needs
		// setting once. Each of these setters marks the layout dirty, so doing it every frame (as
		// before) forced a full-tree relayout every frame the picker existed, the cause of the
		// frame-rate drop. Visibility changes re-trigger layout in update(); a moving anchor (e.g. a
		// dragged panel) marks the layout dirty itself, so the popup still follows it.
		void configure_popup_once();

		void draw_swatch_background(gui_renderer& renderer, const position min, const position max) const;

		shared_ptr_t<gui_font> font_;
		shared_ptr_t<input> input_;
		shared_ptr_t<color_picker_popup> popup_;
		shared_ptr_t<text_box> hex_;

		hsva hsva_;
		color* bound_ = nullptr;
		bool open_ = false;
		bool popup_shown_ = false;
		bool popup_configured_ = false;

		color swatch_border_ = { 0.4f, 0.4f, 0.46f, 1.f };

		function_t<void(color)> on_change_;
	};
}
