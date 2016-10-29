
#include "core/graph_panel.h"

#include <math.h>

#include "papaya_core.h"
#include "libs/types.h"
#include "libs/imgui/imgui.h"
#include "libs/mathlib.h"

// NOTE: Most of this file is heavily work-in-progress at this point

void init_graph_panel(GraphPanel* g) {
    g->scroll_pos = Vec2(0,0);
    g->node_properties_panel_height = 200.0f;
    g->width = 300.0f;
}

void show_graph_panel(PapayaMemory* mem)
{
    static int node_selected = 0;

    GraphPanel* g = &mem->graph_panel;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Vec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, Vec2(5, 2));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, mem->colors[PapayaCol_Clear]);

    float x = mem->window.width - 36 - g->width; // Panel X
    float y = 55; // Panel Y
    float w = g->width; // Panel width
    float h = g->node_properties_panel_height; // Panel width

    // -------------------------------------------------------------------------
#if 1
    ImGui::SetNextWindowPos(Vec2(x, y));
    ImGui::SetNextWindowSize(Vec2(w, h));

    ImGui::Begin("Node properties", 0, mem->window.default_imgui_flags);

    ImGui::Checkbox(mem->cur_doc->nodes[node_selected]->name,
                    &mem->cur_doc->nodes[node_selected]->is_active);
    ImGui::Text("Node properties controls go here");

    ImGui::End();
#endif
    // -------------------------------------------------------------------------

    y += g->node_properties_panel_height;
    h = mem->window.height - 58.0f - h;
    ImGui::SetNextWindowPos(Vec2(x, y));
    ImGui::SetNextWindowSize(Vec2(w, h));

    ImGui::Begin("Nodes", 0, mem->window.default_imgui_flags);

    // bool open_context_menu = false;
    int node_hovered_in_list = -1;
    int node_hovered_in_scene = -1;

    ImGui::BeginGroup();

    const float NODE_SLOT_RADIUS = 4.0f;
    const Vec2 NODE_WINDOW_PADDING(2.0f, 2.0f);

    // Create our child canvas
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, Vec2(1,1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Vec2(0,0));
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImColor(60,60,70,200));
    ImGui::BeginChild("scrolling_region", Vec2(0,0), true, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove);
    ImGui::PushItemWidth(120.0f);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(2);

    // Draw the grid
    {
        ImU32 col = ImColor(200,200,200,40);
        float grid_sz = 50.0f;
        Vec2 win_pos = ImGui::GetCursorScreenPos();
        Vec2 win_sz = ImGui::GetWindowSize();

        // Vertical lines
        float x = fmodf(g->scroll_pos.x, grid_sz);
        while (x < win_sz.x) {
            draw_list->AddLine(win_pos + Vec2(x, 0),
                               win_pos + Vec2(x, win_sz.y), col);
            x += grid_sz;
        }

        // Horizontal lines
        float y = fmodf(g->scroll_pos.y, grid_sz);
        while (y < win_sz.y) {
            draw_list->AddLine(win_pos + Vec2(0, y),
                               win_pos + Vec2(win_sz.x, y), col);
            y += grid_sz;
        };
    }

    // Display links
    Vec2 offset = ImGui::GetCursorScreenPos() + mem->graph_panel.scroll_pos;
    draw_list->ChannelsSetCurrent(0); // Background
    for (int link_idx = 0; link_idx < mem->cur_doc->link_count; link_idx++)
    {
        NodeLink* link = mem->cur_doc->links[link_idx];
        Node* node_inp = mem->cur_doc->nodes[link->input_idx];
        Node* node_out = mem->cur_doc->nodes[link->output_idx];
        Vec2 p1 = offset + node_inp->GetOutputSlotPos(link->input_slot);
        Vec2 p2 = offset + node_out->GetInputSlotPos(link->output_slot);
        draw_list->AddBezierCurve(p1, p1+Vec2(0,-10), p2+Vec2(0,+10), p2, ImColor(200,200,100), 3.0f);
    }

    // Display nodes
    for (int node_idx = 0; node_idx < mem->cur_doc->node_count; node_idx++)
    {
        Node* node = mem->cur_doc->nodes[node_idx];
        ImGui::PushID(node->id);
        Vec2 node_rect_min = offset + node->pos;

        // Display node contents first
        draw_list->ChannelsSetCurrent(1); // Foreground
        bool old_any_active = ImGui::IsAnyItemActive();
        ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
        ImGui::BeginGroup(); // Lock horizontal position

        // TODO: OPTIMIZE: Use mipmaps here.
        // TODO: Unstretch aspect ratio
        ImGui::Image((void*)(intptr_t)node->tex_id,
                     Vec2(31,31),
                     Vec2(0,0), Vec2(1,1));

        ImGui::EndGroup();

        // Save the size of what we have emitted and whether any of the widgets are being used
        bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
        node->size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
        Vec2 node_rect_max = node_rect_min + node->size;

        // Display node box
        draw_list->ChannelsSetCurrent(0); // Background
        ImGui::SetCursorScreenPos(node_rect_min);
        ImGui::InvisibleButton("node", node->size);
        if (ImGui::IsItemHovered()) {
            node_hovered_in_scene = node->id;
            // open_context_menu |= ImGui::IsMouseClicked(1);
        }
        bool node_moving_active = ImGui::IsItemActive();
        if (node_widgets_active || node_moving_active)
            node_selected = node->id;
        if (node_moving_active && ImGui::IsMouseDragging(0))
            node->pos = node->pos + ImGui::GetIO().MouseDelta;

        ImU32 node_bg_color = (node_hovered_in_list == node->id || node_hovered_in_scene == node->id || (node_hovered_in_list == -1 && node_selected == node->id)) ? ImColor(75,75,75) : ImColor(60,60,60);
        draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, 4.0f); 
        draw_list->AddRect(node_rect_min, node_rect_max, ImColor(100,100,100), 4.0f); 
        for (int slot_idx = 0; slot_idx < node->inputs_count; slot_idx++)
            draw_list->AddCircleFilled(offset + node->GetInputSlotPos(slot_idx), NODE_SLOT_RADIUS, ImColor(150,150,150,150));
        for (int slot_idx = 0; slot_idx < node->outputs_count; slot_idx++)
            draw_list->AddCircleFilled(offset + node->GetOutputSlotPos(slot_idx), NODE_SLOT_RADIUS, ImColor(150,150,150,150));

        ImGui::PopID();
    }

    draw_list->ChannelsMerge();

    // Open context menu
    /*if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() && ImGui::IsMouseClicked(1))
    {
        node_selected = node_hovered_in_list = node_hovered_in_scene = -1;
        open_context_menu = true;
    }
    if (open_context_menu)
    {
        ImGui::OpenPopup("context_menu");
        if (node_hovered_in_list != -1)
            node_selected = node_hovered_in_list;
        if (node_hovered_in_scene != -1)
            node_selected = node_hovered_in_scene;
    }

    // Draw context menu
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Vec2(8,8));
    if (ImGui::BeginPopup("context_menu"))
    {
        Node* node = node_selected != -1 ? &mem->cur_doc->nodes[node_selected] : NULL;
        Vec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
        if (node)
        {
            ImGui::Text("Node '%s'", node->name);
            ImGui::Separator();
            if (ImGui::MenuItem("Rename..", NULL, false, false)) {}
            if (ImGui::MenuItem("Delete", NULL, false, false)) {}
            if (ImGui::MenuItem("Copy", NULL, false, false)) {}
        }
        else
        {
            if (ImGui::MenuItem("Add")) {
                mem->cur_doc->nodes.push_back(Node(mem->cur_doc->nodes.Size,
                                              "New node", scene_pos,
                                              NodeType_Raster));
            }
            if (ImGui::MenuItem("Paste", NULL, false, false)) {}
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();*/

    // Scrolling
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(2, 0.0f))
        mem->graph_panel.scroll_pos += ImGui::GetIO().MouseDelta;

    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::EndGroup();

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}