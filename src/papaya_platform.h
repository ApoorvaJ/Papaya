#pragma once
#include "platform/timer.h"

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
