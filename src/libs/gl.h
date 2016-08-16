
#ifndef GL_H
#define GL_H

#include "types.h"
#include <GL/gl.h>

#if defined(__linux__)
#include <dlfcn.h>
#endif // __linux__

#define GLCHK(stmt) stmt; gl::check_error(#stmt, __FILE__, __LINE__)

enum UniformType_ {
    UniformType_Float,
    UniformType_Vec2,
    UniformType_Matrix4,
    UniformType_Color,
    UniformType_COUNT
};

struct Shader {
    uint32 handle;
    int32 attrib_count, uniform_count;
    int32 attribs[8];
    int32 uniforms[8];
};

struct Mesh {
    bool is_line_loop; // 0 -> Triangle, !0 -> Line
    uint32 vbo_size, index_count;
    uint32 vbo_handle, elements_handle;
};

namespace gl {
    bool init();
    void check_error(char* expr, char* file, int line);
    void compile_shader(Shader& shader, const char* file, int line,
                        const char* vert, const char* frag,
                        int32 attrib_count, int32 uniform_count, ...);
    void set_vertex_attribs(Shader& shader);
    void init_quad(Mesh& mesh, Vec2 pos, Vec2 size, uint32 usage);
    void transform_quad(Mesh& mesh, Vec2 pos, Vec2 size);
    void draw_mesh(Mesh& mesh, Shader& shader, bool scissor,
                   int32 uniform_count, ...);
    uint32 allocate_tex(int32 width, int32 height, uint8* data = 0);
}

#define PAPAYA_GL_LIST \
    /* ret, name, params */ \
	GLE(void,      AttachShader,            GLuint program, GLuint shader) \
    GLE(void,      BindBuffer,              GLenum target, GLuint buffer) \
    GLE(void,      BindFramebuffer,         GLenum target, GLuint framebuffer) \
	GLE(void,      BufferData,              GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) \
	GLE(void,      BufferSubData,           GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data) \
	GLE(GLenum,    CheckFramebufferStatus,  GLenum target) \
	GLE(void,      CompileShader,           GLuint shader) \
	GLE(GLuint,    CreateProgram,           void) \
    GLE(GLuint,    CreateShader,            GLenum type) \
	GLE(void,      DeleteBuffers,           GLsizei n, const GLuint *buffers) \
	GLE(void,      DeleteFramebuffers,      GLsizei n, const GLuint *framebuffers) \
	GLE(void,      EnableVertexAttribArray, GLuint index) \
	GLE(void,      DrawBuffers,             GLsizei n, const GLenum *bufs) \
	GLE(void,      FramebufferTexture2D,    GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) \
	GLE(void,      GenBuffers,              GLsizei n, GLuint *buffers) \
	GLE(void,      GenFramebuffers,         GLsizei n, GLuint * framebuffers) \
	GLE(GLint,     GetAttribLocation,       GLuint program, const GLchar *name) \
    GLE(void,      GetShaderInfoLog,        GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) \
	GLE(void,      GetShaderiv,             GLuint shader, GLenum pname, GLint *params) \
	GLE(GLint,     GetUniformLocation,      GLuint program, const GLchar *name) \
    GLE(void,      LinkProgram,             GLuint program) \
    GLE(void,      ShaderSource,            GLuint shader, GLsizei count, const GLchar* const *string, const GLint *length) \
    GLE(void,      Uniform1i,               GLint location, GLint v0) \
    GLE(void,      Uniform1f,               GLint location, GLfloat v0) \
    GLE(void,      Uniform2f,               GLint location, GLfloat v0, GLfloat v1) \
    GLE(void,      Uniform4f,               GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) \
    GLE(void,      UniformMatrix4fv,        GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
    GLE(void,      UseProgram,              GLuint program) \
    GLE(void,      VertexAttribPointer,     GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer) \
    /* end */

#define GLDECL

#define GLE(ret, name, ...) typedef ret GLDECL name##proc(__VA_ARGS__); extern name##proc * gl##name;
PAPAYA_GL_LIST
#undef GLE

#endif //GL_H

// =============================================================================

#ifdef GL_IMPLEMENTATION

#define GLE(ret, name, ...) name##proc * gl##name;
PAPAYA_GL_LIST
#undef GLE

// Initializes GLEW and performs capability check
// Returns true if current context is capable of OpenGL profile required by Papaya.
// If incapable, returns false and reports which profile/extension is unsupported.
bool gl::init()
{
#if defined(__linux__)

	void* libGL = dlopen("libGL.so", RTLD_LAZY);
	if (!libGL) {
		printf("ERROR: libGL.so couldn't be loaded\n");
		return false;
	}

	#define GLE(ret, name, ...) \
            gl##name = (name##proc *) dlsym(libGL, "gl" #name); \
            if (!gl##name) { \
                printf("Function gl" #name " couldn't be loaded\n"); \
                return false; \
            }
	    PAPAYA_GL_LIST
	#undef GLE

#else // __linux__
	#error "GL loading for this platform is not implemented yet."
#endif

    return true;
}

void gl::check_error(char* expr, char* file, int line)
{
    GLenum error = glGetError();
    const char* str = "";

    if      (error == GL_NO_ERROR) { return; }
    else if (error == GL_INVALID_ENUM) { str = "Invalid enum"; }
    else if (error == GL_INVALID_VALUE) { str = "Invalid value"; }
    else if (error == GL_INVALID_OPERATION) { str = "Invalid operation"; }
    else if (error == GL_INVALID_FRAMEBUFFER_OPERATION) { str = "Invalid framebuffer operation"; }
    else if (error == GL_OUT_OF_MEMORY) { str = "Out of memory"; }
    else if (error == GL_STACK_UNDERFLOW) { str = "Stack underflow"; }
    else if (error == GL_STACK_OVERFLOW) { str = "Stack overflow"; }
    else { str = "Undefined error"; }

    printf("OpenGL Error: %s in %s:%d\n", str, file, line);
    printf("    --- Expression: %s\n", expr);
}

internal void print_compilation_errors(uint32 handle, const char* type, const char* file,
                                       int32 line)
{
    int32 compilation_status;
    GLCHK( glGetShaderiv(handle, GL_COMPILE_STATUS, &compilation_status) );
    if (compilation_status != GL_TRUE) {
        printf("Compilation error in %s shader in %s:%d\n", type, file, line);

        char log[4096];
        int32 out_length;
        GLCHK( glGetShaderInfoLog(handle, 4096, &out_length, log) );
        printf("%s", log);
        printf("\n");
    }
}

void gl::compile_shader(Shader& shader, const char* file, int line,
                        const char* vert, const char* frag,
                        int32 attrib_count, int32 uniform_count, ...)
{
    shader.handle = GLCHK( glCreateProgram() );
    uint32 vert_handle = GLCHK( glCreateShader(GL_VERTEX_SHADER) );
    uint32 frag_handle = GLCHK( glCreateShader(GL_FRAGMENT_SHADER) );

    GLCHK( glShaderSource (vert_handle, 1, &vert, 0) );
    GLCHK( glShaderSource (frag_handle, 1, &frag, 0) );
    GLCHK( glCompileShader(vert_handle) );
    GLCHK( glCompileShader(frag_handle) );
    GLCHK( glAttachShader (shader.handle, vert_handle) );
    GLCHK( glAttachShader (shader.handle, frag_handle) );

    print_compilation_errors(vert_handle, "vertex"  , file, line);
    print_compilation_errors(frag_handle, "fragment", file, line);

    GLCHK( glLinkProgram(shader.handle) ); // TODO: Print linking errors

    shader.attrib_count = attrib_count;
    shader.uniform_count = uniform_count;
    va_list args;
    va_start(args, uniform_count);
    for (int32 i = 0; i < attrib_count; i++) {
        const char* name = va_arg(args, const char*);
        shader.attribs[i] = GLCHK( glGetAttribLocation(shader.handle, name) );

        if (shader.attribs[i] == -1) {
            printf("Attribute %s not found in shader at %s:%d\n", name, file, line);
        }
    }

    for (int32 i = 0; i < uniform_count; i++) {
        const char* name = va_arg(args, const char*);
        shader.uniforms[i] = GLCHK( glGetUniformLocation(shader.handle, name) );

        if (shader.uniforms[i] == -1) {
            printf("Uniform %s not found in shader at %s:%d\n", name, file, line);
        }
    }
    va_end(args);
}

void gl::set_vertex_attribs(Shader& shader)
{
    for (int32 i = 0; i < shader.attrib_count; i++) {
        GLCHK( glEnableVertexAttribArray(shader.attribs[i]) );
    }

    #define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    // Position attribute
    GLCHK( glVertexAttribPointer(shader.attribs[0], 2, GL_FLOAT, GL_FALSE,
                                 sizeof(ImDrawVert),
                                 (GLvoid*)OFFSETOF(ImDrawVert, pos)) );
    // UV attribute
    GLCHK( glVertexAttribPointer(shader.attribs[1], 2, GL_FLOAT, GL_FALSE,
                                 sizeof(ImDrawVert),
                                 (GLvoid*)OFFSETOF(ImDrawVert, uv)) );
    if (shader.attrib_count > 2) {
        // Color attribute 
        GLCHK( glVertexAttribPointer(shader.attribs[2], 4, GL_UNSIGNED_BYTE,
                                     GL_TRUE, sizeof(ImDrawVert),
                                     (GLvoid*)OFFSETOF(ImDrawVert, col)) );
    }
    #undef OFFSETOF
}

void gl::init_quad(Mesh& mesh, Vec2 pos, Vec2 size, uint32 usage)
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

void gl::draw_mesh(Mesh& mesh, Shader& shader, bool scissor,
        int32 uniform_count, ...)
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
    GLCHK( glUseProgram(shader.handle) );

    // Uniforms
    {
        va_list args;
        va_start(args, uniform_count);
        for (int32 i = 0; i < uniform_count; i++) {
            switch (va_arg(args, int)) {
                case UniformType_Float: {
                    GLCHK( glUniform1f(shader.uniforms[i],
                                       (float)va_arg(args, double)) );
                } break;

                case UniformType_Matrix4: {
                    GLCHK( glUniformMatrix4fv(shader.uniforms[i], 1, GL_FALSE,
                                              va_arg(args, float*)) );
                } break;

                case UniformType_Vec2: {
                    Vec2 vec = va_arg(args, Vec2);
                    GLCHK( glUniform2f(shader.uniforms[i], vec.x, vec.y) );
                } break;

                case UniformType_Color: {
                    Color col = va_arg(args, Color);
                    GLCHK( glUniform4f(shader.uniforms[i], col.r, col.g, col.b,
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

uint32 gl::allocate_tex(int32 width, int32 height, uint8* data)
{
    uint32 tex;
    GLCHK( glGenTextures(1, &tex) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, tex) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
    GLCHK( glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                        GL_UNSIGNED_BYTE, data) );
    return tex;
}

#endif //GL_IMPLEMENTATION
