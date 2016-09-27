
#include "node.h"
#include "papaya_core.h"
#include "libs/gl_util.h"

void node::init(Node* node, int id, char* name, Vec2 pos, uint8* img,
                PapayaMemory* mem)
{
    node->id = id;
    strncpy(node->name, name, 31);
    node->name[31] = 0;
    node->pos = pos;

    node->type = NodeType_Bitmap;
    node->inputs_count = node->outputs_count = 1;
    node->is_active = true;

    // Allocate GPU texture
    node->texture_id = gl::allocate_tex(mem->doc.width, mem->doc.height, img);
}
