#pragma once

#include "libs/types.h"

#if !defined(_DEBUG ) // TODO: Make this work for gcc
#define PAPAYARELEASE // User-ready release mode
#endif // !_DEBUG


namespace platform
{
    void print(char* Message);
    void start_mouse_capture();
    void stop_mouse_capture();
    void set_mouse_position(int32 x, int32 y);
    void set_cursor_visibility(bool Visible);
    char* open_file_dialog();
    char* save_file_dialog();
    double get_milliseconds();
}
