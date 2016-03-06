namespace Picker
{
    void Init(PapayaMemory* Mem)
    {
        Mem->Picker.CurrentColor = Color(220, 163, 89);
        Mem->Picker.Open         = false;
        Mem->Picker.Pos          = Vec2(34, 120);
        Mem->Picker.Size         = Vec2(292, 331);
        Mem->Picker.HueStripPos  = Vec2(259, 42);
        Mem->Picker.HueStripSize = Vec2(30, 256);
        Mem->Picker.SVBoxPos     = Vec2(0, 42);
        Mem->Picker.SVBoxSize    = Vec2(256, 256);
        Mem->Picker.CursorSV     = Vec2(0.5f, 0.5f);
        Mem->Picker.BoundColor   = 0;
    }

    void SetColor(Color Col, PapayaMemory* Mem)
    {
        Mem->Picker.CurrentColor = Col;
        Mem->Picker.NewColor = Mem->Picker.CurrentColor;
        Math::RGBtoHSV(Mem->Picker.NewColor.r, Mem->Picker.NewColor.g, Mem->Picker.NewColor.b,
                       Mem->Picker.CursorH, Mem->Picker.CursorSV.x, Mem->Picker.CursorSV.y);
    }

    void Show(PapayaMemory* Mem)
    {
        // TODO: Clean styling code
        ImGui::SetNextWindowSize(ImVec2(Mem->Picker.Size.x, 35));
        ImGui::SetNextWindowPos (Mem->Picker.Pos);

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

        ImGui::PushStyleColor(ImGuiCol_Button        , Mem->Colors[PapayaCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered , Mem->Colors[PapayaCol_ButtonHover]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive  , Mem->Colors[PapayaCol_ButtonActive]);
        ImGui::PushStyleColor(ImGuiCol_WindowBg      , Mem->Colors[PapayaCol_Transparent]);

        ImGui::Begin("Color preview", 0, WindowFlags);
        {
            float Width1 = (Mem->Picker.Size.x + 33.0f) / 2.0f;
            float Width2 = Width1 - 33.0f;
            if (ImGui::ImageButton((void*)(intptr_t)Mem->Textures[PapayaTex_UI], ImVec2(Width2, 34), ImVec2(0, 0), ImVec2(0, 0), 0, Mem->Picker.CurrentColor))
            {
                Mem->Picker.Open = false;
                Mem->Picker.BoundColor = 0;
            }
            ImGui::SameLine();
            ImGui::PushID(1);
            if (ImGui::ImageButton((void*)(intptr_t)Mem->Textures[PapayaTex_UI], ImVec2(Width1, 34), ImVec2(0, 0), ImVec2(0, 0), 0, Mem->Picker.NewColor))
            {
                Mem->Picker.Open = false;
                Mem->Picker.CurrentColor = Mem->Picker.NewColor;
                Mem->Picker.BoundColor = 0;
            }
            ImGui::PopID();
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        ImGui::PushStyleVar  (ImGuiStyleVar_WindowPadding, ImVec2(0, 8));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, Mem->Colors[PapayaCol_Clear]);

        ImGui::SetNextWindowSize(Mem->Picker.Size - Vec2(0.0f, 34.0f));
        ImGui::SetNextWindowPos (Mem->Picker.Pos  + Vec2(0.0f, 34.0f));
        ImGui::PushStyleVar     (ImGuiStyleVar_WindowRounding, 0);
        ImGui::Begin("Color picker", 0, WindowFlags);
        {
            ImGui::BeginChild("HSV Gradients", Vec2(315, 258));
            ImGui::EndChild();
            int32 col[3];
            col[0] = (int)(Mem->Picker.NewColor.r * 255.0f);
            col[1] = (int)(Mem->Picker.NewColor.g * 255.0f);
            col[2] = (int)(Mem->Picker.NewColor.b * 255.0f);
            if (ImGui::InputInt3("RGB", col))
            {
                Mem->Picker.NewColor = Color(col[0], col[1], col[2]); // TODO: Clamping
                Math::RGBtoHSV(Mem->Picker.NewColor.r, Mem->Picker.NewColor.g, Mem->Picker.NewColor.b,
                               Mem->Picker.CursorH, Mem->Picker.CursorSV.x, Mem->Picker.CursorSV.y);
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);

        if (Mem->Picker.BoundColor) { *Mem->Picker.BoundColor = Mem->Picker.NewColor; }
    }
}

