#pragma once

#include "libs/types.h"

struct Mouse;
struct Layout;

struct Picker {
    bool is_open;
    Color current_color, new_color;
    Color* bound_color; // This color is changed along with current_color. Zero
                        // if no color is bound.
    Vec2 cursor_sv;
    float cursor_h;
    Vec2 pos, size, hue_strip_pos, hue_strip_size, sv_box_pos, sv_box_size;
    bool dragging_hue, dragging_sv;
};

namespace picker {
    void init(Picker* picker);
    void set_color(Color col, Picker* picker, bool set_new_color_only = false);
    void update(Picker* picker, Color* colors, Mouse& mouse, uint32 blank_texture,
                Layout& layout);
}

