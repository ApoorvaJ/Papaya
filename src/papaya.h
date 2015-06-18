#pragma once

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

enum PapayaInterfaceImage_
{
	PapayaInterfaceImage_TitleBarButtons,
	PapayaInterfaceImage_TitleBarIcon,
	PapayaInterfaceImage_COUNT
};

struct PapayaMemory
{
	uint32 TextureIDs[PapayaInterfaceImage_COUNT];
};