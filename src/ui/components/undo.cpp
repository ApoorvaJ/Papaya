
// TODO: Will need a restructuring with node support. Won't work as-is.

#include "undo.h"
#include "libs/mathlib.h"
#include "libs/linmath.h"
#include "ui.h"
#include "pagl.h"
#include "gl_lite.h"
#include "brush.h"

void undo::init(PapayaMemory* mem)
{
    size_t size = 512 * 1024 * 1024;
    size_t min_size = 3 * (sizeof(UndoData) + 8 * mem->doc->canvas_size.x *
                           mem->doc->canvas_size.y);
    mem->doc->undo.size = math::max(size, min_size);

    mem->doc->undo.start = malloc((size_t)mem->doc->undo.size);
    mem->doc->undo.current_index = -1;

    // TODO: Near-duplicate code from brush release. Combine.
    // Additive render-to-texture
    {
        GLCHK( glDisable(GL_SCISSOR_TEST) );
        GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, mem->misc.fbo) );
        GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                      GL_TEXTURE_2D, mem->misc.fbo_render_tex,
                                      0) );

        GLCHK( glViewport(0, 0, mem->doc->canvas_size.x, mem->doc->canvas_size.y) );
        GLCHK( glUseProgram(mem->shaders[PapayaShader_ImGui]->id) );

        f32 proj_mtx[4][4];
        mat4x4_ortho(proj_mtx, 0.f,
                     mem->doc->canvas_size.x, 0.f,
                     mem->doc->canvas_size.y,
                     -1.f, 1.f);

        GLCHK( glUniformMatrix4fv(mem->shaders[PapayaShader_ImGui]->uniforms[0],
                                  1, GL_FALSE, &proj_mtx[0][0]) );

        GLCHK( glBindBuffer(GL_ARRAY_BUFFER, mem->brush->mesh_RTTAdd->vbo_handle) );
        pagl_set_vertex_attribs(mem->shaders[PapayaShader_ImGui]);

        // GLCHK( glBindTexture(GL_TEXTURE_2D,
        //                      (GLuint)(intptr_t)mem->doc->texture_id) );
        // GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );

        undo::push(&mem->doc->undo, &mem->profile,
                   Vec2i(0,0), Vec2i(mem->doc->canvas_size.x,
                                     mem->doc->canvas_size.y),
                   0, Vec2());

        // u32 temp = mem->misc.fbo_render_tex;
        // mem->misc.fbo_render_tex = mem->doc->texture_id;
        GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                      GL_TEXTURE_2D, mem->misc.fbo_render_tex,
                                      0) );
        // mem->doc->texture_id = temp;

        GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
        GLCHK( glViewport(0, 0, mem->doc->canvas_size.x, mem->doc->canvas_size.y) );

        GLCHK( glDisable(GL_BLEND) );
    }
}

void undo::destroy(PapayaMemory* mem)
{
    free(mem->doc->undo.start);
    mem->doc->undo.start = mem->doc->undo.top = 0;
    mem->doc->undo.base = mem->doc->undo.current = mem->doc->undo.last = 0;
    mem->doc->undo.size = mem->doc->undo.count = 0;
    mem->doc->undo.current_index = -1;
}

// This function reads from the frame buffer and hence needs the appropriate frame buffer to be
// bound before it is called.
void undo::push(UndoBuffer* undo, Profile* profile, Vec2i pos, Vec2i size,
                i8* pre_brush_img, Vec2 line_segment_start_uv)
{
    if (undo->top == 0) {
        // Buffer is empty
        undo->base = (UndoData*)undo->start;
        undo->top  = undo->start;
    } else if (undo->current->next != 0) {
        // Not empty and not at end. Reposition for overwrite.
        u64 bytes_to_right =
            (i8*)undo->start + undo->size - (i8*)undo->current;
        u64 img_size = (undo->current->IsSubRect ? 8 : 4) *
            undo->current->size.x * undo->current->size.y;
        u64 block_size = sizeof(UndoData) + img_size;
        if (bytes_to_right >= block_size) {
            undo->top = (i8*)undo->current + block_size;
        } else {
            undo->top = (i8*)undo->start + block_size - bytes_to_right;
        }
        undo->last = undo->current;
        undo->count = undo->current_index + 1;
    }

    UndoData data = {};
    data.op_code = PapayaUndoOp_Brush;
    data.prev = undo->last;
    data.pos = pos;
    data.size = size;
    data.IsSubRect = (pre_brush_img != 0);
    data.line_segment_start_uv = line_segment_start_uv;

    u64 buf_size = sizeof(UndoData) +
        size.x * size.y * (data.IsSubRect ? 8 : 4);
    void* buf = malloc((size_t)buf_size);

    timer::start(Timer_GetUndoImage);
    memcpy(buf, &data, sizeof(UndoData));
    GLCHK( glReadPixels(pos.x, pos.y, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, (i8*)buf + sizeof(UndoData)) );
    timer::stop(Timer_GetUndoImage);

    if (data.IsSubRect) {
        memcpy((i8*)buf + sizeof(UndoData) + 4 * size.x * size.y, pre_brush_img, 4 * size.x * size.y);
    }

    u64 bytes_to_right = (i8*)undo->start + undo->size - (i8*)undo->top;
    if (bytes_to_right < sizeof(UndoData)) // Not enough space for UndoData. Go to start.
    {
        // Reposition the base pointer
        while (((i8*)undo->base >= (i8*)undo->top ||
            (i8*)undo->base < (i8*)undo->start + buf_size) &&
            undo->count > 0)
        {
            undo->base = undo->base->next;
            undo->base->prev = 0;
            undo->count--;
            undo->current_index--;
        }

        undo->top = undo->start;
        memcpy(undo->top, buf, (size_t)buf_size);

        if (undo->last) { undo->last->next = (UndoData*)undo->top; }
        undo->last = (UndoData*)undo->top;
        undo->top = (i8*)undo->top + buf_size;
    }
    else if (bytes_to_right < buf_size) // Enough space for UndoData, but not for image. Split image data.
    {
        // Reposition the base pointer
        while (((i8*)undo->base >= (i8*)undo->top ||
            (i8*)undo->base  <  (i8*)undo->start + buf_size - bytes_to_right) &&
            undo->count > 0)
        {
            undo->base = undo->base->next;
            undo->base->prev = 0;
            undo->count--;
            undo->current_index--;
        }

        memcpy(undo->top, buf, (size_t)bytes_to_right);
        memcpy(undo->start, (i8*)buf + bytes_to_right, (size_t)(buf_size - bytes_to_right));

        if (bytes_to_right == 0) { undo->top = undo->start; }

        if (undo->last) { undo->last->next = (UndoData*)undo->top; }
        undo->last = (UndoData*)undo->top;
        undo->top = (i8*)undo->start + buf_size - bytes_to_right;
    }
    else // Enough space for everything. Simply append.
    {
        // Reposition the base pointer
        while ((i8*)undo->base >= (i8*)undo->top &&
            (i8*)undo->base < (i8*)undo->top + buf_size &&
            undo->count > 0)
        {
            undo->base = undo->base->next;
            undo->base->prev = 0;
            undo->count--;
            undo->current_index--;
        }

        memcpy(undo->top, buf, (size_t)buf_size);

        if (undo->last) { undo->last->next = (UndoData*)undo->top; }
        undo->last = (UndoData*)undo->top;
        undo->top = (i8*)undo->top + buf_size;
    }

    free(buf);

    undo->current = undo->last;
    undo->count++;
    undo->current_index++;
}

void undo::pop(PapayaMemory* mem, bool load_pre_brush_image)
{
    UndoBuffer* undo = &mem->doc->undo;
    UndoData data = {};
    i8* img = 0;
    bool alloc_used = false;

    memcpy(&data, undo->current, sizeof(UndoData));

    size_t bytes_to_right = (i8*)undo->start + undo->size - (i8*)undo->current;
    size_t img_size =
        (undo->current->IsSubRect ? 8 : 4) * data.size.x * data.size.y;
    if (bytes_to_right - sizeof(UndoData) >= img_size) {
        // Image is contiguously stored
        img = (i8*)undo->current + sizeof(UndoData);
    } else {
        // Image is split
        alloc_used = true;
        img = (i8*)malloc(img_size);
        memcpy(img,
               (i8*)undo->current + sizeof(UndoData), 
               (size_t)bytes_to_right - sizeof(UndoData));
        memcpy((i8*)img + bytes_to_right - sizeof(UndoData),
               undo->start,
               (size_t)(img_size - (bytes_to_right - sizeof(UndoData))));
    }

    // GLCHK( glBindTexture(GL_TEXTURE_2D, mem->doc->texture_id) );
    // GLCHK( glTexSubImage2D(GL_TEXTURE_2D, 0, data.pos.x, data.pos.y,
    //                        data.size.x, data.size.y, GL_RGBA, GL_UNSIGNED_BYTE, 
    //                        img + (load_pre_brush_image ? 
    //                               4 * data.size.x * data.size.y : 0)) );

    if (alloc_used) { free(img); }
}

void undo::visualize_undo_buffer(PapayaMemory* mem)
{
    ImGui::Begin("Undo buffer");

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Buffer line
    f32 width = ImGui::GetWindowSize().x;
    Vec2 Pos = ImGui::GetWindowPos();
    Vec2 P1 = Pos + Vec2(10, 40);
    Vec2 P2 = Pos + Vec2(width - 10, 40);
    draw_list->AddLine(P1, P2, 0xFFFFFFFF);

    // Base mark
    u64 BaseOffset = (i8*)mem->doc->undo.base - (i8*)mem->doc->undo.start;
    f32 BaseX       = P1.x + (f32)BaseOffset / (f32)mem->doc->undo.size * (P2.x - P1.x);
    draw_list->AddLine(Vec2(BaseX, Pos.y + 26), Vec2(BaseX,Pos.y + 54), 0xFFFFFF00);

    // Current mark
    u64 CurrOffset = (i8*)mem->doc->undo.current - (i8*)mem->doc->undo.start;
    f32 CurrX       = P1.x + (f32)CurrOffset / (f32)mem->doc->undo.size * (P2.x - P1.x);
    draw_list->AddLine(Vec2(CurrX, Pos.y + 29), Vec2(CurrX, Pos.y + 51), 0xFFFF00FF);

    // Top mark
    u64 TopOffset = (i8*)mem->doc->undo.top - (i8*)mem->doc->undo.start;
    f32 TopX       = P1.x + (f32)TopOffset / (f32)mem->doc->undo.size * (P2.x - P1.x);
    draw_list->AddLine(Vec2(TopX, Pos.y + 35), Vec2(TopX, Pos.y + 45), 0xFF00FFFF);

    ImGui::Text(" "); //
    ImGui::Text(" "); // Vertical spacers
    ImGui::TextColored(Color(0.0f,1.0f,1.0f,1.0f), "Base    %" PRIu64, BaseOffset);
    ImGui::TextColored(Color(1.0f,0.0f,1.0f,1.0f), "Current %" PRIu64, CurrOffset);
    ImGui::TextColored(Color(1.0f,1.0f,0.0f,1.0f), "Top     %" PRIu64, TopOffset);
    ImGui::Text("Count   %lu", mem->doc->undo.count);
    ImGui::Text("Index   %lu", mem->doc->undo.current_index);

    ImGui::End();
}
