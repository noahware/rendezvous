#pragma once
#include "types.hpp"

namespace rv
{
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

	inline cstd::uint32_t decode_utf8(const char*& s, const char* end) noexcept
	{
		const cstd::uint8_t c0 = static_cast<cstd::uint8_t>(*s);

		if (c0 < 0x80)
		{
			s++;
			return c0;
		}
		else if ((c0 & 0xE0) == 0xC0 && s + 1 < end)
		{
			const cstd::uint32_t cp = ((c0 & 0x1F) << 6)
				| (static_cast<cstd::uint8_t>(s[1]) & 0x3F);
			s += 2;
			return cp;
		}
		else if ((c0 & 0xF0) == 0xE0 && s + 2 < end)
		{
			const cstd::uint32_t cp = ((c0 & 0x0F) << 12)
				| ((static_cast<cstd::uint8_t>(s[1]) & 0x3F) << 6)
				| (static_cast<cstd::uint8_t>(s[2]) & 0x3F);
			s += 3;
			return cp;
		}
		else if ((c0 & 0xF8) == 0xF0 && s + 3 < end)
		{
			const cstd::uint32_t cp = ((c0 & 0x07) << 18)
				| ((static_cast<cstd::uint8_t>(s[1]) & 0x3F) << 12)
				| ((static_cast<cstd::uint8_t>(s[2]) & 0x3F) << 6)
				| (static_cast<cstd::uint8_t>(s[3]) & 0x3F);
			s += 4;
			return cp;
		}

		s++;
		return '?';
	}
}
