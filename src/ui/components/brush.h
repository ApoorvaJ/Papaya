#pragma once

#include "libs/types.h"

struct PapayaMemory;
struct PaglMesh;
struct PaglProgram;

struct Brush {
    i32 diameter;
    i32 max_diameter;
    f32 opacity; // Range: [0.0, 1.0]
    f32 hardness; // Range: [0.0, 1.0]
    bool anti_alias;

    Vec2i paint_area_1, paint_area_2;

    // TODO: Move some of this stuff to the Mouse struct?
    Vec2i rt_drag_start_pos;
    bool rt_drag_with_shift;
    i32 rt_drag_start_diameter;
    f32 rt_drag_start_hardness, rt_drag_start_opacity;
    bool draw_line_segment;
    Vec2 line_segment_start_uv;
    bool being_dragged;
    bool is_straight_drag;
    bool was_straight_drag;
    bool straight_drag_snap_x;
    bool straight_drag_snap_y;
    Vec2 straight_drag_start_uv;

    PaglMesh* mesh_cursor;
    PaglMesh* mesh_RTTBrush; // TODO: Use a canvas mesh instead
    PaglMesh* mesh_RTTAdd;   // 
    PaglProgram* pgm_cursor;
    PaglProgram* pgm_stroke;
};


Brush* init_brush(PapayaMemory* mem);
void destroy_brush(Brush* b);

// TODO: Remove these functions. Use a canvas mesh instead.
void resize_brush_meshes(Brush* b, Vec2 size);
void destroy_brush_meshes(Brush* b);
//

void update_and_render_brush(PapayaMemory* mem);
