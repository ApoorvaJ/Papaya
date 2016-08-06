
#include "prefs.h"
#include <stdio.h>
#include "libs/imgui/imgui.h"
#include "papaya_core.h"

void prefs::show_panel(Picker* picker, Color* colors, Layout& layout) {
    float width = 400.0f;
    ImGui::SetNextWindowPos(ImVec2((float)layout.width - 36 - width, 58));
    ImGui::SetNextWindowSize(ImVec2(width, (float)layout.height - 64));

    // TODO: Once the overall look has been established, make commonly used templates
    ImGuiWindowFlags WindowFlags = 0;
    WindowFlags |= ImGuiWindowFlags_NoTitleBar;
    WindowFlags |= ImGuiWindowFlags_NoResize;
    WindowFlags |= ImGuiWindowFlags_NoMove;
    WindowFlags |= ImGuiWindowFlags_NoScrollbar;
    WindowFlags |= ImGuiWindowFlags_NoCollapse;
    WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5 , 5));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(2 , 2));

    ImGui::PushStyleColor(ImGuiCol_Button, colors[PapayaCol_Button]);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors[PapayaCol_ButtonHover]);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors[PapayaCol_ButtonActive]);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, colors[PapayaCol_Clear]);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, colors[PapayaCol_ButtonActive]);

    ImGui::Begin("Preferences Window", 0, WindowFlags);

    // char buf[512] = {0};
    // ImGui::InputText("Search", buf, 512);
    //
    // ImGui::Separator();

    // Grid
    {
        ImGui::BeginChild("A", ImVec2(100, 0), false);
        const char* categories[]   = { "General", "Appearance", "Memory"};
        static int current_category = 1; // TODO: Remove static
        ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
        // TODO: Listbox doesn't seem to have a way to set the left margin.
        //       Bug? Am I missing something? Fixed in later version of ImGui?
        ImGui::ListBox("", &current_category, categories, 3, 10);
        ImGui::PopItemWidth();
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("B", Vec2(ImGui::GetContentRegionAvailWidth(), 0), false);

        if (current_category == 1) {
            // Appearance
            const char* colorNames[] = {
                "Window color",
                "Workspace color",
                "Button color",
                "Button hover color",
                "Button pressed color",
                "Alpha grid color 1",
                "Alpha grid color 2",
                "Image size preview color 1",
                "Image size preview color 2"
            };

            for (int32 i = PapayaCol_Clear; i <= PapayaCol_ImageSizePreview2; i++) {
                char str[8];
                Color col = colors[i];
                snprintf(str, 8, "#%02x%02x%02x", 
                        (uint32)(col.r * 255.0),
                        (uint32)(col.g * 255.0),
                        (uint32)(col.b * 255.0));

                ImGui::PushStyleColor(ImGuiCol_Button, col);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
                ImGui::PushID(i);
                if (ImGui::Button(str)) {
                    if (picker->bound_color == &colors[i]) {
                        // This color is bound
                        picker->bound_color = 0;
                        picker->is_open = false;
                    }
                    else {
                        // Some other color is bound or picker is unbound 
                        picker->bound_color = &colors[i];
                        picker::set_color(colors[i], picker);
                        picker->is_open = true;
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

