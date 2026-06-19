#pragma once
#include <fstream>
#include <string_view>
#include <string>
#include <numbers>
#include <vector>
#include <memory>
#include <cmath>
#include <array>
#include <optional>
#include <span>
#include <format>
#include <utility>
#include <chrono>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <cstring>
#include <bit>
#include <algorithm>

namespace cstd
{
	typedef signed char        int8_t;
	typedef short              int16_t;
	typedef int                int32_t;
	typedef long long          int64_t;
	typedef unsigned char      byte;
	typedef unsigned char      uint8_t;
	typedef unsigned short     uint16_t;
	typedef unsigned int       uint32_t;
	typedef unsigned long long uint64_t;

	typedef unsigned long long size_t;

	template <class T>
	inline constexpr bool is_floating_point_v = std::is_floating_point_v<T>;

	template <class T>
	inline constexpr bool is_integral_v = std::is_integral_v<T>;

	template <class T, class U>
	inline constexpr bool is_same_v = std::is_same_v<T, U>;
}

template <class T, cstd::size_t Count>
using array_t = std::array<T, Count>;

template <class T>
using vector_t = std::vector<T>;

template <class T>
using span_t = std::span<T>;

template <class T>
using shared_ptr_t = std::shared_ptr<T>;

template <class T>
using enable_shared_from_this_t = std::enable_shared_from_this<T>;

template <class T>
using weak_ptr_t = std::weak_ptr<T>;

template <class T>
using unique_ptr_t = std::unique_ptr<T>;

template <class T>
using optional_t = std::optional<T>;

template <class K, class V>
using unordered_map_t = std::unordered_map<K, V>;

template <class T, class Y>
using pair_t = std::pair<T, Y>;

template <class T>
using hash_t = std::hash<T>;

template <class T>
using function_t = std::function<T>;

template <class T>
using istreambuf_iterator_t = std::istreambuf_iterator<T>;

using ifstream_t = std::ifstream;
using time_point_t = std::chrono::time_point<std::chrono::high_resolution_clock>;

using string_t = std::string;
using wstring_t = std::wstring;

using string_view_t = std::string_view;
using wstring_view_t = std::wstring_view;

namespace cstd
{
	inline void memcpy(void* destination, const void* source, size_t size)
	{
		std::memcpy(destination, source, size);
	}

	// Text formatting. Wraps std::format so callers never touch std directly.
	template <class... Args>
	[[nodiscard]] inline string_t format(const std::format_string<Args...> fmt, Args&&... args)
	{
		return std::format(fmt, std::forward<Args>(args)...);
	}

	inline float sqrtf(const float x)
	{
		return std::sqrtf(x);
	}

	inline float cosf(const float x)
	{
		return std::cosf(x);
	}

	inline float sinf(const float x)
	{
		return std::sinf(x);
	}

	inline float atan2f(const float y, const float x)
	{
		return std::atan2f(y, x);
	}

	inline float fabsf(const float x)
	{
		return std::fabsf(x);
	}

	inline float fminf(const float x, const float y)
	{
		return std::fminf(x, y);
	}

	inline float fmaxf(const float x, const float y)
	{
		return std::fmaxf(x, y);
	}

	inline float floorf(const float x)
	{
		return std::floorf(x);
	}

	inline float roundf(const float x)
	{
		return std::roundf(x);
	}

	inline float ceilf(const float x)
	{
		return std::ceilf(x);
	}

	inline float log10f(const float x)
	{
		return std::log10f(x);
	}

	inline float powf(const float base, const float exp)
	{
		return std::powf(base, exp);
	}

	inline float fmodf(const float x, const float y)
	{
		return std::fmodf(x, y);
	}

	template <class It>
	void sort(It first, It last)
	{
		std::sort(first, last);
	}

	template <class It>
	It unique(It first, It last)
	{
		return std::unique(first, last);
	}

	template <class T, class Y>
	[[nodiscard]] constexpr T bit_cast(const Y& object) noexcept
	{
		return std::bit_cast<T>(object);
	}

	template <class T, class ...Args>
	unique_ptr_t<T> make_unique(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template <class T, class ...Args>
	shared_ptr_t<T> make_shared(Args&&... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template <typename T>
	constexpr std::remove_reference_t<T>&& move(T&& object) noexcept
	{
		return static_cast<std::remove_reference_t<T>&&>(object);
	}

	template <typename T>
	constexpr T&& forward(std::remove_reference_t<T>& object) noexcept
	{
		return static_cast<T&&>(object);
	}

	template <typename T>
	constexpr T&& forward(std::remove_reference_t<T>&& object) noexcept
	{
		return static_cast<T&&>(object);
	}

	namespace ios
	{
		inline constexpr auto binary = std::ios::binary;
	}

	namespace numbers
	{
		inline constexpr float pi_f = std::numbers::pi_v<float>;
	}

	inline time_point_t get_time()
	{
		return std::chrono::high_resolution_clock::now();
	}

	inline float get_time_diff(const time_point_t end, const time_point_t start)
	{
		return std::chrono::duration<float>(end - start).count();
	}
}
