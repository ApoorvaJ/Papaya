#pragma once

#include <math.h>


struct Vec2i
{
    int32 x, y;
    Vec2i() { x = y = 0; }
    Vec2i(int32 _x, int32 _y) { x = _x; y = _y; }
};

inline Vec2i operator*(const Vec2i& lhs, const int32 rhs)  { return Vec2i(lhs.x*rhs, lhs.y*rhs); }
inline Vec2i operator+(const Vec2i& lhs, const Vec2i& rhs) { return Vec2i(lhs.x+rhs.x, lhs.y+rhs.y); }
inline Vec2i operator-(const Vec2i& lhs, const Vec2i& rhs) { return Vec2i(lhs.x-rhs.x, lhs.y-rhs.y); }
inline Vec2i operator*(const Vec2i& lhs, const Vec2i rhs)  { return Vec2i(lhs.x*rhs.x, lhs.y*rhs.y); }
inline Vec2i& operator+=(Vec2i& lhs, const Vec2i& rhs)     { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
inline Vec2i& operator-=(Vec2i& lhs, const Vec2i& rhs)     { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
inline Vec2i& operator*=(Vec2i& lhs, const int32 rhs)      { lhs.x *= rhs; lhs.y *= rhs; return lhs; }

struct ImVec2;

struct Vec2
{
    float x, y;
    Vec2() { x = y = 0.0f; }
    Vec2(float _x, float _y) { x = _x; y = _y; }
    Vec2(Vec2i v) { x = (float)v.x; y = float(v.y); }
};

inline Vec2 operator*(const Vec2& lhs, const float rhs) { return Vec2(lhs.x*rhs, lhs.y*rhs); }
inline Vec2 operator/(const Vec2& lhs, const float rhs) { return Vec2(lhs.x/rhs, lhs.y/rhs); }
inline Vec2 operator+(const Vec2& lhs, const Vec2& rhs) { return Vec2(lhs.x+rhs.x, lhs.y+rhs.y); }
inline Vec2 operator-(const Vec2& lhs, const Vec2& rhs) { return Vec2(lhs.x-rhs.x, lhs.y-rhs.y); }
inline Vec2 operator*(const Vec2& lhs, const Vec2 rhs)  { return Vec2(lhs.x*rhs.x, lhs.y*rhs.y); }
inline Vec2 operator/(const Vec2& lhs, const Vec2 rhs)  { return Vec2(lhs.x/rhs.x, lhs.y/rhs.y); }
inline Vec2& operator+=(Vec2& lhs, const Vec2& rhs)     { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
inline Vec2& operator-=(Vec2& lhs, const Vec2& rhs)     { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
inline Vec2& operator*=(Vec2& lhs, const float rhs)     { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
inline Vec2& operator/=(Vec2& lhs, const float rhs)     { lhs.x /= rhs; lhs.y /= rhs; return lhs; }

struct Color
{
    float r, g, b, a;
    Color()                                              { r = 0.0f, g = 0.0f, b = 0.0f; }
    Color(int _r, int _g, int _b, int _a = 255)          { r = (float)_r / 255.0f; g = (float)_g / 255.0f; b = (float)_b / 255.0f; a = (float)_a / 255.0f; }
    Color(float _r, float _g, float _b, float _a = 1.0f) { r = _r; g = _g; b = _b; a = _a; }
    operator uint32() const                              { return ((uint32)(r*255.f)) |(((uint32)(g*255.f)) << 8) | (((uint32)(b*255.f)) << 16) | (((uint32)(a*255.f)) << 24); }
};

// =================================================================================================

namespace Math
{
    const double Pi = 3.14159265358979323846;

    template<class T> 
    T Min(T a, T b) { return (a < b ? a : b); }

    template<class T> 
    T Max(T a, T b) { return (a > b ? a : b); }

    float Abs(float a)
    {
        return (a < 0.0f ? a * -1.0f : a);
    }

    int32 Abs(int32 a)
    {
        return (a < 0 ? a * -1 : a);
    }

    float Floor(float a)
    {
        int32 i = (int32)a;
        return (float)(i - (i > a));
    }

    int32 Clamp(int32 a, int32 min, int32 max)
    {
        int32 result;
        if (a < min)      { result = min; }
        else if (a > max) { result = max; }
        else              { result = a;   }

        return result;
    }

    float Clamp(float a, float min, float max)
    {
        float result;
        if (a < min)      { result = min; }
        else if (a > max) { result = max; }
        else              { result = a;   }

        return result;
    }

    int32 RoundToInt(float a)
    {
        return (int32)Floor(a + 0.5f);
    }

    Vec2i RoundToVec2i(Vec2 a)
    {
        return Vec2i(RoundToInt(a.x), RoundToInt(a.y));
    }
    
    float Distance(Vec2 a, Vec2 b)
    {
        return sqrtf( (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) );
    }

    float DistanceSquared(Vec2 a, Vec2 b)
    {
        return ( (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) );
    }

    // Color math
    void HSVtoRGB(float h, float s, float v, float& out_r, float& out_g, float& out_b)
    {
        if (s == 0.0f)
        {
            // gray
            out_r = out_g = out_b = v;
            return;
        }

        h = fmodf(h, 1.0f) / (60.0f / 360.0f);
        int   i = (int)h;
        float f = h - (float)i;
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i)
        {
        case 0: out_r = v; out_g = t; out_b = p; break;
        case 1: out_r = q; out_g = v; out_b = p; break;
        case 2: out_r = p; out_g = v; out_b = t; break;
        case 3: out_r = p; out_g = q; out_b = v; break;
        case 4: out_r = t; out_g = p; out_b = v; break;
        case 5: default: out_r = v; out_g = p; out_b = q; break;
        }
    }

    void RGBtoHSV(float r, float g, float b, float& out_h, float& out_s, float& out_v)
    {
        float K = 0.f;
        if (g < b)
        {
            const float tmp = g; g = b; b = tmp;
            K = -1.f;
        }
        if (r < g)
        {
            const float tmp = r; r = g; g = tmp;
            K = -2.f / 6.f - K;
        }

        const float chroma = r - (g < b ? g : b);
        out_h = fabsf(K + (g - b) / (6.f * chroma + 1e-20f));
        out_s = chroma / (r + 1e-20f);
        out_v = r;
    }
}