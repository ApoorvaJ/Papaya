#pragma once

#include "types.h"

#if !defined(_DEBUG ) // TODO: Make this work for gcc
#define PAPAYARELEASE // User-ready release mode
#endif // !_DEBUG


namespace Platform
{
    void Print(char* Message);
    void StartMouseCapture();
    void ReleaseMouseCapture();
    void SetMousePosition(int32 x, int32 y);
    void SetCursorVisibility(bool Visible);
    char* OpenFileDialog();
    char* SaveFileDialog();

    double GetMilliseconds();
}
