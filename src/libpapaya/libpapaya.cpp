#include "libpapaya.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/*
    TODO:
    - Add destroy functions to nodes
*/

/*
    Assumes that the slot struct is zeroed out. If it isn't, pointers may have
    garbage values.
*/
static void init_slot(PapayaSlot* slot, PapayaNode* node, uint8_t is_out,
                      PapayaSlotPos_ pos = PapayaSlotPos_Custom)
{
    slot->node = node;
    slot->is_out = is_out;
    slot->pos = pos;

    switch (pos) {
        case PapayaSlotPos_In:     slot->pos_x = 0.5f; slot->pos_y = 1;    break;
        case PapayaSlotPos_Out:    slot->pos_x = 0.5f; slot->pos_y = 0;    break;
        case PapayaSlotPos_InMask: slot->pos_x = 1;    slot->pos_y = 0.5f; break;
        case PapayaSlotPos_Custom: break;
    }
}

// -----------------------------------------------------------------------------

void init_bitmap_node(PapayaNode* node, const char* name,
                      uint8_t* img, int w, int h, int c)
{
    BitmapNode* b = &node->params.bitmap;

    node->num_slots = 2;
    node->slots = (PapayaSlot*) calloc(node->num_slots * sizeof(PapayaSlot), 1);
    init_slot(&node->slots[0], node, false, PapayaSlotPos_In);
    init_slot(&node->slots[1], node, true, PapayaSlotPos_Out);

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

    // Code is extremely unoptimized. Only proof-of-concept for nailing down
    // the API.
    for (int64_t i = 0; i < 4 * w * h; i += 4) {
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

void init_invert_color_node(PapayaNode* node, const char* name)
{
    InvertColorNode* i = &node->params.invert_color;

    node->num_slots = 3;
    node->slots = (PapayaSlot*) calloc(node->num_slots * sizeof(PapayaSlot), 1);
    init_slot(&node->slots[0], node, false, PapayaSlotPos_In);
    init_slot(&node->slots[1], node, true, PapayaSlotPos_Out);
    init_slot(&node->slots[2], node, false, PapayaSlotPos_InMask);

    node->type = PapayaNodeType_InvertColor;
    node->name = name;
    i->invert_r = i->invert_g = i->invert_b = 1;
}

static void papaya_evaluate_invert_color_node(PapayaNode* node, int w, int h,
                                              uint8_t* out)
{
    PapayaSlot* in = node->slots[0].to[0];
    if (!in) {
        return;
    }
    papaya_evaluate_node(in->node, w, h, out);

    PapayaSlot* mask = node->slots[2].to[0];
    if (mask) {
        // Mask is provided
        uint8_t* m = (uint8_t*) malloc(4 * w * h);

        papaya_evaluate_node(mask->node, w, h, m);
        for (int64_t i = 0; i < 4 * w * h; i += 4) {
            float t = (float)m[i+3] / 255.0f;

            if (node->params.invert_color.invert_r) {
                float r = (float)out[i] / 255.0f;
                r = t * (1.0f - r) + (1.0f - t) * r;
                out[i]   = r * 255.0f;
            }

            if (node->params.invert_color.invert_g) {
                float g = (float)out[i+1] / 255.0f;
                g = t * (1.0f - g) + (1.0f - t) * g;
                out[i+1] = g * 255.0f;
            }

            if (node->params.invert_color.invert_b) {
                float b = (float)out[i+2] / 255.0f;
                b = t * (1.0f - b) + (1.0f - t) * b;
                out[i+2] = b * 255.0f;
            }
        }

        free(m);
    } else {
        // No mask provided
        for (int64_t i = 0; i < 4 * w * h; i += 4) {
            out[i]   = 255 - out[i];
            out[i+1] = 255 - out[i+1];
            out[i+2] = 255 - out[i+2];
        }
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
