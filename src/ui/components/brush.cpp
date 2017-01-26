
#include "brush.h"
#include "libs/mathlib.h"
#include "ui.h"
#include "pagl.h"
#include "gl_lite.h"
#include "color_panel.h"

// This file is currently very rough, because it will be substantially affected
// by the addition of nodes.

static PaglProgram* compile_cursor_shader(u32 vertex_shader);
static PaglProgram* compile_stroke_shader(u32 vertex_shader);

Brush* init_brush(PapayaMemory* mem)
{
    Brush* b = (Brush*) calloc(sizeof(*b), 1);
    b->diameter = 100;
    b->max_diameter = 9999;
    b->hardness = 1.0f;
    b->opacity = 1.0f;
    b->anti_alias = true;
    b->line_segment_start_uv = Vec2(-1.0f, -1.0f);
    b->mesh_cursor = pagl_init_quad_mesh(Vec2(40, 60), Vec2(30, 30),
                                         GL_DYNAMIC_DRAW);
    b->pgm_cursor = compile_cursor_shader(mem->misc.vertex_shader);
    b->pgm_stroke = compile_stroke_shader(mem->misc.vertex_shader);
    return b;
}

void destroy_brush(Brush* b)
{

}

void resize_brush_meshes(Brush* b, Vec2 size)
{
    destroy_brush_meshes(b);
    b->mesh_RTTBrush = pagl_init_quad_mesh(Vec2(0,0), size, GL_STATIC_DRAW);
    b->mesh_RTTAdd = pagl_init_quad_mesh(Vec2(0,0), size, GL_STATIC_DRAW);
}

void destroy_brush_meshes(Brush* b)
{
    if (b->mesh_RTTBrush) {
        pagl_destroy_mesh(b->mesh_RTTBrush);
    }

    if (b->mesh_RTTAdd) {
        pagl_destroy_mesh(b->mesh_RTTAdd);
    }
}

static PaglProgram* compile_cursor_shader(u32 vertex_shader)
{
    const char* frag_src =
"   #version 120                                                            \n"
"                                                                           \n"
"   #define M_PI 3.1415926535897932384626433832795                          \n"
"                                                                           \n"
"   uniform vec4 brush_col;                                                 \n"
"   uniform float hardness;                                                 \n"
"   uniform float pixel_sz; // Size in pixels                               \n"
"                                                                           \n"
"   varying vec2 frag_uv;                                                   \n"
"                                                                           \n"
"                                                                           \n"
"   void main()                                                             \n"
"   {                                                                       \n"
"       float scale = 1.0 / (1.0 - hardness);                               \n"
"       float period = M_PI * scale;                                        \n"
"       float phase = (1.0 - scale) * M_PI * 0.5;                           \n"
"       float dist = distance(frag_uv, vec2(0.5,0.5));                      \n"
"       float alpha = cos((period * dist) + phase);                         \n"
"       if (dist < 0.5 - (0.5/scale)) {                                     \n"
"           alpha = 1.0;                                                    \n"
"       } else if (dist > 0.5) {                                            \n"
"           alpha = 0.0;                                                    \n"
"       }                                                                   \n"
"       float border = 1.0 / pixel_sz; // Thickness of border               \n"
"       gl_FragColor = (dist > 0.5 - border && dist < 0.5) ?                \n"
"           vec4(0.0,0.0,0.0,1.0) :                                         \n"
"           vec4(brush_col.rgb, alpha * brush_col.a);                       \n"
"   }                                                                       \n";

    const char* name = "brush cursor";
    u32 frag = pagl_compile_shader(name, frag_src, GL_FRAGMENT_SHADER);
    return pagl_init_program(name, vertex_shader, frag, 2, 4,
                             "pos", "uv",
                             "proj_mtx", "brush_col", "hardness",
                             "pixel_sz");
}

static PaglProgram* compile_stroke_shader(u32 vertex_shader)
{
    const char* frag_src =
"   #version 120                                                            \n"
"                                                                           \n"
"   #define M_PI 3.1415926535897932384626433832795                          \n"
"                                                                           \n"
"   uniform sampler2D tex;    // uniforms[1]                                \n"
"   uniform vec2 cur_pos;     // uniforms[2]                                \n"
"   uniform vec2 last_pos;    // uniforms[3]                                \n"
"   uniform float radius;     // uniforms[4]                                \n"
"   uniform vec4 brush_col;   // uniforms[5]                                \n"
"   uniform float hardness;   // uniforms[6]                                \n"
"   uniform float inv_aspect; // uniforms[7]                                \n"
"                                                                           \n"
"   varying vec2 frag_uv;                                                   \n"
"                                                                           \n"
"   bool isnan(float val)                                                   \n"
"   {                                                                       \n"
"       return !( val < 0.0 || 0.0 < val || val == 0.0 );                   \n"
"   }                                                                       \n"
"                                                                           \n"
"   void line(vec2 p1, vec2 p2, vec2 uv, float radius,                      \n"
"             out float distLine, out float distp1)                         \n"
"   {                                                                       \n"
"       if (distance(p1,p2) <= 0.0) {                                       \n"
"           distLine = distance(uv, p1);                                    \n"
"           distp1 = 0.0;                                                   \n"
"           return;                                                         \n"
"       }                                                                   \n"
"                                                                           \n"
"       float a = abs(distance(p1, uv));                                    \n"
"       float b = abs(distance(p2, uv));                                    \n"
"       float c = abs(distance(p1, p2));                                    \n"
"       float d = sqrt(c*c + radius*radius);                                \n"
"                                                                           \n"
"       vec2 a1 = normalize(uv - p1);                                       \n"
"       vec2 b1 = normalize(uv - p2);                                       \n"
"       vec2 c1 = normalize(p2 - p1);                                       \n"
"       if (dot(a1,c1) < 0.0) {                                             \n"
"           distLine = a;                                                   \n"
"           distp1 = 0.0;                                                   \n"
"           return;                                                         \n"
"       }                                                                   \n"
"                                                                           \n"
"       if (dot(b1,c1) > 0.0) {                                             \n"
"           distLine = b;                                                   \n"
"           distp1 = 1.0;                                                   \n"
"           return;                                                         \n"
"       }                                                                   \n"
"                                                                           \n"
"       float p = (a + b + c) * 0.5;                                        \n"
"       float h = 2.0 / c * sqrt( p * (p-a) * (p-b) * (p-c) );              \n"
"                                                                           \n"
"       if (isnan(h)) {                                                     \n"
"           distLine = 0.0;                                                 \n"
"           distp1 = a / c;                                                 \n"
"       } else {                                                            \n"
"           distLine = h;                                                   \n"
"           distp1 = sqrt(a*a - h*h) / c;                                   \n"
"       }                                                                   \n"
"   }                                                                       \n"
"                                                                           \n"
"   void main()                                                             \n"
"   {                                                                       \n"
"       vec4 t = texture2D(tex, frag_uv.st);                                \n"
"                                                                           \n"
"       float distLine, distp1;                                             \n"
"       vec2 aspect_uv = vec2(frag_uv.x, frag_uv.y * inv_aspect);           \n"
"       line(last_pos, cur_pos, aspect_uv, radius, distLine, distp1);       \n"
"                                                                           \n"
"       float scale = 1.0 / (2.0 * radius * (1.0 - hardness));              \n"
"       float period = M_PI * scale;                                        \n"
"       float phase = (1.0 - scale * 2.0 * radius) * M_PI * 0.5;            \n"
"       float alpha = cos((period * distLine) + phase);                     \n"
"                                                                           \n"
"       if (distLine < radius - (0.5/scale)) {                              \n"
"           alpha = 1.0;                                                    \n"
"       } else if (distLine > radius) {                                     \n"
"           alpha = 0.0;                                                    \n"
"       }                                                                   \n"
"                                                                           \n"
"       float final_alpha = max(t.a, alpha * brush_col.a);                  \n"
"                                                                           \n"
"       // TODO: Needs improvement. Self-intersection corners look weird.   \n"
"       gl_FragColor = vec4(brush_col.rgb,                                  \n"
"                           clamp(final_alpha,0.0,1.0));                    \n"
"   }                                                                       \n";
    const char* name = "brush stroke";
    u32 frag = pagl_compile_shader(name, frag_src, GL_FRAGMENT_SHADER);
    return pagl_init_program(name, vertex_shader, frag, 2, 8,
                             "pos", "uv",
                             "proj_mtx", "tex", "cur_pos", "last_pos", "radius",
                             "brush_col", "hardness", "inv_aspect");
}
