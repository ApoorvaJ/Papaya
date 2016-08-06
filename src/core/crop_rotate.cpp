
#include "crop_rotate.h"
#include "libs/glew/glew.h"
#include "libs/gl.h"
#include "libs/imgui/imgui.h"
#include "libs/mathlib.h"
#include "libs/linmath.h"
#include "papaya_core.h"

void CropRotate::Init(PapayaMemory* Mem)
{
    // Initialize crop line mesh
    MeshInfo* Mesh = &Mem->meshes[PapayaMesh_CropOutline];
    Mesh->IsLineLoop = true;
    Mesh->IndexCount = 4;
    GLCHK( glGenBuffers(1, &Mesh->VboHandle) );
    GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mesh->VboHandle) );
    GLCHK( glBufferData(GL_ARRAY_BUFFER, sizeof(ImDrawVert) *
                Mesh->IndexCount , 0, GL_DYNAMIC_DRAW) );
}

void CropRotate::Toolbar(PapayaMemory* Mem)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3, 0));
    if (ImGui::Button("-90")) { Mem->crop_rotate.BaseRotation--; }
    ImGui::SameLine();
    if (ImGui::Button("+90")) { Mem->crop_rotate.BaseRotation++; }
    ImGui::SameLine();
    ImGui::PopStyleVar();

    ImGui::PushItemWidth(85);
    ImGui::SliderAngle("Rotate", &Mem->crop_rotate.SliderAngle, -45.0f, 45.0f);
    ImGui::PopItemWidth();

    ImGui::SameLine(ImGui::GetWindowWidth() - 94); // TODO: Magic number alert
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));

    if (ImGui::Button("Apply"))
    {
        bool SizeChanged = (Mem->crop_rotate.BaseRotation % 2 != 0);

        // Swap render texture and document texture handles
        if (SizeChanged)
        {
            int32 Temp      = Mem->doc.Width;
            Mem->doc.Width  = Mem->doc.Height;
            Mem->doc.Height = Temp;

            mat4x4_ortho(Mem->doc.ProjMtx, 0.f, (float)Mem->doc.Width,
                                           0.f, (float)Mem->doc.Height,
                                          -1.f, 1.f);
            Mem->doc.InverseAspect = (float)Mem->doc.Height / (float)Mem->doc.Width;
            GLCHK( glDeleteTextures(1, &Mem->misc.fbo_sample_tex) );
            Mem->misc.fbo_sample_tex = GL::AllocateTexture(Mem->doc.Width,
                Mem->doc.Height);
        }

        GLCHK( glDisable(GL_BLEND) );
        GLCHK( glViewport(0, 0, Mem->doc.Width, Mem->doc.Height) );

        // Bind and clear the frame buffer
        GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, Mem->misc.fbo) );

        GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_2D, Mem->misc.fbo_sample_tex, 0) );

        GLCHK( glClearColor(0.0f, 0.0f, 0.0f, 0.0f) );
        GLCHK( glClear(GL_COLOR_BUFFER_BIT) );

        mat4x4 M, R;
        // Rotate around center
        {
            Vec2 Offset = Vec2(Mem->doc.Width  * 0.5f, Mem->doc.Height * 0.5f);

            mat4x4_dup(M, Mem->doc.ProjMtx);
            mat4x4_translate_in_place(M, Offset.x, Offset.y, 0.f);
            // mat4x4_rotate_Z(R, M, Math::ToRadians(-90));
            mat4x4_rotate_Z(R, M, Mem->crop_rotate.SliderAngle +
                    Math::ToRadians(90.0f * Mem->crop_rotate.BaseRotation));
            if (SizeChanged) { mat4x4_translate_in_place(R, -Offset.y, -Offset.x, 0.f); }
            else             { mat4x4_translate_in_place(R, -Offset.x, -Offset.y, 0.f); }
        }

        // Draw the image onto the frame buffer
        GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mem->meshes[PapayaMesh_RTTAdd].VboHandle) );
        GLCHK( glUseProgram(Mem->shaders[PapayaShader_DeMultiplyAlpha].Handle) );
        GLCHK( glUniformMatrix4fv(Mem->shaders[PapayaShader_ImGui].Uniforms[0],
                                  1, GL_FALSE, (GLfloat*)R) );
        GL::SetVertexAttribs(Mem->shaders[PapayaShader_DeMultiplyAlpha]);
        GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->doc.TextureID) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
        GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );

        uint32 Temp                = Mem->misc.fbo_sample_tex;
        Mem->misc.fbo_sample_tex = Mem->doc.TextureID;
        Mem->doc.TextureID         = Temp;

        if (SizeChanged)
        {
            core::resize_doc(Mem, Mem->doc.Width, Mem->doc.Height);

            // Reposition canvas to maintain apparent position
            int32 Delta = Math::RoundToInt((Mem->doc.Height - Mem->doc.Width)
                    * 0.5f * Mem->doc.CanvasZoom);
            Mem->doc.CanvasPosition.x += Delta;
            Mem->doc.CanvasPosition.y -= Delta;
        }

        // Reset stuff
        GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
        GLCHK( glViewport(0, 0, (int32)ImGui::GetIO().DisplaySize.x, (int32)ImGui::GetIO().DisplaySize.y) );

        Mem->crop_rotate.SliderAngle  = 0.f;
        Mem->crop_rotate.BaseRotation = 0;
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        Mem->crop_rotate.SliderAngle  = 0.f;
        Mem->crop_rotate.BaseRotation = 0;
    }
    ImGui::PopStyleVar();
}

void CropRotate::CropOutline(PapayaMemory* Mem)
{
    // TODO: Function lacks grace
    Vec2 Mouse = Mem->mouse.Pos;
    Vec2 P[4];
    P[0] = Mem->doc.CanvasPosition + Mem->crop_rotate.TopLeft * Mem->doc.CanvasZoom;
    P[2] = Mem->doc.CanvasPosition + Mem->crop_rotate.BotRight * Mem->doc.CanvasZoom;
    P[1] = Vec2(P[0].x, P[2].y);
    P[3] = Vec2(P[2].x, P[0].y);

    // Only change vertex selection if the left mouse button is not down
    if (!Mem->mouse.IsDown[0])
    {
        Mem->crop_rotate.CropMode = 0;

        // Vertex selection
        {
            float MinDist = FLT_MAX;
            int32 MinIndex;

            for (int32 i = 0; i < 4; i++)
            {
                float Dist = Math::Distance(P[i], Mouse);
                if (MinDist > Dist)
                {
                    MinDist = Dist;
                    MinIndex = i;
                }
            }

            if (MinDist < 10.f)
            {
                Mem->crop_rotate.CropMode = 1 << MinIndex;
                goto Dragging;
            }
        }

        // Edge selecton
        {
            float MinDist = FLT_MAX;
            int32 MinIndex;

            for (int32 i = 0; i < 4; i++)
            {
                int32 j = (i + 1) % 4;
                vec2 V1 = { P[i].x  , P[i].y  };
                vec2 V2 = { P[j].x  , P[j].y  };
                vec2 M  = { Mouse.x , Mouse.y };
                vec2 A, B;
                vec2_sub(A, V2, V1);
                vec2_sub(B, M , V1);
                float Dot = vec2_mul_inner(A, B);
                // Continue if projection of mouse doesn't lie on the edge V1-V2
                if (Dot < 0 || Dot > vec2_mul_inner(A, A)) { continue; }
                float Dist = (i % 2 == 0) ?
                             Math::Abs(P[i].x - Mouse.x) : // Vertical edge
                             Math::Abs(P[i].y - Mouse.y);
                if (MinDist > Dist)
                {
                    MinDist = Dist;
                    MinIndex = i;
                }
            }

            if (MinDist < 10.f)
            {
                Mem->crop_rotate.CropMode = (1 << MinIndex) | (1 << (MinIndex + 1) % 4);
                goto Dragging;
            }
        }

        // Entire rect selection
        {
            Vec2 V = (Mouse - Mem->doc.CanvasPosition) / Mem->doc.CanvasZoom;
            if (V.x >= Mem->crop_rotate.TopLeft.x  &&
                V.x <= Mem->crop_rotate.BotRight.x &&
                V.y >= Mem->crop_rotate.TopLeft.y  &&
                V.y <= Mem->crop_rotate.BotRight.y)
            {
                Mem->crop_rotate.CropMode = 15;
                Mem->crop_rotate.RectDragPosition = V - Mem->crop_rotate.TopLeft;
            }
        }
    }

Dragging:
    if (Mem->crop_rotate.CropMode && Mem->mouse.IsDown[0])
    {
        Vec2 V = (Mouse - Mem->doc.CanvasPosition) / Mem->doc.CanvasZoom;
        // TODO: Implement smart-bounds toggle for partial image rotational cropping
        //       while maintaining aspect ratio
        // Whole rect
        if (Mem->crop_rotate.CropMode == 15)
        {
            Vec2 V = ((Mouse - Mem->doc.CanvasPosition) / Mem->doc.CanvasZoom)
                     - Mem->crop_rotate.TopLeft;
            Vec2 Delta = V - Mem->crop_rotate.RectDragPosition;
            Delta.x = Math::Clamp((float)round(Delta.x), -1.f * Mem->crop_rotate.TopLeft.x,
                                  Mem->doc.Width - Mem->crop_rotate.BotRight.x);
            Delta.y = Math::Clamp((float)round(Delta.y), -1.f * Mem->crop_rotate.TopLeft.y,
                                  Mem->doc.Height - Mem->crop_rotate.BotRight.y);
            Mem->crop_rotate.TopLeft  += Delta;
            Mem->crop_rotate.BotRight += Delta;
            goto Drawing;
        }
        // Edges
        if      (Mem->crop_rotate.CropMode == 3)  { Mem->crop_rotate.TopLeft.x  = V.x; }
        else if (Mem->crop_rotate.CropMode == 9)  { Mem->crop_rotate.TopLeft.y  = V.y; }
        else if (Mem->crop_rotate.CropMode == 12) { Mem->crop_rotate.BotRight.x = V.x; }
        else if (Mem->crop_rotate.CropMode == 6)  { Mem->crop_rotate.BotRight.y = V.y; }
        else // Vertices
        {
            if (Mem->crop_rotate.CropMode & 3)  { Mem->crop_rotate.TopLeft.x  = V.x; }
            if (Mem->crop_rotate.CropMode & 9)  { Mem->crop_rotate.TopLeft.y  = V.y; }
            if (Mem->crop_rotate.CropMode & 12) { Mem->crop_rotate.BotRight.x = V.x; }
            if (Mem->crop_rotate.CropMode & 6)  { Mem->crop_rotate.BotRight.y = V.y; }
        }

        if (Mem->crop_rotate.CropMode & 3)
        {
            Mem->crop_rotate.TopLeft.x =
                Math::Clamp((float)round(Mem->crop_rotate.TopLeft.x),
                        0.f, Mem->crop_rotate.BotRight.x - 1);
        }
        if (Mem->crop_rotate.CropMode & 9)
        {
            Mem->crop_rotate.TopLeft.y =
                Math::Clamp((float)round(Mem->crop_rotate.TopLeft.y),
                        0.f, Mem->crop_rotate.BotRight.y - 1);
        }
        if (Mem->crop_rotate.CropMode & 12)
        {
            Mem->crop_rotate.BotRight.x =
                Math::Clamp((float)round(Mem->crop_rotate.BotRight.x),
                        Mem->crop_rotate.TopLeft.x + 1, (float)Mem->doc.Width);
        }
        if (Mem->crop_rotate.CropMode & 6)
        {
            Mem->crop_rotate.BotRight.y =
                Math::Clamp((float)round(Mem->crop_rotate.BotRight.y),
                        Mem->crop_rotate.TopLeft.y + 1, (float)Mem->doc.Height);
        }
    }

Drawing:
    ImDrawVert Verts[4];
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        Verts[0].pos = P[0];              
        Verts[1].pos = P[1];
        Verts[2].pos = P[2];              
        Verts[3].pos = P[3];

        Verts[0].uv = Vec2(0.0f, 0.0f);
        Verts[1].uv = Vec2(0.0f, 1.0f);
        Verts[2].uv = Vec2(1.0f, 1.0f);
        Verts[3].uv = Vec2(1.0f, 0.0f);

        uint32 Col1 = 0xffcc7a00;
        uint32 Col2 = 0xff1189e6;
        uint32 Col3 = 0xff36bb0a;
        uint8 Mode = Mem->crop_rotate.CropMode;
        if (Mode == 15)
        {
            Verts[0].col = Verts[1].col = Verts[2].col = Verts[3].col = Col3;
        }
        else
        {
            Verts[0].col = (Mode & 1) ? Col2 : Col1;
            Verts[1].col = (Mode & 2) ? Col2 : Col1;
            Verts[2].col = (Mode & 4) ? Col2 : Col1;
            Verts[3].col = (Mode & 8) ? Col2 : Col1;
        }
    }
    GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mem->meshes[PapayaMesh_CropOutline].VboHandle) );
    GLCHK( glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Verts), Verts) );

    GL::DrawMesh(Mem->meshes[PapayaMesh_CropOutline],
            Mem->shaders[PapayaShader_VertexColor], true,
            1, UniformType_Matrix4, &Mem->window.ProjMtx[0][0]);
}
