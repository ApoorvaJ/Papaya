#pragma once

#include "libs/types.h"

struct PapayaMemory;

struct GraphPanel {
    Vec2 scroll_pos;
    float node_properties_panel_height;
    float width;
    int cur_node; // Index of current node
    int dragged_node; // Index of the node whose slot is being dragged
    bool dragged_is_output; // Whether that slot is an output slot
};

void init_graph_panel(GraphPanel* g);
void draw_graph_panel(PapayaMemory* mem);
