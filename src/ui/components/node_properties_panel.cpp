#include "components/node_properties_panel.h"

#include "libpapaya.h"
#include "ui.h"
#include "components/graph_panel.h"


void draw_node_properties_panel(PapayaMemory* mem, Vec2 pos, Vec2 sz)
{
    PapayaNode* n = &mem->doc->nodes[mem->graph_panel->cur_node];

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(sz);

    ImGui::Begin("Node properties", 0, mem->window.default_imgui_flags);

    bool* a = (bool*)((char*)&n->is_active);
    ImGui::Checkbox(n->name, a);
    ImGui::Separator();

    switch (n->type) {
        case PapayaNodeType_Bitmap: {

        } break;

        case PapayaNodeType_InvertColor: {
            bool* r = (bool*)((char*)&n->params.invert_color.invert_r);
            bool* g = (bool*)((char*)&n->params.invert_color.invert_g);
            bool* b = (bool*)((char*)&n->params.invert_color.invert_b);

            if (ImGui::Checkbox("Invert red channel", r)) {
                core::update_canvas(mem);
            } else if (ImGui::Checkbox("Invert green channel", g)) {
                core::update_canvas(mem);
            } else if (ImGui::Checkbox("Invert blue channel", b)) {
                core::update_canvas(mem);
            }
        } break;
    }

    ImGui::End();
}
