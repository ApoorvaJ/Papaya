#include "core/nodes_window.h"
#include <math.h>
#include"papaya_core.h"

// NOTE: Most of this file is heavily work-in-progress at this point

// TODO: Remove
static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x+rhs.x, lhs.y+rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x-rhs.x, lhs.y-rhs.y); }

// TODO: Really dumb data structure provided for the example.
//       Note that we storing links are INDICES (not ID) to make example code shorter, obviously a bad idea for any general purpose code.
void nodes_window::show_panel(PapayaMemory* mem)
{
    static ImVector<NodeLink> links;
    static bool inited = false;
    static ImVec2 scrolling = ImVec2(35.0f, -325.0f); // TODO: Automate this positioning
    static bool show_grid = true;
    static int node_selected = 0;
    static bool enabled_node = true;

    float node_props_height = 200.0f;
    float width = 300.0f;

    if (!inited)
    {
        mem->doc.nodes.push_back(Node(0, "L1", Vec2(50,100), NodeType_Raster));
        mem->doc.nodes.push_back(Node(1, "L2", Vec2(100,100), NodeType_Raster));
        mem->doc.nodes.push_back(Node(2, "L3", Vec2(50,50), NodeType_Raster));
        links.push_back(NodeLink(0, 0, 2, 0));
        links.push_back(NodeLink(1, 0, 2, 1));
        inited = true;
    }


    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(5, 2));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, mem->colors[PapayaCol_Clear]);

    // -------------------------------------------------------------------------

    ImGui::SetNextWindowPos(Vec2((float)mem->window.width - 36 - width, 55));
    ImGui::SetNextWindowSize(Vec2(width, node_props_height));

    ImGui::Begin("Node properties", 0, mem->window.default_imgui_flags);

    ImGui::Checkbox(mem->doc.nodes[node_selected].name,
                    &mem->doc.nodes[node_selected].is_active);
    ImGui::Text("Node properties controls go here");

    ImGui::End();

    // -------------------------------------------------------------------------

    ImGui::SetNextWindowPos(
        Vec2((float)mem->window.width - 36 - width, 55 + node_props_height));
    ImGui::SetNextWindowSize(
        Vec2(width, (float)mem->window.height - 58.0f - node_props_height));

    ImGui::Begin("Nodes", 0, mem->window.default_imgui_flags);

    bool open_context_menu = false;
    int node_hovered_in_list = -1;
    int node_hovered_in_scene = -1;

    ImGui::BeginGroup();

    const float NODE_SLOT_RADIUS = 4.0f;
    const ImVec2 NODE_WINDOW_PADDING(8.0f, 8.0f);

    // Create our child canvas
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1,1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImColor(60,60,70,200));
    ImGui::BeginChild("scrolling_region", ImVec2(0,0), true, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove);
    ImGui::PushItemWidth(120.0f);

    ImVec2 offset = ImGui::GetCursorScreenPos() - scrolling; 
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(2);

    // Display grid
    {
        ImU32 GRID_COLOR = ImColor(200,200,200,40);
        float GRID_SZ = 50.0f;
        ImVec2 win_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_sz = ImGui::GetWindowSize();

        for (float x = fmodf(offset.x,GRID_SZ); x < canvas_sz.x; x += GRID_SZ) {
            float dummy_alignment_x = -1.0f; // TODO: Sooper temporary
            draw_list->AddLine(Vec2(x + dummy_alignment_x, 0.0f) + win_pos,
                               Vec2(x + dummy_alignment_x, canvas_sz.y) + win_pos,
                               GRID_COLOR);
        }

        for (float y = fmodf(offset.y,GRID_SZ); y < canvas_sz.y; y += GRID_SZ) {
            float dummy_alignment_y = 33.0f; // TODO: Sooper temporary
            draw_list->AddLine(Vec2(0.0f, y + dummy_alignment_y) + win_pos,
                               Vec2(canvas_sz.x, y + dummy_alignment_y) + win_pos,
                               GRID_COLOR);
        }
    }

    // Display links
    draw_list->ChannelsSetCurrent(0); // Background
    for (int link_idx = 0; link_idx < links.Size; link_idx++)
    {
        NodeLink* link = &links[link_idx];
        Node* node_inp = &mem->doc.nodes[link->InputIdx];
        Node* node_out = &mem->doc.nodes[link->OutputIdx];
        ImVec2 p1 = offset + node_inp->GetOutputSlotPos(link->InputSlot);
        ImVec2 p2 = offset + node_out->GetInputSlotPos(link->OutputSlot);		
        draw_list->AddBezierCurve(p1, p1+ImVec2(0,-10), p2+ImVec2(0,+10), p2, ImColor(200,200,100), 3.0f);
    }

    // Display nodes
    for (int node_idx = 0; node_idx < mem->doc.nodes.Size; node_idx++)
    {
        Node* node = &mem->doc.nodes[node_idx];
        ImGui::PushID(node->id);
        ImVec2 node_rect_min = offset + node->pos;

        // Display node contents first
        draw_list->ChannelsSetCurrent(1); // Foreground
        bool old_any_active = ImGui::IsAnyItemActive();
        ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);
        ImGui::BeginGroup(); // Lock horizontal position
        ImGui::Image((void*)(intptr_t)mem->textures[PapayaTex_UI],
                     Vec2(21,21),
                     Vec2(0,0), Vec2(0,0));
        // ImGui::Text("%s", node->Name);
        ImGui::EndGroup();

        // Save the size of what we have emitted and whether any of the widgets are being used
        bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
        node->size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
        Vec2 node_rect_max = node_rect_min + node->size;

        // Display node box
        draw_list->ChannelsSetCurrent(0); // Background
        ImGui::SetCursorScreenPos(node_rect_min);
        ImGui::InvisibleButton("node", node->size);
        if (ImGui::IsItemHovered())
        {
            node_hovered_in_scene = node->id;
            open_context_menu |= ImGui::IsMouseClicked(1);
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
    if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseHoveringWindow() && ImGui::IsMouseClicked(1))
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
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8,8));
    if (ImGui::BeginPopup("context_menu"))
    {
        Node* node = node_selected != -1 ? &mem->doc.nodes[node_selected] : NULL;
        ImVec2 scene_pos = ImGui::GetMousePosOnOpeningCurrentPopup() - offset;
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
                mem->doc.nodes.push_back(Node(mem->doc.nodes.Size,
                                              "New node", scene_pos,
                                              NodeType_Raster));
            }
            if (ImGui::MenuItem("Paste", NULL, false, false)) {}
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    // Scrolling
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(2, 0.0f))
        scrolling = scrolling - ImGui::GetIO().MouseDelta;

    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::EndGroup();

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}