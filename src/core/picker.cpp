
#include "picker.h"
#include "libs/imgui/imgui.h"
#include "libs/mathlib.h"
#include "papaya_core.h"

void picker::init(Picker* picker)
{
    picker->current_color = Color(220, 163, 89);
    picker->is_open = false;
    picker->pos = Vec2(34, 152);
    picker->size = Vec2(292, 350);
    picker->hue_strip_pos  = Vec2(259, 42);
    picker->hue_strip_size = Vec2(30, 256);
    picker->sv_box_pos = Vec2(0, 42);
    picker->sv_box_size = Vec2(256, 256);
    picker->cursor_sv = Vec2(0.5f, 0.5f);
    picker->bound_color = 0;
}

void picker::set_color(Color col, Picker* picker, bool set_new_color_only)
{
    if (!set_new_color_only) { picker->current_color = col; }
    picker->new_color = col;
    math::rgb_to_hsv(picker->new_color.r, picker->new_color.g, picker->new_color.b,
                   picker->cursor_h, picker->cursor_sv.x, picker->cursor_sv.y);
    if (picker->bound_color) { *picker->bound_color = col; }
}

// TODO: Remove the last param
void picker::update(Picker* picker, Color* colors, Mouse& mouse, uint32 blank)
{
    // Picker panel
    {
        // TODO: Clean styling code
        ImGui::SetNextWindowSize(ImVec2(picker->size.x, 35));
        ImGui::SetNextWindowPos (picker->pos);

        ImGuiWindowFlags WindowFlags = 0;
        WindowFlags |= ImGuiWindowFlags_NoTitleBar;
        WindowFlags |= ImGuiWindowFlags_NoResize;
        WindowFlags |= ImGuiWindowFlags_NoMove;
        WindowFlags |= ImGuiWindowFlags_NoScrollbar;
        WindowFlags |= ImGuiWindowFlags_NoCollapse;
        WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding  , ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding , 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing    , ImVec2(0, 0));

        ImGui::PushStyleColor(ImGuiCol_Button        , colors[PapayaCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered , colors[PapayaCol_ButtonHover]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive  , colors[PapayaCol_ButtonActive]);
        ImGui::PushStyleColor(ImGuiCol_WindowBg      , colors[PapayaCol_Transparent]);

        ImGui::Begin("Color preview", 0, WindowFlags);
        {
            float width1 = (picker->size.x + 33.0f) / 2.0f;
            float width2 = width1 - 33.0f;
            if (ImGui::ImageButton((void*)(intptr_t)blank,
                                   ImVec2(width2, 34), ImVec2(0, 0),
                                   ImVec2(0, 0), 0, picker->current_color)) {
                picker->is_open = false;
                if (picker->bound_color) {
                    *picker->bound_color = picker->current_color;
                }
                picker->bound_color = 0;
            }
            ImGui::SameLine();
            ImGui::PushID(1);
            if (ImGui::ImageButton((void*)(intptr_t)blank,
                                   ImVec2(width1, 34), ImVec2(0, 0),
                                   ImVec2(0, 0), 0, picker->new_color)) {
                picker->is_open = false;
                picker->current_color = picker->new_color;
                picker->bound_color = 0;
            }
            ImGui::PopID();
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        ImGui::PushStyleVar  (ImGuiStyleVar_WindowPadding, ImVec2(0, 8));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, colors[PapayaCol_Clear]);

        ImGui::SetNextWindowSize(picker->size - Vec2(0.0f, 34.0f));
        ImGui::SetNextWindowPos (picker->pos  + Vec2(0.0f, 34.0f));
        ImGui::PushStyleVar     (ImGuiStyleVar_WindowRounding, 0);
        ImGui::Begin("Color picker", 0, WindowFlags);
        {
            ImGui::BeginChild("HSV Gradients", Vec2(315, 258));
            ImGui::EndChild();
            int32 rgbColor[3];
            rgbColor[0] = (int32)(picker->new_color.r * 255.0f);
            rgbColor[1] = (int32)(picker->new_color.g * 255.0f);
            rgbColor[2] = (int32)(picker->new_color.b * 255.0f);
            if (ImGui::InputInt3("RGB", rgbColor)) {
                int32 r = math::clamp(rgbColor[0], 0, 255);
                int32 g = math::clamp(rgbColor[1], 0, 255);
                int32 b = math::clamp(rgbColor[2], 0, 255);
                picker->new_color = Color(r, g, b);
                math::rgb_to_hsv(picker->new_color.r, picker->new_color.g, picker->new_color.b,
                               picker->cursor_h, picker->cursor_sv.x, picker->cursor_sv.y);
            }
            char hexColor[6 + 1]; // null-terminated
            snprintf(hexColor, sizeof(hexColor), "%02x%02x%02x",
                     (int32)(picker->new_color.r * 255.0f),
                     (int32)(picker->new_color.g * 255.0f),
                     (int32)(picker->new_color.b * 255.0f));
            if (ImGui::InputText("Hex", hexColor, sizeof(hexColor),
                                 ImGuiInputTextFlags_CharsHexadecimal)) {
                int32 r = 0, g = 0, b = 0;
                switch (strlen(hexColor)) {
                    case 1: sscanf(hexColor, "%1x", &b); break;
                    case 2: sscanf(hexColor, "%2x", &b); break;
                    case 3: sscanf(hexColor, "%1x%2x", &g, &b); break;
                    case 4: sscanf(hexColor, "%2x%2x", &g, &b); break;
                    case 5: sscanf(hexColor, "%1x%2x%2x", &r, &g, &b); break;
                    case 6: sscanf(hexColor, "%2x%2x%2x", &r, &g, &b); break;
                }
                picker->new_color = Color(r, g, b);
                math::rgb_to_hsv(picker->new_color.r, picker->new_color.g, picker->new_color.b,
                               picker->cursor_h, picker->cursor_sv.x, picker->cursor_sv.y);
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);
    }

    // Update hue picker
    {
        Vec2 pos = picker->pos + picker->hue_strip_pos;

        if (mouse.pressed[0] &&
            mouse.pos.x > pos.x &&
            mouse.pos.x < pos.x + picker->hue_strip_size.x &&
            mouse.pos.y > pos.y &&
            mouse.pos.y < pos.y + picker->hue_strip_size.y) {
            picker->dragging_hue = true;
        }
        else if (mouse.released[0] && picker->dragging_hue) {
            picker->dragging_hue = false;
        }

        if (picker->dragging_hue) {
            picker->cursor_h = (mouse.pos.y - pos.y) / 256.0f;
            picker->cursor_h = 1.f - math::clamp(picker->cursor_h, 0.0f, 1.0f);
        }

    }

    // Update saturation-value picker
    {
        Vec2 pos = picker->pos + picker->sv_box_pos;

        //TODO: Create a rect structure
        if (mouse.pressed[0] &&
            mouse.pos.x > pos.x &&
            mouse.pos.x < pos.x + picker->sv_box_size.x &&
            mouse.pos.y > pos.y &&
            mouse.pos.y < pos.y + picker->sv_box_size.y) {
            picker->dragging_sv = true;
        }
        else if (mouse.released[0] && picker->dragging_sv) {
            picker->dragging_sv = false;
        }

        if (picker->dragging_sv) {
            picker->cursor_sv.x = (mouse.pos.x - pos.x) / 256.f;
            picker->cursor_sv.y = 1.f - (mouse.pos.y - pos.y) / 256.f;
            picker->cursor_sv.x = math::clamp(picker->cursor_sv.x, 0.f, 1.f);
            picker->cursor_sv.y = math::clamp(picker->cursor_sv.y, 0.f, 1.f);

        }
    }

    // Update new color
    {
        float r, g, b;
        math::hsv_to_rgb(picker->cursor_h, picker->cursor_sv.x, picker->cursor_sv.y,
                       r, g, b);
        // Note: Rounding is essential. Without it, RGB->HSV->RGB is a lossy
        // operation.
        picker->new_color = Color(math::round_to_int(r * 255.0f),
                                  math::round_to_int(g * 255.0f),
                                  math::round_to_int(b * 255.0f));
    }

    if (picker->bound_color) { *picker->bound_color = picker->new_color; }
}

