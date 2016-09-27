#pragma once

#include "libs/types.h"
#include "string.h"

enum NodeType_ {
    NodeType_Raster
};

struct Node {
    uint32 id;
    NodeType_ type;
    char name[32];
    bool is_active;

    Vec2 pos, size;
    int inputs_count, outputs_count;

    uint32 texture_id;
    
    // TODO: Change these member functions to C-style functions
    Node(int id_, const char* name_, const Vec2& pos_, NodeType_ type_) {
        id = id_;
        strncpy(name, name_, 31);
        name[31] = 0;
        pos = pos_;
        inputs_count = outputs_count = 1;
        is_active = true;
    }

    Vec2 GetInputSlotPos(int slot_no) const {
        return Vec2(pos.x + size.x * ((float)slot_no+1) / ((float)inputs_count+1),
                    pos.y + size.y);
    }

    Vec2 GetOutputSlotPos(int slot_no) const {
        return Vec2(pos.x + size.x * ((float)slot_no+1) / ((float)outputs_count+1),
                    pos.y);
    }
};

struct NodeLink {
    int input_idx, output_idx;
    int input_slot, output_slot;

    // TODO: Change this member functions to a C-style function
    NodeLink(int input_idx_, int input_slot_, int output_idx_, int output_slot_) {
        input_idx = input_idx_;
        input_slot = input_slot_;
        output_idx = output_idx_;
        output_slot = output_slot_;
    }
};

namespace node {
    void init(Node* node);
}
