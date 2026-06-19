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

	inline glyph_range glyph_range_for_codepoint(const cstd::uint32_t cp) noexcept
	{
		if (cp >= 0x0020 && cp <= 0x007E) return { 0x0020, 0x007E };
		if (cp >= 0x00A0 && cp <= 0x00FF) return { 0x00A0, 0x00FF };
		if (cp >= 0x0100 && cp <= 0x017F) return { 0x0100, 0x017F };
		if (cp >= 0x0180 && cp <= 0x024F) return { 0x0180, 0x024F };
		if (cp >= 0x0300 && cp <= 0x036F) return { 0x0300, 0x036F };
		if (cp >= 0x0370 && cp <= 0x03FF) return { 0x0370, 0x03FF };
		if (cp >= 0x0400 && cp <= 0x052F) return { 0x0400, 0x052F };
		if (cp >= 0x0530 && cp <= 0x058F) return { 0x0530, 0x058F };
		if (cp >= 0x0590 && cp <= 0x05FF) return { 0x0590, 0x05FF };
		if (cp >= 0x0600 && cp <= 0x06FF) return { 0x0600, 0x06FF };
		if (cp >= 0x0750 && cp <= 0x077F) return { 0x0750, 0x077F };
		if (cp >= 0x0900 && cp <= 0x097F) return { 0x0900, 0x097F };
		if (cp >= 0x0980 && cp <= 0x09FF) return { 0x0980, 0x09FF };
		if (cp >= 0x0A00 && cp <= 0x0A7F) return { 0x0A00, 0x0A7F };
		if (cp >= 0x0A80 && cp <= 0x0AFF) return { 0x0A80, 0x0AFF };
		if (cp >= 0x0B00 && cp <= 0x0B7F) return { 0x0B00, 0x0B7F };
		if (cp >= 0x0B80 && cp <= 0x0BFF) return { 0x0B80, 0x0BFF };
		if (cp >= 0x0C00 && cp <= 0x0C7F) return { 0x0C00, 0x0C7F };
		if (cp >= 0x0C80 && cp <= 0x0CFF) return { 0x0C80, 0x0CFF };
		if (cp >= 0x0D00 && cp <= 0x0D7F) return { 0x0D00, 0x0D7F };
		if (cp >= 0x0E00 && cp <= 0x0E7F) return { 0x0E00, 0x0E7F };
		if (cp >= 0x0E80 && cp <= 0x0EFF) return { 0x0E80, 0x0EFF };
		if (cp >= 0x1000 && cp <= 0x109F) return { 0x1000, 0x109F };
		if (cp >= 0x10A0 && cp <= 0x10FF) return { 0x10A0, 0x10FF };
		if (cp >= 0x2000 && cp <= 0x206F) return { 0x2000, 0x206F };
		if (cp >= 0x2190 && cp <= 0x21FF) return { 0x2190, 0x21FF };
		if (cp >= 0x2200 && cp <= 0x22FF) return { 0x2200, 0x22FF };
		if (cp >= 0x2500 && cp <= 0x257F) return { 0x2500, 0x257F };
		if (cp >= 0x2580 && cp <= 0x259F) return { 0x2580, 0x259F };
		if (cp >= 0x25A0 && cp <= 0x25FF) return { 0x25A0, 0x25FF };
		if (cp >= 0x3000 && cp <= 0x303F) return { 0x3000, 0x303F };
		if (cp >= 0x3040 && cp <= 0x309F) return { 0x3040, 0x309F };
		if (cp >= 0x30A0 && cp <= 0x30FF) return { 0x30A0, 0x30FF };
		if (cp >= 0x3100 && cp <= 0x312F) return { 0x3100, 0x312F };
		if (cp >= 0x3130 && cp <= 0x318F) return { 0x3130, 0x318F };
		if (cp >= 0x31A0 && cp <= 0x31BF) return { 0x31A0, 0x31BF };
		if (cp >= 0x31F0 && cp <= 0x31FF) return { 0x31F0, 0x31FF };
		if (cp >= 0x3400 && cp <= 0x4DBF) return { 0x3400, 0x4DBF };
		if (cp >= 0x4E00 && cp <= 0x9FFF) return { 0x4E00, 0x9FFF };
		if (cp >= 0xA000 && cp <= 0xA48F) return { 0xA000, 0xA48F };
		if (cp >= 0xA960 && cp <= 0xA97F) return { 0xA960, 0xA97F };
		if (cp >= 0xAC00 && cp <= 0xD7AF) return { 0xAC00, 0xD7AF };
		if (cp >= 0xF900 && cp <= 0xFAFF) return { 0xF900, 0xFAFF };
		if (cp >= 0xFE10 && cp <= 0xFE1F) return { 0xFE10, 0xFE1F };
		if (cp >= 0xFE30 && cp <= 0xFE4F) return { 0xFE30, 0xFE4F };
		if (cp >= 0xFF00 && cp <= 0xFFEF) return { 0xFF00, 0xFFEF };

		return { cp, cp };
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
			vector_t<glyph_range> ranges;

			while (s < end)
			{
				const glyph_range range = glyph_range_for_codepoint(decode_utf8(s, end));
				bool known = false;

				for (const glyph_range existing : ranges)
				{
					if (existing.first == range.first && existing.last == range.last)
					{
						known = true;
						break;
					}
				}

				if (!known)
				{
					ranges.push_back(range);
					add_range(range);
				}
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
