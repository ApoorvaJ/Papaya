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

GraphPanel* init_graph_panel();
void draw_graph_panel(PapayaMemory* mem);
