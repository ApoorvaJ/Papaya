
/*
    Single-header OpenGL helper library
*/

#ifndef PAGL_H
#define PAGL_H

enum Pagl_UniformType_ {
    Pagl_UniformType_Float,
    Pagl_UniformType_Vec2,
    Pagl_UniformType_Matrix4,
    Pagl_UniformType_Color,
    Pagl_UniformType_COUNT
};

struct Pagl_Program {
    u32 id;
    const char* name;
    i32 num_attribs, num_uniforms;
    i32* attribs;
    i32* uniforms;
};

struct Pagl_Mesh {
    u32 type;
    u32 vbo_size, index_count;
    u32 vbo_handle, elements_handle;
};

// TODO: Turn this off in release mode
#define GLCHK(stmt) stmt; pagl_check_error(#stmt, __FILE__, __LINE__)

// State management
void pagl_check_error(const char* expr, const char* file, i32 line);
void pagl_init(void);
void pagl_destroy(void);
void pagl_push_state(void);
void pagl_pop_state(void);
void pagl_enable(i32 count, ...);
void pagl_disable(i32 count, ...);

// Shaders
u32 pagl_compile_shader(const char* name, const char* src, u32 type);
Pagl_Program* pagl_init_program(const char* name, u32 vert_id, u32 frag_id,
                                i32 num_attribs, i32 num_uniforms, ...);
void pagl_destroy_program(Pagl_Program* p);
void pagl_set_vertex_attribs(Pagl_Program* p);

// Meshes
Pagl_Mesh* pagl_init_quad_mesh(Vec2 pos, Vec2 sz, u32 usage);
void pagl_destroy_mesh(Pagl_Mesh* mesh);
void pagl_transform_quad_mesh(Pagl_Mesh* mesh, Vec2 pos, Vec2 sz);
void pagl_draw_mesh(Pagl_Mesh* mesh, Pagl_Program* pgm, i32 num_uniforms, ...);

// Textures
u32 pagl_alloc_texture(i32 w, i32 h, u8* data);

#endif // PAGL_H


// =============================================================================


#ifdef PAGL_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

/*
    SECTION: State management
*/

struct pagl_state {
    u32 caps_edited;
    u32 caps_state;
};

static pagl_state* state_stack;
static i32 stack_idx;
static i32 stack_size;

void pagl_check_error(const char* expr, const char* file, i32 line)
{
    const char* s;

    switch(glGetError()) {
        case GL_NO_ERROR:                      return;
        case GL_INVALID_ENUM:                  s = "Invalid enum";      break;
        case GL_INVALID_VALUE:                 s = "Invalid value";     break;
        case GL_INVALID_OPERATION:             s = "Invalid operation"; break;
        case GL_OUT_OF_MEMORY:                 s = "Out of memory";     break;
        case GL_STACK_UNDERFLOW:               s = "Stack underflow";   break;
        case GL_STACK_OVERFLOW:                s = "Stack overflow";    break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: s = "Invalid framebuffer operation"; break;
        default:                               s = "Undefined error";   break;
    }

    printf("OpenGL error: %s in %s:%d\n", s, file, line);
    printf("    Expression: %s\n", expr);
}

void pagl_init()
{
    if (state_stack) { return; }
    // The stack is implemented as a static array. Can be converted to a dynamic
    // array later if required.
    stack_idx = -1;
    stack_size = 4;
    state_stack = (pagl_state*) calloc(1, sizeof(pagl_state) * stack_size);
}

void pagl_destroy()
{
    free(state_stack);
}

void pagl_push_state()
{
    stack_idx++;
    assert(stack_idx < stack_size);
}

/*
    Converts an OpenGL capability enum (e.g. GL_SCISSOR_TEST) into its
    corresponding mask to be stored in the internal state stack in this file.
*/
static u32 cap_to_mask(u32 gl_cap)
{
    switch (gl_cap) {
        case GL_BLEND:        return 1 << 0;
        case GL_SCISSOR_TEST: return 1 << 1;
        case GL_DEPTH_TEST:   return 1 << 2;
        case GL_CULL_FACE:    return 1 << 3;
        default:              return 0;
    }
}

static void revert_cap(u32 gl_cap)
{
    u32 m = cap_to_mask(gl_cap);
    pagl_state* s = &state_stack[stack_idx];
    if (s->caps_edited & m) {
        u8 was_on = s->caps_state & m;
        if (was_on) {
            GLCHK( glEnable(gl_cap) );
        } else {
            GLCHK( glDisable(gl_cap) );
        }
    }
}

void pagl_pop_state()
{
    assert(stack_idx >= 0);
    revert_cap(GL_BLEND);
    revert_cap(GL_SCISSOR_TEST);
    revert_cap(GL_DEPTH_TEST);
    revert_cap(GL_CULL_FACE);

    pagl_state* s = &state_stack[stack_idx];
    memset(s, 0, sizeof(pagl_state)); // Zero out the stack frame
    stack_idx--;
}

void pagl_enable(i32 count, ...)
{
    va_list args;
    va_start(args, count);

    for (i32 i = 0; i < count; i++) {
        u32 gl_cap = va_arg(args, u32);

        u8 on;
        GLCHK( glGetBooleanv(gl_cap, &on) );
        if (on) {
            return;
        }

        pagl_state* s = &state_stack[stack_idx];
        u32 m = cap_to_mask(gl_cap);

        // Caps should not be modified twice in a single stack frame.
        assert((s->caps_edited & m) == 0);
        s->caps_edited |= m;
        // No need to set bit in caps_state because it will already be zero.

        GLCHK( glEnable(gl_cap) );
    }

    va_end(args);
}

void pagl_disable(i32 count, ...)
{

    va_list args;
    va_start(args, count);

    for (i32 i = 0; i < count; i++) {
        u32 gl_cap = va_arg(args, u32);

        u8 on;
        GLCHK( glGetBooleanv(gl_cap, &on) );
        if (!on) {
            return;
        }

        pagl_state* s = &state_stack[stack_idx];
        u32 m = cap_to_mask(gl_cap);

        // Caps should not be modified twice in a single stack frame.
        assert((s->caps_edited & m) == 0);
        s->caps_edited |= m;
        s->caps_state |= m;

        GLCHK( glDisable(gl_cap) );
    }

    va_end(args);
}

/*
    SECTION: Shaders
*/

u32 pagl_compile_shader(const char* name, const char* src, u32 type)
{
    u32 id = GLCHK( glCreateShader(type) );
    GLCHK( glShaderSource (id, 1, &src, 0) );
    GLCHK( glCompileShader(id) );

    // Print compilation errors
    i32 res;
    GLCHK( glGetShaderiv(id, GL_COMPILE_STATUS, &res) );
    if (res != GL_TRUE) {
        char log[4096];
        i32 len;

        GLCHK( glGetShaderInfoLog(id, 4096, &len, log) );
        printf("Compilation error in %s shader\n%s\n", name, log);
    }

    return id;
}

Pagl_Program* pagl_init_program(const char* name, u32 vert_id, u32 frag_id,
                                i32 num_attribs, i32 num_uniforms, ...)
{
    Pagl_Program* p = (Pagl_Program*) calloc(sizeof(*p), 1);
    p->name = name;
    p->num_attribs = num_attribs;
    p->num_uniforms = num_uniforms;
    p->attribs = (i32*) calloc(sizeof(i32) * num_attribs, 1);
    p->uniforms = (i32*) calloc(sizeof(i32) * num_uniforms, 1);
    p->id = GLCHK( glCreateProgram() );

    GLCHK( glAttachShader (p->id, vert_id) );
    GLCHK( glAttachShader (p->id, frag_id) );
    GLCHK( glLinkProgram(p->id) );
    // TODO: Print linking errors

    va_list args;
    va_start(args, num_uniforms);
    for (i32 i = 0; i < num_attribs; i++) {
        const char* arg = va_arg(args, const char*);
        p->attribs[i] = GLCHK( glGetAttribLocation(p->id, arg) );

        if (p->attribs[i] == -1) {
            printf("Attribute %s not found in %s program\n", arg, name);
        }
    }

    for (i32 i = 0; i < num_uniforms; i++) {
        const char* arg = va_arg(args, const char*);
        p->uniforms[i] = GLCHK( glGetUniformLocation(p->id, arg) );

        if (p->uniforms[i] == -1) {
            printf("Uniform %s not found in %s program\n", arg, name);
        }
    }
    va_end(args);

    return p;
}

void pagl_destroy_program(Pagl_Program* p)
{
    free(p->attribs);
    free(p->uniforms);
    free(p);
}

void pagl_set_vertex_attribs(Pagl_Program* p)
{
    // Vertex attribs
    i32* a = p->attribs;
    for (i32 i = 0; i < p->num_attribs; i++) {
        GLCHK( glEnableVertexAttribArray(a[i]) );
    }

    // Pos
    GLCHK( glVertexAttribPointer(a[0], 2, GL_FLOAT, GL_FALSE,
                                 sizeof(ImDrawVert),
                                 (GLvoid*)offsetof(ImDrawVert, pos)) );
    // UV
    GLCHK( glVertexAttribPointer(a[1], 2, GL_FLOAT, GL_FALSE,
                                 sizeof(ImDrawVert),
                                 (GLvoid*)offsetof(ImDrawVert, uv)) );
    if (p->num_attribs > 2) {
        // Color attribute
        GLCHK( glVertexAttribPointer(a[2], 4, GL_UNSIGNED_BYTE, GL_TRUE,
                                     sizeof(ImDrawVert),
                                     (GLvoid*)offsetof(ImDrawVert, col)) );
    }
}


/*
    SECTION: Meshes
*/

Pagl_Mesh* pagl_init_quad_mesh(Vec2 pos, Vec2 sz, u32 usage)
{
    Pagl_Mesh* m = (Pagl_Mesh*) calloc(sizeof(*m), 1);
    GLCHK( glGenBuffers  (1, &m->vbo_handle) );
    GLCHK( glBindBuffer  (GL_ARRAY_BUFFER, m->vbo_handle) );
    GLCHK( glBufferData(GL_ARRAY_BUFFER, sizeof(ImDrawVert) * 6,
                        0, (GLenum)usage) );
    m->type = GL_TRIANGLES;
    m->index_count = 6;
    pagl_transform_quad_mesh(m, pos, sz);
    return m;
}

void pagl_destroy_mesh(Pagl_Mesh* mesh)
{
    free(mesh);
}

void pagl_transform_quad_mesh(Pagl_Mesh* mesh, Vec2 pos, Vec2 sz)
{
    float x1 = pos.x;
    float x2 = pos.x + sz.x;
    float y1 = pos.y;
    float y2 = pos.y + sz.y;
    u32 col = 0xffffffff;

    // TODO: Either remove or formalize this dependency on ImGui
    ImDrawVert v[6];
    v[0].pos = Vec2(x1, y2); v[0].uv = Vec2(0.0f, 1.0f); v[0].col = col;
    v[1].pos = Vec2(x1, y1); v[1].uv = Vec2(0.0f, 0.0f); v[1].col = col;
    v[2].pos = Vec2(x2, y2); v[2].uv = Vec2(1.0f, 1.0f); v[2].col = col;
    v[3].pos = Vec2(x2, y1); v[3].uv = Vec2(1.0f, 0.0f); v[3].col = col;
    v[4].pos = Vec2(x2, y2); v[4].uv = Vec2(1.0f, 1.0f); v[4].col = col;
    v[5].pos = Vec2(x1, y1); v[5].uv = Vec2(0.0f, 0.0f); v[5].col = col;

    GLCHK( glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_handle) );
    GLCHK( glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v) );
}

void pagl_draw_mesh(Pagl_Mesh* mesh, Pagl_Program* pgm, i32 num_uniforms, ...)
{
    GLint last_program, last_texture;
    GLCHK( glGetIntegerv(GL_CURRENT_PROGRAM, &last_program) );
    GLCHK( glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture) );
    glDisable(GL_SCISSOR_TEST);
    GLCHK( glEnable(GL_BLEND) );
    GLCHK( glBlendEquation(GL_FUNC_ADD) );
    GLCHK( glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) );

    GLCHK( glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_handle) );
    GLCHK( glUseProgram(pgm->id) );

    // Uniforms
    va_list args;
    va_start(args, num_uniforms);
    for (i32 i = 0; i < num_uniforms; i++) {
        i32 u = pgm->uniforms[i];

        switch (va_arg(args, int)) {
            case Pagl_UniformType_Float: {
                GLCHK( glUniform1f(u, (float)va_arg(args, double)) );
            } break;

            case Pagl_UniformType_Matrix4: {
                GLCHK( glUniformMatrix4fv(u, 1, GL_FALSE, va_arg(args, float*)) );
            } break;

            case Pagl_UniformType_Vec2: {
                Vec2 vec = va_arg(args, Vec2);
                GLCHK( glUniform2f(u, vec.x, vec.y) );
            } break;

            case Pagl_UniformType_Color: {
                Color col = va_arg(args, Color);
                GLCHK( glUniform4f(u, col.r, col.g, col.b, col.a) );
            } break;
        }
    }
    va_end(args);

    pagl_set_vertex_attribs(pgm);

    GLCHK( glLineWidth(2.0f) ); // TODO: Move to callee
    GLCHK( glDrawArrays(mesh->type, 0, mesh->index_count) );
}

/*
    SECTION: Textures
*/ 

u32 pagl_alloc_texture(i32 w, i32 h, u8* data)
{
    u32 tex;
    GLCHK( glGenTextures(1, &tex) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, tex) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
    GLCHK( glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA,
                        GL_UNSIGNED_BYTE, data) );
    return tex;
}

#endif // PAGL_IMPLEMENTATION
