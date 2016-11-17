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

struct PapayaSlot {
    uint8_t is_out; // Boolean to differentiate between input vs output slots
    PapayaSlotType_ type;
    PapayaNode* node; // Node that this slot belongs to

    /*
        Slot(s) that this slot is connected to. 0 if not connected.

        If this is an input slot, only one connection may exist, which will be
        stored in to[0]. If this is an output slot, then multiple connections
        may exist.

        TODO: Move to dynamically sized array
    */
    PapayaSlot* to[16]; 
};

// -----------------------------------------------------------------------------

struct BitmapNode {
    PapayaSlot in;
    PapayaSlot out;

    uint8_t* image;
    int64_t width, height;
};

void init_bitmap_node(PapayaNode* node, char* name,
                      uint8_t* img, int w, int h, int c);

// -----------------------------------------------------------------------------

struct InvertColorNode {
    PapayaSlot in;
    PapayaSlot out;

    int foo;
};

void init_invert_color_node(PapayaNode* node, char* name);

// -----------------------------------------------------------------------------

struct PapayaNode {
    PapayaNodeType_ type;
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
bool papaya_connect(PapayaSlot* out, PapayaSlot* in);
void papaya_disconnect(PapayaSlot* out, PapayaSlot* in);
