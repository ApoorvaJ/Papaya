#pragma once

struct ImVec2;

struct Vec2
{
	float x, y;
	Vec2() { x = y = 0.0f; }
	Vec2(float _x, float _y) { x = _x; y = _y; }
};

inline Vec2 operator*(const Vec2& lhs, const float rhs)		{ return Vec2(lhs.x*rhs, lhs.y*rhs); }
inline Vec2 operator/(const Vec2& lhs, const float rhs)		{ return Vec2(lhs.x/rhs, lhs.y/rhs); }
inline Vec2 operator+(const Vec2& lhs, const Vec2& rhs)		{ return Vec2(lhs.x+rhs.x, lhs.y+rhs.y); }
inline Vec2 operator-(const Vec2& lhs, const Vec2& rhs)		{ return Vec2(lhs.x-rhs.x, lhs.y-rhs.y); }
inline Vec2 operator*(const Vec2& lhs, const Vec2 rhs)		{ return Vec2(lhs.x*rhs.x, lhs.y*rhs.y); }
inline Vec2 operator/(const Vec2& lhs, const Vec2 rhs)		{ return Vec2(lhs.x/rhs.x, lhs.y/rhs.y); }
inline Vec2& operator+=(Vec2& lhs, const Vec2& rhs)			{ lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
inline Vec2& operator-=(Vec2& lhs, const Vec2& rhs)			{ lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
inline Vec2& operator*=(Vec2& lhs, const float rhs)			{ lhs.x *= rhs; lhs.y *= rhs; return lhs; }
inline Vec2& operator/=(Vec2& lhs, const float rhs)			{ lhs.x /= rhs; lhs.y /= rhs; return lhs; }

struct Color
{
	float r, g, b, a;
	Color(int _r, int _g, int _b, int _a = 255)				{ r = (float)_r / 255.0f; g = (float)_g / 255.0f; b = (float)_b / 255.0f; a = (float)_a / 255.0f; }
	Color(float _r, float _g, float _b, float _a = 1.0f)	{ r = _r; g = _g; b = _b; a = _a; }
	operator uint32() const									{ return ((uint32)(r*255.f)) |(((uint32)(g*255.f)) << 8) | (((uint32)(b*255.f)) << 16) | (((uint32)(a*255.f)) << 24); }
};