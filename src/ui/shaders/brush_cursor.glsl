
#version 120

#define M_PI 3.1415926535897932384626433832795

uniform vec4 BrushColor;
uniform float Hardness;
uniform float PixelDiameter;

varying vec2 Frag_UV;


void main()
{
    float Scale = 1.0 / (1.0 - Hardness);
    float Period = M_PI * Scale;
    float Phase = (1.0 - Scale) * M_PI * 0.5;
    float Dist = distance(Frag_UV,vec2(0.5,0.5));
    float Alpha = cos((Period * Dist) + Phase);
    if (Dist < 0.5 - (0.5/Scale)) Alpha = 1.0;
    else if (Dist > 0.5)          Alpha = 0.0;
    float BorderThickness = 1.0 / PixelDiameter;
    gl_FragColor = (Dist > 0.5 - BorderThickness && Dist < 0.5) ?
    vec4(0.0,0.0,0.0,1.0) :
    vec4(BrushColor.r, BrushColor.g, BrushColor.b, 	Alpha * BrushColor.a);
}
