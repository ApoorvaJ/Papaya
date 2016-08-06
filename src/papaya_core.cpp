
#include "papaya_core.h"

#include "libs/glew/glew.h"
#include "libs/stb_image.h"
#include "libs/stb_image_write.h"
#include "libs/imgui/imgui.h"
#include "libs/mathlib.h"
#include "libs/linmath.h"



// This function reads from the frame buffer and hence needs the appropriate frame buffer to be
// bound before it is called.
internal void push_undo(PapayaMemory* mem, Vec2i Pos, Vec2i Size,
                        int8* PreBrushImage, Vec2 LineSegmentStartUV) {
    if (mem->doc.Undo.Top == 0) {
        // Buffer is empty
        mem->doc.Undo.Base = (UndoData*)mem->doc.Undo.Start;
        mem->doc.Undo.Top  = mem->doc.Undo.Start;
    }
    else if (mem->doc.Undo.Current->Next != 0) {
        // Not empty and not at end. Reposition for overwrite.
        uint64 BytesToRight = (int8*)mem->doc.Undo.Start + mem->doc.Undo.Size - (int8*)mem->doc.Undo.Current;
        uint64 ImageSize = (mem->doc.Undo.Current->IsSubRect ? 8 : 4) * mem->doc.Undo.Current->Size.x * mem->doc.Undo.Current->Size.y;
        uint64 BlockSize = sizeof(UndoData) + ImageSize;
        if (BytesToRight >= BlockSize)
        {
            mem->doc.Undo.Top = (int8*)mem->doc.Undo.Current + BlockSize;
        }
        else
        {
            mem->doc.Undo.Top = (int8*)mem->doc.Undo.Start + BlockSize - BytesToRight;
        }
        mem->doc.Undo.Last = mem->doc.Undo.Current;
        mem->doc.Undo.Count = mem->doc.Undo.CurrentIndex + 1;
    }

    UndoData Data           = {};
    Data.OpCode             = PapayaUndoOp_Brush;
    Data.Prev               = mem->doc.Undo.Last;
    Data.Pos                = Pos;
    Data.Size               = Size;
    Data.IsSubRect          = (PreBrushImage != 0);
    Data.LineSegmentStartUV = LineSegmentStartUV;
    uint64 BufSize          = sizeof(UndoData) + Size.x * Size.y * (Data.IsSubRect ? 8 : 4);
    void* Buf               = malloc((size_t)BufSize);

    Timer::StartTime(&mem->debug.Timers[Timer_GetUndoImage]);
    memcpy(Buf, &Data, sizeof(UndoData));
    GLCHK( glReadPixels(Pos.x, Pos.y, Size.x, Size.y, GL_RGBA, GL_UNSIGNED_BYTE, (int8*)Buf + sizeof(UndoData)) );
    Timer::StopTime(&mem->debug.Timers[Timer_GetUndoImage]);

    if (Data.IsSubRect)
    {
        memcpy((int8*)Buf + sizeof(UndoData) + 4 * Size.x * Size.y, PreBrushImage, 4 * Size.x * Size.y);
    }

    uint64 BytesToRight = (int8*)mem->doc.Undo.Start + mem->doc.Undo.Size - (int8*)mem->doc.Undo.Top;
    if (BytesToRight < sizeof(UndoData)) // Not enough space for UndoData. Go to start.
    {
        // Reposition the base pointer
        while (((int8*)mem->doc.Undo.Base >= (int8*)mem->doc.Undo.Top ||
            (int8*)mem->doc.Undo.Base < (int8*)mem->doc.Undo.Start + BufSize) &&
            mem->doc.Undo.Count > 0)
        {
            mem->doc.Undo.Base = mem->doc.Undo.Base->Next;
            mem->doc.Undo.Base->Prev = 0;
            mem->doc.Undo.Count--;
            mem->doc.Undo.CurrentIndex--;
        }

        mem->doc.Undo.Top = mem->doc.Undo.Start;
        memcpy(mem->doc.Undo.Top, Buf, (size_t)BufSize);

        if (mem->doc.Undo.Last) { mem->doc.Undo.Last->Next = (UndoData*)mem->doc.Undo.Top; }
        mem->doc.Undo.Last = (UndoData*)mem->doc.Undo.Top;
        mem->doc.Undo.Top = (int8*)mem->doc.Undo.Top + BufSize;
    }
    else if (BytesToRight < BufSize) // Enough space for UndoData, but not for image. Split image data.
    {
        // Reposition the base pointer
        while (((int8*)mem->doc.Undo.Base >= (int8*)mem->doc.Undo.Top ||
            (int8*)mem->doc.Undo.Base  <  (int8*)mem->doc.Undo.Start + BufSize - BytesToRight) &&
            mem->doc.Undo.Count > 0)
        {
            mem->doc.Undo.Base = mem->doc.Undo.Base->Next;
            mem->doc.Undo.Base->Prev = 0;
            mem->doc.Undo.Count--;
            mem->doc.Undo.CurrentIndex--;
        }

        memcpy(mem->doc.Undo.Top, Buf, (size_t)BytesToRight);
        memcpy(mem->doc.Undo.Start, (int8*)Buf + BytesToRight, (size_t)(BufSize - BytesToRight));

        if (BytesToRight == 0) { mem->doc.Undo.Top = mem->doc.Undo.Start; }

        if (mem->doc.Undo.Last) { mem->doc.Undo.Last->Next = (UndoData*)mem->doc.Undo.Top; }
        mem->doc.Undo.Last = (UndoData*)mem->doc.Undo.Top;
        mem->doc.Undo.Top = (int8*)mem->doc.Undo.Start + BufSize - BytesToRight;
    }
    else // Enough space for everything. Simply append.
    {
        // Reposition the base pointer
        while ((int8*)mem->doc.Undo.Base >= (int8*)mem->doc.Undo.Top &&
            (int8*)mem->doc.Undo.Base < (int8*)mem->doc.Undo.Top + BufSize &&
            mem->doc.Undo.Count > 0)
        {
            mem->doc.Undo.Base = mem->doc.Undo.Base->Next;
            mem->doc.Undo.Base->Prev = 0;
            mem->doc.Undo.Count--;
            mem->doc.Undo.CurrentIndex--;
        }

        memcpy(mem->doc.Undo.Top, Buf, (size_t)BufSize);

        if (mem->doc.Undo.Last) { mem->doc.Undo.Last->Next = (UndoData*)mem->doc.Undo.Top; }
        mem->doc.Undo.Last = (UndoData*)mem->doc.Undo.Top;
        mem->doc.Undo.Top = (int8*)mem->doc.Undo.Top + BufSize;
    }

    free(Buf);

    mem->doc.Undo.Current = mem->doc.Undo.Last;
    mem->doc.Undo.Count++;
    mem->doc.Undo.CurrentIndex++;
}

internal void load_from_undo_buffer(PapayaMemory* Mem, bool LoadPreBrushImage)
{
    UndoData Data  = {};
    int8* Image    = 0;
    bool AllocUsed = false;

    memcpy(&Data, Mem->doc.Undo.Current, sizeof(UndoData));

    size_t BytesToRight = (int8*)Mem->doc.Undo.Start + Mem->doc.Undo.Size - (int8*)Mem->doc.Undo.Current;
    size_t ImageSize = (Mem->doc.Undo.Current->IsSubRect ? 8 : 4) * Data.Size.x * Data.Size.y;
    if (BytesToRight - sizeof(UndoData) >= ImageSize) // Image is contiguously stored
    {
        Image = (int8*)Mem->doc.Undo.Current + sizeof(UndoData);
    }
    else // Image is split
    {
        AllocUsed = true;
        Image = (int8*)malloc(ImageSize);
        memcpy(Image, (int8*)Mem->doc.Undo.Current + sizeof(UndoData), (size_t)BytesToRight - sizeof(UndoData));
        memcpy((int8*)Image + BytesToRight - sizeof(UndoData), Mem->doc.Undo.Start, (size_t)(ImageSize - (BytesToRight - sizeof(UndoData))));
    }

    GLCHK( glBindTexture(GL_TEXTURE_2D, Mem->doc.TextureID) );

    GLCHK( glTexSubImage2D(GL_TEXTURE_2D, 0, Data.Pos.x, Data.Pos.y, Data.Size.x, Data.Size.y, GL_RGBA, GL_UNSIGNED_BYTE, Image + (LoadPreBrushImage ? 4 * Data.Size.x * Data.Size.y : 0)) );

    if (AllocUsed) { free(Image); }
}

void core::resize_doc(PapayaMemory* Mem, int32 Width, int32 Height)
{
    // Free existing texture memory
    if (Mem->misc.fbo_sample_tex)
    {
        GLCHK( glDeleteTextures(1, &Mem->misc.fbo_sample_tex) );
    }
    if (Mem->misc.fbo_render_tex)
    {
        GLCHK( glDeleteTextures(1, &Mem->misc.fbo_render_tex) );
    }

    // Allocate new memory
    Mem->misc.fbo_sample_tex = GL::AllocateTexture(Width, Height);
    Mem->misc.fbo_render_tex = GL::AllocateTexture(Width, Height);

    // Set up meshes for rendering to texture
    {
        Vec2 Size = Vec2((float)Width, (float)Height);
        GL::InitQuad(Mem->meshes[PapayaMesh_RTTBrush], Vec2(0,0), Size, GL_STATIC_DRAW);
        GL::InitQuad(Mem->meshes[PapayaMesh_RTTAdd]  , Vec2(0,0), Size, GL_STATIC_DRAW);
    }
}

bool core::open_doc(char* Path, PapayaMemory* Mem)
{
    Timer::StartTime(&Mem->debug.Timers[Timer_ImageOpen]);

    // Load/create image
    {
        uint8* Texture;
        if (Path)
        {
            Texture = stbi_load(Path, &Mem->doc.Width, &Mem->doc.Height,
                    &Mem->doc.ComponentsPerPixel, 4);
        }
        else
        {
            Texture = (uint8*)calloc(1, Mem->doc.Width * Mem->doc.Height * 4);
        }

        if (!Texture) { return false; }

        Mem->doc.TextureID = GL::AllocateTexture(Mem->doc.Width, Mem->doc.Height, Texture);

        Mem->doc.InverseAspect = (float)Mem->doc.Height / (float)Mem->doc.Width;
        Mem->doc.CanvasZoom = 0.8f * Math::Min((float)Mem->window.Width/(float)Mem->doc.Width, (float)Mem->window.Height/(float)Mem->doc.Height);
        if (Mem->doc.CanvasZoom > 1.0f) { Mem->doc.CanvasZoom = 1.0f; }
        // Center canvas
        {
            int32 TopMargin = 53; // TODO: Put common layout constants in struct
            int32 PosX = Math::RoundToInt((Mem->window.Width - (float)Mem->doc.Width * Mem->doc.CanvasZoom) / 2.0f);
            int32 PosY = TopMargin + Math::RoundToInt((Mem->window.Height - TopMargin - (float)Mem->doc.Height * Mem->doc.CanvasZoom) / 2.0f);
            Mem->doc.CanvasPosition = Vec2i(PosX, PosY);
        }
        free(Texture);
    }

    resize_doc(Mem, Mem->doc.Width, Mem->doc.Height);

    // Set up the frame buffer
    {
        // Create a framebuffer object and bind it
        GLCHK( glDisable(GL_BLEND) );
        GLCHK( glGenFramebuffers(1, &Mem->misc.fbo) );
        GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, Mem->misc.fbo) );

        // Attach the color texture to the FBO
        GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->misc.fbo_render_tex, 0) );

        static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
        GLCHK( glDrawBuffers(1, draw_buffers) );

        GLenum FrameBufferStatus = GLCHK( glCheckFramebufferStatus(GL_FRAMEBUFFER) );
        if(FrameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
        {
            // TODO: Log: Frame buffer not initialized correctly
            exit(1);
        }

        GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
    }

    // Projection matrix
    mat4x4_ortho(Mem->doc.ProjMtx, 0.f, (float)Mem->doc.Width, 0.f, (float)Mem->doc.Height, -1.f, 1.f);

    // Init undo buffer
    {
        size_t Size        = 512 * 1024 * 1024;
        size_t MinSize     = 3 * (sizeof(UndoData) + 8 * Mem->doc.Width * Mem->doc.Height);
        Mem->doc.Undo.Size = Math::Max(Size, MinSize);

        Mem->doc.Undo.Start = malloc((size_t)Mem->doc.Undo.Size);
        Mem->doc.Undo.CurrentIndex = -1;

        // TODO: Near-duplicate code from brush release. Combine.
        // Additive render-to-texture
        {
            GLCHK( glDisable(GL_SCISSOR_TEST) );
            GLCHK( glBindFramebuffer     (GL_FRAMEBUFFER, Mem->misc.fbo) );
            GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->misc.fbo_render_tex, 0) );

            GLCHK( glViewport(0, 0, Mem->doc.Width, Mem->doc.Height) );
            GLCHK( glUseProgram(Mem->shaders[PapayaShader_ImGui].Handle) );

            GLCHK( glUniformMatrix4fv(Mem->shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &Mem->doc.ProjMtx[0][0]) );

            GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mem->meshes[PapayaMesh_RTTAdd].VboHandle) );
            GL::SetVertexAttribs(Mem->shaders[PapayaShader_ImGui]);

            GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->doc.TextureID) );
            GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );

            push_undo(Mem, Vec2i(0,0), Vec2i(Mem->doc.Width, Mem->doc.Height), 0, Vec2());

            uint32 Temp = Mem->misc.fbo_render_tex;
            Mem->misc.fbo_render_tex = Mem->doc.TextureID;
            GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->misc.fbo_render_tex, 0) );
            Mem->doc.TextureID = Temp;

            GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
            GLCHK( glViewport(0, 0, Mem->doc.Width, Mem->doc.Height) );

            GLCHK( glDisable(GL_BLEND) );
        }
    }

    Timer::StopTime(&Mem->debug.Timers[Timer_ImageOpen]);

    //TODO: Move this to adjust after cropping and rotation
    Mem->crop_rotate.TopLeft = Vec2(0,0);
    Mem->crop_rotate.BotRight = Vec2((float)Mem->doc.Width, (float)Mem->doc.Height);

    return true;
}

void core::close_doc(PapayaMemory* Mem)
{
    // Document
    if (Mem->doc.TextureID)
    {
        GLCHK( glDeleteTextures(1, &Mem->doc.TextureID) );
        Mem->doc.TextureID = 0;
    }

    // Undo buffer
    {
        free(Mem->doc.Undo.Start);
        Mem->doc.Undo.Start = Mem->doc.Undo.Top     = 0;
        Mem->doc.Undo.Base  = Mem->doc.Undo.Current = Mem->doc.Undo.Last = 0;
        Mem->doc.Undo.Size  = Mem->doc.Undo.Count   = 0;
        Mem->doc.Undo.CurrentIndex = -1;
    }

    // Frame buffer
    if (Mem->misc.fbo)
    {
        GLCHK( glDeleteFramebuffers(1, &Mem->misc.fbo) );
        Mem->misc.fbo = 0;
    }

    if (Mem->misc.fbo_render_tex)
    {
        GLCHK( glDeleteTextures(1, &Mem->misc.fbo_render_tex) );
        Mem->misc.fbo_render_tex = 0;
    }

    if (Mem->misc.fbo_sample_tex)
    {
        GLCHK( glDeleteTextures(1, &Mem->misc.fbo_sample_tex) );
        Mem->misc.fbo_sample_tex = 0;
    }

    // Vertex Buffer: RTTBrush
    if (Mem->meshes[PapayaMesh_RTTBrush].VboHandle)
    {
        GLCHK( glDeleteBuffers(1, &Mem->meshes[PapayaMesh_RTTBrush].VboHandle) );
        Mem->meshes[PapayaMesh_RTTBrush].VboHandle = 0;
    }

    // Vertex Buffer: RTTAdd
    if (Mem->meshes[PapayaMesh_RTTAdd].VboHandle)
    {
        GLCHK( glDeleteBuffers(1, &Mem->meshes[PapayaMesh_RTTAdd].VboHandle) );
        Mem->meshes[PapayaMesh_RTTAdd].VboHandle = 0;
    }
}

void core::init(PapayaMemory* Mem)
{
    // Init values and load textures
    {
        Mem->doc.Width = Mem->doc.Height = 512;

        Mem->current_tool = PapayaTool_CropRotate;

        Mem->brush.Diameter           = 100;
        Mem->brush.MaxDiameter        = 9999;
        Mem->brush.Hardness           = 1.0f;
        Mem->brush.Opacity            = 1.0f;
        Mem->brush.AntiAlias          = true;
        Mem->brush.LineSegmentStartUV = Vec2(-1.0f, -1.0f);

        Picker::Init(&Mem->picker);
        CropRotate::Init(Mem);

        Mem->misc.draw_overlay      = false;
        Mem->misc.show_metrics      = false;
        Mem->misc.show_undo_buffer   = false;
        Mem->misc.menu_open         = false;
        Mem->misc.prefs_open        = false;
        Mem->misc.preview_image_size = false;

        float OrthoMtx[4][4] =
        {
            { 2.0f,   0.0f,   0.0f,   0.0f },
            { 0.0f,  -2.0f,   0.0f,   0.0f },
            { 0.0f,   0.0f,  -1.0f,   0.0f },
            { -1.0f,  1.0f,   0.0f,   1.0f },
        };
        memcpy(Mem->window.ProjMtx, OrthoMtx, sizeof(OrthoMtx));

        Mem->colors[PapayaCol_Clear]             = Color(45, 45, 48);
        Mem->colors[PapayaCol_Workspace]         = Color(30, 30, 30);
        Mem->colors[PapayaCol_Button]            = Color(92, 92, 94);
        Mem->colors[PapayaCol_ButtonHover]       = Color(64, 64, 64);
        Mem->colors[PapayaCol_ButtonActive]      = Color(0, 122, 204);
        Mem->colors[PapayaCol_AlphaGrid1]        = Color(141, 141, 142);
        Mem->colors[PapayaCol_AlphaGrid2]        = Color(92, 92, 94);
        Mem->colors[PapayaCol_ImageSizePreview1] = Color(55, 55, 55);
        Mem->colors[PapayaCol_ImageSizePreview2] = Color(45, 45, 45);
        Mem->colors[PapayaCol_Transparent]       = Color(0, 0, 0, 0);

        // Load and bind image
        {
            uint8* Image;
            int32 ImageWidth, ImageHeight, ComponentsPerPixel;
            Image = stbi_load("ui.png", &ImageWidth, &ImageHeight, &ComponentsPerPixel, 0);

            // Create texture
            GLuint Id_GLuint;
            GLCHK( glGenTextures  (1, &Id_GLuint) );
            GLCHK( glBindTexture  (GL_TEXTURE_2D, Id_GLuint) );
            GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
            GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
            GLCHK( glTexImage2D   (GL_TEXTURE_2D, 0, GL_RGBA8, ImageWidth, ImageHeight, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, Image) );

            // Store our identifier
            free(Image);
            Mem->textures[PapayaTex_UI] = (uint32)Id_GLuint;
        }
    }

    const char* Vert;
    // Vertex shader
    {
        Vert =
            "   #version 120                                        \n"
            "   uniform mat4 ProjMtx;                               \n" // Uniforms[0]
            "                                                       \n"
            "   attribute vec2 Position;                            \n" // Attributes[0]
            "   attribute vec2 UV;                                  \n" // Attributes[1]
            "   attribute vec4 Color;                               \n" // Attributes[2]
            "                                                       \n"
            "   varying vec2 Frag_UV;                               \n"
            "   varying vec4 Frag_Color;                            \n"
            "                                                       \n"
            "   void main()                                         \n"
            "   {                                                   \n"
            "       Frag_UV = UV;                                   \n"
            "       Frag_Color = Color;                             \n"
            "       gl_Position = ProjMtx * vec4(Position.xy,0,1);  \n"
            "   }                                                   \n";
    }

    // Brush shader
    {
        const char* Frag =
            "   #version 120                                                    \n"
            "                                                                   \n"
            "   #define M_PI 3.1415926535897932384626433832795                  \n"
            "                                                                   \n"
            "   uniform sampler2D Texture;                                      \n" // Uniforms[1]
            "   uniform vec2 Pos;                                               \n" // Uniforms[2]
            "   uniform vec2 LastPos;                                           \n" // Uniforms[3]
            "   uniform float Radius;                                           \n" // Uniforms[4]
            "   uniform vec4 BrushColor;                                        \n" // Uniforms[5]
            "   uniform float Hardness;                                         \n" // Uniforms[6]
            "   uniform float InvAspect;                                        \n" // Uniforms[7]
            "                                                                   \n"
            "   varying vec2 Frag_UV;                                           \n"
            "                                                                   \n"
            "   bool isnan( float val )                                         \n"
            "   {                                                               \n"
            "       return ( val < 0.0 || 0.0 < val || val == 0.0 ) ?           \n"
            "       false : true;                                               \n"
            "   }                                                               \n"
            "                                                                   \n"
            "   void line(vec2 p1, vec2 p2, vec2 uv, float radius, 	            \n"
            "             out float distLine, out float distp1)                 \n"
            "   {                                                               \n"
            "       if (distance(p1,p2) <= 0.0)                                 \n"
            "       {                                                           \n"
            "           distLine = distance(uv, p1);                            \n"
            "           distp1 = 0.0;                                           \n"
            "           return;                                                 \n"
            "       }                                                           \n"
            "                                                                   \n"
            "       float a = abs(distance(p1, uv));                            \n"
            "       float b = abs(distance(p2, uv));                            \n"
            "       float c = abs(distance(p1, p2));                            \n"
            "       float d = sqrt(c*c + radius*radius);                        \n"
            "                                                                   \n"
            "       vec2 a1 = normalize(uv - p1);                               \n"
            "       vec2 b1 = normalize(uv - p2);                               \n"
            "       vec2 c1 = normalize(p2 - p1);                               \n"
            "       if (dot(a1,c1) < 0.0)                                       \n"
            "       {                                                           \n"
            "           distLine = a;                                           \n"
            "           distp1 = 0.0;                                           \n"
            "           return;                                                 \n"
            "       }                                                           \n"
            "                                                                   \n"
            "       if (dot(b1,c1) > 0.0)                                       \n"
            "       {                                                           \n"
            "           distLine = b;                                           \n"
            "           distp1 = 1.0;                                           \n"
            "           return;                                                 \n"
            "       }                                                           \n"
            "                                                                   \n"
            "       float p = (a + b + c) * 0.5;                                \n"
            "       float h = 2.0 / c * sqrt( p * (p-a) * (p-b) * (p-c) );      \n"
            "                                                                   \n"
            "       if (isnan(h))                                               \n"
            "       {                                                           \n"
            "           distLine = 0.0;                                         \n"
            "           distp1 = a / c;                                         \n"
            "       }                                                           \n"
            "       else                                                        \n"
            "       {                                                           \n"
            "           distLine = h;                                           \n"
            "           distp1 = sqrt(a*a - h*h) / c;                           \n"
            "       }                                                           \n"
            "   }                                                               \n"
            "                                                                   \n"
            "   void main()                                                     \n"
            "   {                                                               \n"
            "       vec4 t = texture2D(Texture, Frag_UV.st);                    \n"
            "                                                                   \n"
            "       float distLine, distp1;                                     \n"
            "       vec2 aspectUV = vec2(Frag_UV.x, Frag_UV.y * InvAspect);     \n"
            "       line(LastPos, Pos, aspectUV, Radius, distLine, distp1);     \n"
            "                                                                   \n"
            "       float Scale = 1.0 / (2.0 * Radius * (1.0 - Hardness));      \n"
            "       float Period = M_PI * Scale;                                \n"
            "       float Phase = (1.0 - Scale * 2.0 * Radius) * M_PI * 0.5;    \n"
            "       float Alpha = cos((Period * distLine) + Phase);             \n"
            "       if (distLine < Radius - (0.5/Scale)) Alpha = 1.0;           \n"
            "       if (distLine > Radius) Alpha = 0.0;                         \n"
            "                                                                   \n"
            "       float FinalAlpha = max(t.a, Alpha * BrushColor.a);          \n"
            "                                                                   \n"
            "       gl_FragColor = vec4(BrushColor.r,                           \n"
            "                           BrushColor.g,                           \n"
            "                           BrushColor.b,                           \n"
            "                           clamp(FinalAlpha,0.0,1.0));             \n" // TODO: Needs improvement. Self-intersection corners look weird.
            "   }                                                               \n";

        GL::CompileShader(Mem->shaders[PapayaShader_Brush], __FILE__, __LINE__,
            Vert, Frag, 2, 8,
            "Position", "UV",
            "ProjMtx", "Texture", "Pos", "LastPos", "Radius", "BrushColor", "Hardness", "InvAspect");

    }

    // Brush cursor shader
    {
        const char* Frag =
            "   #version 120                                                                \n"
            "                                                                               \n"
            "   #define M_PI 3.1415926535897932384626433832795                              \n"
            "                                                                               \n"
            "   uniform vec4 BrushColor;                                                    \n" // Uniforms[1]
            "   uniform float Hardness;                                                     \n" // Uniforms[2]
            "   uniform float PixelDiameter;                                                \n" // Uniforms[3]
            "                                                                               \n"
            "   varying vec2 Frag_UV;                                                       \n"
            "                                                                               \n"
            "                                                                               \n"
            "   void main()                                                                 \n"
            "   {                                                                           \n"
            "       float Scale = 1.0 / (1.0 - Hardness);                                   \n"
            "       float Period = M_PI * Scale;                                            \n"
            "       float Phase = (1.0 - Scale) * M_PI * 0.5;                               \n"
            "       float Dist = distance(Frag_UV,vec2(0.5,0.5));                           \n"
            "       float Alpha = cos((Period * Dist) + Phase);                             \n"
            "       if (Dist < 0.5 - (0.5/Scale)) Alpha = 1.0;                              \n"
            "       else if (Dist > 0.5)          Alpha = 0.0;                              \n"
            "       float BorderThickness = 1.0 / PixelDiameter;                            \n"
            "       gl_FragColor = (Dist > 0.5 - BorderThickness && Dist < 0.5) ?           \n"
            "       vec4(0.0,0.0,0.0,1.0) :                                                 \n"
            "       vec4(BrushColor.r, BrushColor.g, BrushColor.b, 	Alpha * BrushColor.a);  \n"
            "   }                                                                           \n";

        GL::CompileShader(Mem->shaders[PapayaShader_BrushCursor], __FILE__, __LINE__,
            Vert, Frag, 2, 4,
            "Position", "UV",
            "ProjMtx", "BrushColor", "Hardness", "PixelDiameter");

        GL::InitQuad(Mem->meshes[PapayaMesh_BrushCursor],
            Vec2(40, 60), Vec2(30, 30), GL_DYNAMIC_DRAW);
    }

    // Eyedropper cursor shader
    {
        const char* Frag =
            "   #version 120                                                                \n"
            "                                                                               \n"
            "   uniform vec4 Color1;                                                        \n" // Uniforms[1]
            "   uniform vec4 Color2;                                                        \n" // Uniforms[2]
            "                                                                               \n"
            "   varying vec2 Frag_UV;                                                       \n"
            "                                                                               \n"
            "   void main()                                                                 \n"
            "   {                                                                           \n"
            "       float d = length(vec2(0.5,0.5) - Frag_UV);                              \n"
            "       float t = 1.0 - clamp((d - 0.49) * 250.0, 0.0, 1.0);                    \n"
            "       t = t - 1.0 + clamp((d - 0.4) * 250.0, 0.0, 1.0);                       \n"
            "       gl_FragColor = (Frag_UV.y < 0.5) ? Color1 : Color2;                     \n"
            "       gl_FragColor.a = t;                                                     \n"
            "   }                                                                           \n";

        GL::CompileShader(Mem->shaders[PapayaShader_EyeDropperCursor], __FILE__, __LINE__,
            Vert, Frag, 2, 3,
            "Position", "UV",
            "ProjMtx", "Color1", "Color2");

        GL::InitQuad(Mem->meshes[PapayaMesh_EyeDropperCursor],
            Vec2(40, 60), Vec2(30, 30), GL_DYNAMIC_DRAW);
    }

    // Picker saturation-value shader
    {
        const char* Frag =
            "   #version 120                                                            \n"
            "                                                                           \n"
            "   uniform float Hue;                                                      \n" // Uniforms[1]
            "   uniform vec2 Cursor;                                                    \n" // Uniforms[2]
            "   uniform float Thickness = 1.0 / 256.0;                                  \n"
            "   uniform float Radius = 0.0075;                                          \n"
            "                                                                           \n"
            "   varying  vec2 Frag_UV;                                                  \n"
            "                                                                           \n"
            "   vec3 hsv2rgb(vec3 c)                                                    \n" // Source: Fast branchless RGB to HSV conversion in GLSL
            "   {                                                                       \n" // http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
            "       vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);                      \n"
            "       vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);                   \n"
            "       return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);           \n"
            "   }                                                                       \n"
            "                                                                           \n"
            "   void main()                                                             \n"
            "   {                                                                       \n"
            "       vec3 RGB = hsv2rgb(vec3(Hue, Frag_UV.x, 1.0 - Frag_UV.y));          \n"
            "       vec2 CursorInv = vec2(Cursor.x, 1.0 - Cursor.y);                    \n"
            "       float Dist = distance(Frag_UV, CursorInv);                          \n"
            "                                                                           \n"
            "       if (Dist > Radius && Dist < Radius + Thickness)                     \n"
            "       {                                                                   \n"
            "           float a = (CursorInv.x < 0.4 && CursorInv.y > 0.6) ? 0.0 : 1.0; \n"
            "           gl_FragColor = vec4(a, a, a, 1.0);                              \n"
            "       }                                                                   \n"
            "       else                                                                \n"
            "       {                                                                   \n"
            "           gl_FragColor = vec4(RGB.x, RGB.y, RGB.z, 1.0);                  \n"
            "       }                                                                   \n"
            "   }                                                                       \n";

        GL::CompileShader(Mem->shaders[PapayaShader_PickerSVBox], __FILE__, __LINE__,
            Vert, Frag, 2, 3,
            "Position", "UV",
            "ProjMtx", "Hue", "Cursor");

        GL::InitQuad(Mem->meshes[PapayaMesh_PickerSVBox],
            Mem->picker.Pos + Mem->picker.SVBoxPos, Mem->picker.SVBoxSize, GL_STATIC_DRAW);
    }

    // Picker hue shader
    {
        const char* Frag =
            "   #version 120                                                    \n"
            "                                                                   \n"
            "   uniform float Cursor;                                           \n" // Uniforms[1]
            "                                                                   \n"
            "   varying  vec2 Frag_UV;                                          \n"
            "                                                                   \n"
            "   vec3 hsv2rgb(vec3 c)                                            \n" // Source: Fast branchless RGB to HSV conversion in GLSL
            "   {                                                               \n" // http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
            "       vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);              \n"
            "       vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);           \n"
            "       return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);   \n"
            "   }                                                               \n"
            "                                                                   \n"
            "   void main()                                                     \n"
            "   {                                                               \n"
            "       vec4 Hue = vec4(hsv2rgb(vec3(1.0-Frag_UV.y, 1.0, 1.0))      \n"
            "                  .xyz,1.0);                                       \n"
            "       if (abs(0.5 - Frag_UV.x) > 0.3333)                          \n"
            "       {                                                           \n"
            "           gl_FragColor = vec4(0.36,0.36,0.37,                     \n"
            "                       float(abs(1.0-Frag_UV.y-Cursor) < 0.0039)); \n"
            "       }                                                           \n"
            "       else                                                        \n"
            "           gl_FragColor = Hue;                                     \n"
            "   }                                                               \n";

        GL::CompileShader(Mem->shaders[PapayaShader_PickerHStrip], __FILE__, __LINE__,
            Vert, Frag, 2, 2,
            "Position", "UV",
            "ProjMtx", "Cursor");

        GL::InitQuad(Mem->meshes[PapayaMesh_PickerHStrip],
            Mem->picker.Pos + Mem->picker.HueStripPos, Mem->picker.HueStripSize, GL_STATIC_DRAW);
    }

    // Imaze size preview shader
    {
        const char* Frag =
            "   #version 120                                                    \n"
            "                                                                   \n"
            "   uniform vec4  Color1;                                           \n" // Uniforms[1]
            "   uniform vec4  Color2;                                           \n" // Uniforms[2]
            "   uniform float Width;                                            \n" // Uniforms[3]
            "   uniform float Height;                                           \n" // Uniforms[4]
            "                                                                   \n"
            "   varying  vec2 Frag_UV;                                          \n"
            "                                                                   \n"
            "   void main()                                                     \n"
            "   {                                                               \n"
            "       float d = mod(Frag_UV.x * Width + Frag_UV.y * Height, 150); \n"
            "       if (d < 75)                                                 \n"
            "           gl_FragColor = Color1;                                  \n"
            "       else                                                        \n"
            "           gl_FragColor = Color2;                                  \n"
            "   }                                                               \n";

        GL::CompileShader(Mem->shaders[PapayaShader_ImageSizePreview], __FILE__, __LINE__,
            Vert, Frag, 2, 5,
            "Position", "UV",
            "ProjMtx", "Color1", "Color2", "Width", "Height");

        GL::InitQuad(Mem->meshes[PapayaMesh_ImageSizePreview],
            Vec2(0,0), Vec2(10,10), GL_DYNAMIC_DRAW);
    }

    // Alpha grid shader
    {
        const char* Frag =
            "   #version 120                                                    \n"
            "                                                                   \n"
            "   uniform vec4  Color1;                                           \n" // Uniforms[1]
            "   uniform vec4  Color2;                                           \n" // Uniforms[2]
            "   uniform float Zoom;                                             \n" // Uniforms[3]
            "   uniform float InvAspect;                                        \n" // Uniforms[4]
            "   uniform float MaxDim;                                           \n" // Uniforms[5]
            "                                                                   \n"
            "   varying  vec2 Frag_UV;                                          \n"
            "                                                                   \n"
            "   void main()                                                     \n"
            "   {                                                               \n"
            "       vec2 aspectUV;                                              \n"
            "       if (InvAspect < 1.0)                                        \n"
            "           aspectUV = vec2(Frag_UV.x, Frag_UV.y * InvAspect);      \n"
            "       else                                                        \n"
            "           aspectUV = vec2(Frag_UV.x / InvAspect, Frag_UV.y);      \n"
            "       vec2 uv = floor(aspectUV.xy * 0.1 * MaxDim * Zoom);         \n"
            "       float a = mod(uv.x + uv.y, 2.0);                            \n"
            "       gl_FragColor = mix(Color1, Color2, a);                      \n"
            "   }                                                               \n";

        GL::CompileShader(Mem->shaders[PapayaShader_AlphaGrid], __FILE__, __LINE__,
            Vert, Frag, 2, 6,
            "Position", "UV",
            "ProjMtx", "Color1", "Color2", "Zoom", "InvAspect", "MaxDim");

        GL::InitQuad(Mem->meshes[PapayaMesh_AlphaGrid],
            Vec2(0,0), Vec2(10,10), GL_DYNAMIC_DRAW);
    }

    // PreMultiply Alpha shader
    {
        const char* Frag =
            "   #version 120                                                        \n"
            "   uniform sampler2D Texture;                                          \n" // Uniforms[1]
            "                                                                       \n"
            "   varying vec2 Frag_UV;                                               \n"
            "   varying vec4 Frag_Color;                                            \n"
            "                                                                       \n"
            "   void main()                                                         \n"
            "   {                                                                   \n"
            "       vec4 col = Frag_Color * texture2D( Texture, Frag_UV.st);        \n"
            "       gl_FragColor = vec4(col.r, col.g, col.b, 1.0) * col.a;          \n"
            "   }                                                                   \n";

        GL::CompileShader(Mem->shaders[PapayaShader_PreMultiplyAlpha], __FILE__, __LINE__,
            Vert, Frag, 3, 2,
            "Position", "UV", "Color",
            "ProjMtx", "Texture");
    }

    // DeMultiply Alpha shader
    {
        const char* Frag =
            "   #version 120                                                        \n"
            "   uniform sampler2D Texture;                                          \n" // Uniforms[1]
            "                                                                       \n"
            "   varying vec2 Frag_UV;                                               \n"
            "   varying vec4 Frag_Color;                                            \n"
            "                                                                       \n"
            "   void main()                                                         \n"
            "   {                                                                   \n"
            "       vec4 col = Frag_Color * texture2D( Texture, Frag_UV.st);        \n"
            "       gl_FragColor = vec4(col.rgb/col.a, col.a);                      \n"
            "   }                                                                   \n";

        GL::CompileShader(Mem->shaders[PapayaShader_DeMultiplyAlpha], __FILE__, __LINE__,
            Vert, Frag, 3, 2,
            "Position", "UV", "Color",
            "ProjMtx", "Texture");
    }

    // ImGui default shader
    {
        const char* Frag =
            "   #version 120                                                        \n"
            "   uniform sampler2D Texture;                                          \n" // Uniforms[1]
            "                                                                       \n"
            "   varying vec2 Frag_UV;                                               \n"
            "   varying vec4 Frag_Color;                                            \n"
            "                                                                       \n"
            "   void main()                                                         \n"
            "   {                                                                   \n"
            "       gl_FragColor = Frag_Color * texture2D( Texture, Frag_UV.st);    \n"
            "   }                                                                   \n";

        GL::CompileShader(Mem->shaders[PapayaShader_ImGui], __FILE__, __LINE__,
            Vert, Frag, 3, 2,
            "Position", "UV", "Color",
            "ProjMtx", "Texture");

        GL::InitQuad(Mem->meshes[PapayaMesh_Canvas],
            Vec2(0,0), Vec2(10,10), GL_DYNAMIC_DRAW);
    }

    // Unlit shader
    {
        const char* Frag =
            "   #version 120                                                        \n"
            "   uniform sampler2D Texture;                                          \n" // Uniforms[1]
            "                                                                       \n"
            "   varying vec2 Frag_UV;                                               \n"
            "   varying vec4 Frag_Color;                                            \n"
            "                                                                       \n"
            "   void main()                                                         \n"
            "   {                                                                   \n"
            "       gl_FragColor = Frag_Color;    \n"
            "   }                                                                   \n";

        GL::CompileShader(Mem->shaders[PapayaShader_VertexColor], __FILE__, __LINE__,
            Vert, Frag, 3, 2,
            "Position", "UV", "Color",
            "ProjMtx", "Texture");

        GL::InitQuad(Mem->meshes[PapayaMesh_Canvas],
            Vec2(0,0), Vec2(10,10), GL_DYNAMIC_DRAW);
    }

    // Setup for ImGui
    {
        GLCHK( glGenBuffers(1, &Mem->meshes[PapayaMesh_ImGui].VboHandle) );
        GLCHK( glGenBuffers(1, &Mem->meshes[PapayaMesh_ImGui].ElementsHandle) );

        // Create fonts texture
        ImGuiIO& io = ImGui::GetIO();

        uint8* pixels;
        int32 width, height;
        //ImFont* my_font0 = io.Fonts->AddFontFromFileTTF("d:\\DroidSans.ttf", 15.0f);
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        GLCHK( glGenTextures  (1, &Mem->textures[PapayaTex_Font]) );
        GLCHK( glBindTexture  (GL_TEXTURE_2D, Mem->textures[PapayaTex_Font]) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
        GLCHK( glTexImage2D   (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );

        // Store our identifier
        io.Fonts->TexID = (void *)(intptr_t)Mem->textures[PapayaTex_Font];

        // Cleanup
        io.Fonts->ClearInputData();
        io.Fonts->ClearTexData();
    }

    // ImGui Style Settings
    {
        ImGuiStyle& Style = ImGui::GetStyle();
        Style.WindowFillAlphaDefault = 1.0f;
        // TODO: Move repeated stuff here by setting global style
    }
}

void core::destroy(PapayaMemory* Mem)
{
    //TODO: Free stuff
}

void core::resize(PapayaMemory* Mem, int32 Width, int32 Height)
{
    Mem->window.Width = Width;
    Mem->window.Height = Height;
    ImGui::GetIO().DisplaySize = ImVec2((float)Width, (float)Height);

    // TODO: Intelligent centering. Recenter canvas only if the image was centered
    //       before window was resized.
    // Center canvas
    int32 TopMargin = 53; // TODO: Put common layout constants in struct
    int32 PosX = Math::RoundToInt((Mem->window.Width - (float)Mem->doc.Width * Mem->doc.CanvasZoom) / 2.0f);
    int32 PosY = TopMargin + Math::RoundToInt((Mem->window.Height - TopMargin - (float)Mem->doc.Height * Mem->doc.CanvasZoom) / 2.0f);
    Mem->doc.CanvasPosition = Vec2i(PosX, PosY);
}

void core::update(PapayaMemory* Mem)
{
    // Initialize frame
    {
        // Current mouse info
        {
            Mem->mouse.Pos = Math::RoundToVec2i(ImGui::GetMousePos());
            Vec2 MousePixelPos = Vec2(Math::Floor((Mem->mouse.Pos.x - Mem->doc.CanvasPosition.x) / Mem->doc.CanvasZoom),
                                      Math::Floor((Mem->mouse.Pos.y - Mem->doc.CanvasPosition.y) / Mem->doc.CanvasZoom));
            Mem->mouse.UV = Vec2(MousePixelPos.x / (float) Mem->doc.Width, MousePixelPos.y / (float) Mem->doc.Height);

            for (int32 i = 0; i < 3; i++)
            {
                Mem->mouse.IsDown[i]   = ImGui::IsMouseDown(i);
                Mem->mouse.Pressed[i]  = (Mem->mouse.IsDown[i] && !Mem->mouse.WasDown[i]);
                Mem->mouse.Released[i] = (!Mem->mouse.IsDown[i] && Mem->mouse.WasDown[i]);
            }

            // OnCanvas test
            {
                Mem->mouse.InWorkspace = true;

                if (Mem->mouse.Pos.x <= 34 ||                      // Document workspace test
                    Mem->mouse.Pos.x >= Mem->window.Width - 3 ||   // TODO: Formalize the window layout and
                    Mem->mouse.Pos.y <= 55 ||                      //       remove magic numbers throughout
                    Mem->mouse.Pos.y >= Mem->window.Height - 3)    //       the code.
                {
                    Mem->mouse.InWorkspace = false;
                }
                else if (Mem->picker.Open &&
                    Mem->mouse.Pos.x > Mem->picker.Pos.x &&                       // Color picker test
                    Mem->mouse.Pos.x < Mem->picker.Pos.x + Mem->picker.Size.x &&  //
                    Mem->mouse.Pos.y > Mem->picker.Pos.y &&                       //
                    Mem->mouse.Pos.y < Mem->picker.Pos.y + Mem->picker.Size.y)    //
                {
                    Mem->mouse.InWorkspace = false;
                }
            }
        }

        // Clear screen buffer
        {
            GLCHK( glViewport(0, 0, (int32)ImGui::GetIO().DisplaySize.x, (int32)ImGui::GetIO().DisplaySize.y) );

            GLCHK( glClearColor(Mem->colors[PapayaCol_Clear].r,
                Mem->colors[PapayaCol_Clear].g,
                Mem->colors[PapayaCol_Clear].b, 1.0f) );
            GLCHK( glClear(GL_COLOR_BUFFER_BIT) );

            GLCHK( glEnable(GL_SCISSOR_TEST) );
            GLCHK( glScissor(34, 3,
                (int32)Mem->window.Width  - 70,
                (int32)Mem->window.Height - 58) ); // TODO: Remove magic numbers

            GLCHK( glClearColor(Mem->colors[PapayaCol_Workspace].r,
                Mem->colors[PapayaCol_Workspace].g,
                Mem->colors[PapayaCol_Workspace].b, 1.0f) );
            GLCHK( glClear(GL_COLOR_BUFFER_BIT) );

            GLCHK( glDisable(GL_SCISSOR_TEST) );
        }

        // Set projection matrix
        Mem->window.ProjMtx[0][0] =  2.0f / ImGui::GetIO().DisplaySize.x;
        Mem->window.ProjMtx[1][1] = -2.0f / ImGui::GetIO().DisplaySize.y;
    }

    // Title Bar Menu
    {
        ImGui::SetNextWindowSize(ImVec2(Mem->window.Width - Mem->window.MenuHorizontalOffset - Mem->window.TitleBarButtonsWidth - 3.0f,
            Mem->window.TitleBarHeight - 10.0f));
        ImGui::SetNextWindowPos(ImVec2(2.0f + Mem->window.MenuHorizontalOffset, 6.0f));

        ImGuiWindowFlags WindowFlags = 0;
        WindowFlags |= ImGuiWindowFlags_NoTitleBar;
        WindowFlags |= ImGuiWindowFlags_NoResize;
        WindowFlags |= ImGuiWindowFlags_NoMove;
        WindowFlags |= ImGuiWindowFlags_NoScrollbar;
        WindowFlags |= ImGuiWindowFlags_NoCollapse;
        WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;
        WindowFlags |= ImGuiWindowFlags_MenuBar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding   , 0);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding     , ImVec2(0, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing , ImVec2(5, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing      , ImVec2(8, 8));

        ImGui::PushStyleColor(ImGuiCol_WindowBg           , Mem->colors[PapayaCol_Transparent]);
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg          , Mem->colors[PapayaCol_Transparent]);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered      , Mem->colors[PapayaCol_ButtonHover]);
        ImGui::PushStyleVar  (ImGuiStyleVar_WindowPadding , ImVec2(8, 4));
        ImGui::PushStyleColor(ImGuiCol_Header             , Mem->colors[PapayaCol_Transparent]);

        Mem->misc.menu_open = false;

        ImGui::Begin("Title Bar Menu", 0, WindowFlags);
        if (ImGui::BeginMenuBar())
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, Mem->colors[PapayaCol_Clear]);
            if (ImGui::BeginMenu("FILE"))
            {
                Mem->misc.menu_open = true;

                if (Mem->doc.TextureID) // A document is already open
                {
                    if (ImGui::MenuItem("Close")) { close_doc(Mem); }
                    if (ImGui::MenuItem("Save"))
                    {
                        char* Path = platform::save_file_dialog();
                        uint8* Texture = (uint8*)malloc(4 * Mem->doc.Width * Mem->doc.Height);
                        if (Path) // TODO: Do this on a separate thread. Massively blocks UI for large images.
                        {
                            GLCHK(glBindTexture(GL_TEXTURE_2D, Mem->doc.TextureID));
                            GLCHK(glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, Texture));

                            int32 Result = stbi_write_png(Path, Mem->doc.Width, Mem->doc.Height, 4, Texture, 4 * Mem->doc.Width);
                            if (!Result)
                            {
                                // TODO: Log: Save failed
                                platform::print("Save failed\n");
                            }

                            free(Texture);
                            free(Path);
                        }
                    }
                }
                else // No document open
                {
                    if (ImGui::MenuItem("Open"))
                    {
                        char* Path = platform::open_file_dialog();
                        if (Path)
                        {
                            open_doc(Path, Mem);
                            free(Path);
                        }
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Quit", "Alt+F4")) { Mem->is_running = false; }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("EDIT"))
            {
                Mem->misc.menu_open = true;
                if (ImGui::MenuItem("Preferences...", 0)) { Mem->misc.prefs_open = true; }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("VIEW"))
            {
                Mem->misc.menu_open = true;
                ImGui::MenuItem("Metrics Window", NULL, &Mem->misc.show_metrics);
                ImGui::MenuItem("Undo Buffer Window", NULL, &Mem->misc.show_undo_buffer);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
            ImGui::PopStyleColor();
        }
        ImGui::End();

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(5);
    }

    // Left toolbar
    {
        ImGui::SetNextWindowSize(ImVec2(36, 650));
        ImGui::SetNextWindowPos (ImVec2( 1, 57));

        ImGuiWindowFlags WindowFlags = 0;
        WindowFlags |= ImGuiWindowFlags_NoTitleBar;
        WindowFlags |= ImGuiWindowFlags_NoResize;
        WindowFlags |= ImGuiWindowFlags_NoMove;
        WindowFlags |= ImGuiWindowFlags_NoScrollbar;
        WindowFlags |= ImGuiWindowFlags_NoCollapse;
        WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding   , 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding    , ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding     , ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing , ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing      , ImVec2(0, 0));

        ImGui::PushStyleColor(ImGuiCol_ButtonActive  , Mem->colors[PapayaCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_WindowBg      , Mem->colors[PapayaCol_Transparent]);

        ImGui::Begin("Left toolbar", 0, WindowFlags);

#define CALCUV(X, Y) ImVec2((float)X/256.0f, (float)Y/256.0f)
        {
            ImGui::PushID(0);
            ImGui::PushStyleColor(ImGuiCol_Button       , (Mem->current_tool == PapayaTool_Brush) ? Mem->colors[PapayaCol_Button] :  Mem->colors[PapayaCol_Transparent]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (Mem->current_tool == PapayaTool_Brush) ? Mem->colors[PapayaCol_Button] :  Mem->colors[PapayaCol_ButtonHover]);
            if (ImGui::ImageButton((void*)(intptr_t)Mem->textures[PapayaTex_UI], ImVec2(20, 20), CALCUV(0, 0), CALCUV(20, 20), 6, ImVec4(0, 0, 0, 0)))
            {
                Mem->current_tool = (Mem->current_tool != PapayaTool_Brush) ? PapayaTool_Brush : PapayaTool_None;

            }
            ImGui::PopStyleColor(2);
            ImGui::PopID();

            ImGui::PushID(1);
            ImGui::PushStyleColor(ImGuiCol_Button       , (Mem->current_tool == PapayaTool_EyeDropper) ? Mem->colors[PapayaCol_Button] :  Mem->colors[PapayaCol_Transparent]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (Mem->current_tool == PapayaTool_EyeDropper) ? Mem->colors[PapayaCol_Button] :  Mem->colors[PapayaCol_ButtonHover]);
            if (ImGui::ImageButton((void*)(intptr_t)Mem->textures[PapayaTex_UI], ImVec2(20, 20), CALCUV(20, 0), CALCUV(40, 20), 6, ImVec4(0, 0, 0, 0)))
            {
                Mem->current_tool = (Mem->current_tool != PapayaTool_EyeDropper) ? PapayaTool_EyeDropper : PapayaTool_None;
            }
            ImGui::PopStyleColor(2);
            ImGui::PopID();

            ImGui::PushID(2);
            ImGui::PushStyleColor(ImGuiCol_Button       , (Mem->current_tool == PapayaTool_CropRotate) ? Mem->colors[PapayaCol_Button] :  Mem->colors[PapayaCol_Transparent]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (Mem->current_tool == PapayaTool_CropRotate) ? Mem->colors[PapayaCol_Button] :  Mem->colors[PapayaCol_ButtonHover]);
            if (ImGui::ImageButton((void*)(intptr_t)Mem->textures[PapayaTex_UI], ImVec2(20, 20), CALCUV(40, 0), CALCUV(60, 20), 6, ImVec4(0, 0, 0, 0)))
            {
                Mem->current_tool = (Mem->current_tool != PapayaTool_CropRotate) ? PapayaTool_CropRotate : PapayaTool_None;
            }
            ImGui::PopStyleColor(2);
            ImGui::PopID();

            ImGui::PushID(3);
            if (ImGui::ImageButton((void*)(intptr_t)Mem->textures[PapayaTex_UI], ImVec2(33, 33), CALCUV(0, 0), CALCUV(0, 0), 0, Mem->picker.CurrentColor))
            {
                Mem->picker.Open = !Mem->picker.Open;
                Picker::SetColor(Mem->picker.CurrentColor, &Mem->picker);
            }
            ImGui::PopID();
        }
#undef CALCUV

        ImGui::End();

        ImGui::PopStyleVar(5);
        ImGui::PopStyleColor(2);
    }

    // Right toolbar
    {
        ImGui::SetNextWindowSize(ImVec2(36, 650));
        ImGui::SetNextWindowPos (ImVec2((float)Mem->window.Width - 36, 57));

        ImGuiWindowFlags WindowFlags = 0;
        WindowFlags |= ImGuiWindowFlags_NoTitleBar;
        WindowFlags |= ImGuiWindowFlags_NoResize;
        WindowFlags |= ImGuiWindowFlags_NoMove;
        WindowFlags |= ImGuiWindowFlags_NoScrollbar;
        WindowFlags |= ImGuiWindowFlags_NoCollapse;
        WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding   , 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding    , ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding     , ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing , ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing      , ImVec2(0, 0));

        ImGui::PushStyleColor(ImGuiCol_ButtonActive  , Mem->colors[PapayaCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_WindowBg      , Mem->colors[PapayaCol_Transparent]);

        ImGui::Begin("Right toolbar", 0, WindowFlags);

#define CALCUV(X, Y) ImVec2((float)X/256.0f, (float)Y/256.0f)
        {
            ImGui::PushID(0);
            ImGui::PushStyleColor(ImGuiCol_Button       , (Mem->misc.prefs_open) ? Mem->colors[PapayaCol_Button] :  Mem->colors[PapayaCol_Transparent]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (Mem->misc.prefs_open) ? Mem->colors[PapayaCol_Button] :  Mem->colors[PapayaCol_ButtonHover]);
            if (ImGui::ImageButton((void*)(intptr_t)Mem->textures[PapayaTex_UI], ImVec2(20, 20), CALCUV(40, 0), CALCUV(60, 20), 6, ImVec4(0, 0, 0, 0)))
            {
                Mem->misc.prefs_open = !Mem->misc.prefs_open;
            }
            ImGui::PopStyleColor(2);
            ImGui::PopID();
        }
#undef CALCUV

        ImGui::End();

        ImGui::PopStyleVar(5);
        ImGui::PopStyleColor(2);
    }

    if (Mem->misc.prefs_open) { Prefs::ShowPanel(&Mem->picker, Mem->colors, Mem->window); }

    // Color Picker
    if (Mem->picker.Open)
    {
        Picker::UpdateAndRender(&Mem->picker, Mem->colors,
                Mem->mouse, Mem->textures[PapayaTex_UI]);
    }

    // Tool Param Bar
    {
        ImGui::SetNextWindowSize(ImVec2((float)Mem->window.Width - 70, 30));
        ImGui::SetNextWindowPos(ImVec2(34, 30));

        ImGuiWindowFlags WindowFlags = 0;
        WindowFlags |= ImGuiWindowFlags_NoTitleBar;
        WindowFlags |= ImGuiWindowFlags_NoResize;
        WindowFlags |= ImGuiWindowFlags_NoMove;
        WindowFlags |= ImGuiWindowFlags_NoScrollbar;
        WindowFlags |= ImGuiWindowFlags_NoCollapse;
        WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse; // TODO: Once the overall look has been established, make commonly used templates

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding , 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding  , ImVec2( 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing    , ImVec2(30, 0));

        ImGui::PushStyleColor(ImGuiCol_Button           , Mem->colors[PapayaCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered    , Mem->colors[PapayaCol_ButtonHover]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive     , Mem->colors[PapayaCol_ButtonActive]);
        ImGui::PushStyleColor(ImGuiCol_WindowBg         , Mem->colors[PapayaCol_Transparent]);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive , Mem->colors[PapayaCol_ButtonActive]);

        bool Show = true;
        ImGui::Begin("Tool param bar", &Show, WindowFlags);

        // New document options. Might convert into modal window later.
        if (!Mem->doc.TextureID) // No document is open
        {
            // Size
            {
                int32 size[2];
                size[0] = Mem->doc.Width;
                size[1] = Mem->doc.Height;
                ImGui::PushItemWidth(85);
                ImGui::InputInt2("Size", size);
                ImGui::PopItemWidth();
                Mem->doc.Width  = Math::Clamp(size[0], 1, 9000);
                Mem->doc.Height = Math::Clamp(size[1], 1, 9000);
                ImGui::SameLine();
                ImGui::Checkbox("Preview", &Mem->misc.preview_image_size);
            }

            // "New" button
            {
                ImGui::SameLine(ImGui::GetWindowWidth() - 70); // TODO: Magic number alert
                if (ImGui::Button("New Image"))
                {
                    Mem->doc.ComponentsPerPixel = 4;
                    open_doc(0, Mem);
                }
            }
        }
        else  // Document is open
        {
            if (Mem->current_tool == PapayaTool_Brush)
            {
                ImGui::PushItemWidth(85);
                ImGui::InputInt("Diameter", &Mem->brush.Diameter);
                Mem->brush.Diameter = Math::Clamp(Mem->brush.Diameter, 1, Mem->brush.MaxDiameter);

                ImGui::PopItemWidth();
                ImGui::PushItemWidth(80);
                ImGui::SameLine();

                float ScaledHardness = Mem->brush.Hardness * 100.0f;
                ImGui::SliderFloat("Hardness", &ScaledHardness, 0.0f, 100.0f, "%.0f");
                Mem->brush.Hardness = ScaledHardness / 100.0f;
                ImGui::SameLine();

                float ScaledOpacity = Mem->brush.Opacity * 100.0f;
                ImGui::SliderFloat("Opacity", &ScaledOpacity, 0.0f, 100.0f, "%.0f");
                Mem->brush.Opacity = ScaledOpacity / 100.0f;
                ImGui::SameLine();

                ImGui::Checkbox("Anti-alias", &Mem->brush.AntiAlias); // TODO: Replace this with a toggleable icon button

                ImGui::PopItemWidth();
            }
            else if (Mem->current_tool == PapayaTool_CropRotate)
            {
                CropRotate::Toolbar(Mem);
            }
        }

        ImGui::End();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(5);
    }

    // Image size preview
    if (!Mem->doc.TextureID && Mem->misc.preview_image_size)
    {
        int32 TopMargin = 53; // TODO: Put common layout constants in struct
        GL::TransformQuad(Mem->meshes[PapayaMesh_ImageSizePreview],
            Vec2((float)(Mem->window.Width - Mem->doc.Width) / 2, TopMargin + (float)(Mem->window.Height - TopMargin - Mem->doc.Height) / 2),
            Vec2((float)Mem->doc.Width, (float)Mem->doc.Height));

        GL::DrawMesh(Mem->meshes[PapayaMesh_ImageSizePreview], Mem->shaders[PapayaShader_ImageSizePreview], true,
            5,
            UniformType_Matrix4, &Mem->window.ProjMtx[0][0],
            UniformType_Color, Mem->colors[PapayaCol_ImageSizePreview1],
            UniformType_Color, Mem->colors[PapayaCol_ImageSizePreview2],
            UniformType_Float, (float) Mem->doc.Width,
            UniformType_Float, (float) Mem->doc.Height);
    }

    if (!Mem->doc.TextureID) { goto EndOfDoc; }

    // Brush tool
    if (Mem->current_tool == PapayaTool_Brush &&
            !Mem->misc.menu_open &&
            (!ImGui::GetIO().KeyAlt || Mem->mouse.IsDown[1] || Mem->mouse.Released[1]))
    {
        // Right mouse dragging
        {
            if (Mem->mouse.Pressed[1])
            {
                Mem->brush.RtDragStartPos      = Mem->mouse.Pos;
                Mem->brush.RtDragStartDiameter = Mem->brush.Diameter;
                Mem->brush.RtDragStartHardness = Mem->brush.Hardness;
                Mem->brush.RtDragStartOpacity  = Mem->brush.Opacity;
                Mem->brush.RtDragWithShift     = ImGui::GetIO().KeyShift;
                platform::start_mouse_capture();
                platform::set_cursor_visibility(false);
            }
            else if (Mem->mouse.IsDown[1])
            {
                if (Mem->brush.RtDragWithShift)
                {
                    float Opacity = Mem->brush.RtDragStartOpacity + (ImGui::GetMouseDragDelta(1).x * 0.0025f);
                    Mem->brush.Opacity = Math::Clamp(Opacity, 0.0f, 1.0f);
                }
                else
                {
                    float Diameter = Mem->brush.RtDragStartDiameter + (ImGui::GetMouseDragDelta(1).x / Mem->doc.CanvasZoom * 2.0f);
                    Mem->brush.Diameter = Math::Clamp((int32)Diameter, 1, Mem->brush.MaxDiameter);

                    float Hardness = Mem->brush.RtDragStartHardness + (ImGui::GetMouseDragDelta(1).y * 0.0025f);
                    Mem->brush.Hardness = Math::Clamp(Hardness, 0.0f, 1.0f);
                }
            }
            else if (Mem->mouse.Released[1])
            {
                platform::stop_mouse_capture();
                platform::set_mouse_position(Mem->brush.RtDragStartPos.x, Mem->brush.RtDragStartPos.y);
                platform::set_cursor_visibility(true);
            }
        }

        if (Mem->mouse.Pressed[0] && Mem->mouse.InWorkspace)
        {
            Mem->brush.BeingDragged = true;
            Mem->brush.DrawLineSegment = ImGui::GetIO().KeyShift && Mem->brush.LineSegmentStartUV.x >= 0.0f;
            if (Mem->picker.Open) { Mem->picker.CurrentColor = Mem->picker.NewColor; }
            Mem->brush.PaintArea1 = Vec2i(Mem->doc.Width + 1, Mem->doc.Height + 1);
            Mem->brush.PaintArea2 = Vec2i(0,0);
        }
        else if (Mem->mouse.Released[0] && Mem->brush.BeingDragged)
        {
            Mem->misc.draw_overlay         = false;
            Mem->brush.BeingDragged       = false;
            Mem->brush.IsStraightDrag     = false;
            Mem->brush.WasStraightDrag    = false;
            Mem->brush.LineSegmentStartUV = Mem->mouse.UV;

            // TODO: Make a vararg-based RTT function
            // Additive render-to-texture
            {
                GLCHK( glDisable(GL_SCISSOR_TEST) );
                GLCHK( glDisable(GL_DEPTH_TEST) );
                GLCHK( glBindFramebuffer     (GL_FRAMEBUFFER, Mem->misc.fbo) );
                GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->misc.fbo_render_tex, 0) );
                GLCHK( glViewport(0, 0, Mem->doc.Width, Mem->doc.Height) );

                Vec2i Pos  = Mem->brush.PaintArea1;
                Vec2i Size = Mem->brush.PaintArea2 - Mem->brush.PaintArea1;
                int8* PreBrushImage = 0;

                // TODO: OPTIMIZE: The following block seems optimizable
                // Render base image for pre-brush undo
                {
                    GLCHK( glDisable(GL_BLEND) );

                    GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mem->meshes[PapayaMesh_RTTAdd].VboHandle) );
                    GLCHK( glUseProgram(Mem->shaders[PapayaShader_ImGui].Handle) );
                    GLCHK( glUniformMatrix4fv(Mem->shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &Mem->doc.ProjMtx[0][0]) );
                    GL::SetVertexAttribs(Mem->shaders[PapayaShader_ImGui]);

                    GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->doc.TextureID) );
                    GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );

                    PreBrushImage = (int8*)malloc(4 * Size.x * Size.y);
                    GLCHK( glReadPixels(Pos.x, Pos.y, Size.x, Size.y, GL_RGBA, GL_UNSIGNED_BYTE, PreBrushImage) );
                }

                // Render base image with premultiplied alpha
                {
                    GLCHK( glUseProgram(Mem->shaders[PapayaShader_PreMultiplyAlpha].Handle) );
                    GLCHK( glUniformMatrix4fv(Mem->shaders[PapayaShader_PreMultiplyAlpha].Uniforms[0], 1, GL_FALSE, &Mem->doc.ProjMtx[0][0]) );
                    GL::SetVertexAttribs(Mem->shaders[PapayaShader_PreMultiplyAlpha]);

                    GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->doc.TextureID) );
                    GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );
                }

                // Render brush overlay with premultiplied alpha
                {
                    GLCHK( glEnable(GL_BLEND) );
                    GLCHK( glBlendEquation(GL_FUNC_ADD) );
                    GLCHK( glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA) );

                    GLCHK( glUseProgram(Mem->shaders[PapayaShader_PreMultiplyAlpha].Handle) );
                    GLCHK( glUniformMatrix4fv(Mem->shaders[PapayaShader_PreMultiplyAlpha].Uniforms[0], 1, GL_FALSE, &Mem->doc.ProjMtx[0][0]) );
                    GL::SetVertexAttribs(Mem->shaders[PapayaShader_PreMultiplyAlpha]);

                    GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->misc.fbo_sample_tex) );
                    GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );
                }

                // Render blended result with demultiplied alpha
                {
                    GLCHK( glDisable(GL_BLEND) );

                    GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->doc.TextureID, 0) );
                    GLCHK( glUseProgram(Mem->shaders[PapayaShader_DeMultiplyAlpha].Handle) );
                    GLCHK( glUniformMatrix4fv(Mem->shaders[PapayaShader_DeMultiplyAlpha].Uniforms[0], 1, GL_FALSE, &Mem->doc.ProjMtx[0][0]) );
                    GL::SetVertexAttribs(Mem->shaders[PapayaShader_DeMultiplyAlpha]);

                    GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->misc.fbo_render_tex) );
                    GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );
                }

                push_undo(Mem, Pos, Size, PreBrushImage, Mem->brush.LineSegmentStartUV);

                if (PreBrushImage) { free(PreBrushImage); }

                GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
                GLCHK( glViewport(0, 0, (int32)ImGui::GetIO().DisplaySize.x, (int32)ImGui::GetIO().DisplaySize.y) );

                GLCHK( glDisable(GL_BLEND) );
            }
        }

        if (Mem->brush.BeingDragged)
        {
            Mem->misc.draw_overlay = true;

            GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, Mem->misc.fbo) );

            if (Mem->mouse.Pressed[0])
            {
                GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->misc.fbo_sample_tex, 0) );
                GLCHK( glClearColor(0.0f, 0.0f, 0.0f, 0.0f) );
                GLCHK( glClear(GL_COLOR_BUFFER_BIT) );
                GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->misc.fbo_render_tex, 0) );
            }
            GLCHK( glViewport(0, 0, Mem->doc.Width, Mem->doc.Height) );

            GLCHK( glDisable(GL_BLEND) );
            GLCHK( glDisable(GL_SCISSOR_TEST) );

            // Setup orthographic projection matrix
            float width  = (float)Mem->doc.Width;
            float height = (float)Mem->doc.Height;
            GLCHK( glUseProgram(Mem->shaders[PapayaShader_Brush].Handle) );

            Mem->brush.WasStraightDrag = Mem->brush.IsStraightDrag;
            Mem->brush.IsStraightDrag = ImGui::GetIO().KeyShift;

            if (!Mem->brush.WasStraightDrag && Mem->brush.IsStraightDrag)
            {
                Mem->brush.StraightDragStartUV = Mem->mouse.UV;
                Mem->brush.StraightDragSnapX = false;
                Mem->brush.StraightDragSnapY = false;
            }

            if (Mem->brush.IsStraightDrag && !Mem->brush.StraightDragSnapX && !Mem->brush.StraightDragSnapY)
            {
                float dx = Math::Abs(Mem->brush.StraightDragStartUV.x - Mem->mouse.UV.x);
                float dy = Math::Abs(Mem->brush.StraightDragStartUV.y - Mem->mouse.UV.y);
                Mem->brush.StraightDragSnapX = (dx < dy);
                Mem->brush.StraightDragSnapY = (dy < dx);
            }

            if (Mem->brush.IsStraightDrag && Mem->brush.StraightDragSnapX)
            {
                Mem->mouse.UV.x = Mem->brush.StraightDragStartUV.x;
                float pixelPos = Mem->mouse.UV.x * Mem->doc.Width + 0.5f;
                Mem->mouse.Pos.x = Math::RoundToInt(pixelPos * Mem->doc.CanvasZoom + Mem->doc.CanvasPosition.x);
                platform::set_mouse_position(Mem->mouse.Pos.x, Mem->mouse.Pos.y);
            }

            if (Mem->brush.IsStraightDrag && Mem->brush.StraightDragSnapY)
            {
                Mem->mouse.UV.y = Mem->brush.StraightDragStartUV.y;
                float pixelPos = Mem->mouse.UV.y * Mem->doc.Height + 0.5f;
                Mem->mouse.Pos.y = Math::RoundToInt(pixelPos * Mem->doc.CanvasZoom + Mem->doc.CanvasPosition.y);
                platform::set_mouse_position(Mem->mouse.Pos.x, Mem->mouse.Pos.y);
            }

            Vec2 Correction = (Mem->brush.Diameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
            Vec2 CorrectedPos = Mem->mouse.UV + Correction;
            Vec2 CorrectedLastPos = (Mem->brush.DrawLineSegment ? Mem->brush.LineSegmentStartUV : Mem->mouse.LastUV) + Correction;

            Mem->brush.DrawLineSegment = false;

#if 0
            // Brush testing routine
            local_persist int32 i = 0;

            if (i%2)
            {
                local_persist int32 j = 0;
                CorrectedPos		= Vec2( j*0.2f,     j*0.2f) + (Mem->brush.Diameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
                CorrectedLastPos	= Vec2((j+1)*0.2f, (j+1)*0.2f) + (Mem->brush.Diameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
                j++;
            }
            else
            {
                local_persist int32 k = 0;
                CorrectedPos		= Vec2( k*0.2f,     1.0f-k*0.2f) + (Mem->brush.Diameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
                CorrectedLastPos	= Vec2((k+1)*0.2f, 1.0f-(k+1)*0.2f) + (Mem->brush.Diameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
                k++;
            }
            i++;
#endif

            // Paint area calculation
            {
                float UVMinX = Math::Min(CorrectedPos.x, CorrectedLastPos.x);
                float UVMinY = Math::Min(CorrectedPos.y, CorrectedLastPos.y);
                float UVMaxX = Math::Max(CorrectedPos.x, CorrectedLastPos.x);
                float UVMaxY = Math::Max(CorrectedPos.y, CorrectedLastPos.y);

                int32 PixelMinX = Math::RoundToInt(UVMinX * Mem->doc.Width  - 0.5f * Mem->brush.Diameter);
                int32 PixelMinY = Math::RoundToInt(UVMinY * Mem->doc.Height - 0.5f * Mem->brush.Diameter);
                int32 PixelMaxX = Math::RoundToInt(UVMaxX * Mem->doc.Width  + 0.5f * Mem->brush.Diameter);
                int32 PixelMaxY = Math::RoundToInt(UVMaxY * Mem->doc.Height + 0.5f * Mem->brush.Diameter);

                Mem->brush.PaintArea1.x = Math::Min(Mem->brush.PaintArea1.x, PixelMinX);
                Mem->brush.PaintArea1.y = Math::Min(Mem->brush.PaintArea1.y, PixelMinY);
                Mem->brush.PaintArea2.x = Math::Max(Mem->brush.PaintArea2.x, PixelMaxX);
                Mem->brush.PaintArea2.y = Math::Max(Mem->brush.PaintArea2.y, PixelMaxY);

                Mem->brush.PaintArea1.x = Math::Clamp(Mem->brush.PaintArea1.x, 0, Mem->doc.Width);
                Mem->brush.PaintArea1.y = Math::Clamp(Mem->brush.PaintArea1.y, 0, Mem->doc.Height);
                Mem->brush.PaintArea2.x = Math::Clamp(Mem->brush.PaintArea2.x, 0, Mem->doc.Width);
                Mem->brush.PaintArea2.y = Math::Clamp(Mem->brush.PaintArea2.y, 0, Mem->doc.Height);
            }

            GLCHK( glUniformMatrix4fv(Mem->shaders[PapayaShader_Brush].Uniforms[0], 1, GL_FALSE, &Mem->doc.ProjMtx[0][0]) );
            GLCHK( glUniform2f(Mem->shaders[PapayaShader_Brush].Uniforms[2], CorrectedPos.x, CorrectedPos.y * Mem->doc.InverseAspect) ); // Pos uniform
            GLCHK( glUniform2f(Mem->shaders[PapayaShader_Brush].Uniforms[3], CorrectedLastPos.x, CorrectedLastPos.y * Mem->doc.InverseAspect) ); // Lastpos uniform
            GLCHK( glUniform1f(Mem->shaders[PapayaShader_Brush].Uniforms[4], (float)Mem->brush.Diameter / ((float)Mem->doc.Width * 2.0f)) );
            float Opacity = Mem->brush.Opacity;
            //if (Mem->tablet.Pressure > 0.0f) { Opacity *= Mem->tablet.Pressure; }
            GLCHK( glUniform4f(Mem->shaders[PapayaShader_Brush].Uniforms[5], Mem->picker.CurrentColor.r,
                        Mem->picker.CurrentColor.g,
                        Mem->picker.CurrentColor.b,
                        Opacity) );
            // Brush hardness
            {
                float Hardness;
                if (Mem->brush.AntiAlias && Mem->brush.Diameter > 2)
                {
                    float AAWidth = 1.0f; // The width of pixels over which the antialiased falloff occurs
                    float Radius  = Mem->brush.Diameter / 2.0f;
                    Hardness      = Math::Min(Mem->brush.Hardness, 1.0f - (AAWidth / Radius));
                }
                else
                {
                    Hardness      = Mem->brush.Hardness;
                }

                GLCHK( glUniform1f(Mem->shaders[PapayaShader_Brush].Uniforms[6], Hardness) );
            }

            GLCHK( glUniform1f(Mem->shaders[PapayaShader_Brush].Uniforms[7], Mem->doc.InverseAspect) ); // Inverse Aspect uniform

            GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mem->meshes[PapayaMesh_RTTBrush].VboHandle) );
            GL::SetVertexAttribs(Mem->shaders[PapayaShader_Brush]);

            GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->misc.fbo_sample_tex) );
            GLCHK( glDrawArrays(GL_TRIANGLES, 0, 6) );

            uint32 Temp = Mem->misc.fbo_render_tex;
            Mem->misc.fbo_render_tex = Mem->misc.fbo_sample_tex;
            Mem->misc.fbo_sample_tex = Temp;
            GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->misc.fbo_render_tex, 0) );

            GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
            GLCHK( glViewport(0, 0, (int32)ImGui::GetIO().DisplaySize.x, (int32)ImGui::GetIO().DisplaySize.y) );
        }


#if 0
        // =========================================================================================
        // Visualization: Brush falloff

        const int32 ArraySize = 256;
        local_persist float Opacities[ArraySize] = { 0 };

        float MaxScale = 90.0f;
        float Scale    = 1.0f / (1.0f - Mem->brush.Hardness);
        float Phase    = (1.0f - Scale) * (float)Math::Pi;
        float Period   = (float)Math::Pi * Scale / (float)ArraySize;

        for (int32 i = 0; i < ArraySize; i++)
        {
            Opacities[i] = (cosf(((float)i * Period) + Phase) + 1.0f) * 0.5f;
            if ((float)i < (float)ArraySize - ((float)ArraySize / Scale)) { Opacities[i] = 1.0f; }
        }

        ImGui::Begin("Brush falloff");
        ImGui::PlotLines("", Opacities, ArraySize, 0, 0, FLT_MIN, FLT_MAX, Vec2(256,256));
        ImGui::End();
        // =========================================================================================
#endif
    }

    // Undo/Redo
    {
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) // Pop undo op
        {
            // TODO: Clean up this workflow
            bool Refresh = false;

            if (ImGui::GetIO().KeyShift &&
                Mem->doc.Undo.CurrentIndex < Mem->doc.Undo.Count - 1 &&
                Mem->doc.Undo.Current->Next != 0) // Redo
            {
                Mem->doc.Undo.Current = Mem->doc.Undo.Current->Next;
                Mem->doc.Undo.CurrentIndex++;
                Mem->brush.LineSegmentStartUV = Mem->doc.Undo.Current->LineSegmentStartUV;
                Refresh = true;
            }
            else if (!ImGui::GetIO().KeyShift &&
                Mem->doc.Undo.CurrentIndex > 0 &&
                Mem->doc.Undo.Current->Prev != 0) // Undo
            {
                if (Mem->doc.Undo.Current->IsSubRect)
                {
                    load_from_undo_buffer(Mem, true);
                }
                else
                {
                    Refresh = true;
                }

                Mem->doc.Undo.Current = Mem->doc.Undo.Current->Prev;
                Mem->doc.Undo.CurrentIndex--;
                Mem->brush.LineSegmentStartUV = Mem->doc.Undo.Current->LineSegmentStartUV;
            }

            if (Refresh)
            {
                load_from_undo_buffer(Mem, false);
            }
        }

        // Visualization: Undo buffer
        if (Mem->misc.show_undo_buffer)
        {
            ImGui::Begin("Undo buffer");

            ImDrawList* DrawList = ImGui::GetWindowDrawList();

            // Buffer line
            float Width = ImGui::GetWindowSize().x;
            Vec2 Pos    = ImGui::GetWindowPos();
            Vec2 P1     = Pos + Vec2(10, 40);
            Vec2 P2     = Pos + Vec2(Width - 10, 40);
            DrawList->AddLine(P1, P2, 0xFFFFFFFF);

            // Base mark
            uint64 BaseOffset = (int8*)Mem->doc.Undo.Base - (int8*)Mem->doc.Undo.Start;
            float BaseX       = P1.x + (float)BaseOffset / (float)Mem->doc.Undo.Size * (P2.x - P1.x);
            DrawList->AddLine(Vec2(BaseX, Pos.y + 26), Vec2(BaseX,Pos.y + 54), 0xFFFFFF00);

            // Current mark
            uint64 CurrOffset = (int8*)Mem->doc.Undo.Current - (int8*)Mem->doc.Undo.Start;
            float CurrX       = P1.x + (float)CurrOffset / (float)Mem->doc.Undo.Size * (P2.x - P1.x);
            DrawList->AddLine(Vec2(CurrX, Pos.y + 29), Vec2(CurrX, Pos.y + 51), 0xFFFF00FF);

            // Last mark
            uint64 LastOffset = (int8*)Mem->doc.Undo.Last - (int8*)Mem->doc.Undo.Start;
            float LastX       = P1.x + (float)LastOffset / (float)Mem->doc.Undo.Size * (P2.x - P1.x);
            //DrawList->AddLine(Vec2(LastX, Pos.y + 32), Vec2(LastX, Pos.y + 48), 0xFF0000FF);

            // Top mark
            uint64 TopOffset = (int8*)Mem->doc.Undo.Top - (int8*)Mem->doc.Undo.Start;
            float TopX       = P1.x + (float)TopOffset / (float)Mem->doc.Undo.Size * (P2.x - P1.x);
            DrawList->AddLine(Vec2(TopX, Pos.y + 35), Vec2(TopX, Pos.y + 45), 0xFF00FFFF);

            ImGui::Text(" "); ImGui::Text(" "); // Vertical spacers
            ImGui::TextColored  (Color(0.0f,1.0f,1.0f,1.0f), "Base    %llu", BaseOffset);
            ImGui::TextColored  (Color(1.0f,0.0f,1.0f,1.0f), "Current %llu", CurrOffset);
            //ImGui::TextColored(Color(1.0f,0.0f,0.0f,1.0f), "Last    %llu", LastOffset);
            ImGui::TextColored  (Color(1.0f,1.0f,0.0f,1.0f), "Top     %llu", TopOffset);
            ImGui::Text         ("Count   %lu", Mem->doc.Undo.Count);
            ImGui::Text         ("Index   %lu", Mem->doc.Undo.CurrentIndex);

            ImGui::End();
        }
    }

    // Canvas zooming and panning
    {
        // Panning
        Mem->doc.CanvasPosition += Math::RoundToVec2i(ImGui::GetMouseDragDelta(2));
        ImGui::ResetMouseDragDelta(2);

        // Zooming
        if (!ImGui::IsMouseDown(2) && ImGui::GetIO().MouseWheel)
        {
            float MinZoom      = 0.01f, MaxZoom = 32.0f;
            float ZoomSpeed    = 0.2f * Mem->doc.CanvasZoom;
            float ScaleDelta   = Math::Min(MaxZoom - Mem->doc.CanvasZoom, ImGui::GetIO().MouseWheel * ZoomSpeed);
            Vec2 OldCanvasZoom = Vec2((float)Mem->doc.Width, (float)Mem->doc.Height) * Mem->doc.CanvasZoom;

            Mem->doc.CanvasZoom += ScaleDelta;
            if (Mem->doc.CanvasZoom < MinZoom) { Mem->doc.CanvasZoom = MinZoom; } // TODO: Dynamically clamp min such that fully zoomed out image is 2x2 pixels?
            Vec2i NewCanvasSize = Math::RoundToVec2i(Vec2((float)Mem->doc.Width, (float)Mem->doc.Height) * Mem->doc.CanvasZoom);

            if ((NewCanvasSize.x > Mem->window.Width || NewCanvasSize.y > Mem->window.Height))
            {
                Vec2 PreScaleMousePos = Vec2(Mem->mouse.Pos - Mem->doc.CanvasPosition) / OldCanvasZoom;
                Vec2 NewPos = Vec2(Mem->doc.CanvasPosition) -
                    Vec2(PreScaleMousePos.x * ScaleDelta * (float)Mem->doc.Width,
                        PreScaleMousePos.y * ScaleDelta * (float)Mem->doc.Height);
                Mem->doc.CanvasPosition = Math::RoundToVec2i(NewPos);
            }
            else // Center canvas
            {
                // TODO: Maybe disable centering on zoom out. Needs more usability testing.
                int32 TopMargin = 53; // TODO: Put common layout constants in struct
                int32 PosX = Math::RoundToInt((Mem->window.Width - (float)Mem->doc.Width * Mem->doc.CanvasZoom) / 2.0f);
                int32 PosY = TopMargin + Math::RoundToInt((Mem->window.Height - TopMargin - (float)Mem->doc.Height * Mem->doc.CanvasZoom) / 2.0f);
                Mem->doc.CanvasPosition = Vec2i(PosX, PosY);
            }
        }
    }

    // Draw alpha grid
    {
        // TODO: Conflate PapayaMesh_AlphaGrid and PapayaMesh_Canvas?
        GL::TransformQuad(Mem->meshes[PapayaMesh_AlphaGrid],
            Mem->doc.CanvasPosition,
            Vec2(Mem->doc.Width * Mem->doc.CanvasZoom, Mem->doc.Height * Mem->doc.CanvasZoom));

        mat4x4 M;
        mat4x4_ortho(M, 0.f, (float)Mem->window.Width, (float)Mem->window.Height, 0.f, -1.f, 1.f);

        if (Mem->current_tool == PapayaTool_CropRotate) // Rotate around center
        {
            mat4x4 R;
            Vec2 Offset = Vec2(Mem->doc.CanvasPosition.x + Mem->doc.Width *
                               Mem->doc.CanvasZoom * 0.5f,
                               Mem->doc.CanvasPosition.y + Mem->doc.Height *
                               Mem->doc.CanvasZoom * 0.5f);

            mat4x4_translate_in_place(M, Offset.x, Offset.y, 0.f);
            mat4x4_rotate_Z(R, M, Math::ToRadians(90.0f * Mem->crop_rotate.BaseRotation));
            mat4x4_translate_in_place(R, -Offset.x, -Offset.y, 0.f);
            mat4x4_dup(M, R);
        }

        GL::DrawMesh(Mem->meshes[PapayaMesh_AlphaGrid], Mem->shaders[PapayaShader_AlphaGrid], true,
            6,
            UniformType_Matrix4, M,
            UniformType_Color, Mem->colors[PapayaCol_AlphaGrid1],
            UniformType_Color, Mem->colors[PapayaCol_AlphaGrid2],
            UniformType_Float, Mem->doc.CanvasZoom,
            UniformType_Float, Mem->doc.InverseAspect,
            UniformType_Float, Math::Max((float)Mem->doc.Width, (float)Mem->doc.Height));
    }

    // Draw canvas
    {
        GL::TransformQuad(Mem->meshes[PapayaMesh_Canvas],
            Mem->doc.CanvasPosition,
            Vec2(Mem->doc.Width * Mem->doc.CanvasZoom, Mem->doc.Height * Mem->doc.CanvasZoom));

        mat4x4 M;
        mat4x4_ortho(M, 0.f, (float)Mem->window.Width, (float)Mem->window.Height, 0.f, -1.f, 1.f);

        if (Mem->current_tool == PapayaTool_CropRotate) // Rotate around center
        {
            mat4x4 R;
            Vec2 Offset = Vec2(Mem->doc.CanvasPosition.x + Mem->doc.Width *
                    Mem->doc.CanvasZoom * 0.5f,
                    Mem->doc.CanvasPosition.y + Mem->doc.Height *
                    Mem->doc.CanvasZoom * 0.5f);

            mat4x4_translate_in_place(M, Offset.x, Offset.y, 0.f);
            mat4x4_rotate_Z(R, M, Mem->crop_rotate.SliderAngle + 
                    Math::ToRadians(90.0f * Mem->crop_rotate.BaseRotation));
            mat4x4_translate_in_place(R, -Offset.x, -Offset.y, 0.f);
            mat4x4_dup(M, R);
        }

        GLCHK( glBindTexture(GL_TEXTURE_2D, Mem->doc.TextureID) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST) );
        GL::DrawMesh(Mem->meshes[PapayaMesh_Canvas], Mem->shaders[PapayaShader_ImGui],
            true, 1,
            UniformType_Matrix4, M);

        if (Mem->misc.draw_overlay)
        {
            GLCHK( glBindTexture  (GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->misc.fbo_sample_tex) );
            GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
            GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST) );
            GL::DrawMesh(Mem->meshes[PapayaMesh_Canvas], Mem->shaders[PapayaShader_ImGui], 1, true,
                UniformType_Matrix4, &Mem->window.ProjMtx[0][0]);
        }
    }

    // Update and draw crop outline
    if (Mem->current_tool == PapayaTool_CropRotate)
    {
        CropRotate::CropOutline(Mem);
    }

    // Draw brush cursor
    {
        if (Mem->current_tool == PapayaTool_Brush &&
            (!ImGui::GetIO().KeyAlt || Mem->mouse.IsDown[1]))
        {
            float ScaledDiameter = Mem->brush.Diameter * Mem->doc.CanvasZoom;

            GL::TransformQuad(Mem->meshes[PapayaMesh_BrushCursor],
                (Mem->mouse.IsDown[1] || Mem->mouse.WasDown[1] ? Mem->brush.RtDragStartPos : Mem->mouse.Pos) - (Vec2(ScaledDiameter,ScaledDiameter) * 0.5f),
                Vec2(ScaledDiameter,ScaledDiameter));

            GL::DrawMesh(Mem->meshes[PapayaMesh_BrushCursor], Mem->shaders[PapayaShader_BrushCursor], true,
                4,
                UniformType_Matrix4, &Mem->window.ProjMtx[0][0],
                UniformType_Color, Color(1.0f, 0.0f, 0.0f, Mem->mouse.IsDown[1] ? Mem->brush.Opacity : 0.0f),
                UniformType_Float, Mem->brush.Hardness,
                UniformType_Float, ScaledDiameter);
        }
    }

    // Eye dropper
    {
        if ((Mem->current_tool == PapayaTool_EyeDropper || (Mem->current_tool == PapayaTool_Brush && ImGui::GetIO().KeyAlt))
            && Mem->mouse.InWorkspace)
        {
            if (Mem->mouse.IsDown[0])
            {
                // Get pixel color
                {
                    float Pixel[3] = { 0 };
                    GLCHK( glReadPixels((int32)Mem->mouse.Pos.x, Mem->window.Height - (int32)Mem->mouse.Pos.y, 1, 1, GL_RGB, GL_FLOAT, Pixel) );
                    Mem->eye_dropper.CurrentColor = Color(Pixel[0], Pixel[1], Pixel[2]);
                }

                Vec2 Size = Vec2(230,230);
                GL::TransformQuad(Mem->meshes[PapayaMesh_EyeDropperCursor],
                    Mem->mouse.Pos - (Size * 0.5f),
                    Size);

                GL::DrawMesh(Mem->meshes[PapayaMesh_EyeDropperCursor], Mem->shaders[PapayaShader_EyeDropperCursor], true,
                    3,
                    UniformType_Matrix4, &Mem->window.ProjMtx[0][0],
                    UniformType_Color, Mem->eye_dropper.CurrentColor,
                    UniformType_Color, Mem->picker.NewColor);
            }
            else if (Mem->mouse.Released[0])
            {
                Picker::SetColor(Mem->eye_dropper.CurrentColor, &Mem->picker, Mem->picker.Open);
            }
        }
    }

EndOfDoc:

    // Metrics window
    {
        if (Mem->misc.show_metrics)
        {
            ImGui::Begin("Metrics");
            if (ImGui::CollapsingHeader("Profiler", 0, true, true))
            {
                ImGui::Columns(3, "profilercolumns");
                ImGui::Separator();
                ImGui::Text("Name");                ImGui::NextColumn();
                ImGui::Text("Cycles");              ImGui::NextColumn();
                ImGui::Text("Millisecs");           ImGui::NextColumn();
                ImGui::Separator();

                for (int32 i = 0; i < Timer_COUNT; i++)
                {
                    ImGui::Text(TimerNames[i]);                                 ImGui::NextColumn();
                    ImGui::Text("%llu", Mem->debug.Timers[i].ElapsedCycles);    ImGui::NextColumn();
                    ImGui::Text("%f" , Mem->debug.Timers[i].ElapsedMs);         ImGui::NextColumn();
                }

                ImGui::Columns(1);
                ImGui::Separator();
            }
            if (ImGui::CollapsingHeader("Input"))
            {
                ImGui::Separator();
                ImGui::Text("Tablet");
                ImGui::Columns(2, "inputcolumns");
                ImGui::Separator();
                ImGui::Text("PosX");                        ImGui::NextColumn();
                ImGui::Text("%d", Mem->tablet.PosX);        ImGui::NextColumn();
                ImGui::Text("PosY");                        ImGui::NextColumn();
                ImGui::Text("%d", Mem->tablet.PosY);        ImGui::NextColumn();
                ImGui::Text("Pressure");                    ImGui::NextColumn();
                ImGui::Text("%f", Mem->tablet.Pressure);    ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::Text("Mouse");
                ImGui::Columns(2, "inputcolumns");
                ImGui::Separator();
                ImGui::Text("PosX");                        ImGui::NextColumn();
                ImGui::Text("%d", Mem->mouse.Pos.x);        ImGui::NextColumn();
                ImGui::Text("PosY");                        ImGui::NextColumn();
                ImGui::Text("%d", Mem->mouse.Pos.y);        ImGui::NextColumn();
                ImGui::Text("Buttons");                     ImGui::NextColumn();
                ImGui::Text("%d %d %d",
                    Mem->mouse.IsDown[0],
                    Mem->mouse.IsDown[1],
                    Mem->mouse.IsDown[2]);                  ImGui::NextColumn();
                ImGui::Columns(1);
                ImGui::Separator();
            }
            ImGui::End();
        }
    }

    ImGui::Render(Mem);

    // Color Picker Panel
    if (Mem->picker.Open)
    {
        // TODO: Investigate how to move this custom shaded quad drawing into
        //       ImGui to get correct draw order.

        // Draw hue picker
        GL::DrawMesh(Mem->meshes[PapayaMesh_PickerHStrip], Mem->shaders[PapayaShader_PickerHStrip], false,
                2,
                UniformType_Matrix4, &Mem->window.ProjMtx[0][0],
                UniformType_Float, Mem->picker.CursorH);

        // Draw saturation-value picker
        GL::DrawMesh(Mem->meshes[PapayaMesh_PickerSVBox], Mem->shaders[PapayaShader_PickerSVBox], false,
                3,
                UniformType_Matrix4, &Mem->window.ProjMtx[0][0],
                UniformType_Float, Mem->picker.CursorH,
                UniformType_Vec2, Mem->picker.CursorSV);
    }

    // Last mouse info
    {
        Mem->mouse.LastPos    = Mem->mouse.Pos;
        Mem->mouse.LastUV     = Mem->mouse.UV;
        Mem->mouse.WasDown[0] = ImGui::IsMouseDown(0);
        Mem->mouse.WasDown[1] = ImGui::IsMouseDown(1);
        Mem->mouse.WasDown[2] = ImGui::IsMouseDown(2);
    }
}

void core::render_imgui(ImDrawData* DrawData, void* MemPtr)
{
    PapayaMemory* Mem = (PapayaMemory*)MemPtr;

    // Backup GL state
    GLint last_program, last_texture, last_array_buffer, last_element_array_buffer;
    GLCHK( glGetIntegerv(GL_CURRENT_PROGRAM, &last_program) );
    GLCHK( glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture) );
    GLCHK( glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer) );
    GLCHK( glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer) );

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    GLCHK( glEnable       (GL_BLEND) );
    GLCHK( glBlendEquation(GL_FUNC_ADD) );
    GLCHK( glBlendFunc    (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) );
    GLCHK( glDisable      (GL_CULL_FACE) );
    GLCHK( glDisable      (GL_DEPTH_TEST) );
    GLCHK( glEnable       (GL_SCISSOR_TEST) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );

    // Handle cases of screen coordinates != from framebuffer coordinates (e.g. retina displays)
    ImGuiIO& io     = ImGui::GetIO();
    float fb_height = io.DisplaySize.y * io.DisplayFramebufferScale.y;
    DrawData->ScaleClipRects(io.DisplayFramebufferScale);

    GLCHK( glUseProgram      (Mem->shaders[PapayaShader_ImGui].Handle) );
    GLCHK( glUniform1i       (Mem->shaders[PapayaShader_ImGui].Uniforms[1], 0) );
    GLCHK( glUniformMatrix4fv(Mem->shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &Mem->window.ProjMtx[0][0]) );

    for (int32 n = 0; n < DrawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = DrawData->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mem->meshes[PapayaMesh_ImGui].VboHandle) );
        GLCHK( glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW) );

        GLCHK( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mem->meshes[PapayaMesh_ImGui].ElementsHandle) );
        GLCHK( glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW) );

        GL::SetVertexAttribs(Mem->shaders[PapayaShader_ImGui]);

        for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
        {
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId) );
                GLCHK( glScissor((int32)pcmd->ClipRect.x, (int32)(fb_height - pcmd->ClipRect.w), (int32)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int32)(pcmd->ClipRect.w - pcmd->ClipRect.y)) );
                GLCHK( glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, idx_buffer_offset) );
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    // Restore modified GL state
    GLCHK( glUseProgram     (last_program) );
    GLCHK( glBindTexture    (GL_TEXTURE_2D, last_texture) );
    GLCHK( glBindBuffer     (GL_ARRAY_BUFFER, last_array_buffer) );
    GLCHK( glBindBuffer     (GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer) );
    GLCHK( glDisable        (GL_SCISSOR_TEST) );
}
