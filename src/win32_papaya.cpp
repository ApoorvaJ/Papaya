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

#include "papaya.h"
#include "papaya.cpp"

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <malloc.h>

// =================================================================================================

global_variable PapayaMemory Memory = {};
global_variable PapayaDebugMemory DebugMemory = {};
global_variable HDC DeviceContext;
global_variable HGLRC RenderingContext;
global_variable int32 OpenGLVersion[2];

global_variable RECT WindowsWorkArea; // Needed because WS_POPUP by default maximizes to cover task bar

// ImGui
global_variable bool bTrue = true;

// =================================================================================================

void Platform::Print(char* Message)
{
	OutputDebugString((LPCSTR)Message);
}

// =================================================================================================

internal void ImGui_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
    if (cmd_lists_count == 0)
        return;

	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    GLint last_program, last_texture;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Setup orthographic projection matrix
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    const float ortho_projection[4][4] =
    {
        { 2.0f/width,	0.0f,			0.0f,		0.0f },
        { 0.0f,			2.0f/-height,	0.0f,		0.0f },
        { 0.0f,			0.0f,			-1.0f,		0.0f },
        { -1.0f,		1.0f,			0.0f,		1.0f },
    };
    glUseProgram(Memory.Shaders[PapayaShader_ImGui].Handle);
    glUniformMatrix4fv(Memory.Shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &ortho_projection[0][0]);
    glUniform1i(Memory.Shaders[PapayaShader_ImGui].Uniforms[1], 0);

    // Grow our buffer according to what we need
    size_t total_vtx_count = 0;
    for (int n = 0; n < cmd_lists_count; n++)
        total_vtx_count += cmd_lists[n]->vtx_buffer.size();
    glBindBuffer(GL_ARRAY_BUFFER, Memory.VertexBuffers[PapayaVertexBuffer_ImGui].VboHandle);
    size_t needed_vtx_size = total_vtx_count * sizeof(ImDrawVert);
    if (Memory.VertexBuffers[PapayaVertexBuffer_ImGui].VboSize < needed_vtx_size)
    {
        Memory.VertexBuffers[PapayaVertexBuffer_ImGui].VboSize = needed_vtx_size + 5000 * sizeof(ImDrawVert);  // Grow buffer
        glBufferData(GL_ARRAY_BUFFER, Memory.VertexBuffers[PapayaVertexBuffer_ImGui].VboSize, NULL, GL_STREAM_DRAW);
    }

    // Copy and convert all vertices into a single contiguous buffer
    unsigned char* buffer_data = (unsigned char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    if (!buffer_data)
        return;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        memcpy(buffer_data, &cmd_list->vtx_buffer[0], cmd_list->vtx_buffer.size() * sizeof(ImDrawVert));
        buffer_data += cmd_list->vtx_buffer.size() * sizeof(ImDrawVert);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(Memory.VertexBuffers[PapayaVertexBuffer_ImGui].VaoHandle);

    int cmd_offset = 0;
    for (int n = 0; n < cmd_lists_count; n++)
    {
        const ImDrawList* cmd_list = cmd_lists[n];
        int vtx_offset = cmd_offset;
        const ImDrawCmd* pcmd_end = cmd_list->commands.end();
        for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)
        {
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
        cmd_offset = vtx_offset;
    }

    // Restore modified state
    glBindVertexArray(0);
    glUseProgram(last_program);
    glDisable(GL_SCISSOR_TEST);
    glBindTexture(GL_TEXTURE_2D, last_texture);
}

internal void ImGui_NewFrame(HWND Window)
{
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
    io.DeltaTime = (float)(current_time - DebugMemory.Time) / DebugMemory.TicksPerSecond;
    DebugMemory.Time = current_time;

    // Hide OS mouse cursor if ImGui is drawing it
    //SetCursor(io.MouseDrawCursor ? NULL : LoadCursor(NULL, IDC_ARROW));

    // Start the frame
    ImGui::NewFrame();
}

internal void ClearAndSwap(void)
{
	if(Memory.InterfaceColors[PapayaInterfaceColor_Clear])
	{
		glClearBufferfv(GL_COLOR, 0, (GLfloat*)&Memory.InterfaceColors[PapayaInterfaceColor_Clear]);
	}
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
		case WM_MBUTTONDOWN:
			io.MouseDown[2] = true;
			return true;
		case WM_MBUTTONUP:
			io.MouseDown[2] = false;
			return true;
		case WM_MOUSEWHEEL:
			io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
			return true;
		case WM_MOUSEMOVE:
		{
			TRACKMOUSEEVENT TrackParam = {};
			TrackParam.dwFlags |= TME_LEAVE;
			TrackParam.hwndTrack = hWnd;
			TrackParam.cbSize = sizeof(TrackParam);
			TrackMouseEvent(&TrackParam);
			io.MousePos.x = (signed short)(lParam);
			io.MousePos.y = (signed short)(lParam >> 16); 
			return true;
		} break;
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
		case WM_MOUSELEAVE:
		{
			ImGui::GetIO().MouseDown[0] = false;
			ImGui::GetIO().MouseDown[1] = false;
			ImGui::GetIO().MouseDown[2] = false;
			return true;
		} break;

		case WM_DESTROY:
		{
			// TODO: Handle this as an error - recreate window?
			Memory.IsRunning = false;
			if (RenderingContext) 
			{
			    wglMakeCurrent(NULL, NULL);
			    wglDeleteContext(RenderingContext);
			}
			ReleaseDC(Window, DeviceContext);
			PostQuitMessage(0);
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
			Memory.IsRunning = false;
		} break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_SIZE:
		{
			if (WParam == SIZE_MAXIMIZED)
			{
				int32 WorkAreaWidth = WindowsWorkArea.right - WindowsWorkArea.left;
				int32 WorkAreaHeight = WindowsWorkArea.bottom - WindowsWorkArea.top;
				SetWindowPos(Window, HWND_TOP, WindowsWorkArea.left, WindowsWorkArea.top, WorkAreaWidth, WorkAreaHeight, NULL);
				Memory.Window.Width = WorkAreaWidth;
				Memory.Window.Height = WorkAreaHeight;
			}
			else
			{
				Memory.Window.Width = (int32) LOWORD(LParam);
				Memory.Window.Height = (int32) HIWORD(LParam);
			}

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

			if (!IsMaximized(Window))
			{
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
			}

			if (Y - WindowRect.top - (IsMaximized(Window) ? Memory.Window.MaximizeOffset : 0.0f) <= (float)Memory.Window.TitleBarHeight && 
				X > WindowRect.left + 200.0f && 
				X < WindowRect.right - (float)(Memory.Window.TitleBarButtonsWidth + 10))
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
	Memory.IsRunning = true;

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
			WS_POPUP | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
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

		SystemParametersInfo(SPI_GETWORKAREA, 0, &WindowsWorkArea, 0);

		uint32 ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
		uint32 ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

		float WindowSize = 0.8f;
		Memory.Window.Width = (uint32)((float)ScreenWidth * WindowSize);
		Memory.Window.Height = (uint32)((float)ScreenHeight * WindowSize);

		uint32 WindowX = (ScreenWidth - Memory.Window.Width) / 2;
		uint32 WindowY = (ScreenHeight - Memory.Window.Height) / 2;

		SetWindowPos(Window, HWND_TOP, WindowX, WindowY, Memory.Window.Width, Memory.Window.Height, NULL);
		//SetWindowPos(Window, HWND_TOP, 0, 0, 1280, 720, NULL);
	}
	#pragma endregion

	#pragma region Initialize OpenGL
	{
		DeviceContext = GetDC(Window);
		PIXELFORMATDESCRIPTOR PixelFormatDescriptor = {};
		int32 PixelFormat;

		#pragma region Setup pixel format
		{
			PixelFormatDescriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
			PixelFormatDescriptor.nVersion = 1;
			PixelFormatDescriptor.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
			PixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
			PixelFormatDescriptor.cColorBits = 32;
			PixelFormatDescriptor.cDepthBits = 32;
			PixelFormatDescriptor.dwLayerMask = PFD_MAIN_PLANE;

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

		#pragma region Create rendering context
		{
			HGLRC TempRenderingContext = wglCreateContext(DeviceContext);
			wglMakeCurrent(DeviceContext, TempRenderingContext);

			if (gl3wInit() != 0)
			{
				// TODO: Log: GL3W Init failed
				exit(1);
			}

			if (!gl3wIsSupported(3,2))
			{
				// TODO: Log: Required OpenGL version not supported
				exit(1);
			}

			RenderingContext = wglCreateContext(DeviceContext); // This creates a context of the latest supported version

			if (!RenderingContext)
			{
				// TODO: Log: Failed to create rendering context
				exit(1);
			}

			wglMakeCurrent(NULL,NULL);
			wglDeleteContext(TempRenderingContext);
			wglMakeCurrent(DeviceContext, RenderingContext);

			glGetIntegerv(GL_MAJOR_VERSION, &OpenGLVersion[0]);
			glGetIntegerv(GL_MINOR_VERSION, &OpenGLVersion[1]);
		}
		#pragma endregion
	}
	#pragma endregion

	Papaya_Initialize(&Memory);

	#pragma region Initialize ImGui
	{
		QueryPerformanceFrequency((LARGE_INTEGER *)&DebugMemory.TicksPerSecond);
		QueryPerformanceCounter((LARGE_INTEGER *)&DebugMemory.Time);

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
	}
	#pragma endregion
	
	Memory.Window.IconWidth = 32;
	Memory.Window.TitleBarButtonsWidth = 109;
	Memory.Window.TitleBarHeight = 30;

	LARGE_INTEGER LastCounter;
	QueryPerformanceCounter(&LastCounter);
	uint64 LastCycleCount = __rdtsc();


	while (Memory.IsRunning)
	{
		#pragma region Windows message handling
		{
			MSG Message;
			while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
			{
				if (Message.message == WM_QUIT)
				{
					Memory.IsRunning = false;
				}

				TranslateMessage(&Message);
				DispatchMessageA(&Message);
			}
		}
		#pragma endregion

		BOOL IsMaximized = IsMaximized(Window);
		// Memory.Window.MaximizeOffset = IsMaximized ? 8.0f : 0.0f; // TODO: Might have to turn this on when activating WS_THICKFRAME for aero snapping to work
		
		ImGui_NewFrame(Window);
		
		Papaya_UpdateAndRender(&Memory, &DebugMemory);

		#pragma region Title Bar Buttons
		{
			ImGui::SetNextWindowSize(ImVec2((float)Memory.Window.TitleBarButtonsWidth,24.0f));
			ImGui::SetNextWindowPos(ImVec2((float)Memory.Window.Width - Memory.Window.TitleBarButtonsWidth - Memory.Window.MaximizeOffset, Memory.Window.MaximizeOffset));
			
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

			ImGui::PushStyleColor(ImGuiCol_Button, Memory.InterfaceColors[PapayaInterfaceColor_Transparent]);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Memory.InterfaceColors[PapayaInterfaceColor_ButtonHover]);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, Memory.InterfaceColors[PapayaInterfaceColor_ButtonActive]);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, Memory.InterfaceColors[PapayaInterfaceColor_Transparent]);

			bool bTrue = true;
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