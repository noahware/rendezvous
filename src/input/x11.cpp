#include "x11.hpp"

#if defined(__linux__)
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <unistd.h>

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

			const auto generic = generic_modifier(k);
			if (generic >= 0)
			{
				state_.keys_down[generic] = true;
				state_.keys_pressed[generic] = true;
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

			const auto generic = generic_modifier(k);
			if (generic >= 0)
			{
				state_.keys_down[generic] = false;
				state_.keys_released[generic] = true;
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

		case SelectionRequest:
		{
			// Another app is pasting our clipboard, hand it the text.
			handle_selection_request(event.xselectionrequest);
			break;
		}

		case SelectionClear:
		{
			// Another app took ownership of CLIPBOARD; future pastes must round-trip.
			clipboard_.clear();
			break;
		}
	}
}

rv::x11_input::key_type rv::x11_input::translate_key(const KeySym sym)
{
	switch (sym)
	{
		case XK_BackSpace:   return static_cast<key_type>(key::backspace);
		case XK_Tab:         return static_cast<key_type>(key::tab);
		case XK_Return:      return static_cast<key_type>(key::enter);
		case XK_Escape:      return static_cast<key_type>(key::escape);
		case XK_space:       return static_cast<key_type>(key::space);
		case XK_Shift_L:     return static_cast<key_type>(key::left_shift);
		case XK_Shift_R:     return static_cast<key_type>(key::right_shift);
		case XK_Control_L:   return static_cast<key_type>(key::left_control);
		case XK_Control_R:   return static_cast<key_type>(key::right_control);
		case XK_Alt_L:       return static_cast<key_type>(key::left_alt);
		case XK_Alt_R:       return static_cast<key_type>(key::right_alt);
		case XK_Pause:       return static_cast<key_type>(key::pause);
		case XK_Caps_Lock:   return static_cast<key_type>(key::caps_lock);
		case XK_Page_Up:     return static_cast<key_type>(key::page_up);
		case XK_Page_Down:   return static_cast<key_type>(key::page_down);
		case XK_End:         return static_cast<key_type>(key::end);
		case XK_Home:        return static_cast<key_type>(key::home);
		case XK_Left:        return static_cast<key_type>(key::left);
		case XK_Up:          return static_cast<key_type>(key::up);
		case XK_Right:       return static_cast<key_type>(key::right);
		case XK_Down:        return static_cast<key_type>(key::down);
		case XK_Print:       return static_cast<key_type>(key::print_screen);
		case XK_Insert:      return static_cast<key_type>(key::insert);
		case XK_Delete:      return static_cast<key_type>(key::del);
		case XK_Num_Lock:    return static_cast<key_type>(key::num_lock);
		case XK_Scroll_Lock: return static_cast<key_type>(key::scroll_lock);

		case XK_KP_0:        return static_cast<key_type>(key::numpad_0);
		case XK_KP_1:        return static_cast<key_type>(key::numpad_1);
		case XK_KP_2:        return static_cast<key_type>(key::numpad_2);
		case XK_KP_3:        return static_cast<key_type>(key::numpad_3);
		case XK_KP_4:        return static_cast<key_type>(key::numpad_4);
		case XK_KP_5:        return static_cast<key_type>(key::numpad_5);
		case XK_KP_6:        return static_cast<key_type>(key::numpad_6);
		case XK_KP_7:        return static_cast<key_type>(key::numpad_7);
		case XK_KP_8:        return static_cast<key_type>(key::numpad_8);
		case XK_KP_9:        return static_cast<key_type>(key::numpad_9);
		case XK_KP_Multiply: return static_cast<key_type>(key::numpad_multiply);
		case XK_KP_Add:      return static_cast<key_type>(key::numpad_add);
		case XK_KP_Subtract: return static_cast<key_type>(key::numpad_subtract);
		case XK_KP_Decimal:  return static_cast<key_type>(key::numpad_decimal);
		case XK_KP_Divide:   return static_cast<key_type>(key::numpad_divide);

		case XK_F1:  return static_cast<key_type>(key::f1);
		case XK_F2:  return static_cast<key_type>(key::f2);
		case XK_F3:  return static_cast<key_type>(key::f3);
		case XK_F4:  return static_cast<key_type>(key::f4);
		case XK_F5:  return static_cast<key_type>(key::f5);
		case XK_F6:  return static_cast<key_type>(key::f6);
		case XK_F7:  return static_cast<key_type>(key::f7);
		case XK_F8:  return static_cast<key_type>(key::f8);
		case XK_F9:  return static_cast<key_type>(key::f9);
		case XK_F10: return static_cast<key_type>(key::f10);
		case XK_F11: return static_cast<key_type>(key::f11);
		case XK_F12: return static_cast<key_type>(key::f12);

		default:
			// Uppercase letters: X11 keysyms 'a'-'z' (0x61-0x7A) → our enum 'a'-'z' (0x41-0x5A)
			if (sym >= XK_a && sym <= XK_z)
				return static_cast<key_type>(key::a) + static_cast<key_type>(sym - XK_a);
			if (sym >= XK_A && sym <= XK_Z)
				return static_cast<key_type>(key::a) + static_cast<key_type>(sym - XK_A);
			// Digits '0'-'9' match directly
			if (sym >= XK_0 && sym <= XK_9)
				return static_cast<key_type>(key::num_0) + static_cast<key_type>(sym - XK_0);
			return -1;
	}
}

rv::x11_input::key_type rv::x11_input::generic_modifier(const key_type specific)
{
	switch (static_cast<key>(specific))
	{
		case key::left_shift:
		case key::right_shift:   return static_cast<key_type>(key::shift);
		case key::left_control:
		case key::right_control: return static_cast<key_type>(key::control);
		case key::left_alt:
		case key::right_alt:     return static_cast<key_type>(key::alt);
		default:                 return -1;
	}
}

rv::x11_input::button_type rv::x11_input::translate_button(const cstd::uint32_t button)
{
	switch (button)
	{
		case Button1: return 0;  // left
		case Button2: return 2;  // middle
		case Button3: return 1;  // right
		case 8:       return 3;  // side back (mouse 4)
		case 9:       return 4;  // side forward (mouse 5)
		default:      return -1;
	}
}

void rv::x11_input::set_window(Display* display, Window window)
{
	display_ = display;
	window_ = window;
}

void rv::x11_input::ensure_atoms()
{
	if (atom_clipboard_ || !display_)
	{
		return;
	}

	atom_clipboard_ = XInternAtom(display_, "CLIPBOARD", False);
	atom_utf8_      = XInternAtom(display_, "UTF8_STRING", False);
	atom_targets_   = XInternAtom(display_, "TARGETS", False);
	atom_property_  = XInternAtom(display_, "RV_CLIPBOARD", False);
}

void rv::x11_input::set_clipboard_text(const string_t& text)
{
	if (!display_ || !window_)
	{
		clipboard_ = text;
		return;
	}

	ensure_atoms();

	clipboard_ = text;
	XSetSelectionOwner(display_, atom_clipboard_, window_, CurrentTime);
	XFlush(display_);
}

void rv::x11_input::handle_selection_request(const XSelectionRequestEvent& request)
{
	ensure_atoms();

	XSelectionEvent response = {};
	response.type      = SelectionNotify;
	response.display   = request.display;
	response.requestor = request.requestor;
	response.selection = request.selection;
	response.target    = request.target;
	response.time      = request.time;
	response.property  = request.property; // None signals refusal

	if (request.target == atom_targets_)
	{
		// Advertise the formats we can serve.
		const Atom targets[] = { atom_targets_, atom_utf8_, XA_STRING };
		XChangeProperty(request.display, request.requestor, request.property,
			XA_ATOM, 32, PropModeReplace,
			reinterpret_cast<const unsigned char*>(targets),
			static_cast<cstd::int32_t>(sizeof(targets) / sizeof(targets[0])));
	}
	else if (request.target == atom_utf8_ || request.target == XA_STRING)
	{
		XChangeProperty(request.display, request.requestor, request.property,
			request.target, 8, PropModeReplace,
			reinterpret_cast<const unsigned char*>(clipboard_.data()),
			static_cast<cstd::int32_t>(clipboard_.size()));
	}
	else
	{
		response.property = None;
	}

	XSendEvent(request.display, request.requestor, True, NoEventMask,
		reinterpret_cast<XEvent*>(&response));
}

string_t rv::x11_input::get_clipboard_text()
{
	if (!display_ || !window_)
	{
		return clipboard_;
	}

	ensure_atoms();

	// Fast path: we still own the selection, so serve the local copy.
	if (XGetSelectionOwner(display_, atom_clipboard_) == window_)
	{
		return clipboard_;
	}

	XConvertSelection(display_, atom_clipboard_, atom_utf8_, atom_property_,
		window_, CurrentTime);
	XFlush(display_);

	// Bounded wait for the owner to deliver the data (~200ms).
	XEvent event;
	bool got_notify = false;

	for (cstd::int32_t i = 0; i < 200; ++i)
	{
		if (XCheckTypedWindowEvent(display_, window_, SelectionNotify, &event))
		{
			got_notify = true;
			break;
		}

		usleep(1000);
	}

	if (!got_notify || event.xselection.property == None)
	{
		return {};
	}

	Atom actual_type = 0;
	cstd::int32_t actual_format = 0;
	unsigned long item_count = 0;
	unsigned long bytes_after = 0;
	unsigned char* data = nullptr;

	const cstd::int32_t status = XGetWindowProperty(display_, window_, atom_property_,
		0, ~0L, True /* delete */, AnyPropertyType,
		&actual_type, &actual_format, &item_count, &bytes_after, &data);

	string_t result;

	if (status == Success && data)
	{
		result.assign(reinterpret_cast<const char*>(data), item_count);
		XFree(data);
	}

	return result;
}

#endif // __linux__
