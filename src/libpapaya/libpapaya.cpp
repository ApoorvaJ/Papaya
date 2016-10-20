#include "libpapaya.h"

#include <stdlib.h>
#include <string.h>

static PapayaBackend_ backend;

void papaya_init(PapayaBackend_ b)
{
    backend = b;
}

void init_bitmap_node(PapayaNode* node, char* name,
                      uint8_t* img, int w, int h, int c)
{
    node->type = PapayaNodeType_Bitmap;
    node->name = name;

    node->params.bitmap.image = img;
    node->params.bitmap.width = w;
    node->params.bitmap.height = h;
}

uint8_t* papaya_evaluate_node(PapayaNode* node, int* w, int* h)
{
    *w = *h = 16;
    size_t s = 4 * (*w) * (*h);
    uint8_t* img = (uint8_t*) malloc(s);
    for (int i = 0; i < 256; i++) {
        ((int*)img)[i] = 0xffff00ff;
    }
    // memset(img, 128, s);
    return img;
}
