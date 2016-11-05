
#version 120

uniform vec4 Color1; // Uniforms[1]
uniform vec4 Color2; // Uniforms[2]

varying vec2 Frag_UV;

void main()
{
    float d = length(vec2(0.5,0.5) - Frag_UV);
    float t = 1.0 - clamp((d - 0.49) * 250.0, 0.0, 1.0);
    t = t - 1.0 + clamp((d - 0.4) * 250.0, 0.0, 1.0);
    gl_FragColor = (Frag_UV.y < 0.5) ? Color1 : Color2;
    gl_FragColor.a = t;
}
