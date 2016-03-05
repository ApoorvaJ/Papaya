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

    void ShowPanel(PapayaMemory* Mem)
    {
        float Width = 400.0f;
        ImGui::SetNextWindowPos(ImVec2((float)Mem->Window.Width - 36 - Width, 58));
        ImGui::SetNextWindowSize(ImVec2(Width, (float)Mem->Window.Height - 64));

        // TODO: Once the overall look has been established, make commonly used templates
        ImGuiWindowFlags WindowFlags = 0;
        WindowFlags |= ImGuiWindowFlags_NoTitleBar;
        WindowFlags |= ImGuiWindowFlags_NoResize;
        WindowFlags |= ImGuiWindowFlags_NoMove;
        WindowFlags |= ImGuiWindowFlags_NoScrollbar;
        WindowFlags |= ImGuiWindowFlags_NoCollapse;
        WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding , 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding  , ImVec2(5 , 5));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding   , ImVec2(2 , 1));

        ImGui::PushStyleColor(ImGuiCol_Button           , Mem->Colors[PapayaCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered    , Mem->Colors[PapayaCol_ButtonHover]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive     , Mem->Colors[PapayaCol_ButtonActive]);
        ImGui::PushStyleColor(ImGuiCol_WindowBg         , Mem->Colors[PapayaCol_Clear]);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive , Mem->Colors[PapayaCol_ButtonActive]);

        ImGui::Begin("Preferences Window", 0, WindowFlags);

        // Title and search box
        {
            char buf[512] = {0};
            ImGui::InputText("Search", buf, 512);
        }

        ImGui::Separator();

        // Grid
        {
            ImGui::BeginChild("A", ImVec2(100, 0), false);
            const char* categories[]   = { "General", "Appearance", "Memory"};
            static int currentCategory = 1; // TODO: Remove static
            ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
            // TODO: Listbox doesn't seem to have a way to set the left margin.
            //       Bug? Am I missing something? Fixed in later version of ImGui?
            ImGui::ListBox("", &currentCategory, categories, IM_ARRAYSIZE(categories), 10);
            ImGui::PopItemWidth();
            ImGui::EndChild();
            ImGui::SameLine();

            ImGui::BeginChild("B", ImVec2(ImGui::GetContentRegionAvailWidth(), 0), false);

            if (currentCategory == 1) // Appearance
            {
                float col[3];
                col[0] = Mem->Colors[PapayaCol_Clear].r;
                col[1] = Mem->Colors[PapayaCol_Clear].g;
                col[2] = Mem->Colors[PapayaCol_Clear].b;
                ImGui::ColorEdit3("Background color", col);
                
                Mem->Colors[PapayaCol_Clear].r = col[0];
                Mem->Colors[PapayaCol_Clear].g = col[1];
                Mem->Colors[PapayaCol_Clear].b = col[2];
            }
            ImGui::EndChild();
        }

        ImGui::End();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(5);
    }
}

