#pragma once

#include "libs/types.h"

struct PapayaMemory;
struct PapayaSlot;

struct GraphPanel {
    Vec2 scroll_pos;
    float node_properties_panel_height;
    float width;
    int cur_node; // Index of current node
    PapayaSlot* dragged_slot;
    PapayaSlot* displaced_slot;
};

void init_graph_panel(GraphPanel* g);
void draw_graph_panel(PapayaMemory* mem);
