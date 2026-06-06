#pragma once
#include "element.hpp"

namespace rv
{
	class element_tree
	{
	public:
		element_tree()
		{
			auto root = make_element<element>(element_size{ styled_size::fill(), styled_size::fill() });

			add(root);

			root_ = cstd::move(root);
		}

		void set_layout_dirty_ptr(bool* ptr) noexcept
		{
			layout_dirty_ptr_ = ptr;

			if (root_)
			{
				root_->set_layout_dirty_ptr(ptr);
			}
		}

		using hash_type = cstd::size_t;

		void add(shared_ptr_t<element> element)
		{
			const hash_type id = id_++;

			add(id, cstd::move(element));
		}

		void add(const hash_type hash, shared_ptr_t<element> element)
		{
			if (layout_dirty_ptr_)
			{
				element->set_layout_dirty_ptr(layout_dirty_ptr_);
			}

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
			auto child = make_element<T>(cstd::forward<Args>(args)...);

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
		bool* layout_dirty_ptr_ = nullptr;
	};
}
