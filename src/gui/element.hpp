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
		horizontal
	};

	enum class alignment : cstd::uint8_t
	{
		start,
		center,
		end
	};

	enum class justify_content : cstd::uint8_t
	{
		start,
		center,
		end,
		space_between,
		space_around
	};

	struct border_vector
	{
		float top = 0.f;
		float right = 0.f;
		float bottom = 0.f;
		float left = 0.f;
	};

	struct element_style
	{
		element_size size;
		optional_t<float> gap;
		optional_t<layout_direction> direction;
		optional_t<alignment> align;
		optional_t<justify_content> justify;
		optional_t<border_vector> margin;
	};

	class element
	{
	public:
		virtual ~element() = default;
		element() noexcept = default;

		explicit element(const element_size size) noexcept
				:	style_{ .size = size } { }

		virtual void update(float dt)
		{
			if (animation_)
			{
				animation_->update(dt);

				if (animation_->is_finished() && animation_->get_fill_mode() == fill_mode::none)
				{
					animation_.reset();
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

		element& visible(const bool visible) noexcept
		{
			visible_ = visible;

			return *this;
		}

		element& animate(keyframe_sequence seq, animation_options opts)
		{
			animation_.emplace(cstd::move(seq), cstd::move(opts));

			return *this;
		}

		element& stop_animation() noexcept
		{
			animation_.reset();

			return *this;
		}

		[[nodiscard]] optional_t<keyframe_props> animated_props() const noexcept
		{
			if (!animation_)
			{
				return {};
			}

			if (animation_->is_finished() && animation_->get_fill_mode() == fill_mode::none)
			{
				return {};
			}

			return animation_->current_props();
		}

		void render(gui_renderer& renderer, const element_style& defaults,
		            const position offset = { 0.f, 0.f }) const
		{
			if (!visible_)
			{
				return;
			}

			// accumulate this element's animation offset
			position total_offset = offset;
			const auto anim = animated_props();

			if (anim && anim->offset)
			{
				total_offset.x += anim->offset->x;
				total_offset.y += anim->offset->y;
			}

			const position min = { computed_pos_.x + total_offset.x, computed_pos_.y + total_offset.y };
			const position max = { min.x + computed_size_.x, min.y + computed_size_.y };
			render_self(renderer, min, max);

			for (const auto& child : children_)
			{
				child->render(renderer, defaults, total_offset);
			}
		}

	protected:
		virtual void render_self(gui_renderer& renderer, position min, position max) const
		{

		}

		vector_2d<float> computed_size_ = { };
		position computed_pos_ = { };
		element_style style_;
		bool hovered_ = false;
		bool visible_ = true;
		optional_t<animation_state> animation_;

		shared_ptr_t<element> parent_ = { };
		vector_t<shared_ptr_t<element>> children_ = { };
	};

	inline void resolve_positions(element& el, const position cursor, const element_style& defaults)
	{
		el.set_computed_pos(cursor);

		if (!el.is_visible())
		{
			return;
		}

		const auto& style = el.style();
		const float gap = style.gap.value_or(defaults.gap.value_or(0.f));
		const bool is_vertical = style.direction.value_or(
			defaults.direction.value_or(layout_direction::horizontal)
		) == layout_direction::vertical;

		const auto al = style.align.value_or(defaults.align.value_or(alignment::start));
		const auto jc = style.justify.value_or(defaults.justify.value_or(justify_content::start));

		const float available_main = is_vertical ? el.computed_size().y : el.computed_size().x;
		const float available_cross = is_vertical ? el.computed_size().x : el.computed_size().y;

		// count visible children and compute total main-axis usage
		float total_main = 0.f;
		cstd::size_t visible_count = 0;

		for (const auto& child : el.children())
		{
			if (!child->is_visible())
			{
				continue;
			}

			const auto child_margin = child->style().margin.value_or(
				defaults.margin.value_or(border_vector{})
			);

			const float child_main = is_vertical
				? child->computed_size().y + child_margin.top + child_margin.bottom
				: child->computed_size().x + child_margin.left + child_margin.right;

			total_main += child_main;
			visible_count++;
		}

		if (visible_count > 1)
		{
			total_main += gap * static_cast<float>(visible_count - 1);
		}

		const float remaining = cstd::fmaxf(0.f, available_main - total_main);

		// justify: compute main-axis offset and effective gap
		float main_offset = 0.f;
		float effective_gap = gap;

		switch (jc)
		{
		case justify_content::start:
			break;
		case justify_content::center:
			main_offset = remaining * 0.5f;
			break;
		case justify_content::end:
			main_offset = remaining;
			break;
		case justify_content::space_between:
			if (visible_count > 1)
			{
				effective_gap = gap + remaining / static_cast<float>(visible_count - 1);
			}
			break;
		case justify_content::space_around:
			if (visible_count > 0)
			{
				const float space = remaining / static_cast<float>(visible_count);
				main_offset = space * 0.5f;
				effective_gap = gap + space;
			}
			break;
		}

		position child_cursor = cursor;

		if (is_vertical)
		{
			child_cursor.y += main_offset;
		}
		else
		{
			child_cursor.x += main_offset;
		}

		bool first_visible = true;

		for (const auto& child : el.children())
		{
			if (!child->is_visible())
			{
				continue;
			}

			if (!first_visible)
			{
				if (is_vertical)
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

			// alignment: cross-axis offset
			const float child_cross = is_vertical
				? child->computed_size().x
				: child->computed_size().y;

			const float cross_margins = is_vertical
				? child_margin.left + child_margin.right
				: child_margin.top + child_margin.bottom;

			float cross_offset = 0.f;

			switch (al)
			{
			case alignment::start:
				break;
			case alignment::center:
				cross_offset = (available_cross - child_cross - cross_margins) * 0.5f;
				break;
			case alignment::end:
				cross_offset = available_cross - child_cross - cross_margins;
				break;
			}

			if (is_vertical)
			{
				child_pos.x += cross_offset;
			}
			else
			{
				child_pos.y += cross_offset;
			}

			resolve_positions(*child, child_pos, defaults);

			// advance cursor along main axis
			if (is_vertical)
			{
				child_cursor.y += child->computed_size().y + child_margin.top + child_margin.bottom;
			}
			else
			{
				child_cursor.x += child->computed_size().x + child_margin.left + child_margin.right;
			}
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
