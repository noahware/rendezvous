#include "render.hpp"
#include "texture.hpp"
#include "../util/file.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace rv
{
	shared_ptr_t<texture> renderer::load_texture_from_memory(const span_t<const cstd::uint8_t> encoded)
	{
		// stbi_load_from_memory takes an int length; guard the narrowing cast.
		if (encoded.empty() || encoded.size() > 0x7FFFFFFFu)
		{
			return { };
		}

		int width = 0;
		int height = 0;
		int channels = 0;

		// force 4 channels so the decoded buffer always matches create_texture's RGBA8 contract.
		stbi_uc* pixels = stbi_load_from_memory(encoded.data(), static_cast<int>(encoded.size()),
		                                        &width, &height, &channels, 4);

		if (!pixels || width <= 0 || height <= 0)
		{
			if (pixels)
			{
				stbi_image_free(pixels);
			}

			return { };
		}

		const span_t<const cstd::uint8_t> rgba{ pixels,
			static_cast<cstd::size_t>(width) * static_cast<cstd::size_t>(height) * 4u };

		// both backends copy the pixels at upload time, so freeing immediately after is safe.
		auto tex = create_texture(rgba, static_cast<cstd::uint32_t>(width), static_cast<cstd::uint32_t>(height));

		stbi_image_free(pixels);

		return tex;
	}

	shared_ptr_t<texture> renderer::load_texture(const string_t& path)
	{
		const auto bytes = read_file(path);

		if (bytes.empty())
		{
			return { };
		}

		return load_texture_from_memory(bytes);
	}
}
