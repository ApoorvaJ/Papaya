
#version 120

uniform vec4  Color1;    // Uniforms[1]
uniform vec4  Color2;    // Uniforms[2]
uniform float Zoom;      // Uniforms[3]
uniform float InvAspect; // Uniforms[4]
uniform float MaxDim;    // Uniforms[5]

varying  vec2 Frag_UV;

void main()
{
    vec2 aspectUV;
    if (InvAspect < 1.0)
        aspectUV = vec2(Frag_UV.x, Frag_UV.y * InvAspect);
    else
        aspectUV = vec2(Frag_UV.x / InvAspect, Frag_UV.y);
    vec2 uv = floor(aspectUV.xy * 0.1 * MaxDim * Zoom);
    float a = mod(uv.x + uv.y, 2.0);
    gl_FragColor = mix(Color1, Color2, a);
}
