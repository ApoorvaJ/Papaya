#include "libs/imgui/imgui.h"

struct PapayaMemory;

// Dummy
struct Node
{
    int     ID;
    char    Name[32];
    ImVec2  Pos, Size;
    float   Value;
    ImVec4  Color;
    int     InputsCount, OutputsCount;

    Node(int id, const char* name, const ImVec2& pos, float value, const ImVec4& color, int inputs_count, int outputs_count) { ID = id; strncpy(Name, name, 31); Name[31] = 0; Pos = pos; Value = value; Color = color; InputsCount = inputs_count; OutputsCount = outputs_count; }

    ImVec2 GetInputSlotPos(int slot_no) const {
        return ImVec2(Pos.x + Size.x * ((float)slot_no+1) / ((float)InputsCount+1),
                      Pos.y + Size.y);
    }

    ImVec2 GetOutputSlotPos(int slot_no) const {
        return ImVec2(Pos.x + Size.x * ((float)slot_no+1) / ((float)OutputsCount+1),
                      Pos.y);
    }
};
struct NodeLink
{
    int     InputIdx, InputSlot, OutputIdx, OutputSlot;

    NodeLink(int input_idx, int input_slot, int output_idx, int output_slot) { InputIdx = input_idx; InputSlot = input_slot; OutputIdx = output_idx; OutputSlot = output_slot; }
};

namespace nodes_window{
    void show_panel(PapayaMemory* mem);
}
