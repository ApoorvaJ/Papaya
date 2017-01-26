#pragma once

#if !defined(_DEBUG ) // TODO: Make this work for gcc
#define PAPAYARELEASE // User-ready release mode
#endif // !_DEBUG

#include "libs/types.h"
#include "pagl.h"
#include "libs/timer.h"
#include "libs/easytab.h"
#include "libs/imgui/imgui.h"

#include "components/crop_rotate.h"
#include "components/node.h"
#include "components/prefs.h"
#include "components/undo.h"

struct ImDrawData;
struct ColorPanel;
struct GraphPanel;
struct PapayaDocument;

enum PapayaTex_ {
    PapayaTex_Font,
    PapayaTex_UI,
    PapayaTex_COUNT
};

enum PapayaCol_ {
    PapayaCol_Clear,
    PapayaCol_Workspace,
    PapayaCol_Button,
    PapayaCol_ButtonHover,
    PapayaCol_ButtonActive,
    PapayaCol_AlphaGrid1,
    PapayaCol_AlphaGrid2,
    PapayaCol_ImageSizePreview1,
    PapayaCol_ImageSizePreview2,
    PapayaCol_Transparent,
    PapayaCol_COUNT
};

enum PapayaMesh_ {
    PapayaMesh_ImGui,
    PapayaMesh_Canvas,
    PapayaMesh_ImageSizePreview,
    PapayaMesh_AlphaGrid,
    PapayaMesh_BrushCursor,
    PapayaMesh_EyeDropperCursor,
    PapayaMesh_CropOutline,
    PapayaMesh_RTTBrush,
    PapayaMesh_RTTAdd,
    PapayaMesh_COUNT
};

enum PapayaShader_ {
    PapayaShader_ImGui,
    PapayaShader_VertexColor,
    PapayaShader_ImageSizePreview,
    PapayaShader_Brush,
    PapayaShader_BrushCursor,
    PapayaShader_EyeDropperCursor,
    PapayaShader_AlphaGrid,
    PapayaShader_PreMultiplyAlpha,
    PapayaShader_DeMultiplyAlpha,
    PapayaShader_COUNT
};

enum PapayaTool_ {
    PapayaTool_None,
    PapayaTool_Brush,
    PapayaTool_EyeDropper,
    PapayaTool_CropRotate,
    PapayaTool_COUNT
};

struct System {
    i32 gl_version[2];
    char* gl_vendor;
    char* gl_renderer;
};

struct Layout {
    i32 width, height;
    u32 menu_horizontal_offset, title_bar_buttons_width, title_bar_height;
    f32 proj_mtx[4][4];
    i32 default_imgui_flags;
};

struct Document {
    Node* nodes[512];
    int node_count;
    Node* final_node;
    Node* current_node;
    i64 next_node_id;

    // Links are automatically maintained. Treat as read-only.
    NodeLink* links[512];
    int link_count;

    i32 width, height;
    i32 components_per_pixel;
    Vec2i canvas_pos;
    f32 canvas_zoom;
    f32 inverse_aspect;
    f32 proj_mtx[4][4];
    UndoBuffer undo;
};

struct Mouse {
    Vec2i pos;
    Vec2i last_pos;
    Vec2 uv;
    Vec2 last_uv;
    bool is_down[3];
    bool was_down[3];
    bool pressed[3];
    bool released[3];
    bool in_workspace;
};

struct Keyboard {
    bool shift;
    bool ctrl;
};

struct Tablet {
    Vec2i pos;
    f32 pressure;
    i32 buttons;
};

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
};

struct EyeDropper {
    Color color;
};

struct Profile {
    i64 current_time; // Used on Windows.
    f32 last_frame_time; // Used on Linux. TODO: Combine this var and the one above.
};

struct Misc {
    // TODO: This entire struct is for stuff to be refactored at some point
    u32 fbo;
    u32 fbo_render_tex, fbo_sample_tex;
    bool draw_overlay;
    bool show_metrics;
    bool show_undo_buffer;
    bool show_nodes;
    bool menu_open;
    bool prefs_open;
    bool preview_image_size;
    i32 preview_width, preview_height;
    u32 canvas_tex; // Temporarily used for visualization during node bringup
    i32 w, h;
    u32 vertex_shader;
};

struct PapayaMemory {
    bool is_running;
    System sys;
    Layout window;
    Keyboard keyboard;
    Mouse mouse;
    Tablet tablet;
    Profile profile;

    ImVector<Document> docs; // TODO: Use custom vector type?
    Document* cur_doc;
    PapayaDocument* doc;

    u32 textures[PapayaTex_COUNT];
    Color colors[PapayaCol_COUNT];
    PaglMesh* meshes[PapayaMesh_COUNT]; // TODO: Instead of array,
                                        //       move to respective files
    PaglProgram* shaders[PapayaShader_COUNT];

    PapayaTool_ current_tool;
    Brush brush;
    EyeDropper eye_dropper;
    CropRotate crop_rotate;
    ColorPanel* color_panel;
    GraphPanel* graph_panel;
    Misc misc;
};

namespace core {
    void init(PapayaMemory* mem);
    void destroy(PapayaMemory* mem);
    void resize(PapayaMemory* mem, i32 width, i32 height);
    void update(PapayaMemory* mem);
    void render_imgui(ImDrawData* draw_data, void* mem_ptr);
    bool open_doc(const char* path, PapayaMemory* mem);
    void close_doc(PapayaMemory* mem);
    void resize_doc(PapayaMemory* mem, i32 width, i32 height);
    void update_canvas(PapayaMemory* mem);
}

namespace platform
{
    void print(const char* Message);
    void start_mouse_capture();
    void stop_mouse_capture();
    void set_mouse_position(i32 x, i32 y);
    void set_cursor_visibility(bool Visible);
    char* open_file_dialog();
    char* save_file_dialog();
}
