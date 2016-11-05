#pragma once

#include "libs/types.h"

// Implicit conversions between papaya_math.h structs and imgui.h structs
#define IM_VEC2_CLASS_EXTRA                                               \
        ImVec2(const Vec2& f) { x = f.x; y = f.y; }                       \
        operator Vec2() const { return Vec2(x,y); }

#define IM_VEC4_CLASS_EXTRA                                               \
        ImVec4(const Color& f){ x = f.r; y = f.g; z = f.b; w = f.a; }     \
        operator Color() const { return Color(x,y,z,w); }
