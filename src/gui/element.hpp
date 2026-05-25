#pragma once
#include "../render/position.hpp"
#include "../util/hash.hpp"
#include "../util/types.hpp"
#include "styled_size.hpp"
#include "animation.hpp"

namespace rv
{
	class gui_renderer;

	template <class T, class ...Args>
	[[nodiscard]] shared_ptr_t<T> make_element(Args&&... args)
	{
		return cstd::make_shared<T>(args...);
	}

	enum class layout_direction : cstd::uint8_t
	{
		vertical,
		horizontal,
		vertical_reverse,
		horizontal_reverse
	};

	[[nodiscard]] inline bool is_vertical(const layout_direction d) noexcept
	{
		return d == layout_direction::vertical || d == layout_direction::vertical_reverse;
	}

	[[nodiscard]] inline bool is_reversed(const layout_direction d) noexcept
	{
		return d == layout_direction::vertical_reverse || d == layout_direction::horizontal_reverse;
	}

	enum class alignment : cstd::uint8_t
	{
		start,
		center,
		end,
		stretch
	};

	enum class justify_content : cstd::uint8_t
	{
		start,
		center,
		end,
		space_between,
		space_around,
		space_evenly
	};

	enum class position_type : cstd::uint8_t
	{
		relative,
		absolute,
		static_v
	};

	enum class wrap_mode : cstd::uint8_t
	{
		no_wrap,
		wrap,
		wrap_reverse
	};

	enum class align_content : cstd::uint8_t
	{
		flex_start,
		flex_end,
		center,
		stretch,
		space_between,
		space_around,
		space_evenly
	};

	enum class overflow_mode : cstd::uint8_t
	{
		visible,
		hidden,
		scroll
	};

	enum class text_align : cstd::uint8_t
	{
		left,
		center,
		right
	};

	enum class text_direction : cstd::uint8_t
	{
		ltr,
		rtl
	};

	struct border_vector
	{
		float top = 0.f;
		float right = 0.f;
		float bottom = 0.f;
		float left = 0.f;
	};

	struct flex_line
	{
		cstd::size_t start_index = 0;
		cstd::size_t count = 0;
		float main_size = 0.f;
		float cross_size = 0.f;
	};

	struct element_style
	{
		element_size size;
		optional_t<float> gap;
		optional_t<float> row_gap;
		optional_t<float> column_gap;
		optional_t<layout_direction> direction;
		optional_t<alignment> align;
		optional_t<alignment> align_self;
		optional_t<justify_content> justify;
		optional_t<border_vector> margin;
		optional_t<border_vector> padding;
		optional_t<border_vector> border_width;
		optional_t<styled_size> min_width;
		optional_t<styled_size> max_width;
		optional_t<styled_size> min_height;
		optional_t<styled_size> max_height;
		optional_t<float> aspect_ratio;
		optional_t<float> flex_grow;
		optional_t<float> flex_shrink;
		optional_t<styled_size> flex_basis;
		optional_t<position_type> position;
		optional_t<styled_size> inset_top;
		optional_t<styled_size> inset_right;
		optional_t<styled_size> inset_bottom;
		optional_t<styled_size> inset_left;
		optional_t<wrap_mode> wrap;
		optional_t<align_content> align_content_v;
		optional_t<overflow_mode> overflow;
		optional_t<text_direction> dir;
		optional_t<color> background_color;
		optional_t<color> text_color;
		optional_t<float> rounding;
		optional_t<color> border_color;
		optional_t<float> font_size;
		optional_t<text_align> text_alignment;
		optional_t<float> transition_speed;
	};

	// helper to resolve gap for the correct axis
	[[nodiscard]] inline float resolve_gap(const element_style& style, const element_style& defaults,
	                                        const bool vertical) noexcept
	{
		if (vertical)
		{
			return style.row_gap.value_or(
				defaults.row_gap.value_or(
					style.gap.value_or(defaults.gap.value_or(0.f))
				)
			);
		}

		return style.column_gap.value_or(
			defaults.column_gap.value_or(
				style.gap.value_or(defaults.gap.value_or(0.f))
			)
		);
	}

	// helper to compute total insets (padding + border_width)
	[[nodiscard]] inline border_vector compute_insets(const element_style& style,
	                                                  const element_style& defaults) noexcept
	{
		const auto pad = style.padding.value_or(defaults.padding.value_or(border_vector{}));
		const auto brd = style.border_width.value_or(defaults.border_width.value_or(border_vector{}));

		return {
			pad.top + brd.top,
			pad.right + brd.right,
			pad.bottom + brd.bottom,
			pad.left + brd.left
		};
	}

	class element
	{
	public:
		virtual ~element() = default;
		element() noexcept = default;

		explicit element(const element_size size) noexcept
				:	style_{ .size = size } { }

		virtual void update(float dt)
		{
			for (auto& anim : animations_)
			{
				anim.update(dt);
			}

			animations_.erase(
				std::remove_if(animations_.begin(), animations_.end(), [](const animation_state& a)
				{
					return a.is_finished() && a.get_fill_mode() == fill_mode::none;
				}),
				animations_.end()
			);

			if (style_.background_color || style_.text_color || style_.rounding || style_.border_color)
			{
				const color target_bg = style_.background_color.value_or(color{ 0.f, 0.f, 0.f, 0.f });
				const color target_tc = style_.text_color.value_or(color{ 1.f, 1.f, 1.f, 1.f });
				const float target_rd = style_.rounding.value_or(0.f);
				const color target_bc = style_.border_color.value_or(color{ 0.f, 0.f, 0.f, 0.f });

				const float tspeed = style_.transition_speed.value_or(0.f);

				if (!transitions_initialized_ || tspeed <= 0.f)
				{
					visual_bg_ = target_bg;
					visual_text_color_ = target_tc;
					visual_rounding_ = target_rd;
					visual_border_color_ = target_bc;
					transitions_initialized_ = true;
				}
				else
				{
					const float factor = cstd::fminf(tspeed * dt, 1.f);
					visual_bg_ = lerp_color(visual_bg_, target_bg, factor);
					visual_text_color_ = lerp_color(visual_text_color_, target_tc, factor);
					visual_rounding_ = lerp(visual_rounding_, target_rd, factor);
					visual_border_color_ = lerp_color(visual_border_color_, target_bc, factor);

					auto close = [](const float a, const float b) noexcept
					{
						return cstd::fabsf(a - b) < 0.001f;
					};

					auto color_close = [&](const color& a, const color& b) noexcept
					{
						return close(a.r, b.r) && close(a.g, b.g) && close(a.b, b.b) && close(a.a, b.a);
					};

					if (color_close(visual_bg_, target_bg))
					{
						visual_bg_ = target_bg;
					}

					if (color_close(visual_text_color_, target_tc))
					{
						visual_text_color_ = target_tc;
					}

					if (close(visual_rounding_, target_rd))
					{
						visual_rounding_ = target_rd;
					}

					if (color_close(visual_border_color_, target_bc))
					{
						visual_border_color_ = target_bc;
					}
				}
			}
		}

		virtual bool on_mouse_click()
		{
			return false;
		}

		virtual bool on_mouse_enter()
		{
			return false;
		}

		virtual bool on_mouse_exit()
		{
			return false;
		}

		[[nodiscard]] span_t<shared_ptr_t<element>> children() noexcept
		{
			return children_;
		}

		[[nodiscard]] span_t<const shared_ptr_t<element>> children() const noexcept
		{
			return children_;
		}

		void add_child(shared_ptr_t<element> child)
		{
			children_.push_back(cstd::move(child));
		}

		template <class T, class ...Args>
		shared_ptr_t<T> make_child(Args&&... args)
		{
			auto child = make_element<T>(args...);

			children_.push_back(child);

			return child;
		}

		[[nodiscard]] const element_size& declared_size() const noexcept
		{
			return style_.size;
		}

		void set_declared_size(const element_size size) noexcept
		{
			style_.size = size;
		}

		[[nodiscard]] vector_2d<float> computed_size() const noexcept
		{
			return computed_size_;
		}

		void set_computed_size(const vector_2d<float> size) noexcept
		{
			computed_size_ = size;
		}

		[[nodiscard]] virtual vector_2d<float> content_size(const vector_2d<float> available) const noexcept
		{
			return { 0.f, 0.f };
		}

		[[nodiscard]] position computed_pos() const noexcept
		{
			return computed_pos_;
		}

		void set_computed_pos(const position pos) noexcept
		{
			computed_pos_ = pos;
		}

		[[nodiscard]] bool is_hovered() const noexcept
		{
			return hovered_;
		}

		void set_hovered(const bool hovered) noexcept
		{
			hovered_ = hovered;
		}

		[[nodiscard]] bool is_pressed() const noexcept
		{
			return pressed_;
		}

		void set_pressed(const bool pressed) noexcept
		{
			pressed_ = pressed;
		}

		[[nodiscard]] bool is_visible() const noexcept
		{
			return visible_;
		}

		void set_visible(const bool visible) noexcept
		{
			visible_ = visible;
		}

		[[nodiscard]] bool contains(const position point) const noexcept
		{
			return point.x >= computed_pos_.x
				&& point.y >= computed_pos_.y
				&& point.x <= computed_pos_.x + computed_size_.x
				&& point.y <= computed_pos_.y + computed_size_.y;
		}

		[[nodiscard]] element_style& style() noexcept
		{
			return style_;
		}

		[[nodiscard]] const element_style& style() const noexcept
		{
			return style_;
		}

		element& gap(const optional_t<float> gap) noexcept
		{
			style_.gap = gap;

			return *this;
		}

		element& direction(const layout_direction direction) noexcept
		{
			style_.direction = direction;

			return *this;
		}

		element& align(const alignment align) noexcept
		{
			style_.align = align;

			return *this;
		}

		element& justify(const justify_content justify) noexcept
		{
			style_.justify = justify;

			return *this;
		}

		element& margin(const border_vector margin) noexcept
		{
			style_.margin = margin;

			return *this;
		}

		element& margin(const float all) noexcept
		{
			style_.margin = border_vector{ all, all, all, all };

			return *this;
		}

		element& padding(const border_vector pad) noexcept
		{
			style_.padding = pad;

			return *this;
		}

		element& padding(const float all) noexcept
		{
			style_.padding = border_vector{ all, all, all, all };

			return *this;
		}

		element& border_width(const border_vector bw) noexcept
		{
			style_.border_width = bw;

			return *this;
		}

		element& border_width(const float all) noexcept
		{
			style_.border_width = border_vector{ all, all, all, all };

			return *this;
		}

		element& row_gap(const float g) noexcept
		{
			style_.row_gap = g;

			return *this;
		}

		element& column_gap(const float g) noexcept
		{
			style_.column_gap = g;

			return *this;
		}

		element& align_self(const alignment a) noexcept
		{
			style_.align_self = a;

			return *this;
		}

		element& min_width(const styled_size s) noexcept
		{
			style_.min_width = s;

			return *this;
		}

		element& max_width(const styled_size s) noexcept
		{
			style_.max_width = s;

			return *this;
		}

		element& min_height(const styled_size s) noexcept
		{
			style_.min_height = s;

			return *this;
		}

		element& max_height(const styled_size s) noexcept
		{
			style_.max_height = s;

			return *this;
		}

		element& aspect_ratio(const float r) noexcept
		{
			style_.aspect_ratio = r;

			return *this;
		}

		element& flex_grow(const float g) noexcept
		{
			style_.flex_grow = g;

			return *this;
		}

		element& flex_shrink(const float s) noexcept
		{
			style_.flex_shrink = s;

			return *this;
		}

		element& flex_basis(const styled_size b) noexcept
		{
			style_.flex_basis = b;

			return *this;
		}

		element& flex(const float grow, const float shrink, const styled_size basis) noexcept
		{
			style_.flex_grow = grow;
			style_.flex_shrink = shrink;
			style_.flex_basis = basis;

			return *this;
		}

		element& positioning(const position_type p) noexcept
		{
			style_.position = p;

			return *this;
		}

		element& inset_top(const styled_size s) noexcept
		{
			style_.inset_top = s;

			return *this;
		}

		element& inset_right(const styled_size s) noexcept
		{
			style_.inset_right = s;

			return *this;
		}

		element& inset_bottom(const styled_size s) noexcept
		{
			style_.inset_bottom = s;

			return *this;
		}

		element& inset_left(const styled_size s) noexcept
		{
			style_.inset_left = s;

			return *this;
		}

		element& wrap(const wrap_mode w) noexcept
		{
			style_.wrap = w;

			return *this;
		}

		element& align_content(const align_content ac) noexcept
		{
			style_.align_content_v = ac;

			return *this;
		}

		element& overflow(const overflow_mode o) noexcept
		{
			style_.overflow = o;

			return *this;
		}

		element& dir(const text_direction d) noexcept
		{
			style_.dir = d;

			return *this;
		}

		element& background_color(const color c) noexcept
		{
			style_.background_color = c;

			return *this;
		}

		element& text_color(const color c) noexcept
		{
			style_.text_color = c;

			return *this;
		}

		element& rounding(const float r) noexcept
		{
			style_.rounding = r;

			return *this;
		}

		element& border_color(const color c) noexcept
		{
			style_.border_color = c;

			return *this;
		}

		element& transition_speed(const float speed) noexcept
		{
			style_.transition_speed = speed;

			return *this;
		}

		element& text_size(const float size) noexcept
		{
			style_.font_size = size;

			return *this;
		}

		element& text_alignment(const text_align align) noexcept
		{
			style_.text_alignment = align;

			return *this;
		}

		void set_flex_lines(vector_t<flex_line> lines) noexcept
		{
			flex_lines_ = cstd::move(lines);
		}

		[[nodiscard]] span_t<const flex_line> flex_lines() const noexcept
		{
			return flex_lines_;
		}

		void apply_scroll(const float delta, const element_style& defaults) noexcept
		{
			const auto dir = style_.direction.value_or(
				defaults.direction.value_or(layout_direction::horizontal)
			);
			const bool vertical = rv::is_vertical(dir);

			// compute total content size along main axis (including gaps + margins)
			const auto insets = compute_insets(style_, defaults);
			const float gap = resolve_gap(style_, defaults, vertical);
			float content_size = 0.f;
			cstd::size_t visible_count = 0;

			for (const auto& child : children_)
			{
				if (!child->is_visible())
				{
					continue;
				}

				const auto child_margin = child->style().margin.value_or(
					defaults.margin.value_or(border_vector{}));

				const float child_main = vertical
					? (child->computed_size().y + child_margin.top + child_margin.bottom)
					: (child->computed_size().x + child_margin.left + child_margin.right);

				content_size += child_main;
				++visible_count;
			}

			// add gaps between visible children
			if (visible_count > 1)
			{
				content_size += gap * static_cast<float>(visible_count - 1);
			}

			// add insets (padding + border)
			const float total_insets = vertical
				? (insets.top + insets.bottom)
				: (insets.left + insets.right);
			content_size += total_insets;

			const float viewport = vertical ? computed_size_.y : computed_size_.x;
			const float max_scroll = cstd::fmaxf(0.f, content_size - viewport);
			const float scroll_speed = 30.f;

			if (vertical)
			{
				scroll_offset_.y += delta * scroll_speed;
				scroll_offset_.y = cstd::fmaxf(-max_scroll, cstd::fminf(0.f, scroll_offset_.y));
			}
			else
			{
				scroll_offset_.x += delta * scroll_speed;
				scroll_offset_.x = cstd::fmaxf(-max_scroll, cstd::fminf(0.f, scroll_offset_.x));
			}
		}

		element& visible(const bool visible) noexcept
		{
			visible_ = visible;

			return *this;
		}

		element& animate(keyframe_sequence seq, animation_options opts)
		{
			animations_.emplace_back(cstd::move(seq), cstd::move(opts));

			return *this;
		}

		element& stop_animations() noexcept
		{
			animations_.clear();

			return *this;
		}

		[[nodiscard]] optional_t<keyframe_props> animated_props() const noexcept
		{
			if (animations_.empty())
			{
				return {};
			}

			// merge all active animations - later ones win per property, offset accumulates
			keyframe_props merged;
			bool has_any = false;

			for (const auto& anim : animations_)
			{
				if (anim.is_finished() && anim.get_fill_mode() == fill_mode::none)
				{
					continue;
				}

				const auto props = anim.current_props();
				has_any = true;

				if (props.col)
				{
					merged.col = props.col;
				}

				if (props.opacity)
				{
					merged.opacity = props.opacity;
				}

				if (props.rounding)
				{
					merged.rounding = props.rounding;
				}

				// offset is additive - stacked slides combine
				if (props.offset)
				{
					if (merged.offset)
					{
						merged.offset->x += props.offset->x;
						merged.offset->y += props.offset->y;
					}
					else
					{
						merged.offset = props.offset;
					}
				}
			}

			if (!has_any)
			{
				return {};
			}

			return merged;
		}

		// defined in gui.hpp after gui_renderer is complete
		void render(gui_renderer& renderer, const element_style& defaults,
		            const position offset = { 0.f, 0.f }) const;

	protected:
		virtual void render_self(gui_renderer& renderer, position min, position max) const
		{

		}

		vector_2d<float> computed_size_ = { };
		position computed_pos_ = { };
		element_style style_;
		bool hovered_ = false;
		bool pressed_ = false;
		bool visible_ = true;
		vector_t<animation_state> animations_;

		color visual_bg_ = { 0.f, 0.f, 0.f, 0.f };
		color visual_text_color_ = { 1.f, 1.f, 1.f, 1.f };
		float visual_rounding_ = 0.f;
		color visual_border_color_ = { 0.f, 0.f, 0.f, 0.f };
		bool transitions_initialized_ = false;

		vector_t<flex_line> flex_lines_;
		position scroll_offset_ = { 0.f, 0.f };

		vector_t<shared_ptr_t<element>> children_ = { };
	};

	// implemented in elements.cpp
	void compute_justify(justify_content jc, float remaining,
	                     cstd::size_t count, float base_gap,
	                     float& out_offset, float& out_gap) noexcept;

	void position_line_children(span_t<const shared_ptr_t<element>> children,
	                            bool effective_reversed,
	                            bool vertical,
	                            float available_main,
	                            float line_cross_size,
	                            justify_content jc,
	                            alignment al,
	                            float gap,
	                            position base_cursor,
	                            float cross_cursor_offset,
	                            const element_style& defaults);

	void resolve_positions(element& el, position cursor, const element_style& defaults);

	void update_all(element& el, float dt);

	class element_tree
	{
	public:
		element_tree()
		{
			auto root = make_element<element>(element_size{ styled_size::fill(), styled_size::fill() });

			add(root);

			root_ = cstd::move(root);
		}

		using hash_type = cstd::size_t;

		void add(shared_ptr_t<element> element)
		{
			const hash_type id = id_++;

			add(id, cstd::move(element));
		}

		void add(const hash_type hash, shared_ptr_t<element> element)
		{
			elements_[hash] = cstd::move(element);
		}

		template <fixed_string S>
		void add(shared_ptr_t<element> element)
		{
			constexpr hash_type hash = rv::hash<string_view_t>{}(string_view_t{ S.data, S.size() });

			static_assert(hash != 0);

			add(hash, cstd::move(element));
		}

		template <class T, class ...Args>
		shared_ptr_t<T> make_child(const shared_ptr_t<element>& parent, Args&&... args)
		{
			auto child = make_element<T>(args...);

			parent->add_child(child);
			add(child);

			return child;
		}

		[[nodiscard]] shared_ptr_t<element> find(const hash_type hash) const noexcept
		{
			const auto it = elements_.find(hash);

			return it != elements_.end() ? it->second : nullptr;
		}

		[[nodiscard]] shared_ptr_t<element> root() const noexcept
		{
			return root_;
		}

		[[nodiscard]] auto begin() noexcept
		{
			return elements_.begin();
		}

		[[nodiscard]] auto end() noexcept
		{
			return elements_.end();
		}

		[[nodiscard]] auto begin() const noexcept
		{
			return elements_.begin();
		}

		[[nodiscard]] auto end() const noexcept
		{
			return elements_.end();
		}

	protected:
		hash_type id_ = 0;

		shared_ptr_t<element> root_;
		unordered_map_t<hash_type, shared_ptr_t<element>> elements_;
	};
}
