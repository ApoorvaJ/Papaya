
#ifndef PREFS_H
#define PREFS_H

struct WindowInfo;

namespace Prefs
{
    void ShowPanel(PickerInfo* Picker, Color* Colors, WindowInfo& Window);
}

#endif // PREFS_H

// =======================================================================================

#ifdef PREFS_IMPLEMENTATION

void Prefs::ShowPanel(PickerInfo* Picker, Color* Colors, WindowInfo& Window)
{
    float Width = 400.0f;
    ImGui::SetNextWindowPos(ImVec2((float)Window.Width - 36 - Width, 58));
    ImGui::SetNextWindowSize(ImVec2(Width, (float)Window.Height - 64));

    // TODO: Once the overall look has been established, make commonly used templates
    ImGuiWindowFlags WindowFlags = 0;
    WindowFlags |= ImGuiWindowFlags_NoTitleBar;
    WindowFlags |= ImGuiWindowFlags_NoResize;
    WindowFlags |= ImGuiWindowFlags_NoMove;
    WindowFlags |= ImGuiWindowFlags_NoScrollbar;
    WindowFlags |= ImGuiWindowFlags_NoCollapse;
    WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding   , 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding    , ImVec2(5 , 5));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing , ImVec2(2 , 2));

    ImGui::PushStyleColor(ImGuiCol_Button           , Colors[PapayaCol_Button]);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered    , Colors[PapayaCol_ButtonHover]);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive     , Colors[PapayaCol_ButtonActive]);
    ImGui::PushStyleColor(ImGuiCol_WindowBg         , Colors[PapayaCol_Clear]);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive , Colors[PapayaCol_ButtonActive]);

    ImGui::Begin("Preferences Window", 0, WindowFlags);

    // char buf[512] = {0};
    // ImGui::InputText("Search", buf, 512);
    //
    // ImGui::Separator();

    // Grid
    {
        ImGui::BeginChild("A", ImVec2(100, 0), false);
        const char* categories[]   = { "General", "Appearance", "Memory"};
        static int currentCategory = 1; // TODO: Remove static
        ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
        // TODO: Listbox doesn't seem to have a way to set the left margin.
        //       Bug? Am I missing something? Fixed in later version of ImGui?
        ImGui::ListBox("", &currentCategory, categories, 3, 10);
        ImGui::PopItemWidth();
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("B", ImVec2(ImGui::GetContentRegionAvailWidth(), 0), false);

        if (currentCategory == 1) // Appearance
        {
            const char* colorNames[] = { "Window color",
                "Workspace color",
                "Button color",
                "Button hover color",
                "Button pressed color",
                "Alpha grid color 1",
                "Alpha grid color 2" };
            for (int32 i = PapayaCol_Clear; i <= PapayaCol_AlphaGrid2; i++)
            {
                char str[8];
                Color col = Colors[i];
                snprintf(str, 8, "#%02x%02x%02x", 
                        (uint32)(col.r * 255.0),
                        (uint32)(col.g * 255.0),
                        (uint32)(col.b * 255.0));

                ImGui::PushStyleColor(ImGuiCol_Button       , col);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive , col);
                ImGui::PushID(i);
                if (ImGui::Button(str))
                {
                    if (Picker->BoundColor == &Colors[i]) // This color is bound
                    {
                        Picker->BoundColor = 0;
                        Picker->Open = false;
                    }
                    else // Some other color is bound or picker is unbound
                    {
                        Picker->BoundColor = &Colors[i];
                        Picker::SetColor(Colors[i], Picker);
                        Picker->Open = true;
                    }
                }
                ImGui::PopID();
                ImGui::PopStyleColor(2);

                ImGui::SameLine();
                ImGui::Text(colorNames[i]);
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(5);
}

#endif // PREFS_IMPLEMENTATION
