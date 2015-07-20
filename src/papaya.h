#pragma once

#define GLEW_STATIC
#include <gl3w.h>
#include <gl3w.c>

#include "papaya_math.h"
#include "papaya_platform.h"
#include "imgui.h"
#include "imgui.cpp"
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
	PapayaVertexBuffer_RTTBrush,
	PapayaVertexBuffer_BrushCursor,
	PapayaVertexBuffer_RTTAdd,
	PapayaVertexBuffer_COUNT
};

enum PapayaShader_
{
	PapayaShader_ImGui,
	PapayaShader_Brush,
	PapayaShader_BrushCursor,
	PapayaShader_COUNT
};

struct PapayaDocument
{
	uint8* Texture;
	int32 Width, Height;
	int32 ComponentsPerPixel;
	uint32 TextureID;
	Vec2 CanvasPosition;
	float CanvasZoom;
};

struct PapayaShader
{
	uint32 Handle;
	int32 Attributes[8];
	int32 Uniforms[8];
};

struct PapayaVertexBuffer
{
	size_t VboSize;
	uint32 VboHandle, VaoHandle;
};

struct PapayaWindow
{
	uint32 Width, Height;
	float MaximizeOffset; // Used to cancel Windows' weirdness when windows are maximized. Refer: http://stackoverflow.com/questions/14667558/why-is-a-maximized-delphi-form-8-pixels-wider-and-higher-than-the-getsystemmetri
	uint32 IconWidth, TitleBarButtonsWidth, TitleBarHeight;
};

struct MouseInfo
{
	Vec2 Pos;
	Vec2 LastPos;
	Vec2 UV;
	Vec2 LastUV;
	bool IsDown[3];
	bool WasDown[3];
};

struct ToolParams
{
	int32 BrushDiameter;
	const int32 MaxBrushDiameter = 9999;
	float BrushOpacity; // Range: 0.0 - 100.0
	float BrushHardness; // Range: 0.0 - 100.0
	Vec2 RightClickDragStartPos;
	int32 RightClickDragStartDiameter;
};

struct PapayaMemory
{
	bool IsRunning;
	uint32 InterfaceTextureIDs[PapayaInterfaceTexture_COUNT];
	Color InterfaceColors[PapayaInterfaceColor_COUNT];
	uint32 CurrentColor;
	PapayaWindow Window;
	PapayaVertexBuffer VertexBuffers[PapayaVertexBuffer_COUNT];
	PapayaShader Shaders[PapayaShader_COUNT];
	MouseInfo Mouse;
	ToolParams Tools;
	PapayaDocument* Documents; // TODO: Use an array or vector instead of bare pointer?

	// TODO: Refactor
	uint32 FrameBufferObject;
	uint32 FboRenderTexture, FboSampleTexture;
	bool DrawOverlay;
	bool DrawCanvas;
};