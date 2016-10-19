#include "libpapaya.h"

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
