#include "x11.hpp"

#if defined(__linux__)
#include <X11/Xutil.h>

void rv::x11_input::handle_event(const XEvent& event)
{
	switch (event.type)
	{
		case KeyPress:
		{
			const KeySym sym = XLookupKeysym(const_cast<XKeyEvent*>(&event.xkey), 0);
			const auto k = translate_key(sym);
			if (key_in_range(k))
			{
				state_.keys_down[k] = true;
				state_.keys_pressed[k] = true;
			}

			array_t<char, 8> buf = {};
			const cstd::int32_t len = XLookupString(const_cast<XKeyEvent*>(&event.xkey), buf.data(),
			                                        static_cast<cstd::int32_t>(buf.size()), nullptr, nullptr);
			for (cstd::int32_t i = 0; i < len; ++i)
			{
				if (static_cast<cstd::uint8_t>(buf[i]) >= 32)
				{
					state_.typed_chars.push_back(static_cast<input_state::char_type>(buf[i]));
				}
			}
			break;
		}

		case KeyRelease:
		{
			const KeySym sym = XLookupKeysym(const_cast<XKeyEvent*>(&event.xkey), 0);
			const auto k = translate_key(sym);
			if (key_in_range(k))
			{
				state_.keys_down[k] = false;
				state_.keys_released[k] = true;
			}
			break;
		}

		case ButtonPress:
		{
			const auto btn = translate_button(event.xbutton.button);
			if (button_in_range(btn))
			{
				state_.mouse_down[btn] = true;
				state_.mouse_clicked[btn] = true;
			}

			if (event.xbutton.button == 4)
				state_.scroll_delta += 1.f;
			else if (event.xbutton.button == 5)
				state_.scroll_delta -= 1.f;
			break;
		}

		case ButtonRelease:
		{
			const auto btn = translate_button(event.xbutton.button);
			if (button_in_range(btn))
			{
				state_.mouse_down[btn] = false;
				state_.mouse_released[btn] = true;
			}
			break;
		}

		case MotionNotify:
		{
			state_.mouse_pos = { static_cast<float>(event.xmotion.x), static_cast<float>(event.xmotion.y) };
			break;
		}
	}
}

rv::x11_input::key_type rv::x11_input::translate_key(const KeySym sym)
{
	switch (sym)
	{
		case XK_BackSpace: return static_cast<key_type>(key::backspace);
		case XK_Return:    return static_cast<key_type>(key::enter);
		case XK_Shift_L:
		case XK_Shift_R:   return static_cast<key_type>(key::shift);
		case XK_Control_L:
		case XK_Control_R: return static_cast<key_type>(key::control);
		case XK_End:       return static_cast<key_type>(key::end);
		case XK_Home:      return static_cast<key_type>(key::home);
		case XK_Left:      return static_cast<key_type>(key::left);
		case XK_Up:        return static_cast<key_type>(key::up);
		case XK_Right:     return static_cast<key_type>(key::right);
		case XK_Down:      return static_cast<key_type>(key::down);
		case XK_Delete:    return static_cast<key_type>(key::del);
		default:
			if (sym >= 0x20 && sym <= 0xFF)
				return static_cast<key_type>(sym);
			return -1;
	}
}

rv::x11_input::button_type rv::x11_input::translate_button(const cstd::uint32_t button)
{
	switch (button)
	{
		case Button1: return 0;
		case Button2: return 2;
		case Button3: return 1;
		default:      return -1;
	}
}

#endif // __linux__
