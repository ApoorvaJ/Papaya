
// TODO: Will need a restructuring with node support. Won't work as-is.

#include "undo.h"
#include "libs/mathlib.h"
#include "libs/gl_util.h"
#include "papaya_core.h"

void undo::init(PapayaMemory* mem)
{
    size_t size = 512 * 1024 * 1024;
    size_t min_size = 3 * (sizeof(UndoData) + 8 * mem->cur_doc->width * mem->cur_doc->height);
    mem->cur_doc->undo.size = math::max(size, min_size);

    mem->cur_doc->undo.start = malloc((size_t)mem->cur_doc->undo.size);
    mem->cur_doc->undo.current_index = -1;

    // TODO: Near-duplicate code from brush release. Combine.
    // Additive render-to-texture
    {
        GLCHK( glDisable(GL_SCISSOR_TEST) );
        GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, mem->misc.fbo) );
        GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                      GL_TEXTURE_2D, mem->misc.fbo_render_tex,
                                      0) );

        GLCHK( glViewport(0, 0, mem->cur_doc->width, mem->cur_doc->height) );
        GLCHK( glUseProgram(mem->shaders[PapayaShader_ImGui].handle) );

        GLCHK( glUniformMatrix4fv(mem->shaders[PapayaShader_ImGui].uniforms[0],
                                  1, GL_FALSE, &mem->cur_doc->proj_mtx[0][0]) );

        GLCHK( glBindBuffer(GL_ARRAY_BUFFER,
                            mem->meshes[PapayaMesh_RTTAdd].vbo_handle) );
        gl::set_vertex_attribs(mem->shaders[PapayaShader_ImGui]);

        // GLCHK( glBindTexture(GL_TEXTURE_2D,
        //                      (GLuint)(intptr_t)mem->cur_doc->texture_id) );
        // GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );

        undo::push(&mem->cur_doc->undo, &mem->profile,
                   Vec2i(0,0), Vec2i(mem->cur_doc->width, mem->cur_doc->height),
                   0, Vec2());

        uint32 temp = mem->misc.fbo_render_tex;
        // mem->misc.fbo_render_tex = mem->cur_doc->texture_id;
        GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                      GL_TEXTURE_2D, mem->misc.fbo_render_tex,
                                      0) );
        // mem->cur_doc->texture_id = temp;

        GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
        GLCHK( glViewport(0, 0, mem->cur_doc->width, mem->cur_doc->height) );

        GLCHK( glDisable(GL_BLEND) );
    }
}

void undo::destroy(PapayaMemory* mem)
{
    free(mem->cur_doc->undo.start);
    mem->cur_doc->undo.start = mem->cur_doc->undo.top = 0;
    mem->cur_doc->undo.base = mem->cur_doc->undo.current = mem->cur_doc->undo.last = 0;
    mem->cur_doc->undo.size = mem->cur_doc->undo.count = 0;
    mem->cur_doc->undo.current_index = -1;
}

// This function reads from the frame buffer and hence needs the appropriate frame buffer to be
// bound before it is called.
void undo::push(UndoBuffer* undo, Profile* profile, Vec2i pos, Vec2i size,
                int8* pre_brush_img, Vec2 line_segment_start_uv)
{
    if (undo->top == 0) {
        // Buffer is empty
        undo->base = (UndoData*)undo->start;
        undo->top  = undo->start;
    } else if (undo->current->next != 0) {
        // Not empty and not at end. Reposition for overwrite.
        uint64 bytes_to_right =
            (int8*)undo->start + undo->size - (int8*)undo->current;
        uint64 img_size = (undo->current->IsSubRect ? 8 : 4) *
            undo->current->size.x * undo->current->size.y;
        uint64 block_size = sizeof(UndoData) + img_size;
        if (bytes_to_right >= block_size) {
            undo->top = (int8*)undo->current + block_size;
        } else {
            undo->top = (int8*)undo->start + block_size - bytes_to_right;
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

    uint64 buf_size = sizeof(UndoData) +
        size.x * size.y * (data.IsSubRect ? 8 : 4);
    void* buf = malloc((size_t)buf_size);

    timer::start(Timer_GetUndoImage);
    memcpy(buf, &data, sizeof(UndoData));
    GLCHK( glReadPixels(pos.x, pos.y, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, (int8*)buf + sizeof(UndoData)) );
    timer::stop(Timer_GetUndoImage);

    if (data.IsSubRect) {
        memcpy((int8*)buf + sizeof(UndoData) + 4 * size.x * size.y, pre_brush_img, 4 * size.x * size.y);
    }

    uint64 bytes_to_right = (int8*)undo->start + undo->size - (int8*)undo->top;
    if (bytes_to_right < sizeof(UndoData)) // Not enough space for UndoData. Go to start.
    {
        // Reposition the base pointer
        while (((int8*)undo->base >= (int8*)undo->top ||
            (int8*)undo->base < (int8*)undo->start + buf_size) &&
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
        undo->top = (int8*)undo->top + buf_size;
    }
    else if (bytes_to_right < buf_size) // Enough space for UndoData, but not for image. Split image data.
    {
        // Reposition the base pointer
        while (((int8*)undo->base >= (int8*)undo->top ||
            (int8*)undo->base  <  (int8*)undo->start + buf_size - bytes_to_right) &&
            undo->count > 0)
        {
            undo->base = undo->base->next;
            undo->base->prev = 0;
            undo->count--;
            undo->current_index--;
        }

        memcpy(undo->top, buf, (size_t)bytes_to_right);
        memcpy(undo->start, (int8*)buf + bytes_to_right, (size_t)(buf_size - bytes_to_right));

        if (bytes_to_right == 0) { undo->top = undo->start; }

        if (undo->last) { undo->last->next = (UndoData*)undo->top; }
        undo->last = (UndoData*)undo->top;
        undo->top = (int8*)undo->start + buf_size - bytes_to_right;
    }
    else // Enough space for everything. Simply append.
    {
        // Reposition the base pointer
        while ((int8*)undo->base >= (int8*)undo->top &&
            (int8*)undo->base < (int8*)undo->top + buf_size &&
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
        undo->top = (int8*)undo->top + buf_size;
    }

    free(buf);

    undo->current = undo->last;
    undo->count++;
    undo->current_index++;
}

void undo::pop(PapayaMemory* mem, bool load_pre_brush_image)
{
    UndoBuffer* undo = &mem->cur_doc->undo;
    UndoData data = {};
    int8* img = 0;
    bool alloc_used = false;

    memcpy(&data, undo->current, sizeof(UndoData));

    size_t bytes_to_right = (int8*)undo->start + undo->size - (int8*)undo->current;
    size_t img_size =
        (undo->current->IsSubRect ? 8 : 4) * data.size.x * data.size.y;
    if (bytes_to_right - sizeof(UndoData) >= img_size) {
        // Image is contiguously stored
        img = (int8*)undo->current + sizeof(UndoData);
    } else {
        // Image is split
        alloc_used = true;
        img = (int8*)malloc(img_size);
        memcpy(img,
               (int8*)undo->current + sizeof(UndoData), 
               (size_t)bytes_to_right - sizeof(UndoData));
        memcpy((int8*)img + bytes_to_right - sizeof(UndoData),
               undo->start,
               (size_t)(img_size - (bytes_to_right - sizeof(UndoData))));
    }

    // GLCHK( glBindTexture(GL_TEXTURE_2D, mem->cur_doc->texture_id) );
    // GLCHK( glTexSubImage2D(GL_TEXTURE_2D, 0, data.pos.x, data.pos.y,
    //                        data.size.x, data.size.y, GL_RGBA, GL_UNSIGNED_BYTE, 
    //                        img + (load_pre_brush_image ? 
    //                               4 * data.size.x * data.size.y : 0)) );

    if (alloc_used) { free(img); }
}

