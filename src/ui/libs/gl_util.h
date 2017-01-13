
#ifndef GL_UTIL_H
#define GL_UTIL_H

#include "gl_lite.h"
#include "types.h"
#include "pagl.h"

// TODO: Turn this off in release mode
#define GLCHK(stmt) stmt; gl::check_error(#stmt, __FILE__, __LINE__)

enum UniformType_ {
    UniformType_Float,
    UniformType_Vec2,
    UniformType_Matrix4,
    UniformType_Color,
    UniformType_COUNT
};

struct Mesh {
    bool is_line_loop; // 0 -> Triangle, !0 -> Line
    u32 vbo_size, index_count;
    u32 vbo_handle, elements_handle;
};

namespace gl {
    void check_error(const char* expr, const char* file, int line);
    char* read_file(const char* file_name);
    void set_vertex_attribs(Pagl_Program* shader);
    void init_quad(Mesh& mesh, Vec2 pos, Vec2 size, u32 usage);
    void transform_quad(Mesh& mesh, Vec2 pos, Vec2 size);
    void draw_mesh(Mesh& mesh, Pagl_Program* shader, bool scissor, i32 uniform_count, ...);
    u32 allocate_tex(i32 width, i32 height, u8* data = 0);
}

#endif //GL_UTIL_H

// =============================================================================

#ifdef GL_UTIL_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>

void gl::check_error(const char* expr, const char* file, int line)
{
    const char* s;

    switch(glGetError()) {
    case GL_NO_ERROR: return;
    case GL_INVALID_ENUM:                  s = "Invalid enum"     ; break;
    case GL_INVALID_VALUE:                 s = "Invalid value"    ; break;
    case GL_INVALID_OPERATION:             s = "Invalid operation"; break;
    case GL_OUT_OF_MEMORY:                 s = "Out of memory"    ; break;
    case GL_STACK_UNDERFLOW:               s = "Stack underflow"  ; break;
    case GL_STACK_OVERFLOW:                s = "Stack overflow"   ; break;
    case GL_INVALID_FRAMEBUFFER_OPERATION: s = "Invalid framebuffer operation"; break;
    default:                               s = "Undefined error"  ; break;
    }

    printf("OpenGL error: %s in %s:%d\n", s, file, line);
    printf("    --- Expression: %s\n", expr);
}

char* gl::read_file(const char* file_name)
{
    char path[512];
    snprintf(path, sizeof(path), "shaders/%s", file_name);
    FILE* f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    size_t f_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buf = (char*)malloc(f_size + 1);
    fread(buf, f_size, 1, f);
    fclose(f);

    buf[f_size] = 0;
    return buf;
}

void gl::set_vertex_attribs(Pagl_Program* shader)
{
    for (i32 i = 0; i < shader->num_attribs; i++) {
        GLCHK( glEnableVertexAttribArray(shader->attribs[i]) );
    }

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    // Position attribute
    GLCHK( glVertexAttribPointer(shader->attribs[0], 2, GL_FLOAT, GL_FALSE,
        sizeof(ImDrawVert),
        (GLvoid*)OFFSETOF(ImDrawVert, pos)) );
    // UV attribute
    GLCHK( glVertexAttribPointer(shader->attribs[1], 2, GL_FLOAT, GL_FALSE,
        sizeof(ImDrawVert),
        (GLvoid*)OFFSETOF(ImDrawVert, uv)) );
    if (shader->num_attribs > 2) {
        // Color attribute 
        GLCHK( glVertexAttribPointer(shader->attribs[2], 4, GL_UNSIGNED_BYTE,
            GL_TRUE, sizeof(ImDrawVert),
            (GLvoid*)OFFSETOF(ImDrawVert, col)) );
    }
#undef OFFSETOF
}

void gl::init_quad(Mesh& mesh, Vec2 pos, Vec2 size, u32 usage)
{
    GLCHK( glGenBuffers  (1, &mesh.vbo_handle) );
    GLCHK( glBindBuffer  (GL_ARRAY_BUFFER, mesh.vbo_handle) );
    GLCHK( glBufferData(GL_ARRAY_BUFFER, sizeof(ImDrawVert) * 6, 0, (GLenum)usage) );
    mesh.index_count = 6;
    transform_quad(mesh, pos, size);
}

void gl::transform_quad(Mesh& mesh, Vec2 pos, Vec2 size)
{
    float x1 = pos.x;
    float x2 = pos.x + size.x;
    float y1 = pos.y;
    float y2 = pos.y + size.y;
    ImDrawVert verts[6];
    {
        verts[0].pos = Vec2(x1,y2); verts[0].uv = Vec2(0.0f, 1.0f); verts[0].col = 0xffffffff;
        verts[1].pos = Vec2(x1,y1); verts[1].uv = Vec2(0.0f, 0.0f); verts[1].col = 0xffffffff;
        verts[2].pos = Vec2(x2,y2); verts[2].uv = Vec2(1.0f, 1.0f); verts[2].col = 0xffffffff;
        verts[3].pos = Vec2(x2,y1); verts[3].uv = Vec2(1.0f, 0.0f); verts[3].col = 0xffffffff;
        verts[4].pos = Vec2(x2,y2); verts[4].uv = Vec2(1.0f, 1.0f); verts[4].col = 0xffffffff;
        verts[5].pos = Vec2(x1,y1); verts[5].uv = Vec2(0.0f, 0.0f); verts[5].col = 0xffffffff;
    }
    GLCHK( glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_handle) );
    GLCHK( glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts) );
}

void gl::draw_mesh(Mesh& mesh, Pagl_Program* shader, bool scissor,
    i32 uniform_count, ...)
{
    GLint last_program, last_texture;
    GLCHK( glGetIntegerv(GL_CURRENT_PROGRAM, &last_program) );
    GLCHK( glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture) );
    GLCHK( glEnable(GL_BLEND) );
    GLCHK( glBlendEquation(GL_FUNC_ADD) );
    GLCHK( glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) );
    GLCHK( glDisable(GL_CULL_FACE) );
    GLCHK( glDisable(GL_DEPTH_TEST) );
    if (scissor) { GLCHK( glEnable(GL_SCISSOR_TEST) ); }
    else{ GLCHK( glDisable(GL_SCISSOR_TEST) ); }

    GLCHK( glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_handle) );
    GLCHK( glUseProgram(shader->id) );

    // Uniforms
    {
        va_list args;
        va_start(args, uniform_count);
        for (i32 i = 0; i < uniform_count; i++) {
            switch (va_arg(args, int)) {
            case UniformType_Float: {
                GLCHK( glUniform1f(shader->uniforms[i],
                    (float)va_arg(args, double)) );
            } break;

            case UniformType_Matrix4: {
                GLCHK( glUniformMatrix4fv(shader->uniforms[i], 1, GL_FALSE,
                    va_arg(args, float*)) );
            } break;

            case UniformType_Vec2: {
                Vec2 vec = va_arg(args, Vec2);
                GLCHK( glUniform2f(shader->uniforms[i], vec.x, vec.y) );
            } break;

            case UniformType_Color: {
                Color col = va_arg(args, Color);
                GLCHK( glUniform4f(shader->uniforms[i], col.r, col.g, col.b,
                    col.a) );
            } break;
            }
        }
        va_end(args);
    }

    // Attribs
    set_vertex_attribs(shader);

    GLCHK( glLineWidth(2.0f) );
    GLCHK( glDrawArrays(mesh.is_line_loop ? GL_LINE_LOOP : GL_TRIANGLES, 0,
        mesh.index_count) );

    // Restore modified state
    GLCHK( glBindBuffer (GL_ARRAY_BUFFER, 0) );
    GLCHK( glUseProgram (last_program) );                // TODO: Necessary?
    GLCHK( glDisable    (GL_SCISSOR_TEST) );             //
    GLCHK( glDisable    (GL_BLEND) );                    //
    GLCHK( glBindTexture(GL_TEXTURE_2D, last_texture) ); //
}

u32 gl::allocate_tex(i32 width, i32 height, u8* data)
{
    u32 tex;
    GLCHK( glGenTextures(1, &tex) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, tex) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
    GLCHK( glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, data) );
    return tex;
}

#endif //GL_UTIL_IMPLEMENTATION
