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

	inline void encode_utf8(const char32_t cp, string_t& out)
	{
		const cstd::uint32_t c = static_cast<cstd::uint32_t>(cp);

		if (c < 0x80)
		{
			out.push_back(static_cast<char>(c));
		}
		else if (c < 0x800)
		{
			out.push_back(static_cast<char>(0xC0 | (c >> 6)));
			out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
		}
		else if (c < 0x10000)
		{
			out.push_back(static_cast<char>(0xE0 | (c >> 12)));
			out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
			out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
		}
		else
		{
			out.push_back(static_cast<char>(0xF0 | (c >> 18)));
			out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
			out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
			out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
		}
	}

	struct glyph_range
	{
		cstd::uint32_t first = 0;
		cstd::uint32_t last = 0;
	};

	inline vector_t<glyph_range> make_glyph_ranges(const cstd::uint32_t first, const cstd::uint32_t last)
	{
		return { glyph_range{ first, last } };
	}

	class glyph_ranges_builder
	{
	public:
		glyph_ranges_builder& add_char(const cstd::uint32_t cp)
		{
			if (cp <= 0x10FFFF && (cp < 0xD800 || cp > 0xDFFF))
			{
				codepoints_.push_back(cp);
			}

			return *this;
		}

		glyph_ranges_builder& add_range(cstd::uint32_t first, cstd::uint32_t last)
		{
			if (first > last || first > 0x10FFFF)
			{
				return *this;
			}

			if (last > 0x10FFFF)
			{
				last = 0x10FFFF;
			}

			for (cstd::uint32_t cp = first; ; ++cp)
			{
				add_char(cp);

				if (cp == last)
				{
					break;
				}
			}

			return *this;
		}

		glyph_ranges_builder& add_range(const glyph_range range)
		{
			return add_range(range.first, range.last);
		}

		glyph_ranges_builder& add_text(const string_view_t text)
		{
			const char* s = text.data();
			const char* const end = s + text.size();

			while (s < end)
			{
				add_char(decode_utf8(s, end));
			}

			return *this;
		}

		glyph_ranges_builder& add_basic_latin()
		{
			return add_range(32, 126);
		}

		[[nodiscard]] vector_t<glyph_range> build() const
		{
			vector_t<cstd::uint32_t> cps = codepoints_;
			vector_t<glyph_range> ranges;

			if (cps.empty())
			{
				return ranges;
			}

			cstd::sort(cps.begin(), cps.end());
			cps.erase(cstd::unique(cps.begin(), cps.end()), cps.end());

			cstd::uint32_t first = cps[0];
			cstd::uint32_t last = cps[0];

			for (cstd::size_t i = 1; i < cps.size(); ++i)
			{
				const cstd::uint32_t cp = cps[i];

				if (cp == last + 1)
				{
					last = cp;
					continue;
				}

				ranges.push_back({ first, last });
				first = last = cp;
			}

			ranges.push_back({ first, last });
			return ranges;
		}

	private:
		vector_t<cstd::uint32_t> codepoints_;
	};
}
