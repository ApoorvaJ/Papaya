#pragma once

#include "libs/types.h"

struct PapayaMemory;

struct GraphPanel {
    Vec2 scroll_pos;
};


void show_graph_panel(PapayaMemory* mem);
void init_graph_panel(PapayaMemory* mem);
