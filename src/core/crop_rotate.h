#pragma once

#include "libs/types.h"

struct Mouse;
struct PapayaMemory; // TODO: Remove. Manage struct knowledge on a need-to-know-basis.

struct CropRotateInfo
{
    int32 BaseRotation; // Multiply this by 90 to get the rotation in degrees
    float SliderAngle;
    Vec2 TopLeft;
    Vec2 BotRight;

    // This is a bitfield that uses the 4 least significant bits to represent
    // whether each of the 4 vertices are active.
    // 
    // 1 Vertex active   -> Vertex drag
    // 2 Vertices active -> Edge drag
    // 4 Vertices active -> Full rect drag
    uint8 CropMode;
    Vec2 RectDragPosition;
};

namespace CropRotate
{
    void Init(PapayaMemory* Mem);
    void Toolbar(PapayaMemory* Mem);
    void CropOutline(PapayaMemory* Mem);
}

