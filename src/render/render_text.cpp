#include "render.hpp"
#include "texture.hpp"
#ifdef RV_USE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H
#else
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#endif
#include "../util/file.hpp"
#include "../util/string.hpp"

using rv::decode_utf8;

void rv::renderer::draw_text(const font &font, const position pos, const string_view_t text, const color col,
                             const float size, const float letter_spacing) noexcept
{
    const shared_ptr_t<texture> font_texture = font.texture();

    if (text.empty() || !font_texture)
    {
        return;
    }

    current_texture_ = font_texture;

    const float scale = size != 0.f ? size / font.baked_size() : 1.f;
    const float baseline = pos.y + font.ascent() * scale;

    float pen = cstd::floorf(pos.x);
    float line_y = cstd::floorf(baseline);

    const char *s = text.data();
    const char *end = s + text.size();

    // Reserve worst case (every byte a visible glyph) and write glyph quads straight into the
    // pending buffers; trim the unused tail afterwards. No per-call heap allocation or copy.
    const vertex_writer w = reserve_indexed(text.size() * 4, text.size() * 6, shader_type::default_shader);

    cstd::uint32_t prev_codepoint = 0;
    cstd::size_t vcount = 0;
    cstd::size_t icount = 0;

    while (s < end)
    {
        const cstd::uint32_t codepoint = decode_utf8(s, end);

        // handle newlines
        if (codepoint == '\n')
        {
            pen = pos.x;
            line_y += font.line_height() * scale;
            prev_codepoint = 0;
            continue;
        }

        if (codepoint == '\r')
        {
            prev_codepoint = 0;
            continue;
        }

        if (prev_codepoint != 0)
        {
            pen += font.kerning(prev_codepoint, codepoint) * scale;
        }

        const glyph &g = font.glyph(codepoint);

        if (g.size.x > 0.f && g.size.y > 0.f)
        {
            const float x0 = pen + g.bearing.x * scale;
            const float y0 = line_y + g.bearing.y * scale;
            const float x1 = x0 + g.size.x * scale;
            const float y1 = y0 + g.size.y * scale;

            const ndc_position a = to_ndc({x0, y0});
            const ndc_position b = to_ndc({x1, y1});

            const cstd::uint32_t base = w.base_index + static_cast<cstd::uint32_t>(vcount);

            w.vertices[vcount + 0] = vertex{.pos = {a.x, a.y}, .col = pack_color(col), .uv = {g.uv0.x, g.uv0.y}};
            w.vertices[vcount + 1] = vertex{.pos = {b.x, a.y}, .col = pack_color(col), .uv = {g.uv1.x, g.uv0.y}};
            w.vertices[vcount + 2] = vertex{.pos = {b.x, b.y}, .col = pack_color(col), .uv = {g.uv1.x, g.uv1.y}};
            w.vertices[vcount + 3] = vertex{.pos = {a.x, b.y}, .col = pack_color(col), .uv = {g.uv0.x, g.uv1.y}};

            w.indices[icount + 0] = base;
            w.indices[icount + 1] = base + 1;
            w.indices[icount + 2] = base + 2;
            w.indices[icount + 3] = base;
            w.indices[icount + 4] = base + 2;
            w.indices[icount + 5] = base + 3;

            vcount += 4;
            icount += 6;
        }

        pen += g.advance * scale + letter_spacing;
        prev_codepoint = codepoint;
    }

    shrink_reserved(vcount, icount);

    current_texture_ = default_texture_;
}

void rv::renderer::add_text_shadow(const font &font, const position pos, const string_view_t text, const color col,
                                   const float shadow_blur, const float size, const bool cut_background) noexcept
{
    const shared_ptr_t<texture> font_texture = font.texture();

    if (text.empty() || !font_texture)
    {
        return;
    }

    current_texture_ = font_texture;

    const float scale = size != 0.f ? size / font.baked_size() : 1.f;
    const float baseline = pos.y + font.ascent() * scale;

    float pen = cstd::floorf(pos.x);
    float line_y = cstd::floorf(baseline);

    const char *s = text.data();
    const char *end = s + text.size();

    // Reserve worst case and write glyph-shadow quads directly into the pending buffers;
    // trim the unused tail afterwards. No per-call heap allocation or copy.
    const vertex_writer w = reserve_indexed(text.size() * 4, text.size() * 6, shader_type::text_shadow_shader);

    cstd::uint32_t prev_codepoint = 0;
    cstd::size_t vcount = 0;
    cstd::size_t icount = 0;

    while (s < end)
    {
        const cstd::uint32_t codepoint = decode_utf8(s, end);

        if (codepoint == '\n')
        {
            pen = pos.x;
            line_y += font.line_height() * scale;
            prev_codepoint = 0;
            continue;
        }

        if (codepoint == '\r')
        {
            prev_codepoint = 0;
            continue;
        }

        if (prev_codepoint != 0)
        {
            pen += font.kerning(prev_codepoint, codepoint) * scale;
        }

        const glyph &g = font.glyph(codepoint);

        if (g.size.x > 0.f && g.size.y > 0.f)
        {
            const float x0 = pen + g.bearing.x * scale - shadow_blur;
            const float y0 = line_y + g.bearing.y * scale - shadow_blur;
            const float x1 = pen + g.bearing.x * scale + g.size.x * scale + shadow_blur;
            const float y1 = line_y + g.bearing.y * scale + g.size.y * scale + shadow_blur;

            const ndc_position a = to_ndc({x0, y0});
            const ndc_position b = to_ndc({x1, y1});

            const float du_per_pixel = (g.uv1.x - g.uv0.x) / (g.size.x * scale);
            const float dv_per_pixel = (g.uv1.y - g.uv0.y) / (g.size.y * scale);

            const float u0 = g.uv0.x - shadow_blur * du_per_pixel;
            const float v0 = g.uv0.y - shadow_blur * dv_per_pixel;
            const float u1 = g.uv1.x + shadow_blur * du_per_pixel;
            const float v1 = g.uv1.y + shadow_blur * dv_per_pixel;

            const array_t<float, 8> data = {
                shadow_blur, du_per_pixel, dv_per_pixel, cut_background ? 1.f : 0.f,
                g.uv0.x, g.uv0.y, g.uv1.x, g.uv1.y
            };

            const cstd::uint32_t base = w.base_index + static_cast<cstd::uint32_t>(vcount);

            w.vertices[vcount + 0] = vertex{.pos = {a.x, a.y}, .col = pack_color(col), .uv = {u0, v0}, .custom_data = data};
            w.vertices[vcount + 1] = vertex{.pos = {b.x, a.y}, .col = pack_color(col), .uv = {u1, v0}, .custom_data = data};
            w.vertices[vcount + 2] = vertex{.pos = {b.x, b.y}, .col = pack_color(col), .uv = {u1, v1}, .custom_data = data};
            w.vertices[vcount + 3] = vertex{.pos = {a.x, b.y}, .col = pack_color(col), .uv = {u0, v1}, .custom_data = data};

            w.indices[icount + 0] = base;
            w.indices[icount + 1] = base + 1;
            w.indices[icount + 2] = base + 2;
            w.indices[icount + 3] = base;
            w.indices[icount + 4] = base + 2;
            w.indices[icount + 5] = base + 3;

            vcount += 4;
            icount += 6;
        }

        pen += g.advance * scale;
        prev_codepoint = codepoint;
    }

    shrink_reserved(vcount, icount);

    current_texture_ = default_texture_;
}

rv::position rv::renderer::calc_text_size(const font &font, const string_view_t text, const float size) const noexcept
{
    if (text.empty() || !font.texture())
    {
        return {0.f, 0.f};
    }

    const float scale = size != 0.f ? size / font.baked_size() : 1.f;
    float max_width = 0.f;
    float current_width = 0.f;
    cstd::size_t lines = 1;

    const char *s = text.data();
    const char *end = s + text.size();

    cstd::uint32_t prev_codepoint = 0;

    while (s < end)
    {
        const cstd::uint32_t codepoint = decode_utf8(s, end);

        if (codepoint == '\n')
        {
            max_width = cstd::fmaxf(max_width, current_width);
            current_width = 0.f;
            lines++;
            prev_codepoint = 0;
            continue;
        }

        if (codepoint == '\r')
        {
            prev_codepoint = 0;
            continue;
        }

        if (prev_codepoint != 0)
        {
            current_width += font.kerning(prev_codepoint, codepoint) * scale;
        }

        const glyph &g = font.glyph(codepoint);
        current_width += g.advance * scale;
        prev_codepoint = codepoint;
    }

    max_width = cstd::fmaxf(max_width, current_width);

    return {max_width, static_cast<float>(lines) * font.line_height() * scale};
}

optional_t<rv::font> rv::renderer::add_font(const span_t<const cstd::uint8_t> bytes, const float pixel_height,
                                            const cstd::uint32_t min_char, const cstd::uint32_t max_char, const bool anti_aliased)
{
    // guard against empty/unreadable font data, stbtt/freetype dereference the
    // pointer unconditionally, so an empty buffer would crash.
    if (bytes.empty())
    {
        return {};
    }

    constexpr cstd::uint32_t glyph_padding = 2;

#ifdef RV_USE_FREETYPE
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library))
    {
        return {};
    }

    FT_Face face = nullptr;
    if (FT_New_Memory_Face(library, bytes.data(), static_cast<FT_Long>(bytes.size()), 0, &face))
    {
        FT_Done_FreeType(library);
        return {};
    }
    
    FT_Size_RequestRec req = {};
    req.type = FT_SIZE_REQUEST_TYPE_REAL_DIM;
    req.width = 0;
    req.height = static_cast<FT_Long>(pixel_height * 64.0f);
    req.horiResolution = 0;
    req.vertResolution = 0;

    FT_Request_Size(face, &req);

#define RV_FT_CEIL(X) (((X + 63) & -64) / 64)

    const float ft_ascent = static_cast<float>(RV_FT_CEIL(face->size->metrics.ascender));
    const float ft_descent = static_cast<float>(RV_FT_CEIL(face->size->metrics.descender));
    const float ft_line_height = static_cast<float>(RV_FT_CEIL(face->size->metrics.height));
    const float ft_line_gap = static_cast<float>(RV_FT_CEIL(face->size->metrics.height - face->size->metrics.ascender + face->size->metrics.descender));
    
    const cstd::size_t glyph_count = static_cast<cstd::size_t>(max_char - min_char + 1);
    vector_t<glyph> glyphs(glyph_count);

    const cstd::int32_t load_flags =
        anti_aliased ? (FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT) : (FT_LOAD_RENDER | FT_LOAD_TARGET_MONO | FT_LOAD_MONOCHROME);

    cstd::uint32_t width = 256;
    cstd::uint32_t height = 256;
    vector_t<cstd::uint8_t> coverage;

    bool atlas_ok = false;
    while (!atlas_ok && width <= 8192)
    {
        coverage.assign(static_cast<cstd::size_t>(width) * height, 0);

        cstd::uint32_t pen_x = glyph_padding;
        cstd::uint32_t pen_y = glyph_padding;
        cstd::uint32_t row_height = 0;
        
        bool overflow = false;
        for (cstd::size_t i = 0; i < glyph_count; i++)
        {
            const cstd::uint32_t c = min_char + static_cast<cstd::uint32_t>(i);
            if (FT_Load_Char(face, c, load_flags))
            {
                continue;
            }

            FT_Bitmap *bitmap = &face->glyph->bitmap;
            if (pen_x + bitmap->width + glyph_padding > width)
            {
                pen_x = glyph_padding;
                pen_y += row_height + glyph_padding;
                row_height = 0;
            }
            
            if (bitmap->rows > row_height)
            {
                row_height = bitmap->rows;
            }
            
            if (pen_y + bitmap->rows + glyph_padding > height)
            {
                overflow = true;
                break;
            }

            for (cstd::uint32_t row = 0; row < bitmap->rows; row++)
            {
                for (cstd::uint32_t col = 0; col < bitmap->width; col++)
                {
                    if (bitmap->pixel_mode == FT_PIXEL_MODE_MONO)
                    {
                        const cstd::uint8_t byte = bitmap->buffer[row * bitmap->pitch + (col / 8)];
                        const bool bit = (byte & (1 << (7 - (col % 8)))) != 0;
                        coverage[(pen_y + row) * width + (pen_x + col)] = bit ? 0xFF : 0;
                    }
                    else
                    {
                        coverage[(pen_y + row) * width + (pen_x + col)] = bitmap->buffer[row * bitmap->pitch + col];
                    }
                }
            }

            glyph &g = glyphs[i];
            g.uv0 = {static_cast<float>(pen_x) / static_cast<float>(width),
                     static_cast<float>(pen_y) / static_cast<float>(height)};
            g.uv1 = {static_cast<float>(pen_x + bitmap->width) / static_cast<float>(width),
                     static_cast<float>(pen_y + bitmap->rows) / static_cast<float>(height)};
            g.size = {static_cast<float>(bitmap->width), static_cast<float>(bitmap->rows)};
            
            g.bearing = {static_cast<float>(face->glyph->bitmap_left), -static_cast<float>(face->glyph->bitmap_top)};
            g.advance = static_cast<float>(RV_FT_CEIL(face->glyph->advance.x));

            pen_x += bitmap->width + glyph_padding;
        }

        if (!overflow)
        {
            atlas_ok = true;
        }
        else
        {
            width *= 2;
            height *= 2;

            // reset glyphs for re-rasterization
            for (auto &g : glyphs)
            {
                g = {};
            }
        }
    }

    if (!atlas_ok)
    {
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return {};
    }
    
    vector_t<cstd::uint8_t> rgba(static_cast<cstd::size_t>(width) * height * 4, 0);
    for (cstd::size_t i = 0; i < coverage.size(); i++)
    {
        const cstd::size_t c = i * 4;
        rgba[c] = 0xFF;
        rgba[c + 1] = 0xFF;
        rgba[c + 2] = 0xFF;
        rgba[c + 3] = coverage[i];
    }

    const auto texture = create_texture(rgba, width, height);
    
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    
    if (!texture)
    {
        return {};
    }
    
    unordered_map_t<cstd::uint64_t, float> kerning_table;

    return font{texture,   glyphs,     min_char,       max_char,    pixel_height,
                ft_ascent, ft_descent, ft_line_height, ft_line_gap, cstd::move(kerning_table)};

#undef RV_FT_CEIL
#else
    stbtt_fontinfo info = {};
    if (!stbtt_InitFont(&info, bytes.data(), stbtt_GetFontOffsetForIndex(bytes.data(), 0)))
    {
        return {};
    }

    const float scale = stbtt_ScaleForPixelHeight(&info, pixel_height);

    cstd::int32_t raw_ascent = 0;
    cstd::int32_t raw_descent = 0;
    cstd::int32_t raw_line_gap = 0;

    stbtt_GetFontVMetrics(&info, &raw_ascent, &raw_descent, &raw_line_gap);

    const float stb_ascent = static_cast<float>(raw_ascent) * scale;
    const float stb_descent = static_cast<float>(raw_descent) * scale;
    const float stb_line_gap = static_cast<float>(raw_line_gap) * scale;
    const float stb_line_height = stb_ascent - stb_descent + stb_line_gap;

    const cstd::size_t glyph_count = static_cast<cstd::size_t>(max_char - min_char + 1);
    
    cstd::uint32_t width = 256;
    cstd::uint32_t height = 256;
    
    vector_t<cstd::uint8_t> coverage;
    vector_t<stbtt_packedchar> packed_chars(glyph_count);

    bool atlas_ok = false;
    const cstd::uint32_t oversample = anti_aliased ? 2 : 1;
    while (!atlas_ok && width <= 8192)
    {
        coverage.assign(static_cast<cstd::size_t>(width) * height, 0);

        stbtt_pack_context pack_ctx = {};

        if (!stbtt_PackBegin(&pack_ctx, coverage.data(), static_cast<cstd::int32_t>(width), static_cast<cstd::int32_t>(height), 0,
                             static_cast<cstd::int32_t>(glyph_padding), nullptr))
        {
            width *= 2;
            height *= 2;
            continue;
        }

        stbtt_PackSetOversampling(&pack_ctx, oversample, oversample);

        stbtt_pack_range range = {};
        range.font_size = pixel_height;
        range.first_unicode_codepoint_in_range = static_cast<cstd::int32_t>(min_char);
        range.num_chars = static_cast<cstd::int32_t>(glyph_count);
        range.chardata_for_range = packed_chars.data();

        const cstd::int32_t pack_result = stbtt_PackFontRanges(&pack_ctx, bytes.data(), 0, &range, 1);
        stbtt_PackEnd(&pack_ctx);

        if (pack_result)
        {
            atlas_ok = true;
        }
        else
        {
            width *= 2;
            height *= 2;
        }
    }

    if (!atlas_ok)
    {
        return {};
    }

    if (!anti_aliased)
    {
        for (cstd::size_t i = 0; i < coverage.size(); i++)
        {
            coverage[i] = coverage[i] > 127 ? 0xFF : 0;
        }
    }

    vector_t<cstd::uint8_t> rgba(static_cast<cstd::size_t>(width) * height * 4, 0);

    for (cstd::size_t i = 0; i < coverage.size(); i++)
    {
        const cstd::size_t c = i * 4;

        rgba[c] = 0xFF;
        rgba[c + 1] = 0xFF;
        rgba[c + 2] = 0xFF;
        rgba[c + 3] = coverage[i];
    }

    const auto texture = create_texture(rgba, width, height);
    if (!texture)
    {
        return {};
    }

    vector_t<glyph> glyphs(glyph_count);

    for (cstd::size_t i = 0; i < glyph_count; i++)
    {
        const stbtt_packedchar &pc = packed_chars[i];
        glyph &g = glyphs[i];

        stbtt_aligned_quad q = {};
        float dummy_x = 0.f, dummy_y = 0.f;
        stbtt_GetPackedQuad(packed_chars.data(), static_cast<cstd::int32_t>(width), static_cast<cstd::int32_t>(height), static_cast<cstd::int32_t>(i), &dummy_x,
                            &dummy_y, &q, 0);

        g.uv0 = {q.s0, q.t0};
        g.uv1 = {q.s1, q.t1};
        g.bearing = {q.x0, q.y0};
        g.size = {q.x1 - q.x0, q.y1 - q.y0};
        g.advance = pc.xadvance;
    }

    unordered_map_t<cstd::uint64_t, float> kerning_table;

    for (cstd::uint32_t left = min_char; left <= max_char; left++)
    {
        const cstd::int32_t left_glyph = stbtt_FindGlyphIndex(&info, static_cast<cstd::int32_t>(left));
        if (!left_glyph)
            continue;

        for (cstd::uint32_t right = min_char; right <= max_char; right++)
        {
            const cstd::int32_t right_glyph = stbtt_FindGlyphIndex(&info, static_cast<cstd::int32_t>(right));
            if (!right_glyph)
                continue;

            const cstd::int32_t kern = stbtt_GetGlyphKernAdvance(&info, left_glyph, right_glyph);
            if (kern != 0)
            {
                const cstd::uint64_t key = (static_cast<cstd::uint64_t>(left) << 32) | static_cast<cstd::uint64_t>(right);
                kerning_table[key] = static_cast<float>(kern) * scale;
            }
        }
    }

    return font{texture, glyphs, min_char, max_char, pixel_height, stb_ascent, stb_descent, stb_line_height, 
                stb_line_gap, cstd::move(kerning_table)};
#endif
}

optional_t<rv::font> rv::renderer::add_font(const string_t &path, const float pixel_height, const cstd::uint32_t min_char,
                                            const cstd::uint32_t max_char, const bool anti_aliased)
{
    const vector_t<cstd::uint8_t> buffer = read_file(path);

    return add_font(buffer, pixel_height, min_char, max_char, anti_aliased);
}
