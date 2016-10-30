
#include "core/graph_panel.h"

#include <math.h>

#include "papaya_core.h"
#include "libs/types.h"
#include "libs/imgui/imgui.h"
#include "libs/mathlib.h"
#include "libpapaya.h"

// NOTE: Most of this file is heavily work-in-progress at this point

void init_graph_panel(GraphPanel* g) {
    g->scroll_pos = Vec2(0,0);
    g->node_properties_panel_height = 200.0f;
    g->width = 300.0f;
    g->cur_node = 0;
}

static void draw_nodes(PapayaMemory* mem)
{
    float node_radius = 4.0f;
    Vec2 offset = ImGui::GetCursorScreenPos() + mem->graph_panel.scroll_pos;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    int node_hovered_in_list = -1;
    int node_hovered_in_scene = -1;

    for (int i = 0; i < mem->doc->num_nodes; i++) {
        PapayaNode* n = &mem->doc->nodes[i];
        Vec2 pos = offset + Vec2(n->pos_x - 1, n->pos_y - 1);
        Vec2 sz = Vec2(36, 36);

        ImGui::PushID(i);

        // Thumbnail
        draw_list->ChannelsSetCurrent(1);
        ImGui::SetCursorScreenPos(pos);
        ImGui::BeginGroup(); // Lock horizontal position
        {
            ImColor c = (node_hovered_in_list == i || node_hovered_in_scene == i ||
                       (node_hovered_in_list == -1 &&
                        mem->graph_panel.cur_node == i)) ?
                ImColor(220,163,89, 150) : ImColor(60,60,60);
            // TODO: Optimization: Use mipmaps here.
            // TODO: Unstretch aspect ratio
            ImGui::Image(0, sz, Vec2(0,0), Vec2(1,1),
                         ImVec4(1,1,1,1), c);
        }
        ImGui::EndGroup();


        draw_list->ChannelsSetCurrent(0);
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("node", sz);
        if (ImGui::IsItemHovered()) {
            node_hovered_in_scene = i;
        }
        bool node_moving_active = ImGui::IsItemActive();
        if (node_moving_active)
            mem->graph_panel.cur_node = i;
        if (node_moving_active && ImGui::IsMouseDragging(0)) {
            n->pos_x += ImGui::GetIO().MouseDelta.x;
            n->pos_y += ImGui::GetIO().MouseDelta.y;
        }

        // Slots
        Vec2 v1, v2; // Output, input slot positions
        {
            v1 = offset + Vec2(n->pos_x + (sz.x * 0.5f), n->pos_y);
            v2 = v1 + Vec2(0, sz.y);
            ImColor c = ImColor(150,150,150,150);

            // draw_list->AddCircleFilled(v1, node_radius, c); // Input
            // draw_list->AddCircleFilled(v2, node_radius, c); // Output
        }

        // Output links
        {
            draw_list->ChannelsSetCurrent(0); // Background

            for (int j = 0; j < n->num_out; j++) {
                Vec2 v3 = offset + Vec2(n->out[j].pos_x,
                                        n->out[j].pos_y)
                                 + Vec2(sz.x * 0.5f, sz.y);
                draw_list->AddBezierCurve(v1,
                                          v1 + Vec2(0,-10),
                                          v3 + Vec2(0,+10),
                                          v3,
                                          ImColor(220, 163,89, 150),
                                          4.0f); // Link thickness
            }
        }

        ImGui::PopID();
    }
}

void draw_graph_panel(PapayaMemory* mem)
{
    GraphPanel* g = &mem->graph_panel;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Vec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, Vec2(5, 2));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, mem->colors[PapayaCol_Clear]);

    float x = mem->window.width - 36 - g->width; // Panel X
    float y = 55; // Panel Y
    float w = g->width; // Panel width
    float h = g->node_properties_panel_height; // Panel height

    // -------------------------------------------------------------------------
#if 1
    ImGui::SetNextWindowPos(Vec2(x, y));
    ImGui::SetNextWindowSize(Vec2(w, h));

    ImGui::Begin("Node properties", 0, mem->window.default_imgui_flags);

    ImGui::Checkbox(mem->doc->nodes[g->cur_node].name,
                    (bool*)&mem->doc->nodes[g->cur_node].is_active);
    ImGui::Text("Node properties controls go here");

    ImGui::End();
#endif
    // -------------------------------------------------------------------------

    y += h;
    h = mem->window.height - 58.0f - h;
    ImGui::SetNextWindowPos(Vec2(x, y));
    ImGui::SetNextWindowSize(Vec2(w, h));

    ImGui::Begin("Nodes", 0, mem->window.default_imgui_flags);

    // bool open_context_menu = false;
    int node_hovered_in_list = -1;
    int node_hovered_in_scene = -1;

    ImGui::BeginGroup();

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

    // Display nodes
    draw_nodes(mem);
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