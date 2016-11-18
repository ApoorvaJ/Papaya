#include "libpapaya.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/*
    TODO:
    - Add destroy functions to nodes
*/

enum SlotPos_ {
    SlotPos_Custom,
    SlotPos_In,
    SlotPos_Out,
    SlotPos_InMask
};

/*
    Assumes that the slot struct is zeroed out. If it isn't, pointers may have
    garbage values.
*/
static void init_slot(PapayaSlot* slot, PapayaNode* node, uint8_t is_out,
                      SlotPos_ pos = SlotPos_Custom)
{
    slot->node = node;
    slot->is_out = is_out;

    switch (pos) {
        case SlotPos_In:     slot->pos_x = 0.5f; slot->pos_y = 1;    break;
        case SlotPos_Out:    slot->pos_x = 0.5f; slot->pos_y = 0;    break;
        case SlotPos_InMask: slot->pos_x = 1;    slot->pos_y = 0.5f; break;
        case SlotPos_Custom: break;
    }
}

// -----------------------------------------------------------------------------

void init_bitmap_node(PapayaNode* node, char* name,
                      uint8_t* img, int w, int h, int c)
{
    BitmapNode* b = &node->params.bitmap;

    node->num_slots = 2;
    node->slots = (PapayaSlot*) calloc(node->num_slots * sizeof(PapayaSlot), 1);
    init_slot(&node->slots[0], node, false, SlotPos_In);
    init_slot(&node->slots[1], node, true, SlotPos_Out);

    node->type = PapayaNodeType_Bitmap;
    node->name = name;

    b->image = img;
    b->width = w;
    b->height = h;

}

static void papaya_evaluate_bitmap_node(PapayaNode* node, int w, int h,
                                        uint8_t* out)
{
    PapayaSlot* from = node->slots[0].to[0];

    if (!from) {
        // No input
        memcpy(out, node->params.bitmap.image, 4 * w * h);
        return;
    }

    PapayaNode* in = from->node;
    papaya_evaluate_node(in, w, h, out);

    uint8_t* img = node->params.bitmap.image;

    // Code is extremely unoptimized. Only proof-of-concept for nailing down
    // the API.
    for (size_t i = 0; i < 4 * w * h; i += 4) {
        float r_d = out[i]   / 255.0f;
        float g_d = out[i+1] / 255.0f;
        float b_d = out[i+2] / 255.0f;
        float a_d = out[i+3] / 255.0f;

        float r_s = node->params.bitmap.image[i]   / 255.0f;
        float g_s = node->params.bitmap.image[i+1] / 255.0f;
        float b_s = node->params.bitmap.image[i+2] / 255.0f;
        float a_s = node->params.bitmap.image[i+3] / 255.0f;

        // Alpha
        float a_f = (a_s + a_d * (1.0f - a_s));
        int a = 255.0f * a_f;
        if (a < 0) { a = 0; }
        else if (a > 255) { a = 255; }
        out[i+3] = (uint8_t) a;

        if (a == 0) {
            out[i] = out[i+1] = out[i+2] = 0;
        } else {
            int r = 255.0f * (r_s*a_s + r_d*a_d*(1.0f-a_s)) / a_f;
            int g = 255.0f * (g_s*a_s + g_d*a_d*(1.0f-a_s)) / a_f;
            int b = 255.0f * (b_s*a_s + b_d*a_d*(1.0f-a_s)) / a_f;
            out[i]   = (uint8_t) r;
            out[i+1] = (uint8_t) g;
            out[i+2] = (uint8_t) b;
        }
    }
}

// -----------------------------------------------------------------------------

void init_invert_color_node(PapayaNode* node, char* name)
{
    InvertColorNode* i = &node->params.invert_color;

    node->num_slots = 2;
    node->slots = (PapayaSlot*) calloc(node->num_slots * sizeof(PapayaSlot), 1);
    init_slot(&node->slots[0], node, false, SlotPos_In);
    init_slot(&node->slots[1], node, true, SlotPos_Out);

    node->type = PapayaNodeType_InvertColor;
    node->name = name;
}

static void papaya_evaluate_invert_color_node(PapayaNode* node, int w, int h,
                                              uint8_t* out)
{
    PapayaSlot* from = node->slots[0].to[0];
    if (!from) {
        return;
    }

    PapayaNode* in = from->node;

    papaya_evaluate_node(in, w, h, out);

    for (size_t i = 0; i < 4 * w * h; i += 4) {
        out[i]   = 255 - out[i];
        out[i+1] = 255 - out[i+1];
        out[i+2] = 255 - out[i+2];
    }
}

// -----------------------------------------------------------------------------

void papaya_evaluate_node(PapayaNode* node, int w, int h, uint8_t* out)
{
    switch (node->type) {
        case PapayaNodeType_Bitmap:
            papaya_evaluate_bitmap_node(node, w, h, out);
            break;
        case PapayaNodeType_InvertColor:
            papaya_evaluate_invert_color_node(node, w, h, out);
            break;
    }
}

bool papaya_connect(PapayaSlot* s1, PapayaSlot* s2)
{
    PapayaSlot* out, *in;
    if (s1->is_out) {
        out = s1;
        in = s2;
    } else {
        out = s2;
        in = s1;
    }

    if (in) {
        if (in->to[0]) {
            if (in->to[0]->node == out->node) {
                // in and out are already connected
                return true;
            }
            papaya_disconnect(in->to[0], in);
        }
        in->to[0] = out;
    }

    for (int32_t i = 0; i < 16; i++) {
        if (out->to[i] == 0) {
            out->to[i] = in;
            break;
        }
    }

    return true;
}

void papaya_disconnect(PapayaSlot* s1, PapayaSlot* s2)
{
    PapayaSlot* out, *in;
    if (s1->is_out) {
        out = s1;
        in = s2;
    } else {
        out = s2;
        in = s1;
    }

    for (int32_t i = 0; i < 16; i++) {
        if (out->to[i] == in) {
            out->to[i] = 0;
            break; // Assumption: only one such connection exists
        }
    }

    if (in->to[0] == out) {
        in->to[0] = 0;
    }
}
