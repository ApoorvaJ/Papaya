#include "libpapaya.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>


void init_bitmap_node(PapayaNode* node, char* name,
                      uint8_t* img, int w, int h, int c)
{
    node->type = PapayaNodeType_Bitmap;
    node->name = name;

    node->params.bitmap.image = img;
    node->params.bitmap.width = w;
    node->params.bitmap.height = h;
}

static void papaya_evaluate_bitmap_node(PapayaNode* node, int w, int h,
                                        uint8_t* out)
{
    PapayaNode* in = node->in;
    if (!in) {
        // No input
        memcpy(out, node->params.bitmap.image, 4 * w * h);
    } else {
        // Input exists
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
}

// -----------------------------------------------------------------------------

void init_invert_color_node(PapayaNode* node, char* name)
{
    node->type = PapayaNodeType_InvertColor;
    node->name = name;
}

static void papaya_evaluate_invert_color_node(PapayaNode* node, int w, int h,
                                              uint8_t* out)
{
    PapayaNode* in = node->in;
    if (!in) { return; }

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
