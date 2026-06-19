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

void rv::renderer::draw_text(const font &font, const position pos, const string_view_t text_in, const color col,
                             const float size, const float letter_spacing) noexcept
{
    const string_view_t text = text_in;
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
    const cstd::uint32_t packed = pack_color(col);
    const vertex_writer w = reserve_indexed(text.size() * 4, text.size() * 6, shader_type::default_shader);

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
            const float x0 = pen + g.bearing.x * scale;
            const float y0 = line_y + g.bearing.y * scale;
            const float x1 = x0 + g.size.x * scale;
            const float y1 = y0 + g.size.y * scale;

            const ndc_position a = to_ndc({x0, y0});
            const ndc_position b = to_ndc({x1, y1});

            const cstd::uint32_t base = w.base_index + static_cast<cstd::uint32_t>(vcount);

            w.vertices[vcount + 0] = vertex{.pos = {a.x, a.y}, .col = packed, .uv = {g.uv0.x, g.uv0.y}};
            w.vertices[vcount + 1] = vertex{.pos = {b.x, a.y}, .col = packed, .uv = {g.uv1.x, g.uv0.y}};
            w.vertices[vcount + 2] = vertex{.pos = {b.x, b.y}, .col = packed, .uv = {g.uv1.x, g.uv1.y}};
            w.vertices[vcount + 3] = vertex{.pos = {a.x, b.y}, .col = packed, .uv = {g.uv0.x, g.uv1.y}};

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

void rv::renderer::add_text_shadow(const font &font, const position pos, const string_view_t text_in, const color col,
                                   const float shadow_blur, const float size, const bool cut_background) noexcept
{
    const string_view_t text = text_in;
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
    const cstd::uint32_t packed = pack_color(col);
    const float cut_bg = cut_background ? 1.f : 0.f;
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
                shadow_blur, du_per_pixel, dv_per_pixel, cut_bg,
                g.uv0.x, g.uv0.y, g.uv1.x, g.uv1.y
            };

            const cstd::uint32_t base = w.base_index + static_cast<cstd::uint32_t>(vcount);

            w.vertices[vcount + 0] = vertex{.pos = {a.x, a.y}, .col = packed, .uv = {u0, v0}, .custom_data = data};
            w.vertices[vcount + 1] = vertex{.pos = {b.x, a.y}, .col = packed, .uv = {u1, v0}, .custom_data = data};
            w.vertices[vcount + 2] = vertex{.pos = {b.x, b.y}, .col = packed, .uv = {u1, v1}, .custom_data = data};
            w.vertices[vcount + 3] = vertex{.pos = {a.x, b.y}, .col = packed, .uv = {u0, v1}, .custom_data = data};

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

rv::position rv::renderer::calc_text_size(const font &font, const string_view_t text_in, const float size) const noexcept
{
    const string_view_t text = text_in;
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

namespace
{
    vector_t<rv::glyph_range> resolved_ranges(const span_t<const rv::glyph_range> ranges)
    {
        return ranges.empty() ? rv::make_glyph_ranges(32, 126) : vector_t<rv::glyph_range>(ranges.begin(), ranges.end());
    }

    void append_codepoints(vector_t<cstd::uint32_t>& out, const vector_t<rv::glyph_range>& ranges)
    {
        for (auto range : ranges)
        {
            if (range.first > range.last || range.first > 0x10FFFF)
            {
                continue;
            }

            if (range.last > 0x10FFFF)
            {
                range.last = 0x10FFFF;
            }

            for (cstd::uint32_t cp = range.first; ; ++cp)
            {
                if (cp < 0xD800 || cp > 0xDFFF)
                {
                    out.push_back(cp);
                }

                if (cp == range.last)
                {
                    break;
                }
            }
        }
    }

    vector_t<rv::glyph_range> compact_codepoints(vector_t<cstd::uint32_t> cps)
    {
        vector_t<rv::glyph_range> ranges;

        if (cps.empty())
        {
            return ranges;
        }

        cstd::sort(cps.begin(), cps.end());
        cps.erase(cstd::unique(cps.begin(), cps.end()), cps.end());

        cstd::uint32_t first = cps[0];
        cstd::uint32_t last = cps[0];

        for (cstd::size_t i = 1; i < cps.size(); ++i)
        {
            const cstd::uint32_t cp = cps[i];

            if (cp == last + 1)
            {
                last = cp;
                continue;
            }

            ranges.push_back({ first, last });
            first = last = cp;
        }

        ranges.push_back({ first, last });
        return ranges;
    }

    cstd::uint64_t kerning_key(const cstd::uint32_t left, const cstd::uint32_t right) noexcept
    {
        return (static_cast<cstd::uint64_t>(left) << 32) | static_cast<cstd::uint64_t>(right);
    }
}

optional_t<rv::font> rv::renderer::add_font(const span_t<const cstd::uint8_t> bytes, const float pixel_height,
                                            const span_t<const glyph_range> input_ranges, const bool anti_aliased)
{
    if (bytes.empty())
    {
        return {};
    }

    constexpr cstd::uint32_t glyph_padding = 2;
    unordered_map_t<cstd::uint32_t, bool> covered;

    struct font_input
    {
        span_t<const cstd::uint8_t> bytes;
        vector_t<glyph_range> ranges;
    };

    const vector_t<glyph_range> ranges = resolved_ranges(input_ranges);
    const array_t<font_input, 1> input_sources = { font_input{ bytes, ranges } };

#ifdef RV_USE_FREETYPE
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library))
    {
        return {};
    }

    struct ft_source
    {
        FT_Face face = nullptr;
        vector_t<glyph_range> ranges;
    };

    vector_t<ft_source> sources;

    for (const auto& input : input_sources)
    {
        if (input.bytes.empty())
        {
            continue;
        }

        FT_Face face = nullptr;
        if (FT_New_Memory_Face(library, input.bytes.data(), static_cast<FT_Long>(input.bytes.size()), 0, &face))
        {
            continue;
        }

        FT_Size_RequestRec req = {};
        req.type = FT_SIZE_REQUEST_TYPE_REAL_DIM;
        req.height = static_cast<FT_Long>(pixel_height * 64.0f);
        FT_Request_Size(face, &req);

        vector_t<cstd::uint32_t> wanted;
        append_codepoints(wanted, resolved_ranges(input.ranges));

        vector_t<cstd::uint32_t> supported;
        for (const cstd::uint32_t cp : wanted)
        {
            if (covered.find(cp) == covered.end() && FT_Get_Char_Index(face, cp) != 0)
            {
                covered[cp] = true;
                supported.push_back(cp);
            }
        }

        vector_t<glyph_range> ranges = compact_codepoints(cstd::move(supported));
        if (ranges.empty())
        {
            FT_Done_Face(face);
            continue;
        }

        sources.push_back({ face, cstd::move(ranges) });
    }

    if (sources.empty())
    {
        FT_Done_FreeType(library);
        return {};
    }

#define RV_FT_CEIL(X) (((X + 63) & -64) / 64)

    FT_Face metric_face = sources[0].face;
    const float ft_ascent = static_cast<float>(RV_FT_CEIL(metric_face->size->metrics.ascender));
    const float ft_descent = static_cast<float>(RV_FT_CEIL(metric_face->size->metrics.descender));
    const float ft_line_height = static_cast<float>(RV_FT_CEIL(metric_face->size->metrics.height));
    const float ft_line_gap = static_cast<float>(RV_FT_CEIL(metric_face->size->metrics.height - metric_face->size->metrics.ascender + metric_face->size->metrics.descender));

    const cstd::int32_t load_flags =
        anti_aliased ? (FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT) : (FT_LOAD_RENDER | FT_LOAD_TARGET_MONO | FT_LOAD_MONOCHROME);

    cstd::uint32_t width = 256;
    cstd::uint32_t height = 256;
    vector_t<cstd::uint8_t> coverage;
    unordered_map_t<cstd::uint32_t, glyph> glyphs;

    bool atlas_ok = false;
    while (!atlas_ok && width <= 8192)
    {
        coverage.assign(static_cast<cstd::size_t>(width) * height, 0);
        glyphs.clear();

        cstd::uint32_t pen_x = glyph_padding;
        cstd::uint32_t pen_y = glyph_padding;
        cstd::uint32_t row_height = 0;
        bool overflow = false;

        for (const auto& source : sources)
        {
            for (const glyph_range range : source.ranges)
            {
                for (cstd::uint32_t cp = range.first; ; ++cp)
                {
                    if (FT_Load_Char(source.face, cp, load_flags))
                    {
                        if (cp == range.last)
                        {
                            break;
                        }

                        continue;
                    }

                    FT_Bitmap* bitmap = &source.face->glyph->bitmap;
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

                    glyph g;
                    g.uv0 = { static_cast<float>(pen_x) / static_cast<float>(width),
                              static_cast<float>(pen_y) / static_cast<float>(height) };
                    g.uv1 = { static_cast<float>(pen_x + bitmap->width) / static_cast<float>(width),
                              static_cast<float>(pen_y + bitmap->rows) / static_cast<float>(height) };
                    g.size = { static_cast<float>(bitmap->width), static_cast<float>(bitmap->rows) };
                    g.bearing = { static_cast<float>(source.face->glyph->bitmap_left), -static_cast<float>(source.face->glyph->bitmap_top) };
                    g.advance = static_cast<float>(RV_FT_CEIL(source.face->glyph->advance.x));
                    glyphs[cp] = g;

                    pen_x += bitmap->width + glyph_padding;

                    if (cp == range.last)
                    {
                        break;
                    }
                }

                if (overflow)
                {
                    break;
                }
            }

            if (overflow)
            {
                break;
            }
        }

        if (!overflow)
        {
            atlas_ok = true;
        }
        else
        {
            width *= 2;
            height *= 2;
        }
    }

    if (!atlas_ok || glyphs.empty())
    {
        for (const auto& source : sources)
        {
            FT_Done_Face(source.face);
        }

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

    for (const auto& source : sources)
    {
        FT_Done_Face(source.face);
    }

    FT_Done_FreeType(library);

    if (!texture)
    {
        return {};
    }

    unordered_map_t<cstd::uint64_t, float> kerning_table;

    return font{ texture, cstd::move(glyphs), pixel_height, ft_ascent, ft_descent, ft_line_height, ft_line_gap, cstd::move(kerning_table) };

#undef RV_FT_CEIL
#else
    struct stb_source
    {
        span_t<const cstd::uint8_t> bytes;
        stbtt_fontinfo info = {};
        vector_t<glyph_range> ranges;
        vector_t<vector_t<stbtt_packedchar>> packed_chars;
        vector_t<stbtt_pack_range> pack_ranges;
    };

    vector_t<stb_source> sources;

    for (const auto& input : input_sources)
    {
        if (input.bytes.empty())
        {
            continue;
        }

        stbtt_fontinfo info = {};
        if (!stbtt_InitFont(&info, input.bytes.data(), stbtt_GetFontOffsetForIndex(input.bytes.data(), 0)))
        {
            continue;
        }

        vector_t<cstd::uint32_t> wanted;
        append_codepoints(wanted, resolved_ranges(input.ranges));

        vector_t<cstd::uint32_t> supported;
        for (const cstd::uint32_t cp : wanted)
        {
            if (covered.find(cp) == covered.end() && stbtt_FindGlyphIndex(&info, static_cast<cstd::int32_t>(cp)) != 0)
            {
                covered[cp] = true;
                supported.push_back(cp);
            }
        }

        vector_t<glyph_range> ranges = compact_codepoints(cstd::move(supported));
        if (!ranges.empty())
        {
            sources.push_back({ input.bytes, info, cstd::move(ranges) });
        }
    }

    if (sources.empty())
    {
        return {};
    }

    const float metric_scale = stbtt_ScaleForPixelHeight(&sources[0].info, pixel_height);
    cstd::int32_t raw_ascent = 0;
    cstd::int32_t raw_descent = 0;
    cstd::int32_t raw_line_gap = 0;
    stbtt_GetFontVMetrics(&sources[0].info, &raw_ascent, &raw_descent, &raw_line_gap);

    const float stb_ascent = static_cast<float>(raw_ascent) * metric_scale;
    const float stb_descent = static_cast<float>(raw_descent) * metric_scale;
    const float stb_line_gap = static_cast<float>(raw_line_gap) * metric_scale;
    const float stb_line_height = stb_ascent - stb_descent + stb_line_gap;

    cstd::uint32_t width = 256;
    cstd::uint32_t height = 256;
    vector_t<cstd::uint8_t> coverage;
    const cstd::uint32_t oversample = anti_aliased ? 2 : 1;

    bool atlas_ok = false;
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

        bool overflow = false;
        for (auto& source : sources)
        {
            source.packed_chars.clear();
            source.pack_ranges.clear();
            source.packed_chars.reserve(source.ranges.size());
            source.pack_ranges.reserve(source.ranges.size());

            for (const glyph_range range : source.ranges)
            {
                source.packed_chars.emplace_back(static_cast<cstd::size_t>(range.last - range.first + 1));
            }

            for (cstd::size_t i = 0; i < source.ranges.size(); ++i)
            {
                const glyph_range range = source.ranges[i];
                stbtt_pack_range pack_range = {};
                pack_range.font_size = pixel_height;
                pack_range.first_unicode_codepoint_in_range = static_cast<cstd::int32_t>(range.first);
                pack_range.num_chars = static_cast<cstd::int32_t>(range.last - range.first + 1);
                pack_range.chardata_for_range = source.packed_chars[i].data();
                source.pack_ranges.push_back(pack_range);
            }

            if (!stbtt_PackFontRanges(&pack_ctx, source.bytes.data(), 0, source.pack_ranges.data(), static_cast<cstd::int32_t>(source.pack_ranges.size())))
            {
                overflow = true;
                break;
            }
        }

        stbtt_PackEnd(&pack_ctx);

        if (!overflow)
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

    unordered_map_t<cstd::uint32_t, glyph> glyphs;
    for (const auto& source : sources)
    {
        for (cstd::size_t range_idx = 0; range_idx < source.ranges.size(); ++range_idx)
        {
            const glyph_range range = source.ranges[range_idx];
            const auto& packed_chars = source.packed_chars[range_idx];

            for (cstd::uint32_t cp = range.first; ; ++cp)
            {
                const cstd::int32_t idx = static_cast<cstd::int32_t>(cp - range.first);
                const stbtt_packedchar& pc = packed_chars[static_cast<cstd::size_t>(idx)];

                stbtt_aligned_quad q = {};
                float dummy_x = 0.f;
                float dummy_y = 0.f;
                stbtt_GetPackedQuad(packed_chars.data(), static_cast<cstd::int32_t>(width), static_cast<cstd::int32_t>(height), idx, &dummy_x, &dummy_y, &q, 0);

                glyph g;
                g.uv0 = { q.s0, q.t0 };
                g.uv1 = { q.s1, q.t1 };
                g.bearing = { q.x0, q.y0 };
                g.size = { q.x1 - q.x0, q.y1 - q.y0 };
                g.advance = pc.xadvance;
                glyphs[cp] = g;

                if (cp == range.last)
                {
                    break;
                }
            }
        }
    }

    unordered_map_t<cstd::uint64_t, float> kerning_table;
    constexpr cstd::size_t kerning_pair_limit = 512;

    for (const auto& source : sources)
    {
        vector_t<cstd::uint32_t> cps;
        append_codepoints(cps, source.ranges);
        if (cps.size() > kerning_pair_limit)
        {
            continue;
        }

        const float scale = stbtt_ScaleForPixelHeight(&source.info, pixel_height);
        for (const cstd::uint32_t left : cps)
        {
            const cstd::int32_t left_glyph = stbtt_FindGlyphIndex(&source.info, static_cast<cstd::int32_t>(left));
            if (!left_glyph)
            {
                continue;
            }

            for (const cstd::uint32_t right : cps)
            {
                const cstd::int32_t right_glyph = stbtt_FindGlyphIndex(&source.info, static_cast<cstd::int32_t>(right));
                if (!right_glyph)
                {
                    continue;
                }

                const cstd::int32_t kern = stbtt_GetGlyphKernAdvance(&source.info, left_glyph, right_glyph);
                if (kern != 0)
                {
                    kerning_table[kerning_key(left, right)] = static_cast<float>(kern) * scale;
                }
            }
        }
    }

    return font{ texture, cstd::move(glyphs), pixel_height, stb_ascent, stb_descent, stb_line_height, stb_line_gap, cstd::move(kerning_table) };
#endif
}

optional_t<rv::font> rv::renderer::add_font(const span_t<const cstd::uint8_t> bytes, const float pixel_height,
                                            const cstd::uint32_t min_char, const cstd::uint32_t max_char, const bool anti_aliased)
{
    const vector_t<glyph_range> ranges = make_glyph_ranges(min_char, max_char);

    return add_font(bytes, pixel_height, span_t<const glyph_range>(ranges.data(), ranges.size()), anti_aliased);
}

optional_t<rv::font> rv::renderer::add_font(const string_t& path, const float pixel_height,
                                            const span_t<const glyph_range> ranges, const bool anti_aliased)
{
    const vector_t<cstd::uint8_t> buffer = read_file(path);

    return add_font(buffer, pixel_height, ranges, anti_aliased);
}

optional_t<rv::font> rv::renderer::add_font(const string_t& path, const float pixel_height, const cstd::uint32_t min_char,
                                            const cstd::uint32_t max_char, const bool anti_aliased)
{
    const vector_t<cstd::uint8_t> buffer = read_file(path);

    return add_font(buffer, pixel_height, min_char, max_char, anti_aliased);
}
