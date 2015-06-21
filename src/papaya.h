#pragma once

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
	PapayaInterfaceTexture_COUNT
};

struct PapayaDocument
{
	uint8* Texture;
	int32 Width, Height;
	int32 ComponentsPerPixel;
	uint32 TextureID;
};

struct PapayaShader
{
	int32 Handle;
	int32 Texture, ProjectionMatrix, Position, UV, Color;
};

struct PapayaGraphicsBuffers
{
	size_t VboSize;
	uint32 VboHandle, VaoHandle;
};

struct PapayaMemory
{
	uint32 InterfaceTextureIDs[PapayaInterfaceTexture_COUNT];
	PapayaGraphicsBuffers GraphicsBuffers;
	PapayaShader DefaultShader;
	PapayaDocument* Documents; // TODO: Use an array or vector instead of bare pointer?
};