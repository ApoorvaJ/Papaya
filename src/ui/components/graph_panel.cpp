
#include "components/graph_panel.h"

#include <math.h>

#include "ui.h"
#include "components/node_properties_panel.h"
#include "libs/types.h"
#include "libs/imgui/imgui.h"
#include "libs/mathlib.h"
#include "libpapaya.h"

const Vec2 node_sz = Vec2(36, 36);
const f32 slot_radius = 4.0f;

GraphPanel* init_graph_panel()
{
    GraphPanel* g = (GraphPanel*) calloc(sizeof(*g), 1);
    g->scroll_pos = Vec2(0,0);
    g->node_properties_panel_height = 200.0f;
    g->width = 300.0f;
    g->cur_node = 0;
    g->dragged_slot = 0;
    g->displaced_slot = 0;
    return g;
}

void destroy_graph_panel(GraphPanel* g)
{
    free(g);
}

static Vec2 get_slot_pos(PapayaSlot* slot)
{
    return Vec2(slot->node->pos_x, slot->node->pos_y)
         + Vec2(node_sz.x * slot->pos_x,
                node_sz.y * slot->pos_y);
}

static PapayaSlot* find_snapped_slot(PapayaMemory* mem, Vec2 offset)
{
    Vec2 m = mem->mouse.pos;
    PapayaNode* snapped_node = 0; // Snapped node

    // Find the node that is currently being moused over
    for (int i = 0; i < mem->doc->num_nodes; i++) {
        PapayaNode* n = &mem->doc->nodes[i];

        Vec2 p = offset + Vec2(n->pos_x, n->pos_y);
        if (m.x >= p.x &&
            m.y >= p.y &&
            m.x <= p.x + node_sz.x &&
            m.y <= p.y + node_sz.y) {
            snapped_node = n;
            break;
        }
    }

    if (snapped_node) {
        PapayaSlot* min_slot = 0;
        f32 min_dist = FLT_MAX;

        // Find the closest slot to the mouse in the snapped node
        for (int j = 0; j < snapped_node->num_slots; j++) {
            PapayaSlot* s = &snapped_node->slots[j];
            f32 dist = math::distance(offset + get_slot_pos(s), m);
            // TODO: Snappability check goes here, to verify the absence of
            // cycles and the compatibility of slots
            if (min_dist > dist) {
                min_dist = dist;
                min_slot = s;
            }
        }

        if (min_slot) {
            // Compatible slot exists. Snap to it.
            return min_slot;
        }
    }

    return 0;
}

static Vec2 get_bezier_bend(PapayaSlot* s)
{
    switch (s->pos){
        case PapayaSlotPos_In:     return Vec2(0,+10);
        case PapayaSlotPos_Out:    return Vec2(0,-10);
        case PapayaSlotPos_InMask: return Vec2(+20,0);
        default:                   return Vec2();
    }
}

/*
    Draws a bezier curve between the two given slots. If a slot is null, then
    the mouse position is used instead of the slot position.
*/
static void draw_link(PapayaSlot* a, PapayaSlot* b, PapayaMemory* mem,
                      Vec2 offset, ImDrawList* draw_list)
{
    Vec2 v1 = a ? offset + get_slot_pos(a) : mem->mouse.pos; // Positions
    Vec2 v2 = b ? offset + get_slot_pos(b) : mem->mouse.pos; //
    Vec2 u1 = a ? get_bezier_bend(a) : Vec2(); // Bends
    Vec2 u2 = b ? get_bezier_bend(b) : Vec2(); //

    draw_list->AddBezierCurve(v1,
                              v1 + u1,
                              v2 + u2,
                              v2,
                              ImColor(220, 163,89, 150),
                              4.0f); // Link thickness
}

static void draw_nodes(PapayaMemory* mem)
{
    GraphPanel* g = mem->graph_panel;
    Vec2 offset = ImGui::GetCursorScreenPos() + g->scroll_pos;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    int hovered_node = -1;

    for (int i = 0; i < mem->doc->num_nodes; i++) {
        PapayaNode* n = &mem->doc->nodes[i];
        Vec2 pos = offset + Vec2(n->pos_x - 1, n->pos_y - 1);

        ImGui::PushID(i);

        // Slots
        for (int j = 0; j < n->num_slots; j++) {
            PapayaSlot* s = &n->slots[j];
            Vec2 pos = offset + Vec2(n->pos_x, n->pos_y)
                              + Vec2(node_sz.x * s->pos_x,
                                     node_sz.y * s->pos_y);
            Vec2 dia = Vec2(2.0f * slot_radius, 2.0f * slot_radius);
            ImGui::PushID(j);
            ImGui::SetCursorScreenPos(pos - (dia * 0.5f));
            ImGui::InvisibleButton("slot", dia);
            draw_list->AddCircleFilled(pos, slot_radius,
                                       (ImGui::IsItemHovered() ?
                                        ImColor(255,150,150,255) :
                                        ImColor(150,150,150,150)));

            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
                if (!s->to[0] || s->is_out) {
                    g->dragged_slot = s;
                } else {
                    g->displaced_slot = s;
                    g->dragged_slot = s->to[0];
                }
            }
            ImGui::PopID();
        }

        draw_list->ChannelsSetCurrent(0);
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("node", node_sz);
        if (ImGui::IsItemHovered()) {
            hovered_node = i;
        }

        if (ImGui::IsItemActive()) {
            if (g->cur_node != i) {
                g->cur_node = i;
                core::update_canvas(mem);
            }
            if (ImGui::IsMouseDragging(0)) {
                n->pos_x += ImGui::GetIO().MouseDelta.x;
                n->pos_y += ImGui::GetIO().MouseDelta.y;
            }
        }

#if 0
        // Node debug info
        {
            ImGui::SetCursorScreenPos(pos + Vec2(node_sz.x + 5,0));
            int in = -1;
            int out = -1;
            if (n->in) {
                in = n->in - mem->doc->nodes;
            }
            if (n->out) {
                out = n->out - mem->doc->nodes;
            }
            // ImGui::Text("in: %d\nout: %d", in, out);
            ImGui::Text("%s", n->name);
        }
#endif

        // Thumbnail
        draw_list->ChannelsSetCurrent(1);
        ImGui::SetCursorScreenPos(pos);
        ImGui::BeginGroup(); // Lock horizontal position
        {
            ImColor c = (hovered_node == i ||
                         g->cur_node == i) ?
                ImColor(220,163,89, 150) : ImColor(60,60,60);
            // TODO: Optimization: Use mipmaps here.
            // TODO: Unstretch aspect ratio
            ImGui::Image(0, node_sz, Vec2(0,0), Vec2(1,1),
                         ImVec4(1,1,1,1), c);
        }
        ImGui::EndGroup();

        // Incoming links
        draw_list->ChannelsSetCurrent(0);

        for (int j = 0; j < n->num_slots; j++) {

            PapayaSlot* t = &n->slots[j];
            if (t->is_out) {
                continue;
            }

            PapayaSlot* b = t->to[0];
            if (!b) {
                continue;
            }

            if (g->dragged_slot &&
                !g->dragged_slot->is_out &&
                g->dragged_slot->node == n) {
                // This link is being dragged. Skip the normal drawing and
                // handle this in the link interaction code
                continue;
            }

            if (g->displaced_slot &&
                g->displaced_slot->node == n) {
                // continue;
            } 

            draw_link(b, t, mem, offset, draw_list);
        }

        ImGui::PopID();
    }

    // Link interaction
    if (g->dragged_slot) {

        PapayaSlot* s = find_snapped_slot(mem, offset);

        draw_link(s, g->dragged_slot, mem, offset, draw_list);

        if (ImGui::IsMouseReleased(0)) {

            if (s) {
                if (g->displaced_slot) {
                    papaya_disconnect(g->displaced_slot, g->dragged_slot);
                }
                papaya_connect(g->dragged_slot, s);

            } else if (g->displaced_slot) {
                // Disconnection
                papaya_disconnect(g->displaced_slot, g->displaced_slot->to[0]);
            }

            g->dragged_slot = 0;
            g->displaced_slot = 0;

            core::update_canvas(mem);
        }

    }
}

void draw_graph_panel(PapayaMemory* mem)
{
    GraphPanel* g = mem->graph_panel;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Vec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, Vec2(5, 2));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, mem->colors[PapayaCol_Clear]);

    // TODO: Streamline this layout code
    f32 x = mem->window.width - 36 - g->width; // Panel X
    f32 y = 55; // Panel Y
    f32 w = g->width; // Panel width
    f32 h = g->node_properties_panel_height; // Panel height

    draw_node_properties_panel(mem, Vec2(x, y), Vec2(w, h));

    y += h;
    h = mem->window.height - 58.0f - h;
    ImGui::SetNextWindowPos(Vec2(x, y));
    ImGui::SetNextWindowSize(Vec2(w, h));

    ImGui::Begin("Nodes", 0, mem->window.default_imgui_flags);

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
        f32 grid_sz = 50.0f;
        Vec2 win_pos = ImGui::GetCursorScreenPos();
        Vec2 win_sz = ImGui::GetWindowSize();

        // Vertical lines
        f32 x = fmodf(g->scroll_pos.x, grid_sz);
        while (x < win_sz.x) {
            draw_list->AddLine(win_pos + Vec2(x, 0),
                               win_pos + Vec2(x, win_sz.y), col);
            x += grid_sz;
        }

        // Horizontal lines
        f32 y = fmodf(g->scroll_pos.y, grid_sz);
        while (y < win_sz.y) {
            draw_list->AddLine(win_pos + Vec2(0, y),
                               win_pos + Vec2(win_sz.x, y), col);
            y += grid_sz;
        };
    }

    // Display nodes
    draw_nodes(mem);
    draw_list->ChannelsMerge();

    // Scrolling
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(2, 0.0f))
        g->scroll_pos += ImGui::GetIO().MouseDelta;

    ImGui::PopItemWidth();
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::EndGroup();

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}
