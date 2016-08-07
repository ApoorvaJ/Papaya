
#ifndef MATHLIB_H
#define MATHLIB_H

#include <math.h>

inline Vec2i operator*(const Vec2i& lhs, const int32 rhs)  { return Vec2i(lhs.x*rhs, lhs.y*rhs); }
inline Vec2i operator+(const Vec2i& lhs, const Vec2i& rhs) { return Vec2i(lhs.x+rhs.x, lhs.y+rhs.y); }
inline Vec2i operator-(const Vec2i& lhs, const Vec2i& rhs) { return Vec2i(lhs.x-rhs.x, lhs.y-rhs.y); }
inline Vec2i operator*(const Vec2i& lhs, const Vec2i rhs)  { return Vec2i(lhs.x*rhs.x, lhs.y*rhs.y); }
inline Vec2i& operator+=(Vec2i& lhs, const Vec2i& rhs)     { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
inline Vec2i& operator-=(Vec2i& lhs, const Vec2i& rhs)     { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
inline Vec2i& operator*=(Vec2i& lhs, const int32 rhs)      { lhs.x *= rhs; lhs.y *= rhs; return lhs; }

struct ImVec2;

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


namespace math {
    const double PI = 3.14159265358979323846;

    template<class T> T min(T a, T b) { return (a < b ? a : b); }
    template<class T> T max(T a, T b) { return (a > b ? a : b); }
    template<class T> T clamp(T a, T min, T max) { return math::min(math::max(a, min), max); }

    float abs(float a);
    int32 abs(int32 a);
    float floor(float a);
    int32 round_to_int(float a);
    Vec2i round_to_vec2i(Vec2 a);
    float distance(Vec2 a, Vec2 b);
    float distance_squared(Vec2 a, Vec2 b);
    void hsv_to_rgb(float h, float s, float v, float& out_r, float& out_g, float& out_b);
    void rgb_to_hsv(float r, float g, float b, float& out_h, float& out_s, float& out_v);
    float to_radians(float degrees);
}

#endif // MATHLIB_H

// =======================================================================================

#ifdef MATHLIB_IMPLEMENTATION


float math::abs(float a) {
    return (a < 0.0f ? a * -1.0f : a);
}

int32 math::abs(int32 a) {
    return (a < 0 ? a * -1 : a);
}

float math::floor(float a) {
    int32 i = (int32)a;
    return (float)(i - (i > a));
}

int32 math::round_to_int(float a) {
    return (int32)floor(a + 0.5f);
}

Vec2i math::round_to_vec2i(Vec2 a) {
    return Vec2i(round_to_int(a.x), round_to_int(a.y));
}

float math::distance(Vec2 a, Vec2 b) {
    return sqrtf( (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) );
}

float math::distance_squared(Vec2 a, Vec2 b) {
    return ( (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) );
}

// Color math
// TODO: Move to an array-based color type
void math::hsv_to_rgb(float h, float s, float v,
                      float& out_r, float& out_g, float& out_b) {
    if (s == 0.0f) {
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

    switch (i) {
        case 0: out_r = v; out_g = t; out_b = p; break;
        case 1: out_r = q; out_g = v; out_b = p; break;
        case 2: out_r = p; out_g = v; out_b = t; break;
        case 3: out_r = p; out_g = q; out_b = v; break;
        case 4: out_r = t; out_g = p; out_b = v; break;
        case 5: default: out_r = v; out_g = p; out_b = q; break;
    }
}

void math::rgb_to_hsv(float r, float g, float b,
                      float& out_h, float& out_s, float& out_v) {
    float K = 0.f;
    if (g < b) {
        const float tmp = g; g = b; b = tmp;
        K = -1.f;
    }
    if (r < g) {
        const float tmp = r; r = g; g = tmp;
        K = -2.f / 6.f - K;
    }

    const float chroma = r - (g < b ? g : b);
    out_h = fabsf(K + (g - b) / (6.f * chroma + 1e-20f));
    out_s = chroma / (r + 1e-20f);
    out_v = r;
}

float math::to_radians(float degrees) {
    return (float)(degrees * PI / 180.f);
}

#endif // MATHLIB_IMPLEMENTATION
