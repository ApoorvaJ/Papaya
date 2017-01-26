#pragma once

#include "libs/types.h"

struct PapayaMemory;
struct PaglMesh;
struct PaglProgram;

struct EyeDropper {
    PaglMesh* mesh;
    PaglProgram* pgm;
};

EyeDropper* init_eye_dropper(PapayaMemory* mem);
void destroy_eye_dropper(EyeDropper* e);
void update_and_render_eye_dropper(PapayaMemory* mem);
