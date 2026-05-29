#pragma once
#include "input.hpp"

#if defined(_WIN32)
#include <windows.h>

namespace rv
{
	class win32_input : public input
	{
	public:
		bool handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	};
}

#endif // _WIN32
