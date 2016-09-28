
#include "node.h"
#include "papaya_core.h"
#include "libs/gl_util.h"

void node::init(Node* node, char* name, Vec2 pos, uint8* img,
                PapayaMemory* mem)
{
    node->id = mem->cur_doc->next_node_id++;
    strncpy(node->name, name, 31);
    node->name[31] = 0;
    node->pos = pos;

    node->type = NodeType_Bitmap;
    node->inputs_count = node->outputs_count = 1;
    node->is_active = true;

    // Allocate GPU texture
    node->tex_id = gl::allocate_tex(mem->cur_doc->width, mem->cur_doc->height, img);
}

void node::destroy(Node* node)
{
    GLCHK( glDeleteTextures(1, &node->tex_id) );
    node->tex_id = 0;
}