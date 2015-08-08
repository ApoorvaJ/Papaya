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

#define PAPAYA_DEFAULT_IMAGE "C:\\Users\\Apoorva\\Pictures\\ImageTest\\test4k.jpg"
#include "papaya.h"
#include "papaya.cpp"

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <malloc.h>
#include <commdlg.h>

// =================================================================================================

global_variable PapayaMemory Memory = {};
global_variable PapayaDebugMemory DebugMemory = {};
global_variable HDC DeviceContext;
global_variable HGLRC RenderingContext;
global_variable int32 OpenGLVersion[2];

global_variable RECT WindowsWorkArea; // Needed because WS_POPUP by default maximizes to cover task bar


// =================================================================================================


void Platform::Print(char* Message)
{
    OutputDebugString((LPCSTR)Message);
}

void Platform::StartMouseCapture()
{
    SetCapture(GetActiveWindow());
}

void Platform::ReleaseMouseCapture()
{
    ReleaseCapture();
}

void Platform::SetMousePosition(Vec2 Pos)
{
    RECT Rect;
    GetWindowRect(GetActiveWindow(), &Rect);
    SetCursorPos(Rect.left + (int32)Pos.x, Rect.top + (int32)Pos.y);
}

void Platform::SetCursorVisibility(bool Visible)
{
    ShowCursor(Visible);
}

char* Platform::OpenFileDialog()
{
    const int32 FileNameSize = 400; // TODO: What's a good value for this?
    char* FileName = (char*)malloc(FileNameSize);

    OPENFILENAME DialogParams   = {};
    DialogParams.lStructSize    = sizeof(OPENFILENAME);
    DialogParams.hwndOwner      = GetActiveWindow();
    DialogParams.lpstrFilter    = "JPEG\0*.jpg;*.jpeg\0PNG\0*.png\0";
    DialogParams.nFilterIndex   = 2;
    DialogParams.lpstrFile      = FileName;
    DialogParams.lpstrFile[0]   = '\0';
    DialogParams.nMaxFile       = FileNameSize;
    DialogParams.Flags          = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    BOOL Result = GetOpenFileNameA(&DialogParams); // TODO: Unicode support?

    if (Result)
    {
        return FileName;
    }
    else
    {
        free(FileName);
        return 0;
    }
}

char* Platform::SaveFileDialog()
{
    const int32 FileNameSize = 400; // TODO: What's a good value for this?
    char* FileName = (char*)malloc(FileNameSize);

    OPENFILENAME DialogParams = {};
    DialogParams.lStructSize = sizeof(OPENFILENAME);
    DialogParams.hwndOwner = GetActiveWindow();
    DialogParams.lpstrFilter = "PNG\0*.png\0";
    DialogParams.nFilterIndex = 2;
    DialogParams.lpstrFile = FileName;
    DialogParams.lpstrFile[0] = '\0';
    DialogParams.nMaxFile = FileNameSize;
    DialogParams.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;

    BOOL Result = GetSaveFileNameA(&DialogParams); // TODO: Unicode support?

    // Suffix .png if required
    {
        int32 L = strlen(FileName);
        if (L <= FileNameSize - 4 &&                                //
            !((FileName[L-4] == '.') &&                             // Ugh.
              (FileName[L-3] == 'p' || FileName[L-3] == 'P') &&     // TODO: Write some string utility functions
              (FileName[L-2] == 'n' || FileName[L-2] == 'N') &&     //       and clean this up.
              (FileName[L-1] == 'g' || FileName[L-1] == 'G')))      //       Also, support for more than just PNGs.
        { 
            strcat(FileName, ".png");
        }
    }

    char Buffer[256];
    sprintf(Buffer, "%s\n", FileName);
    OutputDebugStringA(Buffer);

    if (Result)
    {
        return FileName;
    }
    else
    {
        free(FileName);
        return 0;
    }
}

int64 Platform::GetMilliseconds()
{
    LARGE_INTEGER ms;
    QueryPerformanceCounter(&ms);
    return ms.QuadPart;
}

// =================================================================================================

internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    ImGuiIO& io = ImGui::GetIO();

    switch (Message)
    {
        #pragma region Mouse

        case WM_LBUTTONDOWN:
        {
            io.MouseDown[0] = true;
            return true;
        } break;

        case WM_LBUTTONUP:
        {
            io.MouseDown[0] = false;
            return true;
        } break;

        case WM_RBUTTONDOWN:
        {
            io.MouseDown[1] = true;
            return true;
        } break;
        case WM_RBUTTONUP:
        {
            io.MouseDown[1] = false;
            return true;
        } break;

        case WM_MBUTTONDOWN:
        {
            io.MouseDown[2] = true;
            return true;
        } break;

        case WM_MBUTTONUP:
        {
            io.MouseDown[2] = false;
            return true;
        } break;

        case WM_MOUSEWHEEL:
        {
            io.MouseWheel += GET_WHEEL_DELTA_WPARAM(WParam) > 0 ? +1.0f : -1.0f;
            return true;
        } break;

        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT TrackParam = {};
            TrackParam.dwFlags |= TME_LEAVE;
            TrackParam.hwndTrack = Window;
            TrackParam.cbSize = sizeof(TrackParam);
            TrackMouseEvent(&TrackParam);
            io.MousePos.x = (signed short)(LParam);
            io.MousePos.y = (signed short)(LParam >> 16);
            return true;
        } break;

        case WM_MOUSELEAVE:
        {
            ImGui::GetIO().MouseDown[0] = false;
            ImGui::GetIO().MouseDown[1] = false;
            ImGui::GetIO().MouseDown[2] = false;
            return true;
        } break;

        #pragma endregion

        #pragma region Keyboard

        case WM_KEYDOWN:
        {
            if (WParam < 256)
                io.KeysDown[WParam] = 1;
            return true;
        } break;

        case WM_KEYUP:
        {
            if (WParam < 256)
                io.KeysDown[WParam] = 0;
            return true;
        } break;

        case WM_CHAR:
        {
            // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
            if (WParam > 0 && WParam < 0x10000)
                io.AddInputCharacter((unsigned short)WParam);
            return true;
        } break;

        #pragma endregion

        #pragma region Window handling

        case WM_DESTROY:
        {
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

            // Clear and swap buffers
            {
                if (Memory.InterfaceColors[PapayaInterfaceColor_Clear])
                {
                    glClearBufferfv(GL_COLOR, 0, (GLfloat*)&Memory.InterfaceColors[PapayaInterfaceColor_Clear]);
                }
                SwapBuffers(DeviceContext);
            }
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
        WindowClass.hIcon = (HICON)LoadImage(0, "../../img/papaya.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
        WindowClass.lpszClassName = "PapayaWindowClass";

        if (!RegisterClassA(&WindowClass))
        {
            // TODO: Log: Register window class failed
            return 0;
        }

        Window =
            CreateWindowExA(
            0,                                                          // Extended window style
            WindowClass.lpszClassName,                                  // Class name,
            "Papaya",                                                   // Name,
            WS_POPUP | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,    // Window style
            CW_USEDEFAULT,                                              // X,
            CW_USEDEFAULT,                                              // Y,
            CW_USEDEFAULT,                                              // Width,
            CW_USEDEFAULT,                                              // Height,
            0,                                                          // Window Parent,
            0,                                                          // Menu,
            Instance,                                                   // Handle to the instance,
            0);                                                         // lpParam

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

            if (!gl3wIsSupported(3,1))
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

    Papaya::Initialize(&Memory);

    #pragma region Initialize ImGui
    {
        QueryPerformanceFrequency((LARGE_INTEGER *)&DebugMemory.TicksPerSecond);
        QueryPerformanceCounter((LARGE_INTEGER *)&DebugMemory.Time);

        ImGuiIO& io = ImGui::GetIO();
        io.KeyMap[ImGuiKey_Tab] = VK_TAB;          // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
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

        io.RenderDrawListsFn = Papaya::RenderImGui;
        io.ImeWindowHandle = Window;
    }
    #pragma endregion

    Memory.Window.MenuHorizontalOffset = 32;
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

        #pragma region Start new ImGui frame
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
        #pragma endregion

        Papaya::UpdateAndRender(&Memory, &DebugMemory);

        #pragma region Title Bar Icon
        {
            ImGui::SetNextWindowSize(ImVec2((float)Memory.Window.MenuHorizontalOffset,(float)Memory.Window.TitleBarHeight));
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

            bool bTrue = true;
            ImGui::Begin("Title Bar Icon", &bTrue, WindowFlags);
            ImGui::Image((void*)(intptr_t)Memory.InterfaceTextureIDs[PapayaInterfaceTexture_TitleBarIcon], ImVec2(28,28));
            ImGui::End();

            ImGui::PopStyleColor(1);
            ImGui::PopStyleVar(5);
        }
        #pragma endregion

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

        //ImGui::ShowTestWindow();
        ImGui::Render(&Memory);
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

    Papaya::Shutdown(&Memory);
    return 0;
}
