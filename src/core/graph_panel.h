#pragma once

#include "libs/types.h"

struct PapayaMemory;

struct GraphPanel {
    Vec2 scroll_pos;
    float node_properties_panel_height;
    float width;
};

void init_graph_panel(GraphPanel* g);
void show_graph_panel(PapayaMemory* mem);
