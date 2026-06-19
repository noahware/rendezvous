#pragma once
#include "position.hpp"

namespace rv
{
	class renderer;

	class texture
	{
	public:
		explicit texture(renderer* const renderer, const cstd::uint32_t width = 0, const cstd::uint32_t height = 0) noexcept
				:	renderer_(renderer), width_(width), height_(height) { }

		[[nodiscard]] bool owned_by(const renderer* const renderer) const noexcept
		{
			return renderer_ == renderer;
		}

		// native pixel dimensions; 0 when unknown (e.g. textures adopted from an external SRV).
		[[nodiscard]] cstd::uint32_t width() const noexcept
		{
			return width_;
		}

		[[nodiscard]] cstd::uint32_t height() const noexcept
		{
			return height_;
		}

	protected:
		renderer* renderer_ = nullptr;
		cstd::uint32_t width_ = 0;
		cstd::uint32_t height_ = 0;
	};

	struct glyph
	{
		texture_position uv0;
		texture_position uv1;

		vector_2d<float> size;
		vector_2d<float> bearing;

		float advance = 0.f;
	};

	class font
	{
	public:
		explicit font(shared_ptr_t<class texture> tex, unordered_map_t<cstd::uint32_t, struct glyph> glyphs, const float baked_size,
		              const float ascent, const float descent, const float line_height, const float line_gap,
		              unordered_map_t<cstd::uint64_t, float> kerning_table = { })
				:	texture_(cstd::move(tex)),
					glyphs_(cstd::move(glyphs)),
					baked_size_(baked_size),
					ascent_(ascent),
					descent_(descent),
					line_height_(line_height),
					line_gap_(line_gap),
					kerning_table_(cstd::move(kerning_table)) { }

		[[nodiscard]] const struct glyph& glyph(const cstd::uint32_t c) const
		{
			const auto it = glyphs_.find(c);
			if (it != glyphs_.end())
			{
				return it->second;
			}

			const auto fallback = glyphs_.find('?');
			if (fallback != glyphs_.end())
			{
				return fallback->second;
			}

			if (!glyphs_.empty())
			{
				return glyphs_.begin()->second;
			}

			return fallback_glyph_;
		}

		[[nodiscard]] bool has_glyph(const cstd::uint32_t c) const
		{
			return glyphs_.find(c) != glyphs_.end();
		}

		[[nodiscard]] float kerning(const cstd::uint32_t left, const cstd::uint32_t right) const
		{
			if (kerning_table_.empty())
			{
				return 0.f;
			}

			const cstd::uint64_t key = (static_cast<cstd::uint64_t>(left) << 32) | static_cast<cstd::uint64_t>(right);
			const auto it = kerning_table_.find(key);

			if (it != kerning_table_.end())
			{
				return it->second;
			}

			return 0.f;
		}

		[[nodiscard]] shared_ptr_t<class texture> texture() const
		{
			return texture_;
		}

		[[nodiscard]] float ascent() const noexcept
		{
			return ascent_;
		}

		[[nodiscard]] float descent() const noexcept
		{
			return descent_;
		}

		[[nodiscard]] float line_height() const noexcept
		{
			return line_height_;
		}

		[[nodiscard]] float baked_size() const noexcept
		{
			return baked_size_;
		}

		[[nodiscard]] float line_gap() const noexcept
		{
			return line_gap_;
		}

	protected:
		shared_ptr_t<class texture> texture_;

		unordered_map_t<cstd::uint32_t, struct glyph> glyphs_;
		struct glyph fallback_glyph_;
		float baked_size_;
		float ascent_;
		float descent_;
		float line_height_;
		float line_gap_;

		unordered_map_t<cstd::uint64_t, float> kerning_table_;
	};
}
