
#version 120
uniform mat4 ProjMtx;    // Uniforms[0]

attribute vec2 Position; // Attributes[0]
attribute vec2 UV;       // Attributes[1]
attribute vec4 Color;    // Attributes[2]

varying vec2 Frag_UV;
varying vec4 Frag_Color;

void main()
{
    Frag_UV = UV;
    Frag_Color = Color;
    gl_Position = ProjMtx * vec4(Position.xy,0,1);
}
