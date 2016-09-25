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

    // TODO: Remove this
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

namespace node {
    void init(Node* node);
}

