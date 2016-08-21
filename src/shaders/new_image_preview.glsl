#version 120

uniform vec4  Color1; // Uniforms[1]
uniform vec4  Color2; // Uniforms[2]
uniform float width;  // Uniforms[3]
uniform float height; // Uniforms[4]

varying  vec2 Frag_UV;

void main()
{
    float d = mod(Frag_UV.x * width + Frag_UV.y * height, 150);
    if (d < 75)
        gl_FragColor = Color1;
    else
        gl_FragColor = Color2;
}
