#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace Papaya
{

internal uint32 AllocateEmptyTexture(int32 Width, int32 Height)
{
    uint32 Tex;
    glGenTextures  (1, &Tex);
    glBindTexture  (GL_TEXTURE_2D, Tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D   (GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    return Tex;
}

internal uint32 LoadAndBindImage(char* Path)
{
    uint8* Image;
    int32 ImageWidth, ImageHeight, ComponentsPerPixel;
    Image = stbi_load(Path, &ImageWidth, &ImageHeight, &ComponentsPerPixel, 0);

    // Create texture
    GLuint Id_GLuint;
    glGenTextures  (1, &Id_GLuint);
    glBindTexture  (GL_TEXTURE_2D, Id_GLuint);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D   (GL_TEXTURE_2D, 0, GL_RGBA8, ImageWidth, ImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, Image);

    // Store our identifier
    free(Image);
    return (uint32)Id_GLuint;
}

internal void InitMesh(MeshInfo& Mesh, ShaderInfo Shader, Vec2 Pos, Vec2 Size, GLenum Usage)
{
    glGenBuffers     (1, &Mesh.VboHandle);
    glBindBuffer     (GL_ARRAY_BUFFER, Mesh.VboHandle);
    glGenVertexArrays(1, &Mesh.VaoHandle);
    glBindVertexArray(Mesh.VaoHandle);

    glEnableVertexAttribArray(Shader.Attributes[0]); // Position attribute
    glEnableVertexAttribArray(Shader.Attributes[1]); // UV attribute
    glEnableVertexAttribArray(Shader.Attributes[2]); // Color attribute

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    glVertexAttribPointer(Shader.Attributes[0], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));   // Position attribute
    glVertexAttribPointer(Shader.Attributes[1], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));    // UV attribute
    glVertexAttribPointer(Shader.Attributes[2], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col)); // Color attribute
#undef OFFSETOF

    ImDrawVert Verts[6];
    Verts[0].pos = Vec2(Pos.x, Pos.y);                   Verts[0].uv = Vec2(0.0f, 1.0f); Verts[0].col = 0xffffffff;
    Verts[1].pos = Vec2(Size.x + Pos.x, Pos.y);          Verts[1].uv = Vec2(1.0f, 1.0f); Verts[1].col = 0xffffffff;
    Verts[2].pos = Vec2(Size.x + Pos.x, Size.y + Pos.y); Verts[2].uv = Vec2(1.0f, 0.0f); Verts[2].col = 0xffffffff;
    Verts[3].pos = Vec2(Pos.x, Pos.y);                   Verts[3].uv = Vec2(0.0f, 1.0f); Verts[3].col = 0xffffffff;
    Verts[4].pos = Vec2(Size.x + Pos.x, Size.y + Pos.y); Verts[4].uv = Vec2(1.0f, 0.0f); Verts[4].col = 0xffffffff;
    Verts[5].pos = Vec2(Pos.x, Size.y + Pos.y);          Verts[5].uv = Vec2(0.0f, 0.0f); Verts[5].col = 0xffffffff;

    glBufferData(GL_ARRAY_BUFFER, sizeof(Verts), Verts, Usage);
}

internal void PushUndo(PapayaMemory* Mem)
{
    if (Mem->Doc.Undo.Top == 0) // Buffer is empty
    {
        Mem->Doc.Undo.Base = (UndoData*)Mem->Doc.Undo.Start;
        Mem->Doc.Undo.Top  = Mem->Doc.Undo.Start;
    }
    else if (Mem->Doc.Undo.Current->Next != 0) // Not empty and not at end. Reposition for overwrite.
    {
        uint64 BytesToRight = (int8*)Mem->Doc.Undo.Start + Mem->Doc.Undo.Size - (int8*)Mem->Doc.Undo.Current;
        uint64 BlockSize = sizeof(UndoData) + Mem->Doc.Undo.Current->Size;
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
    Data.Size      = 4 * Mem->Doc.Width * Mem->Doc.Height;
    uint64 BufSize = sizeof(UndoData) + Data.Size;
    void* Buf      = malloc((size_t)BufSize);

    memcpy(Buf, &Data, sizeof(UndoData));
    glFinish();
    glBindTexture(GL_TEXTURE_2D, Mem->Doc.TextureID);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (int8*)Buf + sizeof(UndoData));
    glFinish();

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

internal bool OpenDocument(char* Path, PapayaMemory* Mem)
{
    // Load image
    {
        uint8* Texture = stbi_load(Path, &Mem->Doc.Width, &Mem->Doc.Height, &Mem->Doc.ComponentsPerPixel, 4);

        if (!Texture) { return false; }

        // Create texture
        glGenTextures  (1, &Mem->Doc.TextureID);
        glBindTexture  (GL_TEXTURE_2D, Mem->Doc.TextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D   (GL_TEXTURE_2D, 0, GL_RGBA8, Mem->Doc.Width, Mem->Doc.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Texture);

        Mem->Doc.InverseAspect = (float)Mem->Doc.Height / (float)Mem->Doc.Width;
        Mem->Doc.CanvasZoom = 0.8f * Math::Min((float)Mem->Window.Width/(float)Mem->Doc.Width, (float)Mem->Window.Height/(float)Mem->Doc.Height);
        if (Mem->Doc.CanvasZoom > 1.0f) { Mem->Doc.CanvasZoom = 1.0f; }
        Mem->Doc.CanvasPosition = Vec2((Mem->Window.Width  - (float)Mem->Doc.Width  * Mem->Doc.CanvasZoom)/2.0f,
                                               (Mem->Window.Height - (float)Mem->Doc.Height * Mem->Doc.CanvasZoom)/2.0f); // TODO: Center with respect to canvas, not window
        free(Texture);
    }

    // Set up the frame buffer
    {
        // Create a framebuffer object and bind it
        glGenFramebuffers(1, &Mem->Misc.FrameBufferObject);
        glBindFramebuffer(GL_FRAMEBUFFER, Mem->Misc.FrameBufferObject);

        Mem->Misc.FboRenderTexture = AllocateEmptyTexture(Mem->Doc.Width, Mem->Doc.Height);
        Mem->Misc.FboSampleTexture = AllocateEmptyTexture(Mem->Doc.Width, Mem->Doc.Height);

        // Attach the color texture to the FBO
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Mem->Misc.FboRenderTexture, 0);

        static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, draw_buffers);

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            // TODO: Log: Frame buffer not initialized correctly
            exit(1);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Set up meshes for rendering to texture
    {
        Vec2 Size = Vec2((float)Mem->Doc.Width, (float)Mem->Doc.Height);

        InitMesh(Mem->Meshes[PapayaMesh_RTTBrush],
            Mem->Shaders[PapayaShader_Brush],
            Vec2(0,0), Size, GL_STATIC_DRAW);

        InitMesh(Mem->Meshes[PapayaMesh_RTTAdd],
            Mem->Shaders[PapayaShader_ImGui],
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
        uint64 MaxSize        = 1024 * 1024 * 1024; // Maximum number of bytes to allocate as an undo buffer
        uint64 UndoRecordSize = sizeof(UndoData) + 4 * Mem->Doc.Width * Mem->Doc.Height;
        Mem->Doc.Undo.Size    = Math::Min(100 * UndoRecordSize, MaxSize);
        if (Mem->Doc.Undo.Size < 2 * UndoRecordSize) { Mem->Doc.Undo.Size = 2 * UndoRecordSize; }

        Mem->Doc.Undo.Start = malloc((size_t)Mem->Doc.Undo.Size);
        Mem->Doc.Undo.CurrentIndex = -1;

        PushUndo(Mem);
    }

    return true;
}

internal void CloseDocument(PapayaMemory* Mem)
{
    // Document
    if (Mem->Doc.TextureID)
    {
        glDeleteTextures(1, &Mem->Doc.TextureID);
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
        glDeleteFramebuffers(1, &Mem->Misc.FrameBufferObject);
        Mem->Misc.FrameBufferObject = 0;
    }

    if (Mem->Misc.FboRenderTexture)
    {
        glDeleteTextures(1, &Mem->Misc.FboRenderTexture);
        Mem->Misc.FboRenderTexture = 0;
    }

    if (Mem->Misc.FboSampleTexture)
    {
        glDeleteTextures(1, &Mem->Misc.FboSampleTexture);
        Mem->Misc.FboSampleTexture = 0;
    }

    // Vertex Buffer: RTTBrush
    if (Mem->Meshes[PapayaMesh_RTTBrush].VboHandle)
    {
        glDeleteBuffers(1, &Mem->Meshes[PapayaMesh_RTTBrush].VboHandle);
        Mem->Meshes[PapayaMesh_RTTBrush].VboHandle = 0;
    }

    if (Mem->Meshes[PapayaMesh_RTTBrush].VaoHandle)
    {
        glDeleteVertexArrays(1, &Mem->Meshes[PapayaMesh_RTTBrush].VaoHandle);
        Mem->Meshes[PapayaMesh_RTTBrush].VaoHandle = 0;
    }

    // Vertex Buffer: RTTAdd
    if (Mem->Meshes[PapayaMesh_RTTAdd].VboHandle)
    {
        glDeleteBuffers(1, &Mem->Meshes[PapayaMesh_RTTAdd].VboHandle);
        Mem->Meshes[PapayaMesh_RTTAdd].VboHandle = 0;
    }

    if (Mem->Meshes[PapayaMesh_RTTAdd].VaoHandle)
    {
        glDeleteVertexArrays(1, &Mem->Meshes[PapayaMesh_RTTAdd].VaoHandle);
        Mem->Meshes[PapayaMesh_RTTAdd].VaoHandle = 0;
    }
}

internal void CompileShader(ShaderInfo& Shader, const char* Vert, const char* Frag, int32 AttribCount, int32 UniformCount, ...)
{
    Shader.Handle     = glCreateProgram();
    uint32 VertHandle = glCreateShader(GL_VERTEX_SHADER);
    uint32 FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource (VertHandle, 1, &Vert, 0);
    glShaderSource (FragHandle, 1, &Frag, 0);
    glCompileShader(VertHandle);
    glCompileShader(FragHandle);
    glAttachShader (Shader.Handle, VertHandle);
    glAttachShader (Shader.Handle, FragHandle);

    Util::PrintGlShaderCompilationStatus(VertHandle); // TODO: Print name of compiled shader
    Util::PrintGlShaderCompilationStatus(FragHandle);
    glLinkProgram(Shader.Handle); // TODO: Print linking errors

    va_list Args;
    va_start(Args, UniformCount);
    for (int32 i = 0; i < AttribCount; i++)
    {
        Shader.Attributes[i] = glGetAttribLocation(Shader.Handle, va_arg(Args, const char*));
    }

    for (int32 i = 0; i < UniformCount; i++)
    {
        Shader.Uniforms[i] = glGetUniformLocation(Shader.Handle, va_arg(Args, const char*));
    }
    va_end(Args);
}

void Initialize(PapayaMemory* Mem)
{
    // Init values
    {
        Mem->Brush.Diameter    = 50;
        Mem->Brush.MaxDiameter = 9999;
        Mem->Brush.Hardness    = 90.0f;
        Mem->Brush.Opacity     = 100.0f;

        Mem->Picker.CurrentColor = Color(220, 163, 89);
        Mem->Picker.Open         = false;
        Mem->Picker.Pos          = Vec2(34, 86);
        Mem->Picker.Size         = Vec2(292, 331);
        Mem->Picker.HueStripPos  = Vec2(259, 42);
        Mem->Picker.HueStripSize = Vec2(30, 256);
        Mem->Picker.SVBoxPos     = Vec2(0, 42);
        Mem->Picker.SVBoxSize    = Vec2(256, 256);
        Mem->Picker.CursorSV     = Vec2(0.5f, 0.5f);

        Mem->Misc.DrawCanvas  = true;
        Mem->Misc.DrawOverlay = false;

        float OrthoMtx[4][4] =
        {
            { 2.0f,   0.0f,   0.0f,   0.0f },
            { 0.0f,  -2.0f,   0.0f,   0.0f },
            { 0.0f,   0.0f,  -1.0f,   0.0f },
            { -1.0f,  1.0f,   0.0f,   1.0f },
        };
        memcpy(Mem->Window.ProjMtx, OrthoMtx, sizeof(OrthoMtx));
    }

    const char* Vert;
    // Vertex shader
    {
        Vert = 
        "   #version 330                                        \n"
        "   uniform mat4 ProjMtx;                               \n" // Uniforms[0]
        "                                                       \n"
        "   in vec2 Position;                                   \n" // Attributes[0]
        "   in vec2 UV;                                         \n" // Attributes[1]
        "   in vec4 Color;                                      \n" // Attributes[2]
        "                                                       \n"
        "   out vec2 Frag_UV;                                   \n"
        "   out vec4 Frag_Color;                                \n"
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
"   #version 330                                                    \n"
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
"   in vec2 Frag_UV;                                                \n"
"   out vec4 Out_Color;                                             \n"
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
"       vec4 t = texture(Texture, Frag_UV.st);                      \n"
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
"       Out_Color = vec4(BrushColor.r, BrushColor.g, BrushColor.b,  \n"
"                        clamp(FinalAlpha,0.0,1.0));                \n" // TODO: Needs improvement. Self-intersection corners look weird.
"   }                                                               \n";

    CompileShader(Mem->Shaders[PapayaShader_Brush], Vert, Frag, 3, 8,
        "Position", "UV", "Color",
        "ProjMtx", "Texture", "Pos", "LastPos", "Radius", "BrushColor", "Hardness", "InvAspect");

    }

    // Brush cursor shader
    {
    const char* Frag =
"   #version 330                                                                \n"
"                                                                               \n"
"   #define M_PI 3.1415926535897932384626433832795                              \n"
"                                                                               \n"
"   uniform vec4 BrushColor;                                                    \n" // Uniforms[1]
"   uniform float Hardness;                                                     \n" // Uniforms[2]
"   uniform float PixelDiameter;                                                \n" // Uniforms[3]
"                                                                               \n"
"   in vec2 Frag_UV;                                                            \n"
"                                                                               \n"
"   out vec4 Out_Color;                                                         \n"
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
"       Out_Color = (Dist > 0.5 - BorderThickness && Dist < 0.5) ?              \n"
"       vec4(0.0,0.0,0.0,1.0) :                                                 \n"
"       vec4(BrushColor.r, BrushColor.g, BrushColor.b, 	Alpha * BrushColor.a);  \n"
"   }                                                                           \n";

    CompileShader(Mem->Shaders[PapayaShader_BrushCursor], Vert, Frag, 3, 4,
        "Position", "UV", "Color",
        "ProjMtx", "BrushColor", "Hardness", "PixelDiameter");

    InitMesh(Mem->Meshes[PapayaMesh_BrushCursor],
        Mem->Shaders[PapayaShader_BrushCursor],
        Vec2(40, 60), Vec2(30, 30), GL_DYNAMIC_DRAW);
    }

    // Picker saturation-value shader
    {
    const char* Frag =
"   #version 330                                                            \n"
"                                                                           \n"
"   uniform float Hue;                                                      \n" // Uniforms[1]
"   uniform vec2 Cursor;                                                    \n" // Uniforms[2]
"   uniform float Thickness = 1.0 / 256.0;                                  \n"
"   uniform float Radius = 0.0075;                                          \n"
"                                                                           \n"
"   in  vec2 Frag_UV;                                                       \n"
"   out vec4 Out_Color;                                                     \n"
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
"           Out_Color = vec4(a, a, a, 1.0);                                 \n"
"       }                                                                   \n"
"       else                                                                \n"
"       {                                                                   \n"
"           Out_Color = vec4(RGB.x, RGB.y, RGB.z, 1.0);                     \n"
"       }                                                                   \n"
"   }                                                                       \n";

    CompileShader(Mem->Shaders[PapayaShader_PickerSVBox], Vert, Frag, 3, 3,
        "Position", "UV", "Color",
        "ProjMtx", "Hue", "Cursor");

    InitMesh(Mem->Meshes[PapayaMesh_PickerSVBox],
        Mem->Shaders[PapayaShader_PickerSVBox],
        Mem->Picker.Pos + Mem->Picker.SVBoxPos, Mem->Picker.SVBoxSize, GL_STATIC_DRAW);
    }

    // Picker hue shader
    {
    const char* Frag =
"   #version 330                                                    \n"
"                                                                   \n"
"   uniform float Cursor;                                           \n" // Uniforms[1]
"                                                                   \n"
"   in  vec2 Frag_UV;                                               \n"
"   out vec4 Out_Color;                                             \n"
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
"           Out_Color = vec4(0.36,0.36,0.37,                        \n"
"                       float(abs(Frag_UV.y - Cursor) < 0.0039));   \n"
"       }                                                           \n"
"       else                                                        \n"
"           Out_Color = Hue;                                        \n"
"   }                                                               \n";

    CompileShader(Mem->Shaders[PapayaShader_PickerHStrip], Vert, Frag, 3, 2,
        "Position", "UV", "Color",
        "ProjMtx", "Cursor");

    InitMesh(Mem->Meshes[PapayaMesh_PickerHStrip],
        Mem->Shaders[PapayaShader_PickerHStrip],
        Mem->Picker.Pos + Mem->Picker.HueStripPos, Mem->Picker.HueStripSize, GL_STATIC_DRAW);
    }

    // ImGui default shader
    {
    const char* Frag =
"   #version 330                                                \n"
"   uniform sampler2D Texture;                                  \n" // Uniforms[1]
"                                                               \n"
"   in vec2 Frag_UV;                                            \n"
"   in vec4 Frag_Color;                                         \n"
"                                                               \n"
"   out vec4 Out_Color;                                         \n"
"   void main()                                                 \n"
"   {                                                           \n"
"       Out_Color = Frag_Color * texture( Texture, Frag_UV.st); \n"
"   }                                                           \n";

    CompileShader(Mem->Shaders[PapayaShader_ImGui], Vert, Frag, 3, 2,
        "Position", "UV", "Color",
        "ProjMtx", "Texture");

    InitMesh(Mem->Meshes[PapayaMesh_Canvas],
        Mem->Shaders[PapayaShader_ImGui],
        Vec2(0,0), Vec2(10,10), GL_DYNAMIC_DRAW);
    }

    // Setup for ImGui
    {
        glGenBuffers(1, &Mem->Meshes[PapayaMesh_ImGui].VboHandle);
        glGenBuffers(1, &Mem->Meshes[PapayaMesh_ImGui].ElementsHandle);

        glGenVertexArrays        (1, &Mem->Meshes[PapayaMesh_ImGui].VaoHandle);
        glBindVertexArray        (Mem->Meshes[PapayaMesh_ImGui].VaoHandle);
        glBindBuffer             (GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_ImGui].VboHandle);
        glEnableVertexAttribArray(Mem->Shaders[PapayaShader_ImGui].Attributes[0]);
        glEnableVertexAttribArray(Mem->Shaders[PapayaShader_ImGui].Attributes[1]);
        glEnableVertexAttribArray(Mem->Shaders[PapayaShader_ImGui].Attributes[2]);

    #define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
        glVertexAttribPointer(Mem->Shaders[PapayaShader_ImGui].Attributes[0], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
        glVertexAttribPointer(Mem->Shaders[PapayaShader_ImGui].Attributes[1], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
        glVertexAttribPointer(Mem->Shaders[PapayaShader_ImGui].Attributes[2], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
    #undef OFFSETOF
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Create fonts texture
        ImGuiIO& io = ImGui::GetIO();

        unsigned char* pixels;
        int width, height;
        //ImFont* my_font0 = io.Fonts->AddFontFromFileTTF("d:\\DroidSans.ttf", 15.0f);
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        glGenTextures  (1, &Mem->Textures[PapayaTex_Font]);
        glBindTexture  (GL_TEXTURE_2D, Mem->Textures[PapayaTex_Font]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D   (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        // Store our identifier
        io.Fonts->TexID = (void *)(intptr_t)Mem->Textures[PapayaTex_Font];

        // Cleanup
        io.Fonts->ClearInputData();
        io.Fonts->ClearTexData();
    }

    // Load interface textures and colors
    {
        // Texture IDs
        Mem->Textures[PapayaTex_TitleBarButtons] = LoadAndBindImage("../../img/win32_titlebar_buttons.png");
        Mem->Textures[PapayaTex_TitleBarIcon]    = LoadAndBindImage("../../img/win32_titlebar_icon.png");
        Mem->Textures[PapayaTex_InterfaceIcons]  = LoadAndBindImage("../../img/interface_icons.png");

        // Colors
#if 1
        Mem->Colors[PapayaCol_Clear]         = Color(45,45,48); // Dark grey
#else
        Mem->Colors[PapayaCol_Clear]         = Color(114,144,154); // Light blue
#endif
        Mem->Colors[PapayaCol_Workspace]     = Color(30,30,30); // Light blue
        Mem->Colors[PapayaCol_Transparent]   = Color(0,0,0,0);
        Mem->Colors[PapayaCol_Button]        = Color(92,92,94);
        Mem->Colors[PapayaCol_ButtonHover]   = Color(64,64,64);
        Mem->Colors[PapayaCol_ButtonActive]  = Color(0,122,204);
    }

    // ImGui Style Settings
    {
        ImGuiStyle& Style = ImGui::GetStyle();
        Style.WindowFillAlphaDefault = 1.0f;
        // TODO: Move repeated stuff here by setting global style
    }

#ifdef PAPAYA_DEFAULT_IMAGE
    OpenDocument(PAPAYA_DEFAULT_IMAGE, Mem);
#endif
}

void Shutdown(PapayaMemory* Mem)
{
    //TODO: Free stuff
}

void UpdateAndRender(PapayaMemory* Mem, PapayaDebugMemory* DebugMem)
{
    // Initialize frame
    {
        // Current mouse info
        {
            Mem->Mouse.Pos = ImGui::GetMousePos();
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
            glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
            glClearBufferfv(GL_COLOR, 0, (GLfloat*)&Mem->Colors[PapayaCol_Clear]);

            glEnable(GL_SCISSOR_TEST);
            glScissor(34, 3,
                     (int)Mem->Window.Width  - 37, (int)Mem->Window.Height - 58); // TODO: Remove magic numbers

            glClearBufferfv(GL_COLOR, 0, (float*)&Mem->Colors[PapayaCol_Workspace]);
            glDisable(GL_SCISSOR_TEST);
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

        bool bTrue = true;
        ImGui::Begin("Title Bar Menu", &bTrue, WindowFlags);
        if (ImGui::BeginMenuBar())
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, Mem->Colors[PapayaCol_Clear]);
            if (ImGui::BeginMenu("FILE"))
            {
                if (ImGui::MenuItem("Open"))
                {
                    char* Path = Platform::OpenFileDialog();
                    if (Path)
                    {
                        CloseDocument(Mem);
                        OpenDocument(Path, Mem);
                        free(Path);
                    }
                }

                if (ImGui::MenuItem("Close"))
                {
                    CloseDocument(Mem);
                }

                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    char* Path = Platform::SaveFileDialog();
                    uint8* Texture = (uint8*)malloc(4 * Mem->Doc.Width * Mem->Doc.Height);
                    if (Path) // TODO: Do this on a separate thread. Massively blocks UI for large images.
                    {
                        glFinish();
                        glBindTexture(GL_TEXTURE_2D, Mem->Doc.TextureID);
                        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, Texture);
                        glFinish();

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
                if (ImGui::MenuItem("Save As..")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Quit", "Alt+F4")) { Mem->IsRunning = false; }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("VIEW"))
            {
                ImGui::MenuItem("Metrics Window", NULL, &Mem->Misc.ShowMetricsWindow);
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
        ImGui::SetNextWindowPos (ImVec2( 1, 55));

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

        ImGui::PushStyleColor(ImGuiCol_Button        , Mem->Colors[PapayaCol_Transparent]);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered , Mem->Colors[PapayaCol_ButtonHover]);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive  , Mem->Colors[PapayaCol_ButtonActive]);
        ImGui::PushStyleColor(ImGuiCol_WindowBg      , Mem->Colors[PapayaCol_Transparent]);

        bool Show = true;
        ImGui::Begin("Left toolbar", &Show, WindowFlags);

        #define CALCUV(X, Y) ImVec2((float)X*20.0f/256.0f, (float)Y*20.0f/256.0f)
        {
            ImGui::PushID(0);
            if (ImGui::ImageButton((void*)(intptr_t)Mem->Textures[PapayaTex_InterfaceIcons], ImVec2(20, 20), CALCUV(0, 0), CALCUV(1, 1), 6, ImVec4(0, 0, 0, 0)))
            {

            }
            ImGui::PopID();

            ImGui::PushID(1);
            if (ImGui::ImageButton((void*)(intptr_t)Mem->Textures[PapayaTex_InterfaceIcons], ImVec2(33, 33), CALCUV(0, 0), CALCUV(0, 0), 0, Mem->Picker.CurrentColor))
            {
                Mem->Picker.Open = !Mem->Picker.Open;
                if (Mem->Picker.Open)
                {
                    Mem->Picker.NewColor = Mem->Picker.CurrentColor;
                    Math::RGBtoHSV(Mem->Picker.NewColor.r, Mem->Picker.NewColor.g, Mem->Picker.NewColor.b,
                                   Mem->Picker.CursorH, Mem->Picker.CursorSV.x, Mem->Picker.CursorSV.y);
                }
            }
            ImGui::PopID();
        }
        #undef CALCUV

        ImGui::End();

        ImGui::PopStyleVar(5);
        ImGui::PopStyleColor(4);
    }

    // Color Picker
    {
        if (Mem->Picker.Open)
        {
            // TODO: Clean styling code
            ImGui::SetNextWindowSize(ImVec2(Mem->Picker.Size.x, 35));
            ImGui::SetNextWindowPos (Mem->Picker.Pos);

            ImGuiWindowFlags WindowFlags = 0;
            WindowFlags |= ImGuiWindowFlags_NoTitleBar;
            WindowFlags |= ImGuiWindowFlags_NoResize;
            WindowFlags |= ImGuiWindowFlags_NoMove;
            WindowFlags |= ImGuiWindowFlags_NoScrollbar;
            WindowFlags |= ImGuiWindowFlags_NoCollapse;
            WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding  , ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding , 0);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing    , ImVec2(0, 0));

            ImGui::PushStyleColor(ImGuiCol_Button        , Mem->Colors[PapayaCol_Button]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered , Mem->Colors[PapayaCol_ButtonHover]);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive  , Mem->Colors[PapayaCol_ButtonActive]);
            ImGui::PushStyleColor(ImGuiCol_WindowBg      , Mem->Colors[PapayaCol_Transparent]);

            ImGui::Begin("Color preview", 0, WindowFlags);
            {
                float Width1 = (Mem->Picker.Size.x + 33.0f) / 2.0f;
                float Width2 = Width1 - 33.0f;
                if (ImGui::ImageButton((void*)(intptr_t)Mem->Textures[PapayaTex_InterfaceIcons], ImVec2(Width2, 34), ImVec2(0, 0), ImVec2(0, 0), 0, Mem->Picker.CurrentColor))
                {
                    Mem->Picker.Open = false;
                }
                ImGui::SameLine();
                ImGui::PushID(1);
                if (ImGui::ImageButton((void*)(intptr_t)Mem->Textures[PapayaTex_InterfaceIcons], ImVec2(Width1, 34), ImVec2(0, 0), ImVec2(0, 0), 0, Mem->Picker.NewColor))
                {
                    Mem->Picker.Open = false;
                    Mem->Picker.CurrentColor = Mem->Picker.NewColor;
                }
                ImGui::PopID();
            }
            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(3);

            ImGui::PushStyleVar  (ImGuiStyleVar_WindowPadding, ImVec2(0, 8));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, Mem->Colors[PapayaCol_Clear]);

            ImGui::SetNextWindowSize(Mem->Picker.Size - Vec2(0.0f, 34.0f));
            ImGui::SetNextWindowPos (Mem->Picker.Pos  + Vec2(0.0f, 34.0f));
            ImGui::PushStyleVar     (ImGuiStyleVar_WindowRounding, 0);
            ImGui::Begin("Color picker", 0, WindowFlags);
            {
                ImGui::BeginChild("HSV Gradients", Vec2(315, 258));
                ImGui::EndChild();
                int32 col[3];
                col[0] = (int)(Mem->Picker.NewColor.r * 255.0f);
                col[1] = (int)(Mem->Picker.NewColor.g * 255.0f);
                col[2] = (int)(Mem->Picker.NewColor.b * 255.0f);
                if (ImGui::InputInt3("RGB", col))
                {
                    Mem->Picker.NewColor = Color(col[0], col[1], col[2]); // TODO: Clamping
                    Math::RGBtoHSV(Mem->Picker.NewColor.r, Mem->Picker.NewColor.g, Mem->Picker.NewColor.b,
                                   Mem->Picker.CursorH, Mem->Picker.CursorSV.x, Mem->Picker.CursorSV.y);
                }
            }
            ImGui::End();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(4);
        }
    }

    if (!Mem->Doc.TextureID) { goto EndOfDoc; }

    // Tool Param Bar
    {
        ImGui::SetNextWindowSize(ImVec2((float)Mem->Window.Width - 37, 30));
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
        ImGui::PushItemWidth(85);

        ImGui::InputInt("Diameter", &Mem->Brush.Diameter);
        Mem->Brush.Diameter = Math::Clamp(Mem->Brush.Diameter, 1, Mem->Brush.MaxDiameter);

        ImGui::PopItemWidth();
        ImGui::PushItemWidth(80);
        ImGui::SameLine();
        ImGui::SliderFloat("Hardness", &Mem->Brush.Hardness, 0.0f, 100.0f, "%.0f");
        ImGui::SameLine();
        ImGui::SliderFloat("Opacity", &Mem->Brush.Opacity, 0.0f, 100.0f, "%.0f");

        ImGui::PopItemWidth();
        ImGui::End();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(5);
    }

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
                    float Opacity = Mem->Brush.RtDragStartOpacity + (ImGui::GetMouseDragDelta(1).x * 0.25f);
                    Mem->Brush.Opacity = Math::Clamp(Opacity, 0.0f, 100.0f);
                }
                else
                {
                    float Diameter = Mem->Brush.RtDragStartDiameter + (ImGui::GetMouseDragDelta(1).x / Mem->Doc.CanvasZoom * 2.0f);
                    Mem->Brush.Diameter = Math::Clamp((int32)Diameter, 1, Mem->Brush.MaxDiameter);

                    float Hardness = Mem->Brush.RtDragStartHardness + (ImGui::GetMouseDragDelta(1).y * 0.25f);
                    Mem->Brush.Hardness = Math::Clamp(Hardness, 0.0f, 100.0f);
                }
            }
            else if (Mem->Mouse.Released[1])
            {
                Platform::ReleaseMouseCapture();
                Platform::SetMousePosition(Mem->Brush.RtDragStartPos);
                Platform::SetCursorVisibility(true);
            }
        }

        if (Mem->Mouse.Pressed[0] && Mem->Mouse.InWorkspace)
        {
            Mem->Brush.BeingDragged = true;
            if (Mem->Picker.Open)
            {
                Mem->Picker.CurrentColor = Mem->Picker.NewColor;
            }
        }
        else if (Mem->Mouse.Released[0] && Mem->Brush.BeingDragged)
        {
            // Additive render-to-texture
            {
                glDisable(GL_SCISSOR_TEST);
                glBindFramebuffer   (GL_FRAMEBUFFER, Mem->Misc.FrameBufferObject);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Mem->Misc.FboRenderTexture, 0);

                glViewport(0, 0, Mem->Doc.Width, Mem->Doc.Height);
                glUseProgram(Mem->Shaders[PapayaShader_ImGui].Handle);

                glUniformMatrix4fv(Mem->Shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &Mem->Doc.ProjMtx[0][0]);
                //glUniform1i(Mem->Shaders[PapayaShader_ImGui].Uniforms[1], 0); // Texture uniform

                glBindBuffer(GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_RTTAdd].VboHandle);
                glBindVertexArray(Mem->Meshes[PapayaMesh_RTTAdd].VaoHandle);

                glEnable(GL_BLEND);
                glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX); // TODO: Handle the case where the original texture has an alpha below 1.0
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->Doc.TextureID);
                glDrawArrays (GL_TRIANGLES, 0, 6);
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->Misc.FboSampleTexture);
                glDrawArrays (GL_TRIANGLES, 0, 6);

                uint32 Temp = Mem->Misc.FboRenderTexture;
                Mem->Misc.FboRenderTexture = Mem->Doc.TextureID;
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Mem->Misc.FboRenderTexture, 0);
                Mem->Doc.TextureID = Temp;

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);

                glDisable(GL_BLEND);
            }

            PushUndo(Mem);

            Mem->Misc.DrawOverlay   = false;
            Mem->Brush.BeingDragged = false;
        }

        if (Mem->Brush.BeingDragged)
        {
            Mem->Misc.DrawOverlay = true;

            glBindFramebuffer(GL_FRAMEBUFFER, Mem->Misc.FrameBufferObject);

            if (Mem->Mouse.Pressed[0])
            {
                GLuint clearColor[4] = {0, 0, 0, 0};
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Mem->Misc.FboSampleTexture, 0);
                glClearBufferuiv(GL_COLOR, 0, clearColor);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Mem->Misc.FboRenderTexture, 0);
            }
            glViewport(0, 0, Mem->Doc.Width, Mem->Doc.Height);

            glDisable(GL_BLEND);
            glDisable(GL_SCISSOR_TEST);

            // Setup orthographic projection matrix
            float width  = (float)Mem->Doc.Width;
            float height = (float)Mem->Doc.Height;
            glUseProgram(Mem->Shaders[PapayaShader_Brush].Handle);

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

            glUniformMatrix4fv(Mem->Shaders[PapayaShader_Brush].Uniforms[0], 1, GL_FALSE, &Mem->Doc.ProjMtx[0][0]);
            //glUniform1i(Mem->Shaders[PapayaShader_Brush].Uniforms[1], Mem->Doc.TextureID); // Texture uniform
            glUniform2f(Mem->Shaders[PapayaShader_Brush].Uniforms[2], CorrectedPos.x, CorrectedPos.y * Mem->Doc.InverseAspect); // Pos uniform
            glUniform2f(Mem->Shaders[PapayaShader_Brush].Uniforms[3], CorrectedLastPos.x, CorrectedLastPos.y * Mem->Doc.InverseAspect); // Lastpos uniform
            glUniform1f(Mem->Shaders[PapayaShader_Brush].Uniforms[4], (float)Mem->Brush.Diameter / ((float)Mem->Doc.Width * 2.0f));
            float Opacity = Mem->Brush.Opacity / 100.0f; //(Math::Distance(CorrectedLastPos, CorrectedPos) > 0.0 ? Mem->Brush.Opacity / 100.0f : 0.0f);
            if (Mem->Tablet.Pressure > 0.0f) { Opacity *= Mem->Tablet.Pressure; }
            glUniform4f(Mem->Shaders[PapayaShader_Brush].Uniforms[5], Mem->Picker.CurrentColor.r,
                                                                      Mem->Picker.CurrentColor.g,
                                                                      Mem->Picker.CurrentColor.b,
                                                                      Opacity);
            glUniform1f(Mem->Shaders[PapayaShader_Brush].Uniforms[6], Mem->Brush.Hardness / 100.0f);
            glUniform1f(Mem->Shaders[PapayaShader_Brush].Uniforms[7], Mem->Doc.InverseAspect); // Inverse Aspect uniform

            glBindBuffer(GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_RTTBrush].VboHandle);
            glBindVertexArray(Mem->Meshes[PapayaMesh_RTTBrush].VaoHandle);

            glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->Misc.FboSampleTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            uint32 Temp = Mem->Misc.FboRenderTexture;
            Mem->Misc.FboRenderTexture = Mem->Misc.FboSampleTexture;
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Mem->Misc.FboRenderTexture, 0);
            Mem->Misc.FboSampleTexture = Temp;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        }

#if 0
        // =========================================================================================
        // Visualization: Brush falloff

        const int32 ArraySize = 256;
        local_persist float Opacities[ArraySize] = { 0 };

        float MaxScale = 90.0f;
        float t        = Mem->Brush.Hardness / 100.0f;
        float Scale    = 1.0f / (1.0f - t);
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
                Mem->Doc.Undo.Current = Mem->Doc.Undo.Current->Prev;
                Mem->Doc.Undo.CurrentIndex--;
                Refresh = true;
            }

            if (Refresh)
            {
                UndoData Data  = {};
                void* Image    = 0;
                bool AllocUsed = false;

                memcpy(&Data, Mem->Doc.Undo.Current, sizeof(UndoData));

                int64 BytesToRight = (int8*)Mem->Doc.Undo.Start + Mem->Doc.Undo.Size - (int8*)Mem->Doc.Undo.Current;
                if (BytesToRight - sizeof(UndoData) >= Data.Size) // Image is contiguously stored
                {
                    Image = (int8*)Mem->Doc.Undo.Current + sizeof(UndoData);
                }
                else // Image is split
                {
                    AllocUsed = true;
                    Image = malloc((size_t)Data.Size);
                    memcpy(Image, (int8*)Mem->Doc.Undo.Current + sizeof(UndoData), (size_t)BytesToRight - sizeof(UndoData));
                    memcpy((int8*)Image + BytesToRight - sizeof(UndoData), Mem->Doc.Undo.Start, (size_t)(Data.Size - (BytesToRight - sizeof(UndoData))));
                }

                glBindTexture(GL_TEXTURE_2D, Mem->Doc.TextureID);
                glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, Mem->Doc.Width, Mem->Doc.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Image);

                if (AllocUsed) { free(Image); }
            }
        }

#if 0
        // =========================================================================================
        // Visualization: Undo buffer

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
        ImGui::TextColored  (Color(0.0f,1.0f,1.0f,1.0f), "Base    %lu", BaseOffset);
        ImGui::TextColored  (Color(1.0f,0.0f,1.0f,1.0f), "Current %lu", CurrOffset);
        //ImGui::TextColored(Color(1.0f,0.0f,0.0f,1.0f), "Last    %lu", LastOffset);
        ImGui::TextColored  (Color(1.0f,1.0f,0.0f,1.0f), "Top     %lu", TopOffset);
        ImGui::Text         ("Count   %lu", Mem->Doc.Undo.Count);
        ImGui::Text         ("Index   %lu", Mem->Doc.Undo.CurrentIndex);

        ImGui::End();
        // =========================================================================================
#endif
    }

    // Canvas zooming and panning
    {
        // Panning
        Mem->Doc.CanvasPosition += ImGui::GetMouseDragDelta(2);
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
            Vec2 NewCanvasSize = Vec2((float)Mem->Doc.Width, (float)Mem->Doc.Height) * Mem->Doc.CanvasZoom;

            if ((NewCanvasSize.x > Mem->Window.Width || NewCanvasSize.y > Mem->Window.Height))
            {
                Vec2 PreScaleMousePos = (Mem->Mouse.Pos - Mem->Doc.CanvasPosition) / OldCanvasZoom;
                Mem->Doc.CanvasPosition -= Vec2(PreScaleMousePos.x * ScaleDelta * (float)Mem->Doc.Width, PreScaleMousePos.y * ScaleDelta * (float)Mem->Doc.Height);
            }
            else // TODO: Maybe disable centering on zoom out. Needs more usability testing.
            {
                Vec2 WindowSize         = Vec2((float)Mem->Window.Width, (float)Mem->Window.Height);
                Mem->Doc.CanvasPosition = (WindowSize - NewCanvasSize) * 0.5f;
            }
        }
    }

    // Draw canvas
    {
        // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
        GLint last_program, last_texture;
        glGetIntegerv  (GL_CURRENT_PROGRAM, &last_program);
        glGetIntegerv  (GL_TEXTURE_BINDING_2D, &last_texture);
        glEnable       (GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc    (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable      (GL_CULL_FACE);
        glDisable      (GL_DEPTH_TEST);
        glEnable       (GL_SCISSOR_TEST);
        glActiveTexture(GL_TEXTURE0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (Mem->Doc.CanvasZoom >= 2.0f) ? GL_NEAREST : GL_NEAREST);// : GL_LINEAR);

        glUseProgram      (Mem->Shaders[PapayaShader_ImGui].Handle);
        glUniformMatrix4fv(Mem->Shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &Mem->Window.ProjMtx[0][0]); // Projection matrix uniform
        glUniform1i       (Mem->Shaders[PapayaShader_ImGui].Uniforms[1], 0); // Texture uniform

        ImDrawVert Verts[6];
        {
            Vec2 Position = Mem->Doc.CanvasPosition;
            Vec2 Size     = Vec2(Mem->Doc.Width * Mem->Doc.CanvasZoom, Mem->Doc.Height * Mem->Doc.CanvasZoom);
            Verts[0].pos = Vec2(Position.x, Position.y);                    Verts[0].uv = Vec2(0.0f, 0.0f); Verts[0].col = 0xffffffff;
            Verts[1].pos = Vec2(Size.x + Position.x, Position.y);           Verts[1].uv = Vec2(1.0f, 0.0f); Verts[1].col = 0xffffffff;
            Verts[2].pos = Vec2(Size.x + Position.x, Size.y + Position.y);  Verts[2].uv = Vec2(1.0f, 1.0f); Verts[2].col = 0xffffffff;
            Verts[3].pos = Vec2(Position.x, Position.y);                    Verts[3].uv = Vec2(0.0f, 0.0f); Verts[3].col = 0xffffffff;
            Verts[4].pos = Vec2(Size.x + Position.x, Size.y + Position.y);  Verts[4].uv = Vec2(1.0f, 1.0f); Verts[4].col = 0xffffffff;
            Verts[5].pos = Vec2(Position.x, Size.y + Position.y);           Verts[5].uv = Vec2(0.0f, 1.0f); Verts[5].col = 0xffffffff;
        }
        glBindBuffer   (GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_Canvas].VboHandle);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Verts), Verts);

        glBindVertexArray(Mem->Meshes[PapayaMesh_Canvas].VaoHandle);

        if (Mem->Misc.DrawCanvas)
        {
            glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->Doc.TextureID);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        if (Mem->Misc.DrawOverlay)
        {
            glBindTexture  (GL_TEXTURE_2D, (GLuint)(intptr_t)Mem->Misc.FboSampleTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glDrawArrays   (GL_TRIANGLES, 0, 6);
        }

        // Restore modified state
        glBindVertexArray(0);
        glUseProgram (last_program);
        glDisable    (GL_SCISSOR_TEST);
        glDisable    (GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, last_texture);
    }

    // Draw brush cursor
    {
        // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
        GLint last_program, last_texture;
        glGetIntegerv  (GL_CURRENT_PROGRAM, &last_program);
        glGetIntegerv  (GL_TEXTURE_BINDING_2D, &last_texture);
        glEnable       (GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc    (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable      (GL_CULL_FACE);
        glDisable      (GL_DEPTH_TEST);
        glEnable       (GL_SCISSOR_TEST);

        glUseProgram      (Mem->Shaders[PapayaShader_BrushCursor].Handle);
        glUniformMatrix4fv(Mem->Shaders[PapayaShader_BrushCursor].Uniforms[0], 1, GL_FALSE, &Mem->Window.ProjMtx[0][0]); // Projection matrix uniform
        glUniform4f       (Mem->Shaders[PapayaShader_BrushCursor].Uniforms[1], 1.0f, 0.0f, 0.0f, Mem->Mouse.IsDown[1] ? Mem->Brush.Opacity/100.0f : 0.0f); // Brush color
        glUniform1f       (Mem->Shaders[PapayaShader_BrushCursor].Uniforms[2], Mem->Brush.Hardness / 100.0f); // Hardness
        glUniform1f       (Mem->Shaders[PapayaShader_BrushCursor].Uniforms[3], Mem->Brush.Diameter * Mem->Doc.CanvasZoom); // PixelDiameter

        ImDrawVert Verts[6];
        {
            float ScaledDiameter = Mem->Brush.Diameter * Mem->Doc.CanvasZoom;
            Vec2 Size            = Vec2(ScaledDiameter,ScaledDiameter);
            Vec2 Position        = (Mem->Mouse.IsDown[1] || Mem->Mouse.WasDown[1] ? Mem->Brush.RtDragStartPos : Mem->Mouse.Pos) - (Size * 0.5f);
            Verts[0].pos = Vec2(Position.x, Position.y);                    Verts[0].uv = Vec2(0.0f, 0.0f); Verts[0].col = 0xffffffff;
            Verts[1].pos = Vec2(Size.x + Position.x, Position.y);           Verts[1].uv = Vec2(1.0f, 0.0f); Verts[1].col = 0xffffffff;
            Verts[2].pos = Vec2(Size.x + Position.x, Size.y + Position.y);  Verts[2].uv = Vec2(1.0f, 1.0f); Verts[2].col = 0xffffffff;
            Verts[3].pos = Vec2(Position.x, Position.y);                    Verts[3].uv = Vec2(0.0f, 0.0f); Verts[3].col = 0xffffffff;
            Verts[4].pos = Vec2(Size.x + Position.x, Size.y + Position.y);  Verts[4].uv = Vec2(1.0f, 1.0f); Verts[4].col = 0xffffffff;
            Verts[5].pos = Vec2(Position.x, Size.y + Position.y);           Verts[5].uv = Vec2(0.0f, 1.0f); Verts[5].col = 0xffffffff;
        }
        glBindBuffer     (GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_BrushCursor].VboHandle);
        glBufferSubData  (GL_ARRAY_BUFFER, 0, sizeof(Verts), Verts);
        glBindVertexArray(Mem->Meshes[PapayaMesh_BrushCursor].VaoHandle);

        /*glScissor(34 + (int)Mem->Window.MaximizeOffset,
                  3 + (int)Mem->Window.MaximizeOffset,
                  (int)Mem->Window.Width - 37 - (2 * (int)Mem->Window.MaximizeOffset),
                  (int)Mem->Window.Height - 58 - (2 * (int)Mem->Window.MaximizeOffset));*/

        glDrawArrays(GL_TRIANGLES, 0, 6);


        // Restore modified state
        glBindVertexArray(0);
        glUseProgram (last_program);
        glDisable    (GL_SCISSOR_TEST);
        glDisable    (GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, last_texture);
    }

EndOfDoc:

    // Metrics window
    {
        if (Mem->Misc.ShowMetricsWindow)
        {
            ImGui::Begin("Metrics");
            //if (ImGui::CollapsingHeader("Input"))
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
                ImGui::Text("%f", Mem->Mouse.Pos.x);        ImGui::NextColumn();
                ImGui::Text("PosY");                        ImGui::NextColumn();
                ImGui::Text("%f", Mem->Mouse.Pos.y);        ImGui::NextColumn();
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
    {
        if (Mem->Picker.Open)
        {
            // Update hue picker
            {
                Vec2 Pos = Mem->Picker.Pos + Mem->Picker.HueStripPos;

                if (Mem->Mouse.Pressed[0] &&
                    Mem->Mouse.Pos.x > Pos.x &&
                    Mem->Mouse.Pos.x < Pos.x + Mem->Picker.HueStripSize.x &&
                    Mem->Mouse.Pos.y > Pos.y &&
                    Mem->Mouse.Pos.y < Pos.y + Mem->Picker.HueStripSize.y)
                {
                    Mem->Picker.DraggingHue = true;
                }
                else if (Mem->Mouse.Released[0] && Mem->Picker.DraggingHue)
                {
                    Mem->Picker.DraggingHue = false;
                }

                if (Mem->Picker.DraggingHue)
                {
                    Mem->Picker.CursorH = 1.0f - (Mem->Mouse.Pos.y - Pos.y) / 256.0f;
                    Mem->Picker.CursorH = Math::Clamp(Mem->Picker.CursorH, 0.0f, 1.0f);
                }

            }

            // Update saturation-value picker
            {
                Vec2 Pos = Mem->Picker.Pos + Mem->Picker.SVBoxPos;

                if (Mem->Mouse.Pressed[0] &&
                    Mem->Mouse.Pos.x > Pos.x &&
                    Mem->Mouse.Pos.x < Pos.x + Mem->Picker.SVBoxSize.x &&
                    Mem->Mouse.Pos.y > Pos.y &&
                    Mem->Mouse.Pos.y < Pos.y + Mem->Picker.SVBoxSize.y)
                {
                    Mem->Picker.DraggingSV = true;
                }
                else if (Mem->Mouse.Released[0] && Mem->Picker.DraggingSV)
                {
                    Mem->Picker.DraggingSV = false;
                }

                if (Mem->Picker.DraggingSV)
                {
                    Mem->Picker.CursorSV.x =        (Mem->Mouse.Pos.x - Pos.x) / 256.0f;
                    Mem->Picker.CursorSV.y = 1.0f - (Mem->Mouse.Pos.y - Pos.y) / 256.0f;
                    Mem->Picker.CursorSV.x = Math::Clamp(Mem->Picker.CursorSV.x, 0.0f, 1.0f);
                    Mem->Picker.CursorSV.y = Math::Clamp(Mem->Picker.CursorSV.y, 0.0f, 1.0f);

                }
            }

            // Update new color
            {
                float r, g, b;
                Math::HSVtoRGB(Mem->Picker.CursorH, Mem->Picker.CursorSV.x, Mem->Picker.CursorSV.y, r, g, b);
                Mem->Picker.NewColor = Color(Math::RoundToInt(r * 255.0f),  // Note: Rounding is essential.
                                             Math::RoundToInt(g * 255.0f),  //       Without it, RGB->HSV->RGB
                                             Math::RoundToInt(b * 255.0f)); //       is a lossy operation.
            }

            // Draw hue picker
            {
                // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
                GLint last_program, last_texture;
                glGetIntegerv  (GL_CURRENT_PROGRAM, &last_program);
                glGetIntegerv  (GL_TEXTURE_BINDING_2D, &last_texture);
                glEnable       (GL_BLEND);
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc    (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable      (GL_CULL_FACE);
                glDisable      (GL_DEPTH_TEST);

                glUseProgram      (Mem->Shaders[PapayaShader_PickerHStrip].Handle);
                glUniformMatrix4fv(Mem->Shaders[PapayaShader_PickerHStrip].Uniforms[0], 1, GL_FALSE, &Mem->Window.ProjMtx[0][0]); // Projection matrix uniform
                glUniform1f       (Mem->Shaders[PapayaShader_PickerHStrip].Uniforms[1], Mem->Picker.CursorH); // Cursor

                glBindBuffer     (GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_PickerHStrip].VboHandle);
                glBindVertexArray(Mem->Meshes[PapayaMesh_PickerHStrip].VaoHandle);

                glDrawArrays(GL_TRIANGLES, 0, 6);

                // Restore modified state
                glBindVertexArray(0);
                glUseProgram (last_program);
                glDisable    (GL_BLEND);
                glBindTexture(GL_TEXTURE_2D, last_texture);
            }

            // Draw saturation-value picker
            {
                // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
                GLint last_program, last_texture;
                glGetIntegerv  (GL_CURRENT_PROGRAM, &last_program);
                glGetIntegerv  (GL_TEXTURE_BINDING_2D, &last_texture);
                glEnable       (GL_BLEND);
                glBlendEquation(GL_FUNC_ADD);
                glBlendFunc    (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDisable      (GL_CULL_FACE);
                glDisable      (GL_DEPTH_TEST);

                glUseProgram      (Mem->Shaders[PapayaShader_PickerSVBox].Handle);
                glUniformMatrix4fv(Mem->Shaders[PapayaShader_PickerSVBox].Uniforms[0], 1, GL_FALSE, &Mem->Window.ProjMtx[0][0]); // Projection matrix uniform
                glUniform1f       (Mem->Shaders[PapayaShader_PickerSVBox].Uniforms[1], Mem->Picker.CursorH); // Hue
                glUniform2f       (Mem->Shaders[PapayaShader_PickerSVBox].Uniforms[2], Mem->Picker.CursorSV.x, Mem->Picker.CursorSV.y); //Cursor                                 // Current

                glBindBuffer     (GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_PickerSVBox].VboHandle);
                glBindVertexArray(Mem->Meshes[PapayaMesh_PickerSVBox].VaoHandle);

                glDrawArrays(GL_TRIANGLES, 0, 6);

                // Restore modified state
                glBindVertexArray(0);
                glUseProgram (last_program);
                glDisable    (GL_BLEND);
                glBindTexture(GL_TEXTURE_2D, last_texture);
            }
        }
    }

    // Last mouse info
    {
        Mem->Mouse.LastPos    = ImGui::GetMousePos();
        Mem->Mouse.LastUV     = Mem->Mouse.UV;
        Mem->Mouse.WasDown[0] = ImGui::IsMouseDown(0);
        Mem->Mouse.WasDown[1] = ImGui::IsMouseDown(1);
        Mem->Mouse.WasDown[2] = ImGui::IsMouseDown(2);
    }
}

void RenderImGui(ImDrawData* DrawData, void* MemPtr)
{
    PapayaMemory* Mem = (PapayaMemory*)MemPtr;

    // Backup GL state
    GLint last_program, last_texture, last_array_buffer, last_element_array_buffer, last_vertex_array;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable       (GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc    (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable      (GL_CULL_FACE);
    glDisable      (GL_DEPTH_TEST);
    glEnable       (GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Handle cases of screen coordinates != from framebuffer coordinates (e.g. retina displays)
    ImGuiIO& io     = ImGui::GetIO();
    float fb_height = io.DisplaySize.y * io.DisplayFramebufferScale.y;
    DrawData->ScaleClipRects(io.DisplayFramebufferScale);

    glUseProgram      (Mem->Shaders[PapayaShader_ImGui].Handle);
    glUniform1i       (Mem->Shaders[PapayaShader_ImGui].Uniforms[1], 0);
    glUniformMatrix4fv(Mem->Shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &Mem->Window.ProjMtx[0][0]);
    glBindVertexArray (Mem->Meshes[PapayaMesh_ImGui].VaoHandle);

    for (int n = 0; n < DrawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = DrawData->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_ImGui].VboHandle);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Mem->Meshes[PapayaMesh_ImGui].ElementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);

        for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
        {
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    // Restore modified GL state
    glUseProgram     (last_program);
    glBindTexture    (GL_TEXTURE_2D, last_texture);
    glBindBuffer     (GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer     (GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBindVertexArray(last_vertex_array);
    glDisable        (GL_SCISSOR_TEST);
}

}
