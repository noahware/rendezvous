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
#if defined(_WIN32)
		backspace = 0x08,
		enter     = 0x0D,
		shift     = 0x10,
		control   = 0x11,
		end       = 0x23,
		home      = 0x24,
		left      = 0x25,
		up        = 0x26,
		right     = 0x27,
		down      = 0x28,
		del       = 0x2E,
#else
#error "rv::key codes are not defined for this platform; add a mapping for the active backend."
#endif
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
		using key_type = std::int32_t;
		using button_type = std::int32_t;

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
	};
}
