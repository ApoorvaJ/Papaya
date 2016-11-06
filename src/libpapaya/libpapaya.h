#pragma once

#include <stdint.h>
#include <stdlib.h>

enum PapayaNodeType_ {
    PapayaNodeType_Bitmap,
    PapayaNodeType_InvertColor
};

// -----------------------------------------------------------------------------

struct PapayaNode;

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
    int type;
    char* name;
    float pos_x, pos_y;
    uint8_t is_active;

    PapayaNode* in;
    PapayaNode* in_mask;
    PapayaNode* out;

    PapayaNode** outs;
    size_t num_outs;
    size_t size_outs;

    union {
        BitmapNode bitmap;
        InvertColorNode invert_color;
    } params;
};

struct PapayaDocument {
    // Consumer-facing
    PapayaNode* nodes;
    size_t num_nodes;

    // Editor-facing
};

// -----------------------------------------------------------------------------

void papaya_evaluate_node(PapayaNode* node, int w, int h, uint8_t* out);
bool papaya_connect_nodes(PapayaNode* n1, PapayaNode* n2);