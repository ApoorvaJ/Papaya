
#include "node.h"
#include "papaya_core.h"
#include "libs/gl_util.h"

Node* node::init(char* name, Vec2 pos, uint8* img, PapayaMemory* mem)
{
    Node* n = (Node*)malloc(sizeof(*n));
    n->id = mem->cur_doc->next_node_id++;
    strncpy(n->name, name, 31);
    n->name[31] = 0;
    n->pos = pos;

    n->type = NodeType_Bitmap;
    n->inputs_count = n->outputs_count = 0;
    n->is_active = true;

    // Allocate GPU texture
    n->tex_id = gl::allocate_tex(mem->cur_doc->width, mem->cur_doc->height, img);

    // Register in current doc
    mem->cur_doc->nodes[mem->cur_doc->node_count] = n; // TODO: Array bounds check
    mem->cur_doc->node_count++;

    return n;
}

void node::destroy(Node* node)
{
    GLCHK( glDeleteTextures(1, &node->tex_id) );
    node->tex_id = 0;
}

void node::connect(Node* from, Node* to, PapayaMemory* mem)
{
    from->outputs[from->outputs_count++] = to;
    to->inputs[to->inputs_count++] = from;

    // Find node indices in array
    int f = -1, t = -1;
    for (int i = 0; i < mem->cur_doc->node_count; i++) {
        if (from->id == mem->cur_doc->nodes[i]->id) { f = i; }
        if (to->id   == mem->cur_doc->nodes[i]->id) { t = i; }
        if (f >= 0 && t >= 0) { break; }
    }

    // Create link
    NodeLink* l = (NodeLink*)malloc(sizeof(*l));
    l->input_idx = f;
    l->output_idx = t;
    l->input_slot = l->output_slot = 0;
    mem->cur_doc->links[mem->cur_doc->link_count++] = l;

}