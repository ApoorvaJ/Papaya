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

global_variable PapayaMemory Memory = {};
global_variable PapayaDebugMemory DebugMemory = {};
global_variable bool32 GlobalRunning;
global_variable HDC DeviceContext;
global_variable HGLRC RenderingContext;
global_variable int32 OpenGLVersion[2];

// ImGui
global_variable bool bTrue = true;
global_variable bool bFalse = false;

// Title bar
const float TitleBarButtonsWidth = 109;
const uint32 TitleBarHeight = 30;

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
    glUseProgram(Memory.DefaultShader.Handle);
    glUniform1i(Memory.DefaultShader.Texture, 0);
    glUniformMatrix4fv(Memory.DefaultShader.ProjectionMatrix, 1, GL_FALSE, &ortho_projection[0][0]);

    // Grow our buffer according to what we need
    size_t total_vtx_count = 0;
    for (int n = 0; n < cmd_lists_count; n++)
        total_vtx_count += cmd_lists[n]->vtx_buffer.size();
    glBindBuffer(GL_ARRAY_BUFFER, Memory.GraphicsBuffers.VboHandle);
    size_t needed_vtx_size = total_vtx_count * sizeof(ImDrawVert);
    if (Memory.GraphicsBuffers.VboSize < needed_vtx_size)
    {
        Memory.GraphicsBuffers.VboSize = needed_vtx_size + 5000 * sizeof(ImDrawVert);  // Grow buffer
        glBufferData(GL_ARRAY_BUFFER, Memory.GraphicsBuffers.VboSize, NULL, GL_STREAM_DRAW);
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
    glBindVertexArray(Memory.GraphicsBuffers.VaoHandle);

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
	glClearColor(Memory.InterfaceColors[PapayaInterfaceColor_Clear].x, 
				 Memory.InterfaceColors[PapayaInterfaceColor_Clear].y, 
				 Memory.InterfaceColors[PapayaInterfaceColor_Clear].z, 
				 Memory.InterfaceColors[PapayaInterfaceColor_Clear].w);
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
		case WM_DESTROY:
		{
			// TODO: Handle this as an error - recreate window?
			GlobalRunning = false;
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
			Memory.Window.Width = (int32) LOWORD(LParam);
			Memory.Window.Height = (int32) HIWORD(LParam);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();

			float FrustumX = (float)Memory.Window.Width/(float)Memory.Window.Height;
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
				32,								// prefered color depth // TODO: What is the optimal value for color depth?
				0, 0, 0, 0, 0, 0,				// color bits (ignored)
				0,								// no alpha buffer
				0,								// alpha bits (ignored)
				0,								// no accumulation buffer
				0, 0, 0, 0,						// accum bits (ignored)
				32,								// depth buffer // TODO: What is the optimal value for depth buffer depth?
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

		#pragma region Create rendering context
		{
			HGLRC TempRenderingContext = wglCreateContext(DeviceContext);
			wglMakeCurrent(DeviceContext, TempRenderingContext);

			GLenum GlewInitResult = glewInit();
			if (GlewInitResult != GLEW_OK)
			{
				// TODO: Log: GLEW Init failed
				exit(1);
			}

			int InitAttributes[] =
			{
				WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
				WGL_CONTEXT_MINOR_VERSION_ARB, 2,
				WGL_CONTEXT_FLAGS_ARB, 0,
				0
			};

			if(wglewIsSupported("WGL_ARB_create_context") == 1)
			{
				RenderingContext = wglCreateContextAttribsARB(DeviceContext, 0, InitAttributes);
				wglMakeCurrent(NULL,NULL);
				wglDeleteContext(TempRenderingContext);
				wglMakeCurrent(DeviceContext, RenderingContext);
			}
			else
			{
				// It is not possible to make a GL 3.x context.
				// TODO: Handle this using a message box or something
				exit(1);
			}

			if (!RenderingContext)
			{
				// TODO: Log: Failed to create rendering context
				exit(1);
			}

			glGetIntegerv(GL_MAJOR_VERSION, &OpenGLVersion[0]);
			glGetIntegerv(GL_MINOR_VERSION, &OpenGLVersion[1]);
		}
		#pragma endregion

		#pragma region Setup for ImGui
		{
			// Create device objects
			const GLchar *vertex_shader = 
			R"(
				#version 330
				uniform mat4 ProjMtx;
				in vec2 Position;
				in vec2 UV;
				in vec4 Color;
				out vec2 Frag_UV;
				out vec4 Frag_Color;
				void main()
				{
					Frag_UV = UV;
					Frag_Color = Color;
					gl_Position = ProjMtx * vec4(Position.xy,0,1);
				}
			)";

			const GLchar* fragment_shader = 
			R"(
				#version 330
				uniform sampler2D Texture;
				in vec2 Frag_UV;
				in vec4 Frag_Color;
				out vec4 Out_Color;
				void main()
				{
					Out_Color = Frag_Color * texture( Texture, Frag_UV.st);
				}
			)";

			Memory.DefaultShader.Handle = glCreateProgram();
			int32 g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
			int32 g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
			glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
			glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
			glCompileShader(g_VertHandle);
			glCompileShader(g_FragHandle);
			glAttachShader(Memory.DefaultShader.Handle, g_VertHandle);
			glAttachShader(Memory.DefaultShader.Handle, g_FragHandle);
			glLinkProgram(Memory.DefaultShader.Handle);

			Memory.DefaultShader.Texture			= glGetUniformLocation(Memory.DefaultShader.Handle, "Texture");
			Memory.DefaultShader.ProjectionMatrix	= glGetUniformLocation(Memory.DefaultShader.Handle, "ProjMtx");
			Memory.DefaultShader.Position			= glGetAttribLocation (Memory.DefaultShader.Handle, "Position");
			Memory.DefaultShader.UV					= glGetAttribLocation (Memory.DefaultShader.Handle, "UV");
			Memory.DefaultShader.Color				= glGetAttribLocation (Memory.DefaultShader.Handle, "Color");

			glGenBuffers(1, &Memory.GraphicsBuffers.VboHandle);

			glGenVertexArrays(1, &Memory.GraphicsBuffers.VaoHandle);
			glBindVertexArray(Memory.GraphicsBuffers.VaoHandle);
			glBindBuffer(GL_ARRAY_BUFFER, Memory.GraphicsBuffers.VboHandle);
			glEnableVertexAttribArray(Memory.DefaultShader.Position);
			glEnableVertexAttribArray(Memory.DefaultShader.UV);
			glEnableVertexAttribArray(Memory.DefaultShader.Color);

		#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
			glVertexAttribPointer(Memory.DefaultShader.Position, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
			glVertexAttribPointer(Memory.DefaultShader.UV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
			glVertexAttribPointer(Memory.DefaultShader.Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
		#undef OFFSETOF
			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			// Create fonts texture
			ImGuiIO& io = ImGui::GetIO();

			unsigned char* pixels;
			int width, height;
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

			glGenTextures(1, &Memory.InterfaceTextureIDs[PapayaInterfaceTexture_Font]);
			glBindTexture(GL_TEXTURE_2D, Memory.InterfaceTextureIDs[PapayaInterfaceTexture_Font]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

			// Store our identifier
			io.Fonts->TexID = (void *)(intptr_t)Memory.InterfaceTextureIDs[PapayaInterfaceTexture_Font];

			// Cleanup (don't clear the input data if you want to append new fonts later)
			io.Fonts->ClearInputData();
			io.Fonts->ClearTexData();
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
		#pragma region Windows message handling
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
		}
		#pragma endregion

		BOOL IsMaximized = IsMaximized(Window);
		Memory.Window.MaximizeOffset = IsMaximized ? 8.0f : 0.0f;
		float IconWidth = 32.0f;
		
		ImGui_NewFrame(Window);
		
		#pragma region Title Bar Buttons
		{
			ImGui::SetNextWindowSize(ImVec2(TitleBarButtonsWidth,24));
			ImGui::SetNextWindowPos(ImVec2((float)Memory.Window.Width-TitleBarButtonsWidth - Memory.Window.MaximizeOffset, Memory.Window.MaximizeOffset));
			
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
			ImGui::SetNextWindowPos(ImVec2(1.0f + Memory.Window.MaximizeOffset, 1.0f + Memory.Window.MaximizeOffset));

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

			ImGui::PushStyleColor(ImGuiCol_WindowBg, Memory.InterfaceColors[PapayaInterfaceColor_Transparent]);

			ImGui::Begin("Title Bar Icon", &bTrue, WindowFlags);
			ImGui::Image((void*)Memory.InterfaceTextureIDs[PapayaInterfaceTexture_TitleBarIcon], ImVec2(28,28));
			ImGui::End();

			ImGui::PopStyleColor(1);
			ImGui::PopStyleVar(5);
		}
		#pragma endregion

		#pragma region Title Bar Menu
		{
			ImGui::SetNextWindowSize(ImVec2(Memory.Window.Width - IconWidth - TitleBarButtonsWidth - 3.0f - (2.0f * Memory.Window.MaximizeOffset),(float)TitleBarHeight - 10.0f));
			ImGui::SetNextWindowPos(ImVec2(2.0f + IconWidth + Memory.Window.MaximizeOffset, 1.0f + Memory.Window.MaximizeOffset + 5.0f));

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

			ImGui::PushStyleColor(ImGuiCol_WindowBg, Memory.InterfaceColors[PapayaInterfaceColor_Transparent]);
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, Memory.InterfaceColors[PapayaInterfaceColor_Transparent]);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Memory.InterfaceColors[PapayaInterfaceColor_ButtonHover]);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8,4));
			ImGui::PushStyleColor(ImGuiCol_Header, Memory.InterfaceColors[PapayaInterfaceColor_Transparent]);

			ImGui::Begin("Title Bar Menu", &bTrue, WindowFlags);
			bool foo;
			if (ImGui::BeginMenuBar())
			{
				ImGui::PushStyleColor(ImGuiCol_WindowBg, Memory.InterfaceColors[PapayaInterfaceColor_Clear]);
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
			static float f = 0.0;
            ImGui::Text("FILE EDIT IMAGE LAYER TYPE SELECT");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&Memory.InterfaceColors[PapayaInterfaceColor_Clear]);
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
        glClearColor(Memory.InterfaceColors[PapayaInterfaceColor_Clear].x, 
					 Memory.InterfaceColors[PapayaInterfaceColor_Clear].y, 
					 Memory.InterfaceColors[PapayaInterfaceColor_Clear].z, 
					 Memory.InterfaceColors[PapayaInterfaceColor_Clear].w);
        glClear(GL_COLOR_BUFFER_BIT);

		Papaya_UpdateAndRender(&Memory, &DebugMemory);

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