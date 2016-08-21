
#version 120

uniform float Hue;   // Uniforms[1]
uniform vec2 Cursor; // Uniforms[2]
uniform float Thickness = 1.0 / 256.0;
uniform float Radius = 0.0075;

varying  vec2 Frag_UV;

// Source: Fast branchless RGB to HSV conversion in GLSL
// http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main()
{
    vec3 RGB = hsv2rgb(vec3(Hue, Frag_UV.x, 1.0 - Frag_UV.y));
    vec2 CursorInv = vec2(Cursor.x, 1.0 - Cursor.y);
    float Dist = distance(Frag_UV, CursorInv);

    if (Dist > Radius && Dist < Radius + Thickness)
    {
        float a = (CursorInv.x < 0.4 && CursorInv.y > 0.6) ? 0.0 : 1.0;
        gl_FragColor = vec4(a, a, a, 1.0);
    }
    else
    {
        gl_FragColor = vec4(RGB.x, RGB.y, RGB.z, 1.0);
    }
}
