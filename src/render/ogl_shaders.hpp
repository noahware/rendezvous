#pragma once

namespace rv
{
	namespace ogl_shaders
	{
		struct shader_set
		{
			const char* vertex;
			const char* default_fragment;
			const char* rect_fragment;
			const char* shadow_fragment;
			const char* image_fragment;
			const char* text_shadow_fragment;
			const char* blur_fragment;
		};

		// ---- GLSL 120 (OpenGL 2) ----

		namespace gl2
		{
			constexpr const char* vertex_shader = R"glsl(
#version 120

attribute vec2 a_position;
attribute vec4 a_color;
attribute vec2 a_uv;
attribute vec4 a_custom_data;
attribute vec4 a_custom_data2;

varying vec4 v_color;
varying vec2 v_uv;
varying vec4 v_custom_data;
varying vec4 v_custom_data2;
varying vec2 v_screen_pos;

uniform vec2 u_display_size;

void main()
{
    gl_Position = vec4(a_position, 0.0, 1.0);
    v_color = a_color;
    v_uv = a_uv;
    v_custom_data = a_custom_data;
    v_custom_data2 = a_custom_data2;

    v_screen_pos = vec2(
        (a_position.x + 1.0) * u_display_size.x * 0.5,
        (1.0 - a_position.y) * u_display_size.y * 0.5
    );
}
)glsl";

			constexpr const char* default_fragment = R"glsl(
#version 120

varying vec4 v_color;
varying vec2 v_uv;
varying vec4 v_custom_data;
varying vec4 v_custom_data2;
varying vec2 v_screen_pos;

uniform sampler2D u_texture;
uniform vec2 u_clip_min;
uniform vec2 u_clip_max;
uniform vec4 u_clip_radii;
uniform float u_clip_enabled;

float sd_round_rect(vec2 p, vec2 b, vec4 r)
{
    float max_rad = min(b.x, b.y);
    r = min(r, vec4(max_rad));
    vec2 s = step(0.0, p);
    float rad_top = mix(r.w, r.x, s.x);
    float rad_bottom = mix(r.z, r.y, s.x);
    float rad = mix(rad_top, rad_bottom, s.y);
    if (rad <= 0.001)
    {
        vec2 q = abs(p) - b;
        return max(q.x, q.y);
    }
    vec2 q = abs(p) - b + rad;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rad;
}

float apply_clip(vec2 screen_pos)
{
    if (u_clip_enabled < 0.5) return 1.0;
    vec2 center = (u_clip_min + u_clip_max) * 0.5;
    vec2 half_size = (u_clip_max - u_clip_min) * 0.5;
    float d = sd_round_rect(screen_pos - center, half_size, u_clip_radii);
    return clamp(0.5 - d, 0.0, 1.0);
}

void main()
{
    vec4 result = v_color * texture2D(u_texture, v_uv);
    result.a *= apply_clip(v_screen_pos);
    gl_FragColor = result;
}
)glsl";

			constexpr const char* rect_fragment = R"glsl(
#version 120

varying vec4 v_color;
varying vec2 v_uv;
varying vec4 v_custom_data;
varying vec4 v_custom_data2;
varying vec2 v_screen_pos;

uniform sampler2D u_texture;
uniform vec2 u_clip_min;
uniform vec2 u_clip_max;
uniform vec4 u_clip_radii;
uniform float u_clip_enabled;

float sd_round_rect(vec2 p, vec2 b, vec4 r)
{
    float max_rad = min(b.x, b.y);
    r = min(r, vec4(max_rad));
    vec2 s = step(0.0, p);
    float rad_top = mix(r.w, r.x, s.x);
    float rad_bottom = mix(r.z, r.y, s.x);
    float rad = mix(rad_top, rad_bottom, s.y);
    if (rad <= 0.001)
    {
        vec2 q = abs(p) - b;
        return max(q.x, q.y);
    }
    vec2 q = abs(p) - b + rad;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rad;
}

float apply_clip(vec2 screen_pos)
{
    if (u_clip_enabled < 0.5) return 1.0;
    vec2 center = (u_clip_min + u_clip_max) * 0.5;
    vec2 half_size = (u_clip_max - u_clip_min) * 0.5;
    float d = sd_round_rect(screen_pos - center, half_size, u_clip_radii);
    return clamp(0.5 - d, 0.0, 1.0);
}

void main()
{
    vec2 rect_size = v_custom_data.xy;
    float thickness = v_custom_data.z;
    float is_radial = v_custom_data.w;
    vec4 radii = v_custom_data2;

    vec2 p = v_uv;

    vec4 base_color = v_color;
    if (is_radial > 0.5)
    {
        vec4 color_in = radii;
        float radius = rect_size.x * 0.5;
        float dist = length(p);
        float t = clamp(dist / radius, 0.0, 1.0);
        base_color = mix(color_in, v_color, t);
        radii = vec4(radius, radius, radius, radius);
    }

    float d = sd_round_rect(p, rect_size * 0.5, radii);

    float alpha = 0.0;
    if (thickness > 0.0)
    {
        float alpha_outer = 1.0 - smoothstep(-0.5, 0.5, d);
        float alpha_inner = 1.0 - smoothstep(-0.5, 0.5, d + thickness);
        alpha = alpha_outer - alpha_inner;
    }
    else if (thickness < 0.0)
    {
        float d_stroke = abs(d) - (-thickness) * 0.5;
        alpha = 1.0 - smoothstep(-0.5, 0.5, d_stroke);
    }
    else
    {
        alpha = 1.0 - smoothstep(-0.5, 0.5, d);
    }

    vec4 result = vec4(base_color.rgb, base_color.a * alpha);
    result.a *= apply_clip(v_screen_pos);
    gl_FragColor = result;
}
)glsl";

			constexpr const char* shadow_fragment = R"glsl(
#version 120

varying vec4 v_color;
varying vec2 v_uv;
varying vec4 v_custom_data;
varying vec4 v_custom_data2;
varying vec2 v_screen_pos;

uniform sampler2D u_texture;
uniform vec2 u_clip_min;
uniform vec2 u_clip_max;
uniform vec4 u_clip_radii;
uniform float u_clip_enabled;

float sd_round_rect(vec2 p, vec2 b, vec4 r)
{
    float max_rad = min(b.x, b.y);
    r = min(r, vec4(max_rad));
    vec2 s = step(0.0, p);
    float rad_top = mix(r.w, r.x, s.x);
    float rad_bottom = mix(r.z, r.y, s.x);
    float rad = mix(rad_top, rad_bottom, s.y);
    if (rad <= 0.001)
    {
        vec2 q = abs(p) - b;
        return max(q.x, q.y);
    }
    vec2 q = abs(p) - b + rad;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rad;
}

float apply_clip(vec2 screen_pos)
{
    if (u_clip_enabled < 0.5) return 1.0;
    vec2 center = (u_clip_min + u_clip_max) * 0.5;
    vec2 half_size = (u_clip_max - u_clip_min) * 0.5;
    float d = sd_round_rect(screen_pos - center, half_size, u_clip_radii);
    return clamp(0.5 - d, 0.0, 1.0);
}

void main()
{
    vec2 rect_size = v_custom_data.xy;
    float cutout = v_custom_data.z;
    float shadow_blur = max(v_custom_data.w, 0.001);
    vec4 radii = v_custom_data2;

    float d = sd_round_rect(v_uv, rect_size * 0.5, radii);

    float cutout_alpha = 1.0;
    if (cutout > 0.5) cutout_alpha = clamp(d + 0.5, 0.0, 1.0);

    float dist = max(d, 0.0);
    float x = clamp(dist / shadow_blur, 0.0, 1.0);
    float alpha = pow(1.0 - x, 3.0) * cutout_alpha;

    vec4 result = vec4(v_color.rgb, v_color.a * alpha);
    result.a *= apply_clip(v_screen_pos);
    gl_FragColor = result;
}
)glsl";

			constexpr const char* image_fragment = R"glsl(
#version 120

varying vec4 v_color;
varying vec2 v_uv;
varying vec4 v_custom_data;
varying vec4 v_custom_data2;
varying vec2 v_screen_pos;

uniform sampler2D u_texture;
uniform vec2 u_clip_min;
uniform vec2 u_clip_max;
uniform vec4 u_clip_radii;
uniform float u_clip_enabled;

float sd_round_rect(vec2 p, vec2 b, vec4 r)
{
    float max_rad = min(b.x, b.y);
    r = min(r, vec4(max_rad));
    vec2 s = step(0.0, p);
    float rad_top = mix(r.w, r.x, s.x);
    float rad_bottom = mix(r.z, r.y, s.x);
    float rad = mix(rad_top, rad_bottom, s.y);
    if (rad <= 0.001)
    {
        vec2 q = abs(p) - b;
        return max(q.x, q.y);
    }
    vec2 q = abs(p) - b + rad;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rad;
}

float apply_clip(vec2 screen_pos)
{
    if (u_clip_enabled < 0.5) return 1.0;
    vec2 center = (u_clip_min + u_clip_max) * 0.5;
    vec2 half_size = (u_clip_max - u_clip_min) * 0.5;
    float d = sd_round_rect(screen_pos - center, half_size, u_clip_radii);
    return clamp(0.5 - d, 0.0, 1.0);
}

void main()
{
    vec2 rect_size = v_custom_data.xy;
    vec4 radii = v_custom_data2;
    vec2 p = v_custom_data.zw;

    float d = sd_round_rect(p, rect_size * 0.5, radii);
    float alpha = clamp(0.5 - d, 0.0, 1.0);

    vec4 tex_color = texture2D(u_texture, v_uv);
    vec4 result = vec4(tex_color.rgb * v_color.rgb, tex_color.a * v_color.a * alpha);
    result.a *= apply_clip(v_screen_pos);
    gl_FragColor = result;
}
)glsl";

			constexpr const char* text_shadow_fragment = R"glsl(
#version 120
#extension GL_ARB_shader_texture_lod : enable

varying vec4 v_color;
varying vec2 v_uv;
varying vec4 v_custom_data;
varying vec4 v_custom_data2;
varying vec2 v_screen_pos;

uniform sampler2D u_texture;
uniform vec2 u_clip_min;
uniform vec2 u_clip_max;
uniform vec4 u_clip_radii;
uniform float u_clip_enabled;

float sd_round_rect(vec2 p, vec2 b, vec4 r)
{
    float max_rad = min(b.x, b.y);
    r = min(r, vec4(max_rad));
    vec2 s = step(0.0, p);
    float rad_top = mix(r.w, r.x, s.x);
    float rad_bottom = mix(r.z, r.y, s.x);
    float rad = mix(rad_top, rad_bottom, s.y);
    if (rad <= 0.001)
    {
        vec2 q = abs(p) - b;
        return max(q.x, q.y);
    }
    vec2 q = abs(p) - b + rad;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rad;
}

float apply_clip(vec2 screen_pos)
{
    if (u_clip_enabled < 0.5) return 1.0;
    vec2 center = (u_clip_min + u_clip_max) * 0.5;
    vec2 half_size = (u_clip_max - u_clip_min) * 0.5;
    float d = sd_round_rect(screen_pos - center, half_size, u_clip_radii);
    return clamp(0.5 - d, 0.0, 1.0);
}

void main()
{
    float blur_radius = v_custom_data.x;
    float du_per_pixel = v_custom_data.y;
    float dv_per_pixel = v_custom_data.z;
    float cut_bg = v_custom_data.w;

    vec2 uv_min = v_custom_data2.xy;
    vec2 uv_max = v_custom_data2.zw;

    int samples = int(ceil(blur_radius));

    if (samples <= 0)
    {
        if (v_uv.x < uv_min.x || v_uv.x > uv_max.x ||
            v_uv.y < uv_min.y || v_uv.y > uv_max.y)
        {
            gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
            return;
        }
        float a = texture2DLod(u_texture, v_uv, 0.0).a;
        vec4 res = vec4(v_color.rgb, v_color.a * a);
        res.a *= apply_clip(v_screen_pos);
        gl_FragColor = res;
        return;
    }

    float total_alpha = 0.0;
    float total_weight = 0.0;
    float sigma = max(blur_radius / 2.0, 0.1);
    float two_sigma_sq = 2.0 * sigma * sigma;

    // Bounded, linearly-filtered Gaussian: cap taps per axis, step proportionally to the blur,
    // let the linear sampler interpolate skipped texels (near-identical for a soft shadow).
    int side = int(clamp(ceil(blur_radius / 2.5), 1.0, 6.0));
    float fstep = blur_radius / float(side);

    for (int sx = -side; sx <= side; ++sx)
    {
        for (int sy = -side; sy <= side; ++sy)
        {
            float ox = float(sx) * fstep;
            float oy = float(sy) * fstep;
            float dist_sq = ox*ox + oy*oy;
            if (dist_sq <= blur_radius * blur_radius)
            {
                vec2 sample_uv = v_uv + vec2(ox * du_per_pixel, oy * dv_per_pixel);
                float weight = exp(-dist_sq / two_sigma_sq);

                if (sample_uv.x >= uv_min.x && sample_uv.x <= uv_max.x &&
                    sample_uv.y >= uv_min.y && sample_uv.y <= uv_max.y)
                {
                    total_alpha += texture2DLod(u_texture, sample_uv, 0.0).a * weight;
                }
                total_weight += weight;
            }
        }
    }

    float final_alpha = total_weight > 0.0 ? (total_alpha / total_weight) : 0.0;
    final_alpha = clamp(final_alpha * 1.5, 0.0, 1.0);

    if (cut_bg > 0.5)
    {
        if (v_uv.x >= uv_min.x && v_uv.x <= uv_max.x &&
            v_uv.y >= uv_min.y && v_uv.y <= uv_max.y)
        {
            final_alpha *= clamp(1.0 - texture2DLod(u_texture, v_uv, 0.0).a, 0.0, 1.0);
        }
    }

    vec4 result = vec4(v_color.rgb, v_color.a * final_alpha);
    result.a *= apply_clip(v_screen_pos);
    gl_FragColor = result;
}
)glsl";

			constexpr const char* blur_fragment = R"glsl(
#version 120

varying vec4 v_color;
varying vec2 v_uv;
varying vec4 v_custom_data;
varying vec4 v_custom_data2;
varying vec2 v_screen_pos;

uniform sampler2D u_texture;

void main()
{
    vec2 dir = v_custom_data.xy;
    vec2 texel = v_custom_data.zw;
    vec2 step_v = dir * texel;

    vec4 sum = texture2D(u_texture, v_uv) * 0.1964825501511404;

    sum += texture2D(u_texture, v_uv + step_v * 1.0) * 0.2969069646728344;
    sum += texture2D(u_texture, v_uv - step_v * 1.0) * 0.2969069646728344;
    sum += texture2D(u_texture, v_uv + step_v * 2.0) * 0.2195956971744843;
    sum += texture2D(u_texture, v_uv - step_v * 2.0) * 0.2195956971744843;
    sum += texture2D(u_texture, v_uv + step_v * 3.0) * 0.1216189697978516;
    sum += texture2D(u_texture, v_uv - step_v * 3.0) * 0.1216189697978516;
    sum += texture2D(u_texture, v_uv + step_v * 4.0) * 0.0540539696760839;
    sum += texture2D(u_texture, v_uv - step_v * 4.0) * 0.0540539696760839;
    sum += texture2D(u_texture, v_uv + step_v * 5.0) * 0.0216215478581836;
    sum += texture2D(u_texture, v_uv - step_v * 5.0) * 0.0216215478581836;
    sum += texture2D(u_texture, v_uv + step_v * 6.0) * 0.0043243095716367;
    sum += texture2D(u_texture, v_uv - step_v * 6.0) * 0.0043243095716367;

    gl_FragColor = sum;
}
)glsl";

			constexpr shader_set set = { vertex_shader, default_fragment, rect_fragment, shadow_fragment, image_fragment, text_shadow_fragment, blur_fragment };
		}

		// ---- GLSL 330 core (OpenGL 3) ----
		// Differences: layout(location=N), in/out, texture()/textureLod(), out vec4 frag_color, layout(std140) uniform ClipBuffer UBO

		namespace gl3
		{
			constexpr const char* vertex_shader = R"glsl(
#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec4 a_custom_data;
layout(location = 4) in vec4 a_custom_data2;

out vec4 v_color;
out vec2 v_uv;
out vec4 v_custom_data;
out vec4 v_custom_data2;
out vec2 v_screen_pos;

uniform vec2 u_display_size;

void main()
{
    gl_Position = vec4(a_position, 0.0, 1.0);
    v_color = a_color;
    v_uv = a_uv;
    v_custom_data = a_custom_data;
    v_custom_data2 = a_custom_data2;

    v_screen_pos = vec2(
        (a_position.x + 1.0) * u_display_size.x * 0.5,
        (1.0 - a_position.y) * u_display_size.y * 0.5
    );
}
)glsl";

			constexpr const char* default_fragment = R"glsl(
#version 330 core

in vec4 v_color;
in vec2 v_uv;
in vec4 v_custom_data;
in vec4 v_custom_data2;
in vec2 v_screen_pos;

out vec4 frag_color;

uniform sampler2D u_texture;

layout(std140) uniform ClipBuffer
{
    vec2 clip_min;
    vec2 clip_max;
    vec4 clip_radii;
    float clip_enabled;
};

float sd_round_rect(vec2 p, vec2 b, vec4 r)
{
    float max_rad = min(b.x, b.y);
    r = min(r, vec4(max_rad));
    vec2 s = step(0.0, p);
    float rad_top = mix(r.w, r.x, s.x);
    float rad_bottom = mix(r.z, r.y, s.x);
    float rad = mix(rad_top, rad_bottom, s.y);
    if (rad <= 0.001)
    {
        vec2 q = abs(p) - b;
        return max(q.x, q.y);
    }
    vec2 q = abs(p) - b + rad;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rad;
}

float apply_clip(vec2 screen_pos)
{
    if (clip_enabled < 0.5) return 1.0;
    vec2 center = (clip_min + clip_max) * 0.5;
    vec2 half_size = (clip_max - clip_min) * 0.5;
    float d = sd_round_rect(screen_pos - center, half_size, clip_radii);
    return clamp(0.5 - d, 0.0, 1.0);
}

void main()
{
    vec4 result = v_color * texture(u_texture, v_uv);
    result.a *= apply_clip(v_screen_pos);
    frag_color = result;
}
)glsl";

			constexpr const char* rect_fragment = R"glsl(
#version 330 core

in vec4 v_color;
in vec2 v_uv;
in vec4 v_custom_data;
in vec4 v_custom_data2;
in vec2 v_screen_pos;

out vec4 frag_color;

uniform sampler2D u_texture;

layout(std140) uniform ClipBuffer
{
    vec2 clip_min;
    vec2 clip_max;
    vec4 clip_radii;
    float clip_enabled;
};

float sd_round_rect(vec2 p, vec2 b, vec4 r)
{
    float max_rad = min(b.x, b.y);
    r = min(r, vec4(max_rad));
    vec2 s = step(0.0, p);
    float rad_top = mix(r.w, r.x, s.x);
    float rad_bottom = mix(r.z, r.y, s.x);
    float rad = mix(rad_top, rad_bottom, s.y);
    if (rad <= 0.001)
    {
        vec2 q = abs(p) - b;
        return max(q.x, q.y);
    }
    vec2 q = abs(p) - b + rad;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rad;
}

float apply_clip(vec2 screen_pos)
{
    if (clip_enabled < 0.5) return 1.0;
    vec2 center = (clip_min + clip_max) * 0.5;
    vec2 half_size = (clip_max - clip_min) * 0.5;
    float d = sd_round_rect(screen_pos - center, half_size, clip_radii);
    return clamp(0.5 - d, 0.0, 1.0);
}

void main()
{
    vec2 rect_size = v_custom_data.xy;
    float thickness = v_custom_data.z;
    float is_radial = v_custom_data.w;
    vec4 radii = v_custom_data2;

    vec2 p = v_uv;

    vec4 base_color = v_color;
    if (is_radial > 0.5)
    {
        vec4 color_in = radii;
        float radius = rect_size.x * 0.5;
        float dist = length(p);
        float t = clamp(dist / radius, 0.0, 1.0);
        base_color = mix(color_in, v_color, t);
        radii = vec4(radius);
    }

    float d = sd_round_rect(p, rect_size * 0.5, radii);

    float alpha = 0.0;
    if (thickness > 0.0)
    {
        float alpha_outer = 1.0 - smoothstep(-0.5, 0.5, d);
        float alpha_inner = 1.0 - smoothstep(-0.5, 0.5, d + thickness);
        alpha = alpha_outer - alpha_inner;
    }
    else if (thickness < 0.0)
    {
        float d_stroke = abs(d) - (-thickness) * 0.5;
        alpha = 1.0 - smoothstep(-0.5, 0.5, d_stroke);
    }
    else
    {
        alpha = 1.0 - smoothstep(-0.5, 0.5, d);
    }

    vec4 result = vec4(base_color.rgb, base_color.a * alpha);
    result.a *= apply_clip(v_screen_pos);
    frag_color = result;
}
)glsl";

			constexpr const char* shadow_fragment = R"glsl(
#version 330 core

in vec4 v_color;
in vec2 v_uv;
in vec4 v_custom_data;
in vec4 v_custom_data2;
in vec2 v_screen_pos;

out vec4 frag_color;

uniform sampler2D u_texture;

layout(std140) uniform ClipBuffer
{
    vec2 clip_min;
    vec2 clip_max;
    vec4 clip_radii;
    float clip_enabled;
};

float sd_round_rect(vec2 p, vec2 b, vec4 r)
{
    float max_rad = min(b.x, b.y);
    r = min(r, vec4(max_rad));
    vec2 s = step(0.0, p);
    float rad_top = mix(r.w, r.x, s.x);
    float rad_bottom = mix(r.z, r.y, s.x);
    float rad = mix(rad_top, rad_bottom, s.y);
    if (rad <= 0.001)
    {
        vec2 q = abs(p) - b;
        return max(q.x, q.y);
    }
    vec2 q = abs(p) - b + rad;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rad;
}

float apply_clip(vec2 screen_pos)
{
    if (clip_enabled < 0.5) return 1.0;
    vec2 center = (clip_min + clip_max) * 0.5;
    vec2 half_size = (clip_max - clip_min) * 0.5;
    float d = sd_round_rect(screen_pos - center, half_size, clip_radii);
    return clamp(0.5 - d, 0.0, 1.0);
}

void main()
{
    vec2 rect_size = v_custom_data.xy;
    float cutout = v_custom_data.z;
    float shadow_blur = max(v_custom_data.w, 0.001);
    vec4 radii = v_custom_data2;

    float d = sd_round_rect(v_uv, rect_size * 0.5, radii);

    float cutout_alpha = 1.0;
    if (cutout > 0.5) cutout_alpha = clamp(d + 0.5, 0.0, 1.0);

    float dist = max(d, 0.0);
    float x = clamp(dist / shadow_blur, 0.0, 1.0);
    float alpha = pow(1.0 - x, 3.0) * cutout_alpha;

    vec4 result = vec4(v_color.rgb, v_color.a * alpha);
    result.a *= apply_clip(v_screen_pos);
    frag_color = result;
}
)glsl";

			constexpr const char* image_fragment = R"glsl(
#version 330 core

in vec4 v_color;
in vec2 v_uv;
in vec4 v_custom_data;
in vec4 v_custom_data2;
in vec2 v_screen_pos;

out vec4 frag_color;

uniform sampler2D u_texture;

layout(std140) uniform ClipBuffer
{
    vec2 clip_min;
    vec2 clip_max;
    vec4 clip_radii;
    float clip_enabled;
};

float sd_round_rect(vec2 p, vec2 b, vec4 r)
{
    float max_rad = min(b.x, b.y);
    r = min(r, vec4(max_rad));
    vec2 s = step(0.0, p);
    float rad_top = mix(r.w, r.x, s.x);
    float rad_bottom = mix(r.z, r.y, s.x);
    float rad = mix(rad_top, rad_bottom, s.y);
    if (rad <= 0.001)
    {
        vec2 q = abs(p) - b;
        return max(q.x, q.y);
    }
    vec2 q = abs(p) - b + rad;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rad;
}

float apply_clip(vec2 screen_pos)
{
    if (clip_enabled < 0.5) return 1.0;
    vec2 center = (clip_min + clip_max) * 0.5;
    vec2 half_size = (clip_max - clip_min) * 0.5;
    float d = sd_round_rect(screen_pos - center, half_size, clip_radii);
    return clamp(0.5 - d, 0.0, 1.0);
}

void main()
{
    vec2 rect_size = v_custom_data.xy;
    vec4 radii = v_custom_data2;
    vec2 p = v_custom_data.zw;

    float d = sd_round_rect(p, rect_size * 0.5, radii);
    float alpha = clamp(0.5 - d, 0.0, 1.0);

    vec4 tex_color = texture(u_texture, v_uv);
    vec4 result = vec4(tex_color.rgb * v_color.rgb, tex_color.a * v_color.a * alpha);
    result.a *= apply_clip(v_screen_pos);
    frag_color = result;
}
)glsl";

			constexpr const char* text_shadow_fragment = R"glsl(
#version 330 core

in vec4 v_color;
in vec2 v_uv;
in vec4 v_custom_data;
in vec4 v_custom_data2;
in vec2 v_screen_pos;

out vec4 frag_color;

uniform sampler2D u_texture;

layout(std140) uniform ClipBuffer
{
    vec2 clip_min;
    vec2 clip_max;
    vec4 clip_radii;
    float clip_enabled;
};

float sd_round_rect(vec2 p, vec2 b, vec4 r)
{
    float max_rad = min(b.x, b.y);
    r = min(r, vec4(max_rad));
    vec2 s = step(0.0, p);
    float rad_top = mix(r.w, r.x, s.x);
    float rad_bottom = mix(r.z, r.y, s.x);
    float rad = mix(rad_top, rad_bottom, s.y);
    if (rad <= 0.001)
    {
        vec2 q = abs(p) - b;
        return max(q.x, q.y);
    }
    vec2 q = abs(p) - b + rad;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rad;
}

float apply_clip(vec2 screen_pos)
{
    if (clip_enabled < 0.5) return 1.0;
    vec2 center = (clip_min + clip_max) * 0.5;
    vec2 half_size = (clip_max - clip_min) * 0.5;
    float d = sd_round_rect(screen_pos - center, half_size, clip_radii);
    return clamp(0.5 - d, 0.0, 1.0);
}

void main()
{
    float blur_radius = v_custom_data.x;
    float du_per_pixel = v_custom_data.y;
    float dv_per_pixel = v_custom_data.z;
    float cut_bg = v_custom_data.w;

    vec2 uv_min = v_custom_data2.xy;
    vec2 uv_max = v_custom_data2.zw;

    int samples = int(ceil(blur_radius));

    if (samples <= 0)
    {
        if (v_uv.x < uv_min.x || v_uv.x > uv_max.x ||
            v_uv.y < uv_min.y || v_uv.y > uv_max.y)
        {
            frag_color = vec4(0.0);
            return;
        }
        float a = textureLod(u_texture, v_uv, 0.0).a;
        vec4 res = vec4(v_color.rgb, v_color.a * a);
        res.a *= apply_clip(v_screen_pos);
        frag_color = res;
        return;
    }

    float total_alpha = 0.0;
    float total_weight = 0.0;
    float sigma = max(blur_radius / 2.0, 0.1);
    float two_sigma_sq = 2.0 * sigma * sigma;

    // Bounded, linearly-filtered Gaussian: cap taps per axis, step proportionally to the blur,
    // let the linear sampler interpolate skipped texels (near-identical for a soft shadow).
    int side = int(clamp(ceil(blur_radius / 2.5), 1.0, 6.0));
    float fstep = blur_radius / float(side);

    for (int sx = -side; sx <= side; ++sx)
    {
        for (int sy = -side; sy <= side; ++sy)
        {
            float ox = float(sx) * fstep;
            float oy = float(sy) * fstep;
            float dist_sq = ox*ox + oy*oy;
            if (dist_sq <= blur_radius * blur_radius)
            {
                vec2 sample_uv = v_uv + vec2(ox * du_per_pixel, oy * dv_per_pixel);
                float weight = exp(-dist_sq / two_sigma_sq);

                if (sample_uv.x >= uv_min.x && sample_uv.x <= uv_max.x &&
                    sample_uv.y >= uv_min.y && sample_uv.y <= uv_max.y)
                {
                    total_alpha += textureLod(u_texture, sample_uv, 0.0).a * weight;
                }
                total_weight += weight;
            }
        }
    }

    float final_alpha = total_weight > 0.0 ? (total_alpha / total_weight) : 0.0;
    final_alpha = clamp(final_alpha * 1.5, 0.0, 1.0);

    if (cut_bg > 0.5)
    {
        if (v_uv.x >= uv_min.x && v_uv.x <= uv_max.x &&
            v_uv.y >= uv_min.y && v_uv.y <= uv_max.y)
        {
            final_alpha *= clamp(1.0 - textureLod(u_texture, v_uv, 0.0).a, 0.0, 1.0);
        }
    }

    vec4 result = vec4(v_color.rgb, v_color.a * final_alpha);
    result.a *= apply_clip(v_screen_pos);
    frag_color = result;
}
)glsl";

			constexpr const char* blur_fragment = R"glsl(
#version 330 core

in vec4 v_color;
in vec2 v_uv;
in vec4 v_custom_data;
in vec4 v_custom_data2;
in vec2 v_screen_pos;

out vec4 frag_color;

uniform sampler2D u_texture;

void main()
{
    vec2 dir = v_custom_data.xy;
    vec2 texel = v_custom_data.zw;
    vec2 step_v = dir * texel;

    vec4 sum = texture(u_texture, v_uv) * 0.1964825501511404;

    sum += texture(u_texture, v_uv + step_v * 1.0) * 0.2969069646728344;
    sum += texture(u_texture, v_uv - step_v * 1.0) * 0.2969069646728344;
    sum += texture(u_texture, v_uv + step_v * 2.0) * 0.2195956971744843;
    sum += texture(u_texture, v_uv - step_v * 2.0) * 0.2195956971744843;
    sum += texture(u_texture, v_uv + step_v * 3.0) * 0.1216189697978516;
    sum += texture(u_texture, v_uv - step_v * 3.0) * 0.1216189697978516;
    sum += texture(u_texture, v_uv + step_v * 4.0) * 0.0540539696760839;
    sum += texture(u_texture, v_uv - step_v * 4.0) * 0.0540539696760839;
    sum += texture(u_texture, v_uv + step_v * 5.0) * 0.0216215478581836;
    sum += texture(u_texture, v_uv - step_v * 5.0) * 0.0216215478581836;
    sum += texture(u_texture, v_uv + step_v * 6.0) * 0.0043243095716367;
    sum += texture(u_texture, v_uv - step_v * 6.0) * 0.0043243095716367;

    frag_color = sum;
}
)glsl";

			constexpr shader_set set = { vertex_shader, default_fragment, rect_fragment, shadow_fragment, image_fragment, text_shadow_fragment, blur_fragment };
		}
	}
}
