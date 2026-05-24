#pragma once
#include "../render/position.hpp"
#include "../util/hash.hpp"
#include "../util/types.hpp"
#include "styled_size.hpp"

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

	struct element_style
	{
		optional_t<float> gap;
		layout_direction direction = layout_direction::horizontal;
	};

	class element
	{
	public:
		virtual ~element() = default;
		element() noexcept = default;

		explicit element(const element_size size) noexcept
				:	size_(size) { }

		virtual void on_mouse_click()
		{
			
		}

		virtual void on_mouse_enter()
		{

		}

		virtual void on_mouse_exit()
		{

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

		void add_child(shared_ptr_t<element> element)
		{
			children_.push_back(cstd::move(element));
		}

		template <class T, class ...Args>
		shared_ptr_t<element> make_child(Args&&... args)
		{
			auto element = make_element<T>(args...);

			children_.push_back(element);

			return element;
		}

		[[nodiscard]] vector_2d<float> size() const noexcept
		{
			return computed_size_;
		}

		[[nodiscard]] const element_size& declared_size() const noexcept
		{
			return size_;
		}

		void set_declared_size(const element_size size) noexcept
		{
			size_ = size;
		}

		[[nodiscard]] vector_2d<float> computed_size() const noexcept
		{
			return computed_size_;
		}

		void set_computed_size(const vector_2d<float> size) noexcept
		{
			computed_size_ = size;
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

		void render(gui_renderer& renderer, const struct position cursor) const
		{
			const struct position max = { cursor.x + computed_size_.x, cursor.y + computed_size_.y };
			render_self(renderer, cursor, max);

			const float gap = style_.gap.value_or(0.f);
			struct position child_cursor = cursor;

			for (const auto& child : children_)
			{
				child->render(renderer, child_cursor);

				if (style_.direction == layout_direction::vertical)
					child_cursor.y += child->computed_size_.y + gap;
				else
					child_cursor.x += child->computed_size_.x + gap;
			}
		}

	protected:
		virtual void render_self(gui_renderer& renderer, struct position min, struct position max) const
		{

		}

		element_size size_;
		vector_2d<float> computed_size_ = { };
		element_style style_;

		shared_ptr_t<element> parent_ = { };
		vector_t<shared_ptr_t<element>> children_ = { };
	};

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
