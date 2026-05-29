#include "win32.hpp"
#include "../log/log.hpp"
#include <windowsx.h>

bool rv::win32_input::handle_message(const HWND hwnd, const UINT msg, const WPARAM wparam, const LPARAM lparam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (wparam < 256)
		{
			if (!state_.keys_down[wparam])
				state_.keys_pressed[wparam] = true;
			state_.keys_down[wparam] = true;
		}
		return true;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		if (wparam < 256)
		{
			if (state_.keys_down[wparam])
				state_.keys_released[wparam] = true;
			state_.keys_down[wparam] = false;
			state_.keys_pressed[wparam] = false;
		}
		return true;
	case WM_CHAR:
	{
		const auto input_char = static_cast<input_state::char_type>(wparam);

		if (input_char >= 0x20 && input_char != 0x7F)
		{
			state_.typed_chars.push_back(input_char);
		}

		return true;
	}
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		if (!state_.mouse_down[0]) state_.mouse_clicked[0] = true;
		state_.mouse_down[0] = true;
		return true;

	case WM_LBUTTONUP:
		if (state_.mouse_down[0]) state_.mouse_released[0] = true;
		state_.mouse_down[0] = false;
		state_.mouse_clicked[0] = false;
		return true;

	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		if (!state_.mouse_down[1]) state_.mouse_clicked[1] = true;
		state_.mouse_down[1] = true;
		return true;

	case WM_RBUTTONUP:
		if (state_.mouse_down[1]) state_.mouse_released[1] = true;
		state_.mouse_down[1] = false;
		state_.mouse_clicked[1] = false;
		return true;

	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		if (!state_.mouse_down[2]) state_.mouse_clicked[2] = true;
		state_.mouse_down[2] = true;
		return true;

	case WM_MBUTTONUP:
		if (state_.mouse_down[2]) state_.mouse_released[2] = true;
		state_.mouse_down[2] = false;
		state_.mouse_clicked[2] = false;
		return true;

	case WM_MOUSEWHEEL:
		state_.scroll_delta += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wparam)) / static_cast<float>(WHEEL_DELTA);
		return true;

	case WM_MOUSEMOVE:
		state_.mouse_pos.x = static_cast<float>(GET_X_LPARAM(lparam));
		state_.mouse_pos.y = static_cast<float>(GET_Y_LPARAM(lparam));
		return true;
		
	case WM_SETCURSOR:
		if (LOWORD(lparam) == HTCLIENT)
		{
			if (current_cursor_ == cursor_type::none)
			{
				::SetCursor(NULL);
			}
			else
			{
				LPTSTR win32_cursor = IDC_ARROW;
				switch (current_cursor_)
				{
				case cursor_type::arrow:        win32_cursor = IDC_ARROW; break;
				case cursor_type::text_input:   win32_cursor = IDC_IBEAM; break;
				case cursor_type::resize_all:   win32_cursor = IDC_SIZEALL; break;
				case cursor_type::resize_ns:    win32_cursor = IDC_SIZENS; break;
				case cursor_type::resize_ew:    win32_cursor = IDC_SIZEWE; break;
				case cursor_type::resize_nesw:  win32_cursor = IDC_SIZENESW; break;
				case cursor_type::resize_nwse:  win32_cursor = IDC_SIZENWSE; break;
				case cursor_type::hand:         win32_cursor = IDC_HAND; break;
				case cursor_type::not_allowed:  win32_cursor = IDC_NO; break;
				default:                        win32_cursor = IDC_ARROW; break;
				}
				::SetCursor(::LoadCursor(NULL, win32_cursor));
			}
			return true;
		}
		return false;
	}

	return false;
}
