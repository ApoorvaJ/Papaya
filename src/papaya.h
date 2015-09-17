#pragma once

#define GLEW_STATIC
#include "gl3w.h"
#include "gl3w.c"

#include "papaya_math.h"
#include "papaya_platform.h"
#include "imgui.h"
#include "imgui_draw.cpp"
#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "papaya_util.h"

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

enum PapayaInterfaceTexture_
{
    PapayaInterfaceTexture_Font,
    PapayaInterfaceTexture_TitleBarButtons,
    PapayaInterfaceTexture_TitleBarIcon,
    PapayaInterfaceTexture_InterfaceIcons,
    PapayaInterfaceTexture_COUNT
};

enum PapayaInterfaceColor_
{
    PapayaInterfaceColor_Clear,
    PapayaInterfaceColor_Workspace,
    PapayaInterfaceColor_Transparent,
    PapayaInterfaceColor_Button,
    PapayaInterfaceColor_ButtonHover,
    PapayaInterfaceColor_ButtonActive,
    PapayaInterfaceColor_COUNT
};

enum PapayaVertexBuffer_
{
    PapayaVertexBuffer_ImGui,
    PapayaVertexBuffer_Canvas,
    PapayaVertexBuffer_BrushCursor,
    PapayaVertexBuffer_PickerHStrip,
    PapayaVertexBuffer_PickerSVBox,
    PapayaVertexBuffer_RTTBrush,
    PapayaVertexBuffer_RTTAdd,
    PapayaVertexBuffer_COUNT
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

struct PapayaDocument
{
    int32 Width, Height;
    int32 ComponentsPerPixel;
    uint32 TextureID;
    Vec2 CanvasPosition;
    float CanvasZoom;
    float InverseAspect;
};

struct PapayaShader
{
    uint32 Handle;
    int32 Attributes[8];
    int32 Uniforms[8];
};

struct PapayaVertexBuffer // TODO: Now with indexed rendering, this struct contains the index buffer handle. Is this name appropriate?
{
    size_t VboSize;
    uint32 VboHandle, VaoHandle, ElementsHandle;
};

struct PapayaWindow
{
    uint32 Width, Height;
    uint32 MenuHorizontalOffset, TitleBarButtonsWidth, TitleBarHeight;
};

struct SystemInfo
{
    int32 OpenGLVersion[2];
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

struct ToolParams
{
    // Brush params
    int32 BrushDiameter;
    static const int32 MaxBrushDiameter = 9999;
    float BrushOpacity; // Range: 0.0 - 100.0
    float BrushHardness; // Range: 0.0 - 100.0

    // Right-click info for brush manipulation // TODO: Move some of this stuff to the MouseInfo struct?
    Vec2 RightClickDragStartPos;
    bool RightClickShiftPressed; // Right-click-drag started with shift pressed
    int32 RightClickDragStartDiameter;
    float RightClickDragStartHardness, RightClickDragStartOpacity;
    bool DraggingBrush;

    //
    Color CurrentColor, NewColor;
    bool ColorPickerOpen;
    float NewColorHue, NewColorSaturation, NewColorValue;
    Vec2 PickerPosition, PickerSize, HueStripPosition, HueStripSize, SVBoxPosition, SVBoxSize;
    Vec2 NewColorSV;
    bool DraggingHue, DraggingSV;
};

struct PapayaMemory
{
    bool IsRunning;
    uint32 InterfaceTextureIDs[PapayaInterfaceTexture_COUNT];
    Color InterfaceColors[PapayaInterfaceColor_COUNT];
    PapayaWindow Window;
    SystemInfo System;
    PapayaVertexBuffer VertexBuffers[PapayaVertexBuffer_COUNT];
    PapayaShader Shaders[PapayaShader_COUNT];
    MouseInfo Mouse;
    ToolParams Tools;
    PapayaDocument Document;

    // TODO: Refactor
    uint32 FrameBufferObject;
    uint32 FboRenderTexture, FboSampleTexture;
    bool DrawOverlay;
    bool DrawCanvas;
};
