#pragma once
#include "input.hpp"

namespace rv
{
	[[nodiscard]] inline string_view_t key_name(const key k) noexcept
	{
		switch (k)
		{
		case key::none:             return "None";
		case key::mouse_left:       return "Mouse Left";
		case key::mouse_right:      return "Mouse Right";
		case key::mouse_middle:     return "Mouse Middle";
		case key::mouse_4:          return "Mouse 4";
		case key::mouse_5:          return "Mouse 5";
		case key::backspace:        return "Backspace";
		case key::tab:              return "Tab";
		case key::enter:            return "Enter";
		case key::escape:           return "Escape";
		case key::space:            return "Space";
		case key::shift:            return "Shift";
		case key::control:          return "Ctrl";
		case key::alt:              return "Alt";
		case key::pause:            return "Pause";
		case key::caps_lock:        return "Caps Lock";
		case key::page_up:          return "Page Up";
		case key::page_down:        return "Page Down";
		case key::end:              return "End";
		case key::home:             return "Home";
		case key::left:             return "Left";
		case key::up:               return "Up";
		case key::right:            return "Right";
		case key::down:             return "Down";
		case key::print_screen:     return "Print Screen";
		case key::insert:           return "Insert";
		case key::del:              return "Delete";

		case key::num_0:            return "0";
		case key::num_1:            return "1";
		case key::num_2:            return "2";
		case key::num_3:            return "3";
		case key::num_4:            return "4";
		case key::num_5:            return "5";
		case key::num_6:            return "6";
		case key::num_7:            return "7";
		case key::num_8:            return "8";
		case key::num_9:            return "9";

		case key::a:                return "A";
		case key::b:                return "B";
		case key::c:                return "C";
		case key::d:                return "D";
		case key::e:                return "E";
		case key::f:                return "F";
		case key::g:                return "G";
		case key::h:                return "H";
		case key::i:                return "I";
		case key::j:                return "J";
		case key::k:                return "K";
		case key::l:                return "L";
		case key::m:                return "M";
		case key::n:                return "N";
		case key::o:                return "O";
		case key::p:                return "P";
		case key::q:                return "Q";
		case key::r:                return "R";
		case key::s:                return "S";
		case key::t:                return "T";
		case key::u:                return "U";
		case key::v:                return "V";
		case key::w:                return "W";
		case key::x:                return "X";
		case key::y:                return "Y";
		case key::z:                return "Z";

		case key::numpad_0:         return "Num 0";
		case key::numpad_1:         return "Num 1";
		case key::numpad_2:         return "Num 2";
		case key::numpad_3:         return "Num 3";
		case key::numpad_4:         return "Num 4";
		case key::numpad_5:         return "Num 5";
		case key::numpad_6:         return "Num 6";
		case key::numpad_7:         return "Num 7";
		case key::numpad_8:         return "Num 8";
		case key::numpad_9:         return "Num 9";
		case key::numpad_multiply:  return "Num *";
		case key::numpad_add:       return "Num +";
		case key::numpad_subtract:  return "Num -";
		case key::numpad_decimal:   return "Num .";
		case key::numpad_divide:    return "Num /";

		case key::f1:               return "F1";
		case key::f2:               return "F2";
		case key::f3:               return "F3";
		case key::f4:               return "F4";
		case key::f5:               return "F5";
		case key::f6:               return "F6";
		case key::f7:               return "F7";
		case key::f8:               return "F8";
		case key::f9:               return "F9";
		case key::f10:              return "F10";
		case key::f11:              return "F11";
		case key::f12:              return "F12";

		case key::num_lock:         return "Num Lock";
		case key::scroll_lock:      return "Scroll Lock";

		case key::left_shift:       return "Left Shift";
		case key::right_shift:      return "Right Shift";
		case key::left_control:     return "Left Ctrl";
		case key::right_control:    return "Right Ctrl";
		case key::left_alt:         return "Left Alt";
		case key::right_alt:        return "Right Alt";

		case key::semicolon:        return ";";
		case key::equals:           return "=";
		case key::comma:            return ",";
		case key::minus:            return "-";
		case key::period:           return ".";
		case key::slash:            return "/";
		case key::backtick:         return "`";
		case key::left_bracket:     return "[";
		case key::backslash:        return "\\";
		case key::right_bracket:    return "]";
		case key::quote:            return "'";

		default:                    return "Unknown";
		}
	}

	[[nodiscard]] inline string_view_t key_display_name(const key k) noexcept
	{
		return key_name(k);
	}
}
