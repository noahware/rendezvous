#pragma once
#include "../util/hash.hpp"
#include "../util/types.hpp"

namespace rv
{
	class element
	{
	public:
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

	protected:
		shared_ptr_t<element> parent_;
		vector_t<shared_ptr_t<element>> children_;
	};

	class element_tree
	{
	public:
		using hash_type = cstd::size_t;

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

	protected:
		unordered_map_t<hash_type, shared_ptr_t<element>> elements_;
	};
}
