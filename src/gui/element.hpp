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

			// remove finished animations with fill_mode::none
			animations_.erase(
				std::remove_if(animations_.begin(), animations_.end(), [](const animation_state& a)
				{
					return a.is_finished() && a.get_fill_mode() == fill_mode::none;
				}),
				animations_.end()
			);
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

		[[nodiscard]] bool child_of(const shared_ptr_t<element>& parent) const noexcept
		{
			return parent_ == parent;
		}

		[[nodiscard]] shared_ptr_t<element> parent() const noexcept
		{
			return parent_;
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

		[[nodiscard]] vector_2d<float> size() const noexcept
		{
			return computed_size_;
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
		bool visible_ = true;
		vector_t<animation_state> animations_;

		vector_t<flex_line> flex_lines_;
		position scroll_offset_ = { 0.f, 0.f };

		shared_ptr_t<element> parent_ = { };
		vector_t<shared_ptr_t<element>> children_ = { };
	};

	// forward declaration for recursive call from position_line_children
	inline void resolve_positions(element& el, const position cursor, const element_style& defaults);

	// helper: compute justify offsets for a line of children
	inline void compute_justify(const justify_content jc, const float remaining,
	                            const cstd::size_t count, const float base_gap,
	                            float& out_offset, float& out_gap) noexcept
	{
		out_offset = 0.f;
		out_gap = base_gap;

		switch (jc)
		{
		case justify_content::start:
			break;
		case justify_content::center:
			out_offset = remaining * 0.5f;
			break;
		case justify_content::end:
			out_offset = remaining;
			break;
		case justify_content::space_between:
			if (count > 1)
			{
				out_gap = base_gap + remaining / static_cast<float>(count - 1);
			}
			break;
		case justify_content::space_around:
			if (count > 0)
			{
				const float space = remaining / static_cast<float>(count);
				out_offset = space * 0.5f;
				out_gap = base_gap + space;
			}
			break;
		case justify_content::space_evenly:
			if (count > 0)
			{
				const float space = remaining / static_cast<float>(count + 1);
				out_offset = space;
				out_gap = base_gap + space;
			}
			break;
		}
	}

	// helper: position children along a single line
	inline void position_line_children(span_t<const shared_ptr_t<element>> children,
	                                   const bool effective_reversed,
	                                   const bool vertical,
	                                   const float available_main,
	                                   const float line_cross_size,
	                                   const justify_content jc,
	                                   const alignment al,
	                                   const float gap,
	                                   const position base_cursor,
	                                   const float cross_cursor_offset,
	                                   const element_style& defaults)
	{
		// compute main-axis usage
		float total_main_used = 0.f;
		const cstd::size_t count = children.size();

		for (const auto& child : children)
		{
			const auto cm = child->style().margin.value_or(
				defaults.margin.value_or(border_vector{})
			);

			total_main_used += vertical
				? child->computed_size().y + cm.top + cm.bottom
				: child->computed_size().x + cm.left + cm.right;
		}

		if (count > 1)
		{
			total_main_used += gap * static_cast<float>(count - 1);
		}

		const float remaining = cstd::fmaxf(0.f, available_main - total_main_used);

		float main_offset = 0.f;
		float effective_gap = gap;
		compute_justify(jc, remaining, count, gap, main_offset, effective_gap);

		// build iteration order
		vector_t<cstd::size_t> order;
		order.reserve(count);

		for (cstd::size_t i = 0; i < count; ++i)
		{
			order.push_back(effective_reversed ? (count - 1 - i) : i);
		}

		// position children
		float main_cursor = main_offset;
		bool first = true;

		for (const auto idx : order)
		{
			const auto& child = children[idx];

			if (!first)
			{
				main_cursor += effective_gap;
			}

			first = false;

			const auto child_margin = child->style().margin.value_or(
				defaults.margin.value_or(border_vector{})
			);

			const float child_cross = vertical
				? child->computed_size().x
				: child->computed_size().y;

			const float cross_margins = vertical
				? child_margin.left + child_margin.right
				: child_margin.top + child_margin.bottom;

			// cross-axis alignment within line
			const auto child_al = child->style().align_self.value_or(al);
			float cross_offset = 0.f;

			switch (child_al)
			{
			case alignment::start:
				break;
			case alignment::center:
				cross_offset = (line_cross_size - child_cross - cross_margins) * 0.5f;
				break;
			case alignment::end:
				cross_offset = line_cross_size - child_cross - cross_margins;
				break;
			case alignment::stretch:
				break;
			}

			position child_pos = base_cursor;

			if (vertical)
			{
				child_pos.y += main_cursor + child_margin.top;
				child_pos.x += cross_cursor_offset + cross_offset + child_margin.left;
			}
			else
			{
				child_pos.x += main_cursor + child_margin.left;
				child_pos.y += cross_cursor_offset + cross_offset + child_margin.top;
			}

			resolve_positions(*child, child_pos, defaults);

			// advance main cursor
			main_cursor += vertical
				? child->computed_size().y + child_margin.top + child_margin.bottom
				: child->computed_size().x + child_margin.left + child_margin.right;
		}
	}

	inline void resolve_positions(element& el, const position cursor, const element_style& defaults)
	{
		el.set_computed_pos(cursor);

		if (!el.is_visible())
		{
			return;
		}

		const auto& style = el.style();
		const auto dir = style.direction.value_or(
			defaults.direction.value_or(layout_direction::horizontal)
		);
		const bool vertical = rv::is_vertical(dir);
		const bool reversed = rv::is_reversed(dir);

		// RTL flips horizontal to horizontal_reverse
		const auto tdir = style.dir.value_or(defaults.dir.value_or(text_direction::ltr));
		const bool effective_reversed = (tdir == text_direction::rtl && !vertical) ? !reversed : reversed;

		const auto al = style.align.value_or(defaults.align.value_or(alignment::start));
		const auto jc = style.justify.value_or(defaults.justify.value_or(justify_content::start));
		const float gap = resolve_gap(style, defaults, vertical);
		const float cross_gap = resolve_gap(style, defaults, !vertical);

		// compute insets (padding + border)
		const auto insets = compute_insets(style, defaults);

		const float available_main = (vertical ? el.computed_size().y : el.computed_size().x)
			- (vertical ? insets.top + insets.bottom : insets.left + insets.right);
		const float available_cross = (vertical ? el.computed_size().x : el.computed_size().y)
			- (vertical ? insets.left + insets.right : insets.top + insets.bottom);

		// collect visible flow children (skip absolute) — in original order
		vector_t<shared_ptr_t<element>> flow_children;
		vector_t<shared_ptr_t<element>> abs_children;

		for (const auto& child : el.children())
		{
			if (!child->is_visible())
			{
				continue;
			}

			const auto child_pos_type = child->style().position.value_or(position_type::relative);

			if (child_pos_type == position_type::absolute)
			{
				abs_children.push_back(child);
			}
			else
			{
				flow_children.push_back(child);
			}
		}

		// base position = cursor + insets
		const position base = { cursor.x + insets.left, cursor.y + insets.top };

		const auto& flex_lines_ref = el.flex_lines();
		const auto wrap_v = style.wrap.value_or(defaults.wrap.value_or(wrap_mode::no_wrap));
		const bool multi_line = wrap_v != wrap_mode::no_wrap && flex_lines_ref.size() > 1;

		if (multi_line)
		{
			// ── multi-line wrap positioning ──
			const cstd::size_t line_count = flex_lines_ref.size();

			// compute total lines cross size
			float total_lines_cross = 0.f;

			for (const auto& line : flex_lines_ref)
			{
				total_lines_cross += line.cross_size;
			}

			if (line_count > 1)
			{
				total_lines_cross += cross_gap * static_cast<float>(line_count - 1);
			}

			const float cross_remaining = cstd::fmaxf(0.f, available_cross - total_lines_cross);

			// align-content distribution
			const auto ac = style.align_content_v.value_or(
				defaults.align_content_v.value_or(align_content::flex_start)
			);

			float cross_start_offset = 0.f;
			float effective_cross_gap = cross_gap;

			switch (ac)
			{
			case align_content::flex_start:
				break;
			case align_content::flex_end:
				cross_start_offset = cross_remaining;
				break;
			case align_content::center:
				cross_start_offset = cross_remaining * 0.5f;
				break;
			case align_content::stretch:
				// stretch already handled in layout()
				break;
			case align_content::space_between:
				if (line_count > 1)
				{
					effective_cross_gap = cross_gap + cross_remaining / static_cast<float>(line_count - 1);
				}
				break;
			case align_content::space_around:
				if (line_count > 0)
				{
					const float space = cross_remaining / static_cast<float>(line_count);
					cross_start_offset = space * 0.5f;
					effective_cross_gap = cross_gap + space;
				}
				break;
			case align_content::space_evenly:
				if (line_count > 0)
				{
					const float space = cross_remaining / static_cast<float>(line_count + 1);
					cross_start_offset = space;
					effective_cross_gap = cross_gap + space;
				}
				break;
			}

			const bool wrap_reversed = wrap_v == wrap_mode::wrap_reverse;

			// build line order
			vector_t<cstd::size_t> line_order;
			line_order.reserve(line_count);

			for (cstd::size_t i = 0; i < line_count; ++i)
			{
				line_order.push_back(wrap_reversed ? (line_count - 1 - i) : i);
			}

			// position each line
			float cross_cursor = cross_start_offset;

			for (cstd::size_t li = 0; li < line_order.size(); ++li)
			{
				if (li > 0)
				{
					cross_cursor += effective_cross_gap;
				}

				const auto& line = flex_lines_ref[line_order[li]];

				// get children for this line from flow_children using line indices
				// (flex_lines store indices into the original flow order)
				vector_t<shared_ptr_t<element>> line_children;

				for (cstd::size_t i = line.start_index; i < line.start_index + line.count; ++i)
				{
					if (i < flow_children.size())
					{
						line_children.push_back(flow_children[i]);
					}
				}

				position_line_children(
					line_children,
					effective_reversed, vertical,
					available_main, line.cross_size,
					jc, al, gap,
					base, cross_cursor,
					defaults
				);

				cross_cursor += line.cross_size;
			}
		}
		else
		{
			// ── single-line positioning (original path) ──

			// if reversed, reverse flow children order
			if (effective_reversed)
			{
				std::reverse(flow_children.begin(), flow_children.end());
			}

			// count visible flow children and compute total main-axis usage
			float total_main = 0.f;
			const cstd::size_t visible_count = flow_children.size();

			for (const auto& child : flow_children)
			{
				const auto child_margin = child->style().margin.value_or(
					defaults.margin.value_or(border_vector{})
				);

				const float child_main = vertical
					? child->computed_size().y + child_margin.top + child_margin.bottom
					: child->computed_size().x + child_margin.left + child_margin.right;

				total_main += child_main;
			}

			if (visible_count > 1)
			{
				total_main += gap * static_cast<float>(visible_count - 1);
			}

			const float remaining = cstd::fmaxf(0.f, available_main - total_main);

			// justify: compute main-axis offset and effective gap
			float main_offset = 0.f;
			float effective_gap = gap;
			compute_justify(jc, remaining, visible_count, gap, main_offset, effective_gap);

			position child_cursor = base;

			if (vertical)
			{
				child_cursor.y += main_offset;
			}
			else
			{
				child_cursor.x += main_offset;
			}

			bool first_visible = true;

			for (const auto& child : flow_children)
			{
				if (!first_visible)
				{
					if (vertical)
					{
						child_cursor.y += effective_gap;
					}
					else
					{
						child_cursor.x += effective_gap;
					}
				}

				first_visible = false;

				const auto child_margin = child->style().margin.value_or(
					defaults.margin.value_or(border_vector{})
				);

				// apply margin offset
				position child_pos = child_cursor;
				child_pos.x += child_margin.left;
				child_pos.y += child_margin.top;

				// alignment: cross-axis offset (with align_self override)
				const auto child_al = child->style().align_self.value_or(al);

				const float child_cross = vertical
					? child->computed_size().x
					: child->computed_size().y;

				const float cross_margins = vertical
					? child_margin.left + child_margin.right
					: child_margin.top + child_margin.bottom;

				float cross_offset = 0.f;

				switch (child_al)
				{
				case alignment::start:
					break;
				case alignment::center:
					cross_offset = (available_cross - child_cross - cross_margins) * 0.5f;
					break;
				case alignment::end:
					cross_offset = available_cross - child_cross - cross_margins;
					break;
				case alignment::stretch:
					// stretch is handled in layout() — here just treat as start
					break;
				}

				if (vertical)
				{
					child_pos.x += cross_offset;
				}
				else
				{
					child_pos.y += cross_offset;
				}

				resolve_positions(*child, child_pos, defaults);

				// advance cursor along main axis
				if (vertical)
				{
					child_cursor.y += child->computed_size().y + child_margin.top + child_margin.bottom;
				}
				else
				{
					child_cursor.x += child->computed_size().x + child_margin.left + child_margin.right;
				}
			}
		}

		// position absolute children
		for (const auto& child : abs_children)
		{
			const auto child_margin = child->style().margin.value_or(
				defaults.margin.value_or(border_vector{})
			);
			const auto& cs = child->style();

			const auto resolve_inset = [](const optional_t<styled_size>& sv, const float avail) -> optional_t<float>
			{
				if (!sv.has_value())
				{
					return {};
				}

				switch (sv->mode)
				{
				case size_mode::px:      return sv->value;
				case size_mode::percent: return avail * (sv->value / 100.f);
				default:                 return {};
				}
			};

			const float content_w = el.computed_size().x - insets.left - insets.right;
			const float content_h = el.computed_size().y - insets.top - insets.bottom;

			const auto left_v = resolve_inset(cs.inset_left, content_w);
			const auto right_v = resolve_inset(cs.inset_right, content_w);
			const auto top_v = resolve_inset(cs.inset_top, content_h);
			const auto bottom_v = resolve_inset(cs.inset_bottom, content_h);

			float child_x = cursor.x + insets.left;
			float child_y = cursor.y + insets.top;

			if (left_v.has_value())
			{
				child_x += *left_v + child_margin.left;
			}
			else if (right_v.has_value())
			{
				child_x += content_w - *right_v - child->computed_size().x - child_margin.right;
			}
			else
			{
				child_x += child_margin.left;
			}

			if (top_v.has_value())
			{
				child_y += *top_v + child_margin.top;
			}
			else if (bottom_v.has_value())
			{
				child_y += content_h - *bottom_v - child->computed_size().y - child_margin.bottom;
			}
			else
			{
				child_y += child_margin.top;
			}

			resolve_positions(*child, position{ child_x, child_y }, defaults);
		}
	}

	inline void update_all(element& el, const float dt)
	{
		if (!el.is_visible())
		{
			return;
		}

		el.update(dt);

		for (const auto& child : el.children())
		{
			update_all(*child, dt);
		}
	}

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
