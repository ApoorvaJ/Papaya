#pragma once

#include "libs/types.h"

struct Mouse;
struct Layout;

struct ColorPanel {
    bool is_open;
    Color current_color, new_color;
    Color* bound_color; // This color is changed along with current_color. Zero
                        // if no color is bound.
    Vec2 cursor_sv;
    f32 cursor_h;
    Vec2 pos, size, hue_strip_pos, hue_strip_size, sv_box_pos, sv_box_size;
    bool dragging_hue, dragging_sv;
};

ColorPanel* init_color_panel();
void destroy_color_panel(ColorPanel* c);
// TODO: This API could use some attention
void color_panel_set_color(Color col, ColorPanel* c,
                           bool set_new_color_only = false);
void update_color_panel(ColorPanel* c, Color* colors, Mouse& mouse,
                        u32 blank_texture, Layout& layout);
