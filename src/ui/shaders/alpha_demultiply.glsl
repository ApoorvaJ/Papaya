
#version 120
uniform sampler2D Texture; // Uniforms[1]

varying vec2 Frag_UV;
varying vec4 Frag_Color;

void main()
{
    vec4 col = Frag_Color * texture2D( Texture, Frag_UV.st);
    gl_FragColor = vec4(col.rgb/col.a, col.a);
}
