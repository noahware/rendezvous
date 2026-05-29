#include "text_edit.hpp"

// Header above pulled in stb_textedit.h in header mode (declarations only) and
// defined the rv_te_* callback helpers the macros expand to. Now compile the
// implementation: the helpers already exist, so the static stb functions link.
#define STB_TEXTEDIT_IMPLEMENTATION
#include <stb_textedit.h>

namespace rv
{
	void te_init(STB_TexteditState& state, const bool single_line)
	{
		stb_textedit_initialize_state(&state, single_line ? 1 : 0);
	}

	void te_key(text_edit_buffer& buf, STB_TexteditState& state, const cstd::uint32_t key)
	{
		stb_textedit_key(&buf, &state, key);
	}

	void te_click(text_edit_buffer& buf, STB_TexteditState& state, const float x, const float y)
	{
		stb_textedit_click(&buf, &state, x, y);
	}

	void te_drag(text_edit_buffer& buf, STB_TexteditState& state, const float x, const float y)
	{
		stb_textedit_drag(&buf, &state, x, y);
	}
}
