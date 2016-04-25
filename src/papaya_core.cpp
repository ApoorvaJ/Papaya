
#include "papaya_core.h"

#include "libs/glew/glew.h"
#include "libs/stb_image.h"
#include "libs/stb_image_write.h"
#include "libs/imgui/imgui.h"
#include "libs/mathlib.h"
#include "libs/linmath.h"

internal uint32 AllocateEmptyTexture(int32 Width, int32 Height)
{
    uint32 Tex;
    GLCHK( glGenTextures  (1, &Tex) );
    GLCHK( glBindTexture  (GL_TEXTURE_2D, Tex) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
    GLCHK( glTexImage2D   (GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, 0) );
    return Tex;
}

internal uint32 LoadAndBindImage(char* Path)
{
    uint8* Image;
    int32 ImageWidth, ImageHeight, ComponentsPerPixel;
    Image = stbi_load(Path, &ImageWidth, &ImageHeight, &ComponentsPerPixel, 0);

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
    return (uint32)Id_GLuint;
}

// This function reads from the frame buffer and hence needs the appropriate frame buffer to be
// bound before it is called.
internal void PushUndo(PapayaMemory* Mem, Vec2i Pos, Vec2i Size, int8* PreBrushImage)
{
    if (Mem->Doc.Undo.Top == 0) // Buffer is empty
    {
        Mem->Doc.Undo.Base = (UndoData*)Mem->Doc.Undo.Start;
        Mem->Doc.Undo.Top  = Mem->Doc.Undo.Start;
    }
    else if (Mem->Doc.Undo.Current->Next != 0) // Not empty and not at end. Reposition for overwrite.
    {
        uint64 BytesToRight = (int8*)Mem->Doc.Undo.Start + Mem->Doc.Undo.Size - (int8*)Mem->Doc.Undo.Current;
        uint64 ImageSize = (Mem->Doc.Undo.Current->IsSubRect ? 8 : 4) * Mem->Doc.Undo.Current->Size.x * Mem->Doc.Undo.Current->Size.y;
        uint64 BlockSize = sizeof(UndoData) + ImageSize;
        if (BytesToRight >= BlockSize)
        {
            Mem->Doc.Undo.Top = (int8*)Mem->Doc.Undo.Current + BlockSize;
        }
        else
        {
            Mem->Doc.Undo.Top = (int8*)Mem->Doc.Undo.Start + BlockSize - BytesToRight;
        }
        Mem->Doc.Undo.Last = Mem->Doc.Undo.Current;
        Mem->Doc.Undo.Count = Mem->Doc.Undo.CurrentIndex + 1;
    }

    UndoData Data  = {};
    Data.OpCode    = PapayaUndoOp_Brush;
    Data.Prev      = Mem->Doc.Undo.Last;
    Data.Pos       = Pos;
    Data.Size      = Size;
    Data.IsSubRect = (PreBrushImage != 0);
    uint64 BufSize = sizeof(UndoData) + Size.x * Size.y * (Data.IsSubRect ? 8 : 4);
    void* Buf      = malloc((size_t)BufSize);

    Timer::StartTime(&Mem->Debug.Timers[Timer_GetUndoImage]);
    memcpy(Buf, &Data, sizeof(UndoData));
    GLCHK( glReadPixels(Pos.x, Pos.y, Size.x, Size.y, GL_RGBA, GL_UNSIGNED_BYTE, (int8*)Buf + sizeof(UndoData)) );
    Timer::StopTime(&Mem->Debug.Timers[Timer_GetUndoImage]);

    if (Data.IsSubRect)
    {
        memcpy((int8*)Buf + sizeof(UndoData) + 4 * Size.x * Size.y, PreBrushImage, 4 * Size.x * Size.y);
    }

    uint64 BytesToRight = (int8*)Mem->Doc.Undo.Start + Mem->Doc.Undo.Size - (int8*)Mem->Doc.Undo.Top;
    if (BytesToRight < sizeof(UndoData)) // Not enough space for UndoData. Go to start.
    {
        // Reposition the base pointer
        while (((int8*)Mem->Doc.Undo.Base >= (int8*)Mem->Doc.Undo.Top ||
            (int8*)Mem->Doc.Undo.Base < (int8*)Mem->Doc.Undo.Start + BufSize) &&
            Mem->Doc.Undo.Count > 0)
        {
            Mem->Doc.Undo.Base = Mem->Doc.Undo.Base->Next;
            Mem->Doc.Undo.Base->Prev = 0;
            Mem->Doc.Undo.Count--;
            Mem->Doc.Undo.CurrentIndex--;
        }

        Mem->Doc.Undo.Top = Mem->Doc.Undo.Start;
        memcpy(Mem->Doc.Undo.Top, Buf, (size_t)BufSize);

        if (Mem->Doc.Undo.Last) { Mem->Doc.Undo.Last->Next = (UndoData*)Mem->Doc.Undo.Top; }
        Mem->Doc.Undo.Last = (UndoData*)Mem->Doc.Undo.Top;
        Mem->Doc.Undo.Top = (int8*)Mem->Doc.Undo.Top + BufSize;
    }
    else if (BytesToRight < BufSize) // Enough space for UndoData, but not for image. Split image data.
    {
        // Reposition the base pointer
        while (((int8*)Mem->Doc.Undo.Base >= (int8*)Mem->Doc.Undo.Top ||
            (int8*)Mem->Doc.Undo.Base  <  (int8*)Mem->Doc.Undo.Start + BufSize - BytesToRight) &&
            Mem->Doc.Undo.Count > 0)
        {
            Mem->Doc.Undo.Base = Mem->Doc.Undo.Base->Next;
            Mem->Doc.Undo.Base->Prev = 0;
            Mem->Doc.Undo.Count--;
            Mem->Doc.Undo.CurrentIndex--;
        }

        memcpy(Mem->Doc.Undo.Top, Buf, (size_t)BytesToRight);
        memcpy(Mem->Doc.Undo.Start, (int8*)Buf + BytesToRight, (size_t)(BufSize - BytesToRight));

        if (BytesToRight == 0) { Mem->Doc.Undo.Top = Mem->Doc.Undo.Start; }

        if (Mem->Doc.Undo.Last) { Mem->Doc.Undo.Last->Next = (UndoData*)Mem->Doc.Undo.Top; }
        Mem->Doc.Undo.Last = (UndoData*)Mem->Doc.Undo.Top;
        Mem->Doc.Undo.Top = (int8*)Mem->Doc.Undo.Start + BufSize - BytesToRight;
    }
    else // Enough space for everything. Simply append.
    {
        // Reposition the base pointer
        while ((int8*)Mem->Doc.Undo.Base >= (int8*)Mem->Doc.Undo.Top &&
            (int8*)Mem->Doc.Undo.Base < (int8*)Mem->Doc.Undo.Top + BufSize &&
            Mem->Doc.Undo.Count > 0)
        {
            Mem->Doc.Undo.Base = Mem->Doc.Undo.Base->Next;
            Mem->Doc.Undo.Base->Prev = 0;
            Mem->Doc.Undo.Count--;
            Mem->Doc.Undo.CurrentIndex--;
        }

        memcpy(Mem->Doc.Undo.Top, Buf, (size_t)BufSize);

        if (Mem->Doc.Undo.Last) { Mem->Doc.Undo.Last->Next = (UndoData*)Mem->Doc.Undo.Top; }
        Mem->Doc.Undo.Last = (UndoData*)Mem->Doc.Undo.Top;
        Mem->Doc.Undo.Top = (int8*)Mem->Doc.Undo.Top + BufSize;
    }

    free(Buf);

    Mem->Doc.Undo.Current = Mem->Doc.Undo.Last;
    Mem->Doc.Undo.Count++;
    Mem->Doc.Undo.CurrentIndex++;
}

internal void LoadFromUndoBuffer(PapayaMemory* Mem, bool LoadPreBrushImage)
{
    UndoData Data  = {};
    int8* Image    = 0;
    bool AllocUsed = false;

    memcpy(&Data, Mem->Doc.Undo.Current, sizeof(UndoData));

    size_t BytesToRight = (int8*)Mem->Doc.Undo.Start + Mem->Doc.Undo.Size - (int8*)Mem->Doc.Undo.Current;
    size_t ImageSize = (Mem->Doc.Undo.Current->IsSubRect ? 8 : 4) * Data.Size.x * Data.Size.y;
    if (BytesToRight - sizeof(UndoData) >= ImageSize) // Image is contiguously stored
    {
        Image = (int8*)Mem->Doc.Undo.Current + sizeof(UndoData);
    }
    else // Image is split
    {
        AllocUsed = true;
        Image = (int8*)malloc(ImageSize);
        memcpy(Image, (int8*)Mem->Doc.Undo.Current + sizeof(UndoData), (size_t)BytesToRight - sizeof(UndoData));
        memcpy((int8*)Image + BytesToRight - sizeof(UndoData), Mem->Doc.Undo.Start, (size_t)(ImageSize - (BytesToRight - sizeof(UndoData))));
    }

    GLCHK( glBindTexture(GL_TEXTURE_2D, Mem->Doc.TextureID) );

    GLCHK( glTexSubImage2D(GL_TEXTURE_2D, 0, Data.Pos.x, Data.Pos.y, Data.Size.x, Data.Size.y, GL_RGBA, GL_UNSIGNED_BYTE, Image + (LoadPreBrushImage ? 4 * Data.Size.x * Data.Size.y : 0)) );

    if (AllocUsed) { free(Image); }
}

bool Core::OpenDocument(char* Path, PapayaMemory* Mem)
{
    Timer::StartTime(&Mem->Debug.Timers[Timer_ImageOpen]);

    // Load/create image
    {
        uint8* Texture;
        if (Path)
        {
            Texture = stbi_load(Path, &Mem->Doc.Width, &Mem->Doc.Height,
                    &Mem->Doc.ComponentsPerPixel, 4);
        }
        else
        {
            Texture = (uint8*)calloc(1, Mem->Doc.Width * Mem->Doc.Height * 4);
        }

        if (!Texture) { return false; }

        // Create texture
        GLCHK( glGenTextures  (1, &Mem->Doc.TextureID) );
        GLCHK( glBindTexture  (GL_TEXTURE_2D, Mem->Doc.TextureID) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
        GLCHK( glTexImage2D   (GL_TEXTURE_2D, 0, GL_RGBA8, Mem->Doc.Width, Mem->Doc.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Texture) );

        Mem->Doc.InverseAspect = (float)Mem->Doc.Height / (float)Mem->Doc.Width;
        Mem->Doc.CanvasZoom = 0.8f * Math::Min((float)Mem->Window.Width/(float)Mem->Doc.Width, (float)Mem->Window.Height/(float)Mem->Doc.Height);
        if (Mem->Doc.CanvasZoom > 1.0f) { Mem->Doc.CanvasZoom = 1.0f; }
        // Center canvas
        {
            int32 TopMargin = 53; // TODO: Put common layout constants in struct
            int32 PosX = Math::RoundToInt((Mem->Window.Width - (float)Mem->Doc.Width * Mem->Doc.CanvasZoom) / 2.0f);
            int32 PosY = TopMargin + Math::RoundToInt((Mem->Window.Height - TopMargin - (float)Mem->Doc.Height * Mem->Doc.CanvasZoom) / 2.0f);
            Mem->Doc.CanvasPosition = Vec2i(PosX, PosY);
        }
        free(Texture);
    }

    // Set up the frame buffer
    {
        // Create a framebuffer object and bind it
        GLCHK( glDisable(GL_BLEND) );
        GLCHK( glGenFramebuffers(1, &Mem->Misc.FrameBufferObject) );
        GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, Mem->Misc.FrameBufferObject) );

        Mem->Misc.FboRenderTexture = AllocateEmptyTexture(Mem->Doc.Width, Mem->Doc.Height);
        Mem->Misc.FboSampleTexture = AllocateEmptyTexture(Mem->Doc.Width, Mem->Doc.Height);

        // Attach the color texture to the FBO
        GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->Misc.FboRenderTexture, 0) );

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

    // Set up meshes for rendering to texture
    {
        Vec2 Size = Vec2((float)Mem->Doc.Width, (float)Mem->Doc.Height);

        GL::InitQuad(Mem->Meshes[PapayaMesh_RTTBrush],
            Vec2(0,0), Size, GL_STATIC_DRAW);

        GL::InitQuad(Mem->Meshes[PapayaMesh_RTTAdd],
            Vec2(0, 0), Size, GL_STATIC_DRAW);
    }

    // Projection matrix
    {
        float w = (float)Mem->Doc.Width;
        float h = (float)Mem->Doc.Height;
        float OrthoMtx[4][4] =
        {
            { 2.0f/w,  0.0f,    0.0f,   0.0f },
            { 0.0f,   -2.0f/h,  0.0f,   0.0f },
            { 0.0f,    0.0f,   -1.0f,   0.0f },
            {-1.0f,    1.0f,    0.0f,   1.0f },
        };
        memcpy(Mem->Doc.ProjMtx, OrthoMtx, sizeof(OrthoMtx));
    }

    // Init undo buffer
    {
        size_t MaxSize        = 512 * 1024 * 1024; // Maximum number of bytes to allocate as an undo buffer
        size_t UndoRecordSize = sizeof(UndoData) + 4 * Mem->Doc.Width * Mem->Doc.Height;
        Mem->Doc.Undo.Size    = Math::Min(100 * UndoRecordSize, MaxSize);
        if (Mem->Doc.Undo.Size < 2 * UndoRecordSize) { Mem->Doc.Undo.Size = 2 * UndoRecordSize; }

        Mem->Doc.Undo.Start = malloc((size_t)Mem->Doc.Undo.Size);
        Mem->Doc.Undo.CurrentIndex = -1;

        // TODO: Near-duplicate code from brush release. Combine.
        // Additive render-to-texture
        {
            GLCHK( glDisable(GL_SCISSOR_TEST) );
            GLCHK( glBindFramebuffer     (GL_FRAMEBUFFER, Mem->Misc.FrameBufferObject) );
            GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->Misc.FboRenderTexture, 0) );

            GLCHK( glViewport(0, 0, Mem->Doc.Width, Mem->Doc.Height) );
            GLCHK( glUseProgram(Mem->Shaders[PapayaShader_ImGui].Handle) );

            GLCHK( glUniformMatrix4fv(Mem->Shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &Mem->Doc.ProjMtx[0][0]) );

            GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_RTTAdd].VboHandle) );
            GL::SetVertexAttribs(Mem->Shaders[PapayaShader_ImGui]);

            GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->Doc.TextureID) );
            GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );

            PushUndo(Mem, Vec2i(0,0), Vec2i(Mem->Doc.Width, Mem->Doc.Height), 0);

            uint32 Temp = Mem->Misc.FboRenderTexture;
            Mem->Misc.FboRenderTexture = Mem->Doc.TextureID;
            GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->Misc.FboRenderTexture, 0) );
            Mem->Doc.TextureID = Temp;

            GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
            GLCHK( glViewport(0, 0, Mem->Doc.Width, Mem->Doc.Height) );

            GLCHK( glDisable(GL_BLEND) );
        }
    }

    Timer::StopTime(&Mem->Debug.Timers[Timer_ImageOpen]);

    return true;
}

void Core::CloseDocument(PapayaMemory* Mem)
{
    // Document
    if (Mem->Doc.TextureID)
    {
        GLCHK( glDeleteTextures(1, &Mem->Doc.TextureID) );
        Mem->Doc.TextureID = 0;
    }

    // Undo buffer
    {
        free(Mem->Doc.Undo.Start);
        Mem->Doc.Undo.Start = Mem->Doc.Undo.Top     = 0;
        Mem->Doc.Undo.Base  = Mem->Doc.Undo.Current = Mem->Doc.Undo.Last = 0;
        Mem->Doc.Undo.Size  = Mem->Doc.Undo.Count   = 0;
        Mem->Doc.Undo.CurrentIndex = -1;
    }

    // Frame buffer
    if (Mem->Misc.FrameBufferObject)
    {
        GLCHK( glDeleteFramebuffers(1, &Mem->Misc.FrameBufferObject) );
        Mem->Misc.FrameBufferObject = 0;
    }

    if (Mem->Misc.FboRenderTexture)
    {
        GLCHK( glDeleteTextures(1, &Mem->Misc.FboRenderTexture) );
        Mem->Misc.FboRenderTexture = 0;
    }

    if (Mem->Misc.FboSampleTexture)
    {
        GLCHK( glDeleteTextures(1, &Mem->Misc.FboSampleTexture) );
        Mem->Misc.FboSampleTexture = 0;
    }

    // Vertex Buffer: RTTBrush
    if (Mem->Meshes[PapayaMesh_RTTBrush].VboHandle)
    {
        GLCHK( glDeleteBuffers(1, &Mem->Meshes[PapayaMesh_RTTBrush].VboHandle) );
        Mem->Meshes[PapayaMesh_RTTBrush].VboHandle = 0;
    }

    // Vertex Buffer: RTTAdd
    if (Mem->Meshes[PapayaMesh_RTTAdd].VboHandle)
    {
        GLCHK( glDeleteBuffers(1, &Mem->Meshes[PapayaMesh_RTTAdd].VboHandle) );
        Mem->Meshes[PapayaMesh_RTTAdd].VboHandle = 0;
    }
}

void Core::Initialize(PapayaMemory* Mem)
{
    // Init values and load textures
    {
        Mem->Doc.Width = Mem->Doc.Height = 512;

        Mem->CurrentTool = PapayaTool_CropRotate;

        Mem->Brush.Diameter    = 0;
        Mem->Brush.MaxDiameter = 9999;
        Mem->Brush.Hardness    = 1.0f;
        Mem->Brush.Opacity     = 1.0f;
        Mem->Brush.AntiAlias   = true;

        Picker::Init(&Mem->Picker);

        Mem->Misc.DrawCanvas       = true;
        Mem->Misc.DrawOverlay      = false;
        Mem->Misc.ShowMetrics      = false;
        Mem->Misc.ShowUndoBuffer   = false;
        Mem->Misc.MenuOpen         = false;
        Mem->Misc.PrefsOpen        = false;
        Mem->Misc.PreviewImageSize = false;

        float OrthoMtx[4][4] =
        {
            { 2.0f,   0.0f,   0.0f,   0.0f },
            { 0.0f,  -2.0f,   0.0f,   0.0f },
            { 0.0f,   0.0f,  -1.0f,   0.0f },
            { -1.0f,  1.0f,   0.0f,   1.0f },
        };
        memcpy(Mem->Window.ProjMtx, OrthoMtx, sizeof(OrthoMtx));

        Mem->Colors[PapayaCol_Clear]             = Color(45, 45, 48);
        Mem->Colors[PapayaCol_Workspace]         = Color(30, 30, 30);
        Mem->Colors[PapayaCol_Button]            = Color(92, 92, 94);
        Mem->Colors[PapayaCol_ButtonHover]       = Color(64, 64, 64);
        Mem->Colors[PapayaCol_ButtonActive]      = Color(0, 122, 204);
        Mem->Colors[PapayaCol_AlphaGrid1]        = Color(141, 141, 142);
        Mem->Colors[PapayaCol_AlphaGrid2]        = Color(92, 92, 94);
        Mem->Colors[PapayaCol_ImageSizePreview1] = Color(55, 55, 55);
        Mem->Colors[PapayaCol_ImageSizePreview2] = Color(45, 45, 45);
        Mem->Colors[PapayaCol_Transparent]       = Color(0, 0, 0, 0);

        Mem->Textures[PapayaTex_UI] = LoadAndBindImage("ui.png");
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

        GL::CompileShader(Mem->Shaders[PapayaShader_Brush], __FILE__, __LINE__,
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

        GL::CompileShader(Mem->Shaders[PapayaShader_BrushCursor], __FILE__, __LINE__,
            Vert, Frag, 2, 4,
            "Position", "UV",
            "ProjMtx", "BrushColor", "Hardness", "PixelDiameter");

        GL::InitQuad(Mem->Meshes[PapayaMesh_BrushCursor],
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

        GL::CompileShader(Mem->Shaders[PapayaShader_EyeDropperCursor], __FILE__, __LINE__,
            Vert, Frag, 2, 3,
            "Position", "UV",
            "ProjMtx", "Color1", "Color2");

        GL::InitQuad(Mem->Meshes[PapayaMesh_EyeDropperCursor],
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
            "       vec3 RGB = hsv2rgb(vec3(Hue, Frag_UV.x, Frag_UV.y));                \n"
            "       float Dist = distance(Frag_UV, Cursor);                             \n"
            "                                                                           \n"
            "       if (Dist > Radius && Dist < Radius + Thickness)                     \n"
            "       {                                                                   \n"
            "           float a = (Cursor.x < 0.4 && Cursor.y > 0.6) ? 0.0 : 1.0;       \n"
            "           gl_FragColor = vec4(a, a, a, 1.0);                              \n"
            "       }                                                                   \n"
            "       else                                                                \n"
            "       {                                                                   \n"
            "           gl_FragColor = vec4(RGB.x, RGB.y, RGB.z, 1.0);                  \n"
            "       }                                                                   \n"
            "   }                                                                       \n";

        GL::CompileShader(Mem->Shaders[PapayaShader_PickerSVBox], __FILE__, __LINE__,
            Vert, Frag, 2, 3,
            "Position", "UV",
            "ProjMtx", "Hue", "Cursor");

        GL::InitQuad(Mem->Meshes[PapayaMesh_PickerSVBox],
            Mem->Picker.Pos + Mem->Picker.SVBoxPos, Mem->Picker.SVBoxSize, GL_STATIC_DRAW);
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
            "       vec4 Hue = vec4(hsv2rgb(vec3(Frag_UV.y, 1.0, 1.0)).xyz,1.0);\n"
            "       if (abs(0.5 - Frag_UV.x) > 0.3333)                          \n"
            "       {                                                           \n"
            "           gl_FragColor = vec4(0.36,0.36,0.37,                     \n"
            "                       float(abs(Frag_UV.y - Cursor) < 0.0039));   \n"
            "       }                                                           \n"
            "       else                                                        \n"
            "           gl_FragColor = Hue;                                     \n"
            "   }                                                               \n";

        GL::CompileShader(Mem->Shaders[PapayaShader_PickerHStrip], __FILE__, __LINE__,
            Vert, Frag, 2, 2,
            "Position", "UV",
            "ProjMtx", "Cursor");

        GL::InitQuad(Mem->Meshes[PapayaMesh_PickerHStrip],
            Mem->Picker.Pos + Mem->Picker.HueStripPos, Mem->Picker.HueStripSize, GL_STATIC_DRAW);
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

        GL::CompileShader(Mem->Shaders[PapayaShader_ImageSizePreview], __FILE__, __LINE__,
            Vert, Frag, 2, 5,
            "Position", "UV",
            "ProjMtx", "Color1", "Color2", "Width", "Height");

        GL::InitQuad(Mem->Meshes[PapayaMesh_ImageSizePreview],
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

        GL::CompileShader(Mem->Shaders[PapayaShader_AlphaGrid], __FILE__, __LINE__,
            Vert, Frag, 2, 6,
            "Position", "UV",
            "ProjMtx", "Color1", "Color2", "Zoom", "InvAspect", "MaxDim");

        GL::InitQuad(Mem->Meshes[PapayaMesh_AlphaGrid],
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

        GL::CompileShader(Mem->Shaders[PapayaShader_PreMultiplyAlpha], __FILE__, __LINE__,
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

        GL::CompileShader(Mem->Shaders[PapayaShader_DeMultiplyAlpha], __FILE__, __LINE__,
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

        GL::CompileShader(Mem->Shaders[PapayaShader_ImGui], __FILE__, __LINE__,
            Vert, Frag, 3, 2,
            "Position", "UV", "Color",
            "ProjMtx", "Texture");

        GL::InitQuad(Mem->Meshes[PapayaMesh_Canvas],
            Vec2(0,0), Vec2(10,10), GL_DYNAMIC_DRAW);
    }

    // Setup for ImGui
    {
        GLCHK( glGenBuffers(1, &Mem->Meshes[PapayaMesh_ImGui].VboHandle) );
        GLCHK( glGenBuffers(1, &Mem->Meshes[PapayaMesh_ImGui].ElementsHandle) );

        // Create fonts texture
        ImGuiIO& io = ImGui::GetIO();

        unsigned char* pixels;
        int width, height;
        //ImFont* my_font0 = io.Fonts->AddFontFromFileTTF("d:\\DroidSans.ttf", 15.0f);
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        GLCHK( glGenTextures  (1, &Mem->Textures[PapayaTex_Font]) );
        GLCHK( glBindTexture  (GL_TEXTURE_2D, Mem->Textures[PapayaTex_Font]) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
        GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
        GLCHK( glTexImage2D   (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels) );

        // Store our identifier
        io.Fonts->TexID = (void *)(intptr_t)Mem->Textures[PapayaTex_Font];

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

void Core::Shutdown(PapayaMemory* Mem)
{
    //TODO: Free stuff
}

void Core::UpdateAndRender(PapayaMemory* Mem)
{
    // Initialize frame
    {
        // Current mouse info
        {
            Mem->Mouse.Pos = Math::RoundToVec2i(ImGui::GetMousePos());
            Vec2 MousePixelPos = Vec2(Math::Floor((Mem->Mouse.Pos.x - Mem->Doc.CanvasPosition.x) / Mem->Doc.CanvasZoom),
                Math::Floor((Mem->Mouse.Pos.y - Mem->Doc.CanvasPosition.y) / Mem->Doc.CanvasZoom));
            Mem->Mouse.UV = Vec2(MousePixelPos.x / (float) Mem->Doc.Width, MousePixelPos.y / (float) Mem->Doc.Height);

            for (int32 i = 0; i < 3; i++)
            {
                Mem->Mouse.IsDown[i]   = ImGui::IsMouseDown(i);
                Mem->Mouse.Pressed[i]  = (Mem->Mouse.IsDown[i] && !Mem->Mouse.WasDown[i]);
                Mem->Mouse.Released[i] = (!Mem->Mouse.IsDown[i] && Mem->Mouse.WasDown[i]);
            }

            // OnCanvas test
            {
                Mem->Mouse.InWorkspace = true;

                if (Mem->Mouse.Pos.x <= 34 ||                      // Document workspace test
                    Mem->Mouse.Pos.x >= Mem->Window.Width - 3 ||   // TODO: Formalize the window layout and
                    Mem->Mouse.Pos.y <= 55 ||                      //       remove magic numbers throughout
                    Mem->Mouse.Pos.y >= Mem->Window.Height - 3)    //       the code.
                {
                    Mem->Mouse.InWorkspace = false;
                }
                else if (Mem->Picker.Open &&
                    Mem->Mouse.Pos.x > Mem->Picker.Pos.x &&                       // Color picker test
                    Mem->Mouse.Pos.x < Mem->Picker.Pos.x + Mem->Picker.Size.x &&  //
                    Mem->Mouse.Pos.y > Mem->Picker.Pos.y &&                       //
                    Mem->Mouse.Pos.y < Mem->Picker.Pos.y + Mem->Picker.Size.y)    //
                {
                    Mem->Mouse.InWorkspace = false;
                }
            }
        }

        // Clear screen buffer
        {
            GLCHK( glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y) );

            GLCHK( glClearColor(Mem->Colors[PapayaCol_Clear].r,
                Mem->Colors[PapayaCol_Clear].g,
                Mem->Colors[PapayaCol_Clear].b, 1.0f) );
            GLCHK( glClear(GL_COLOR_BUFFER_BIT) );

            GLCHK( glEnable(GL_SCISSOR_TEST) );
            GLCHK( glScissor(34, 3,
                (int)Mem->Window.Width  - 70,
                (int)Mem->Window.Height - 58) ); // TODO: Remove magic numbers

            GLCHK( glClearColor(Mem->Colors[PapayaCol_Workspace].r,
                Mem->Colors[PapayaCol_Workspace].g,
                Mem->Colors[PapayaCol_Workspace].b, 1.0f) );
            GLCHK( glClear(GL_COLOR_BUFFER_BIT) );

            GLCHK( glDisable(GL_SCISSOR_TEST) );
        }

        // Set projection matrix
        Mem->Window.ProjMtx[0][0] =  2.0f / ImGui::GetIO().DisplaySize.x;
        Mem->Window.ProjMtx[1][1] = -2.0f / ImGui::GetIO().DisplaySize.y;
    }

    // Title Bar Menu
    {
        ImGui::SetNextWindowSize(ImVec2(Mem->Window.Width - Mem->Window.MenuHorizontalOffset - Mem->Window.TitleBarButtonsWidth - 3.0f,
            Mem->Window.TitleBarHeight - 10.0f));
        ImGui::SetNextWindowPos(ImVec2(2.0f + Mem->Window.MenuHorizontalOffset, 6.0f));

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

        ImGui::PushStyleColor(ImGuiCol_WindowBg           , Mem->Colors[PapayaCol_Transparent]);
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg          , Mem->Colors[PapayaCol_Transparent]);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered      , Mem->Colors[PapayaCol_ButtonHover]);
        ImGui::PushStyleVar  (ImGuiStyleVar_WindowPadding , ImVec2(8, 4));
        ImGui::PushStyleColor(ImGuiCol_Header             , Mem->Colors[PapayaCol_Transparent]);

        Mem->Misc.MenuOpen = false;

        ImGui::Begin("Title Bar Menu", 0, WindowFlags);
        if (ImGui::BeginMenuBar())
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, Mem->Colors[PapayaCol_Clear]);
            if (ImGui::BeginMenu("FILE"))
            {
                Mem->Misc.MenuOpen = true;

                if (Mem->Doc.TextureID) // A document is already open
                {
                    if (ImGui::MenuItem("Close")) { CloseDocument(Mem); }
                    if (ImGui::MenuItem("Save"))
                    {
                        char* Path = Platform::SaveFileDialog();
                        uint8* Texture = (uint8*)malloc(4 * Mem->Doc.Width * Mem->Doc.Height);
                        if (Path) // TODO: Do this on a separate thread. Massively blocks UI for large images.
                        {
                            GLCHK(glBindTexture(GL_TEXTURE_2D, Mem->Doc.TextureID));
                            GLCHK(glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, Texture));

                            int32 Result = stbi_write_png(Path, Mem->Doc.Width, Mem->Doc.Height, 4, Texture, 4 * Mem->Doc.Width);
                            if (!Result)
                            {
                                // TODO: Log: Save failed
                                Platform::Print("Save failed\n");
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
                        char* Path = Platform::OpenFileDialog();
                        if (Path)
                        {
                            OpenDocument(Path, Mem);
                            free(Path);
                        }
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Quit", "Alt+F4")) { Mem->IsRunning = false; }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("EDIT"))
            {
                Mem->Misc.MenuOpen = true;
                if (ImGui::MenuItem("Preferences...", 0)) { Mem->Misc.PrefsOpen = true; }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("VIEW"))
            {
                Mem->Misc.MenuOpen = true;
                ImGui::MenuItem("Metrics Window", NULL, &Mem->Misc.ShowMetrics);
                ImGui::MenuItem("Undo Buffer Window", NULL, &Mem->Misc.ShowUndoBuffer);
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

        ImGui::PushStyleColor(ImGuiCol_ButtonActive  , Mem->Colors[PapayaCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_WindowBg      , Mem->Colors[PapayaCol_Transparent]);

        ImGui::Begin("Left toolbar", 0, WindowFlags);

#define CALCUV(X, Y) ImVec2((float)X/256.0f, (float)Y/256.0f)
        {
            ImGui::PushID(0);
            ImGui::PushStyleColor(ImGuiCol_Button       , (Mem->CurrentTool == PapayaTool_Brush) ? Mem->Colors[PapayaCol_Button] :  Mem->Colors[PapayaCol_Transparent]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (Mem->CurrentTool == PapayaTool_Brush) ? Mem->Colors[PapayaCol_Button] :  Mem->Colors[PapayaCol_ButtonHover]);
            if (ImGui::ImageButton((void*)(intptr_t)Mem->Textures[PapayaTex_UI], ImVec2(20, 20), CALCUV(0, 0), CALCUV(20, 20), 6, ImVec4(0, 0, 0, 0)))
            {
                Mem->CurrentTool = (Mem->CurrentTool != PapayaTool_Brush) ? PapayaTool_Brush : PapayaTool_None;

            }
            ImGui::PopStyleColor(2);
            ImGui::PopID();

            ImGui::PushID(1);
            ImGui::PushStyleColor(ImGuiCol_Button       , (Mem->CurrentTool == PapayaTool_EyeDropper) ? Mem->Colors[PapayaCol_Button] :  Mem->Colors[PapayaCol_Transparent]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (Mem->CurrentTool == PapayaTool_EyeDropper) ? Mem->Colors[PapayaCol_Button] :  Mem->Colors[PapayaCol_ButtonHover]);
            if (ImGui::ImageButton((void*)(intptr_t)Mem->Textures[PapayaTex_UI], ImVec2(20, 20), CALCUV(20, 0), CALCUV(40, 20), 6, ImVec4(0, 0, 0, 0)))
            {
                Mem->CurrentTool = (Mem->CurrentTool != PapayaTool_EyeDropper) ? PapayaTool_EyeDropper : PapayaTool_None;
            }
            ImGui::PopStyleColor(2);
            ImGui::PopID();

            ImGui::PushID(2);
            ImGui::PushStyleColor(ImGuiCol_Button       , (Mem->CurrentTool == PapayaTool_CropRotate) ? Mem->Colors[PapayaCol_Button] :  Mem->Colors[PapayaCol_Transparent]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (Mem->CurrentTool == PapayaTool_CropRotate) ? Mem->Colors[PapayaCol_Button] :  Mem->Colors[PapayaCol_ButtonHover]);
            if (ImGui::ImageButton((void*)(intptr_t)Mem->Textures[PapayaTex_UI], ImVec2(20, 20), CALCUV(40, 0), CALCUV(60, 20), 6, ImVec4(0, 0, 0, 0)))
            {
                Mem->CurrentTool = (Mem->CurrentTool != PapayaTool_CropRotate) ? PapayaTool_CropRotate : PapayaTool_None;
            }
            ImGui::PopStyleColor(2);
            ImGui::PopID();

            ImGui::PushID(3);
            if (ImGui::ImageButton((void*)(intptr_t)Mem->Textures[PapayaTex_UI], ImVec2(33, 33), CALCUV(0, 0), CALCUV(0, 0), 0, Mem->Picker.CurrentColor))
            {
                Mem->Picker.Open = !Mem->Picker.Open;
                Picker::SetColor(Mem->Picker.CurrentColor, &Mem->Picker);
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
        ImGui::SetNextWindowPos (ImVec2((float)Mem->Window.Width - 36, 57));

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

        ImGui::PushStyleColor(ImGuiCol_ButtonActive  , Mem->Colors[PapayaCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_WindowBg      , Mem->Colors[PapayaCol_Transparent]);

        ImGui::Begin("Right toolbar", 0, WindowFlags);

#define CALCUV(X, Y) ImVec2((float)X/256.0f, (float)Y/256.0f)
        {
            ImGui::PushID(0);
            ImGui::PushStyleColor(ImGuiCol_Button       , (Mem->Misc.PrefsOpen) ? Mem->Colors[PapayaCol_Button] :  Mem->Colors[PapayaCol_Transparent]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (Mem->Misc.PrefsOpen) ? Mem->Colors[PapayaCol_Button] :  Mem->Colors[PapayaCol_ButtonHover]);
            if (ImGui::ImageButton((void*)(intptr_t)Mem->Textures[PapayaTex_UI], ImVec2(20, 20), CALCUV(40, 0), CALCUV(60, 20), 6, ImVec4(0, 0, 0, 0)))
            {
                Mem->Misc.PrefsOpen = !Mem->Misc.PrefsOpen;
            }
            ImGui::PopStyleColor(2);
            ImGui::PopID();
        }
#undef CALCUV

        ImGui::End();

        ImGui::PopStyleVar(5);
        ImGui::PopStyleColor(2);
    }

    if (Mem->Misc.PrefsOpen) { Prefs::ShowPanel(&Mem->Picker, Mem->Colors, Mem->Window); }

    // Color Picker
    if (Mem->Picker.Open)
    {
        Picker::UpdateAndRender(&Mem->Picker, Mem->Colors,
                Mem->Mouse, Mem->Textures[PapayaTex_UI]);
    }

    // Tool Param Bar
    {
        ImGui::SetNextWindowSize(ImVec2((float)Mem->Window.Width - 70, 30));
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

        ImGui::PushStyleColor(ImGuiCol_Button           , Mem->Colors[PapayaCol_Button]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered    , Mem->Colors[PapayaCol_ButtonHover]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive     , Mem->Colors[PapayaCol_ButtonActive]);
        ImGui::PushStyleColor(ImGuiCol_WindowBg         , Mem->Colors[PapayaCol_Transparent]);
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive , Mem->Colors[PapayaCol_ButtonActive]);

        bool Show = true;
        ImGui::Begin("Tool param bar", &Show, WindowFlags);

        // New document options. Might convert into modal window later.
        if (!Mem->Doc.TextureID) // No document is open
        {
            // Size
            {
                int32 size[2];
                size[0] = Mem->Doc.Width;
                size[1] = Mem->Doc.Height;
                ImGui::PushItemWidth(85);
                ImGui::InputInt2("Size", size);
                ImGui::PopItemWidth();
                Mem->Doc.Width  = Math::Clamp(size[0], 1, 9000);
                Mem->Doc.Height = Math::Clamp(size[1], 1, 9000);
                ImGui::SameLine();
                ImGui::Checkbox("Preview", &Mem->Misc.PreviewImageSize);
            }

            // "New" button
            {
                ImGui::SameLine(ImGui::GetWindowWidth() - 70); // TODO: Magic number alert
                if (ImGui::Button("New Image"))
                {
                    Mem->Doc.ComponentsPerPixel = 4;
                    OpenDocument(0, Mem);
                }
            }
        }
        else  // Document is open
        {
            if (Mem->CurrentTool == PapayaTool_Brush)
            {
                ImGui::PushItemWidth(85);
                ImGui::InputInt("Diameter", &Mem->Brush.Diameter);
                Mem->Brush.Diameter = Math::Clamp(Mem->Brush.Diameter, 1, Mem->Brush.MaxDiameter);

                ImGui::PopItemWidth();
                ImGui::PushItemWidth(80);
                ImGui::SameLine();

                float ScaledHardness = Mem->Brush.Hardness * 100.0f;
                ImGui::SliderFloat("Hardness", &ScaledHardness, 0.0f, 100.0f, "%.0f");
                Mem->Brush.Hardness = ScaledHardness / 100.0f;
                ImGui::SameLine();

                float ScaledOpacity = Mem->Brush.Opacity * 100.0f;
                ImGui::SliderFloat("Opacity", &ScaledOpacity, 0.0f, 100.0f, "%.0f");
                Mem->Brush.Opacity = ScaledOpacity / 100.0f;
                ImGui::SameLine();

                ImGui::Checkbox("Anti-alias", &Mem->Brush.AntiAlias); // TODO: Replace this with a toggleable icon button

                ImGui::PopItemWidth();
            }
            else if (Mem->CurrentTool == PapayaTool_CropRotate)
            {
                if (ImGui::Button("-90"))
                {
                    Mem->CropRotate.BaseAngle -= Math::ToRadians(90.f);
                }
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3, 0));
                ImGui::SameLine();
                if (ImGui::Button("+90"))
                {
                    Mem->CropRotate.BaseAngle += Math::ToRadians(90.f);
                }
                ImGui::SameLine();
                ImGui::PopStyleVar();

                ImGui::PushItemWidth(85);
                ImGui::SliderAngle("Rotate", &Mem->CropRotate.SliderAngle, -45.0f, 45.0f);
                ImGui::PopItemWidth();

                ImGui::SameLine(ImGui::GetWindowWidth() - 94); // TODO: Magic number alert
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));

                if (ImGui::Button("Apply"))
                {
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
                        Vec2 Offset = Vec2(Mem->Doc.Width  * 0.5f,
                                           Mem->Doc.Height * 0.5f);

                        mat4x4_dup(M, Mem->Doc.ProjMtx);
                        mat4x4_translate_in_place(M, Offset.x, Offset.y, 0.f);
                        mat4x4_rotate_Z(R, M, -(Mem->CropRotate.SliderAngle +
                                    Mem->CropRotate.BaseAngle));
                        mat4x4_translate_in_place(R, -Offset.x, -Offset.y, 0.f);
                    }

                    // Draw the image onto the frame buffer
                    GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_RTTAdd].VboHandle) );
                    GLCHK( glUseProgram(Mem->Shaders[PapayaShader_DeMultiplyAlpha].Handle) );
                    GLCHK( glUniformMatrix4fv(
                                Mem->Shaders[PapayaShader_ImGui].Uniforms[0],
                                1, GL_FALSE, (GLfloat*)R) );
                    GL::SetVertexAttribs(Mem->Shaders[PapayaShader_DeMultiplyAlpha]);
                    GLCHK( glBindTexture(GL_TEXTURE_2D,
                                (GLuint)(intptr_t)Mem->Doc.TextureID) );
                    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
                    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
                    GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );

                    // Swap render texture and document texture handles
                    uint32 Temp = Mem->Misc.FboSampleTexture;
                    Mem->Misc.FboSampleTexture = Mem->Doc.TextureID;
                    Mem->Doc.TextureID = Temp;

                    // Reset stuff
                    GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
                    GLCHK( glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y) );

                    Mem->CropRotate.SliderAngle = 0.f;
                    Mem->CropRotate.BaseAngle   = 0.f;
                }

                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    Mem->CropRotate.SliderAngle = 0.f;
                    Mem->CropRotate.BaseAngle   = 0.f;
                }
                ImGui::PopStyleVar();
            }
        }

        ImGui::End();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(5);
    }

    // Image size preview
    if (!Mem->Doc.TextureID && Mem->Misc.PreviewImageSize)
    {
        int32 TopMargin = 53; // TODO: Put common layout constants in struct
        GL::TransformQuad(Mem->Meshes[PapayaMesh_ImageSizePreview],
            Vec2((Mem->Window.Width - Mem->Doc.Width) / 2, TopMargin + (Mem->Window.Height - TopMargin - Mem->Doc.Height) / 2),
            Vec2(Mem->Doc.Width, Mem->Doc.Height));

        GL::DrawQuad(Mem->Meshes[PapayaMesh_ImageSizePreview], Mem->Shaders[PapayaShader_ImageSizePreview], true,
            5,
            UniformType_Matrix4, &Mem->Window.ProjMtx[0][0],
            UniformType_Color, Mem->Colors[PapayaCol_ImageSizePreview1],
            UniformType_Color, Mem->Colors[PapayaCol_ImageSizePreview2],
            UniformType_Float, (float) Mem->Doc.Width,
            UniformType_Float, (float) Mem->Doc.Height);
    }

    if (!Mem->Doc.TextureID) { goto EndOfDoc; }

    // Brush tool
    {
        /*
        if (ImGui::IsKeyPressed(VK_UP, false))
        {
        Mem->Brush.Diameter++;
        Mem->Misc.DrawCanvas = !Mem->Misc.DrawCanvas;
        }
        if (ImGui::IsKeyPressed(VK_DOWN, false))
        {
        Mem->Brush.Diameter--;
        Mem->Misc.DrawOverlay = !Mem->Misc.DrawOverlay;
        }
        if (ImGui::IsKeyPressed(VK_NUMPAD1, false))
        {
        Mem->Doc.CanvasZoom = 1.0;
        }
        */

        if (Mem->CurrentTool == PapayaTool_Brush &&
            !Mem->Misc.MenuOpen &&
            (!ImGui::GetIO().KeyAlt || Mem->Mouse.IsDown[1] || Mem->Mouse.Released[1]))
        {
            // Right mouse dragging
            {
                if (Mem->Mouse.Pressed[1])
                {
                    Mem->Brush.RtDragStartPos      = Mem->Mouse.Pos;
                    Mem->Brush.RtDragStartDiameter = Mem->Brush.Diameter;
                    Mem->Brush.RtDragStartHardness = Mem->Brush.Hardness;
                    Mem->Brush.RtDragStartOpacity  = Mem->Brush.Opacity;
                    Mem->Brush.RtDragWithShift     = ImGui::GetIO().KeyShift;
                    Platform::StartMouseCapture();
                    Platform::SetCursorVisibility(false);
                }
                else if (Mem->Mouse.IsDown[1])
                {
                    if (Mem->Brush.RtDragWithShift)
                    {
                        float Opacity = Mem->Brush.RtDragStartOpacity + (ImGui::GetMouseDragDelta(1).x * 0.0025f);
                        Mem->Brush.Opacity = Math::Clamp(Opacity, 0.0f, 1.0f);
                    }
                    else
                    {
                        float Diameter = Mem->Brush.RtDragStartDiameter + (ImGui::GetMouseDragDelta(1).x / Mem->Doc.CanvasZoom * 2.0f);
                        Mem->Brush.Diameter = Math::Clamp((int32)Diameter, 1, Mem->Brush.MaxDiameter);

                        float Hardness = Mem->Brush.RtDragStartHardness + (ImGui::GetMouseDragDelta(1).y * 0.0025f);
                        Mem->Brush.Hardness = Math::Clamp(Hardness, 0.0f, 1.0f);
                    }
                }
                else if (Mem->Mouse.Released[1])
                {
                    Platform::ReleaseMouseCapture();
                    Platform::SetMousePosition(Mem->Brush.RtDragStartPos.x, Mem->Brush.RtDragStartPos.y);
                    Platform::SetCursorVisibility(true);
                }
            }

            if (Mem->Mouse.Pressed[0] && Mem->Mouse.InWorkspace)
            {
                Mem->Brush.BeingDragged = true;
                if (Mem->Picker.Open) { Mem->Picker.CurrentColor = Mem->Picker.NewColor; }
                Mem->Brush.PaintArea1 = Vec2i(Mem->Doc.Width + 1, Mem->Doc.Height + 1);
                Mem->Brush.PaintArea2 = Vec2i(0,0);
            }
            else if (Mem->Mouse.Released[0] && Mem->Brush.BeingDragged)
            {
                // TODO: Make a vararg-based RTT function
                // Additive render-to-texture
                {
                    GLCHK( glDisable(GL_SCISSOR_TEST) );
                    GLCHK( glDisable(GL_DEPTH_TEST) );
                    GLCHK( glBindFramebuffer     (GL_FRAMEBUFFER, Mem->Misc.FrameBufferObject) );
                    GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->Misc.FboRenderTexture, 0) );
                    GLCHK( glViewport(0, 0, Mem->Doc.Width, Mem->Doc.Height) );

                    Vec2i Pos  = Mem->Brush.PaintArea1;
                    Vec2i Size = Mem->Brush.PaintArea2 - Mem->Brush.PaintArea1;
                    int8* PreBrushImage = 0;

                    // TODO: OPTIMIZE: The following block seems optimizable
                    // Render base image for pre-brush undo
                    {
                        GLCHK( glDisable(GL_BLEND) );

                        GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_RTTAdd].VboHandle) );
                        GLCHK( glUseProgram(Mem->Shaders[PapayaShader_ImGui].Handle) );
                        GLCHK( glUniformMatrix4fv(Mem->Shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &Mem->Doc.ProjMtx[0][0]) );
                        GL::SetVertexAttribs(Mem->Shaders[PapayaShader_ImGui]);

                        GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->Doc.TextureID) );
                        GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );

                        PreBrushImage = (int8*)malloc(4 * Size.x * Size.y);
                        GLCHK( glReadPixels(Pos.x, Pos.y, Size.x, Size.y, GL_RGBA, GL_UNSIGNED_BYTE, PreBrushImage) );
                    }

                    // Render base image with premultiplied alpha
                    {
                        GLCHK( glUseProgram(Mem->Shaders[PapayaShader_PreMultiplyAlpha].Handle) );
                        GLCHK( glUniformMatrix4fv(Mem->Shaders[PapayaShader_PreMultiplyAlpha].Uniforms[0], 1, GL_FALSE, &Mem->Doc.ProjMtx[0][0]) );
                        GL::SetVertexAttribs(Mem->Shaders[PapayaShader_PreMultiplyAlpha]);

                        GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->Doc.TextureID) );
                        GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );
                    }

                    // Render brush overlay with premultiplied alpha
                    {
                        GLCHK( glEnable(GL_BLEND) );
                        GLCHK( glBlendEquation(GL_FUNC_ADD) );
                        GLCHK( glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA) );

                        GLCHK( glUseProgram(Mem->Shaders[PapayaShader_PreMultiplyAlpha].Handle) );
                        GLCHK( glUniformMatrix4fv(Mem->Shaders[PapayaShader_PreMultiplyAlpha].Uniforms[0], 1, GL_FALSE, &Mem->Doc.ProjMtx[0][0]) );
                        GL::SetVertexAttribs(Mem->Shaders[PapayaShader_PreMultiplyAlpha]);

                        GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->Misc.FboSampleTexture) );
                        GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );
                    }

                    // Render blended result with demultiplied alpha
                    {
                        GLCHK( glDisable(GL_BLEND) );

                        GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->Doc.TextureID, 0) );
                        GLCHK( glUseProgram(Mem->Shaders[PapayaShader_DeMultiplyAlpha].Handle) );
                        GLCHK( glUniformMatrix4fv(Mem->Shaders[PapayaShader_DeMultiplyAlpha].Uniforms[0], 1, GL_FALSE, &Mem->Doc.ProjMtx[0][0]) );
                        GL::SetVertexAttribs(Mem->Shaders[PapayaShader_DeMultiplyAlpha]);

                        GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->Misc.FboRenderTexture) );
                        GLCHK( glDrawArrays (GL_TRIANGLES, 0, 6) );
                    }

                    PushUndo(Mem, Pos, Size, PreBrushImage);

                    if (PreBrushImage) { free(PreBrushImage); }

                    GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
                    GLCHK( glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y) );

                    GLCHK( glDisable(GL_BLEND) );
                }

                Mem->Misc.DrawOverlay   = false;
                Mem->Brush.BeingDragged = false;
            }

            if (Mem->Brush.BeingDragged)
            {
                Mem->Misc.DrawOverlay = true;

                GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, Mem->Misc.FrameBufferObject) );

                if (Mem->Mouse.Pressed[0])
                {
                    GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->Misc.FboSampleTexture, 0) );
                    GLCHK( glClearColor(0.0f, 0.0f, 0.0f, 0.0f) );
                    GLCHK( glClear(GL_COLOR_BUFFER_BIT) );
                    GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->Misc.FboRenderTexture, 0) );
                }
                GLCHK( glViewport(0, 0, Mem->Doc.Width, Mem->Doc.Height) );

                GLCHK( glDisable(GL_BLEND) );
                GLCHK( glDisable(GL_SCISSOR_TEST) );

                // Setup orthographic projection matrix
                float width  = (float)Mem->Doc.Width;
                float height = (float)Mem->Doc.Height;
                GLCHK( glUseProgram(Mem->Shaders[PapayaShader_Brush].Handle) );

                Vec2 CorrectedPos     = Mem->Mouse.UV     + (Mem->Brush.Diameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
                Vec2 CorrectedLastPos = Mem->Mouse.LastUV + (Mem->Brush.Diameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));

#if 0
                // Brush testing routine
                local_persist int32 i = 0;

                if (i%2)
                {
                    local_persist int32 j = 0;
                    CorrectedPos		= Vec2( j*0.2f,     j*0.2f) + (Mem->Brush.Diameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
                    CorrectedLastPos	= Vec2((j+1)*0.2f, (j+1)*0.2f) + (Mem->Brush.Diameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
                    j++;
                }
                else
                {
                    local_persist int32 k = 0;
                    CorrectedPos		= Vec2( k*0.2f,     1.0f-k*0.2f) + (Mem->Brush.Diameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
                    CorrectedLastPos	= Vec2((k+1)*0.2f, 1.0f-(k+1)*0.2f) + (Mem->Brush.Diameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
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

                    int32 PixelMinX = Math::RoundToInt(UVMinX * Mem->Doc.Width  - 0.5f * Mem->Brush.Diameter);
                    int32 PixelMinY = Math::RoundToInt(UVMinY * Mem->Doc.Height - 0.5f * Mem->Brush.Diameter);
                    int32 PixelMaxX = Math::RoundToInt(UVMaxX * Mem->Doc.Width  + 0.5f * Mem->Brush.Diameter);
                    int32 PixelMaxY = Math::RoundToInt(UVMaxY * Mem->Doc.Height + 0.5f * Mem->Brush.Diameter);

                    Mem->Brush.PaintArea1.x = Math::Min(Mem->Brush.PaintArea1.x, PixelMinX);
                    Mem->Brush.PaintArea1.y = Math::Min(Mem->Brush.PaintArea1.y, PixelMinY);
                    Mem->Brush.PaintArea2.x = Math::Max(Mem->Brush.PaintArea2.x, PixelMaxX);
                    Mem->Brush.PaintArea2.y = Math::Max(Mem->Brush.PaintArea2.y, PixelMaxY);

                    Mem->Brush.PaintArea1.x = Math::Clamp(Mem->Brush.PaintArea1.x, 0, Mem->Doc.Width);
                    Mem->Brush.PaintArea1.y = Math::Clamp(Mem->Brush.PaintArea1.y, 0, Mem->Doc.Height);
                    Mem->Brush.PaintArea2.x = Math::Clamp(Mem->Brush.PaintArea2.x, 0, Mem->Doc.Width);
                    Mem->Brush.PaintArea2.y = Math::Clamp(Mem->Brush.PaintArea2.y, 0, Mem->Doc.Height);
                }

                GLCHK( glUniformMatrix4fv(Mem->Shaders[PapayaShader_Brush].Uniforms[0], 1, GL_FALSE, &Mem->Doc.ProjMtx[0][0]) );
                GLCHK( glUniform2f(Mem->Shaders[PapayaShader_Brush].Uniforms[2], CorrectedPos.x, CorrectedPos.y * Mem->Doc.InverseAspect) ); // Pos uniform
                GLCHK( glUniform2f(Mem->Shaders[PapayaShader_Brush].Uniforms[3], CorrectedLastPos.x, CorrectedLastPos.y * Mem->Doc.InverseAspect) ); // Lastpos uniform
                GLCHK( glUniform1f(Mem->Shaders[PapayaShader_Brush].Uniforms[4], (float)Mem->Brush.Diameter / ((float)Mem->Doc.Width * 2.0f)) );
                float Opacity = Mem->Brush.Opacity;
                //if (Mem->Tablet.Pressure > 0.0f) { Opacity *= Mem->Tablet.Pressure; }
                GLCHK( glUniform4f(Mem->Shaders[PapayaShader_Brush].Uniforms[5], Mem->Picker.CurrentColor.r,
                    Mem->Picker.CurrentColor.g,
                    Mem->Picker.CurrentColor.b,
                    Opacity) );
                // Brush hardness
                {
                    float Hardness;
                    if (Mem->Brush.AntiAlias && Mem->Brush.Diameter > 2)
                    {
                        float AAWidth = 1.0f; // The width of pixels over which the antialiased falloff occurs
                        float Radius  = Mem->Brush.Diameter / 2.0f;
                        Hardness      = Math::Min(Mem->Brush.Hardness, 1.0f - (AAWidth / Radius));
                    }
                    else
                    {
                        Hardness      = Mem->Brush.Hardness;
                    }

                    GLCHK( glUniform1f(Mem->Shaders[PapayaShader_Brush].Uniforms[6], Hardness) );
                }

                GLCHK( glUniform1f(Mem->Shaders[PapayaShader_Brush].Uniforms[7], Mem->Doc.InverseAspect) ); // Inverse Aspect uniform

                GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_RTTBrush].VboHandle) );
                GL::SetVertexAttribs(Mem->Shaders[PapayaShader_Brush]);

                GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->Misc.FboSampleTexture) );
                GLCHK( glDrawArrays(GL_TRIANGLES, 0, 6) );

                uint32 Temp = Mem->Misc.FboRenderTexture;
                Mem->Misc.FboRenderTexture = Mem->Misc.FboSampleTexture;
                GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Mem->Misc.FboRenderTexture, 0) );
                Mem->Misc.FboSampleTexture = Temp;

                GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
                GLCHK( glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y) );
            }


#if 0
            // =========================================================================================
            // Visualization: Brush falloff

            const int32 ArraySize = 256;
            local_persist float Opacities[ArraySize] = { 0 };

            float MaxScale = 90.0f;
            float Scale    = 1.0f / (1.0f - Mem->Brush.Hardness);
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
    }

    // Undo/Redo
    {
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) // Pop undo op
        {
            // TODO: Clean up this workflow
            bool Refresh = false;

            if (ImGui::GetIO().KeyShift &&
                Mem->Doc.Undo.CurrentIndex < Mem->Doc.Undo.Count - 1 &&
                Mem->Doc.Undo.Current->Next != 0) // Redo
            {
                Mem->Doc.Undo.Current = Mem->Doc.Undo.Current->Next;
                Mem->Doc.Undo.CurrentIndex++;
                Refresh = true;
            }
            else if (!ImGui::GetIO().KeyShift &&
                Mem->Doc.Undo.CurrentIndex > 0 &&
                Mem->Doc.Undo.Current->Prev != 0) // Undo
            {
                if (Mem->Doc.Undo.Current->IsSubRect)
                {
                    LoadFromUndoBuffer(Mem, true);
                }
                else
                {
                    Refresh = true;
                }

                Mem->Doc.Undo.Current = Mem->Doc.Undo.Current->Prev;
                Mem->Doc.Undo.CurrentIndex--;
            }

            if (Refresh)
            {
                LoadFromUndoBuffer(Mem, false);
            }
        }

        // Visualization: Undo buffer
        if (Mem->Misc.ShowUndoBuffer)
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
            uint64 BaseOffset = (int8*)Mem->Doc.Undo.Base - (int8*)Mem->Doc.Undo.Start;
            float BaseX       = P1.x + (float)BaseOffset / (float)Mem->Doc.Undo.Size * (P2.x - P1.x);
            DrawList->AddLine(Vec2(BaseX, Pos.y + 26), Vec2(BaseX,Pos.y + 54), 0xFFFFFF00);

            // Current mark
            uint64 CurrOffset = (int8*)Mem->Doc.Undo.Current - (int8*)Mem->Doc.Undo.Start;
            float CurrX       = P1.x + (float)CurrOffset / (float)Mem->Doc.Undo.Size * (P2.x - P1.x);
            DrawList->AddLine(Vec2(CurrX, Pos.y + 29), Vec2(CurrX, Pos.y + 51), 0xFFFF00FF);

            // Last mark
            uint64 LastOffset = (int8*)Mem->Doc.Undo.Last - (int8*)Mem->Doc.Undo.Start;
            float LastX       = P1.x + (float)LastOffset / (float)Mem->Doc.Undo.Size * (P2.x - P1.x);
            //DrawList->AddLine(Vec2(LastX, Pos.y + 32), Vec2(LastX, Pos.y + 48), 0xFF0000FF);

            // Top mark
            uint64 TopOffset = (int8*)Mem->Doc.Undo.Top - (int8*)Mem->Doc.Undo.Start;
            float TopX       = P1.x + (float)TopOffset / (float)Mem->Doc.Undo.Size * (P2.x - P1.x);
            DrawList->AddLine(Vec2(TopX, Pos.y + 35), Vec2(TopX, Pos.y + 45), 0xFF00FFFF);

            ImGui::Text(" "); ImGui::Text(" "); // Vertical spacers
            ImGui::TextColored  (Color(0.0f,1.0f,1.0f,1.0f), "Base    %llu", BaseOffset);
            ImGui::TextColored  (Color(1.0f,0.0f,1.0f,1.0f), "Current %llu", CurrOffset);
            //ImGui::TextColored(Color(1.0f,0.0f,0.0f,1.0f), "Last    %llu", LastOffset);
            ImGui::TextColored  (Color(1.0f,1.0f,0.0f,1.0f), "Top     %llu", TopOffset);
            ImGui::Text         ("Count   %lu", Mem->Doc.Undo.Count);
            ImGui::Text         ("Index   %lu", Mem->Doc.Undo.CurrentIndex);

            ImGui::End();
        }
    }

    // Canvas zooming and panning
    {
        // Panning
        Mem->Doc.CanvasPosition += Math::RoundToVec2i(ImGui::GetMouseDragDelta(2));
        ImGui::ResetMouseDragDelta(2);

        // Zooming
        if (!ImGui::IsMouseDown(2) && ImGui::GetIO().MouseWheel)
        {
            float MinZoom      = 0.01f, MaxZoom = 32.0f;
            float ZoomSpeed    = 0.2f * Mem->Doc.CanvasZoom;
            float ScaleDelta   = Math::Min(MaxZoom - Mem->Doc.CanvasZoom, ImGui::GetIO().MouseWheel * ZoomSpeed);
            Vec2 OldCanvasZoom = Vec2((float)Mem->Doc.Width, (float)Mem->Doc.Height) * Mem->Doc.CanvasZoom;

            Mem->Doc.CanvasZoom += ScaleDelta;
            if (Mem->Doc.CanvasZoom < MinZoom) { Mem->Doc.CanvasZoom = MinZoom; } // TODO: Dynamically clamp min such that fully zoomed out image is 2x2 pixels?
            Vec2i NewCanvasSize = Math::RoundToVec2i(Vec2((float)Mem->Doc.Width, (float)Mem->Doc.Height) * Mem->Doc.CanvasZoom);

            if ((NewCanvasSize.x > Mem->Window.Width || NewCanvasSize.y > Mem->Window.Height))
            {
                Vec2 PreScaleMousePos = Vec2(Mem->Mouse.Pos - Mem->Doc.CanvasPosition) / OldCanvasZoom;
                Vec2 NewPos = Vec2(Mem->Doc.CanvasPosition) -
                    Vec2(PreScaleMousePos.x * ScaleDelta * (float)Mem->Doc.Width,
                        PreScaleMousePos.y * ScaleDelta * (float)Mem->Doc.Height);
                Mem->Doc.CanvasPosition = Math::RoundToVec2i(NewPos);
            }
            else // Center canvas
            {
                // TODO: Maybe disable centering on zoom out. Needs more usability testing.
                int32 TopMargin = 53; // TODO: Put common layout constants in struct
                int32 PosX = Math::RoundToInt((Mem->Window.Width - (float)Mem->Doc.Width * Mem->Doc.CanvasZoom) / 2.0f);
                int32 PosY = TopMargin + Math::RoundToInt((Mem->Window.Height - TopMargin - (float)Mem->Doc.Height * Mem->Doc.CanvasZoom) / 2.0f);
                Mem->Doc.CanvasPosition = Vec2i(PosX, PosY);
            }
        }
    }

    // Draw alpha grid
    {
        GL::TransformQuad(Mem->Meshes[PapayaMesh_AlphaGrid],
            Mem->Doc.CanvasPosition,
            Vec2(Mem->Doc.Width * Mem->Doc.CanvasZoom, Mem->Doc.Height * Mem->Doc.CanvasZoom));

        GL::DrawQuad(Mem->Meshes[PapayaMesh_AlphaGrid], Mem->Shaders[PapayaShader_AlphaGrid], true,
            6,
            UniformType_Matrix4, &Mem->Window.ProjMtx[0][0],
            UniformType_Color, Mem->Colors[PapayaCol_AlphaGrid1],
            UniformType_Color, Mem->Colors[PapayaCol_AlphaGrid2],
            UniformType_Float, Mem->Doc.CanvasZoom,
            UniformType_Float, Mem->Doc.InverseAspect,
            UniformType_Float, Math::Max((float)Mem->Doc.Width, (float)Mem->Doc.Height));
    }

    // Draw canvas
    {
        GL::TransformQuad(Mem->Meshes[PapayaMesh_Canvas],
            Mem->Doc.CanvasPosition,
            Vec2(Mem->Doc.Width * Mem->Doc.CanvasZoom, Mem->Doc.Height * Mem->Doc.CanvasZoom));

        GLCHK( glActiveTexture(GL_TEXTURE0) );

        if (Mem->Misc.DrawCanvas)
        {
            mat4x4 M;
            mat4x4_ortho(M, 0.f, (float)Mem->Window.Width,
                         (float)Mem->Window.Height, 0.f,
                         -1.f, 1.f);

            // Rotate around center
            {
                mat4x4 R;
                Vec2 Offset = Vec2(Mem->Doc.CanvasPosition.x + Mem->Doc.Width *
                                   Mem->Doc.CanvasZoom * 0.5f,
                                   Mem->Doc.CanvasPosition.y + Mem->Doc.Height *
                                   Mem->Doc.CanvasZoom * 0.5f);

                mat4x4_translate_in_place(M, Offset.x, Offset.y, 0.f);
                mat4x4_rotate_Z(R, M, Mem->CropRotate.SliderAngle + 
                        Mem->CropRotate.BaseAngle);
                mat4x4_translate_in_place(R, -Offset.x, -Offset.y, 0.f);
                mat4x4_dup(M, R);
            }

            GLCHK( glBindTexture(GL_TEXTURE_2D, Mem->Doc.TextureID) );
            GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
            GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST) );
            GL::DrawQuad(Mem->Meshes[PapayaMesh_Canvas], Mem->Shaders[PapayaShader_ImGui],
                true, 1,
                UniformType_Matrix4, M);
        }

        if (Mem->Misc.DrawOverlay)
        {
            GLCHK( glBindTexture  (GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->Misc.FboSampleTexture) );
            GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
            GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST) );
            GL::DrawQuad(Mem->Meshes[PapayaMesh_Canvas], Mem->Shaders[PapayaShader_ImGui], 1, true,
                UniformType_Matrix4, &Mem->Window.ProjMtx[0][0]);
        }
    }

    // Draw brush cursor
    {
        if (Mem->CurrentTool == PapayaTool_Brush &&
            (!ImGui::GetIO().KeyAlt || Mem->Mouse.IsDown[1]))
        {
            float ScaledDiameter = Mem->Brush.Diameter * Mem->Doc.CanvasZoom;

            GL::TransformQuad(Mem->Meshes[PapayaMesh_BrushCursor],
                (Mem->Mouse.IsDown[1] || Mem->Mouse.WasDown[1] ? Mem->Brush.RtDragStartPos : Mem->Mouse.Pos) - (Vec2(ScaledDiameter,ScaledDiameter) * 0.5f),
                Vec2(ScaledDiameter,ScaledDiameter));

            GL::DrawQuad(Mem->Meshes[PapayaMesh_BrushCursor], Mem->Shaders[PapayaShader_BrushCursor], true,
                4,
                UniformType_Matrix4, &Mem->Window.ProjMtx[0][0],
                UniformType_Color, Color(1.0f, 0.0f, 0.0f, Mem->Mouse.IsDown[1] ? Mem->Brush.Opacity : 0.0f),
                UniformType_Float, Mem->Brush.Hardness,
                UniformType_Float, ScaledDiameter);
        }
    }

    // Eye dropper
    {
        if ((Mem->CurrentTool == PapayaTool_EyeDropper || (Mem->CurrentTool == PapayaTool_Brush && ImGui::GetIO().KeyAlt))
            && Mem->Mouse.InWorkspace)
        {
            if (Mem->Mouse.IsDown[0])
            {
                // Get pixel color
                {
                    float Pixel[3] = { 0 };
                    GLCHK( glReadPixels((int32)Mem->Mouse.Pos.x, Mem->Window.Height - (int32)Mem->Mouse.Pos.y, 1, 1, GL_RGB, GL_FLOAT, Pixel) );
                    Mem->EyeDropper.CurrentColor = Color(Pixel[0], Pixel[1], Pixel[2]);
                }

                Vec2 Size = Vec2(230,230);
                GL::TransformQuad(Mem->Meshes[PapayaMesh_EyeDropperCursor],
                    Mem->Mouse.Pos - (Size * 0.5f),
                    Size);

                GL::DrawQuad(Mem->Meshes[PapayaMesh_EyeDropperCursor], Mem->Shaders[PapayaShader_EyeDropperCursor], true,
                    3,
                    UniformType_Matrix4, &Mem->Window.ProjMtx[0][0],
                    UniformType_Color, Mem->EyeDropper.CurrentColor,
                    UniformType_Color, Mem->Picker.NewColor);
            }
            else if (Mem->Mouse.Released[0])
            {
                Picker::SetColor(Mem->EyeDropper.CurrentColor, &Mem->Picker, Mem->Picker.Open);
            }
        }
    }

EndOfDoc:

    // Metrics window
    {
        if (Mem->Misc.ShowMetrics)
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
                    ImGui::Text("%llu", Mem->Debug.Timers[i].ElapsedCycles);    ImGui::NextColumn();
                    ImGui::Text("%f" , Mem->Debug.Timers[i].ElapsedMs);         ImGui::NextColumn();
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
                ImGui::Text("%d", Mem->Tablet.PosX);        ImGui::NextColumn();
                ImGui::Text("PosY");                        ImGui::NextColumn();
                ImGui::Text("%d", Mem->Tablet.PosY);        ImGui::NextColumn();
                ImGui::Text("Pressure");                    ImGui::NextColumn();
                ImGui::Text("%f", Mem->Tablet.Pressure);    ImGui::NextColumn();

                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::Text("Mouse");
                ImGui::Columns(2, "inputcolumns");
                ImGui::Separator();
                ImGui::Text("PosX");                        ImGui::NextColumn();
                ImGui::Text("%d", Mem->Mouse.Pos.x);        ImGui::NextColumn();
                ImGui::Text("PosY");                        ImGui::NextColumn();
                ImGui::Text("%d", Mem->Mouse.Pos.y);        ImGui::NextColumn();
                ImGui::Text("Buttons");                     ImGui::NextColumn();
                ImGui::Text("%d %d %d",
                    Mem->Mouse.IsDown[0],
                    Mem->Mouse.IsDown[1],
                    Mem->Mouse.IsDown[2]);                  ImGui::NextColumn();
                ImGui::Columns(1);
                ImGui::Separator();
            }
            ImGui::End();
        }
    }

    ImGui::Render(Mem);

    // Color Picker Panel
    if (Mem->Picker.Open)
    {
        // TODO: Investigate how to move this custom shaded quad drawing into
        //       ImGui to get correct draw order.

        // Draw hue picker
        GL::DrawQuad(Mem->Meshes[PapayaMesh_PickerHStrip], Mem->Shaders[PapayaShader_PickerHStrip], false,
                2,
                UniformType_Matrix4, &Mem->Window.ProjMtx[0][0],
                UniformType_Float, Mem->Picker.CursorH);

        // Draw saturation-value picker
        GL::DrawQuad(Mem->Meshes[PapayaMesh_PickerSVBox], Mem->Shaders[PapayaShader_PickerSVBox], false,
                3,
                UniformType_Matrix4, &Mem->Window.ProjMtx[0][0],
                UniformType_Float, Mem->Picker.CursorH,
                UniformType_Vec2, Mem->Picker.CursorSV);
    }

    // Last mouse info
    {
        Mem->Mouse.LastPos    = Math::RoundToVec2i(ImGui::GetMousePos());
        Mem->Mouse.LastUV     = Mem->Mouse.UV;
        Mem->Mouse.WasDown[0] = ImGui::IsMouseDown(0);
        Mem->Mouse.WasDown[1] = ImGui::IsMouseDown(1);
        Mem->Mouse.WasDown[2] = ImGui::IsMouseDown(2);
    }
}

void Core::RenderImGui(ImDrawData* DrawData, void* MemPtr)
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

    GLCHK( glUseProgram      (Mem->Shaders[PapayaShader_ImGui].Handle) );
    GLCHK( glUniform1i       (Mem->Shaders[PapayaShader_ImGui].Uniforms[1], 0) );
    GLCHK( glUniformMatrix4fv(Mem->Shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &Mem->Window.ProjMtx[0][0]) );

    for (int n = 0; n < DrawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = DrawData->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_ImGui].VboHandle) );
        GLCHK( glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW) );

        GLCHK( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_ImGui].ElementsHandle) );
        GLCHK( glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW) );

        GL::SetVertexAttribs(Mem->Shaders[PapayaShader_ImGui]);

        for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
        {
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                GLCHK( glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId) );
                GLCHK( glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y)) );
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
