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

void DrawAQuad()
{
	glClearColor(0.0, 0.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int main(int argc, char **argv)
{
	PapayaMemory Memory = {0};
	PapayaDebugMemory DebugMemory = {0};

	Display* Display;
	Window Window;
	XVisualInfo* VisualInfo;
	Atom WmDeleteMessage;

	// Create window
	{
	 	Display = XOpenDisplay(0);
	 	if (!Display)
	 	{
			// TODO: Log: Error opening connection to the X server
			exit(1);
	 	}

		int32 Attributes[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	 	VisualInfo = glXChooseVisual(Display, 0, Attributes);
		if(VisualInfo == NULL)
		{
			// TODO: Log: No appropriate visual found
			exit(1);
		}

		XSetWindowAttributes SetWindowAttributes = {0};
		SetWindowAttributes.colormap = XCreateColormap(Display, DefaultRootWindow(Display), VisualInfo->visual, AllocNone);
		SetWindowAttributes.event_mask = ExposureMask | KeyPressMask;

		Window = XCreateWindow(Display, DefaultRootWindow(Display), 0, 0, 600, 600, 0, VisualInfo->depth, InputOutput, VisualInfo->visual, CWColormap | CWEventMask, &SetWindowAttributes);
		XMapWindow(Display, Window);
	 	XStoreName(Display, Window, "Papaya");

		WmDeleteMessage = XInternAtom(Display, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(Display, Window, &WmDeleteMessage, 1);
	}

	// Create OpenGL context
	{
		GLXContext GraphicsContext = glXCreateContext(Display, VisualInfo, 0, GL_TRUE);
		glXMakeCurrent(Display, Window, GraphicsContext);
		if (gl3wInit() != 0)
		{
			// TODO: Log: GL3W Init failed
			Platform::Print("Gl3w init failed.\n");
			exit(1);
		}

		if (!gl3wIsSupported(3,1))
		{
			// TODO: Log: Required OpenGL version not supported
			Platform::Print("Required OpenGL version not supported.\n");
			exit(1);
		}

		glGetIntegerv(GL_MAJOR_VERSION, &Memory.System.OpenGLVersion[0]);
		glGetIntegerv(GL_MINOR_VERSION, &Memory.System.OpenGLVersion[1]);
		printf("%d, %d\n", Memory.System.OpenGLVersion[0], Memory.System.OpenGLVersion[1]);
	}

	Papaya::Initialize(&Memory);

	// Initialize ImGui
	{
		// TODO: Profiler timer setup

		ImGuiIO& io = ImGui::GetIO();
		// TODO: Keyboard mappings

		io.RenderDrawListsFn = Papaya::RenderImGui;
	}

	Memory.IsRunning = true;

	while (Memory.IsRunning)
	{
		XEvent Event;
		XNextEvent(Display, &Event);
		switch (Event.type)
		{
			case Expose:
			{
				// Start new ImGui frame
				{
					ImGuiIO& io = ImGui::GetIO();

					XWindowAttributes WindowAttributes;
					XGetWindowAttributes(Display, Window, &WindowAttributes);
					io.DisplaySize = ImVec2((float)WindowAttributes.width, (float)WindowAttributes.height);
					ImGui::NewFrame();
				}

				Papaya::UpdateAndRender(&Memory, &DebugMemory);

				ImGui::Render(&Memory);
                glXSwapBuffers(Display, Window);
			} break;

			case ClientMessage:
			{
				if (Event.xclient.data.l[0] == WmDeleteMessage) { Memory.IsRunning = false; }
			} break;
		}
	}

	return 0;
/*

	Atom WmDeleteMessage = XInternAtom(Display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(Display, Window, &WmDeleteMessage, 1);

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

	Memory.IsRunning = true;

	while (Memory.IsRunning)
	{
		XEvent Event;
		XNextEvent(Display, &Event);
		switch (Event.type)
		{
			case Expose:
			{
				//
			} break;

			case ClientMessage:
			{
				if (Event.xclient.data.l[0] == WmDeleteMessage) { Memory.IsRunning = false; }
			} break;
		}
	}

	return 0;*/
}
