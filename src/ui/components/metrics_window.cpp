
#include "metrics_window.h"
#include "ui.h"
#include "libs/imgui/imgui.h"
#include <inttypes.h>

void metrics_window::update(PapayaMemory* mem)
{
    if (!mem->misc.show_metrics) { return; }
    
    ImGui::Begin("Metrics");

    // ========
    // Profiler
    // ========
    if (ImGui::CollapsingHeader("Profiler", 0, true, true)) {
        ImGui::Columns(3, "profilercolumns");
        ImGui::Separator();
        ImGui::Text("Name");                                ImGui::NextColumn();
        ImGui::Text("Cycles");                              ImGui::NextColumn();
        ImGui::Text("Millisecs");                           ImGui::NextColumn();
        ImGui::Separator();

        for (i32 i = 0; i < Timer_COUNT; i++) {
            ImGui::Text("%s", get_timer_name(i));           ImGui::NextColumn();
            ImGui::Text("%" PRIu64,
                timers[i].elapsed_cycles);                  ImGui::NextColumn();
            ImGui::Text("%f", timers[i].elapsed_ms);        ImGui::NextColumn();
        }

        ImGui::Columns(1);
        ImGui::Separator();
    }

    // =====
    // Input
    // =====
    if (ImGui::CollapsingHeader("Input", 0, true, true)) {
        ImGui::Separator();

        // Keyboard
        // --------
        ImGui::Text("Keyboard");
        ImGui::Columns(2, "inputcolumns");
        ImGui::Separator();
        ImGui::Text("Shift");                               ImGui::NextColumn();
        ImGui::Text("%d", mem->keyboard.shift);             ImGui::NextColumn();
        ImGui::Text("Ctrl");                                ImGui::NextColumn();
        ImGui::Text("%d", mem->keyboard.ctrl);              ImGui::NextColumn();
        ImGui::Text("Z");                                   ImGui::NextColumn();
        ImGui::Text("%d", ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)));              ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();

        // Tablet
        // ------
        ImGui::Text("Tablet");
        ImGui::Columns(2, "inputcolumns");
        ImGui::Separator();
        ImGui::Text("PosX");                                ImGui::NextColumn();
        ImGui::Text("%d", mem->tablet.pos.x);               ImGui::NextColumn();
        ImGui::Text("PosY");                                ImGui::NextColumn();
        ImGui::Text("%d", mem->tablet.pos.y);               ImGui::NextColumn();
        ImGui::Text("Pressure");                            ImGui::NextColumn();
        ImGui::Text("%f", mem->tablet.pressure);            ImGui::NextColumn();
        ImGui::Text("Pen touch");                           ImGui::NextColumn();
        ImGui::Text((mem->tablet.buttons & EasyTab_Buttons_Pen_Touch) ?
                    "1" : "0");                             ImGui::NextColumn();
        ImGui::Text("Pen lower");                           ImGui::NextColumn();
        ImGui::Text((mem->tablet.buttons & EasyTab_Buttons_Pen_Lower) ?
                    "1" : "0");                             ImGui::NextColumn();
        ImGui::Text("Pen upper");                           ImGui::NextColumn();
        ImGui::Text((mem->tablet.buttons & EasyTab_Buttons_Pen_Upper) ?
                    "1" : "0");                             ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();

        // Mouse
        // ------
        ImGui::Text("Mouse");
        ImGui::Columns(2, "inputcolumns");
        ImGui::Separator();
        ImGui::Text("PosX");                                ImGui::NextColumn();
        ImGui::Text("%d", mem->mouse.pos.x);                ImGui::NextColumn();
        ImGui::Text("PosY");                                ImGui::NextColumn();
        ImGui::Text("%d", mem->mouse.pos.y);                ImGui::NextColumn();
        ImGui::Text("Buttons");                             ImGui::NextColumn();
        ImGui::Text("%d %d %d",
            mem->mouse.is_down[0],
            mem->mouse.is_down[1],
            mem->mouse.is_down[2]);                         ImGui::NextColumn();
        ImGui::Columns(1);
        ImGui::Separator();
    }
    ImGui::End();
}
