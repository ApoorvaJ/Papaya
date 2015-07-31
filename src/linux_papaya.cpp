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

#include <X11/Xlib.h>
#include <unistd.h>
#include <stdio.h>

// =================================================================================================

void Platform::Print(char* Message)
{
	printf("%s", Message);
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
  Display* Display = XOpenDisplay(0);
  if (!Display)
  {
    //TODO: Log: Error opening connection to the X server
    return 1;
  }

  int32 BlackColor = BlackPixel(Display, DefaultScreen(Display));
  int32 WhiteColor = WhitePixel(Display, DefaultScreen(Display));

  Window Window = XCreateSimpleWindow(Display, DefaultRootWindow(Display), 0, 0, 200, 100, 0, BlackColor, BlackColor);

  XSelectInput(Display, Window, StructureNotifyMask);
  XMapWindow(Display, Window);
  GC GraphicsContext = XCreateGC(Display, Window, 0, 0);
  XSetForeground(Display, GraphicsContext, WhiteColor);
  for(;;)
  {
    XEvent Event;
    XNextEvent(Display, &Event);
    if (Event.type == MapNotify) { break; }
  }

  XDrawLine(Display, Window, GraphicsContext, 10, 60, 180, 20);

  XFlush(Display);
  sleep(10);

  return 0;
}
