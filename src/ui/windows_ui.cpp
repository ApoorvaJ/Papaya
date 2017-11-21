
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <malloc.h>
#include <commdlg.h>
#undef GetWindowFont // Windows API macro clashes with ImGui function

#include "ui.h"

#include "gl_lite.h"
#include "libs/imgui/imgui.h"
#include "libs/easytab.h"

#ifndef PAPAYARELEASE
#define PAPAYA_DEFAULT_IMAGE "C:\\Users\\Apoorva\\Pictures\\ImageTest\\fruits.png"
#endif // !PAPAYARELEASE

// =================================================================================================

static PapayaMemory mem = {};
static HDC device_context;
static HGLRC rendering_context;
static RECT windows_work_area; // Needed because WS_POPUP by default maximizes to cover task bar

// =================================================================================================

void platform::print(const char* msg)
{
    OutputDebugString((LPCSTR)msg);
}

void platform::start_mouse_capture()
{
    SetCapture(GetActiveWindow());
}

void platform::stop_mouse_capture()
{
    ReleaseCapture();
}

void platform::set_mouse_position(i32 x, i32 y)
{
    RECT rect;
    GetWindowRect(GetActiveWindow(), &rect);
    SetCursorPos(rect.left + x, rect.top + y);
}

void platform::set_cursor_visibility(bool visible)
{
    ShowCursor(visible);
}

char* platform::open_file_dialog()
{
    const i32 file_name_size = MAX_PATH;
    char* file_name = (char*)malloc(file_name_size);

    OPENFILENAME dialog_params = {};
    dialog_params.lStructSize = sizeof(OPENFILENAME);
    dialog_params.hwndOwner = GetActiveWindow();
    dialog_params.lpstrFilter = "JPEG\0*.jpg;*.jpeg\0PNG\0*.png\0";
    dialog_params.nFilterIndex = 2;
    dialog_params.lpstrFile = file_name;
    dialog_params.lpstrFile[0] = '\0';
    dialog_params.nMaxFile = file_name_size;
    dialog_params.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    BOOL result = GetOpenFileNameA(&dialog_params); // TODO: Unicode support?

    if (result) {
        return file_name;
    } else {
        free(file_name);
        return 0;
    }
}

char* platform::save_file_dialog()
{
    const i32 file_nameSize = MAX_PATH;
    char* file_name = (char*)malloc(file_nameSize);

    OPENFILENAME dialog_params = {};
    dialog_params.lStructSize = sizeof(OPENFILENAME);
    dialog_params.hwndOwner = GetActiveWindow();
    dialog_params.lpstrFilter = "PNG\0*.png\0";
    dialog_params.nFilterIndex = 2;
    dialog_params.lpstrFile = file_name;
    dialog_params.lpstrFile[0] = '\0';
    dialog_params.nMaxFile = file_nameSize;
    dialog_params.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;

    BOOL result = GetSaveFileNameA(&dialog_params); // TODO: Unicode support?

    // Suffix .png if required
    {
        size_t len = strlen(file_name);
        if (len <= file_nameSize - 4 &&                                   //
            !((file_name[len-4] == '.') &&                                // Ugh.
              (file_name[len-3] == 'p' || file_name[len-3] == 'P') &&     // TODO: Write some string utility functions
              (file_name[len-2] == 'n' || file_name[len-2] == 'N') &&     //       and clean this up.
              (file_name[len-1] == 'g' || file_name[len-1] == 'G'))) {    //       Also, support for more than just PNGs.
            strcat(file_name, ".png");
        }
    }

    char buffer[MAX_PATH+2];
    sprintf(buffer, "%s\n", file_name);
    OutputDebugStringA(buffer);

    if (result) {
        return file_name;
    } else {
        free(file_name);
        return 0;
    }
}

// =================================================================================================

static LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT msg, WPARAM w_param, LPARAM l_param)
{
    if (EasyTab_HandleEvent(window, msg, l_param, w_param) == EASYTAB_OK) {
        return true;  // Tablet input
    }

    LRESULT result = 0;
    ImGuiIO& io = ImGui::GetIO();

    switch (msg) {
        // Mouse
        case WM_LBUTTONDOWN: {
            io.MouseDown[0] = true;
            return true;
        } break;

        case WM_LBUTTONUP: {
            io.MouseDown[0] = false;
            return true;
        } break;

        case WM_RBUTTONDOWN: {
            io.MouseDown[1] = true;
            return true;
        } break;

        case WM_RBUTTONUP: {
            io.MouseDown[1] = false;
            return true;
        } break;

        case WM_MBUTTONDOWN: {
            io.MouseDown[2] = true;
            return true;
        } break;

        case WM_MBUTTONUP: {
            io.MouseDown[2] = false;
            return true;
        } break;

        case WM_MOUSEWHEEL: {
            io.MouseWheel += GET_WHEEL_DELTA_WPARAM(w_param) > 0 ? +1.0f : -1.0f;
            return true;
        } break;

        case WM_MOUSEMOVE: {
            TRACKMOUSEEVENT track_param = {};
            track_param.dwFlags |= TME_LEAVE;
            track_param.hwndTrack = window;
            track_param.cbSize = sizeof(track_param);
            TrackMouseEvent(&track_param);
            io.MousePos.x = (signed short)(l_param);
            io.MousePos.y = (signed short)(l_param >> 16);
            return true;
        } break;

        case WM_MOUSELEAVE: {
            ImGui::GetIO().MouseDown[0] = false;
            ImGui::GetIO().MouseDown[1] = false;
            ImGui::GetIO().MouseDown[2] = false;
            return true;
        } break;

        // Keyboard
        case WM_KEYDOWN: {
            if (w_param < 256)
                io.KeysDown[w_param] = 1;
            return true;
        } break;

        case WM_KEYUP: {
            if (w_param < 256)
                io.KeysDown[w_param] = 0;
            return true;
        } break;

        case WM_CHAR: {
            // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
            if (w_param > 0 && w_param < 0x10000)
                io.AddInputCharacter((unsigned short)w_param);
            return true;
        } break;


        // window handling
        case WM_DESTROY: {
            mem.is_running = false;
            if (rendering_context)
            {
                wglMakeCurrent(NULL, NULL);
                wglDeleteContext(rendering_context);
            }
            ReleaseDC(window, device_context);
            PostQuitMessage(0);
        } break;

        case WM_PAINT: {
            PAINTSTRUCT paint;
            BeginPaint(window, &paint);
            // TODO: Redraw here
            EndPaint(window, &paint);
        } break;

        case WM_CLOSE: {
            // TODO: Handle this with a message to the user?
            mem.is_running = false;
        } break;

        case WM_SIZE: {
            i32 width, height;

            if (w_param == SIZE_MAXIMIZED) {
                i32 work_area_width = windows_work_area.right - windows_work_area.left;
                i32 work_area_height = windows_work_area.bottom - windows_work_area.top;
                SetWindowPos(window, HWND_TOP, windows_work_area.left, windows_work_area.top, work_area_width, work_area_height, NULL);
                width = work_area_width;
                height = work_area_height;
            }
            else
            {
                width = (i32) LOWORD(l_param);
                height = (i32) HIWORD(l_param);
            }

            core::resize(&mem, width, height);

            // Clear and swap buffers
            {
                if (mem.colors[PapayaCol_Clear]) {
                    glClearBufferfv(GL_COLOR, 0, (GLfloat*)&mem.colors[PapayaCol_Clear]);
                }
                SwapBuffers(device_context);
            }
        } break;

        // WM_NCHITTEST

        case WM_NCHITTEST: {
            const LONG border_width = 8; //in pixels
            RECT window_rect;
            GetWindowRect(window, &window_rect);
            long X = GET_X_LPARAM(l_param);
            long Y = GET_Y_LPARAM(l_param);

            if (!IsMaximized(window))
            {
                //bottom left corner
                if (X >= window_rect.left && X < window_rect.left + border_width &&
                    Y < window_rect.bottom && Y >= window_rect.bottom - border_width) {
                    return HTBOTTOMLEFT;
                }
                //bottom right corner
                if (X < window_rect.right && X >= window_rect.right - border_width &&
                    Y < window_rect.bottom && Y >= window_rect.bottom - border_width) {
                    return HTBOTTOMRIGHT;
                }
                //top left corner
                if (X >= window_rect.left && X < window_rect.left + border_width &&
                    Y >= window_rect.top && Y < window_rect.top + border_width) {
                    return HTTOPLEFT;
                }
                //top right corner
                if (X < window_rect.right && X >= window_rect.right - border_width &&
                    Y >= window_rect.top && Y < window_rect.top + border_width) {
                    return HTTOPRIGHT;
                }
                //left border
                if (X >= window_rect.left && X < window_rect.left + border_width) {
                    return HTLEFT;
                }
                //right border
                if (X < window_rect.right && X >= window_rect.right - border_width) {
                    return HTRIGHT;
                }
                //bottom border
                if (Y < window_rect.bottom && Y >= window_rect.bottom - border_width) {
                    return HTBOTTOM;
                }
                //top border
                if (Y >= window_rect.top && Y < window_rect.top + border_width) {
                    return HTTOP;
                }
            }

            if (Y - window_rect.top <= (f32)mem.window.title_bar_height &&
                X > window_rect.left + 200.0f &&
                X < window_rect.right - (f32)(mem.window.title_bar_buttons_width + 10)) {
                return HTCAPTION;
            }

            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return HTCLIENT;
        } break;

        default: {
            result = DefWindowProcA(window, msg, w_param, l_param);
        } break;
    }

    return(result);
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_code)
{
    timer::init();

    QueryPerformanceCounter((LARGE_INTEGER *)&mem.profile);
    timer::start(Timer_Startup);

    mem.is_running = true;

    // Set current path to this executable's path
    {
        HMODULE module = GetModuleHandleA(NULL);
        char path_string[MAX_PATH];
        u32 path_len = GetModuleFileNameA(module, path_string, MAX_PATH);
        if (path_len != -1) {
            char *last_slash = strrchr(path_string, '\\');
            if (last_slash) { *last_slash = '\0'; }
            SetCurrentDirectoryA(path_string);
        }
    }

    HWND window;
    // Create window
    {
        WNDCLASSA window_class = {};
        window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        window_class.lpfnWndProc = Win32MainWindowCallback;
        window_class.hInstance = instance;
        const char* ico_path = "papaya.ico";
        window_class.hIcon = (HICON)LoadImage(0, ico_path, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);
        window_class.lpszClassName = "PapayaWindowClass";

        if (!RegisterClassA(&window_class)) {
            // TODO: Log: Register window class failed
            return 0;
        }

        window =
            CreateWindowExA(
            0,                                                          // Extended window style
            window_class.lpszClassName,                                  // Class name,
            "Papaya",                                                   // Name,
            WS_POPUP | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,    // window style
            CW_USEDEFAULT,                                              // X,
            CW_USEDEFAULT,                                              // Y,
            CW_USEDEFAULT,                                              // width,
            CW_USEDEFAULT,                                              // height,
            0,                                                          // window Parent,
            0,                                                          // Menu,
            instance,                                                   // Handle to the instance,
            0);                                                         // lpParam

        if (!window) {
            // TODO: Log: Create window failed
            return 0;
        }

        SystemParametersInfo(SPI_GETWORKAREA, 0, &windows_work_area, 0);

#ifdef PAPAYARELEASE

        SetWindowPos(window, HWND_TOP, 0, 0, 600, 600, NULL);
        ShowWindow(window, SW_MAXIMIZE);
#else
        u32 screen_width = GetSystemMetrics(SM_CXSCREEN);
        u32 screen_height = GetSystemMetrics(SM_CYSCREEN);

        mem.window.width = (u32)((f32)screen_width * 0.5);
        mem.window.height = (u32)((f32)screen_height * 0.8);

        u32 window_x = (screen_width - mem.window.width) / 2;
        u32 window_y = (screen_height - mem.window.height) / 2;

        SetWindowPos(window, HWND_TOP, window_x, window_y, mem.window.width, mem.window.height, NULL);
#endif // PAPAYARELEASE

        // Get window size
        {
            RECT window_rect = { 0 };
            BOOL foo = GetClientRect(window, &window_rect);
            mem.window.width = window_rect.right - window_rect.left;
            mem.window.height = window_rect.bottom - window_rect.top;
        }
    }

    // Initialize OpenGL
    {
        device_context = GetDC(window);

        // Setup pixel format
        {
            PIXELFORMATDESCRIPTOR pixel_format_desc = { 0 };
            // TODO: Program seems to work perfectly fine without all other params except dwFlags.
            //       Can we skip other params for the sake of brevity?
            pixel_format_desc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
            pixel_format_desc.nVersion = 1;
            pixel_format_desc.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
            pixel_format_desc.iPixelType = PFD_TYPE_RGBA;
            pixel_format_desc.cColorBits = 32;
            pixel_format_desc.cDepthBits = 32;
            pixel_format_desc.dwLayerMask = PFD_MAIN_PLANE;
            //

            i32 pixel_format = ChoosePixelFormat(device_context, &pixel_format_desc);
            if (!pixel_format) { exit(1); } // TODO: Log: Choose pixel format failed
            if (!SetPixelFormat(device_context, pixel_format, &pixel_format_desc)) { exit(1); } // TODO: Log: Set pixel format failed
        }

        // Create rendering context
        {
            // TODO: Create "proper" context?
            //       https://www.opengl.org/wiki/Creating_an_OpenGL_Context_(WGL)#Proper_Context_Creation

            HGLRC rendering_context = wglCreateContext(device_context);
            wglMakeCurrent(device_context, rendering_context);

            if (!gl_lite_init()) { exit(1); }

            glGetIntegerv(GL_MAJOR_VERSION, &mem.sys.gl_version[0]);
            glGetIntegerv(GL_MINOR_VERSION, &mem.sys.gl_version[1]);
            mem.sys.gl_vendor = (char*)glGetString(GL_VENDOR);
            mem.sys.gl_renderer = (char*)glGetString(GL_RENDERER);
        }

        // Disable vsync
        //if (wglewIsSupported("WGL_EXT_swap_control")) { wglSwapIntervalEXT(0); }
    }

    // Initialize tablet
    EasyTab_Load(window);

    core::init(&mem);

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

        io.RenderDrawListsFn = core::render_imgui;
        io.ImeWindowHandle = window;
    }

    mem.window.menu_horizontal_offset = 32;
    mem.window.title_bar_buttons_width = 109;
    mem.window.title_bar_height = 30;

    timer::stop(Timer_Startup);

    // Handle command line arguments (if present)
    if (strlen(cmd_line)) {
        // Remove f64 quotes from string if present
        char* ptr_read  = cmd_line;
        char* ptr_write = cmd_line;
        while (*ptr_read) 
        {
            *ptr_write = *ptr_read++;
            if (*ptr_write != '\"') { ptr_write++; }
        }
        *ptr_write = '\0';
        core::open_doc(cmd_line, &mem); 
    }

#ifdef PAPAYA_DEFAULT_IMAGE
    core::open_doc(PAPAYA_DEFAULT_IMAGE, &mem);
#endif // PAPAYA_DEFAULT_IMAGE

    while (mem.is_running) {
        timer::start(Timer_Frame);

        // Windows message handling
        {
            MSG msg;
            while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT) {
                    mem.is_running = false;
                }

                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }

        // Tablet input // TODO: Put this in papaya.cpp
        {
            mem.tablet.pressure = EasyTab->Pressure;
            mem.tablet.pos.x = EasyTab->PosX;
            mem.tablet.pos.y = EasyTab->PosY;
            mem.tablet.buttons = EasyTab->Buttons;
        }

        BOOL is_maximized = IsMaximized(window);
        if (IsIconic(window)) { goto EndOfFrame; }

        // Start new ImGui frame
        {
            ImGuiIO& io = ImGui::GetIO();

            // Setup display size (every frame to accommodate for window resizing)
            RECT rect;
            GetClientRect(window, &rect);
            io.DisplaySize = ImVec2((f32)(rect.right - rect.left), (f32)(rect.bottom - rect.top));

            // Read keyboard modifiers inputs
            io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;

            // Setup time step
            INT64 current_time;
            QueryPerformanceCounter((LARGE_INTEGER *)&current_time);
            io.DeltaTime = (f32)(current_time - mem.profile.current_time) * 
                           (f32)timer::get_freq() / 1000.0f;
            mem.profile.current_time = current_time; // TODO: Move Imgui timers from Debug to their own struct

            // Hide OS mouse cursor if ImGui is drawing it
            //SetCursor(io.MouseDrawCursor ? NULL : LoadCursor(NULL, IDC_ARROW));

            // Start the frame
            ImGui::NewFrame();
        }

        // Title Bar Icon
        {
            ImGui::SetNextWindowSize(ImVec2((f32)mem.window.menu_horizontal_offset,(f32)mem.window.title_bar_height));
            ImGui::SetNextWindowPos(ImVec2(1.0f, 1.0f));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2,2));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0,0));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

            ImGui::PushStyleColor(ImGuiCol_WindowBg, mem.colors[PapayaCol_Transparent]);

            bool bTrue = true;
            ImGui::Begin("Title Bar Icon", &bTrue, mem.window.default_imgui_flags);

            #define CALCUV(X, Y) ImVec2((f32)X/256.0f, (f32)Y/256.0f)
            ImGui::Image((void*)(intptr_t)mem.textures[PapayaTex_UI], ImVec2(28,28), CALCUV(0,200), CALCUV(28,228));
            #undef CALCUV

            ImGui::End();

            ImGui::PopStyleColor(1);
            ImGui::PopStyleVar(5);
        }

        // Title Bar Buttons
        {
            ImGui::SetNextWindowSize(ImVec2((f32)mem.window.title_bar_buttons_width,24.0f));
            ImGui::SetNextWindowPos(ImVec2((f32)mem.window.width - mem.window.title_bar_buttons_width, 0.0f));

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0,0));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

            ImGui::PushStyleColor(ImGuiCol_Button, mem.colors[PapayaCol_Transparent]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, mem.colors[PapayaCol_ButtonHover]);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, mem.colors[PapayaCol_ButtonActive]);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, mem.colors[PapayaCol_Transparent]);

            bool bTrue = true;

            #define CALCUV(X, Y) ImVec2((f32)X/256.0f, (f32)Y/256.0f)
            ImGui::Begin("Title Bar Buttons", &bTrue, mem.window.default_imgui_flags);

            ImGui::PushID(0);
            if(ImGui::ImageButton((void*)(size_t)mem.textures[PapayaTex_UI], 
                                  ImVec2(34,26), CALCUV(62,200), CALCUV(96,226),
                                  1, ImVec4(0,0,0,0))) {
                ShowWindow(window, SW_MINIMIZE);
            }

            ImVec2 start_uv = is_maximized ? CALCUV(28,226) : CALCUV(62,226);
            ImVec2 end_uv = is_maximized ? CALCUV(62,252) : CALCUV(96,252);

            ImGui::PopID();
            ImGui::SameLine();
            ImGui::PushID(1);

            if(ImGui::ImageButton((void*)(size_t)mem.textures[PapayaTex_UI],
                                  ImVec2(34,26), start_uv, end_uv,
                                  1, ImVec4(0,0,0,0))) {
                if (is_maximized) {
                    ShowWindow(window, SW_RESTORE);
                } else {
                    ShowWindow(window, SW_MAXIMIZE);
                }
            }

            ImGui::PopID();
            ImGui::SameLine();
            ImGui::PushID(2);

            if(ImGui::ImageButton((void*)(size_t)mem.textures[PapayaTex_UI],
                                  ImVec2(34,26), CALCUV(28,200), CALCUV(62,226),
                                  1, ImVec4(0,0,0,0))) {
                SendMessage(window, WM_CLOSE, 0, 0);
            }

            ImGui::PopID();
            ImGui::End();
            #undef CALCUV

            ImGui::PopStyleVar(5);
            ImGui::PopStyleColor(4);
        }

        // ImGui::ShowTestWindow();
        core::update(&mem);
        SwapBuffers(device_context);

    EndOfFrame:
        timer::stop(Timer_Frame);
        f64 frame_rate = (mem.current_tool == PapayaTool_Brush && mem.mouse.is_down[0]) ?
                           500.0 : 60.0;
        f64 frame_time = 1000.0 / frame_rate;
        f64 sleep_time = frame_time - timers[Timer_Frame].elapsed_ms;
        timers[Timer_Sleep].elapsed_ms = sleep_time;
        if (sleep_time > 0) { Sleep((i32)sleep_time); }
    }

    core::destroy(&mem);

    EasyTab_Unload();

    return 0;
}
