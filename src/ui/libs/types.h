#pragma once

#include <stdint.h>
#include <stdio.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

struct Vec2 {
    float x, y;
    Vec2() { x = y = 0.0f; }
    Vec2(float _x, float _y) { x = _x; y = _y; }
};

struct Vec2i {
    i32 x, y;
    Vec2i() { x = y = 0; }
    Vec2i(i32 _x, i32 _y) { x = _x; y = _y; }
    operator Vec2() const { return Vec2((float)x, (float)y); }
};

struct Color {
    float r, g, b, a;

    Color() { r = 0.0f, g = 0.0f, b = 0.0f; }
    Color(int _r, int _g, int _b, int _a = 255) {
        r = (float)_r / 255.0f;
        g = (float)_g / 255.0f;
        b = (float)_b / 255.0f;
        a = (float)_a / 255.0f;
    }
    Color(float _r, float _g, float _b, float _a = 1.0f) {
        r = _r; g = _g; b = _b; a = _a;
    }

    operator u32() const {
        return ( (u32)(r * 255.0f)) |
               (((u32)(g * 255.0f)) << 8) |
               (((u32)(b * 255.0f)) << 16) |
               (((u32)(a * 255.0f)) << 24);
    }
};

