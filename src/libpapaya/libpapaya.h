#pragma once

#include <stdint.h>

enum PapayaNodeType_ {
    PapayaNodeType_Bitmap,
    PapayaNodeType_InvertColor
};

// -----------------------------------------------------------------------------

struct PapayaNode;

// -----------------------------------------------------------------------------

struct BitmapNode {
    PapayaNode* in;
    PapayaNode* in_mask;
    PapayaNode* out;
    int64_t out_len;
    
    uint8_t* image;
    int64_t width, height;
};

void init_bitmap_node(PapayaNode* node, char* name,
                      uint8_t* img, int w, int h, int c);

// -----------------------------------------------------------------------------

struct InvertColorNode {
    PapayaNode* in;
    PapayaNode* in_mask;
    PapayaNode* out;
    int64_t out_len;
};

void init_invert_color_node(PapayaNode* node, char* name);

// -----------------------------------------------------------------------------

struct PapayaNode {
    int type;
    char* name;

    union {
        BitmapNode bitmap;
        InvertColorNode invert_color;
    } params;
};

// -----------------------------------------------------------------------------

void papaya_evaluate_node(PapayaNode* node, int w, int h, uint8_t* out);
