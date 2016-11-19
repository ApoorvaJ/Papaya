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

enum PapayaSlotPos_ {
    PapayaSlotPos_Custom,
    PapayaSlotPos_In,
    PapayaSlotPos_Out,
    PapayaSlotPos_InMask
};


struct PapayaSlot {
    uint8_t is_out; // Boolean to differentiate between input vs output slots
    PapayaSlotType_ type;
    PapayaNode* node; // Node that this slot belongs to

    /*
        The X, Y coordinates of the Slot. (0,0) represents the top-left corner
        of the node and (1,1) represents the bottom right. Values may be greater
        than 1.0 or smaller than 0.0 to represent positions outside the slot.
    */
    float pos_x, pos_y;
    PapayaSlotPos_ pos; // Enum used to denote commonly used slot positions

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
    uint8_t* image;
    int64_t width, height;
};

void init_bitmap_node(PapayaNode* node, char* name,
                      uint8_t* img, int w, int h, int c);

// -----------------------------------------------------------------------------

struct InvertColorNode {
    int foo;
};

void init_invert_color_node(PapayaNode* node, char* name);

// -----------------------------------------------------------------------------

struct PapayaNode {
    PapayaNodeType_ type;
    char* name;
    float pos_x, pos_y;
    uint8_t is_active;

    PapayaSlot* slots;
    size_t num_slots;

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
