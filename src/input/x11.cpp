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

		case SelectionRequest:
		{
			// Another app is pasting our clipboard — hand it the text.
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
			static_cast<int>(sizeof(targets) / sizeof(targets[0])));
	}
	else if (request.target == atom_utf8_ || request.target == XA_STRING)
	{
		XChangeProperty(request.display, request.requestor, request.property,
			request.target, 8, PropModeReplace,
			reinterpret_cast<const unsigned char*>(clipboard_.data()),
			static_cast<int>(clipboard_.size()));
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

	for (int i = 0; i < 200; ++i)
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
	int actual_format = 0;
	unsigned long item_count = 0;
	unsigned long bytes_after = 0;
	unsigned char* data = nullptr;

	const int status = XGetWindowProperty(display_, window_, atom_property_,
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
