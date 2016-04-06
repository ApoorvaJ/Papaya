
#include <windows.h>
#include <windowsx.h>
#include <malloc.h>
#include <commdlg.h>
#undef GetWindowFont // Windows API macro clashes with ImGui function
#include "papaya_platform.h"

// =================================================================================================

#pragma warning (disable: 4312) // Warning C4312 during 64 bit compilation: 'type cast': conversion from 'uint32' to 'void *' of greater size
#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"
#pragma warning (default: 4312)

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"

// =================================================================================================
#include "papaya_core.h"

#define GLEW_STATIC
#include "lib/glew/glew.c"
#include "lib/glew/wglew.h"

#include "lib/imgui/imgui.h"
#include "lib/imgui/imgui.cpp"


#define MATHLIB_IMPLEMENTATION
#include "lib/mathlib.h"

#define GL_IMPLEMENTATION
#include "lib/gl.h"
#define TIMER_IMPLEMENTATION
#include "lib/timer.h"
#define EASYTAB_IMPLEMENTATION
#include "lib/easytab.h"


#ifndef PAPAYARELEASE
#define PAPAYA_DEFAULT_IMAGE "C:\\Users\\Apoorva\\Pictures\\ImageTest\\fruits.png"
#endif // !PAPAYARELEASE

// =================================================================================================

global_variable PapayaMemory Mem = {};
global_variable HDC DeviceContext;
global_variable HGLRC RenderingContext;
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

void Platform::SetMousePosition(int32 x, int32 y)
{
    RECT Rect;
    GetWindowRect(GetActiveWindow(), &Rect);
    SetCursorPos(Rect.left + x, Rect.top + y);
}

void Platform::SetCursorVisibility(bool Visible)
{
    ShowCursor(Visible);
}

char* Platform::OpenFileDialog()
{
    const int32 FileNameSize = MAX_PATH;
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
    const int32 FileNameSize = MAX_PATH;
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
        size_t L = strlen(FileName);
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

double Platform::GetMilliseconds()
{
    LARGE_INTEGER ms;
    QueryPerformanceCounter(&ms);
    return (double)ms.QuadPart;
}


// =================================================================================================

internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    if (EasyTab_HandleEvent(Window, Message, LParam, WParam) == EASYTAB_OK)
    {
        return true;  // Tablet input
    }

    LRESULT Result = 0;
    ImGuiIO& io = ImGui::GetIO();

    switch (Message)
    {
        // Mouse

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


        // Keyboard

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


        // Window handling

        case WM_DESTROY:
        {
            Mem.IsRunning = false;
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
            Mem.IsRunning = false;
        } break;

        case WM_SIZE:
        {
            if (WParam == SIZE_MAXIMIZED)
            {
                int32 WorkAreaWidth = WindowsWorkArea.right - WindowsWorkArea.left;
                int32 WorkAreaHeight = WindowsWorkArea.bottom - WindowsWorkArea.top;
                SetWindowPos(Window, HWND_TOP, WindowsWorkArea.left, WindowsWorkArea.top, WorkAreaWidth, WorkAreaHeight, NULL);
                Mem.Window.Width = WorkAreaWidth;
                Mem.Window.Height = WorkAreaHeight;
            }
            else
            {
                Mem.Window.Width = (int32) LOWORD(LParam);
                Mem.Window.Height = (int32) HIWORD(LParam);
            }

            // Clear and swap buffers
            {
                if (Mem.Colors[PapayaCol_Clear])
                {
                    glClearBufferfv(GL_COLOR, 0, (GLfloat*)&Mem.Colors[PapayaCol_Clear]);
                }
                SwapBuffers(DeviceContext);
            }
        } break;

        // WM_NCHITTEST

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

            if (Y - WindowRect.top <= (float)Mem.Window.TitleBarHeight &&
                X > WindowRect.left + 200.0f &&
                X < WindowRect.right - (float)(Mem.Window.TitleBarButtonsWidth + 10))
            {
                return HTCAPTION;
            }

            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return HTCLIENT;
        } break;

        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    // Tick frequency
    {
        int64 TicksPerSecond;
        QueryPerformanceFrequency((LARGE_INTEGER *)&TicksPerSecond);
        Timer::Init(1000.0 / TicksPerSecond);
    }

    QueryPerformanceCounter((LARGE_INTEGER *)&Mem.Debug.Time);
    Timer::StartTime(&Mem.Debug.Timers[Timer_Startup]);

    Mem.IsRunning = true;

#ifdef PAPAYARELEASE
    // Set current path to this executable's path
    {
        HMODULE Module = GetModuleHandleA(NULL);
        char PathString[MAX_PATH];
        uint32 PathLength = GetModuleFileNameA(Module, PathString, MAX_PATH);
        if (PathLength != -1)
        {
            char *LastSlash = strrchr(PathString, '\\');
            if (LastSlash) { *LastSlash = '\0'; }
            SetCurrentDirectoryA(PathString);
        }
    }
#endif // PAPAYARELEASE

    HWND Window;
    // Create Window
    {
        WNDCLASSA WindowClass = {};
        WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        WindowClass.lpfnWndProc = Win32MainWindowCallback;
        WindowClass.hInstance = Instance;
#ifdef PAPAYARELEASE
        const char* IcoPath = "papaya.ico";
#else
        const char* IcoPath = "../../img/papaya.ico";
#endif // PAPAYARELEASE
        WindowClass.hIcon = (HICON)LoadImage(0, IcoPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
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

#ifdef PAPAYARELEASE

        SetWindowPos(Window, HWND_TOP, 0, 0, 600, 600, NULL);
        ShowWindow(Window, SW_MAXIMIZE);
#else
        uint32 ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
        uint32 ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

        Mem.Window.Width = (uint32)((float)ScreenWidth * 0.5);
        Mem.Window.Height = (uint32)((float)ScreenHeight * 0.8);

        uint32 WindowX = (ScreenWidth - Mem.Window.Width) / 2;
        uint32 WindowY = (ScreenHeight - Mem.Window.Height) / 2;

        SetWindowPos(Window, HWND_TOP, WindowX, WindowY, Mem.Window.Width, Mem.Window.Height, NULL);
#endif // PAPAYARELEASE

        // Get window size
        {
            RECT WindowRect = { 0 };
            BOOL foo = GetClientRect(Window, &WindowRect);
            Mem.Window.Width  = WindowRect.right - WindowRect.left;
            Mem.Window.Height = WindowRect.bottom - WindowRect.top;
        }
    }

    // Initialize OpenGL
    {
        DeviceContext = GetDC(Window);

        // Setup pixel format
        {
            PIXELFORMATDESCRIPTOR PixelFormatDesc = { 0 };
            // TODO: Program seems to work perfectly fine without all other params except dwFlags.
            //       Can we skip other params for the sake of brevity?
            PixelFormatDesc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
            PixelFormatDesc.nVersion = 1;
            PixelFormatDesc.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
            PixelFormatDesc.iPixelType = PFD_TYPE_RGBA;
            PixelFormatDesc.cColorBits = 32;
            PixelFormatDesc.cDepthBits = 32;
            PixelFormatDesc.dwLayerMask = PFD_MAIN_PLANE;
            //

            int32 PixelFormat = ChoosePixelFormat(DeviceContext, &PixelFormatDesc);
            if (!PixelFormat) { exit(1); } // TODO: Log: Choose pixel format failed
            if (!SetPixelFormat(DeviceContext, PixelFormat, &PixelFormatDesc)) { exit(1); } // TODO: Log: Set pixel format failed
        }

        // Create rendering context
        {
            // TODO: Create "proper" context?
            //       https://www.opengl.org/wiki/Creating_an_OpenGL_Context_(WGL)#Proper_Context_Creation

            HGLRC RenderingContext = wglCreateContext(DeviceContext);
            wglMakeCurrent(DeviceContext, RenderingContext);

            if (!GL::InitAndValidate()) { exit(1); }

            glGetIntegerv(GL_MAJOR_VERSION, &Mem.System.OpenGLVersion[0]);
            glGetIntegerv(GL_MINOR_VERSION, &Mem.System.OpenGLVersion[1]);
        }

        // Disable vsync
        if (wglewIsSupported("WGL_EXT_swap_control")) { wglSwapIntervalEXT(0); }
    }

    // Initialize tablet
    EasyTab_Load(Window);

    Core::Initialize(&Mem);

    // Initialize ImGui
    {
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

        io.RenderDrawListsFn = Core::RenderImGui;
        io.ImeWindowHandle = Window;
    }


    Mem.Window.MenuHorizontalOffset = 32;
    Mem.Window.TitleBarButtonsWidth = 109;
    Mem.Window.TitleBarHeight = 30;

    Timer::StopTime(&Mem.Debug.Timers[Timer_Startup]);

    // Handle command line arguments (if present)
    if (strlen(CommandLine)) 
    {
        // Remove double quotes from string if present
        {
            char* PtrRead  = CommandLine;
            char* PtrWrite = CommandLine;
            while (*PtrRead) 
            {
                *PtrWrite = *PtrRead++;
                if (*PtrWrite != '\"') { PtrWrite++; }
            }
            *PtrWrite = '\0';
        }
        Core::OpenDocument(CommandLine, &Mem); 
    }

#ifdef PAPAYA_DEFAULT_IMAGE
    Core::OpenDocument(PAPAYA_DEFAULT_IMAGE, &Mem);
#endif // PAPAYA_DEFAULT_IMAGE

    while (Mem.IsRunning)
    {
        Timer::StartTime(&Mem.Debug.Timers[Timer_Frame]);

        // Windows message handling
        {
            MSG Message;
            while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
            {
                if (Message.message == WM_QUIT)
                {
                    Mem.IsRunning = false;
                }

                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            }
        }

        // Tablet input // TODO: Put this in papaya.cpp
        {
            Mem.Tablet.Pressure = EasyTab->Pressure;
            Mem.Tablet.PosX = EasyTab->PosX;
            Mem.Tablet.PosY = EasyTab->PosY;
        }

        BOOL IsMaximized = IsMaximized(Window);
        if (IsIconic(Window)) { goto EndOfFrame; }

        // Start new ImGui frame
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
            io.DeltaTime = (float)(current_time - Mem.Debug.Time) * 
                           (float)Timer::GetFrequency() / 1000.0f;
            Mem.Debug.Time = current_time; // TODO: Move Imgui timers from Debug to their own struct

            // Hide OS mouse cursor if ImGui is drawing it
            //SetCursor(io.MouseDrawCursor ? NULL : LoadCursor(NULL, IDC_ARROW));

            // Start the frame
            ImGui::NewFrame();
        }

        // Title Bar Icon
        {
            ImGui::SetNextWindowSize(ImVec2((float)Mem.Window.MenuHorizontalOffset,(float)Mem.Window.TitleBarHeight));
            ImGui::SetNextWindowPos(ImVec2(1.0f, 1.0f));

            ImGuiWindowFlags WindowFlags = 0; // TODO: Create a commonly-used set of Window flags, and use them across ImGui windows
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

            ImGui::PushStyleColor(ImGuiCol_WindowBg, Mem.Colors[PapayaCol_Transparent]);

            bool bTrue = true;
            ImGui::Begin("Title Bar Icon", &bTrue, WindowFlags);

            #define CALCUV(X, Y) ImVec2((float)X/256.0f, (float)Y/256.0f)
            ImGui::Image((void*)(intptr_t)Mem.Textures[PapayaTex_UI], ImVec2(28,28), CALCUV(0,200), CALCUV(28,228));
            #undef CALCUV

            ImGui::End();

            ImGui::PopStyleColor(1);
            ImGui::PopStyleVar(5);
        }

        // Title Bar Buttons
        {
            ImGui::SetNextWindowSize(ImVec2((float)Mem.Window.TitleBarButtonsWidth,24.0f));
            ImGui::SetNextWindowPos(ImVec2((float)Mem.Window.Width - Mem.Window.TitleBarButtonsWidth, 0.0f));

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

            ImGui::PushStyleColor(ImGuiCol_Button, Mem.Colors[PapayaCol_Transparent]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Mem.Colors[PapayaCol_ButtonHover]);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Mem.Colors[PapayaCol_ButtonActive]);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, Mem.Colors[PapayaCol_Transparent]);

            bool bTrue = true;

            #define CALCUV(X, Y) ImVec2((float)X/256.0f, (float)Y/256.0f)
            ImGui::Begin("Title Bar Buttons", &bTrue, WindowFlags);

            ImGui::PushID(0);
            if(ImGui::ImageButton((void*)(size_t)Mem.Textures[PapayaTex_UI], ImVec2(34,26), CALCUV(62,200),CALCUV(96,226), 1, ImVec4(0,0,0,0)))
            {
                ShowWindow(Window, SW_MINIMIZE);
            }

            ImVec2 StartUV = IsMaximized ? CALCUV(28,226) : CALCUV(62,226);
            ImVec2 EndUV   = IsMaximized ? CALCUV(62,252) : CALCUV(96,252);

            ImGui::PopID();
            ImGui::SameLine();
            ImGui::PushID(1);

            if(ImGui::ImageButton((void*)(size_t)Mem.Textures[PapayaTex_UI], ImVec2(34,26), StartUV, EndUV, 1, ImVec4(0,0,0,0)))
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

            if(ImGui::ImageButton((void*)(size_t)Mem.Textures[PapayaTex_UI], ImVec2(34,26), CALCUV(28,200),CALCUV(62,226), 1, ImVec4(0,0,0,0)))
            {
                SendMessage(Window, WM_CLOSE, 0, 0);
            }

            ImGui::PopID();

            ImGui::End();
            #undef CALCUV

            ImGui::PopStyleVar(5);
            ImGui::PopStyleColor(4);
        }

        // ImGui::ShowTestWindow();
        Core::UpdateAndRender(&Mem);
        SwapBuffers(DeviceContext);

    EndOfFrame:
        Timer::StopTime(&Mem.Debug.Timers[Timer_Frame]);
        double FrameRate = (Mem.CurrentTool == PapayaTool_Brush) ? 500.0 : 60.0;
        double FrameTime = 1000.0 / FrameRate;
        double SleepTime = FrameTime - Mem.Debug.Timers[Timer_Frame].ElapsedMs;
        Mem.Debug.Timers[Timer_Sleep].ElapsedMs = SleepTime;
        if (SleepTime > 0) { Sleep((int32)SleepTime); }
    }

    Core::Shutdown(&Mem);

    EasyTab_Unload();

    return 0;
}
