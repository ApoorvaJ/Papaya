#pragma once

#include <stdint.h>

enum PapayaNodeType_ {
    PapayaNodeType_Bitmap,
    PapayaNodeType_HueSat
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

struct HueSatNode {
    PapayaNode* in;
    PapayaNode* in_mask;
    PapayaNode* out;
    int64_t out_len;

    float hue;
    float sat;
};

// -----------------------------------------------------------------------------

struct PapayaNode {
    int type;
    char* name;

    union {
        BitmapNode bitmap;
        HueSatNode huesat;
    } params;
};

// -----------------------------------------------------------------------------

void papaya_evaluate_node(PapayaNode* node, int w, int h, uint8_t* out);
