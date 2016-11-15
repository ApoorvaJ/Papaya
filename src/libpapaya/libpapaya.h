#pragma once

/*
    TODO:
    - Consider moving away from C++ to pure C
*/

#include <stdint.h>
#include <stdlib.h>

struct PapayaNode;

enum PapayaNodeType_ {
    PapayaNodeType_Bitmap,
    PapayaNodeType_InvertColor
};

enum PapayaSlotType_ {
    PapayaSlotType_Raster,
    PapayaSlotType_Vector,
};

struct PapayaOutputSlot;

struct PapayaInputSlot {
    PapayaSlotType_ type;
    PapayaNode* node; // Node that this slot belongs to
    PapayaOutputSlot* from; // Output slot that this node is getting data
                            // from.
};

struct PapayaOutputSlot {
    PapayaSlotType_ type;
    PapayaNode* node; // Node that this slot belongs to
    PapayaInputSlot* to[16]; // Input slots that this node is sending data to.
                             // 0 if not connected.
                             // TODO: Move to dynamically sized array
};

// -----------------------------------------------------------------------------

struct BitmapNode {
    PapayaInputSlot in;
    PapayaOutputSlot out;

    uint8_t* image;
    int64_t width, height;
};

void init_bitmap_node(PapayaNode* node, char* name,
                      uint8_t* img, int w, int h, int c);

// -----------------------------------------------------------------------------

struct InvertColorNode {
    PapayaInputSlot in;
    PapayaOutputSlot out;

    int foo;
};

void init_invert_color_node(PapayaNode* node, char* name);

// -----------------------------------------------------------------------------

struct PapayaNode {
    int type;
    char* name;
    float pos_x, pos_y;
    uint8_t is_active;

    union {
        BitmapNode bitmap;
        InvertColorNode invert_color;
    } params;
};

struct PapayaDocument {
    PapayaNode* nodes;
    size_t num_nodes;
};

// -----------------------------------------------------------------------------

void papaya_evaluate_node(PapayaNode* node, int w, int h, uint8_t* out);
bool papaya_connect(PapayaOutputSlot* out, PapayaInputSlot* in);
void papaya_disconnect(PapayaOutputSlot* out, PapayaInputSlot* in);
