#pragma once
#include "../gui.hpp"
#include "../text_metrics.hpp"
#include "../../util/types.hpp"

namespace rv
{
	// The editable string object stb_textedit operates on. Holds the text as a
	// codepoint buffer plus the font metrics the layout/width callbacks need.
	struct text_edit_buffer
	{
		vector_t<char32_t> chars;
		shared_ptr_t<gui_font> font;
		float scale = 1.f;
		float line_height = 0.f;
		float wrap_width = 0.f;
		bool single_line = true;
	};
}

// High bit OR'd into command keys so STB_TEXTEDIT_KEYTOTEXT can tell control
// inputs apart from typed codepoints (the Win32 pattern from the stb docs).
#define RV_TE_CMD_BIT 0x40000000u

#define STB_TEXTEDIT_CHARTYPE         char32_t
#define STB_TEXTEDIT_POSITIONTYPE     cstd::int32_t
#define STB_TEXTEDIT_KEYTYPE          cstd::uint32_t
#define STB_TEXTEDIT_STRING           rv::text_edit_buffer
#define STB_TEXTEDIT_NEWLINE          U'\n'
#define STB_TEXTEDIT_GETWIDTH_NEWLINE (-1.0f)

#define STB_TEXTEDIT_STRINGLEN(obj)            (static_cast<cstd::int32_t>((obj)->chars.size()))
#define STB_TEXTEDIT_GETCHAR(obj, i)           ((obj)->chars[static_cast<cstd::size_t>(i)])
#define STB_TEXTEDIT_GETWIDTH(obj, n, i)       rv_te_get_width((obj), (n), (i))
#define STB_TEXTEDIT_KEYTOTEXT(k)              rv_te_key_to_text((k))
#define STB_TEXTEDIT_LAYOUTROW(r, obj, n)      rv_te_layout_row((r), (obj), (n))
#define STB_TEXTEDIT_DELETECHARS(obj, i, n)    (rv_te_delete_chars((obj), (i), (n)), 1)
#define STB_TEXTEDIT_INSERTCHARS(obj, i, c, n) rv_te_insert_chars((obj), (i), (c), (n))

#define STB_TEXTEDIT_K_SHIFT     0x20000000u
#define STB_TEXTEDIT_K_LEFT      (RV_TE_CMD_BIT | 1u)
#define STB_TEXTEDIT_K_RIGHT     (RV_TE_CMD_BIT | 2u)
#define STB_TEXTEDIT_K_UP        (RV_TE_CMD_BIT | 3u)
#define STB_TEXTEDIT_K_DOWN      (RV_TE_CMD_BIT | 4u)
#define STB_TEXTEDIT_K_LINESTART (RV_TE_CMD_BIT | 5u)
#define STB_TEXTEDIT_K_LINEEND   (RV_TE_CMD_BIT | 6u)
#define STB_TEXTEDIT_K_TEXTSTART (RV_TE_CMD_BIT | 7u)
#define STB_TEXTEDIT_K_TEXTEND   (RV_TE_CMD_BIT | 8u)
#define STB_TEXTEDIT_K_DELETE    (RV_TE_CMD_BIT | 9u)
#define STB_TEXTEDIT_K_BACKSPACE (RV_TE_CMD_BIT | 10u)
#define STB_TEXTEDIT_K_UNDO      (RV_TE_CMD_BIT | 11u)
#define STB_TEXTEDIT_K_REDO      (RV_TE_CMD_BIT | 12u)
#define STB_TEXTEDIT_K_PGUP      (RV_TE_CMD_BIT | 13u)
#define STB_TEXTEDIT_K_PGDOWN    (RV_TE_CMD_BIT | 14u)

// Header-file mode: declares STB_TexteditState / StbTexteditRow. The
// implementation is compiled separately in text_edit.cpp.
#include <stb_textedit.h>

// Callback helpers, defined here (after the header so StbTexteditRow exists)
// and referenced by the function-like macros above when the implementation TU
// expands them.
inline float rv_te_get_width(rv::text_edit_buffer* obj, cstd::int32_t n, cstd::int32_t i)
{
	const cstd::size_t idx = static_cast<cstd::size_t>(n + i);

	if (idx >= obj->chars.size())
	{
		return 0.f;
	}

	const char32_t ch = obj->chars[idx];

	if (ch == U'\n')
	{
		return STB_TEXTEDIT_GETWIDTH_NEWLINE;
	}

	if (!obj->font)
	{
		return 0.f;
	}

	// Account for kerning against the previous glyph on the same row (i > 0
	// means there is a preceding char), matching how the renderer measures runs.
	const cstd::uint32_t prev = (i > 0)
		? static_cast<cstd::uint32_t>(obj->chars[idx - 1])
		: 0u;

	return rv::glyph_step(*obj->font, prev, static_cast<cstd::uint32_t>(ch), obj->scale);
}

inline cstd::int32_t rv_te_key_to_text(cstd::uint32_t key)
{
	return (key & RV_TE_CMD_BIT) ? -1 : static_cast<cstd::int32_t>(key);
}

inline void rv_te_layout_row(StbTexteditRow* row, rv::text_edit_buffer* obj, cstd::int32_t n)
{
	const cstd::int32_t len = static_cast<cstd::int32_t>(obj->chars.size());
	float width = 0.f;
	cstd::int32_t i = n;
	cstd::uint32_t prev = 0;

	while (i < len)
	{
		const char32_t ch = obj->chars[static_cast<cstd::size_t>(i)];

		if (ch == U'\n')
		{
			++i; // consume the newline into this row
			break;
		}

		const float adv = obj->font
			? rv::glyph_step(*obj->font, prev, static_cast<cstd::uint32_t>(ch), obj->scale)
			: 0.f;

		// Word-wrap on width for multiline; never wrap single-line fields.
		if (!obj->single_line && obj->wrap_width > 0.f && i > n && width + adv > obj->wrap_width)
		{
			break;
		}

		width += adv;
		prev = static_cast<cstd::uint32_t>(ch);
		++i;
	}

	row->x0 = 0.f;
	row->x1 = width;
	row->baseline_y_delta = obj->line_height;
	row->ymin = 0.f;
	row->ymax = obj->line_height;
	row->num_chars = i - n;
}

inline void rv_te_delete_chars(rv::text_edit_buffer* obj, cstd::int32_t i, cstd::int32_t n)
{
	const auto first = obj->chars.begin() + i;
	obj->chars.erase(first, first + n);
}

inline cstd::int32_t rv_te_insert_chars(rv::text_edit_buffer* obj, cstd::int32_t i, const char32_t* text, cstd::int32_t n)
{
	obj->chars.insert(obj->chars.begin() + i, text, text + n);
	return 1;
}

namespace rv
{
	// Thin non-static wrappers over stb_textedit's static functions, defined in
	// text_edit.cpp where STB_TEXTEDIT_IMPLEMENTATION is set.
	void te_init(STB_TexteditState& state, bool single_line);
	void te_key(text_edit_buffer& buf, STB_TexteditState& state, cstd::uint32_t key);
	void te_click(text_edit_buffer& buf, STB_TexteditState& state, float x, float y);
	void te_drag(text_edit_buffer& buf, STB_TexteditState& state, float x, float y);
}
