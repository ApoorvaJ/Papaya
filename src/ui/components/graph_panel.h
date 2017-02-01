#pragma once

#include "libs/types.h"

struct PapayaMemory;
struct PapayaSlot;

struct GraphPanel {
    Vec2 scroll_pos;
    f32 node_properties_panel_height;
    f32 width;
    size_t cur_node; // Index of current node
    PapayaSlot* dragged_slot;
    PapayaSlot* displaced_slot;
};

GraphPanel* init_graph_panel();
void destroy_graph_panel(GraphPanel* g);
void draw_graph_panel(PapayaMemory* mem);
