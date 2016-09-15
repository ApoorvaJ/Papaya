/*
    EasyKey - Single-header multi-platform keyboard library

    ----------------------------------------------------------------------------
    USAGE
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    EXAMPLES
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    CREDITS
    ----------------------------------------------------------------------------
    ----------------------------------------------------------------------------
    LICENSE
    ----------------------------------------------------------------------------
    This software is in the public domain. Where that dedication is not
    recognized, you are granted a perpetual, irrevocable license to copy,
    distribute, and modify this file as you see fit.
*/

// TODO: Create a good enum mapping and key state recording mechanism.

// =============================================================================
// Header section
// =============================================================================

#ifndef EASYKEY_H
#define EASYKEY_H

#include <stdint.h> // TODO: Review which includes are actually needed.
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------------
// Enums
// -----------------------------------------------------------------------------
typedef enum
{
    EASYKEY_OK = 0,

    // Errors
    EASYKEY_MEMORY_ERROR           = -1,
    EASYKEY_X11_ERROR              = -2,

    EASYKEY_EVENT_NOT_HANDLED      = -16,
} EasyKeyResult;

// -----------------------------------------------------------------------------
// Structs
// -----------------------------------------------------------------------------
typedef struct
{
    bool shift;
    bool ctrl;
    bool z;
} EasyKeyInfo;

extern EasyKeyInfo* easykey;

// -----------------------------------------------------------------------------
// Function declarations
// -----------------------------------------------------------------------------
#if defined(__linux__)

    EasyKeyResult easykey_init();
    EasyKeyResult easykey_handle_event(XEvent* event, Display* disp);
    EasyKeyResult easykey_unload();

#elif defined(_WIN32)

    EasyKeyResult easykey_init();
    EasyKeyResult easykey_handle_event();
    EasyKeyResult easykey_unload();

#else

    // Save some trouble when porting.
    #error "Unsupported platform."

#endif // __linux__ _WIN32
// -----------------------------------------------------------------------------

#endif // EASYKEY_H



// =============================================================================
// Implementation section
// =============================================================================

#ifdef EASYKEY_IMPLEMENTATION

#ifdef __linux__
#include <X11/Xutil.h>
#endif // __linux__

EasyKeyInfo* easykey;

// -----------------------------------------------------------------------------
// Linux implementation
// -----------------------------------------------------------------------------
#ifdef __linux__

EasyKeyResult easykey_init()
{
    easykey = (EasyKeyInfo*)malloc(sizeof(EasyKeyInfo));
    if (!easykey) { return EASYKEY_MEMORY_ERROR; }

#ifdef EASYKEY_IMGUI

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab]        = 0;
    io.KeyMap[ImGuiKey_LeftArrow]  = 1;
    io.KeyMap[ImGuiKey_RightArrow] = 2;
    io.KeyMap[ImGuiKey_UpArrow]    = 3;
    io.KeyMap[ImGuiKey_DownArrow]  = 4;
    io.KeyMap[ImGuiKey_PageUp]     = 5;
    io.KeyMap[ImGuiKey_PageDown]   = 6;
    io.KeyMap[ImGuiKey_Home]       = 7;
    io.KeyMap[ImGuiKey_End]        = 8;
    io.KeyMap[ImGuiKey_Delete]     = 9;
    io.KeyMap[ImGuiKey_Backspace]  = 10;
    io.KeyMap[ImGuiKey_Enter]      = 11;
    io.KeyMap[ImGuiKey_Escape]     = 12;
    io.KeyMap[ImGuiKey_A]          = 'a';
    io.KeyMap[ImGuiKey_C]          = 'c';
    io.KeyMap[ImGuiKey_V]          = 'v';
    io.KeyMap[ImGuiKey_X]          = 'x';
    io.KeyMap[ImGuiKey_Y]          = 'y';
    io.KeyMap[ImGuiKey_Z]          = 'z';
        
#endif // EASYKEY_IMGUI

    return EASYKEY_OK;
}

EasyKeyResult easykey_handle_event(XEvent* event, Display* disp)
{
    if (event->type != KeyPress && event->type != KeyRelease) {
        return EASYKEY_EVENT_NOT_HANDLED;
    }

    if (event->type == KeyRelease &&
        XEventsQueued(disp, QueuedAfterReading)) {
        XEvent next_event;
        XPeekEvent(disp, &next_event);
        if (next_event.type == KeyPress &&
            next_event.xkey.time == event->xkey.time &&
            next_event.xkey.keycode == event->xkey.keycode) {
            return EASYKEY_OK;
        }
    }

    // List of keysym's can be found in keysymdef.h or here
    // [http://www.cl.cam.ac.uk/~mgk25/ucs/keysymdef.h]

    // TODO: Put ImGui guards in here
    ImGuiIO& io = ImGui::GetIO();
    bool down = (event->type == KeyPress);

    KeySym keysym;
    XComposeStatus status;
    char input; // TODO: Do we need an array here?
    XLookupString(&event->xkey, &input, 1, &keysym, &status);
    if (down) { io.AddInputCharacter(input); }

    switch (keysym) {
        case XK_Control_L:
        case XK_Control_R: {
            io.KeyCtrl = down;
        } break;
        case XK_Shift_L:
        case XK_Shift_R: {
            io.KeyShift = down;
        } break;
        case XK_Alt_L:
        case XK_Alt_R: { io.KeyAlt = down; } break;

        case XK_Tab:        { io.KeysDown[0]  = down; } break;
        case XK_Left:       { io.KeysDown[1]  = down; } break;
        case XK_Right:      { io.KeysDown[2]  = down; } break;
        case XK_Up:         { io.KeysDown[3]  = down; } break;
        case XK_Down:       { io.KeysDown[4]  = down; } break;
        case XK_Page_Up:    { io.KeysDown[5]  = down; } break;
        case XK_Page_Down:  { io.KeysDown[6]  = down; } break;
        case XK_Home:       { io.KeysDown[7]  = down; } break;
        case XK_End:        { io.KeysDown[8]  = down; } break;
        case XK_Delete:     { io.KeysDown[9]  = down; } break;
        case XK_BackSpace:  { io.KeysDown[10] = down; } break;
        case XK_Return:     { io.KeysDown[11] = down; } break;
        case XK_Escape:     { io.KeysDown[12] = down; } break;

        case XK_a: case XK_A:
        case XK_c: case XK_C:
        case XK_v: case XK_V:
        case XK_x: case XK_X:
        case XK_y: case XK_Y:
        case XK_z: case XK_Z:
            // if uppercase, convert to lowercase
            if (keysym <= 'Z') { keysym += 32; }
            io.KeysDown[keysym] = down;
            break;
    }

    return EASYKEY_OK;
}

EasyKeyResult easykey_unload()
{
    free(easykey);
    return EASYKEY_OK;
}

#endif // __linux__


// -----------------------------------------------------------------------------
// Windows implementation
// -----------------------------------------------------------------------------
#ifdef WIN32

EasyKeyResult easykey_init()
{
    easykey = (EasyKeyInfo*)malloc(sizeof(EasyKeyInfo));
    if (!easykey) { return EASYKEY_MEMORY_ERROR; }


    return EASYKEY_OK;
}

EasyKeyResult easykey_handle_event()
{
    return EASYKEY_OK;
}

EasyKeyResult easykey_unload()
{
    free(easykey);
    return EASYKEY_OK;
}

#endif // WIN32
// -----------------------------------------------------------------------------


#endif // EASYKEY_IMPLEMENTATION
