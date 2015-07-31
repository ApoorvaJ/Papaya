#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#pragma GCC diagnostic ignored "-Wwrite-strings" //TODO: Check if all string warnings can be eliminated without suppression
#include <x86intrin.h>

#include "papaya.h"
#include "papaya.cpp"

#include <stdio.h>

// =================================================================================================

void Platform::Print(char* Message)
{
	//
}

void Platform::StartMouseCapture()
{
	//
}

void Platform::ReleaseMouseCapture()
{
	//
}

void Platform::SetMousePosition(Vec2 Pos)
{
	//
}

void Platform::SetCursorVisibility(bool Visible)
{
	//
}

char* Platform::OpenFileDialog()
{
	return 0;
}

int64 Platform::GetMilliseconds()
{
	return 0;
}

// =================================================================================================

int main(int argc, char **argv)
{
  printf("Hello World\n");
  return 0;
}
