
#include "picker.h"
#include "libs/imgui/imgui.h"
#include "libs/mathlib.h"
#include "papaya_core.h"

void Picker::Init(PickerInfo* Picker)
{
    Picker->CurrentColor = Color(220, 163, 89);
    Picker->Open         = false;
    Picker->Pos          = Vec2(34, 120);
    Picker->Size         = Vec2(292, 350);
    Picker->HueStripPos  = Vec2(259, 42);
    Picker->HueStripSize = Vec2(30, 256);
    Picker->SVBoxPos     = Vec2(0, 42);
    Picker->SVBoxSize    = Vec2(256, 256);
    Picker->CursorSV     = Vec2(0.5f, 0.5f);
    Picker->BoundColor   = 0;
}

void Picker::SetColor(Color Col, PickerInfo* Picker, bool SetNewColorOnly)
{
    if (!SetNewColorOnly) { Picker->CurrentColor = Col; }
    Picker->NewColor = Col;
    Math::RGBtoHSV(Picker->NewColor.r, Picker->NewColor.g, Picker->NewColor.b,
            Picker->CursorH, Picker->CursorSV.x, Picker->CursorSV.y);
    if (Picker->BoundColor) { *Picker->BoundColor = Col; }
}

// TODO: Remove the last param
void Picker::UpdateAndRender(PickerInfo* Picker, Color* Colors, MouseInfo& Mouse,
        uint32 BlankTexture)
{
    // Picker panel
    {
        // TODO: Clean styling code
        ImGui::SetNextWindowSize(ImVec2(Picker->Size.x, 35));
        ImGui::SetNextWindowPos (Picker->Pos);

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

        ImGui::PushStyleColor(ImGuiCol_Button        , Colors[PapayaCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered , Colors[PapayaCol_ButtonHover]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive  , Colors[PapayaCol_ButtonActive]);
        ImGui::PushStyleColor(ImGuiCol_WindowBg      , Colors[PapayaCol_Transparent]);

        ImGui::Begin("Color preview", 0, WindowFlags);
        {
            float Width1 = (Picker->Size.x + 33.0f) / 2.0f;
            float Width2 = Width1 - 33.0f;
            if (ImGui::ImageButton((void*)(intptr_t)BlankTexture, ImVec2(Width2, 34), ImVec2(0, 0), ImVec2(0, 0), 0, Picker->CurrentColor))
            {
                Picker->Open = false;
                if (Picker->BoundColor) { *Picker->BoundColor = Picker->CurrentColor; }
                Picker->BoundColor = 0;
            }
            ImGui::SameLine();
            ImGui::PushID(1);
            if (ImGui::ImageButton((void*)(intptr_t)BlankTexture, ImVec2(Width1, 34), ImVec2(0, 0), ImVec2(0, 0), 0, Picker->NewColor))
            {
                Picker->Open = false;
                Picker->CurrentColor = Picker->NewColor;
                Picker->BoundColor = 0;
            }
            ImGui::PopID();
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        ImGui::PushStyleVar  (ImGuiStyleVar_WindowPadding, ImVec2(0, 8));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors[PapayaCol_Clear]);

        ImGui::SetNextWindowSize(Picker->Size - Vec2(0.0f, 34.0f));
        ImGui::SetNextWindowPos (Picker->Pos  + Vec2(0.0f, 34.0f));
        ImGui::PushStyleVar     (ImGuiStyleVar_WindowRounding, 0);
        ImGui::Begin("Color picker", 0, WindowFlags);
        {
            ImGui::BeginChild("HSV Gradients", Vec2(315, 258));
            ImGui::EndChild();
            int32 rgbColor[3];
            rgbColor[0] = (int32)(Picker->NewColor.r * 255.0f);
            rgbColor[1] = (int32)(Picker->NewColor.g * 255.0f);
            rgbColor[2] = (int32)(Picker->NewColor.b * 255.0f);
            if (ImGui::InputInt3("RGB", rgbColor))
            {
                int32 r = Math::Clamp(rgbColor[0], 0, 255);
                int32 g = Math::Clamp(rgbColor[1], 0, 255);
                int32 b = Math::Clamp(rgbColor[2], 0, 255);
                Picker->NewColor = Color(r, g, b);
                Math::RGBtoHSV(Picker->NewColor.r, Picker->NewColor.g, Picker->NewColor.b,
                        Picker->CursorH, Picker->CursorSV.x, Picker->CursorSV.y);
            }
            char hexColor[6 + 1]; // null-terminated
            snprintf(hexColor, sizeof(hexColor), "%02x%02x%02x",
                    (int32)(Picker->NewColor.r * 255.0f),
                    (int32)(Picker->NewColor.g * 255.0f),
                    (int32)(Picker->NewColor.b * 255.0f));
            if (ImGui::InputText("Hex", hexColor, sizeof(hexColor), ImGuiInputTextFlags_CharsHexadecimal))
            {
                int32 r = 0, g = 0, b = 0;
                switch (strlen(hexColor))
                {
                    case 1: sscanf(hexColor, "%1x", &b); break;
                    case 2: sscanf(hexColor, "%2x", &b); break;
                    case 3: sscanf(hexColor, "%1x%2x", &g, &b); break;
                    case 4: sscanf(hexColor, "%2x%2x", &g, &b); break;
                    case 5: sscanf(hexColor, "%1x%2x%2x", &r, &g, &b); break;
                    case 6: sscanf(hexColor, "%2x%2x%2x", &r, &g, &b); break;
                }
                Picker->NewColor = Color(r, g, b);
                Math::RGBtoHSV(Picker->NewColor.r, Picker->NewColor.g, Picker->NewColor.b,
                        Picker->CursorH, Picker->CursorSV.x, Picker->CursorSV.y);
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);
    }

    // Update hue picker
    {
        Vec2 Pos = Picker->Pos + Picker->HueStripPos;

        if (Mouse.Pressed[0] &&
                Mouse.Pos.x > Pos.x &&
                Mouse.Pos.x < Pos.x + Picker->HueStripSize.x &&
                Mouse.Pos.y > Pos.y &&
                Mouse.Pos.y < Pos.y + Picker->HueStripSize.y)
        {
            Picker->DraggingHue = true;
        }
        else if (Mouse.Released[0] && Picker->DraggingHue)
        {
            Picker->DraggingHue = false;
        }

        if (Picker->DraggingHue)
        {
            Picker->CursorH = 1.0f - (Mouse.Pos.y - Pos.y) / 256.0f;
            Picker->CursorH = Math::Clamp(Picker->CursorH, 0.0f, 1.0f);
        }

    }

    // Update saturation-value picker
    {
        Vec2 Pos = Picker->Pos + Picker->SVBoxPos;

        if (Mouse.Pressed[0] &&
                Mouse.Pos.x > Pos.x &&
                Mouse.Pos.x < Pos.x + Picker->SVBoxSize.x &&
                Mouse.Pos.y > Pos.y &&
                Mouse.Pos.y < Pos.y + Picker->SVBoxSize.y)
        {
            Picker->DraggingSV = true;
        }
        else if (Mouse.Released[0] && Picker->DraggingSV)
        {
            Picker->DraggingSV = false;
        }

        if (Picker->DraggingSV)
        {
            Picker->CursorSV.x =        (Mouse.Pos.x - Pos.x) / 256.0f;
            Picker->CursorSV.y = 1.0f - (Mouse.Pos.y - Pos.y) / 256.0f;
            Picker->CursorSV.x = Math::Clamp(Picker->CursorSV.x, 0.0f, 1.0f);
            Picker->CursorSV.y = Math::Clamp(Picker->CursorSV.y, 0.0f, 1.0f);

        }
    }

    // Update new color
    {
        float r, g, b;
        Math::HSVtoRGB(Picker->CursorH, Picker->CursorSV.x, Picker->CursorSV.y, r, g, b);
        Picker->NewColor = Color(Math::RoundToInt(r * 255.0f),  // Note: Rounding is essential.
                Math::RoundToInt(g * 255.0f),  //       Without it, RGB->HSV->RGB
                Math::RoundToInt(b * 255.0f)); //       is a lossy operation.
    }

    if (Picker->BoundColor) { *Picker->BoundColor = Picker->NewColor; }
}

