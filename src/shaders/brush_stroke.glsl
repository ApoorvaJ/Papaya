
#version 120

#define M_PI 3.1415926535897932384626433832795

uniform sampler2D Texture; // Uniforms[1]
uniform vec2 Pos;          // Uniforms[2]
uniform vec2 LastPos;      // Uniforms[3]
uniform float Radius;      // Uniforms[4]
uniform vec4 BrushColor;   // Uniforms[5]
uniform float Hardness;    // Uniforms[6]
uniform float InvAspect;   // Uniforms[7]

varying vec2 Frag_UV;

bool isnan( float val )
{
    return ( val < 0.0 || 0.0 < val || val == 0.0 ) ?
    false : true;
}

void line(vec2 p1, vec2 p2, vec2 uv, float radius,
          out float distLine, out float distp1)
{
    if (distance(p1,p2) <= 0.0)
    {
        distLine = distance(uv, p1);
        distp1 = 0.0;
        return;
    }

    float a = abs(distance(p1, uv));
    float b = abs(distance(p2, uv));
    float c = abs(distance(p1, p2));
    float d = sqrt(c*c + radius*radius);

    vec2 a1 = normalize(uv - p1);
    vec2 b1 = normalize(uv - p2);
    vec2 c1 = normalize(p2 - p1);
    if (dot(a1,c1) < 0.0)
    {
        distLine = a;
        distp1 = 0.0;
        return;
    }

    if (dot(b1,c1) > 0.0)
    {
        distLine = b;
        distp1 = 1.0;
        return;
    }

    float p = (a + b + c) * 0.5;
    float h = 2.0 / c * sqrt( p * (p-a) * (p-b) * (p-c) );

    if (isnan(h))
    {
        distLine = 0.0;
        distp1 = a / c;
    }
    else
    {
        distLine = h;
        distp1 = sqrt(a*a - h*h) / c;
    }
}

void main()
{
    vec4 t = texture2D(Texture, Frag_UV.st);

    float distLine, distp1;
    vec2 aspectUV = vec2(Frag_UV.x, Frag_UV.y * InvAspect);
    line(LastPos, Pos, aspectUV, Radius, distLine, distp1);

    float Scale = 1.0 / (2.0 * Radius * (1.0 - Hardness));
    float Period = M_PI * Scale;
    float Phase = (1.0 - Scale * 2.0 * Radius) * M_PI * 0.5;
    float Alpha = cos((Period * distLine) + Phase);
    if (distLine < Radius - (0.5/Scale)) Alpha = 1.0;
    if (distLine > Radius) Alpha = 0.0;

    float FinalAlpha = max(t.a, Alpha * BrushColor.a);

    // TODO: Needs improvement. Self-intersection corners look weird.
    gl_FragColor = vec4(BrushColor.r,
                        BrushColor.g,
                        BrushColor.b,
                        clamp(FinalAlpha,0.0,1.0));
}
