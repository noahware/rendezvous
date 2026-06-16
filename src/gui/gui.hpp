#pragma once
#include "../render/render.hpp"
#include "../render/texture.hpp"
#include "../input/input.hpp"
#include "../util/string.hpp"
#include "layout.hpp"

namespace rv
{
	class gui_font
	{
	public:
		virtual ~gui_font() = default;
		[[nodiscard]] virtual float glyph_advance(cstd::uint32_t codepoint) const noexcept = 0;
		[[nodiscard]] virtual float kerning(cstd::uint32_t left, cstd::uint32_t right) const noexcept = 0;
		[[nodiscard]] virtual float line_height() const noexcept = 0;
		[[nodiscard]] virtual float ascent() const noexcept = 0;
		[[nodiscard]] virtual float descent() const noexcept = 0;
		[[nodiscard]] virtual float baked_size() const noexcept = 0;
	};

	class gui_font_impl : public gui_font
	{
	public:
		explicit gui_font_impl(const font& f) noexcept
			: font_(f) { }

		[[nodiscard]] float glyph_advance(const cstd::uint32_t codepoint) const noexcept override
		{
			return font_.glyph(codepoint).advance;
		}

		[[nodiscard]] float kerning(const cstd::uint32_t left, const cstd::uint32_t right) const noexcept override
		{
			return font_.kerning(left, right);
		}

		[[nodiscard]] float line_height() const noexcept override
		{
			return font_.line_height();
		}

		[[nodiscard]] float ascent() const noexcept override
		{
			return font_.ascent();
		}

		[[nodiscard]] float descent() const noexcept override
		{
			return font_.descent();
		}

		[[nodiscard]] float baked_size() const noexcept override
		{
			return font_.baked_size();
		}

		[[nodiscard]] const font& underlying() const noexcept
		{
			return font_;
		}

	protected:
		const font& font_;
	};

	// GUI-facing handle to a renderer texture. Widgets hold one of these instead of the concrete
	// `texture`, so GUI code never depends on the render backend (mirrors gui_font / gui_renderer).
	class gui_texture
	{
	public:
		virtual ~gui_texture() = default;
		// native pixel dimensions; 0 when unknown (e.g. a texture adopted from an external SRV).
		[[nodiscard]] virtual cstd::uint32_t width() const noexcept = 0;
		[[nodiscard]] virtual cstd::uint32_t height() const noexcept = 0;
	};

	class gui_texture_impl : public gui_texture
	{
	public:
		explicit gui_texture_impl(shared_ptr_t<texture> tex) noexcept
			: texture_(cstd::move(tex)) { }

		[[nodiscard]] cstd::uint32_t width() const noexcept override
		{
			return texture_ ? texture_->width() : 0;
		}

		[[nodiscard]] cstd::uint32_t height() const noexcept override
		{
			return texture_ ? texture_->height() : 0;
		}

		[[nodiscard]] const shared_ptr_t<texture>& underlying() const noexcept
		{
			return texture_;
		}

	protected:
		shared_ptr_t<texture> texture_;
	};

	class gui_renderer
	{
	public:
		virtual ~gui_renderer() = default;
		virtual void draw_rect(position min, position max, color col, float thickness = 1.f, float rounding = 0.f) noexcept = 0;
		virtual void draw_rect_filled(position min, position max, color col, float rounding = 0.f, rounding_flags flags = rounding_flags_all) noexcept = 0;
		virtual void draw_rect_filled_multi_color(position min, position max, color col_tl, color col_tr, color col_br, color col_bl, float rounding = 0.f, rounding_flags flags = rounding_flags_all) noexcept = 0;
		virtual void draw_rect(position min, position max, color col, float thickness, corner_radii radii) noexcept = 0;
		virtual void draw_rect_filled(position min, position max, color col, corner_radii radii) noexcept = 0;
		virtual void draw_rect_filled_multi_color(position min, position max, color col_tl, color col_tr, color col_br, color col_bl, corner_radii radii) noexcept = 0;
		virtual void draw_circle(position pos, float radius, color col, float thickness = 1.f, cstd::size_t segment_count = 32) noexcept = 0;
		virtual void draw_circle_filled(position pos, float radius, color col) noexcept = 0;
		virtual void draw_shadow_rect(position min, position max, color col, float rounding = 0.f,
		                              float shadow_blur = 15.f, float shadow_spread = 0.f,
		                              rounding_flags flags = rounding_flags_all,
		                              bool cut_background = false) noexcept = 0;
		virtual void draw_shadow_rect_multi_color(position min, position max, color col_tl, color col_tr,
		                                          color col_br, color col_bl, float rounding = 0.f,
		                                          float shadow_blur = 15.f, float shadow_spread = 0.f,
		                                          rounding_flags flags = rounding_flags_all,
		                                          bool cut_background = false) noexcept = 0;
		virtual void draw_image(shared_ptr_t<gui_texture> tex, position min, position max, position uv_min = { 0.f, 0.f }, position uv_max = { 1.f, 1.f }, color tint = { 1.f, 1.f, 1.f, 1.f }) noexcept = 0;
		virtual void draw_image_rounded(shared_ptr_t<gui_texture> tex, position min, position max, float rounding, rounding_flags flags = rounding_flags_all, position uv_min = { 0.f, 0.f }, position uv_max = { 1.f, 1.f }, color tint = { 1.f, 1.f, 1.f, 1.f }) noexcept = 0;
		[[nodiscard]] virtual shared_ptr_t<gui_texture> load_texture(const string_t& path) = 0;
		[[nodiscard]] virtual shared_ptr_t<gui_texture> load_texture_from_memory(span_t<const cstd::uint8_t> encoded) = 0;
		virtual void draw_text(const gui_font& font, position pos, string_view_t text, color col, float size = 0.f, float letter_spacing = 0.f) noexcept = 0;
		virtual void add_text_shadow(const gui_font& font, position pos, string_view_t text, color col, float blur, float size = 0.f) noexcept = 0;
		virtual void draw_line(position a, position b, color col, float thickness = 1.f) noexcept = 0;
		virtual void add_path_point(position p) noexcept = 0;
		virtual void draw_lined_path(color col, float thickness = 1.f, bool closed = true, float fringe = 1.f,
		                             cap_style cap = cap_style::flat, join_style join = join_style::miter) noexcept = 0;
		virtual void draw_filled_path(color col, float fringe = 1.f) noexcept = 0;
		virtual void draw_filled_path_monotone(color col, float baseline_y) noexcept = 0;
		virtual void push_clip_rect(position min, position max, float rounding = 0.f, rounding_flags flags = rounding_flags_all) noexcept = 0;
		virtual void pop_clip_rect() noexcept = 0;
		virtual float delta_time() const noexcept = 0;

		// vertex-range editing used by element::render() for subtree opacity and scale transforms.
		[[nodiscard]] virtual cstd::size_t vertex_count() const noexcept = 0;
		virtual void modify_alpha(cstd::size_t start, cstd::size_t end, float alpha) noexcept = 0;
		virtual void modify_scale(cstd::size_t start, cstd::size_t end, position center, float scale) noexcept = 0;

		virtual void execute_backdrop_blur(position min, position max, float sigma, float rounding) noexcept = 0;
	};

	class gui_renderer_impl : public gui_renderer
	{
	public:
		explicit gui_renderer_impl(shared_ptr_t<renderer> renderer)
				:	renderer_(cstd::move(renderer)) { }

		void draw_rect(const position min, const position max, const color col, const float thickness,
		               const float rounding) noexcept override
		{
			return renderer_->draw_rect(min, max, col, thickness, rounding);
		}

		void draw_rect_filled(const position min, const position max, const color col, const float rounding,
		                      const rounding_flags flags) noexcept override
		{
			return renderer_->draw_rect_filled(min, max, col, rounding, flags);
		}

		void draw_rect_filled_multi_color(const position min, const position max, const color col_tl, const color col_tr,
		                                  const color col_br, const color col_bl, const float rounding,
		                                  const rounding_flags flags) noexcept override
		{
			return renderer_->draw_rect_filled_multi_color(min, max, col_tl, col_tr, col_br, col_bl, rounding, flags);
		}

		void draw_rect(const position min, const position max, const color col, const float thickness,
		               const corner_radii radii) noexcept override
		{
			return renderer_->draw_rect(min, max, col, thickness, radii);
		}

		void draw_rect_filled(const position min, const position max, const color col, const corner_radii radii) noexcept override
		{
			return renderer_->draw_rect_filled(min, max, col, radii);
		}

		void draw_rect_filled_multi_color(const position min, const position max, const color col_tl, const color col_tr,
		                                  const color col_br, const color col_bl, const corner_radii radii) noexcept override
		{
			return renderer_->draw_rect_filled_multi_color(min, max, col_tl, col_tr, col_br, col_bl, radii);
		}

		[[nodiscard]] cstd::size_t vertex_count() const noexcept override
		{
			return renderer_->vertex_count();
		}

		void modify_alpha(const cstd::size_t start, const cstd::size_t end, const float alpha) noexcept override
		{
			renderer_->modify_alpha(start, end, alpha);
		}

		void modify_scale(const cstd::size_t start, const cstd::size_t end, const position center, const float scale) noexcept override
		{
			renderer_->modify_scale(start, end, center, scale);
		}

		void draw_circle(const position pos, const float radius, const color col, const float thickness,
		                 const cstd::size_t segment_count) noexcept override
		{
			return renderer_->draw_circle(pos, radius, col, thickness, segment_count);
		}

		void draw_circle_filled(const position pos, const float radius, const color col) noexcept override
		{
			return renderer_->draw_circle_filled(pos, radius, col);
		}

		void draw_shadow_rect(const position min, const position max, const color col, const float rounding,
		                      const float shadow_blur, const float shadow_spread,
		                      const rounding_flags flags, const bool cut_background) noexcept override
		{
			return renderer_->draw_shadow_rect(min, max, col, rounding, shadow_blur, shadow_spread, flags, cut_background);
		}

		void draw_shadow_rect_multi_color(const position min, const position max, const color col_tl, const color col_tr,
		                                  const color col_br, const color col_bl, const float rounding,
		                                  const float shadow_blur, const float shadow_spread,
		                                  const rounding_flags flags, const bool cut_background) noexcept override
		{
			return renderer_->draw_shadow_rect_multi_color(min, max, col_tl, col_tr, col_br, col_bl, rounding, shadow_blur, shadow_spread, flags, cut_background);
		}

		void draw_image(shared_ptr_t<gui_texture> tex, const position min, const position max, const position uv_min,
		                const position uv_max, const color tint) noexcept override
		{
			if (!tex) { return; }
			renderer_->draw_image(static_cast<gui_texture_impl*>(tex.get())->underlying(), min, max, uv_min, uv_max, tint);
		}

		void draw_image_rounded(shared_ptr_t<gui_texture> tex, const position min, const position max, const float rounding,
		                        const rounding_flags flags, const position uv_min, const position uv_max,
		                        const color tint) noexcept override
		{
			if (!tex) { return; }
			renderer_->draw_image_rounded(static_cast<gui_texture_impl*>(tex.get())->underlying(), min, max, rounding, flags, uv_min, uv_max, tint);
		}

		// wrap the decoded texture in a gui_texture; null underlying (load failed) stays null so the
		// image widget / background draw keep skipping it via their `if (!tex)` guards.
		[[nodiscard]] shared_ptr_t<gui_texture> load_texture(const string_t& path) override
		{
			auto tex = renderer_->load_texture(path);
			return tex ? cstd::make_shared<gui_texture_impl>(cstd::move(tex)) : nullptr;
		}

		[[nodiscard]] shared_ptr_t<gui_texture> load_texture_from_memory(const span_t<const cstd::uint8_t> encoded) override
		{
			auto tex = renderer_->load_texture_from_memory(encoded);
			return tex ? cstd::make_shared<gui_texture_impl>(cstd::move(tex)) : nullptr;
		}

		void draw_text(const gui_font& gf, const position pos, const string_view_t text,
		               const color col, const float size, const float letter_spacing) noexcept override
		{
			const auto* impl = static_cast<const gui_font_impl*>(&gf);
			renderer_->draw_text(impl->underlying(), pos, text, col, size, letter_spacing);
		}

		void add_text_shadow(const gui_font& gf, const position pos, const string_view_t text,
		                     const color col, const float blur, const float size) noexcept override
		{
			const auto* impl = static_cast<const gui_font_impl*>(&gf);
			renderer_->add_text_shadow(impl->underlying(), pos, text, col, blur, size);
		}

		void draw_line(const position a, const position b, const color col, const float thickness) noexcept override
		{
			renderer_->draw_line(a, b, col, thickness);
		}

		void add_path_point(const position p) noexcept override
		{
			renderer_->add_path_point(p);
		}

		void draw_lined_path(const color col, const float thickness, const bool closed, const float fringe,
		                     const cap_style cap, const join_style join) noexcept override
		{
			renderer_->draw_lined_path(col, thickness, closed, fringe, cap, join);
		}

		void draw_filled_path(const color col, const float fringe) noexcept override
		{
			renderer_->draw_filled_path(col, fringe);
		}

		void draw_filled_path_monotone(const color col, const float baseline_y) noexcept override
		{
			renderer_->draw_filled_path_monotone(col, baseline_y);
		}

		void push_clip_rect(const position min, const position max, const float rounding = 0.f, const rounding_flags flags = rounding_flags_all) noexcept override
		{
			renderer_->push_clip_rect(min, max, rounding, flags);
		}

		void pop_clip_rect() noexcept override
		{
			renderer_->pop_clip_rect();
		}

		float delta_time() const noexcept override
		{
			return renderer_->state().delta_time;
		}

		void execute_backdrop_blur(const position min, const position max, const float sigma, const float rounding) noexcept override
		{
			renderer_->execute_backdrop_blur(min, max, sigma, rounding);
		}

	protected:
		shared_ptr_t<renderer> renderer_;
	};

	// Per-frame GUI stage timings in milliseconds, smoothed with an EMA so the readout is stable.
	// Populated by gui::render(); surface them via a value_inspector or overlay.
	struct gui_profile
	{
		float layout_ms = 0.f;
		float update_ms = 0.f;
		float events_ms = 0.f;
		float render_ms = 0.f;
		float total_ms = 0.f;
	};

	class portrait_background_element final : public element
	{
	public:
		portrait_background_element() noexcept
			: element(element_size{ styled_size::fill(), styled_size::fill() })
		{
			z_index(z_index_popup - 1);
			style_.position = position_type::absolute;
		}

		bool on_mouse_click() override;
	};

	class gui
	{
	public:
		explicit gui(unique_ptr_t<gui_renderer> renderer, shared_ptr_t<input> input)
				:	renderer_(cstd::move(renderer)), input_(cstd::move(input))
		{
			tree_.set_layout_dirty_ptr(&layout_dirty_);
		}

		void attach(const shared_ptr_t<gui>& self)
		{
			tree_.root()->set_gui(self);
		}

		// raise a floating panel above its siblings: hand out a monotonically increasing z within the
		// panel layer (kept just below the popup layer). called by panel on click (focus to front).
		[[nodiscard]] cstd::int32_t raise_panel() noexcept
		{
			if (next_panel_z_ < z_index_popup - 1)
			{
				++next_panel_z_;
			}

			return next_panel_z_;
		}

		void show_portrait_focus(const shared_ptr_t<element>& target, const float blur_sigma = 8.f,
		                        const color dim = { 0, 0, 0, 0.3f })
		{
			if (!portrait_background_)
			{
				portrait_background_ = tree_.make_child<portrait_background_element>(tree_.root());
			}

			portrait_background_->backdrop_blur(blur_sigma);
			portrait_background_->background_color(dim);
			portrait_background_->set_visible(true);
			portrait_target_ = target;
			target->z_index(z_index_popup);
			layout_dirty_ = true;
		}

		void hide_portrait_focus()
		{
			if (portrait_background_)
			{
				portrait_background_->set_visible(false);
			}

			if (portrait_target_)
			{
				portrait_target_->z_index(z_index_panel);
				portrait_target_ = {};
			}

			layout_dirty_ = true;
		}

		[[nodiscard]] bool has_portrait_focus() const noexcept
		{
			return portrait_target_ != nullptr;
		}

		RV_WIDGET_FACTORY_DECLS

		void render(const vector_2d<float> display_size)
		{
			const auto root = tree_.root();
			const auto t_start = cstd::get_time();

			const bool size_changed = display_size.x != last_display_size_.x
				|| display_size.y != last_display_size_.y;

			if (layout_dirty_ || size_changed)
			{
				layout(*root, display_size, default_style_);
				resolve_positions(*root, position{ 0.f, 0.f }, default_style_);
				layout_dirty_ = false;
				last_display_size_ = display_size;
			}

			const auto t_layout = cstd::get_time();

			// apply the cursor resolved by the previous frame's process_events as this frame's baseline,
			// before update_all lets widgets request a dynamic cursor (panel resize edges, text caret).
			if (input_)
			{
				input_->set_cursor(frame_cursor_);
			}

			update_all(*root, renderer_->delta_time());
			const auto t_update = cstd::get_time();

			process_events();
			const auto t_events = cstd::get_time();

			// advance the hover-to-show timer; reset whenever the hovered target changes.
			// tooltip_hover_ holds this frame's live element, so deref stays safe.
			const float dt = renderer_->delta_time();

			if (tooltip_hover_ != tooltip_target_)
			{
				tooltip_target_ = tooltip_hover_;
				tooltip_timer_ = 0.f;
			}
			else if (tooltip_target_)
			{
				tooltip_timer_ += dt;
			}

			// topmost elements (popups, panels) are collected during the tree pass and drawn afterwards
			// so they paint over everything, regardless of their position in the tree.
			vector_t<deferred_render> overlays;
			root->render(*renderer_, default_style_, position{ 0.f, 0.f }, &overlays);

			// drain the overlay list level by level: rendering a deferred element can surface further
			// deferred descendants (e.g. a popup nested inside a now-topmost panel must escape the
			// panel's clip and draw above it). each level is z-sorted; deeper levels draw later/on top.
			while (!overlays.empty())
			{
				// stable insertion sort by z ascending: higher z draws last (on top), ties keep order.
				for (cstd::size_t i = 1; i < overlays.size(); ++i)
				{
					const deferred_render key = overlays[i];
					const cstd::int32_t key_z = key.el ? key.el->z_index() : 0;
					cstd::size_t j = i;

					while (j > 0 && (overlays[j - 1].el ? overlays[j - 1].el->z_index() : 0) > key_z)
					{
						overlays[j] = overlays[j - 1];
						--j;
					}

					overlays[j] = key;
				}

				vector_t<deferred_render> next;

				for (const auto& cmd : overlays)
				{
					if (cmd.el)
					{
						cmd.el->render(*renderer_, default_style_, cmd.offset, &next);
					}
				}

				overlays = cstd::move(next);
			}

			// drawn last and unclipped, so the tooltip always paints over the tree.
			if (tooltip_target_ && tooltip_timer_ >= tooltip_delay && font_ && input_)
			{
				draw_tooltip(tooltip_target_->tooltip(), input_->mouse_pos(), display_size);
			}

			// stage timings, smoothed so the readout doesn't jitter frame to frame.
			const auto t_end = cstd::get_time();
			constexpr float ema = 0.15f;
			profile_.layout_ms += ema * (cstd::get_time_diff(t_layout, t_start) * 1000.f - profile_.layout_ms);
			profile_.update_ms += ema * (cstd::get_time_diff(t_update, t_layout) * 1000.f - profile_.update_ms);
			profile_.events_ms += ema * (cstd::get_time_diff(t_events, t_update) * 1000.f - profile_.events_ms);
			profile_.render_ms += ema * (cstd::get_time_diff(t_end, t_events) * 1000.f - profile_.render_ms);
			profile_.total_ms  += ema * (cstd::get_time_diff(t_end, t_start) * 1000.f - profile_.total_ms);
		}

		void set_font(shared_ptr_t<gui_font> font) noexcept
		{
			font_ = cstd::move(font);
		}

		[[nodiscard]] shared_ptr_t<element> root() const noexcept
		{
			return tree_.root();
		}

		[[nodiscard]] element_style& default_style() noexcept
		{
			return default_style_;
		}

		[[nodiscard]] const element_style& default_style() const noexcept
		{
			return default_style_;
		}

		[[nodiscard]] gui_profile& profile() noexcept
		{
			return profile_;
		}

		[[nodiscard]] const gui_profile& profile() const noexcept
		{
			return profile_;
		}

		[[nodiscard]] const shared_ptr_t<input>& get_input() const noexcept
		{
			return input_;
		}

		[[nodiscard]] const shared_ptr_t<gui_font>& font() const noexcept
		{
			return font_;
		}

		// decode an image into a gui_texture via the underlying renderer, so GUI-only code can build
		// textures (for add_image / background_image) without holding the raw renderer.
		[[nodiscard]] shared_ptr_t<gui_texture> load_texture(const string_t& path)
		{
			return renderer_->load_texture(path);
		}

		[[nodiscard]] shared_ptr_t<gui_texture> load_texture_from_memory(const span_t<const cstd::uint8_t> encoded)
		{
			return renderer_->load_texture_from_memory(encoded);
		}

		template <class T, class ...Args>
		shared_ptr_t<T> make_child(const shared_ptr_t<element>& parent, Args&&... args)
		{
			return tree_.make_child<T>(parent, cstd::forward<Args>(args)...);
		}

	protected:
		void process_events_recursive(const shared_ptr_t<element>& el, const position& mouse_pos, const bool mouse_clicked, const bool mouse_down, bool& click_handled, bool& enter_handled, shared_ptr_t<element>& scroll_target, element*& clicked, element*& tooltip_hover, element*& cursor_hover, const bool ancestor_disabled)
		{
			if (!el->is_visible())
			{
				return;
			}

			// disabled cascades down: an element inside a disabled ancestor is inert too.
			const bool disabled = ancestor_disabled || el->is_disabled();

			const auto& children = el->children();
			for (auto it = children.rbegin(); it != children.rend(); ++it)
			{
				// topmost children (popups) are dispatched first by process_events for input
				// priority, so skip them here to avoid double-processing.
				if ((*it)->is_topmost())
				{
					continue;
				}

				process_events_recursive(*it, mouse_pos, mouse_clicked, mouse_down, click_handled, enter_handled, scroll_target, clicked, tooltip_hover, cursor_hover, disabled);
			}

			// a disabled element (or one within a disabled subtree) takes no input: clear any stale
			// hover/press so it can't get visually stuck, and contribute no click/hover/cursor/scroll.
			// it still renders, dimmed via resolve_visual().
			if (disabled)
			{
				if (el->is_hovered())
				{
					el->set_hovered(false);
					el->on_mouse_exit();
				}

				el->set_pressed(false);

				return;
			}

			const position visual_min = el->visual_pos();
			const position visual_max = { visual_min.x + el->computed_size().x, visual_min.y + el->computed_size().y };
			const bool hovering = mouse_pos.x >= visual_min.x && mouse_pos.x <= visual_max.x &&
			                      mouse_pos.y >= visual_min.y && mouse_pos.y <= visual_max.y;

			// children are walked first (topmost-first), so the first hovered element
			// with a tooltip is the deepest/topmost one under the cursor.
			if (hovering && !tooltip_hover && el->has_tooltip())
			{
				tooltip_hover = el.get();
			}

			// likewise the deepest hovered element that requests a cursor wins.
			if (hovering && !cursor_hover && el->style().cursor)
			{
				cursor_hover = el.get();
			}

			if (!click_handled && mouse_clicked && hovering)
			{
				click_handled = el->on_mouse_click();

				if (click_handled)
				{
					clicked = el.get();
				}
			}

			if (!enter_handled && hovering && !el->is_hovered())
			{
				el->set_hovered(true);
				enter_handled = el->on_mouse_enter();
			}

			if (!hovering && el->is_hovered())
			{
				el->set_hovered(false);
				el->on_mouse_exit();
			}

			el->set_pressed(hovering && mouse_down);

			if (hovering && el->style().overflow.value_or(overflow_mode::visible) == overflow_mode::scroll)
			{
				if (!scroll_target)
				{
					scroll_target = el;
				}
			}
		}

		// Gathers every visible topmost element (popups) into out, in tree (DFS) order. Dispatched
		// before the rest of the tree so overlays win clicks/hover over whatever they cover.
		void collect_topmost(const shared_ptr_t<element>& el, vector_t<shared_ptr_t<element>>& out) const
		{
			if (!el->is_visible())
			{
				return;
			}

			for (const auto& child : el->children())
			{
				if (child->is_topmost())
				{
					out.push_back(child);
				}

				collect_topmost(child, out);
			}
		}

		void process_events()
		{
			if (!input_)
			{
				return;
			}

			const position mouse_pos = input_->mouse_pos();
			const bool mouse_clicked = input_->is_mouse_clicked(0);
			const bool mouse_down = input_->is_mouse_down(0);
			const float scroll_delta = input_->scroll_delta();

			bool click_handled = false;
			bool enter_handled = false;

			shared_ptr_t<element> scroll_target;
			element* clicked = nullptr;
			element* tooltip_hover = nullptr;
			element* cursor_hover = nullptr;

			if (tree_.root())
			{
				// topmost popups first (reverse DFS → most-recently-nested on top), then the rest
				// of the tree (which skips topmost subtrees). They share the handled flags, so a
				// popup claims the click/hover ahead of anything it visually covers.
				vector_t<shared_ptr_t<element>> topmost;
				collect_topmost(tree_.root(), topmost);

				// hit-test front-first: stable-sort by z ascending (preserving DFS order for ties),
				// then walk in reverse so the highest-z overlay (the front-most) claims clicks first.
				for (cstd::size_t i = 1; i < topmost.size(); ++i)
				{
					const shared_ptr_t<element> key = topmost[i];
					const cstd::int32_t kz = key->z_index();
					cstd::size_t j = i;

					while (j > 0 && topmost[j - 1]->z_index() > kz)
					{
						topmost[j] = topmost[j - 1];
						--j;
					}

					topmost[j] = key;
				}

				for (auto it = topmost.rbegin(); it != topmost.rend(); ++it)
				{
					process_events_recursive(*it, mouse_pos, mouse_clicked, mouse_down, click_handled, enter_handled, scroll_target, clicked, tooltip_hover, cursor_hover, false);
				}

				process_events_recursive(tree_.root(), mouse_pos, mouse_clicked, mouse_down, click_handled, enter_handled, scroll_target, clicked, tooltip_hover, cursor_hover, false);
			}

			// remember the cursor the deepest hovered element requested (arrow otherwise). it's applied
			// at the top of the next render() as a baseline, so widgets that set a dynamic cursor in
			// update_all (panel resize edges, text caret) override it rather than being clobbered.
			frame_cursor_ = (cursor_hover && cursor_hover->style().cursor) ? *cursor_hover->style().cursor : cursor_type::arrow;

			tooltip_hover_ = tooltip_hover;

			// focus only changes on a click, so it persists across frames otherwise.
			// the consuming element keeps focus if focusable; everyone else is cleared
			// (clicking a button or empty space defocuses).
			if (mouse_clicked)
			{
				// focus is singular, so clear only the previously-focused element instead of
				// scanning the whole registry every click. weak_ptr covers the case where that
				// element was removed from the tree since it took focus.
				const bool keep = clicked && clicked->focusable();

				if (const auto prev = focused_el_.lock(); prev && (!keep || prev.get() != clicked))
				{
					prev->set_focused(false);
				}

				if (keep)
				{
					clicked->set_focused(true);
					focused_el_ = clicked->shared_from_this();
				}
				else
				{
					focused_el_.reset();
				}
			}

			// apply scroll to deepest scrollable
			if (scroll_target && scroll_delta != 0.f)
			{
				scroll_target->apply_scroll(scroll_delta, default_style_);
			}
		}

		// Floating hint box near the cursor. Supports embedded newlines and clamps to the
		// display so it never spills off-screen.
		void draw_tooltip(const string_t& text, const position mouse, const vector_2d<float> display_size) const
		{
			if (text.empty() || !font_)
			{
				return;
			}

			const float baked = font_->baked_size();
			const float scale = (tooltip_font_size > 0.f && baked > 0.f) ? tooltip_font_size / baked : 1.f;
			const float line_h = font_->line_height() * scale;

			// measure the widest line and count lines in one pass
			float max_w = 0.f;
			float cur_w = 0.f;
			cstd::int32_t line_count = 1;
			cstd::uint32_t prev = 0;

			const char* s = text.data();
			const char* const end = s + text.size();

			while (s < end)
			{
				const cstd::uint32_t cp = decode_utf8(s, end);

				if (cp == U'\n')
				{
					max_w = cstd::fmaxf(max_w, cur_w);
					cur_w = 0.f;
					prev = 0;
					++line_count;

					continue;
				}

				float adv = font_->glyph_advance(cp) * scale;

				if (prev != 0)
				{
					adv += font_->kerning(prev, cp) * scale;
				}

				cur_w += adv;
				prev = cp;
			}

			max_w = cstd::fmaxf(max_w, cur_w);

			constexpr float pad_x = 8.f;
			constexpr float pad_y = 5.f;
			const float box_w = max_w + pad_x * 2.f;
			const float box_h = line_h * static_cast<float>(line_count) + pad_y * 2.f;

			position min = { mouse.x + 14.f, mouse.y + 18.f };

			if (min.x + box_w > display_size.x)
			{
				min.x = cstd::fmaxf(0.f, display_size.x - box_w);
			}

			if (min.y + box_h > display_size.y)
			{
				min.y = cstd::fmaxf(0.f, mouse.y - box_h - 4.f);
			}

			const position max = { min.x + box_w, min.y + box_h };

			renderer_->draw_shadow_rect(min, max, color{ 0.f, 0.f, 0.f, 0.45f }, 4.f, 10.f, 0.f, rounding_flags_all, false);
			renderer_->draw_rect_filled(min, max, color{ 0.1f, 0.1f, 0.12f, 0.97f }, 4.f, rounding_flags_all);
			renderer_->draw_rect(min, max, color{ 0.32f, 0.32f, 0.38f, 1.f }, 1.f, 4.f);

			// draw each line, splitting on newlines
			position pen = { min.x + pad_x, min.y + pad_y };
			cstd::size_t line_start = 0;

			for (cstd::size_t i = 0; i <= text.size(); ++i)
			{
				if (i == text.size() || text[i] == '\n')
				{
					const string_view_t line{ text.data() + line_start, i - line_start };

					if (!line.empty())
					{
						renderer_->draw_text(*font_, pen, line, color{ 0.95f, 0.95f, 0.97f, 1.f }, tooltip_font_size);
					}

					pen.y += line_h;
					line_start = i + 1;
				}
			}
		}

		static constexpr float tooltip_delay = 0.4f;
		static constexpr float tooltip_font_size = 14.f;

		unique_ptr_t<gui_renderer> renderer_;
		shared_ptr_t<input> input_;
		element_tree tree_;
		element_style default_style_;

		shared_ptr_t<gui_font> font_;
		element* tooltip_target_ = nullptr;
		element* tooltip_hover_ = nullptr;
		weak_ptr_t<element> focused_el_;
		float tooltip_timer_ = 0.f;
		cursor_type frame_cursor_ = cursor_type::arrow;
		cstd::int32_t next_panel_z_ = z_index_panel;

		shared_ptr_t<portrait_background_element> portrait_background_;
		shared_ptr_t<element> portrait_target_;

		bool layout_dirty_ = true;
		vector_2d<float> last_display_size_ = { };
		gui_profile profile_ = { };
	};

	inline bool portrait_background_element::on_mouse_click()
	{
		if (const auto g = gui_.lock())
		{
			g->hide_portrait_focus();
		}

		return false;
	}

	// the gui directly, so the add_* factories can reach the gui through their weak back-pointer.
	[[nodiscard]] inline shared_ptr_t<gui> make_gui(unique_ptr_t<gui_renderer> renderer, shared_ptr_t<input> input)
	{
		auto g = cstd::make_shared<gui>(cstd::move(renderer), cstd::move(input));
		g->attach(g);

		return g;
	}
}
