// What a fragment shader of the user interface is given, and the colour the
// engine draws. The shader of the engine, flat.frag, writes that colour as
// it is. A shader of an element includes this file, asks for the colour with
// ui_base(), and writes what it makes of it:
//
//     #version 450
//     #extension GL_GOOGLE_include_directive : require
//     #include "../ui-shader.glsl"
//
//     layout (set = 2, binding = 0) uniform Values
//     {
//         float intensity;
//         vec4 tint;
//     } values;
//
//     void main()
//     {
//         vec4 base = ui_base();
//         frag_color = base * values.tint * values.intensity;
//     }
//
// The members of `Values` are the names a file writes under `shader_values`.
// A colour is written to `frag_color` with its alpha multiplied into it.

#include "ui-frame.glsl"

layout (location = 0) in vec2 tex_coord;
layout (location = 1) in vec4 color;
layout (location = 2) in float textured;
layout (location = 3) in float shape_place;
layout (location = 4) in vec2 local;

layout (location = 0) out vec4 frag_color;

// The texture, with alpha multiplied into its colours, and the sampler it
// is read through, bound apart so that every target of the shaders binds
// what the source says: texture(sampler2D(image, image_sampler), uv).
layout (set = 0, binding = 0) uniform texture2D image;
layout (set = 0, binding = 1) uniform sampler image_sampler;

// has to match Shape2D of the core
struct Shape
{
    // half the width, half the height, the kind, nothing
    vec4 box;
    vec4 radii;
    vec4 widths;
    vec4 gradient;
    vec4 colors[4];
    vec4 stop_positions[2];
    vec4 stop_colors[8];
};

layout (std430, set = 1, binding = 0) readonly buffer Shapes
{
    Shape shapes[];
} all_shapes;

const float UI_FILL = 0.0;
const float UI_BORDER = 1.0;
const float UI_SHADOW = 2.0;
const float UI_INSET_SHADOW = 3.0;
const float UI_TEXT = 4.0;

// Seconds since the user interface was started.
float ui_time()
{
    return frame.state.x;
}

// Where the pixel is in the box of the element, from 0 at its left and its
// top to 1 at its right and its bottom.
vec2 ui_element_uv()
{
    return (gl_FragCoord.xy - frame.element.xy) / max(frame.element.zw, vec2(0.001));
}

// The size of the box of the element in pixels.
vec2 ui_element_size()
{
    return frame.element.zw;
}

// The distance to the outline of a box with round corners, below 0 inside.
float ui_rounded_box(vec2 point, vec2 half_size, vec4 radii)
{
    float radius = point.x < 0.0
        ? (point.y < 0.0 ? radii.x : radii.w)
        : (point.y < 0.0 ? radii.y : radii.z);

    vec2 from_corner = abs(point) - half_size + radius;
    return length(max(from_corner, 0.0)) + min(max(from_corner.x, from_corner.y), 0.0) - radius;
}

// The same for corners that are wider than high or higher than wide, as
// the inside of a border has them whose sides differ in width.
float ui_elliptic_box(vec2 point, vec2 half_size, vec4 radii_x, vec4 radii_y)
{
    vec2 radius = point.x < 0.0
        ? (point.y < 0.0 ? vec2(radii_x.x, radii_y.x) : vec2(radii_x.w, radii_y.w))
        : (point.y < 0.0 ? vec2(radii_x.y, radii_y.y) : vec2(radii_x.z, radii_y.z));

    vec2 from_corner = abs(point) - half_size + radius;

    if (from_corner.x > 0.0 && from_corner.y > 0.0 && radius.x > 0.0 && radius.y > 0.0)
    {
        // how far outside the ellipse, by how fast that changes with the
        // place, which is the distance near the outline
        vec2 scaled = from_corner / radius;
        float reach = length(scaled);
        float slope = length(scaled / radius) / max(reach, 0.0001);
        return (reach - 1.0) / max(slope, 0.0001);
    }

    return max(from_corner.x - radius.x, from_corner.y - radius.y);
}

// How many units of `local` a pixel is, which is 1 unless the element is
// made larger or smaller.
float ui_pixel()
{
    return max(max(length(dFdx(local)), length(dFdy(local))), 0.0001);
}

// How much of a pixel lies inside an outline it has the distance `d` to.
// The edge is smoothed over one pixel.
float ui_coverage(float d)
{
    return clamp(0.5 - d / ui_pixel(), 0.0, 1.0);
}

float ui_erf(float x)
{
    float s = sign(x);
    float a = abs(x);
    float b = 1.0 + (0.278393 + (0.230389 + 0.078108 * (a * a)) * a) * a;
    b *= b;
    return s - s / (b * b);
}

// A number from 0 to 1 that differs from pixel to pixel, to break up the
// steps of a slow gradient.
float ui_noise()
{
    return fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

// The colour of a gradient at a point of the box, with alpha multiplied in.
vec4 ui_gradient(Shape shape, vec2 point)
{
    int count = int(shape.gradient.z);
    vec2 half_size = max(shape.box.xy, vec2(0.0001));
    float along;

    if (shape.gradient.x > 1.5)
    {
        // an ellipse that reaches the corners of the box
        along = length(point / (half_size * 1.41421356));
    } else
    {
        vec2 direction = vec2(sin(shape.gradient.y), -cos(shape.gradient.y));
        float reach = abs(2.0 * half_size.x * direction.x) + abs(2.0 * half_size.y * direction.y);
        along = dot(point, direction) / max(reach, 0.0001) + 0.5;
    }

    float first = shape.stop_positions[0].x;
    vec4 result = shape.stop_colors[0];
    result.rgb *= result.a;

    for (int i = 1; i < count && i < 8; i++)
    {
        float from = shape.stop_positions[(i - 1) / 4][(i - 1) % 4];
        float to = shape.stop_positions[i / 4][i % 4];

        vec4 next = shape.stop_colors[i];
        next.rgb *= next.a;

        float part = to > from ? clamp((along - from) / (to - from), 0.0, 1.0) : step(to, along);
        result = mix(result, next, part);
    }

    // a quarter of a step of noise, so that a slow gradient has no bands
    result.rgb += (ui_noise() - 0.5) / 255.0 * result.a;
    return result;
}

vec4 ui_shape(Shape shape, vec4 texel, vec4 tint, float distance_value)
{
    float kind = shape.box.z;
    vec2 half_size = shape.box.xy;

    if (kind < UI_FILL + 0.5)
    {
        vec4 paint = shape.gradient.x > 0.5
            ? ui_gradient(shape, local) * color.a
            : texel * tint;

        return paint * ui_coverage(ui_rounded_box(local, half_size, shape.radii));
    }

    if (kind < UI_BORDER + 0.5)
    {
        // top, right, bottom, left
        vec4 widths = shape.widths;

        vec2 inner_least = -half_size + vec2(widths.w, widths.x);
        vec2 inner_most = half_size - vec2(widths.y, widths.z);
        vec2 inner_center = (inner_least + inner_most) / 2.0;
        vec2 inner_half = max((inner_most - inner_least) / 2.0, vec2(0.0));

        // the corners of the inside are those of the outside, less the
        // sides that meet there
        vec4 radii_x = max(shape.radii - vec4(widths.w, widths.y, widths.y, widths.w), 0.0);
        vec4 radii_y = max(shape.radii - vec4(widths.x, widths.x, widths.z, widths.z), 0.0);

        float outside = ui_coverage(ui_rounded_box(local, half_size, shape.radii));
        float inside = ui_coverage(ui_elliptic_box(local - inner_center, inner_half, radii_x, radii_y));

        // The side a pixel belongs to is the one it lies least deep in,
        // counted in widths of that side. Two sides then meet along the
        // line from the corner of the outside to that of the inside.
        vec4 depth = vec4(
            (local.y + half_size.y) / max(widths.x, 0.0001),
            (half_size.x - local.x) / max(widths.y, 0.0001),
            (half_size.y - local.y) / max(widths.z, 0.0001),
            (local.x + half_size.x) / max(widths.w, 0.0001));

        depth += vec4(lessThanEqual(widths, vec4(0.0))) * 1e6;

        vec4 side = shape.colors[0];
        float least = depth.x;
        if (depth.y < least) { least = depth.y; side = shape.colors[1]; }
        if (depth.z < least) { least = depth.z; side = shape.colors[2]; }
        if (depth.w < least) { least = depth.w; side = shape.colors[3]; }

        side.a *= color.a;
        side.rgb *= side.a;
        return side * outside * (1.0 - inside);
    }

    if (kind < UI_INSET_SHADOW + 0.5)
    {
        bool is_inset = kind > UI_SHADOW + 0.5;

        vec2 offset = shape.widths.xy;
        float deviation = shape.widths.z;
        float spread = is_inset ? -shape.widths.w : shape.widths.w;

        // a corner that is square stays square
        vec4 radii = max(shape.radii + spread * vec4(greaterThan(shape.radii, vec4(0.0))), 0.0);
        float d = ui_rounded_box(local - offset, max(half_size + spread, vec2(0.0)), radii);

        float covered = deviation > 0.01
            ? 0.5 - 0.5 * ui_erf(d / (1.41421356 * deviation))
            : ui_coverage(d);

        float box = ui_coverage(ui_rounded_box(local, half_size, shape.radii));
        float alpha = is_inset ? (1.0 - covered) * box : covered * (1.0 - box);

        // less than a step of noise, so that a wide shadow has no bands
        alpha = clamp(alpha + (ui_noise() - 0.5) / 255.0 * step(0.002, alpha), 0.0, 1.0);

        return tint * alpha;
    }

    // Glyphs that are kept as distances. What the texture holds is 0.5 on
    // the outline, and reaches 0 and 1 this many pixels from it.
    float reach = max(shape.widths.z, 0.0001);
    float d = (distance_value - 0.5) * 2.0 * reach;

    float line = shape.widths.x;
    float blur = shape.widths.y;

    if (blur > 0.0)
    {
        // no further than the distances reach, where the picture of a
        // glyph ends
        float soft = min(blur, reach * 0.9);
        return tint * smoothstep(-soft, soft, d);
    }

    float fill = clamp(d + 0.5, 0.0, 1.0);
    vec4 result = tint * fill;

    if (line > 0.0)
    {
        // half of the line lies over the glyph, as that of CSS does
        float outer = clamp(d + line / 2.0 + 0.5, 0.0, 1.0);
        float inner = clamp(d - line / 2.0 + 0.5, 0.0, 1.0);

        vec4 stroke = shape.colors[0];
        stroke.a *= color.a;
        stroke.rgb *= stroke.a;
        stroke *= outer * (1.0 - inner);

        result = stroke + result * (1.0 - stroke.a);
    }

    return result;
}

// The colour the engine draws for the pixel, with alpha multiplied in.
vec4 ui_base()
{
    // read where every pixel passes, which is what working out how fast
    // the place in the texture changes asks for
    vec4 sampled = texture(sampler2D(image, image_sampler), tex_coord);

    // a corner that reads no texture is white where the texture would be
    vec4 texel = mix(vec4(1.0), sampled, clamp(textured, 0.0, 1.0));

    // Alpha is multiplied into the colour here. What is written is then
    // added to what is behind it, less the part this pixel covers.
    vec4 tint = vec4(color.rgb * color.a, color.a);

    vec4 result;

    if (shape_place >= -0.5)
    {
        Shape shape = all_shapes.shapes[int(frame.state.y + shape_place + 0.5)];
        result = ui_shape(shape, texel, tint, sampled.a);
    } else if (textured > 1.5)
    {
        // distances without a shape: the pixels they reach over follow
        // from how fast the place in the texture changes
        vec2 texels = fwidth(tex_coord) * vec2(textureSize(sampler2D(image, image_sampler), 0));
        float reach = 8.0 / max(max(texels.x, texels.y), 0.0001);
        result = tint * clamp((sampled.a - 0.5) * 2.0 * reach + 0.5, 0.0, 1.0);
    } else
    {
        result = texel * tint;
    }

    if (frame.state.z > 0.5)
    {
        float d = ui_rounded_box(gl_FragCoord.xy - frame.clip_box.xy, frame.clip_box.zw, frame.clip_radii);
        result *= clamp(0.5 - d, 0.0, 1.0);
    }

    return result;
}
