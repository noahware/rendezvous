#pragma once
#include "../util/types.hpp"
#include "../render/position.hpp"

namespace rv
{
	struct position;

	enum class cursor_type : cstd::uint8_t
	{
		none = 0,
		arrow,
		text_input,
		resize_all,
		resize_ns,
		resize_ew,
		resize_nesw,
		resize_nwse,
		hand,
		not_allowed
	};

	enum class key : cstd::int32_t
	{
		none          = 0x00,

		// Mouse buttons
		mouse_left    = 0x01,
		mouse_right   = 0x02,
		mouse_middle  = 0x04,
		mouse_4       = 0x05,
		mouse_5       = 0x06,

		// Typing
		backspace     = 0x08,
		tab           = 0x09,
		enter         = 0x0D,
		escape        = 0x1B,
		space         = 0x20,

		// Modifiers (generic)
		shift         = 0x10,
		control       = 0x11,
		alt           = 0x12,
		pause         = 0x13,
		caps_lock     = 0x14,

		// Navigation
		page_up       = 0x21,
		page_down     = 0x22,
		end           = 0x23,
		home          = 0x24,
		left          = 0x25,
		up            = 0x26,
		right         = 0x27,
		down          = 0x28,
		print_screen  = 0x2C,
		insert        = 0x2D,
		del           = 0x2E,

		// Digits
		num_0         = 0x30,
		num_1         = 0x31,
		num_2         = 0x32,
		num_3         = 0x33,
		num_4         = 0x34,
		num_5         = 0x35,
		num_6         = 0x36,
		num_7         = 0x37,
		num_8         = 0x38,
		num_9         = 0x39,

		// Letters
		a             = 0x41,
		b             = 0x42,
		c             = 0x43,
		d             = 0x44,
		e             = 0x45,
		f             = 0x46,
		g             = 0x47,
		h             = 0x48,
		i             = 0x49,
		j             = 0x4A,
		k             = 0x4B,
		l             = 0x4C,
		m             = 0x4D,
		n             = 0x4E,
		o             = 0x4F,
		p             = 0x50,
		q             = 0x51,
		r             = 0x52,
		s             = 0x53,
		t             = 0x54,
		u             = 0x55,
		v             = 0x56,
		w             = 0x57,
		x             = 0x58,
		y             = 0x59,
		z             = 0x5A,

		// Numpad
		numpad_0      = 0x60,
		numpad_1      = 0x61,
		numpad_2      = 0x62,
		numpad_3      = 0x63,
		numpad_4      = 0x64,
		numpad_5      = 0x65,
		numpad_6      = 0x66,
		numpad_7      = 0x67,
		numpad_8      = 0x68,
		numpad_9      = 0x69,
		numpad_multiply = 0x6A,
		numpad_add    = 0x6B,
		numpad_subtract = 0x6D,
		numpad_decimal = 0x6E,
		numpad_divide = 0x6F,

		// Function keys
		f1            = 0x70,
		f2            = 0x71,
		f3            = 0x72,
		f4            = 0x73,
		f5            = 0x74,
		f6            = 0x75,
		f7            = 0x76,
		f8            = 0x77,
		f9            = 0x78,
		f10           = 0x79,
		f11           = 0x7A,
		f12           = 0x7B,

		// Lock keys
		num_lock      = 0x90,
		scroll_lock   = 0x91,

		// Left/right modifiers
		left_shift    = 0xA0,
		right_shift   = 0xA1,
		left_control  = 0xA2,
		right_control = 0xA3,
		left_alt      = 0xA4,
		right_alt     = 0xA5,

		// Punctuation / OEM keys
		semicolon     = 0xBA,
		equals        = 0xBB,
		comma         = 0xBC,
		minus         = 0xBD,
		period        = 0xBE,
		slash         = 0xBF,
		backtick      = 0xC0,
		left_bracket  = 0xDB,
		backslash     = 0xDC,
		right_bracket = 0xDD,
		quote         = 0xDE,
	};

	struct input_state
	{
		using char_type = cstd::uint32_t;

		constexpr static cstd::int32_t key_count = 256;
		constexpr static cstd::int32_t button_count = 5;

		array_t<bool, key_count> keys_down = {};
		array_t<bool, key_count> keys_pressed = {};
		array_t<bool, key_count> keys_released = {};
		array_t<bool, button_count> mouse_down = {};
		array_t<bool, button_count> mouse_clicked = {};
		array_t<bool, button_count> mouse_released = {};
		vector_t<char_type> typed_chars = {};
		position mouse_pos = {};
		float scroll_delta = 0.f;
	};

	class input
	{
	public:
		using key_type = cstd::int32_t;
		using button_type = cstd::int32_t;

		virtual ~input() = default;

		// Clipboard access. The base implementation is an in-process fallback;
		// platform backends override these with the real OS clipboard.
		virtual void set_clipboard_text(const string_t& text)
		{
			clipboard_ = text;
		}

		[[nodiscard]] virtual string_t get_clipboard_text()
		{
			return clipboard_;
		}

		void reset()
		{
			state_.keys_pressed.fill(false);
			state_.keys_released.fill(false);
			state_.mouse_clicked.fill(false);
			state_.mouse_released.fill(false);
			state_.typed_chars.clear();
			state_.scroll_delta = 0.f;
		}

		[[nodiscard]] cursor_type get_cursor() const noexcept
		{
			return current_cursor_;
		}

		void set_cursor(const cursor_type cursor) noexcept
		{
			current_cursor_ = cursor;
		}

		[[nodiscard]] bool is_key_down(const key_type key) const noexcept
		{
			return key_in_range(key) && state_.keys_down[key];
		}

		[[nodiscard]] bool is_key_pressed(const key_type key) const noexcept
		{
			return key_in_range(key) && state_.keys_pressed[key];
		}

		[[nodiscard]] bool is_key_released(const key_type key) const noexcept
		{
			return key_in_range(key) && state_.keys_released[key];
		}

		[[nodiscard]] bool is_key_down(const key k) const noexcept
		{
			return is_key_down(static_cast<key_type>(k));
		}

		[[nodiscard]] bool is_key_pressed(const key k) const noexcept
		{
			return is_key_pressed(static_cast<key_type>(k));
		}

		[[nodiscard]] bool is_key_released(const key k) const noexcept
		{
			return is_key_released(static_cast<key_type>(k));
		}

		[[nodiscard]] bool is_mouse_down(const button_type button) const noexcept
		{
			return button_in_range(button) && state_.mouse_down[button];
		}

		[[nodiscard]] bool is_mouse_clicked(const button_type button) const noexcept
		{
			return button_in_range(button) && state_.mouse_clicked[button];
		}

		[[nodiscard]] bool is_mouse_released(const button_type button) const noexcept
		{
			return button_in_range(button) && state_.mouse_released[button];
		}

		[[nodiscard]] position mouse_pos() const noexcept
		{
			return state_.mouse_pos;
		}

		[[nodiscard]] float scroll_delta() const noexcept
		{
			return state_.scroll_delta;
		}

		[[nodiscard]] span_t<const input_state::char_type> typed_chars() const noexcept
		{
			return { state_.typed_chars.data(), state_.typed_chars.size() };
		}

		void clear_typed_chars()
		{
			state_.typed_chars.clear();
		}

	protected:
		[[nodiscard]] static bool key_in_range(const key_type key) noexcept
		{
			return 0 <= key && key < input_state::key_count;
		}

		[[nodiscard]] static bool button_in_range(const button_type button) noexcept
		{
			return 0 <= button && button < input_state::button_count;
		}

		input_state state_;
		cursor_type current_cursor_ = cursor_type::arrow;
		string_t clipboard_;
	};
}
