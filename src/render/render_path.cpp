#include "render.hpp"
#include "texture.hpp"
#include "../util/triangulate.hpp"

void rv::renderer::add_path_point(const position pos)
{
    path_points_.push_back(pos);
}

void rv::renderer::draw_lined_path(const color col, const float thickness, const bool closed, const float fringe_width,
                                   const cap_style cap, const join_style join)
{
    const cstd::size_t n = path_points_.size();

    if (n <= 1)
    {
        path_points_.clear();

        return;
    }

    const float half = thickness * 0.5f;
    const color transparent{col.r, col.g, col.b, 0.f};
    const cstd::uint32_t packed_col = pack_color(col);
    const cstd::uint32_t packed_transparent = pack_color(transparent);

    const auto make_join = [join](const position previous, const position current, const position next) -> position
    {
        const auto dir_in = (current - previous).normalise();
        const auto dir_out = (next - current).normalise();

        const auto in_eff = dir_in ? dir_in : dir_out;
        const auto out_eff = dir_out ? dir_out : dir_in;

        const auto tangent = (in_eff + out_eff).normalise();
        const auto normal = tangent.perpendicular();

        const float c = tangent.dot(out_eff);
        const float scale = (join == join_style::bevel) ? 1.0f : (1.f / cstd::fmaxf(c, 0.25f));

        const auto [x, y] = normal * scale;

        return {x, y};
    };

    const auto prev_of = [&](const cstd::size_t i) -> position
    {
        if (0 < i)
        {
            return path_points_[i - 1];
        }

        return closed ? path_points_[n - 1] : path_points_[i];
    };

    const auto next_of = [&](const cstd::size_t i) -> position
    {
        if (i + 1 < n)
        {
            return path_points_[i + 1];
        }

        return closed ? path_points_[0] : path_points_[i];
    };

    const cstd::size_t segments = closed ? n : n - 1;

    // Reserve a worst-case block (base quads + round-cap geometry for two caps) and write
    // directly into the pending buffers, trimming the unused tail afterwards. Indices are
    // computed as local 0-based vertex indices and shifted by base_index on write.
    const vertex_writer w = reserve_indexed(n * 4 + 32, segments * 18 + 320, shader_type::default_shader);

    cstd::size_t vcount = 0;
    cstd::size_t icount = 0;

    const auto push_vertex = [&](const vertex &v) noexcept { w.vertices[vcount++] = v; };
    const auto push_index = [&](const cstd::uint32_t local) noexcept { w.indices[icount++] = w.base_index + local; };

    for (cstd::size_t i = 0; i < n; i++)
    {
        const position current_pos = path_points_[i];
        const position current_join = make_join(prev_of(i), current_pos, next_of(i));

        const auto core = current_join * half;
        const auto outer = current_join * (half + fringe_width);

        push_vertex(vertex{.pos = to_ndc({current_pos.x + outer.x, current_pos.y + outer.y}), .col = packed_transparent});
        push_vertex(vertex{.pos = to_ndc({current_pos.x + core.x, current_pos.y + core.y}), .col = packed_col});
        push_vertex(vertex{.pos = to_ndc({current_pos.x - core.x, current_pos.y - core.y}), .col = packed_col});
        push_vertex(vertex{.pos = to_ndc({current_pos.x - outer.x, current_pos.y - outer.y}), .col = packed_transparent});
    }

    for (cstd::size_t i = 0; i < segments; i++)
    {
        const cstd::uint32_t idx = static_cast<cstd::uint32_t>(i * 4);
        const cstd::uint32_t nxt = static_cast<cstd::uint32_t>(((i + 1) % n) * 4);

        push_index(idx);
        push_index(nxt);
        push_index(nxt + 1);
        push_index(idx);
        push_index(nxt + 1);
        push_index(idx + 1);

        push_index(idx + 1);
        push_index(nxt + 1);
        push_index(nxt + 2);
        push_index(idx + 1);
        push_index(nxt + 2);
        push_index(idx + 2);

        push_index(idx + 2);
        push_index(nxt + 2);
        push_index(nxt + 3);
        push_index(idx + 2);
        push_index(nxt + 3);
        push_index(idx + 3);
    }

    if (!closed)
    {
        if (cap == cap_style::round)
        {
            const cstd::uint32_t cap_segments = 8;

            auto build_cap = [&](const position p, const auto dir, const cstd::uint32_t v_outer_start,
                                 const cstd::uint32_t v_core_start, const cstd::uint32_t v_outer_end,
                                 const cstd::uint32_t v_core_end)
            {
                const float phi = std::atan2f(dir.y, dir.x);
                const float theta_start = phi + cstd::numbers::pi_f / 2.f;

                const cstd::uint32_t center_idx = static_cast<cstd::uint32_t>(vcount);
                push_vertex(vertex{.pos = to_ndc(p), .col = packed_col});

                cstd::uint32_t prev_outer_idx = v_outer_start;
                cstd::uint32_t prev_core_idx = v_core_start;

                for (cstd::uint32_t j = 1; j <= cap_segments; j++)
                {
                    cstd::uint32_t cur_outer_idx, cur_core_idx;

                    if (j == cap_segments)
                    {
                        cur_outer_idx = v_outer_end;
                        cur_core_idx = v_core_end;
                    }
                    else
                    {
                        const float a =
                            theta_start + (static_cast<float>(j) / static_cast<float>(cap_segments)) * cstd::numbers::pi_f;
                        const float cx = cstd::cosf(a);
                        const float cy = cstd::sinf(a);

                        cur_outer_idx = static_cast<cstd::uint32_t>(vcount);
                        push_vertex(
                            vertex{.pos = to_ndc({p.x + cx * (half + fringe_width), p.y + cy * (half + fringe_width)}),
                                   .col = packed_transparent});

                        cur_core_idx = static_cast<cstd::uint32_t>(vcount);
                        push_vertex(vertex{.pos = to_ndc({p.x + cx * half, p.y + cy * half}), .col = packed_col});
                    }

                    push_index(prev_core_idx);
                    push_index(cur_core_idx);
                    push_index(center_idx);
                    push_index(center_idx);
                    push_index(cur_core_idx);
                    push_index(prev_core_idx);

                    push_index(prev_outer_idx);
                    push_index(cur_outer_idx);
                    push_index(cur_core_idx);
                    push_index(cur_core_idx);
                    push_index(cur_outer_idx);
                    push_index(prev_outer_idx);

                    push_index(prev_outer_idx);
                    push_index(cur_core_idx);
                    push_index(prev_core_idx);
                    push_index(prev_core_idx);
                    push_index(cur_core_idx);
                    push_index(prev_outer_idx);

                    prev_outer_idx = cur_outer_idx;
                    prev_core_idx = cur_core_idx;
                }
            };

            // start cap
            const position p0 = path_points_[0];
            const position p1 = path_points_[1];
            build_cap(p0, (p1 - p0).normalise(), 3, 2, 0, 1);

            // end cap
            const position pe = path_points_[n - 1];
            const position p_prev = path_points_[n - 2];
            const cstd::uint32_t base = static_cast<cstd::uint32_t>((n - 1) * 4);
            build_cap(pe, (p_prev - pe).normalise(), base + 0, base + 1, base + 3, base + 2);
        }
        else
        {
            // flat caps
            const position p0 = path_points_[0];
            const position p1 = path_points_[1];

            auto dir_start = (p1 - p0).normalise();
            auto norm_start = dir_start.perpendicular();

            const auto core_s = norm_start * half;
            const auto outer_s = norm_start * (half + fringe_width);

            const position cap_p0 = {p0.x - dir_start.x * fringe_width, p0.y - dir_start.y * fringe_width};

            const cstd::uint32_t v_start = static_cast<cstd::uint32_t>(vcount);
            push_vertex(vertex{.pos = to_ndc({cap_p0.x + outer_s.x, cap_p0.y + outer_s.y}), .col = packed_transparent});
            push_vertex(vertex{.pos = to_ndc({cap_p0.x + core_s.x, cap_p0.y + core_s.y}), .col = packed_transparent});
            push_vertex(vertex{.pos = to_ndc({cap_p0.x - core_s.x, cap_p0.y - core_s.y}), .col = packed_transparent});
            push_vertex(vertex{.pos = to_ndc({cap_p0.x - outer_s.x, cap_p0.y - outer_s.y}), .col = packed_transparent});

            const cstd::uint32_t idx = v_start;
            const cstd::uint32_t nxt = 0;

            // push both windings to ensure it renders regardless of the culling mode
            push_index(idx);
            push_index(nxt);
            push_index(nxt + 1);
            push_index(idx);
            push_index(nxt + 1);
            push_index(nxt);
            push_index(idx);
            push_index(nxt + 1);
            push_index(idx + 1);
            push_index(idx);
            push_index(idx + 1);
            push_index(nxt + 1);

            push_index(idx + 1);
            push_index(nxt + 1);
            push_index(nxt + 2);
            push_index(idx + 1);
            push_index(nxt + 2);
            push_index(nxt + 1);
            push_index(idx + 1);
            push_index(nxt + 2);
            push_index(idx + 2);
            push_index(idx + 1);
            push_index(idx + 2);
            push_index(nxt + 2);

            push_index(idx + 2);
            push_index(nxt + 2);
            push_index(nxt + 3);
            push_index(idx + 2);
            push_index(nxt + 3);
            push_index(nxt + 2);
            push_index(idx + 2);
            push_index(nxt + 3);
            push_index(idx + 3);
            push_index(idx + 2);
            push_index(idx + 3);
            push_index(nxt + 3);

            // end cap
            const position pe = path_points_[n - 1];
            const position p_prev = path_points_[n - 2];
            auto dir_end = (pe - p_prev).normalise();
            auto norm_end = dir_end.perpendicular();

            const auto core_e = norm_end * half;
            const auto outer_e = norm_end * (half + fringe_width);

            const position cap_pe = {pe.x + dir_end.x * fringe_width, pe.y + dir_end.y * fringe_width};

            const cstd::uint32_t v_end = static_cast<cstd::uint32_t>(vcount);
            push_vertex(vertex{.pos = to_ndc({cap_pe.x + outer_e.x, cap_pe.y + outer_e.y}), .col = packed_transparent});
            push_vertex(vertex{.pos = to_ndc({cap_pe.x + core_e.x, cap_pe.y + core_e.y}), .col = packed_transparent});
            push_vertex(vertex{.pos = to_ndc({cap_pe.x - core_e.x, cap_pe.y - core_e.y}), .col = packed_transparent});
            push_vertex(vertex{.pos = to_ndc({cap_pe.x - outer_e.x, cap_pe.y - outer_e.y}), .col = packed_transparent});

            const cstd::uint32_t idx_end = static_cast<cstd::uint32_t>((n - 1) * 4);
            const cstd::uint32_t nxt_end = v_end;

            push_index(idx_end);
            push_index(nxt_end);
            push_index(nxt_end + 1);
            push_index(idx_end);
            push_index(nxt_end + 1);
            push_index(nxt_end);
            push_index(idx_end);
            push_index(nxt_end + 1);
            push_index(idx_end + 1);
            push_index(idx_end);
            push_index(idx_end + 1);
            push_index(nxt_end + 1);

            push_index(idx_end + 1);
            push_index(nxt_end + 1);
            push_index(nxt_end + 2);
            push_index(idx_end + 1);
            push_index(nxt_end + 2);
            push_index(nxt_end + 1);
            push_index(idx_end + 1);
            push_index(nxt_end + 2);
            push_index(idx_end + 2);
            push_index(idx_end + 1);
            push_index(idx_end + 2);
            push_index(nxt_end + 2);

            push_index(idx_end + 2);
            push_index(nxt_end + 2);
            push_index(nxt_end + 3);
            push_index(idx_end + 2);
            push_index(nxt_end + 3);
            push_index(nxt_end + 2);
            push_index(idx_end + 2);
            push_index(nxt_end + 3);
            push_index(idx_end + 3);
            push_index(idx_end + 2);
            push_index(idx_end + 3);
            push_index(nxt_end + 3);
        }
    }

    shrink_reserved(vcount, icount);

    path_points_.clear();
}

void rv::renderer::draw_filled_path(const color col, const float fringe_width)
{
    if (path_points_.size() <= 2)
    {
        path_points_.clear();

        return;
    }

    const cstd::uint32_t packed_col = pack_color(col);
    const vector_t<cstd::uint32_t> indices = triangulate::execute(path_points_);

    if (!indices.empty())
    {
        const vertex_writer w = reserve_indexed(path_points_.size(), indices.size(), shader_type::default_shader);

        cstd::size_t v = 0;
        for (const position &p : path_points_)
        {
            w.vertices[v++] = vertex{.pos = to_ndc(p), .col = packed_col};
        }

        for (cstd::size_t k = 0; k < indices.size(); ++k)
        {
            w.indices[k] = w.base_index + indices[k];
        }
    }

    draw_lined_path(col, 0.f, true, fringe_width);

    path_points_.clear();
}

void rv::renderer::draw_filled_path_monotone(const color col, const float baseline_y)
{
    const cstd::uint32_t n = static_cast<cstd::uint32_t>(path_points_.size());

    if (n < 2)
    {
        path_points_.clear();

        return;
    }

    const cstd::uint32_t packed_col = pack_color(col);
    const vertex_writer w = reserve_indexed(n * 2, (n - 1) * 6, shader_type::default_shader);

    cstd::size_t v = 0;
    for (const position &p : path_points_)
    {
        w.vertices[v++] = vertex{.pos = to_ndc(p), .col = packed_col};
        w.vertices[v++] = vertex{.pos = to_ndc(position{p.x, baseline_y}), .col = packed_col};
    }

    cstd::size_t k = 0;
    for (cstd::uint32_t i = 0; i + 1 < n; ++i)
    {
        const cstd::uint32_t t0 = i * 2;
        const cstd::uint32_t b0 = i * 2 + 1;
        const cstd::uint32_t t1 = (i + 1) * 2;
        const cstd::uint32_t b1 = (i + 1) * 2 + 1;

        w.indices[k++] = w.base_index + t0;
        w.indices[k++] = w.base_index + t1;
        w.indices[k++] = w.base_index + b1;

        w.indices[k++] = w.base_index + t0;
        w.indices[k++] = w.base_index + b1;
        w.indices[k++] = w.base_index + b0;
    }

    path_points_.clear();
}

void rv::renderer::draw_shadow_lined_path(const color col, const float thickness, const float shadow_blur, const bool closed)
{
    draw_lined_path(col, thickness, closed, shadow_blur, cap_style::round, join_style::bevel);
}

void rv::renderer::draw_shadow_filled_path(const color col, const float shadow_blur)
{
    const cstd::size_t n = path_points_.size();
    if (n <= 2)
    {
        path_points_.clear();
        return;
    }

    const cstd::uint32_t packed_col = pack_color(col);
    const vector_t<cstd::uint32_t> core_indices = triangulate::execute(path_points_);
    if (!core_indices.empty())
    {
        const vertex_writer cw = reserve_indexed(n, core_indices.size(), shader_type::default_shader);

        cstd::size_t v = 0;
        for (const position &p : path_points_)
        {
            cw.vertices[v++] = vertex{.pos = to_ndc(p), .col = packed_col};
        }

        for (cstd::size_t k = 0; k < core_indices.size(); ++k)
        {
            cw.indices[k] = cw.base_index + core_indices[k];
        }
    }

    if (shadow_blur > 0.f)
    {
        float area = 0.f;
        for (cstd::size_t i = 0; i < n; i++)
        {
            const auto p0 = path_points_[i];
            const auto p1 = path_points_[(i + 1) % n];
            area += (p1.x - p0.x) * (p1.y + p0.y);
        }
        const bool is_cw = area < 0.f;

        const color transparent{col.r, col.g, col.b, 0.f};
        const cstd::uint32_t packed_transparent = pack_color(transparent);

        const vertex_writer w = reserve_indexed(n * 11, n * 21, shader_type::default_shader);

        cstd::size_t vcount = 0;
        cstd::size_t icount = 0;

        const auto push_vertex = [&](const vertex &v) noexcept { w.vertices[vcount++] = v; };
        const auto push_index = [&](const cstd::uint32_t local) noexcept { w.indices[icount++] = w.base_index + local; };

        for (cstd::size_t i = 0; i < n; i++)
        {
            const position p0 = path_points_[i];
            const position p1 = path_points_[(i + 1) % n];

            auto dir = (p1 - p0).normalise();
            auto norm = dir.perpendicular();
            if (!is_cw)
            {
                norm.x = -norm.x;
                norm.y = -norm.y;
            }

            const cstd::uint32_t base = static_cast<cstd::uint32_t>(vcount);
            push_vertex(vertex{.pos = to_ndc(p0), .col = packed_col});
            push_vertex(
                vertex{.pos = to_ndc({p0.x + norm.x * shadow_blur, p0.y + norm.y * shadow_blur}), .col = packed_transparent});
            push_vertex(vertex{.pos = to_ndc(p1), .col = packed_col});
            push_vertex(
                vertex{.pos = to_ndc({p1.x + norm.x * shadow_blur, p1.y + norm.y * shadow_blur}), .col = packed_transparent});

            if (is_cw)
            {
                push_index(base + 0);
                push_index(base + 1);
                push_index(base + 2);
                push_index(base + 1);
                push_index(base + 3);
                push_index(base + 2);
            }
            else
            {
                push_index(base + 0);
                push_index(base + 2);
                push_index(base + 1);
                push_index(base + 1);
                push_index(base + 2);
                push_index(base + 3);
            }

            const position p_prev = path_points_[(i + n - 1) % n];
            auto dir_prev = (p0 - p_prev).normalise();
            auto norm_prev = dir_prev.perpendicular();
            if (!is_cw)
            {
                norm_prev.x = -norm_prev.x;
                norm_prev.y = -norm_prev.y;
            }

            const float cross = dir_prev.x * dir.y - dir_prev.y * dir.x;
            const bool is_convex = is_cw ? (cross > 0.f) : (cross < 0.f);

            if (is_convex)
            {
                float a0 = cstd::atan2f(norm_prev.y, norm_prev.x);
                float a1 = cstd::atan2f(norm.y, norm.x);

                if (is_cw && a1 < a0)
                    a1 += cstd::numbers::pi_f * 2.f;
                if (!is_cw && a1 > a0)
                    a1 -= cstd::numbers::pi_f * 2.f;

                const cstd::uint32_t cap_segments = 5;
                const cstd::uint32_t center_idx = static_cast<cstd::uint32_t>(vcount);
                push_vertex(vertex{.pos = to_ndc(p0), .col = packed_col});

                cstd::uint32_t prev_arc_idx = static_cast<cstd::uint32_t>(vcount);
                push_vertex(vertex{.pos = to_ndc({p0.x + norm_prev.x * shadow_blur, p0.y + norm_prev.y * shadow_blur}),
                                   .col = packed_transparent});

                for (cstd::uint32_t j = 1; j <= cap_segments; j++)
                {
                    const float a = a0 + (a1 - a0) * (static_cast<float>(j) / static_cast<float>(cap_segments));
                    const float cx = cstd::cosf(a);
                    const float cy = cstd::sinf(a);

                    const cstd::uint32_t cur_arc_idx = static_cast<cstd::uint32_t>(vcount);
                    push_vertex(
                        vertex{.pos = to_ndc({p0.x + cx * shadow_blur, p0.y + cy * shadow_blur}), .col = packed_transparent});

                    if (is_cw)
                    {
                        push_index(center_idx);
                        push_index(prev_arc_idx);
                        push_index(cur_arc_idx);
                    }
                    else
                    {
                        push_index(center_idx);
                        push_index(cur_arc_idx);
                        push_index(prev_arc_idx);
                    }

                    prev_arc_idx = cur_arc_idx;
                }
            }
        }

        shrink_reserved(vcount, icount);
    }
    path_points_.clear();
}

void rv::renderer::add_arc_path(const position pos, const float radius, const float a_min, const float a_max,
                                const cstd::size_t segment_count) noexcept
{
    for (cstd::size_t i = 0; i < segment_count; i++)
    {
        const float a = a_min + (static_cast<float>(i) / static_cast<float>(segment_count)) * (a_max - a_min);

        add_path_point({pos.x + cstd::cosf(a) * radius, pos.y + cstd::sinf(a) * radius});
    }
}

void rv::renderer::add_circle_path(const position pos, const float radius, const cstd::size_t segment_count) noexcept
{
    add_arc_path(pos, radius, 0.f, cstd::numbers::pi_f * 2.f, static_cast<cstd::int32_t>(segment_count));
}
