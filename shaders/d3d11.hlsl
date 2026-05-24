struct vs_input
{
    float2 position : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float4 custom_data : TEXCOORD1;
    float4 custom_data2 : TEXCOORD2;
};

struct ps_input
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float4 custom_data : TEXCOORD1;
    float4 custom_data2 : TEXCOORD2;
};

cbuffer clip_buffer : register(b0)
{
    float2 clip_min;
    float2 clip_max;
    float4 clip_radii;
    float clip_enabled;
    float3 _padding;
};

Texture2D textr : register(t0);
SamplerState samplr : register(s0);

float sd_round_rect(float2 p, float2 b, float4 r)
{
    float max_rad = min(b.x, b.y);
    r = min(r, max_rad);

    float2 s = step(0.0, p);
    
    float rad_top = lerp(r.w, r.x, s.x);
    float rad_bottom = lerp(r.z, r.y, s.x);
    float rad = lerp(rad_top, rad_bottom, s.y);

    if (rad <= 0.001f)
    {
        float2 q = abs(p) - b;
        return max(q.x, q.y);
    }

    float2 q = abs(p) - b + rad;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rad;
}

float apply_clip(float2 screen_pos)
{
    if (clip_enabled < 0.5f)
    {
        return 1.0f;
    }

    float2 center = (clip_min + clip_max) * 0.5f;
    float2 half_size = (clip_max - clip_min) * 0.5f;
    float2 p = screen_pos - center;

    float d = sd_round_rect(p, half_size, clip_radii);

    return saturate(0.5f - d);
}

ps_input vertex_shader(vs_input input)
{
    ps_input output;

    output.position = float4(input.position, 0.f, 1.f);
    output.color = input.color;
    output.uv = input.uv;
    output.custom_data = input.custom_data;
    output.custom_data2 = input.custom_data2;
    
    return output;
}

float4 pixel_shader(ps_input input) : SV_TARGET
{
    float4 result = input.color * textr.Sample(samplr, input.uv);
    result.a *= apply_clip(input.position.xy);
    return result;
}

float4 rect_pixel_shader(ps_input input) : SV_TARGET
{
    float2 rect_size = input.custom_data.xy;
    float thickness = input.custom_data.z;
    float is_radial = input.custom_data.w;
    float4 radii = input.custom_data2;
    
    float2 p = input.uv;
    
    float4 base_color = input.color;
    if (is_radial > 0.5f)
    {
        float4 color_in = radii;
        float radius = rect_size.x * 0.5f;
        float dist = length(p);
        float t = saturate(dist / radius);
        base_color = lerp(color_in, input.color, t);
        
        radii = float4(radius, radius, radius, radius);
    }
    
    float d = sd_round_rect(p, rect_size * 0.5f, radii);
    
    float alpha = 0.0f;
    if (thickness > 0.0)
    {
        float outer_d = d;
        float inner_d = d + thickness;
        
        float alpha_outer = 1.0f - smoothstep(-0.5f, 0.5f, outer_d);
        float alpha_inner = 1.0f - smoothstep(-0.5f, 0.5f, inner_d);
        
        alpha = alpha_outer - alpha_inner;
    }
    else if (thickness < 0.0)
    {
        float abs_t = -thickness;
        float d_stroke = abs(d) - abs_t * 0.5f;
        alpha = 1.0f - smoothstep(-0.5f, 0.5f, d_stroke);
    }
    else
    {
        alpha = 1.0f - smoothstep(-0.5f, 0.5f, d);
    }
    
    float4 result = float4(base_color.rgb, base_color.a * alpha);
    result.a *= apply_clip(input.position.xy);
    return result;
}

float4 shadow_pixel_shader(ps_input input) : SV_TARGET
{
    float2 rect_size = input.custom_data.xy;
    float cutout = input.custom_data.z;
    float shadow_blur = max(input.custom_data.w, 0.001f);
    float4 radii = input.custom_data2;
    
    float2 p = input.uv;
    float d = sd_round_rect(p, rect_size * 0.5f, radii);
    
    float cutout_alpha = 1.0f;
    if (cutout > 0.5f)
    {
        cutout_alpha = saturate(d + 0.5f);
    }
    
    float dist = max(d, 0.0f);
    float x = saturate(dist / shadow_blur);
    
    float alpha = pow(1.0f - x, 3.0f) * cutout_alpha;
    
    float4 result = float4(input.color.rgb, input.color.a * alpha);
    result.a *= apply_clip(input.position.xy);
    return result;
}

float4 image_pixel_shader(ps_input input) : SV_TARGET
{
    float2 rect_size = input.custom_data.xy;
    float4 radii = input.custom_data2;

    float2 p = input.custom_data.zw;
    
    float d = sd_round_rect(p, rect_size * 0.5f, radii);
    
    float alpha = saturate(0.5f - d);
    
    float4 tex_color = textr.Sample(samplr, input.uv);
    float4 result = float4(tex_color.rgb * input.color.rgb, tex_color.a * input.color.a * alpha);
    result.a *= apply_clip(input.position.xy);
    return result;
}