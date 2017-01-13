
/*
    Single-header OpenGL helper functions
*/

#ifndef PAGL_H
#define PAGL_H

struct Pagl_Program {
    u32 id;
    const char* name;
    i32 num_attribs, num_uniforms;
    i32* attribs;
    i32* uniforms;
};

void pagl_init(void);
void pagl_destroy(void);
void pagl_push_state(void);

void pagl_pop_state(void);
void pagl_enable(i32 count, ...);
void pagl_disable(i32 count, ...);

u32 pagl_compile_shader(const char* name, const char* src, u32 type);
Pagl_Program* pagl_init_program(const char* name, u32 vert_id, u32 frag_id,
                                i32 num_attribs, i32 num_uniforms, ...);
void pagl_destroy_program(Pagl_Program* p);

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

#endif // PAGL_IMPLEMENTATION
