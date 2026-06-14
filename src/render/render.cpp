#include "render.hpp"
#include "texture.hpp"

bool rv::renderer::init()
{
    if (!init_backend())
    {
        return false;
    }

    constexpr array_t<const cstd::uint8_t, 4> white = {0xFF, 0xFF, 0xFF, 0xFF};

    default_texture_ = create_texture(white, 1, 1);
    current_texture_ = default_texture_;

    state_.last_time = cstd::get_time();

    return static_cast<bool>(default_texture_);
}

void rv::renderer::begin_frame(const vector_2d<float> display_size) noexcept
{
    const time_point_t current_time = cstd::get_time();

    state_.delta_time = cstd::get_time_diff(current_time, state_.last_time);
    state_.time += state_.delta_time;
    state_.frame_rate = state_.delta_time > 0.f ? (1.f / state_.delta_time) : 0.f;

    state_.last_time = current_time;
    state_.display_size = display_size;

    inv_display_x_ = display_size.x > 0.f ? 2.f / display_size.x : 0.f;
    inv_display_y_ = display_size.y > 0.f ? 2.f / display_size.y : 0.f;

    stats_ = {};

    pending_vertices_.reserve(peak_vertex_count_);
    pending_indices_.reserve(peak_index_count_);
    pending_batches_.reserve(peak_batch_count_);

    begin_frame_backend(display_size);
}

void rv::renderer::push_clip_rect(const position min, const position max, const float rounding,
                                  const rounding_flags flags) noexcept
{
    if (clip_rects_.empty())
    {
        clip_rects_.push_back({{min, max}, rounding, flags});
    }
    else
    {
        const auto& parent = clip_rects_.back();
        
        position intersected_min;
        intersected_min.x = cstd::fmaxf(min.x, parent.bounds.min.x);
        intersected_min.y = cstd::fmaxf(min.y, parent.bounds.min.y);
        
        position intersected_max;
        intersected_max.x = cstd::fmaxf(intersected_min.x, cstd::fminf(max.x, parent.bounds.max.x));
        intersected_max.y = cstd::fmaxf(intersected_min.y, cstd::fminf(max.y, parent.bounds.max.y));
        
        float new_rounding = rounding;
        rounding_flags new_flags = flags;

        if (rounding == 0.f && parent.rounding > 0.f)
        {
            new_rounding = parent.rounding;
            new_flags = parent.flags;
        }

        clip_rects_.push_back({{intersected_min, intersected_max}, new_rounding, new_flags});
    }
}

void rv::renderer::pop_clip_rect() noexcept
{
    if (!clip_rects_.empty())
    {
        clip_rects_.pop_back();
    }
}

rv::vertex_writer rv::renderer::reserve_indexed(const cstd::size_t vertex_count, const cstd::size_t index_count,
                                                const shader_type shader) noexcept
{
    if (vertex_count == 0 || index_count == 0)
    {
        return vertex_writer{{}, {}, 0};
    }

    const optional_t<clip_rect_data> current_clip = clip_rects_.empty() ? optional_t<clip_rect_data>() : clip_rects_.back();

    if (pending_batches_.empty() || current_texture_ != pending_batches_.back().texture ||
        pending_batches_.back().shader != shader || pending_batches_.back().clip_rect != current_clip)
    {
        pending_batches_.push_back(vertex_batch{static_cast<cstd::uint32_t>(pending_vertices_.size()), 0,
                                                static_cast<cstd::uint32_t>(pending_indices_.size()), 0, current_texture_, shader,
                                                current_clip});
        ++stats_.draw_calls;
    }

    auto &current_batch = pending_batches_.back();
    const cstd::uint32_t base_index = current_batch.vertex_count;

    current_batch.vertex_count += static_cast<cstd::uint32_t>(vertex_count);
    current_batch.index_count += static_cast<cstd::uint32_t>(index_count);

    const cstd::size_t vtx_start = pending_vertices_.size();
    const cstd::size_t idx_start = pending_indices_.size();

    pending_vertices_.resize(vtx_start + vertex_count);
    pending_indices_.resize(idx_start + index_count);

    stats_.vertices += static_cast<cstd::int32_t>(vertex_count);
    stats_.indices += static_cast<cstd::int32_t>(index_count);

    last_reserve_vertices_ = vertex_count;
    last_reserve_indices_ = index_count;

    return vertex_writer{{pending_vertices_.data() + vtx_start, vertex_count},
                         {pending_indices_.data() + idx_start, index_count}, base_index};
}

void rv::renderer::shrink_reserved(const cstd::size_t used_vertices, const cstd::size_t used_indices) noexcept
{
    if (pending_batches_.empty() || used_vertices > last_reserve_vertices_ || used_indices > last_reserve_indices_)
    {
        return;
    }

    const cstd::uint32_t trimmed_vertices = static_cast<cstd::uint32_t>(last_reserve_vertices_ - used_vertices);
    const cstd::uint32_t trimmed_indices = static_cast<cstd::uint32_t>(last_reserve_indices_ - used_indices);

    stats_.vertices -= static_cast<cstd::int32_t>(trimmed_vertices);
    stats_.indices -= static_cast<cstd::int32_t>(trimmed_indices);

    pending_vertices_.resize(pending_vertices_.size() - trimmed_vertices);
    pending_indices_.resize(pending_indices_.size() - trimmed_indices);

    auto &current_batch = pending_batches_.back();
    current_batch.vertex_count -= trimmed_vertices;
    current_batch.index_count -= trimmed_indices;

    last_reserve_vertices_ = used_vertices;
    last_reserve_indices_ = used_indices;
}

void rv::renderer::draw_vertices(const span_t<const vertex> vertices, const shader_type shader) noexcept
{
    const cstd::uint32_t count = static_cast<cstd::uint32_t>(vertices.size());
    const vertex_writer w = reserve_indexed(count, count, shader);

    if (w.vertices.empty())
    {
        return;
    }

    cstd::memcpy(w.vertices.data(), vertices.data(), count * sizeof(vertex));

    for (cstd::uint32_t i = 0; i < count; ++i)
    {
        w.indices[i] = w.base_index + i;
    }
}

void rv::renderer::draw_indexed_vertices(const span_t<const vertex> vertices, const span_t<const cstd::uint32_t> indices,
                                         const shader_type shader) noexcept
{
    const vertex_writer w = reserve_indexed(vertices.size(), indices.size(), shader);

    if (w.vertices.empty())
    {
        return;
    }

    cstd::memcpy(w.vertices.data(), vertices.data(), vertices.size() * sizeof(vertex));

    for (cstd::size_t i = 0; i < indices.size(); ++i)
    {
        w.indices[i] = indices[i] + w.base_index;
    }
}

rv::state &rv::renderer::state() noexcept
{
    return state_;
}

const rv::state &rv::renderer::state() const noexcept
{
    return state_;
}

const rv::render_stats &rv::renderer::stats() const noexcept
{
    return stats_;
}

cstd::size_t rv::renderer::vertex_count() const noexcept
{
    return pending_vertices_.size();
}

span_t<rv::vertex> rv::renderer::get_vertices() noexcept
{
    return span_t<rv::vertex>(pending_vertices_);
}

span_t<const rv::vertex> rv::renderer::get_vertices() const noexcept
{
    return span_t<const rv::vertex>(pending_vertices_);
}

void rv::renderer::modify_alpha(const cstd::size_t start_idx, const cstd::size_t end_idx, const float alpha) noexcept
{
    if (alpha >= 1.0f)
    {
        return;
    }

    for (cstd::size_t i = start_idx; i < end_idx && i < pending_vertices_.size(); i++)
    {
        // col is packed RGBA8; scale just the alpha byte (bits 24..31).
        cstd::uint32_t &packed = pending_vertices_[i].col;
        const cstd::uint32_t a = (packed >> 24) & 0xFFu;
        const cstd::uint32_t scaled = static_cast<cstd::uint32_t>(static_cast<float>(a) * alpha + 0.5f);
        packed = (packed & 0x00FFFFFFu) | (scaled << 24);
    }
}

void rv::renderer::modify_color(const cstd::size_t start_idx, const cstd::size_t end_idx, const color col) noexcept
{
    for (cstd::size_t i = start_idx; i < end_idx && i < pending_vertices_.size(); i++)
    {
        pending_vertices_[i].col = pack_color(col);
    }
}

void rv::renderer::modify_scale(const cstd::size_t start_idx, const cstd::size_t end_idx, const position center,
                                const float scale) noexcept
{
    if (scale == 1.f)
    {
        return;
    }

    const ndc_position center_ndc = to_ndc(center);

    for (cstd::size_t i = start_idx; i < end_idx && i < pending_vertices_.size(); i++)
    {
        pending_vertices_[i].pos.x = center_ndc.x + (pending_vertices_[i].pos.x - center_ndc.x) * scale;
        pending_vertices_[i].pos.y = center_ndc.y + (pending_vertices_[i].pos.y - center_ndc.y) * scale;
    }
}

rv::ndc_position rv::renderer::to_ndc(const position pos) const noexcept
{
    return {pos.x * inv_display_x_ - 1.f, 1.f - pos.y * inv_display_y_};
}

rv::clip_constants rv::renderer::pack_clip_constants(const optional_t<clip_rect_data>& clip) noexcept
{
    clip_constants out = {};

    if (!clip.has_value())
    {
        return out;
    }

    const clip_rect_data& c = clip.value();

    out.clip_min[0] = c.bounds.min.x;
    out.clip_min[1] = c.bounds.min.y;
    out.clip_max[0] = c.bounds.max.x;
    out.clip_max[1] = c.bounds.max.y;
    out.clip_enabled = 1.f;

    const float r = c.rounding;
    out.clip_radii[0] = (c.flags & rounding_flags_top_right) ? r : 0.f;
    out.clip_radii[1] = (c.flags & rounding_flags_bottom_right) ? r : 0.f;
    out.clip_radii[2] = (c.flags & rounding_flags_bottom_left) ? r : 0.f;
    out.clip_radii[3] = (c.flags & rounding_flags_top_left) ? r : 0.f;

    return out;
}

void rv::renderer::flush_for_capture() noexcept
{
    flush_pending_vertices();
}

void rv::renderer::execute_backdrop_blur(const position min, const position max, const float sigma, const float rounding) noexcept
{
    const float elem_w = max.x - min.x;
    const float elem_h = max.y - min.y;
    if (elem_w <= 0.f || elem_h <= 0.f || sigma <= 0.f) return;

    const float pad = sigma * 2.f;
    const float cap_x = (min.x - pad > 0.f) ? (min.x - pad) : 0.f;
    const float cap_y = (min.y - pad > 0.f) ? (min.y - pad) : 0.f;
    const float cap_r = (max.x + pad < state_.display_size.x) ? (max.x + pad) : state_.display_size.x;
    const float cap_b = (max.y + pad < state_.display_size.y) ? (max.y + pad) : state_.display_size.y;

    const cstd::uint32_t cap_w = static_cast<cstd::uint32_t>(cap_r - cap_x);
    const cstd::uint32_t cap_h = static_cast<cstd::uint32_t>(cap_b - cap_y);
    if (cap_w == 0 || cap_h == 0) return;

    const cstd::uint32_t half_w = (cap_w + 1) / 2;
    const cstd::uint32_t half_h = (cap_h + 1) / 2;
    if (half_w == 0 || half_h == 0) return;

    flush_for_capture();

    auto capture_rt = create_render_target(cap_w, cap_h);
    auto rt_a = create_render_target(half_w, half_h);
    auto rt_b = create_render_target(half_w, half_h);
    if (!capture_rt || !rt_a || !rt_b) return;

    capture_backbuffer_region(capture_rt,
        static_cast<cstd::uint32_t>(cap_x), static_cast<cstd::uint32_t>(cap_y), cap_w, cap_h);

    auto saved_clips = cstd::move(clip_rects_);
    clip_rects_.clear();
    auto saved_texture = current_texture_;
    const auto saved_display = state_.display_size;

    constexpr cstd::uint32_t white_packed = 0xFFFFFFFF;

    set_render_target(rt_a);
    state_.display_size = {static_cast<float>(half_w), static_cast<float>(half_h)};
    {
        current_texture_ = capture_rt;
        const float texel_x = 1.f / static_cast<float>(cap_w);
        const float texel_y = 1.f / static_cast<float>(cap_h);

        auto writer = reserve_indexed(4, 6, shader_type::blur_shader);
        if (!writer.vertices.empty())
        {
            writer.vertices[0].pos = {-1.f, 1.f};  writer.vertices[0].col = white_packed;
            writer.vertices[0].uv = {0.f, 0.f};    writer.vertices[0].custom_data = {1.f, 0.f, texel_x, texel_y, 0.f, 0.f, 0.f, 0.f};
            writer.vertices[1].pos = { 1.f, 1.f};  writer.vertices[1].col = white_packed;
            writer.vertices[1].uv = {1.f, 0.f};    writer.vertices[1].custom_data = {1.f, 0.f, texel_x, texel_y, 0.f, 0.f, 0.f, 0.f};
            writer.vertices[2].pos = { 1.f,-1.f};  writer.vertices[2].col = white_packed;
            writer.vertices[2].uv = {1.f, 1.f};    writer.vertices[2].custom_data = {1.f, 0.f, texel_x, texel_y, 0.f, 0.f, 0.f, 0.f};
            writer.vertices[3].pos = {-1.f,-1.f};  writer.vertices[3].col = white_packed;
            writer.vertices[3].uv = {0.f, 1.f};    writer.vertices[3].custom_data = {1.f, 0.f, texel_x, texel_y, 0.f, 0.f, 0.f, 0.f};

            writer.indices[0] = writer.base_index;     writer.indices[1] = writer.base_index + 1;
            writer.indices[2] = writer.base_index + 2;  writer.indices[3] = writer.base_index;
            writer.indices[4] = writer.base_index + 2;  writer.indices[5] = writer.base_index + 3;
        }
        flush_pending_vertices();
    }
    reset_render_target();
    state_.display_size = saved_display;

    set_render_target(rt_b);
    state_.display_size = {static_cast<float>(half_w), static_cast<float>(half_h)};
    {
        current_texture_ = rt_a;
        const float texel_x = 1.f / static_cast<float>(half_w);
        const float texel_y = 1.f / static_cast<float>(half_h);

        auto writer = reserve_indexed(4, 6, shader_type::blur_shader);
        if (!writer.vertices.empty())
        {
            writer.vertices[0].pos = {-1.f, 1.f};  writer.vertices[0].col = white_packed;
            writer.vertices[0].uv = {0.f, 0.f};    writer.vertices[0].custom_data = {0.f, 1.f, texel_x, texel_y, 0.f, 0.f, 0.f, 0.f};
            writer.vertices[1].pos = { 1.f, 1.f};  writer.vertices[1].col = white_packed;
            writer.vertices[1].uv = {1.f, 0.f};    writer.vertices[1].custom_data = {0.f, 1.f, texel_x, texel_y, 0.f, 0.f, 0.f, 0.f};
            writer.vertices[2].pos = { 1.f,-1.f};  writer.vertices[2].col = white_packed;
            writer.vertices[2].uv = {1.f, 1.f};    writer.vertices[2].custom_data = {0.f, 1.f, texel_x, texel_y, 0.f, 0.f, 0.f, 0.f};
            writer.vertices[3].pos = {-1.f,-1.f};  writer.vertices[3].col = white_packed;
            writer.vertices[3].uv = {0.f, 1.f};    writer.vertices[3].custom_data = {0.f, 1.f, texel_x, texel_y, 0.f, 0.f, 0.f, 0.f};

            writer.indices[0] = writer.base_index;     writer.indices[1] = writer.base_index + 1;
            writer.indices[2] = writer.base_index + 2;  writer.indices[3] = writer.base_index;
            writer.indices[4] = writer.base_index + 2;  writer.indices[5] = writer.base_index + 3;
        }
        flush_pending_vertices();
    }
    reset_render_target();
    state_.display_size = saved_display;
    clip_rects_ = cstd::move(saved_clips);
    current_texture_ = saved_texture;

    const float u_offset = (min.x - cap_x) / static_cast<float>(cap_w);
    const float v_offset = (min.y - cap_y) / static_cast<float>(cap_h);
    const float u_scale = elem_w / static_cast<float>(cap_w);
    const float v_scale = elem_h / static_cast<float>(cap_h);

    const float u0 = u_offset;
    const float u1 = u_offset + u_scale;
    float v0, v1;

    if (rt_uv_flipped_)
    {
        v0 = 1.f - v_offset;
        v1 = 1.f - (v_offset + v_scale);
    }
    else
    {
        v0 = v_offset;
        v1 = v_offset + v_scale;
    }

    draw_image_rounded(rt_b, min, max, rounding, rounding_flags_all, {u0, v0}, {u1, v1});
}
