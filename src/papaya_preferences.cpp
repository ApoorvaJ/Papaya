namespace Prefs
{
    void Init(PapayaPref* Prefs, int32 Count)
    {

    }

    void LoadFromFile(PapayaPref* Prefs, int32 Count)
    {

    }

    void WriteToFile(PapayaPref* Prefs, int32 Count)
    {

    }

    void ShowWindow(PapayaMemory* Mem)
    {
        ImGui::SetNextWindowSize(ImVec2((float)Mem->Window.Width  - 61, 
                                        (float)Mem->Window.Height - 58));
        ImGui::SetNextWindowPos(ImVec2(34, 55));

        ImGuiWindowFlags WindowFlags = 0;
        WindowFlags |= ImGuiWindowFlags_NoTitleBar;
        WindowFlags |= ImGuiWindowFlags_NoResize;
        WindowFlags |= ImGuiWindowFlags_NoMove;
        WindowFlags |= ImGuiWindowFlags_NoScrollbar;
        WindowFlags |= ImGuiWindowFlags_NoCollapse;
        WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse; // TODO: Once the overall look has been established, make commonly used templates

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding , 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding  , ImVec2( 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing    , ImVec2(30, 0));

        ImGui::PushStyleColor(ImGuiCol_Button           , Mem->Colors[PapayaCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered    , Mem->Colors[PapayaCol_ButtonHover]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive     , Mem->Colors[PapayaCol_ButtonActive]);
        ImGui::PushStyleColor(ImGuiCol_WindowBg         , Mem->Colors[PapayaCol_Transparent]);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive , Mem->Colors[PapayaCol_ButtonActive]);

        ImGui::Begin("Preferences Window");
        if (ImGui::Button("OK"))
        {
            Mem->Misc.PrefsOpen = false;
        }
        ImGui::End();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(5);
    }
}

