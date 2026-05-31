#pragma once
#include "input.hpp"

#if defined(__linux__)
#include <X11/Xlib.h>
#include <X11/keysym.h>

namespace rv
{
	class x11_input : public input
	{
	public:
		void handle_event(const XEvent& event);

		// Supplies the display/window the clipboard (CLIPBOARD selection) needs.
		void set_window(Display* display, Window window);

		void set_clipboard_text(const string_t& text) override;
		[[nodiscard]] string_t get_clipboard_text() override;

	private:
		static key_type translate_key(KeySym sym);
		static key_type generic_modifier(key_type specific);
		static button_type translate_button(cstd::uint32_t button);

		// Interns and caches the selection-related atoms on first use.
		void ensure_atoms();
		void handle_selection_request(const XSelectionRequestEvent& request);

		Display* display_ = nullptr;
		Window window_ = 0;

		Atom atom_clipboard_ = 0;
		Atom atom_utf8_ = 0;
		Atom atom_targets_ = 0;
		Atom atom_property_ = 0;
	};
}

#endif // __linux__
