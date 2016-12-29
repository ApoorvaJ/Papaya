
#include "prefs.h"
#include <stdio.h>
#include "libs/imgui/imgui.h"
#include "ui.h"
#include "components/color_panel.h"

void prefs::show_panel(ColorPanel* color_panel, Color* colors, Layout& layout)
{
    f32 width = 400.0f;
    ImGui::SetNextWindowPos(ImVec2((f32)layout.width - 36 - width, 58));
    ImGui::SetNextWindowSize(ImVec2(width, (f32)layout.height - 64));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5 , 5));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(2 , 2));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, colors[PapayaCol_Clear]);

    ImGui::Begin("Preferences Window", 0, layout.default_imgui_flags);

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

            for (i32 i = PapayaCol_Clear; i <= PapayaCol_ImageSizePreview2; i++) {
                char str[8];
                Color col = colors[i];
                snprintf(str, 8, "#%02x%02x%02x", 
                        (u32)(col.r * 255.0),
                        (u32)(col.g * 255.0),
                        (u32)(col.b * 255.0));

                ImGui::PushStyleColor(ImGuiCol_Button, col);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
                ImGui::PushID(i);
                if (ImGui::Button(str)) {
                    if (color_panel->bound_color == &colors[i]) {
                        // This color is bound
                        color_panel->bound_color = 0;
                        color_panel->is_open = false;
                    }
                    else {
                        // Some other color is bound or picker is unbound 
                        color_panel->bound_color = &colors[i];
                        color_panel_set_color(colors[i], color_panel);
                        color_panel->is_open = true;
                    }
                }
                ImGui::PopID();
                ImGui::PopStyleColor(2);

                ImGui::SameLine();
                ImGui::Text("%s", colorNames[i]);
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);
}

