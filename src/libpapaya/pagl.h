
/*
    Single-header OpenGL helper functions
*/

#ifndef PAGL_H
#define PAGL_H

void pagl_init(void);
void pagl_destroy(void);
void pagl_push_state(void);

void pagl_pop_state(void);
void pagl_enable(i32 count, ...);
void pagl_disable(i32 count, ...);

#endif // PAGL_H


// =============================================================================


#ifdef PAGL_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

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
    stack_size = 16;
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

#endif // PAGL_IMPLEMENTATION
