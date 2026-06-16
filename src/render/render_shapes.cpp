#include "render.hpp"
#include "texture.hpp"

namespace
{
    inline void fill_quad_indices(const rv::vertex_writer& w) noexcept
    {
        w.indices[0] = w.base_index;
        w.indices[1] = w.base_index + 1;
        w.indices[2] = w.base_index + 2;
        w.indices[3] = w.base_index + 1;
        w.indices[4] = w.base_index + 4;
        w.indices[5] = w.base_index + 2;
    }

    inline void fill_sequential_indices(const rv::vertex_writer &w, const cstd::uint32_t count) noexcept
    {
        for (cstd::uint32_t i = 0; i < count; ++i)
        {
            w.indices[i] = w.base_index + i;
        }
    }
}

void rv::renderer::draw_rect(const position min, const position max, const color col, const float thickness,
                             const float rounding) noexcept
{
    const float width = max.x - min.x;
    const float height = max.y - min.y;

    const float cx = min.x + width * 0.5f;
    const float cy = min.y + height * 0.5f;

    const float qw = (width * 0.5f) + 1.f;
    const float qh = (height * 0.5f) + 1.f;

    const ndc_position n0 = to_ndc({cx - qw, cy - qh});
    const ndc_position n1 = to_ndc({cx + qw, cy + qh});

    const float r = rounding;
    const array_t<float, 8> data = {width, height, thickness, 0.f, r, r, r, r};
    const cstd::uint32_t packed = pack_color(col);

    const vertex_writer w = reserve_indexed(6, 6, shader_type::rect_shader);

    w.vertices[0] = vertex{.pos = {n0.x, n0.y}, .col = packed, .uv = {-qw, -qh}, .custom_data = data};
    w.vertices[1] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[2] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};
    w.vertices[3] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[4] = vertex{.pos = {n1.x, n1.y}, .col = packed, .uv = { qw,  qh}, .custom_data = data};
    w.vertices[5] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};

    fill_sequential_indices(w, 6);
}

void rv::renderer::draw_rect_filled(const position min, const position max, const color col, const float rounding,
                                    const rounding_flags flags) noexcept
{
    draw_rect_filled_multi_color(min, max, col, col, col, col, rounding, flags);
}

void rv::renderer::draw_rect_filled_multi_color(const position min, const position max, const color col_tl, const color col_tr,
                                                const color col_br, const color col_bl, const float rounding,
                                                const rounding_flags flags) noexcept
{
    const float width = max.x - min.x;
    const float height = max.y - min.y;

    const float cx = min.x + width * 0.5f;
    const float cy = min.y + height * 0.5f;

    const float qw = (width * 0.5f) + 1.f;
    const float qh = (height * 0.5f) + 1.f;

    const ndc_position n0 = to_ndc({cx - qw, cy - qh});
    const ndc_position n1 = to_ndc({cx + qw, cy + qh});

    const float rtl = (flags & rounding_flags_top_left) ? rounding : 0.f;
    const float rtr = (flags & rounding_flags_top_right) ? rounding : 0.f;
    const float rbr = (flags & rounding_flags_bottom_right) ? rounding : 0.f;
    const float rbl = (flags & rounding_flags_bottom_left) ? rounding : 0.f;

    const array_t<float, 8> data = {width, height, 0.f, 0.f, rtr, rbr, rbl, rtl};

    const cstd::uint32_t packed_tl = pack_color(col_tl);
    const cstd::uint32_t packed_tr = pack_color(col_tr);
    const cstd::uint32_t packed_br = pack_color(col_br);
    const cstd::uint32_t packed_bl = pack_color(col_bl);

    const vertex_writer w = reserve_indexed(6, 6, shader_type::rect_shader);

    w.vertices[0] = vertex{.pos = {n0.x, n0.y}, .col = packed_tl, .uv = {-qw, -qh}, .custom_data = data};
    w.vertices[1] = vertex{.pos = {n1.x, n0.y}, .col = packed_tr, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[2] = vertex{.pos = {n0.x, n1.y}, .col = packed_bl, .uv = {-qw,  qh}, .custom_data = data};
    w.vertices[3] = vertex{.pos = {n1.x, n0.y}, .col = packed_tr, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[4] = vertex{.pos = {n1.x, n1.y}, .col = packed_br, .uv = { qw,  qh}, .custom_data = data};
    w.vertices[5] = vertex{.pos = {n0.x, n1.y}, .col = packed_bl, .uv = {-qw,  qh}, .custom_data = data};

    fill_sequential_indices(w, 6);
}

void rv::renderer::draw_rect_filled(const position min, const position max, const color col, const corner_radii radii) noexcept
{
    const float width = max.x - min.x;
    const float height = max.y - min.y;

    const float cx = min.x + width * 0.5f;
    const float cy = min.y + height * 0.5f;

    const float qw = (width * 0.5f) + 1.f;
    const float qh = (height * 0.5f) + 1.f;

    const ndc_position n0 = to_ndc({cx - qw, cy - qh});
    const ndc_position n1 = to_ndc({cx + qw, cy + qh});

    const array_t<float, 8> data = {width, height, 0.f, 0.f, radii.tr, radii.br, radii.bl, radii.tl};
    const cstd::uint32_t packed = pack_color(col);

    const vertex_writer w = reserve_indexed(6, 6, shader_type::rect_shader);

    w.vertices[0] = vertex{.pos = {n0.x, n0.y}, .col = packed, .uv = {-qw, -qh}, .custom_data = data};
    w.vertices[1] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[2] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};
    w.vertices[3] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[4] = vertex{.pos = {n1.x, n1.y}, .col = packed, .uv = { qw,  qh}, .custom_data = data};
    w.vertices[5] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};

    fill_sequential_indices(w, 6);
}

void rv::renderer::draw_rect_filled_multi_color(const position min, const position max, const color col_tl,
                                                const color col_tr, const color col_br, const color col_bl,
                                                const corner_radii radii) noexcept
{
    const float width = max.x - min.x;
    const float height = max.y - min.y;

    const float cx = min.x + width * 0.5f;
    const float cy = min.y + height * 0.5f;

    const float qw = (width * 0.5f) + 1.f;
    const float qh = (height * 0.5f) + 1.f;

    const ndc_position n0 = to_ndc({cx - qw, cy - qh});
    const ndc_position n1 = to_ndc({cx + qw, cy + qh});

    const array_t<float, 8> data = {width, height, 0.f, 0.f, radii.tr, radii.br, radii.bl, radii.tl};

    const cstd::uint32_t packed_tl = pack_color(col_tl);
    const cstd::uint32_t packed_tr = pack_color(col_tr);
    const cstd::uint32_t packed_br = pack_color(col_br);
    const cstd::uint32_t packed_bl = pack_color(col_bl);

    const vertex_writer w = reserve_indexed(6, 6, shader_type::rect_shader);

    w.vertices[0] = vertex{.pos = {n0.x, n0.y}, .col = packed_tl, .uv = {-qw, -qh}, .custom_data = data};
    w.vertices[1] = vertex{.pos = {n1.x, n0.y}, .col = packed_tr, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[2] = vertex{.pos = {n0.x, n1.y}, .col = packed_bl, .uv = {-qw,  qh}, .custom_data = data};
    w.vertices[3] = vertex{.pos = {n1.x, n0.y}, .col = packed_tr, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[4] = vertex{.pos = {n1.x, n1.y}, .col = packed_br, .uv = { qw,  qh}, .custom_data = data};
    w.vertices[5] = vertex{.pos = {n0.x, n1.y}, .col = packed_bl, .uv = {-qw,  qh}, .custom_data = data};

    fill_sequential_indices(w, 6);
}

void rv::renderer::draw_rect(const position min, const position max, const color col, const float thickness,
                             const corner_radii radii) noexcept
{
    const float width = max.x - min.x;
    const float height = max.y - min.y;

    const float cx = min.x + width * 0.5f;
    const float cy = min.y + height * 0.5f;

    const float qw = (width * 0.5f) + 1.f;
    const float qh = (height * 0.5f) + 1.f;

    const ndc_position n0 = to_ndc({cx - qw, cy - qh});
    const ndc_position n1 = to_ndc({cx + qw, cy + qh});

    const array_t<float, 8> data = {width, height, thickness, 0.f, radii.tr, radii.br, radii.bl, radii.tl};
    const cstd::uint32_t packed = pack_color(col);

    const vertex_writer w = reserve_indexed(6, 6, shader_type::rect_shader);

    w.vertices[0] = vertex{.pos = {n0.x, n0.y}, .col = packed, .uv = {-qw, -qh}, .custom_data = data};
    w.vertices[1] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[2] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};
    w.vertices[3] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[4] = vertex{.pos = {n1.x, n1.y}, .col = packed, .uv = { qw,  qh}, .custom_data = data};
    w.vertices[5] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};

    fill_sequential_indices(w, 6);
}

void rv::renderer::draw_shadow_rect(const position min, const position max, const color col, const float rounding,
                                    const float shadow_blur, const float shadow_spread, const rounding_flags flags,
                                    const bool cut_background) noexcept
{
    const float width = max.x - min.x;
    const float height = max.y - min.y;

    const float effective_width = cstd::fmaxf(0.f, width + 2.f * shadow_spread);
    const float effective_height = cstd::fmaxf(0.f, height + 2.f * shadow_spread);
    const float effective_rounding = cstd::fmaxf(0.f, rounding + shadow_spread);

    const float cx = min.x + width * 0.5f;
    const float cy = min.y + height * 0.5f;

    const float qw = (effective_width * 0.5f) + shadow_blur;
    const float qh = (effective_height * 0.5f) + shadow_blur;

    const ndc_position n0 = to_ndc({cx - qw, cy - qh});
    const ndc_position n1 = to_ndc({cx + qw, cy + qh});

    const float rtl = (flags & rounding_flags_top_left) ? effective_rounding : 0.f;
    const float rtr = (flags & rounding_flags_top_right) ? effective_rounding : 0.f;
    const float rbr = (flags & rounding_flags_bottom_right) ? effective_rounding : 0.f;
    const float rbl = (flags & rounding_flags_bottom_left) ? effective_rounding : 0.f;

    const array_t<float, 8> data =
    {
        effective_width, effective_height, cut_background ? 1.f : 0.f, shadow_blur, rtr, rbr, rbl, rtl
    };
    const cstd::uint32_t packed = pack_color(col);

    const vertex_writer w = reserve_indexed(6, 6, shader_type::shadow_shader);

    w.vertices[0] = vertex{.pos = {n0.x, n0.y}, .col = packed, .uv = {-qw, -qh}, .custom_data = data};
    w.vertices[1] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[2] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};
    w.vertices[3] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[4] = vertex{.pos = {n1.x, n1.y}, .col = packed, .uv = { qw,  qh}, .custom_data = data};
    w.vertices[5] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};

    fill_sequential_indices(w, 6);
}

void rv::renderer::draw_shadow_rect_multi_color(const position min, const position max, const color col_tl,
                                                const color col_tr, const color col_br, const color col_bl,
                                                const float rounding, const float shadow_blur, const float shadow_spread,
                                                const rounding_flags flags, const bool cut_background) noexcept
{
    const float width = max.x - min.x;
    const float height = max.y - min.y;

    const float effective_width = cstd::fmaxf(0.f, width + 2.f * shadow_spread);
    const float effective_height = cstd::fmaxf(0.f, height + 2.f * shadow_spread);
    const float effective_rounding = cstd::fmaxf(0.f, rounding + shadow_spread);

    const float cx = min.x + width * 0.5f;
    const float cy = min.y + height * 0.5f;

    const float qw = (effective_width * 0.5f) + shadow_blur;
    const float qh = (effective_height * 0.5f) + shadow_blur;

    const ndc_position n0 = to_ndc({cx - qw, cy - qh});
    const ndc_position n1 = to_ndc({cx + qw, cy + qh});

    const float rtl = (flags & rounding_flags_top_left) ? effective_rounding : 0.f;
    const float rtr = (flags & rounding_flags_top_right) ? effective_rounding : 0.f;
    const float rbr = (flags & rounding_flags_bottom_right) ? effective_rounding : 0.f;
    const float rbl = (flags & rounding_flags_bottom_left) ? effective_rounding : 0.f;

    const array_t<float, 8> data =
    {
        effective_width, effective_height, cut_background ? 1.f : 0.f, shadow_blur, rtr, rbr, rbl, rtl
    };

    const cstd::uint32_t packed_tl = pack_color(col_tl);
    const cstd::uint32_t packed_tr = pack_color(col_tr);
    const cstd::uint32_t packed_br = pack_color(col_br);
    const cstd::uint32_t packed_bl = pack_color(col_bl);

    const vertex_writer w = reserve_indexed(6, 6, shader_type::shadow_shader);

    w.vertices[0] = vertex{.pos = {n0.x, n0.y}, .col = packed_tl, .uv = {-qw, -qh}, .custom_data = data};
    w.vertices[1] = vertex{.pos = {n1.x, n0.y}, .col = packed_tr, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[2] = vertex{.pos = {n0.x, n1.y}, .col = packed_bl, .uv = {-qw,  qh}, .custom_data = data};
    w.vertices[3] = vertex{.pos = {n1.x, n0.y}, .col = packed_tr, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[4] = vertex{.pos = {n1.x, n1.y}, .col = packed_br, .uv = { qw,  qh}, .custom_data = data};
    w.vertices[5] = vertex{.pos = {n0.x, n1.y}, .col = packed_bl, .uv = {-qw,  qh}, .custom_data = data};

    fill_sequential_indices(w, 6);
}

void rv::renderer::draw_line(const position a, const position b, const color col, const float thickness) noexcept
{
    add_path_point(a);
    add_path_point(b);

    draw_lined_path(col, thickness, false);
}

void rv::renderer::draw_shadow_line(const position a, const position b, const color col, const float thickness,
                                    const float shadow_blur) noexcept
{
    add_path_point(a);
    add_path_point(b);

    draw_shadow_lined_path(col, thickness, shadow_blur, false);
}

void rv::renderer::draw_circle(const position pos, const float radius, const color col, const float thickness,
                               const cstd::size_t segment_count) noexcept
{
    const float qw = radius + (thickness * 0.5f) + 1.f;
    const float qh = radius + (thickness * 0.5f) + 1.f;

    const ndc_position n0 = to_ndc({pos.x - qw, pos.y - qh});
    const ndc_position n1 = to_ndc({pos.x + qw, pos.y + qh});

    const array_t<float, 8> data = {radius * 2.f, radius * 2.f, -thickness, 0.f, radius, radius, radius, radius};
    const cstd::uint32_t packed = pack_color(col);

    const vertex_writer w = reserve_indexed(6, 6, shader_type::rect_shader);

    w.vertices[0] = vertex{.pos = {n0.x, n0.y}, .col = packed, .uv = {-qw, -qh}, .custom_data = data};
    w.vertices[1] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[2] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};
    w.vertices[3] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[4] = vertex{.pos = {n1.x, n1.y}, .col = packed, .uv = { qw,  qh}, .custom_data = data};
    w.vertices[5] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};

    fill_sequential_indices(w, 6);
}

void rv::renderer::draw_circle_filled(const position pos, const float radius, const color col,
                                      const cstd::size_t segment_count) noexcept
{
    const float qw = radius + 1.f;
    const float qh = radius + 1.f;

    const ndc_position n0 = to_ndc({pos.x - qw, pos.y - qh});
    const ndc_position n1 = to_ndc({pos.x + qw, pos.y + qh});

    const array_t<float, 8> data = {radius * 2.f, radius * 2.f, 0.f, 0.f, radius, radius, radius, radius};
    const cstd::uint32_t packed = pack_color(col);

    const vertex_writer w = reserve_indexed(6, 6, shader_type::rect_shader);

    w.vertices[0] = vertex{.pos = {n0.x, n0.y}, .col = packed, .uv = {-qw, -qh}, .custom_data = data};
    w.vertices[1] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[2] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};
    w.vertices[3] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[4] = vertex{.pos = {n1.x, n1.y}, .col = packed, .uv = { qw,  qh}, .custom_data = data};
    w.vertices[5] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};

    fill_sequential_indices(w, 6);
}

void rv::renderer::draw_circle_filled_radial(const position pos, const float radius, const color col_in, const color col_out,
                                             const cstd::size_t segment_count) noexcept
{
    const float qw = radius + 1.f;
    const float qh = radius + 1.f;

    const ndc_position n0 = to_ndc({pos.x - qw, pos.y - qh});
    const ndc_position n1 = to_ndc({pos.x + qw, pos.y + qh});

    const array_t<float, 8> data = {radius * 2.f, radius * 2.f, 0.f, 1.f, col_in.r, col_in.g, col_in.b, col_in.a};
    const cstd::uint32_t packed = pack_color(col_out);

    const vertex_writer w = reserve_indexed(6, 6, shader_type::rect_shader);

    w.vertices[0] = vertex{.pos = {n0.x, n0.y}, .col = packed, .uv = {-qw, -qh}, .custom_data = data};
    w.vertices[1] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[2] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};
    w.vertices[3] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[4] = vertex{.pos = {n1.x, n1.y}, .col = packed, .uv = { qw,  qh}, .custom_data = data};
    w.vertices[5] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};

    fill_sequential_indices(w, 6);
}

void rv::renderer::draw_shadow_circle(const position pos, const float radius, const color col, const float shadow_blur,
                                      const bool cut_background) noexcept
{
    const float qw = radius + shadow_blur;
    const float qh = radius + shadow_blur;

    const ndc_position n0 = to_ndc({pos.x - qw, pos.y - qh});
    const ndc_position n1 = to_ndc({pos.x + qw, pos.y + qh});

    const array_t<float, 8> data =
    {
        radius * 2.f, radius * 2.f, cut_background ? 1.f : 0.f, shadow_blur, radius, radius,
        radius, radius
    };
    const cstd::uint32_t packed = pack_color(col);

    const vertex_writer w = reserve_indexed(6, 6, shader_type::shadow_shader);

    w.vertices[0] = vertex{.pos = {n0.x, n0.y}, .col = packed, .uv = {-qw, -qh}, .custom_data = data};
    w.vertices[1] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[2] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};
    w.vertices[3] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = { qw, -qh}, .custom_data = data};
    w.vertices[4] = vertex{.pos = {n1.x, n1.y}, .col = packed, .uv = { qw,  qh}, .custom_data = data};
    w.vertices[5] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {-qw,  qh}, .custom_data = data};

    fill_sequential_indices(w, 6);
}

void rv::renderer::draw_image(const shared_ptr_t<texture> tex, const position min, const position max, const position uv_min,
                              const position uv_max, const color tint) noexcept
{
    draw_image_rounded(tex, min, max, 0.f, rounding_flags_all, uv_min, uv_max, tint);
}

void rv::renderer::draw_image_rounded(const shared_ptr_t<texture> tex, const position min, const position max,
                                      const float rounding, const rounding_flags flags, const position uv_min,
                                      const position uv_max, const color tint) noexcept
{
    const float width = max.x - min.x;
    const float height = max.y - min.y;

    if (!tex || width <= 0.f || height <= 0.f)
    {
        return;
    }

    const float cx = min.x + width * 0.5f;
    const float cy = min.y + height * 0.5f;

    const float qw = (width * 0.5f) + 1.f;
    const float qh = (height * 0.5f) + 1.f;

    const ndc_position n0 = to_ndc({cx - qw, cy - qh});
    const ndc_position n1 = to_ndc({cx + qw, cy + qh});

    const float rtl = (flags & rounding_flags_top_left) ? rounding : 0.f;
    const float rtr = (flags & rounding_flags_top_right) ? rounding : 0.f;
    const float rbr = (flags & rounding_flags_bottom_right) ? rounding : 0.f;
    const float rbl = (flags & rounding_flags_bottom_left) ? rounding : 0.f;

    const float du = uv_max.x - uv_min.x;
    const float dv = uv_max.y - uv_min.y;

    const float u0 = uv_min.x + (-1.f / width) * du;
    const float u1 = uv_min.x + (1.f + 1.f / width) * du;
    const float v0 = uv_min.y + (-1.f / height) * dv;
    const float v1 = uv_min.y + (1.f + 1.f / height) * dv;

    const cstd::uint32_t packed = pack_color(tint);

    current_texture_ = tex;

    const vertex_writer w = reserve_indexed(6, 6, shader_type::image_shader);

    w.vertices[0] = vertex{.pos = {n0.x, n0.y}, .col = packed, .uv = {u0, v0}, .custom_data = {width, height, -qw, -qh, rtr, rbr, rbl, rtl}};
    w.vertices[1] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = {u1, v0}, .custom_data = {width, height,  qw, -qh, rtr, rbr, rbl, rtl}};
    w.vertices[2] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {u0, v1}, .custom_data = {width, height, -qw,  qh, rtr, rbr, rbl, rtl}};
    w.vertices[3] = vertex{.pos = {n1.x, n0.y}, .col = packed, .uv = {u1, v0}, .custom_data = {width, height,  qw, -qh, rtr, rbr, rbl, rtl}};
    w.vertices[4] = vertex{.pos = {n1.x, n1.y}, .col = packed, .uv = {u1, v1}, .custom_data = {width, height,  qw,  qh, rtr, rbr, rbl, rtl}};
    w.vertices[5] = vertex{.pos = {n0.x, n1.y}, .col = packed, .uv = {u0, v1}, .custom_data = {width, height, -qw,  qh, rtr, rbr, rbl, rtl}};

    fill_sequential_indices(w, 6);

    current_texture_ = default_texture_;
}
