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

#include "imgui.h"
#include "imgui.cpp"

#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/wglew.h>
#include <glew.c>

#include "papaya.h"
#include "papaya.cpp"

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <malloc.h>

// TODO: Put these into own memory buffer
global_variable bool32 GlobalRunning;
global_variable HDC DeviceContext;
global_variable HGLRC RenderingContext;
global_variable HPALETTE Palette;
global_variable uint32 WindowWidth, WindowHeight;

global_variable float Time = 0.0f;

// imgui
global_variable INT64 g_Time = 0;
global_variable INT64 g_TicksPerSecond = 0;
global_variable GLuint g_FontTexture = 0;
global_variable bool bTrue = true;
global_variable bool bFalse = false;

// title bar
const float TitleBarButtonsWidth = 109;
const uint32 TitleBarHeight = 30;

// Colors

#if 1
global_variable ImVec4 ClearColor			= ImVec4(0.1764f, 0.1764f, 0.1882f, 1.0f); // Dark grey
#else
global_variable ImVec4 ClearColor			= ImVec4(0.4470f, 0.5647f, 0.6039f, 1.0f); // Light blue
#endif
global_variable ImVec4 TransparentColor		= ImVec4(0.0f,    0.0f,    0.0f,    0.0f);
global_variable ImVec4 ButtonHoverColor		= ImVec4(0.25f,   0.25f,   0.25f,   1.0f);
global_variable ImVec4 ButtonActiveColor	= ImVec4(0.0f,    0.48f,   0.8f,    1.0f);

internal void ImGui_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
    if (cmd_lists_count == 0)
        return;

    // We are using the OpenGL fixed pipeline to make the example code simpler to read!
    // A probable faster way to render would be to collate all vertices from all cmd_lists into a single vertex buffer.
    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);
    //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context

    // Setup orthographic projection matrix
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

	// TODO: Adjust this if required to try and reduce font blurriness
	float offset = 0.0f;
    glOrtho(0.0f+offset, width+offset, height+offset, 0.0f+offset, -1.0f, +1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Render command lists
    #define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        const unsigned char* vtx_buffer = (const unsigned char*)&cmd_list->vtx_buffer.front();
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, pos)));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, uv)));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, col)));

        int vtx_offset = 0;
        for (size_t cmd_i = 0; cmd_i < cmd_list->commands.size(); cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->commands[cmd_i];
            if (pcmd->user_callback)
            {
                pcmd->user_callback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->texture_id);
                glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
                glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
            }
            vtx_offset += pcmd->vtx_count;
        }
    }
    #undef OFFSETOF

    // Restore modified state
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindTexture(GL_TEXTURE_2D, 0);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

internal void ImGui_NewFrame(HWND Window)
{
     if (!g_FontTexture)
	 {
        ImGuiIO& io = ImGui::GetIO();

		// Build texture
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

		// Create texture
		glGenTextures(1, &g_FontTexture);
		glBindTexture(GL_TEXTURE_2D, g_FontTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);

		// Store our identifier
		io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;

		// Cleanup (don't clear the input data if you want to append new fonts later)
		io.Fonts->ClearInputData();
		io.Fonts->ClearTexData();
	 }

    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
	RECT rect;
    GetClientRect(Window, &rect);
    io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

	// Read keyboard modifiers inputs
    io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;

    // Setup time step
    INT64 current_time;
    QueryPerformanceCounter((LARGE_INTEGER *)&current_time); 
    io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
    g_Time = current_time;

	// Hide OS mouse cursor if ImGui is drawing it
	//SetCursor(io.MouseDrawCursor ? NULL : LoadCursor(NULL, IDC_ARROW));

    // Start the frame
    ImGui::NewFrame();
}

internal void ClearAndSwap(void)
{
	glClearColor(ClearColor.x, ClearColor.y, ClearColor.z, ClearColor.w);
	glClear(GL_COLOR_BUFFER_BIT);
	SwapBuffers(DeviceContext);
}

internal LRESULT ImGui_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImGuiIO& io = ImGui::GetIO();
    switch (msg)
    {
		case WM_LBUTTONDOWN:
			io.MouseDown[0] = true;
			return true;
		case WM_LBUTTONUP:
			io.MouseDown[0] = false; 
			return true;
		case WM_RBUTTONDOWN:
			io.MouseDown[1] = true; 
			return true;
		case WM_RBUTTONUP:
			io.MouseDown[1] = false; 
			return true;
		case WM_MOUSEWHEEL:
			io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
			return true;
		case WM_MOUSEMOVE:
			io.MousePos.x = (signed short)(lParam);
			io.MousePos.y = (signed short)(lParam >> 16); 
			return true;
		case WM_KEYDOWN:
			if (wParam < 256)
				io.KeysDown[wParam] = 1;
			return true;
		case WM_KEYUP:
			if (wParam < 256)
				io.KeysDown[wParam] = 0;
			return true;
		case WM_CHAR:
			// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
			if (wParam > 0 && wParam < 0x10000)
				io.AddInputCharacter((unsigned short)wParam);
			return true;
    }
    return 0;
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	if (ImGui_WndProcHandler(Window, Message, WParam, LParam))
        return true;

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
				//// set viewing projection
				//glMatrixMode(GL_PROJECTION);
				//float FrustumX = 16.0f/9.0f;
				//glFrustum(-0.5f * FrustumX, 0.5f *FrustumX, -0.5F, 0.5F, 1.0F, 3.0F);

				//// position viewer
				//glMatrixMode(GL_MODELVIEW);
				//glTranslatef(0.0F, 0.0F, -2.0F);

				//// position object
				//glRotatef(30.0F, 1.0F, 0.0F, 0.0F);
				//glRotatef(30.0F, 0.0F, 1.0F, 0.0F);

				//glEnable(GL_DEPTH_TEST);
				//glEnable(GL_LIGHTING);
				//glEnable(GL_LIGHT0);
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
			}
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			BeginPaint(Window, &Paint);
			// TODO: Redraw here
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
			WindowWidth = (int32) LOWORD(LParam);
			WindowHeight = (int32) HIWORD(LParam);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();

			float FrustumX = (float)WindowWidth/(float)WindowHeight;
			glFrustum(-0.5f * FrustumX, 0.5f *FrustumX, -0.5F, 0.5F, 1.0F, 3.0F);
			glMatrixMode(GL_MODELVIEW);

			ClearAndSwap();
		} break;

		#pragma region WM_NCHITTEST

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

			if (Y - WindowRect.top - (IsMaximized(Window) ? 8.0f : 0.0f) <= TitleBarHeight && X > WindowRect.left + 200.0f && X < WindowRect.right - (TitleBarButtonsWidth + 10))
			{
				return HTCAPTION;
			}

			SetCursor(LoadCursor(NULL, IDC_ARROW));
			return HTCLIENT;
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
	GlobalRunning = true;

	HWND Window;
	#pragma region Create Window
	{
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

		Window =
			CreateWindowExA(
			0,																							// Extended window style
			WindowClass.lpszClassName,																	// Class name,
			"Papaya",																					// Name,
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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
		WindowWidth = (uint32)((float)ScreenWidth * WindowSize);
		WindowHeight = (uint32)((float)ScreenHeight * WindowSize);

		uint32 WindowX = (ScreenWidth - WindowWidth) / 2;
		uint32 WindowY = (ScreenHeight - WindowHeight) / 2;

		SetWindowPos(Window, HWND_TOP, WindowX, WindowY, WindowWidth, WindowHeight, NULL);
		//SetWindowPos(Window, HWND_TOP, 0, 0, 1280, 720, NULL);
	}
	#pragma endregion

#if PAPAYA_INTERNAL
	LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
	LPVOID BaseAddress = 0;
#endif

	glewInit();

	PapayaMemory Memory = {};
	Papaya_Initialize(&Memory);

	#pragma region Initialize ImGui
	{
		QueryPerformanceFrequency((LARGE_INTEGER *)&g_TicksPerSecond);
		QueryPerformanceCounter((LARGE_INTEGER *)&g_Time);

		ImGuiIO& io = ImGui::GetIO();
		io.KeyMap[ImGuiKey_Tab] = VK_TAB;                              // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
		io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
		io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
		io.KeyMap[ImGuiKey_Home] = VK_HOME;
		io.KeyMap[ImGuiKey_End] = VK_END;
		io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
		io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
		io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
		io.KeyMap[ImGuiKey_A] = 'A';
		io.KeyMap[ImGuiKey_C] = 'C';
		io.KeyMap[ImGuiKey_V] = 'V';
		io.KeyMap[ImGuiKey_X] = 'X';
		io.KeyMap[ImGuiKey_Y] = 'Y';
		io.KeyMap[ImGuiKey_Z] = 'Z';

		io.RenderDrawListsFn = ImGui_RenderDrawLists;
		io.ImeWindowHandle = Window;

		//ImFont* my_font0 = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 20.0f);
		//ImFont* my_font0 = io.Fonts->AddFontFromFileTTF("d:\\DroidSans.ttf", 16.0f);
	}
	#pragma endregion
	
	bool show_test_window = true;
    bool show_another_window = false;

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

		//Redraw();

		ImGui_NewFrame(Window);

		BOOL IsMaximized = IsMaximized(Window);
		float IconWidth = 32.0f;
		
		#pragma region Title Bar Buttons
		{

			ImGui::SetNextWindowSize(ImVec2(TitleBarButtonsWidth,24));
			ImGui::SetNextWindowPos(ImVec2((float)WindowWidth-TitleBarButtonsWidth - (IsMaximized ? 8.0f:0.0f), (IsMaximized ? 8.0f:0.0f)));
			
			ImGuiWindowFlags WindowFlags = 0;
			WindowFlags |= ImGuiWindowFlags_NoTitleBar;
			WindowFlags |= ImGuiWindowFlags_NoResize;
			WindowFlags |= ImGuiWindowFlags_NoMove;
			WindowFlags |= ImGuiWindowFlags_NoScrollbar;
			WindowFlags |= ImGuiWindowFlags_NoCollapse;
			WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0,0));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

			ImGui::PushStyleColor(ImGuiCol_Button, TransparentColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ButtonHoverColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ButtonActiveColor);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));

			ImGui::Begin("Title Bar Buttons", &bTrue, WindowFlags);

			ImGui::PushID(0);
			if(ImGui::ImageButton((void*)Memory.InterfaceTextureIDs[PapayaInterfaceTexture_TitleBarButtons], ImVec2(34,26), ImVec2(0.5,0), ImVec2(1,0.5f), 1, ImVec4(0,0,0,0)))
			{
				ShowWindow(Window, SW_MINIMIZE);
			}

			ImVec2 StartUV = IsMaximized ? ImVec2(0.0f,0.5f) : ImVec2(0.5f,0.5f);
			ImVec2 EndUV   = IsMaximized ? ImVec2(0.5f,1.0f) : ImVec2(1.0f,1.0f);
			
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::PushID(1);

			if(ImGui::ImageButton((void*)Memory.InterfaceTextureIDs[PapayaInterfaceTexture_TitleBarButtons], ImVec2(34,26), StartUV, EndUV, 1, ImVec4(0,0,0,0)))
			{
				if (IsMaximized)
				{
					ShowWindow(Window, SW_RESTORE);
				}
				else
				{
					ShowWindow(Window, SW_MAXIMIZE);
				}
			}

			ImGui::PopID();
			ImGui::SameLine();
			ImGui::PushID(2);

			if(ImGui::ImageButton((void*)Memory.InterfaceTextureIDs[PapayaInterfaceTexture_TitleBarButtons], ImVec2(34,26), ImVec2(0,0), ImVec2(0.5f,0.5f), 1, ImVec4(0,0,0,0)))
			{
				SendMessage(Window, WM_CLOSE, 0, 0);
			}

			ImGui::PopID();

			ImGui::End();

			ImGui::PopStyleVar(5);
			ImGui::PopStyleColor(4);
		}
		#pragma endregion

		#pragma region Title Bar Icon
		{
			ImGui::SetNextWindowSize(ImVec2(IconWidth,(float)TitleBarHeight));
			ImGui::SetNextWindowPos(ImVec2(1.0f + (IsMaximized ? 8.0f:0.0f), 1.0f + (IsMaximized ? 8.0f:0.0f)));

			ImGuiWindowFlags WindowFlags = 0;
			WindowFlags |= ImGuiWindowFlags_NoTitleBar;
			WindowFlags |= ImGuiWindowFlags_NoResize;
			WindowFlags |= ImGuiWindowFlags_NoMove;
			WindowFlags |= ImGuiWindowFlags_NoScrollbar;
			WindowFlags |= ImGuiWindowFlags_NoCollapse;
			WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2,2));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0,0));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

			ImGui::PushStyleColor(ImGuiCol_WindowBg, TransparentColor);

			ImGui::Begin("Title Bar Icon", &bTrue, WindowFlags);
			ImGui::Image((void*)Memory.InterfaceTextureIDs[PapayaInterfaceTexture_TitleBarIcon], ImVec2(28,28));
			ImGui::End();

			ImGui::PopStyleColor(1);
			ImGui::PopStyleVar(5);
		}
		#pragma endregion

		#pragma region Title Bar Menu
		{
			ImGui::SetNextWindowSize(ImVec2(WindowWidth - IconWidth - TitleBarButtonsWidth - 3.0f - (IsMaximized ? 16.0f:0.0f),(float)TitleBarHeight - 10.0f));
			ImGui::SetNextWindowPos(ImVec2(2.0f + IconWidth + (IsMaximized ? 8.0f:0.0f), 1.0f + (IsMaximized ? 8.0f:0.0f) + 5.0f));

			ImGuiWindowFlags WindowFlags = 0;
			WindowFlags |= ImGuiWindowFlags_NoTitleBar;
			WindowFlags |= ImGuiWindowFlags_NoResize;
			WindowFlags |= ImGuiWindowFlags_NoMove;
			WindowFlags |= ImGuiWindowFlags_NoScrollbar;
			WindowFlags |= ImGuiWindowFlags_NoCollapse;
			WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;
			WindowFlags |= ImGuiWindowFlags_MenuBar;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,4));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(5,5));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8,8));

			ImGui::PushStyleColor(ImGuiCol_WindowBg, TransparentColor);
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, TransparentColor);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ButtonHoverColor);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8,4));
			ImGui::PushStyleColor(ImGuiCol_Header, TransparentColor);

			ImGui::Begin("Title Bar Menu", &bTrue, WindowFlags);
			bool foo;
			if (ImGui::BeginMenuBar())
			{
				ImGui::PushStyleColor(ImGuiCol_WindowBg, ClearColor);
				if (ImGui::BeginMenu("FILE"))
				{
					#pragma region File Menu
					{
						if (ImGui::MenuItem("New")) {}
						if (ImGui::MenuItem("Open", "Ctrl+O")) {}
						if (ImGui::BeginMenu("Open Recent"))
						{
							ImGui::MenuItem("fish_hat.c");
							ImGui::MenuItem("fish_hat.inl");
							ImGui::MenuItem("fish_hat.h");
							if (ImGui::BeginMenu("More.."))
							{
								ImGui::MenuItem("Hello");
								ImGui::MenuItem("Sailor");
								if (ImGui::BeginMenu("Recurse.."))
								{
									ShowExampleMenuFile();
									ImGui::EndMenu();
								}
								ImGui::EndMenu();
							}
							ImGui::EndMenu();
						}
						if (ImGui::MenuItem("Save", "Ctrl+S")) {}
						if (ImGui::MenuItem("Save As..")) {}
						ImGui::Separator();
						if (ImGui::BeginMenu("Options"))
						{
							static bool enabled = true;
							ImGui::MenuItem("Enabled", "", &enabled);
							ImGui::BeginChild("child", ImVec2(0, 60), true);
							for (int i = 0; i < 10; i++)
								ImGui::Text("Scrolling Text %d", i);
							ImGui::EndChild();
							static float f = 0.5f;
							ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
							ImGui::InputFloat("Input", &f, 0.1f);
							ImGui::EndMenu();
						}
						if (ImGui::BeginMenu("Colors"))
						{
							for (int i = 0; i < ImGuiCol_COUNT; i++)
								ImGui::MenuItem(ImGui::GetStyleColName((ImGuiCol)i));
							ImGui::EndMenu();
						}
						if (ImGui::BeginMenu("Disabled", false)) // Disabled
						{
							IM_ASSERT(0);
						}
						if (ImGui::MenuItem("Checked", NULL, true)) {}
						if (ImGui::MenuItem("Quit", "Alt+F4")) { GlobalRunning = false; }
					}
					#pragma endregion
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("EDIT"))
				{
					#pragma region Edit Menu
					{
						ImGui::MenuItem("Metrics", NULL, &foo);
						ImGui::MenuItem("Main menu bar", NULL, &foo);
						ImGui::MenuItem("Console", NULL, &foo);
						ImGui::MenuItem("Simple layout", NULL, &foo);
						ImGui::MenuItem("Long text display", NULL, &foo);
						ImGui::MenuItem("Auto-resizing window", NULL, &foo);
						ImGui::MenuItem("Simple overlay", NULL, &foo);
						ImGui::MenuItem("Manipulating window title", NULL, &foo);
						ImGui::MenuItem("Custom rendering", NULL, &foo);
					}
					#pragma endregion
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
				ImGui::PopStyleColor();
			}
			ImGui::End();

			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar(5);
		}
		#pragma endregion

		//=========================================
#if 1
		{
			static float f = 0.0f;
            ImGui::Text("FILE EDIT IMAGE LAYER TYPE SELECT");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&ClearColor);
            if (ImGui::Button("Test Window")) show_test_window ^= 1;
            if (ImGui::Button("Another Window")) show_another_window ^= 1;
			if (ImGui::Button("Maximize")) { ShowWindow(Window, SW_MAXIMIZE); }
			if (ImGui::Button("Restore")) { ShowWindow(Window, SW_RESTORE); }
			if (ImGui::Button("Minimize")) { ShowWindow(Window, SW_MINIMIZE); }
			if (ImGui::Button("Close")) { SendMessage(Window, WM_CLOSE, 0, 0); }
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		}

		// 2. Show another simple window, this time using an explicit Begin/End pair
        if (show_another_window)
        {
            ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }

#endif
        // Rendering
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(ClearColor.x, ClearColor.y, ClearColor.z, ClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);

		Papaya_UpdateAndRender(&Memory);

        ImGui::Render();
        SwapBuffers(DeviceContext);
		//=========================================

		//Sleep(15);

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

	Papaya_Shutdown(&Memory);

	return 0;
}