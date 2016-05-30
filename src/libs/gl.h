
#ifndef GL_H
#define GL_H

#include "types.h"

#define GLCHK(stmt) stmt; GL::CheckError(#stmt, __FILE__, __LINE__)

enum UniformType_
{
    UniformType_Float,
    UniformType_Vec2,
    UniformType_Matrix4,
    UniformType_Color,
    UniformType_COUNT
};

struct ShaderInfo
{
    uint32 Handle;
    int32 AttribCount, UniformCount;
    int32 Attributes[8];
    int32 Uniforms[8];
};

struct MeshInfo
{
    bool IsLineLoop; // 0 -> Triangle, !0 -> Line
    size_t VboSize, IndexCount;
    uint32 VboHandle, ElementsHandle;
};

namespace GL
{
    bool InitAndValidate();
    void CheckError(const char* Expr, const char* File, int Line);
    void CompileShader(ShaderInfo& Shader, const char* File, int Line, const char* Vert, const char* Frag, int32 AttribCount, int32 UniformCount, ...);
    void SetVertexAttribs(const ShaderInfo& Shader);
    void InitQuad(MeshInfo& Mesh, Vec2 Pos, Vec2 Size, uint32 Usage);
    void TransformQuad(const MeshInfo& Mesh, Vec2 Pos, Vec2 Size);
    void DrawMesh(const MeshInfo& Mesh, const ShaderInfo& Shader, bool Scissor, int32 UniformCount, ...);
    uint32 AllocateTexture(int32 Width, int32 Height, uint8* Data = 0);
}

#endif //GL_H

// =======================================================================================

#ifdef GL_IMPLEMENTATION

// Initializes GLEW and performs capability check
// Returns true if current context is capable of OpenGL profile required by Papaya.
// If incapable, returns false and reports which profile/extension is unsupported.
bool GL::InitAndValidate()
{
    // TODO: Reporting via message box or logging before returning false
    if (glewInit() != GLEW_OK) 
    { 
        printf("GLEW initialization failed.\n");
        return false; 
    }

    // Core profile capability check
    if (!glewIsSupported("GL_VERSION_2_1")) 
    {
        printf("This hardware doesn't support OpenGL 2.1.\n");
        return false; 
    }

    // Extension capability checks
    //                    Extension                     OpenGL dependency version
    //                    ==========                    =========================
    if (!glewIsSupported("GL_ARB_framebuffer_object"))  // OpenGL 1.1
    {
        printf("This hardware doesn't support OpenGL extension GL_ARB_framebuffer_object.\n");
        return false;
    }

    return true;
}

void GL::CheckError(const char* Expr, const char* File, int Line)
{
    GLenum Error = glGetError();
    const char* Str = "";

    if      (Error == GL_NO_ERROR)                      { return; }
    else if (Error == GL_INVALID_ENUM)                  { Str = "Invalid enum"; }
    else if (Error == GL_INVALID_VALUE)                 { Str = "Invalid value"; }
    else if (Error == GL_INVALID_OPERATION)             { Str = "Invalid operation"; }
    else if (Error == GL_INVALID_FRAMEBUFFER_OPERATION) { Str = "Invalid framebuffer operation"; }
    else if (Error == GL_OUT_OF_MEMORY)                 { Str = "Out of memory"; }
    else if (Error == GL_STACK_UNDERFLOW)               { Str = "Stack underflow"; }
    else if (Error == GL_STACK_OVERFLOW)                { Str = "Stack overflow"; }
    else                                                { Str = "Undefined error"; }

    printf("OpenGL Error: %s in %s:%d\n", Str, File, Line);
    printf("    --- Expression: %s\n", Expr);
}

internal void PrintCompileErrors(uint32 Handle, const char* Type, const char* File, int32 Line)
{
    int32 CompileStatus;
    GLCHK( glGetShaderiv(Handle, GL_COMPILE_STATUS, &CompileStatus) );
    if (CompileStatus != GL_TRUE)
    {
        printf("Compilation error in %s shader in %s:%d\n", Type, File, Line);

        char ErrorLog[4096];
        int32 OutLength;
        GLCHK( glGetShaderInfoLog(Handle, 4096, &OutLength, ErrorLog) );
        printf("%s", ErrorLog);
        printf("\n");
    }
}

void GL::CompileShader(ShaderInfo& Shader, const char* File, int Line, const char* Vert, const char* Frag, int32 AttribCount, int32 UniformCount, ...)
{
    Shader.Handle     = GLCHK( glCreateProgram() );
    uint32 VertHandle = GLCHK( glCreateShader(GL_VERTEX_SHADER) );
    uint32 FragHandle = GLCHK( glCreateShader(GL_FRAGMENT_SHADER) );

    GLCHK( glShaderSource (VertHandle, 1, &Vert, 0) );
    GLCHK( glShaderSource (FragHandle, 1, &Frag, 0) );
    GLCHK( glCompileShader(VertHandle) );
    GLCHK( glCompileShader(FragHandle) );
    GLCHK( glAttachShader (Shader.Handle, VertHandle) );
    GLCHK( glAttachShader (Shader.Handle, FragHandle) );

    PrintCompileErrors(VertHandle, "vertex"  , File, Line);
    PrintCompileErrors(FragHandle, "fragment", File, Line);

    GLCHK( glLinkProgram(Shader.Handle) ); // TODO: Print linking errors

    Shader.AttribCount = AttribCount;
    Shader.UniformCount = UniformCount;
    va_list Args;
    va_start(Args, UniformCount);
    for (int32 i = 0; i < AttribCount; i++)
    {
        const char* Name = va_arg(Args, const char*);
        Shader.Attributes[i] = GLCHK( glGetAttribLocation(Shader.Handle, Name) );

        if (Shader.Attributes[i] == -1)
        {
            printf("Attribute %s not found in shader at %s:%d\n", Name, File, Line);
        }
    }

    for (int32 i = 0; i < UniformCount; i++)
    {
        const char* Name = va_arg(Args, const char*);
        Shader.Uniforms[i] = GLCHK( glGetUniformLocation(Shader.Handle, Name) );

        if (Shader.Uniforms[i] == -1)
        {
            printf("Uniform %s not found in shader at %s:%d\n", Name, File, Line);
        }
    }
    va_end(Args);
}

void GL::SetVertexAttribs(const ShaderInfo& Shader)
{
    for (int32 i = 0; i < Shader.AttribCount; i++)
    {
        GLCHK( glEnableVertexAttribArray(Shader.Attributes[i]) );
    }

    #define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
    GLCHK( glVertexAttribPointer(Shader.Attributes[0], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos)) );   // Position attribute
    GLCHK( glVertexAttribPointer(Shader.Attributes[1], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv)) );    // UV attribute
    if (Shader.AttribCount > 2)
    {
        GLCHK( glVertexAttribPointer(Shader.Attributes[2], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col)) ); // Color attribute
    }
    #undef OFFSETOF
}

void GL::InitQuad(MeshInfo& Mesh, Vec2 Pos, Vec2 Size, uint32 Usage)
{
    GLCHK( glGenBuffers  (1, &Mesh.VboHandle) );
    GLCHK( glBindBuffer  (GL_ARRAY_BUFFER, Mesh.VboHandle) );
    GLCHK( glBufferData(GL_ARRAY_BUFFER, sizeof(ImDrawVert) * 6, 0, (GLenum)Usage) );
    Mesh.IndexCount = 6;
    TransformQuad(Mesh, Pos, Size);
}

void GL::TransformQuad(const MeshInfo& Mesh, Vec2 Pos, Vec2 Size)
{
    float X1 = Pos.x;
    float X2 = Pos.x + Size.x;
    float Y1 = Pos.y;
    float Y2 = Pos.y + Size.y;
    ImDrawVert Verts[6];
    {
        Verts[0].pos = Vec2(X1,Y2); Verts[0].uv = Vec2(0.0f, 1.0f); Verts[0].col = 0xffffffff;
        Verts[1].pos = Vec2(X1,Y1); Verts[1].uv = Vec2(0.0f, 0.0f); Verts[1].col = 0xffffffff;
        Verts[2].pos = Vec2(X2,Y2); Verts[2].uv = Vec2(1.0f, 1.0f); Verts[2].col = 0xffffffff;
        Verts[3].pos = Vec2(X2,Y1); Verts[3].uv = Vec2(1.0f, 0.0f); Verts[3].col = 0xffffffff;
        Verts[4].pos = Vec2(X2,Y2); Verts[4].uv = Vec2(1.0f, 1.0f); Verts[4].col = 0xffffffff;
        Verts[5].pos = Vec2(X1,Y1); Verts[5].uv = Vec2(0.0f, 0.0f); Verts[5].col = 0xffffffff;
    }
    GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mesh.VboHandle) );
    GLCHK( glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Verts), Verts) );
}

void GL::DrawMesh(const MeshInfo& Mesh, const ShaderInfo& Shader, bool Scissor,
        int32 UniformCount, ...)
{
    GLint last_program, last_texture;
    GLCHK( glGetIntegerv  (GL_CURRENT_PROGRAM, &last_program) );
    GLCHK( glGetIntegerv  (GL_TEXTURE_BINDING_2D, &last_texture) );
    GLCHK( glEnable       (GL_BLEND) );
    GLCHK( glBlendEquation(GL_FUNC_ADD) );
    GLCHK( glBlendFunc    (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) );
    GLCHK( glDisable      (GL_CULL_FACE) );
    GLCHK( glDisable      (GL_DEPTH_TEST) );
    if (Scissor) { GLCHK( glEnable (GL_SCISSOR_TEST) ); }
    else         { GLCHK( glDisable(GL_SCISSOR_TEST) ); }

    GLCHK( glBindBuffer  (GL_ARRAY_BUFFER, Mesh.VboHandle) );
    GLCHK( glUseProgram   (Shader.Handle) );

    // Uniforms
    {
        va_list Args;
        va_start(Args, UniformCount);
        for (int32 i = 0; i < UniformCount; i++)
        {
            switch (va_arg(Args, int))
            {
                case UniformType_Float:
                {
                    GLCHK( glUniform1f(Shader.Uniforms[i], (float)va_arg(Args, double)) );
                } break;

                case UniformType_Matrix4:
                {
                    GLCHK( glUniformMatrix4fv(Shader.Uniforms[i], 1, GL_FALSE, va_arg(Args, float*)) );
                } break;

                case UniformType_Vec2:
                {
                    Vec2 Vec = va_arg(Args, Vec2);
                    GLCHK( glUniform2f(Shader.Uniforms[i], Vec.x, Vec.y) );
                } break;

                case UniformType_Color:
                {
                    Color Col = va_arg(Args, Color);
                    GLCHK( glUniform4f(Shader.Uniforms[i], Col.r, Col.g, Col.b, Col.a) );
                } break;
            }
        }
        va_end(Args);
    }

    // Attribs
    SetVertexAttribs(Shader);

    GLCHK( glDrawArrays(Mesh.IsLineLoop ? GL_LINE_LOOP : GL_TRIANGLES, 0, Mesh.IndexCount) );

    // Restore modified state
    GLCHK( glBindBuffer (GL_ARRAY_BUFFER, 0) );
    GLCHK( glUseProgram (last_program) );                // TODO: Necessary?
    GLCHK( glDisable    (GL_SCISSOR_TEST) );             //
    GLCHK( glDisable    (GL_BLEND) );                    //
    GLCHK( glBindTexture(GL_TEXTURE_2D, last_texture) ); //
}

uint32 GL::AllocateTexture(int32 Width, int32 Height, uint8* Data)
{
    uint32 Tex;
    GLCHK( glGenTextures  (1, &Tex) );
    GLCHK( glBindTexture  (GL_TEXTURE_2D, Tex) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
    GLCHK( glTexImage2D   (GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, Data) );
    return Tex;
}

#endif //GL_IMPLEMENTATION
