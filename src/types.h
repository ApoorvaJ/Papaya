#pragma once

#include <stdint.h>
#include <stdio.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct Vec2i
{
    int32 x, y;
    Vec2i() { x = y = 0; }
    Vec2i(int32 _x, int32 _y) { x = _x; y = _y; }
};

struct Vec2
{
    float x, y;
    Vec2() { x = y = 0.0f; }
    Vec2(float _x, float _y) { x = _x; y = _y; }
    Vec2(Vec2i v) { x = (float)v.x; y = float(v.y); }
};

struct Color
{
    float r, g, b, a;
    Color()                                              { r = 0.0f, g = 0.0f, b = 0.0f; }
    Color(int _r, int _g, int _b, int _a = 255)          { r = (float)_r / 255.0f; g = (float)_g / 255.0f; b = (float)_b / 255.0f; a = (float)_a / 255.0f; }
    Color(float _r, float _g, float _b, float _a = 1.0f) { r = _r; g = _g; b = _b; a = _a; }
    operator uint32() const                              { return ((uint32)(r*255.f)) |(((uint32)(g*255.f)) << 8) | (((uint32)(b*255.f)) << 16) | (((uint32)(a*255.f)) << 24); }
};

