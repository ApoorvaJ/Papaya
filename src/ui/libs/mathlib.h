
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
    float to_radians(float degrees);
}

#endif // MATHLIB_H

// =======================================================================================

#ifdef MATHLIB_IMPLEMENTATION


float math::abs(float a)
{
    return (a < 0.0f ? a * -1.0f : a);
}

int32 math::abs(int32 a)
{
    return (a < 0 ? a * -1 : a);
}

float math::floor(float a)
{
    int32 i = (int32)a;
    return (float)(i - (i > a));
}

int32 math::round_to_int(float a)
{
    return (int32)floor(a + 0.5f);
}

Vec2i math::round_to_vec2i(Vec2 a)
{
    return Vec2i(round_to_int(a.x), round_to_int(a.y));
}

float math::distance(Vec2 a, Vec2 b)
{
    return sqrtf( (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) );
}

float math::distance_squared(Vec2 a, Vec2 b)
{
    return ( (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) );
}

float math::to_radians(float degrees)
{
    return (float)(degrees * PI / 180.f);
}

#endif // MATHLIB_IMPLEMENTATION
