#pragma once
#include "types.hpp"

namespace rv
{
	using hash_type = cstd::size_t;

	template <class T>
	constexpr static hash_type hash_bytes(const span_t<const T> input) noexcept
	{
		static_assert(sizeof(T) == sizeof(cstd::uint8_t), "T must be size of byte");

		constexpr hash_type basis = 0xCF29CE484222325;
		constexpr hash_type prime = 0x100000001B3;

		hash_type output = basis;

		for (const T i : input)
		{
			output ^= i;
			output *= prime;
		}

		return output;
	}

	template <class T>
	class hash
	{
	public:
		using size_type = size_t;

		[[nodiscard]] constexpr hash_type operator()(const T& value) const
		{
			using bytes_type = array_t<cstd::uint8_t, sizeof(T)>;
			const bytes_type bytes = cstd::bit_cast<bytes_type>(value);

			return hash_bytes(bytes);
		}
	};

	template <>
	class hash<string_view_t>
	{
	public:
		[[nodiscard]] constexpr hash_type operator()(const string_view_t& value) const
		{
			return hash_bytes<char>(span_t{ value.data(), value.size() });
		}
	};

	template <>
	class hash<string_t>
	{
	public:
		[[nodiscard]] hash_type operator()(const string_t& value) const
		{
			return hash_bytes(span_t{ reinterpret_cast<const cstd::uint8_t*>(value.data()), value.size() });
		}
	};

	template <cstd::size_t N>
	struct fixed_string
	{
		char data[N]{};

		constexpr fixed_string(const char(&s)[N]) noexcept
		{
			for (cstd::size_t i = 0; i < N; i++)
			{
				data[i] = s[i];
			}
		}

		[[nodiscard]] constexpr cstd::size_t size() const noexcept
		{
			return N - 1;
		}
	};

	template <cstd::size_t N>
	fixed_string(const char(&)[N]) -> fixed_string<N>;
}
