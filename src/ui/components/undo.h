#pragma once

#include "libs/types.h"

struct PapayaMemory;
struct Profile;

enum PapayaUndoOp_ {
    PapayaUndoOp_Brush,
    PapayaUndoOp_COUNT
};

struct UndoData {
    // TODO: Convert into a union of structs once multiple types of undo ops are required
    u8 op_code; // Stores enum of type PapayaUndoOp_
    UndoData* prev, *next;
    Vec2i pos, size; // Position and size of the suffixed data block
    bool IsSubRect; // If true, then the suffixed image data contains two subrects - before and after the brush
    Vec2 line_segment_start_uv;
    // Image data goes after this
};

struct UndoBuffer {
    void* start;   // Pointer to beginning of undo buffer memory block // TODO: Change pointer types to i8*?
    void* top;     // Pointer to the top of the undo stack
    UndoData* base;    // Pointer to the base of the undo stack
    UndoData* current; // Pointer to the current location in the undo stack. Goes back and forth during undo-redo.
    UndoData* last;    // Last undo data block in the buffer. Should be located just before Top.
    size_t size;  // Size of the undo buffer in bytes
    size_t count;  // Number of undo ops in buffer
    size_t current_index; // Index of the current undo data block from the beginning
};

namespace undo {
    void init(PapayaMemory* mem);
    void destroy(PapayaMemory* mem);
    void push(UndoBuffer* undo, Profile* profile, Vec2i pos, Vec2i size,
              i8* pre_brush_img, Vec2 line_segment_start_uv);
    void pop(PapayaMemory* mem, bool load_pre_brush_image);
}

