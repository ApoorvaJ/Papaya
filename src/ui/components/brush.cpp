
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
    destroy_brush_meshes(b);
    pagl_destroy_mesh(b->mesh_cursor);
    pagl_destroy_program(b->pgm_cursor);
    pagl_destroy_program(b->pgm_stroke);
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

/*
    Falloff visualization - only used for debug purposes. Not marked as static
    to avoid the unused function warning-as-error
*/
void visualize_brush_falloff(PapayaMemory* mem)
{
    const i32 num = 256; // Number of samples in graph
    static f32 opacities[num] = { 0 };

    f32 scale = 1.0f / (1.0f - mem->brush->hardness);
    f32 phase = (1.0f - scale) * (f32)M_PI;
    f32 period = (f32)M_PI * scale / (f32)num;

    for (i32 i = 0; i < num; i++)
    {
        opacities[i] = (cosf(((f32)i * period) + phase) + 1.0f) * 0.5f;
        if ((f32)i < (f32)num - ((f32)num / scale)) { opacities[i] = 1.0f; }
    }

    ImGui::Begin("Brush falloff");
    ImGui::PlotLines("", opacities, num, 0, 0, FLT_MIN, FLT_MAX, Vec2(256,256));
    ImGui::End();
}

static void clear_brush_frame_buffer(PapayaMemory* mem)
{
    GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, mem->misc.fbo) );
    GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_TEXTURE_2D, mem->misc.fbo_sample_tex, 0) );
    GLCHK( glClearColor(0.0f, 0.0f, 0.0f, 0.0f) );
    GLCHK( glClear(GL_COLOR_BUFFER_BIT) );
    GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mem->misc.fbo_render_tex, 0) );
    GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
}

static void draw_brush_stroke(PapayaMemory* mem)
{
    Brush* b = mem->brush;
    mem->misc.draw_overlay = true;

    // TODO: Handle shift press for straight brush strokes

    // Bind frame buffer
    GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, mem->misc.fbo) );
    GLCHK( glViewport(0, 0, mem->cur_doc->width, mem->cur_doc->height) );
    GLCHK( glDisable(GL_BLEND) );
    GLCHK( glDisable(GL_SCISSOR_TEST) );

    // TODO: Call pagl_draw_mesh
    {
        GLCHK( glUseProgram(b->pgm_stroke->id) );

        f32 width  = (f32)mem->cur_doc->width;
        f32 height = (f32)mem->cur_doc->height;
        Vec2 Correction = (b->diameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
        Vec2 CorrectedPos = mem->mouse.uv + Correction;
        Vec2 CorrectedLastPos = (b->draw_line_segment ? b->line_segment_start_uv : mem->mouse.last_uv) + Correction;

        b->draw_line_segment = false;

        PaglProgram* pgm = b->pgm_stroke;
        GLCHK( glUniformMatrix4fv(pgm->uniforms[0], 1, GL_FALSE, &mem->cur_doc->proj_mtx[0][0]) );
        GLCHK( glUniform2f(pgm->uniforms[2], CorrectedPos.x, CorrectedPos.y * mem->cur_doc->inverse_aspect) ); // Pos uniform
        GLCHK( glUniform2f(pgm->uniforms[3], CorrectedLastPos.x, CorrectedLastPos.y * mem->cur_doc->inverse_aspect) ); // Lastpos uniform
        GLCHK( glUniform1f(pgm->uniforms[4], (f32)b->diameter / ((f32)mem->cur_doc->width * 2.0f)) );
        f32 Opacity = b->opacity;
        //if (mem->tablet.Pressure > 0.0f) { Opacity *= mem->tablet.Pressure; }
        GLCHK( glUniform4f(pgm->uniforms[5], mem->color_panel->current_color.r,
                    mem->color_panel->current_color.g,
                    mem->color_panel->current_color.b,
                    Opacity) );
        // Brush hardness
        {
            f32 Hardness;
            if (b->anti_alias && b->diameter > 2)
            {
                f32 AAWidth = 1.0f; // The width of pixels over which the antialiased falloff occurs
                f32 Radius  = b->diameter / 2.0f;
                Hardness      = math::min(b->hardness, 1.0f - (AAWidth / Radius));
            }
            else
            {
                Hardness      = b->hardness;
            }

            GLCHK( glUniform1f(pgm->uniforms[6], Hardness) );
        }

        GLCHK( glUniform1f(pgm->uniforms[7], mem->cur_doc->inverse_aspect) ); // Inverse Aspect uniform

        GLCHK( glBindBuffer(GL_ARRAY_BUFFER, b->mesh_RTTBrush->vbo_handle) );
        pagl_set_vertex_attribs(pgm);

        GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)mem->misc.fbo_sample_tex) );
        GLCHK( glDrawArrays(GL_TRIANGLES, 0, 6) );
    }

    // Swap textures
    u32 Temp = mem->misc.fbo_render_tex;
    mem->misc.fbo_render_tex = mem->misc.fbo_sample_tex;
    mem->misc.fbo_sample_tex = Temp;
    // TODO: Necessary?
    GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_TEXTURE_2D, mem->misc.fbo_render_tex, 0) );

    // Reset frame buffer and viewport
    GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
    GLCHK( glViewport(0, 0, (i32)ImGui::GetIO().DisplaySize.x, (i32)ImGui::GetIO().DisplaySize.y) );


}

static void merge_brush_stroke(PapayaMemory* mem)
{
    Brush* b = mem->brush;

    mem->misc.draw_overlay = false;
    b->being_dragged = false;
    b->is_straight_drag = false;
    b->was_straight_drag = false;
    b->line_segment_start_uv = mem->mouse.uv;

    // TODO: Implement the actual merging
}

void update_and_render_brush(PapayaMemory* mem)
{
    Brush* b = mem->brush;
    Mouse* mouse = &mem->mouse;

    if (mouse->pressed[1]) {

        // Right click started
        b->rt_drag_start_pos = mouse->pos;
        b->rt_drag_start_diameter = b->diameter;
        b->rt_drag_start_hardness = b->hardness;
        b->rt_drag_start_opacity = b->opacity;
        b->rt_drag_with_shift = ImGui::GetIO().KeyShift;
        platform::start_mouse_capture();
        platform::set_cursor_visibility(false);

    } else if (mouse->is_down[1]) {

        // Right mouse dragged
        Vec2 delta = ImGui::GetMouseDragDelta(1);
        if (b->rt_drag_with_shift) {
            f32 opacity = b->rt_drag_start_opacity + (delta.x * 0.0025f);
            b->opacity = math::clamp(opacity, 0.0f, 1.0f);
        } else {
            f32 zoom = mem->cur_doc ? mem->cur_doc->canvas_zoom : 1.0f;
            f32 dia = b->rt_drag_start_diameter + (delta.x / zoom * 2.0f);
            b->diameter = math::clamp((i32)dia, 1, b->max_diameter);

            f32 hardness = b->rt_drag_start_hardness + (delta.y * 0.0025f);
            b->hardness = math::clamp(hardness, 0.0f, 1.0f);
        }

    } else if (mouse->released[1]) {

        // Right click released
        platform::stop_mouse_capture();
        platform::set_mouse_position(b->rt_drag_start_pos.x,
                                     b->rt_drag_start_pos.y);
        platform::set_cursor_visibility(true);

    } else if (mouse->pressed[0] && mouse->in_workspace) {

        // Left click started
        b->being_dragged = true;
        b->draw_line_segment = (ImGui::GetIO().KeyShift &&
                                b->line_segment_start_uv.x >= 0.0f);
        b->paint_area_1 = Vec2i(mem->cur_doc->width + 1,
                                mem->cur_doc->height + 1);
        b->paint_area_2 = Vec2i(0,0);
        clear_brush_frame_buffer(mem);

        ColorPanel* c = mem->color_panel;
        if (c->is_open) {
            c->current_color = c->new_color;
        }

    } else if (mouse->is_down[0]) {

        // Left mouse dragged
        draw_brush_stroke(mem);

    } else if (mouse->released[0] && b->being_dragged) {

        // Left click released
        merge_brush_stroke(mem);
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
