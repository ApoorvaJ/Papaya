#pragma once

#include "libs/types.h"

struct Mouse;
struct PapayaMemory; // TODO: Remove. Manage struct knowledge on a need-to-know-basis.

struct CropRotate {
    i32 base_rotation; // Multiply this by 90 to get the rotation in degrees
    f32 slider_angle;
    Vec2 top_left;
    Vec2 bot_right;

    // This is a bitfield that uses the 4 least significant bits to represent
    // whether each of the 4 vertices are active.
    // 
    // 1 Vertex active   -> Vertex drag
    // 2 Vertices active -> Edge drag
    // 4 Vertices active -> Full rect drag
    u8 crop_mode;
    Vec2 rect_drag_position;
};

namespace crop_rotate {
    void init(PapayaMemory* mem);
    void toolbar(PapayaMemory* mem);
    void crop_outline(PapayaMemory* mem);
}

