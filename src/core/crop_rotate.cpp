
#include "crop_rotate.h"
#include "libs/gl_util.h"
#include "libs/imgui/imgui.h"
#include "libs/mathlib.h"
#include "libs/linmath.h"
#include "papaya_core.h"

void crop_rotate::init(PapayaMemory* mem)
{
    // Initialize crop line mesh
    Mesh* mesh = &mem->meshes[PapayaMesh_CropOutline];
    mesh->is_line_loop = true;
    mesh->index_count = 4;
    GLCHK( glGenBuffers(1, &mesh->vbo_handle) );
    GLCHK( glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_handle) );
    GLCHK( glBufferData(GL_ARRAY_BUFFER, sizeof(ImDrawVert) * mesh->index_count,
                        0, GL_DYNAMIC_DRAW) );
}

void crop_rotate::toolbar(PapayaMemory* mem)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3, 0));
    if (ImGui::Button("-90")) { mem->crop_rotate.base_rotation--; }
    ImGui::SameLine();
    if (ImGui::Button("+90")) { mem->crop_rotate.base_rotation++; }
    ImGui::SameLine();
    ImGui::PopStyleVar();

    ImGui::PushItemWidth(85);
    ImGui::SliderAngle("Rotate", &mem->crop_rotate.slider_angle, -45.0f, 45.0f);
    ImGui::PopItemWidth();

    ImGui::SameLine(ImGui::GetWindowWidth() - 94); // TODO: Magic number alert
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));

    if (ImGui::Button("Apply")) {
        bool size_changed = (mem->crop_rotate.base_rotation % 2 != 0);

        // Swap render texture and document texture handles
        if (size_changed) {
            int32 temp = mem->doc.width;
            mem->doc.width  = mem->doc.height;
            mem->doc.height = temp;

            mat4x4_ortho(mem->doc.proj_mtx,
                         0.f, (float)mem->doc.width,
                         0.f, (float)mem->doc.height,
                         -1.f, 1.f);
            mem->doc.inverse_aspect = (float)mem->doc.height /
                                      (float)mem->doc.width;
            GLCHK( glDeleteTextures(1, &mem->misc.fbo_sample_tex) );
            mem->misc.fbo_sample_tex = gl::allocate_tex(mem->doc.width,
                                                           mem->doc.height);
        }

        GLCHK( glDisable(GL_BLEND) );
        GLCHK( glViewport(0, 0, mem->doc.width, mem->doc.height) );

        // Bind and clear the frame buffer
        GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, mem->misc.fbo) );
        GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                      GL_TEXTURE_2D, mem->misc.fbo_sample_tex, 0) );

        GLCHK( glClearColor(0.0f, 0.0f, 0.0f, 0.0f) );
        GLCHK( glClear(GL_COLOR_BUFFER_BIT) );

        mat4x4 m, r;
        // Rotate around center
        {
            Vec2 offset = Vec2(mem->doc.width  * 0.5f,
                               mem->doc.height * 0.5f);
            mat4x4_dup(m, mem->doc.proj_mtx);
            mat4x4_translate_in_place(m, offset.x, offset.y, 0.f);
            // mat4x4_rotate_Z(r, m, math::to_radians(-90));
            mat4x4_rotate_Z(r, m, mem->crop_rotate.slider_angle +
                            math::to_radians(90.0f * mem->crop_rotate.base_rotation));
            if (size_changed) {
                mat4x4_translate_in_place(r, -offset.y, -offset.x, 0.f);
            } else {
                mat4x4_translate_in_place(r, -offset.x, -offset.y, 0.f);
            }
        }

        // Draw the image onto the frame buffer
        GLCHK( glBindBuffer(GL_ARRAY_BUFFER, mem->meshes[PapayaMesh_RTTAdd].vbo_handle) );
        GLCHK( glUseProgram(mem->shaders[PapayaShader_DeMultiplyAlpha].handle) );
        GLCHK( glUniformMatrix4fv(mem->shaders[PapayaShader_ImGui].uniforms[0],
                                  1, GL_FALSE, (GLfloat*)r) );
        gl::set_vertex_attribs(mem->shaders[PapayaShader_DeMultiplyAlpha]);
        GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)mem->doc.texture_id) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
        GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );

        uint32 temp = mem->misc.fbo_sample_tex;
        mem->misc.fbo_sample_tex = mem->doc.texture_id;
        mem->doc.texture_id = temp;

        if (size_changed) {
            core::resize_doc(mem, mem->doc.width, mem->doc.height);

            // Reposition canvas to maintain apparent position
            int32 delta = math::round_to_int((mem->doc.height - mem->doc.width)
                    * 0.5f * mem->doc.canvas_zoom);
            mem->doc.canvas_pos.x += delta;
            mem->doc.canvas_pos.y -= delta;
        }

        // Reset stuff
        GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
        GLCHK( glViewport(0, 0, (int32)ImGui::GetIO().DisplaySize.x, (int32)ImGui::GetIO().DisplaySize.y) );

        mem->crop_rotate.slider_angle = 0.f;
        mem->crop_rotate.base_rotation = 0;
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        mem->crop_rotate.slider_angle = 0.f;
        mem->crop_rotate.base_rotation = 0;
    }

    ImGui::PopStyleVar();
}

void crop_rotate::crop_outline(PapayaMemory* mem)
{
    // TODO: Function lacks grace
    Vec2 mouse = mem->mouse.pos;
    Vec2 p[4];
    p[0] = mem->doc.canvas_pos + mem->crop_rotate.top_left * mem->doc.canvas_zoom;
    p[2] = mem->doc.canvas_pos + mem->crop_rotate.bot_right * mem->doc.canvas_zoom;
    p[1] = Vec2(p[0].x, p[2].y);
    p[3] = Vec2(p[2].x, p[0].y);

    // Only change vertex selection if the left mouse button is not down
    if (!mem->mouse.is_down[0]) {
        mem->crop_rotate.crop_mode = 0;

        // Vertex selection
        {
            float min_dist = FLT_MAX;
            int32 min_index;

            for (int32 i = 0; i < 4; i++) {
                float dist = math::distance(p[i], mouse);
                if (min_dist > dist) {
                    min_dist = dist;
                    min_index = i;
                }
            }

            if (min_dist < 10.f) {
                mem->crop_rotate.crop_mode = 1 << min_index;
                goto dragging;
            }
        }

        // Edge selecton
        {
            float min_dist = FLT_MAX;
            int32 min_index;

            for (int32 i = 0; i < 4; i++) {
                int32 j = (i + 1) % 4;
                vec2 v1 = { p[i].x  , p[i].y  };
                vec2 v2 = { p[j].x  , p[j].y  };
                vec2 m  = { mouse.x , mouse.y };
                vec2 a, b;
                vec2_sub(a, v2, v1);
                vec2_sub(b, m , v1);
                float dot = vec2_mul_inner(a, b);
                // Continue if projection of mouse doesn't lie on the edge v1-v2
                if (dot < 0 || dot > vec2_mul_inner(a, a)) { continue; }
                float dist = (i % 2 == 0) ? math::abs(p[i].x - mouse.x) // Vertical edge
                                          : math::abs(p[i].y - mouse.y);
                if (min_dist > dist) {
                    min_dist = dist;
                    min_index = i;
                }
            }

            if (min_dist < 10.f) {
                mem->crop_rotate.crop_mode = (1 << min_index) |
                                             (1 << (min_index + 1) % 4);
                goto dragging;
            }
        }

        // Entire rect selection
        {
            Vec2 v = (mouse - mem->doc.canvas_pos) / mem->doc.canvas_zoom;
            if (v.x >= mem->crop_rotate.top_left.x  &&
                v.x <= mem->crop_rotate.bot_right.x &&
                v.y >= mem->crop_rotate.top_left.y  &&
                v.y <= mem->crop_rotate.bot_right.y) {
                mem->crop_rotate.crop_mode = 15;
                mem->crop_rotate.rect_drag_position = v - mem->crop_rotate.top_left;
            }
        }
    }

dragging:
    if (mem->crop_rotate.crop_mode && mem->mouse.is_down[0]) {
        Vec2 v = (mouse - mem->doc.canvas_pos) / mem->doc.canvas_zoom;
        // TODO: Implement smart-bounds toggle for partial image rotational cropping
        //       while maintaining aspect ratio
        // Whole rect
        if (mem->crop_rotate.crop_mode == 15) {
            Vec2 v = ((mouse - mem->doc.canvas_pos) / mem->doc.canvas_zoom)
                     - mem->crop_rotate.top_left;
            Vec2 delta = v - mem->crop_rotate.rect_drag_position;
            delta.x = math::clamp((float)round(delta.x),
                                  -1.f * mem->crop_rotate.top_left.x,
                                  mem->doc.width - mem->crop_rotate.bot_right.x);
            delta.y = math::clamp((float)round(delta.y),
                                  -1.f * mem->crop_rotate.top_left.y,
                                  mem->doc.height - mem->crop_rotate.bot_right.y);
            mem->crop_rotate.top_left  += delta;
            mem->crop_rotate.bot_right += delta;
            goto drawing;
        }

        // Edges
        if      (mem->crop_rotate.crop_mode == 3)  { mem->crop_rotate.top_left.x = v.x;  }
        else if (mem->crop_rotate.crop_mode == 9)  { mem->crop_rotate.top_left.y = v.y;  }
        else if (mem->crop_rotate.crop_mode == 12) { mem->crop_rotate.bot_right.x = v.x; }
        else if (mem->crop_rotate.crop_mode == 6)  { mem->crop_rotate.bot_right.y = v.y; }
        else {
            // Vertices
            if (mem->crop_rotate.crop_mode & 3)  { mem->crop_rotate.top_left.x = v.x;  }
            if (mem->crop_rotate.crop_mode & 9)  { mem->crop_rotate.top_left.y = v.y;  }
            if (mem->crop_rotate.crop_mode & 12) { mem->crop_rotate.bot_right.x = v.x; }
            if (mem->crop_rotate.crop_mode & 6)  { mem->crop_rotate.bot_right.y = v.y; }
        }

        if (mem->crop_rotate.crop_mode & 3) {
            mem->crop_rotate.top_left.x =
                math::clamp((float)round(mem->crop_rotate.top_left.x),
                            0.f,
                            mem->crop_rotate.bot_right.x - 1);
        }
        if (mem->crop_rotate.crop_mode & 9) {
            mem->crop_rotate.top_left.y =
                math::clamp((float)round(mem->crop_rotate.top_left.y),
                            0.f,
                            mem->crop_rotate.bot_right.y - 1);
        }
        if (mem->crop_rotate.crop_mode & 12) {
            mem->crop_rotate.bot_right.x =
                math::clamp((float)round(mem->crop_rotate.bot_right.x),
                            mem->crop_rotate.top_left.x + 1,
                            (float)mem->doc.width);
        }
        if (mem->crop_rotate.crop_mode & 6) {
            mem->crop_rotate.bot_right.y =
                math::clamp((float)round(mem->crop_rotate.bot_right.y),
                            mem->crop_rotate.top_left.y + 1,
                            (float)mem->doc.height);
        }
    }

drawing:
    ImDrawVert verts[4];
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        verts[0].pos = p[0];              
        verts[1].pos = p[1];
        verts[2].pos = p[2];              
        verts[3].pos = p[3];

        verts[0].uv = Vec2(0.0f, 0.0f);
        verts[1].uv = Vec2(0.0f, 1.0f);
        verts[2].uv = Vec2(1.0f, 1.0f);
        verts[3].uv = Vec2(1.0f, 0.0f);

        uint32 col1 = 0xffcc7a00;
        uint32 col2 = 0xff1189e6;
        uint32 col3 = 0xff36bb0a;
        uint8 Mode = mem->crop_rotate.crop_mode;
        if (Mode == 15) {
            verts[0].col = verts[1].col = verts[2].col = verts[3].col = col3;
        }
        else {
            verts[0].col = (Mode & 1) ? col2 : col1;
            verts[1].col = (Mode & 2) ? col2 : col1;
            verts[2].col = (Mode & 4) ? col2 : col1;
            verts[3].col = (Mode & 8) ? col2 : col1;
        }
    }
    GLCHK( glBindBuffer(GL_ARRAY_BUFFER, mem->meshes[PapayaMesh_CropOutline].vbo_handle) );
    GLCHK( glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts) );

    gl::draw_mesh(mem->meshes[PapayaMesh_CropOutline],
                 mem->shaders[PapayaShader_VertexColor], true,
                 1, UniformType_Matrix4, &mem->window.proj_mtx[0][0]);
}
