#pragma once
#include "../render/position.hpp"
#include "../util/types.hpp"

#include <algorithm>

namespace rv
{
	enum class easing : cstd::uint8_t
	{
		linear,
		// sine
		ease_in_sine, ease_out_sine, ease_in_out_sine,
		// polynomial
		ease_in_quad, ease_out_quad, ease_in_out_quad,
		ease_in_cubic, ease_out_cubic, ease_in_out_cubic,
		ease_in_quart, ease_out_quart, ease_in_out_quart,
		ease_in_quint, ease_out_quint, ease_in_out_quint,
		// exponential / circular
		ease_in_expo, ease_out_expo, ease_in_out_expo,
		ease_in_circ, ease_out_circ, ease_in_out_circ,
		// overshoot
		ease_in_back, ease_out_back, ease_in_out_back,
		// springy
		ease_in_elastic, ease_out_elastic, ease_in_out_elastic,
		// bounce
		ease_in_bounce, ease_out_bounce, ease_in_out_bounce,
	};

	namespace detail
	{
		inline constexpr float c1 = 1.70158f;
		inline constexpr float c2 = c1 * 1.525f;
		inline constexpr float c3 = c1 + 1.f;
		inline constexpr float c4 = (2.f * cstd::numbers::pi_f) / 3.f;
		inline constexpr float c5 = (2.f * cstd::numbers::pi_f) / 4.5f;

		[[nodiscard]] inline float bounce_out(float t) noexcept
		{
			constexpr float n1 = 7.5625f;
			constexpr float d1 = 2.75f;

			if (t < 1.f / d1)
			{
				return n1 * t * t;
			}
			if (t < 2.f / d1)
			{
				t -= 1.5f / d1;
				return n1 * t * t + 0.75f;
			}
			if (t < 2.5f / d1)
			{
				t -= 2.25f / d1;
				return n1 * t * t + 0.9375f;
			}

			t -= 2.625f / d1;
			return n1 * t * t + 0.984375f;
		}
	}

	[[nodiscard]] inline float apply_easing(const easing e, const float t) noexcept
	{
		using namespace detail;
		const float pi = cstd::numbers::pi_f;

		switch (e)
		{
			case easing::linear:
				return t;

			// sine
			case easing::ease_in_sine:
				return 1.f - cstd::cosf((t * pi) / 2.f);
			case easing::ease_out_sine:
				return cstd::sinf((t * pi) / 2.f);
			case easing::ease_in_out_sine:
				return -(cstd::cosf(pi * t) - 1.f) / 2.f;

			// quad
			case easing::ease_in_quad:
				return t * t;
			case easing::ease_out_quad:
				return 1.f - (1.f - t) * (1.f - t);
			case easing::ease_in_out_quad:
				return t < 0.5f
					? 2.f * t * t
					: 1.f - cstd::powf(-2.f * t + 2.f, 2.f) / 2.f;

			// cubic
			case easing::ease_in_cubic:
				return t * t * t;
			case easing::ease_out_cubic:
				return 1.f - cstd::powf(1.f - t, 3.f);
			case easing::ease_in_out_cubic:
				return t < 0.5f
					? 4.f * t * t * t
					: 1.f - cstd::powf(-2.f * t + 2.f, 3.f) / 2.f;

			// quart
			case easing::ease_in_quart:
				return t * t * t * t;
			case easing::ease_out_quart:
				return 1.f - cstd::powf(1.f - t, 4.f);
			case easing::ease_in_out_quart:
				return t < 0.5f
					? 8.f * t * t * t * t
					: 1.f - cstd::powf(-2.f * t + 2.f, 4.f) / 2.f;

			// quint
			case easing::ease_in_quint:
				return t * t * t * t * t;
			case easing::ease_out_quint:
				return 1.f - cstd::powf(1.f - t, 5.f);
			case easing::ease_in_out_quint:
				return t < 0.5f
					? 16.f * t * t * t * t * t
					: 1.f - cstd::powf(-2.f * t + 2.f, 5.f) / 2.f;

			// expo
			case easing::ease_in_expo:
				return t == 0.f ? 0.f : cstd::powf(2.f, 10.f * t - 10.f);
			case easing::ease_out_expo:
				return t == 1.f ? 1.f : 1.f - cstd::powf(2.f, -10.f * t);
			case easing::ease_in_out_expo:
				if (t == 0.f) return 0.f;
				if (t == 1.f) return 1.f;
				return t < 0.5f
					? cstd::powf(2.f, 20.f * t - 10.f) / 2.f
					: (2.f - cstd::powf(2.f, -20.f * t + 10.f)) / 2.f;

			// circ
			case easing::ease_in_circ:
				return 1.f - cstd::sqrtf(1.f - t * t);
			case easing::ease_out_circ:
				return cstd::sqrtf(1.f - (t - 1.f) * (t - 1.f));
			case easing::ease_in_out_circ:
				return t < 0.5f
					? (1.f - cstd::sqrtf(1.f - cstd::powf(2.f * t, 2.f))) / 2.f
					: (cstd::sqrtf(1.f - cstd::powf(-2.f * t + 2.f, 2.f)) + 1.f) / 2.f;

			// back
			case easing::ease_in_back:
				return c3 * t * t * t - c1 * t * t;
			case easing::ease_out_back:
				return 1.f + c3 * cstd::powf(t - 1.f, 3.f) + c1 * cstd::powf(t - 1.f, 2.f);
			case easing::ease_in_out_back:
				return t < 0.5f
					? (cstd::powf(2.f * t, 2.f) * ((c2 + 1.f) * 2.f * t - c2)) / 2.f
					: (cstd::powf(2.f * t - 2.f, 2.f) * ((c2 + 1.f) * (2.f * t - 2.f) + c2) + 2.f) / 2.f;

			// elastic
			case easing::ease_in_elastic:
				if (t == 0.f) return 0.f;
				if (t == 1.f) return 1.f;
				return -cstd::powf(2.f, 10.f * t - 10.f) * cstd::sinf((10.f * t - 10.75f) * c4);
			case easing::ease_out_elastic:
				if (t == 0.f) return 0.f;
				if (t == 1.f) return 1.f;
				return cstd::powf(2.f, -10.f * t) * cstd::sinf((10.f * t - 0.75f) * c4) + 1.f;
			case easing::ease_in_out_elastic:
				if (t == 0.f) return 0.f;
				if (t == 1.f) return 1.f;
				return t < 0.5f
					? -(cstd::powf(2.f, 20.f * t - 10.f) * cstd::sinf((20.f * t - 11.125f) * c5)) / 2.f
					: (cstd::powf(2.f, -20.f * t + 10.f) * cstd::sinf((20.f * t - 11.125f) * c5)) / 2.f + 1.f;

			// bounce
			case easing::ease_out_bounce:
				return bounce_out(t);
			case easing::ease_in_bounce:
				return 1.f - bounce_out(1.f - t);
			case easing::ease_in_out_bounce:
				return t < 0.5f
					? (1.f - bounce_out(1.f - 2.f * t)) / 2.f
					: (1.f + bounce_out(2.f * t - 1.f)) / 2.f;
		}

		return t;
	}

	[[nodiscard]] inline float lerp(const float a, const float b, const float t) noexcept
	{
		return a + (b - a) * t;
	}

	[[nodiscard]] inline color lerp_color(const color& a, const color& b, const float t) noexcept
	{
		// assign directly to avoid the color constructor's >1.0 normalization
		// which would break with easing overshoot (back/elastic)
		color result;
		result.r = a.r + (b.r - a.r) * t;
		result.g = a.g + (b.g - a.g) * t;
		result.b = a.b + (b.b - a.b) * t;
		result.a = cstd::fmaxf(0.f, cstd::fminf(1.f, a.a + (b.a - a.a) * t));

		return result;
	}

	[[nodiscard]] inline position lerp_position(const position& a, const position& b, const float t) noexcept
	{
		return position{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
	}

	struct keyframe_props
	{
		optional_t<color> col;
		optional_t<float> opacity;
		optional_t<float> rounding;
		optional_t<position> offset;
	};

	struct keyframe
	{
		float progress = 0.f;
		keyframe_props props;
		easing ease = easing::linear;
	};

	struct keyframe_sequence
	{
		vector_t<keyframe> frames;

		keyframe_sequence& add(const float progress, const keyframe_props props,
		                       const easing ease = easing::linear)
		{
			frames.push_back(keyframe{ progress, props, ease });

			std::sort(frames.begin(), frames.end(), [](const keyframe& a, const keyframe& b)
			{
				return a.progress < b.progress;
			});

			return *this;
		}
	};

	enum class animation_direction : cstd::uint8_t
	{
		normal,
		reverse,
		alternate
	};

	enum class fill_mode : cstd::uint8_t
	{
		none,
		forwards
	};

	struct animation_options
	{
		float duration = 1.f;
		easing ease = easing::linear;
		float delay = 0.f;
		cstd::int32_t iterations = 1;
		animation_direction direction = animation_direction::normal;
		fill_mode fill = fill_mode::forwards;
		function_t<void()> on_complete;
	};

	class animation_state
	{
	public:
		animation_state(keyframe_sequence seq, animation_options opts)
				:	sequence_(cstd::move(seq)), options_(cstd::move(opts)),
					reverse_(opts.direction == animation_direction::reverse) { }

		void update(const float dt)
		{
			if (finished_)
			{
				return;
			}

			elapsed_ += dt;

			const float active_time = elapsed_ - options_.delay;

			if (active_time < 0.f)
			{
				return;
			}

			if (options_.duration <= 0.f)
			{
				finished_ = true;
				current_progress_ = reverse_ ? 0.f : 1.f;

				if (options_.on_complete)
				{
					options_.on_complete();
				}

				return;
			}

			float raw = active_time / options_.duration;

			if (raw >= 1.f)
			{
				if (options_.iterations == -1 || current_iteration_ < options_.iterations - 1)
				{
					current_iteration_++;
					elapsed_ = options_.delay + cstd::fmodf(active_time, options_.duration);

					if (options_.direction == animation_direction::alternate)
					{
						reverse_ = !reverse_;
					}

					raw = (elapsed_ - options_.delay) / options_.duration;
				}
				else
				{
					raw = 1.f;
					finished_ = true;

					if (options_.on_complete)
					{
						options_.on_complete();
					}
				}
			}

			raw = cstd::fmaxf(0.f, cstd::fminf(1.f, raw));

			float progress = reverse_ ? 1.f - raw : raw;

			progress = apply_easing(options_.ease, progress);

			current_progress_ = progress;
		}

		[[nodiscard]] bool is_finished() const noexcept
		{
			return finished_;
		}

		[[nodiscard]] fill_mode get_fill_mode() const noexcept
		{
			return options_.fill;
		}

		[[nodiscard]] keyframe_props current_props() const
		{
			return interpolate(current_progress_);
		}

	private:
		[[nodiscard]] keyframe_props interpolate(const float progress) const
		{
			const auto& frames = sequence_.frames;

			if (frames.empty())
			{
				return {};
			}

			if (frames.size() == 1)
			{
				return frames[0].props;
			}

			// find bracketing keyframes
			if (progress <= frames.front().progress)
			{
				return frames.front().props;
			}

			if (progress >= frames.back().progress)
			{
				return frames.back().props;
			}

			cstd::size_t i = 0;

			for (; i < frames.size() - 1; i++)
			{
				if (progress >= frames[i].progress && progress <= frames[i + 1].progress)
				{
					break;
				}
			}

			const auto& a = frames[i];
			const auto& b = frames[i + 1];

			const float span = b.progress - a.progress;

			if (span <= 0.f)
			{
				return a.props;
			}

			float local_t = (progress - a.progress) / span;

			local_t = apply_easing(a.ease, local_t);

			// interpolate each optional property
			keyframe_props result;

			if (a.props.col && b.props.col)
			{
				result.col = lerp_color(*a.props.col, *b.props.col, local_t);
			}
			else if (a.props.col)
			{
				result.col = *a.props.col;
			}
			else if (b.props.col)
			{
				result.col = *b.props.col;
			}

			if (a.props.opacity && b.props.opacity)
			{
				result.opacity = lerp(*a.props.opacity, *b.props.opacity, local_t);
			}
			else if (a.props.opacity)
			{
				result.opacity = *a.props.opacity;
			}
			else if (b.props.opacity)
			{
				result.opacity = *b.props.opacity;
			}

			if (a.props.rounding && b.props.rounding)
			{
				result.rounding = lerp(*a.props.rounding, *b.props.rounding, local_t);
			}
			else if (a.props.rounding)
			{
				result.rounding = *a.props.rounding;
			}
			else if (b.props.rounding)
			{
				result.rounding = *b.props.rounding;
			}

			if (a.props.offset && b.props.offset)
			{
				result.offset = lerp_position(*a.props.offset, *b.props.offset, local_t);
			}
			else if (a.props.offset)
			{
				result.offset = *a.props.offset;
			}
			else if (b.props.offset)
			{
				result.offset = *b.props.offset;
			}

			return result;
		}

		keyframe_sequence sequence_;
		animation_options options_;
		float elapsed_ = 0.f;
		float current_progress_ = 0.f;
		cstd::int32_t current_iteration_ = 0;
		bool finished_ = false;
		bool reverse_ = false;
	};
}
