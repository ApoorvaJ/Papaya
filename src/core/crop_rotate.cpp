
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
    MeshInfo* Mesh = &Mem->Meshes[PapayaMesh_CropOutline];
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
    if (ImGui::Button("-90")) { Mem->CropRotate.BaseRotation--; }
    ImGui::SameLine();
    if (ImGui::Button("+90")) { Mem->CropRotate.BaseRotation++; }
    ImGui::SameLine();
    ImGui::PopStyleVar();

    ImGui::PushItemWidth(85);
    ImGui::SliderAngle("Rotate", &Mem->CropRotate.SliderAngle, -45.0f, 45.0f);
    ImGui::PopItemWidth();

    ImGui::SameLine(ImGui::GetWindowWidth() - 94); // TODO: Magic number alert
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));

    if (ImGui::Button("Apply"))
    {
        bool SizeChanged = (Mem->CropRotate.BaseRotation % 2 != 0);

        // Swap render texture and document texture handles
        if (SizeChanged)
        {
            int32 Temp      = Mem->Doc.Width;
            Mem->Doc.Width  = Mem->Doc.Height;
            Mem->Doc.Height = Temp;

            mat4x4_ortho(Mem->Doc.ProjMtx, 0.f, (float)Mem->Doc.Width,
                                           0.f, (float)Mem->Doc.Height,
                                          -1.f, 1.f);
            Mem->Doc.InverseAspect = (float)Mem->Doc.Height / (float)Mem->Doc.Width;
            GLCHK( glDeleteTextures(1, &Mem->Misc.FboSampleTexture) );
            Mem->Misc.FboSampleTexture = GL::AllocateTexture(Mem->Doc.Width,
                Mem->Doc.Height);
        }

        GLCHK( glDisable(GL_BLEND) );
        GLCHK( glViewport(0, 0, Mem->Doc.Width, Mem->Doc.Height) );

        // Bind and clear the frame buffer
        GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, Mem->Misc.FrameBufferObject) );

        GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_2D, Mem->Misc.FboSampleTexture, 0) );

        GLCHK( glClearColor(0.0f, 0.0f, 0.0f, 0.0f) );
        GLCHK( glClear(GL_COLOR_BUFFER_BIT) );

        mat4x4 M, R;
        // Rotate around center
        {
            Vec2 Offset = Vec2(Mem->Doc.Width  * 0.5f, Mem->Doc.Height * 0.5f);

            mat4x4_dup(M, Mem->Doc.ProjMtx);
            mat4x4_translate_in_place(M, Offset.x, Offset.y, 0.f);
            // mat4x4_rotate_Z(R, M, Math::ToRadians(-90));
            mat4x4_rotate_Z(R, M, Mem->CropRotate.SliderAngle +
                    Math::ToRadians(90.0f * Mem->CropRotate.BaseRotation));
            if (SizeChanged) { mat4x4_translate_in_place(R, -Offset.y, -Offset.x, 0.f); }
            else             { mat4x4_translate_in_place(R, -Offset.x, -Offset.y, 0.f); }
        }

        // Draw the image onto the frame buffer
        GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_RTTAdd].VboHandle) );
        GLCHK( glUseProgram(Mem->Shaders[PapayaShader_DeMultiplyAlpha].Handle) );
        GLCHK( glUniformMatrix4fv(Mem->Shaders[PapayaShader_ImGui].Uniforms[0],
                                  1, GL_FALSE, (GLfloat*)R) );
        GL::SetVertexAttribs(Mem->Shaders[PapayaShader_DeMultiplyAlpha]);
        GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->Doc.TextureID) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
        GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );

        uint32 Temp                = Mem->Misc.FboSampleTexture;
        Mem->Misc.FboSampleTexture = Mem->Doc.TextureID;
        Mem->Doc.TextureID         = Temp;

        if (SizeChanged)
        {
            Core::ResizeBuffers(Mem, Mem->Doc.Width, Mem->Doc.Height);

            // Reposition canvas to maintain apparent position
            int32 Delta = Math::RoundToInt((Mem->Doc.Height - Mem->Doc.Width)
                    * 0.5f * Mem->Doc.CanvasZoom);
            Mem->Doc.CanvasPosition.x += Delta;
            Mem->Doc.CanvasPosition.y -= Delta;
        }

        // Reset stuff
        GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
        GLCHK( glViewport(0, 0, (int32)ImGui::GetIO().DisplaySize.x, (int32)ImGui::GetIO().DisplaySize.y) );

        Mem->CropRotate.SliderAngle  = 0.f;
        Mem->CropRotate.BaseRotation = 0;
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        Mem->CropRotate.SliderAngle  = 0.f;
        Mem->CropRotate.BaseRotation = 0;
    }
    ImGui::PopStyleVar();
}

void CropRotate::CropOutline(PapayaMemory* Mem)
{
    // TODO: Function lacks grace
    Vec2 Mouse = Mem->Mouse.Pos;
    Vec2 P[4];
    P[0] = Mem->Doc.CanvasPosition + Mem->CropRotate.TopLeft * Mem->Doc.CanvasZoom;
    P[2] = Mem->Doc.CanvasPosition + Mem->CropRotate.BotRight * Mem->Doc.CanvasZoom;
    P[1] = Vec2(P[0].x, P[2].y);
    P[3] = Vec2(P[2].x, P[0].y);

    // Only change vertex selection if the left mouse button is not down
    if (!Mem->Mouse.IsDown[0])
    {
        Mem->CropRotate.CropMode = 0;

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
                Mem->CropRotate.CropMode = 1 << MinIndex;
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
                Mem->CropRotate.CropMode = (1 << MinIndex) | (1 << (MinIndex + 1) % 4);
                goto Dragging;
            }
        }

        // Entire rect selection
        {
            Vec2 V = (Mouse - Mem->Doc.CanvasPosition) / Mem->Doc.CanvasZoom;
            if (V.x >= Mem->CropRotate.TopLeft.x  &&
                V.x <= Mem->CropRotate.BotRight.x &&
                V.y >= Mem->CropRotate.TopLeft.y  &&
                V.y <= Mem->CropRotate.BotRight.y)
            {
                Mem->CropRotate.CropMode = 15;
                Mem->CropRotate.RectDragPosition = V - Mem->CropRotate.TopLeft;
            }
        }
    }

Dragging:
    if (Mem->CropRotate.CropMode && Mem->Mouse.IsDown[0])
    {
        Vec2 V = (Mouse - Mem->Doc.CanvasPosition) / Mem->Doc.CanvasZoom;
        // TODO: Implement smart-bounds toggle for partial image rotational cropping
        //       while maintaining aspect ratio
        // Whole rect
        if (Mem->CropRotate.CropMode == 15)
        {
            Vec2 V = ((Mouse - Mem->Doc.CanvasPosition) / Mem->Doc.CanvasZoom)
                     - Mem->CropRotate.TopLeft;
            Vec2 Delta = V - Mem->CropRotate.RectDragPosition;
            Delta.x = Math::Clamp((float)round(Delta.x), -1.f * Mem->CropRotate.TopLeft.x,
                                  Mem->Doc.Width - Mem->CropRotate.BotRight.x);
            Delta.y = Math::Clamp((float)round(Delta.y), -1.f * Mem->CropRotate.TopLeft.y,
                                  Mem->Doc.Height - Mem->CropRotate.BotRight.y);
            Mem->CropRotate.TopLeft  += Delta;
            Mem->CropRotate.BotRight += Delta;
            goto Drawing;
        }
        // Edges
        if      (Mem->CropRotate.CropMode == 3)  { Mem->CropRotate.TopLeft.x  = V.x; }
        else if (Mem->CropRotate.CropMode == 9)  { Mem->CropRotate.TopLeft.y  = V.y; }
        else if (Mem->CropRotate.CropMode == 12) { Mem->CropRotate.BotRight.x = V.x; }
        else if (Mem->CropRotate.CropMode == 6)  { Mem->CropRotate.BotRight.y = V.y; }
        else // Vertices
        {
            if (Mem->CropRotate.CropMode & 3)  { Mem->CropRotate.TopLeft.x  = V.x; }
            if (Mem->CropRotate.CropMode & 9)  { Mem->CropRotate.TopLeft.y  = V.y; }
            if (Mem->CropRotate.CropMode & 12) { Mem->CropRotate.BotRight.x = V.x; }
            if (Mem->CropRotate.CropMode & 6)  { Mem->CropRotate.BotRight.y = V.y; }
        }

        if (Mem->CropRotate.CropMode & 3)
        {
            Mem->CropRotate.TopLeft.x =
                Math::Clamp((float)round(Mem->CropRotate.TopLeft.x),
                        0.f, Mem->CropRotate.BotRight.x - 1);
        }
        if (Mem->CropRotate.CropMode & 9)
        {
            Mem->CropRotate.TopLeft.y =
                Math::Clamp((float)round(Mem->CropRotate.TopLeft.y),
                        0.f, Mem->CropRotate.BotRight.y - 1);
        }
        if (Mem->CropRotate.CropMode & 12)
        {
            Mem->CropRotate.BotRight.x =
                Math::Clamp((float)round(Mem->CropRotate.BotRight.x),
                        Mem->CropRotate.TopLeft.x + 1, (float)Mem->Doc.Width);
        }
        if (Mem->CropRotate.CropMode & 6)
        {
            Mem->CropRotate.BotRight.y =
                Math::Clamp((float)round(Mem->CropRotate.BotRight.y),
                        Mem->CropRotate.TopLeft.y + 1, (float)Mem->Doc.Height);
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
        uint8 Mode = Mem->CropRotate.CropMode;
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
    GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_CropOutline].VboHandle) );
    GLCHK( glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Verts), Verts) );

    GL::DrawMesh(Mem->Meshes[PapayaMesh_CropOutline],
            Mem->Shaders[PapayaShader_VertexColor], true,
            1, UniformType_Matrix4, &Mem->Window.ProjMtx[0][0]);
}
