#include <stdint.h>

#define internal static 
#define local_persist static 
#define global_variable static

#define Pi32 3.14159265359f

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

#include "papaya.h"
#include "papaya.cpp"

#include <windows.h>
#include <windowsx.h>
#include <gl/GL.h>
#include <stdio.h>
#include <malloc.h>


#include "win32_papaya.h"

global_variable bool32 GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable HDC DeviceContext;
global_variable HGLRC RenderingContext;
global_variable HPALETTE Palette;

global_variable float Time = 0.0f;

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return(Result);
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	// TODO: Bulletproof this.
	// Maybe don't free first, free after, then free first if that fails.

	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;

	int BytesPerPixel = 4;

	// When the biHeight field is negative, this is the clue to
	// Windows to treat this bitmap as top-down, not bottom-up, meaning that
	// the first three bytes of the image are the color for the top left pixel
	// in the bitmap, not the bottom left!
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = Width*BytesPerPixel;

	// TODO: Probably clear this to black
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
	// TODO: Aspect ratio correction
	// TODO: Play with stretch modes
	StretchDIBits(DeviceContext,
		/*
		X, Y, Width, Height,
		X, Y, Width, Height,
		*/
		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory,
		&Buffer->Info,
		DIB_RGB_COLORS, SRCCOPY);
}

void Redraw(void)
{
	// Clear color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw six faces of a cube
	Time += 0.05f;
	float Size = sin(Time) * 0.2f + 0.8f;
	float HalfSize = Size * 0.5f;

    glBegin(GL_QUADS);
    glNormal3f( 0.0F, 0.0F, Size);
    glVertex3f( HalfSize, HalfSize, HalfSize); glVertex3f(-HalfSize, HalfSize, HalfSize);
    glVertex3f(-HalfSize,-HalfSize, HalfSize); glVertex3f( HalfSize,-HalfSize, HalfSize);

    glNormal3f( 0.0F, 0.0F,-Size);
    glVertex3f(-HalfSize,-HalfSize,-HalfSize); glVertex3f(-HalfSize, HalfSize,-HalfSize);
    glVertex3f( HalfSize, HalfSize,-HalfSize); glVertex3f( HalfSize,-HalfSize,-HalfSize);

    glNormal3f( 0.0F, Size, 0.0F);
    glVertex3f( HalfSize, HalfSize, HalfSize); glVertex3f( HalfSize, HalfSize,-HalfSize);
    glVertex3f(-HalfSize, HalfSize,-HalfSize); glVertex3f(-HalfSize, HalfSize, HalfSize);

    glNormal3f( 0.0F,-Size, 0.0F);
    glVertex3f(-HalfSize,-HalfSize,-HalfSize); glVertex3f( HalfSize,-HalfSize,-HalfSize);
    glVertex3f( HalfSize,-HalfSize, HalfSize); glVertex3f(-HalfSize,-HalfSize, HalfSize);

    glNormal3f( Size, 0.0F, 0.0F);
    glVertex3f( HalfSize, HalfSize, HalfSize); glVertex3f( HalfSize,-HalfSize, HalfSize);
    glVertex3f( HalfSize,-HalfSize,-HalfSize); glVertex3f( HalfSize, HalfSize,-HalfSize);

    glNormal3f(-Size, 0.0F, 0.0F);
    glVertex3f(-HalfSize,-HalfSize,-HalfSize); glVertex3f(-HalfSize,-HalfSize, HalfSize);
    glVertex3f(-HalfSize, HalfSize, HalfSize); glVertex3f(-HalfSize, HalfSize,-HalfSize);
    glEnd();

    SwapBuffers(DeviceContext);
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message)
	{
		case WM_CREATE:
		{
			DeviceContext = GetDC(Window);
			PIXELFORMATDESCRIPTOR PixelFormatDescriptor;
			int32 PixelFormat;

			#pragma region Setup pixel format
			{
				PixelFormatDescriptor = 
				{
					sizeof(PIXELFORMATDESCRIPTOR),	// size
					1,								// version
					PFD_SUPPORT_OPENGL |			
					PFD_DRAW_TO_WINDOW |			
					PFD_DOUBLEBUFFER,				// support double-buffering
					PFD_TYPE_RGBA,					// color type
					16,								// prefered color depth
					0, 0, 0, 0, 0, 0,				// color bits (ignored)
					0,								// no alpha buffer
					0,								// alpha bits (ignored)
					0,								// no accumulation buffer
					0, 0, 0, 0,						// accum bits (ignored)
					16,								// depth buffer
					0,								// no stencil buffer
					0,								// no auxiliary buffers
					PFD_MAIN_PLANE,					// main layer
					0,								// reserved
					0, 0, 0,						// no layer, visible, damage masks
				};

				PixelFormat = ChoosePixelFormat(DeviceContext, &PixelFormatDescriptor);
				if (!PixelFormat)
				{
					// TODO: Log: Choose pixel format failed
					exit(1);
				}

				if (!SetPixelFormat(DeviceContext, PixelFormat, &PixelFormatDescriptor)) 
				{
				    // TODO: Log: Set pixel format failed
				    exit(1);
				}
			}
			#pragma endregion

			#pragma region Setup palette
			{
				if (PixelFormatDescriptor.dwFlags & PFD_NEED_PALETTE) 
				{
					int32 PaletteSize = 1 << PixelFormatDescriptor.cColorBits;

					LOGPALETTE* LogicalPalette = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) + PaletteSize * sizeof(PALETTEENTRY)); // TODO: Switch to game memory?
					LogicalPalette->palVersion = 0x300;
					LogicalPalette->palNumEntries = PaletteSize;

					// Build a simple RGB color palette
					{
						int32 RedMask =		(1 << PixelFormatDescriptor.cRedBits)	- 1;
						int32 GreenMask =	(1 << PixelFormatDescriptor.cGreenBits) - 1;
						int32 BlueMask =	(1 << PixelFormatDescriptor.cBlueBits)	- 1;

						for (int32 i=0; i < PaletteSize; i++) 
						{
							LogicalPalette->palPalEntry[i].peRed	= (((i >> PixelFormatDescriptor.cRedShift) & RedMask)		* 255) / RedMask;
							LogicalPalette->palPalEntry[i].peGreen	= (((i >> PixelFormatDescriptor.cGreenShift) & GreenMask)	* 255) / GreenMask;
							LogicalPalette->palPalEntry[i].peBlue	= (((i >> PixelFormatDescriptor.cBlueShift) & BlueMask)		* 255) / BlueMask;
							LogicalPalette->palPalEntry[i].peFlags	= 0;
						}
					}

					Palette = CreatePalette(LogicalPalette);
					free(LogicalPalette); // TODO: Adapt for game memory?

					if (Palette) 
					{
						SelectPalette(DeviceContext, Palette, FALSE);
						RealizePalette(DeviceContext);
					}
				}
			}
			#pragma endregion

			RenderingContext = wglCreateContext(DeviceContext);
			wglMakeCurrent(DeviceContext, RenderingContext);

			#pragma region GL Initialization
			{
				// set viewing projection
				glMatrixMode(GL_PROJECTION);
				float FrustumX = 16.0f/9.0f;
				glFrustum(-0.5f * FrustumX, 0.5f *FrustumX, -0.5F, 0.5F, 1.0F, 3.0F);

				// position viewer
				glMatrixMode(GL_MODELVIEW);
				glTranslatef(0.0F, 0.0F, -2.0F);

				// position object
				glRotatef(30.0F, 1.0F, 0.0F, 0.0F);
				glRotatef(30.0F, 0.0F, 1.0F, 0.0F);

				glEnable(GL_DEPTH_TEST);
				glEnable(GL_LIGHTING);
				glEnable(GL_LIGHT0);
			}
			#pragma endregion
		} break;

		case WM_DESTROY:
		{
			// TODO: Handle this as an error - recreate window?
			GlobalRunning = false;
			if (RenderingContext) 
			{
			    wglMakeCurrent(NULL, NULL);
			    wglDeleteContext(RenderingContext);
			}
			if (Palette) 
			{
			    DeleteObject(Palette);
			}
			ReleaseDC(Window, DeviceContext);
			PostQuitMessage(0);
		} break;

		case WM_PALETTECHANGED:
		{
			// Realize palette if this is *not* the current window
			if (RenderingContext && Palette && (HWND) WParam != Window) 
			{
				UnrealizeObject(Palette);
				SelectPalette(DeviceContext, Palette, FALSE);
				RealizePalette(DeviceContext);
				Redraw();
			}
		} break;
		case WM_QUERYNEWPALETTE:
		{
			// Realize palette if this is the current window
			if (RenderingContext && Palette) 
			{
				UnrealizeObject(Palette);
				SelectPalette(DeviceContext, Palette, FALSE);
				RealizePalette(DeviceContext);
				Redraw();
			}
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			BeginPaint(Window, &Paint);
			// win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			// Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
			if (RenderingContext)
			{
				Redraw();
			}
			EndPaint(Window, &Paint);
		} break;

		case WM_CLOSE:
		{
			// TODO: Handle this with a message to the user?
			GlobalRunning = false;
		} break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_NCCALCSIZE:
		{
			//this kills the window frame and title bar we added with
			//WS_THICKFRAME and WS_CAPTION
			return 0;
		} break;

		case WM_SIZE:
		{
			/*uint32 NewWidth = LParam & 0xFFFF;
			uint32 NewHeight = LParam >> 16;*/
			uint32 NewWidth = (int32) LOWORD(LParam);
			uint32 NewHeight = (int32) HIWORD(LParam);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			float FrustumX = (float)NewWidth/(float)NewHeight;
			glFrustum(-0.5f * FrustumX, 0.5f *FrustumX, -0.5F, 0.5F, 1.0F, 3.0F);
			glMatrixMode(GL_MODELVIEW);
			//Win32ResizeDIBSection(&GlobalBackbuffer, NewWidth, NewHeight);
			
			if (RenderingContext)
			{
				glViewport(0, 0, NewWidth, NewHeight);
			}
			
			//
			//Result = DefWindowProcA(Window, Message, WParam, LParam);

		} break;

		#pragma region WM_CHITTEST

		case WM_NCHITTEST:
		{
			const LONG BorderWidth = 8; //in pixels
			RECT WindowRect;
			GetWindowRect(Window, &WindowRect);
			long X = GET_X_LPARAM(LParam);
			long Y = GET_Y_LPARAM(LParam);

			//bottom left corner
			if (X >= WindowRect.left && X < WindowRect.left + BorderWidth &&
				Y < WindowRect.bottom && Y >= WindowRect.bottom - BorderWidth)
			{
				return HTBOTTOMLEFT;
			}
			//bottom right corner
			if (X < WindowRect.right && X >= WindowRect.right - BorderWidth &&
				Y < WindowRect.bottom && Y >= WindowRect.bottom - BorderWidth)
			{
				return HTBOTTOMRIGHT;
			}
			//top left corner
			if (X >= WindowRect.left && X < WindowRect.left + BorderWidth &&
				Y >= WindowRect.top && Y < WindowRect.top + BorderWidth)
			{
				return HTTOPLEFT;
			}
			//top right corner
			if (X < WindowRect.right && X >= WindowRect.right - BorderWidth &&
				Y >= WindowRect.top && Y < WindowRect.top + BorderWidth)
			{
				return HTTOPRIGHT;
			}
			//left border
			if (X >= WindowRect.left && X < WindowRect.left + BorderWidth)
			{
				return HTLEFT;
			}
			//right border
			if (X < WindowRect.right && X >= WindowRect.right - BorderWidth)
			{
				return HTRIGHT;
			}
			//bottom border
			if (Y < WindowRect.bottom && Y >= WindowRect.bottom - BorderWidth)
			{
				return HTBOTTOM;
			}
			//top border
			if (Y >= WindowRect.top && Y < WindowRect.top + BorderWidth)
			{
				return HTTOP;
			}

			const uint32 TitleBarHeight = 30;
			if (Y - WindowRect.top <= TitleBarHeight)
			{
				return HTCAPTION;
			}
		} break;

		#pragma endregion

		#pragma region Input

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32 VKCode = WParam;
			bool32 WasDown = ((LParam & (1 << 30)) != 0);
			bool32 IsDown = ((LParam & (1 << 31)) == 0);
			if (WasDown != IsDown)
			{
				if (VKCode == 'W')
				{
				}
				else if (VKCode == 'A')
				{
				}
				else if (VKCode == 'S')
				{
				}
				else if (VKCode == 'D')
				{
				}
				else if (VKCode == 'Q')
				{
				}
				else if (VKCode == 'E')
				{
				}
				else if (VKCode == VK_UP)
				{
				}
				else if (VKCode == VK_LEFT)
				{
				}
				else if (VKCode == VK_DOWN)
				{
				}
				else if (VKCode == VK_RIGHT)
				{
				}
				else if (VKCode == VK_ESCAPE)
				{
					/*OutputDebugStringA("ESCAPE: ");
					if (IsDown)
					{
						OutputDebugStringA("IsDown ");
					}
					if (WasDown)
					{
						OutputDebugStringA("WasDown");
					}
					OutputDebugStringA("\n");*/
					GlobalRunning = false;
				}
				else if (VKCode == VK_SPACE)
				{
				}
			}

			bool32 AltKeyWasDown = (LParam & (1 << 29));
			if ((VKCode == VK_F4) && AltKeyWasDown)
			{
				GlobalRunning = false;
			}
		} break;

		#pragma endregion

		default:
		{
			Result = DefWindowProcA(Window, Message, WParam, LParam);
		} break;
	}

	return(Result);
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WNDCLASSA WindowClass = {};
	WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	// TODO: Add an icon
	// WindowClass.hIcon;
	WindowClass.lpszClassName = "PapayaWindowClass";

	if (!RegisterClassA(&WindowClass))
	{
		// TODO: Log: Register window class failed
		return 0;
	}

	HWND Window =
		CreateWindowExA(
		0,																							// Extended window style
		WindowClass.lpszClassName,																	// Class name,
		"Papaya",																					// Name,
		WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE,		// Style,
		CW_USEDEFAULT,																				// X,
		CW_USEDEFAULT,																				// Y,
		CW_USEDEFAULT,																				// Width,
		CW_USEDEFAULT,																				// Height,
		0,																							// Window Parent,
		0,																							// Menu,
		Instance,																					// Handle to the instance,
		0);																							// lpParam

	if (!Window)
	{
		// TODO: Log: Create window failed
		return 0;
	}

	uint32 ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	uint32 ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	float WindowSize = 0.8f;
	uint32 WindowWidth = (uint32)((float)ScreenWidth * WindowSize);
	uint32 WindowHeight = (uint32)((float)ScreenHeight * WindowSize);

	uint32 WindowX = (ScreenWidth - WindowWidth) / 2;
	uint32 WindowY = (ScreenHeight - WindowHeight) / 2;

	//SetWindowPos(Window, HWND_TOP, WindowX, WindowY, WindowWidth, WindowHeight, NULL);
	SetWindowPos(Window, HWND_TOP, 0, 0, 1280, 720, NULL);

	GlobalRunning = true;

#if PAPAYA_INTERNAL
	LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
	LPVOID BaseAddress = 0;
#endif

	game_memory GameMemory = {};
	GameMemory.PermanentStorageSize = Megabytes(64);
	GameMemory.TransientStorageSize = Megabytes(512);

	// TODO: Handle various memory footprints (USING SYSTEM METRICS)
	uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
	GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize,
		MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage +
		GameMemory.PermanentStorageSize);

	if (!GameMemory.PermanentStorage || !GameMemory.TransientStorage)
	{
		// TODO: Log: Initial memory allocation failed
		return 0;
	}

	LARGE_INTEGER LastCounter;
	QueryPerformanceCounter(&LastCounter);
	uint64 LastCycleCount = __rdtsc();
	while (GlobalRunning)
	{
		MSG Message;

		while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
		{
			if (Message.message == WM_QUIT)
			{
				GlobalRunning = false;
			}

			TranslateMessage(&Message);
			DispatchMessageA(&Message);
		}

		game_offscreen_buffer Buffer = {};
		Buffer.Memory = GlobalBackbuffer.Memory;
		Buffer.Width = GlobalBackbuffer.Width;
		Buffer.Height = GlobalBackbuffer.Height;
		Buffer.Pitch = GlobalBackbuffer.Pitch;
		//GameUpdateAndRender(&GameMemory, &Buffer);

		/*win32_window_dimension Dimension = Win32GetWindowDimension(Window);
		Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext,
			Dimension.Width, Dimension.Height);*/
		Redraw();
		uint64 EndCycleCount = __rdtsc();

		LARGE_INTEGER EndCounter;
		QueryPerformanceCounter(&EndCounter);

		uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
		int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
		real64 MSPerFrame = (((1000.0f*(real64)CounterElapsed) / (real64)PerfCountFrequency));
		real64 FPS = (real64)PerfCountFrequency / (real64)CounterElapsed;
		real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));

#if 0
		char Buffer[256];
		sprintf(Buffer, "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
		OutputDebugStringA(Buffer);
#endif

		LastCounter = EndCounter;
		LastCycleCount = EndCycleCount;
	}

	return 0;
}
