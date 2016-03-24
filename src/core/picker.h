
#ifndef PICKER_H
#define PICKER_H

struct PickerInfo
{
    bool Open;
    Color CurrentColor, NewColor;
    Color* BoundColor; // This color is changed along with CurrentColor. Zero if no color is bound.
    Vec2 CursorSV;
    float CursorH;

    Vec2 Pos, Size, HueStripPos, HueStripSize, SVBoxPos, SVBoxSize;
    bool DraggingHue, DraggingSV;
};

namespace Picker
{
    void Init(PickerInfo* Picker);
    void SetColor(Color Col, PickerInfo* Picker, bool SetNewColorOnly = false);
    void Show(PickerInfo* Picker, Color* Colors, uint32 BlankTexture);
}
#endif // PICKER_H

// =======================================================================================

#ifdef PICKER_IMPLEMENTATION

void Picker::Init(PickerInfo* Picker)
{
    Picker->CurrentColor = Color(220, 163, 89);
    Picker->Open         = false;
    Picker->Pos          = Vec2(34, 120);
    Picker->Size         = Vec2(292, 331);
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
}

void Picker::Show(PickerInfo* Picker, Color* Colors, uint32 BlankTexture) // TODO: Remove the last param
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
        int32 col[3];
        col[0] = (int)(Picker->NewColor.r * 255.0f);
        col[1] = (int)(Picker->NewColor.g * 255.0f);
        col[2] = (int)(Picker->NewColor.b * 255.0f);
        if (ImGui::InputInt3("RGB", col))
        {
            Picker->NewColor = Color(col[0], col[1], col[2]); // TODO: Clamping
            Math::RGBtoHSV(Picker->NewColor.r, Picker->NewColor.g, Picker->NewColor.b,
                    Picker->CursorH, Picker->CursorSV.x, Picker->CursorSV.y);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    if (Picker->BoundColor) { *Picker->BoundColor = Picker->NewColor; }
}

#endif // PICKER_IMPLEMENTATION
