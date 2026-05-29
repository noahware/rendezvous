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

	private:
		static key_type translate_key(KeySym sym);
		static button_type translate_button(cstd::uint32_t button);
	};
}

#endif // __linux__
