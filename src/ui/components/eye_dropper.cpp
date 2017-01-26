
#include "eye_dropper.h"
#include "libs/mathlib.h"
#include "ui.h"
#include "pagl.h"
#include "gl_lite.h"
#include "color_panel.h"

static PaglProgram* compile_shaders(u32 vertex_shader);

EyeDropper* init_eye_dropper(PapayaMemory* mem)
{
    EyeDropper* e = (EyeDropper*) calloc(sizeof(*e), 1);
    e->mesh = pagl_init_quad_mesh(Vec2(40, 60), Vec2(30, 30), GL_DYNAMIC_DRAW);
    e->pgm = compile_shaders(mem->misc.vertex_shader);
    return e;
}

void destroy_eye_dropper(EyeDropper* e)
{
    pagl_destroy_mesh(e->mesh);
    pagl_destroy_program(e->pgm);
    free(e);
}

void update_and_render_eye_dropper(PapayaMemory* mem)
{
    // Get pixel color
    f32 pix[3] = { 0 };
    GLCHK( glReadPixels((i32)mem->mouse.pos.x,
                        mem->window.height - (i32)mem->mouse.pos.y,
                        1, 1, GL_RGB, GL_FLOAT, pix) );
    Color col = Color(pix[0], pix[1], pix[2]);

    if (mem->mouse.is_down[0]) {
        Vec2 sz = Vec2(230,230);
        pagl_transform_quad_mesh(mem->eye_dropper->mesh,
                                 mem->mouse.pos - (sz * 0.5f), sz);

        pagl_push_state();
        pagl_enable(1, GL_SCISSOR_TEST);
        pagl_draw_mesh(mem->eye_dropper->mesh, mem->eye_dropper->pgm,
                       3,
                       Pagl_UniformType_Matrix4, &mem->window.proj_mtx[0][0],
                       Pagl_UniformType_Color, col,
                       Pagl_UniformType_Color, mem->color_panel->new_color);
        pagl_pop_state();
    } else if (mem->mouse.released[0]) {
        color_panel_set_color(col, mem->color_panel, mem->color_panel->is_open);
    }
}

static PaglProgram* compile_shaders(u32 vertex_shader)
{
    const char* frag_src =
"   #version 120                                                            \n"
"                                                                           \n"
"   uniform vec4 col1; // Uniforms[1]                                       \n"
"   uniform vec4 col2; // Uniforms[2]                                       \n"
"                                                                           \n"
"   varying vec2 frag_uv;                                                   \n"
"                                                                           \n"
"   void main()                                                             \n"
"   {                                                                       \n"
"       float d = length(vec2(0.5,0.5) - frag_uv);                          \n"
"       float t = 1.0 - clamp((d - 0.49) * 250.0, 0.0, 1.0);                \n"
"       t = t - 1.0 + clamp((d - 0.4) * 250.0, 0.0, 1.0);                   \n"
"       gl_FragColor = (frag_uv.y < 0.5) ? col1: col2;                      \n"
"       gl_FragColor.a = t;                                                 \n"
"   }                                                                       \n";

    const char* name = "eye dropper cursor";
    u32 frag = pagl_compile_shader(name, frag_src, GL_FRAGMENT_SHADER);
    return pagl_init_program(name, vertex_shader, frag, 2, 3,
                               "pos", "uv",
                               "proj_mtx", "col1", "col2");
}
