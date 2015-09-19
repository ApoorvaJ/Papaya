#pragma once

#include "gl3w.h"
#include "gl3w.c"

#include "papaya_math.h"
#include "papaya_platform.h"
#include "imgui.h"
#include "imgui_draw.cpp"
#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "papaya_util.h"

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

enum PapayaTex_
{
    PapayaTex_Font,
    PapayaTex_TitleBarButtons,
    PapayaTex_TitleBarIcon,
    PapayaTex_InterfaceIcons,
    PapayaTex_COUNT
};

enum PapayaCol_
{
    PapayaCol_Clear,
    PapayaCol_Workspace,
    PapayaCol_Transparent,
    PapayaCol_Button,
    PapayaCol_ButtonHover,
    PapayaCol_ButtonActive,
    PapayaCol_COUNT
};

enum PapayaVtxBuf_
{
    PapayaVtxBuf_ImGui,
    PapayaVtxBuf_Canvas,
    PapayaVtxBuf_BrushCursor,
    PapayaVtxBuf_PickerHStrip,
    PapayaVtxBuf_PickerSVBox,
    PapayaVtxBuf_RTTBrush,
    PapayaVtxBuf_RTTAdd,
    PapayaVtxBuf_COUNT
};

enum PapayaShader_
{
    PapayaShader_ImGui,
    PapayaShader_Brush,
    PapayaShader_BrushCursor,
    PapayaShader_PickerHStrip,
    PapayaShader_PickerSVBox,
    PapayaShader_COUNT
};

struct SystemInfo
{
    int32 OpenGLVersion[2];
};

struct WindowInfo
{
    uint32 Width, Height;
    uint32 MenuHorizontalOffset, TitleBarButtonsWidth, TitleBarHeight;
};

struct DocumentInfo
{
    int32 Width, Height;
    int32 ComponentsPerPixel;
    uint32 TextureID;
    Vec2 CanvasPosition;
    float CanvasZoom;
    float InverseAspect;
};

struct MouseInfo
{
    Vec2 Pos;
    Vec2 LastPos;
    Vec2 UV;
    Vec2 LastUV;
    bool IsDown[3];
    bool WasDown[3];
    bool Pressed[3];
    bool Released[3];
    bool InWorkspace;
};

struct ShaderInfo
{
    uint32 Handle;
    int32 Attributes[8];
    int32 Uniforms[8];
};

struct VtxBufInfo // TODO: Now with indexed rendering, this struct contains the index buffer handle. Is this name appropriate?
{
    size_t VboSize;
    uint32 VboHandle, VaoHandle, ElementsHandle;
};

struct BrushInfo
{
    int32 Diameter;
    int32 MaxDiameter;
    float Opacity;  // Range: 0.0 - 100.0
    float Hardness; // Range: 0.0 - 100.0

    // TODO: Move some of this stuff to the MouseInfo struct?
    Vec2 RtDragStartPos;
    bool RtDragWithShift;
    int32 RtDragStartDiameter;
    float RtDragStartHardness, RtDragStartOpacity;
    bool BeingDragged;
};

struct PickerInfo
{
    bool Open;
    Color CurrentColor, NewColor;
    Vec2 CursorSV;
    float CursorH;

    Vec2 Pos, Size, HueStripPos, HueStripSize, SVBoxPos, SVBoxSize;
    bool DraggingHue, DraggingSV;
};

struct MiscInfo // TODO: This entire struct is for stuff to be refactored at some point
{
    uint32 FrameBufferObject;
    uint32 FboRenderTexture, FboSampleTexture;
    bool DrawOverlay;
    bool DrawCanvas;
};

struct PapayaMemory
{
    bool IsRunning;
    SystemInfo System;
    WindowInfo Window;
    DocumentInfo Doc;
    MouseInfo Mouse;

    uint32 Textures[PapayaTex_COUNT];
    Color Colors[PapayaCol_COUNT];
    VtxBufInfo VertexBuffers[PapayaVtxBuf_COUNT];
    ShaderInfo Shaders[PapayaShader_COUNT];
    BrushInfo Brush;
    PickerInfo Picker;
    MiscInfo Misc;
};
