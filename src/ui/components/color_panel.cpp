
#include "color_panel.h"
#include "libs/imgui/imgui.h"
#include "libs/mathlib.h"
#include "ui.h"

ColorPanel* init_color_panel()
{
    ColorPanel* c = (ColorPanel*) calloc(sizeof(*c), 1);
    c->current_color = Color(220, 163, 89);
    c->is_open = false;
    c->pos = Vec2(34, 153);
    c->size = Vec2(292, 350);
    c->hue_strip_pos  = Vec2(259, 42);
    c->hue_strip_size = Vec2(30, 256);
    c->sv_box_pos = Vec2(0, 42);
    c->sv_box_size = Vec2(256, 256);
    c->cursor_sv = Vec2(0.5f, 0.5f);
    c->bound_color = 0;
}

void destroy_color_panel(ColorPanel* c)
{
    free(c);
}

void color_panel_set_color(Color col, ColorPanel* c, bool set_new_color_only)
{
    if (!set_new_color_only) { c->current_color = col; }
    c->new_color = col;
    math::rgb_to_hsv(c->new_color.r, c->new_color.g, c->new_color.b,
                     c->cursor_h, c->cursor_sv.x, c->cursor_sv.y);
    if (c->bound_color) { *c->bound_color = col; }
}

// TODO: Remove the last param
void update_color_panel(ColorPanel* c, Color* colors, Mouse& mouse,
                        uint32 blank_texture, Layout& layout)
{
    // Picker panel
    {
        // TODO: Clean styling code
        ImGui::SetNextWindowSize(ImVec2(c->size.x, 35));
        ImGui::SetNextWindowPos (c->pos);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding  , ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding , 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing    , ImVec2(0, 0));

        ImGui::Begin("Color preview", 0, layout.default_imgui_flags);
        {
            float width1 = (c->size.x + 33.0f) / 2.0f;
            float width2 = width1 - 33.0f;
            if (ImGui::ImageButton((void*)(intptr_t)blank_texture,
                                   ImVec2(width2, 33), ImVec2(0, 0),
                                   ImVec2(0, 0), 0, c->current_color)) {
                c->is_open = false;
                if (c->bound_color) {
                    *c->bound_color = c->current_color;
                }
                c->bound_color = 0;
            }
            ImGui::SameLine();
            ImGui::PushID(1);
            if (ImGui::ImageButton((void*)(intptr_t)blank_texture,
                                   ImVec2(width1, 33), ImVec2(0, 0),
                                   ImVec2(0, 0), 0, c->new_color)) {
                c->is_open = false;
                c->current_color = c->new_color;
                c->bound_color = 0;
            }
            ImGui::PopID();
        }
        ImGui::End();
        ImGui::PopStyleVar(3);

        ImGui::PushStyleVar  (ImGuiStyleVar_WindowPadding, ImVec2(0, 8));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, colors[PapayaCol_Clear]);

        ImGui::SetNextWindowSize(c->size - Vec2(0.0f, 34.0f));
        ImGui::SetNextWindowPos (c->pos  + Vec2(0.0f, 34.0f));
        ImGui::PushStyleVar     (ImGuiStyleVar_WindowRounding, 0);
        ImGui::Begin("Color picker", 0, layout.default_imgui_flags);
        {
            ImGui::BeginChild("HSV Gradients", Vec2(315, 258));
            ImGui::EndChild();
            int32 rgbColor[3];
            rgbColor[0] = (int32)(c->new_color.r * 255.0f);
            rgbColor[1] = (int32)(c->new_color.g * 255.0f);
            rgbColor[2] = (int32)(c->new_color.b * 255.0f);
            if (ImGui::InputInt3("RGB", rgbColor)) {
                int32 r = math::clamp(rgbColor[0], 0, 255);
                int32 g = math::clamp(rgbColor[1], 0, 255);
                int32 b = math::clamp(rgbColor[2], 0, 255);
                c->new_color = Color(r, g, b);
                math::rgb_to_hsv(c->new_color.r, c->new_color.g, c->new_color.b,
                               c->cursor_h, c->cursor_sv.x, c->cursor_sv.y);
            }
            char hexColor[6 + 1]; // null-terminated
            snprintf(hexColor, sizeof(hexColor), "%02x%02x%02x",
                     (int32)(c->new_color.r * 255.0f),
                     (int32)(c->new_color.g * 255.0f),
                     (int32)(c->new_color.b * 255.0f));
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
                c->new_color = Color(r, g, b);
                math::rgb_to_hsv(c->new_color.r, c->new_color.g, c->new_color.b,
                               c->cursor_h, c->cursor_sv.x, c->cursor_sv.y);
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    // Update hue picker
    {
        Vec2 pos = c->pos + c->hue_strip_pos;

        if (mouse.pressed[0] &&
            mouse.pos.x > pos.x &&
            mouse.pos.x < pos.x + c->hue_strip_size.x &&
            mouse.pos.y > pos.y &&
            mouse.pos.y < pos.y + c->hue_strip_size.y) {
            c->dragging_hue = true;
        }
        else if (mouse.released[0] && c->dragging_hue) {
            c->dragging_hue = false;
        }

        if (c->dragging_hue) {
            c->cursor_h = (mouse.pos.y - pos.y) / 256.0f;
            c->cursor_h = 1.f - math::clamp(c->cursor_h, 0.0f, 1.0f);
        }

    }

    // Update saturation-value picker
    {
        Vec2 pos = c->pos + c->sv_box_pos;

        //TODO: Create a rect structure
        if (mouse.pressed[0] &&
            mouse.pos.x > pos.x &&
            mouse.pos.x < pos.x + c->sv_box_size.x &&
            mouse.pos.y > pos.y &&
            mouse.pos.y < pos.y + c->sv_box_size.y) {
            c->dragging_sv = true;
        }
        else if (mouse.released[0] && c->dragging_sv) {
            c->dragging_sv = false;
        }

        if (c->dragging_sv) {
            c->cursor_sv.x = (mouse.pos.x - pos.x) / 256.f;
            c->cursor_sv.y = 1.f - (mouse.pos.y - pos.y) / 256.f;
            c->cursor_sv.x = math::clamp(c->cursor_sv.x, 0.f, 1.f);
            c->cursor_sv.y = math::clamp(c->cursor_sv.y, 0.f, 1.f);

        }
    }

    // Update new color
    {
        float r, g, b;
        math::hsv_to_rgb(c->cursor_h, c->cursor_sv.x, c->cursor_sv.y,
                       r, g, b);
        // Note: Rounding is essential. Without it, RGB->HSV->RGB is a lossy
        // operation.
        c->new_color = Color(math::round_to_int(r * 255.0f),
                                  math::round_to_int(g * 255.0f),
                                  math::round_to_int(b * 255.0f));
    }

    if (c->bound_color) { *c->bound_color = c->new_color; }
}

