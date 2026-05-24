#pragma once
#include "../util/types.hpp"

namespace rv
{
	enum class size_mode : cstd::uint8_t
	{
		px,
		percent,
		auto_v,
		fill
	};

	struct styled_size
	{
		size_mode mode = size_mode::px;
		float value = 0.f;

		constexpr styled_size() noexcept = default;

		constexpr styled_size(const float px_value) noexcept
				:	value(px_value) { }

		[[nodiscard]] static constexpr styled_size px(const float v) noexcept
		{
			return { size_mode::px, v };
		}

		[[nodiscard]] static constexpr styled_size percent(const float v) noexcept
		{
			return { size_mode::percent, v };
		}

		[[nodiscard]] static constexpr styled_size auto_v() noexcept
		{
			return{size_mode::auto_v, 0.f };
		}

		[[nodiscard]] static constexpr styled_size fill() noexcept
		{
			return { size_mode::fill, 0.f };
		}

	private:
		constexpr styled_size(const size_mode m, const float v) noexcept
				:	mode(m), value(v) { }
	};

	struct element_size
	{
		styled_size width = styled_size::auto_v();
		styled_size height = styled_size::auto_v();

		constexpr element_size() noexcept = default;

		constexpr element_size(const styled_size w, const styled_size h) noexcept
				:	width(w), height(h) { }
	};
}
