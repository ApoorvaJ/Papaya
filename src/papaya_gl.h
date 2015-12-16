#pragma once

#define GLCHK(stmt) stmt; GL::CheckError(#stmt, __FILE__, __LINE__)

namespace GL
{
    // Initializes GLEW and performs capability check
    // Returns true if current context is capable of OpenGL profile required by Papaya.
    // If incapable, returns false and reports which profile/extension is unsupported.
    bool InitAndValidate()
    {
        // TODO: Reporting via message box or logging before returning false
        if (glewInit() != GLEW_OK) { return false; }

        // Core profile capability check
        if (!GL_VERSION_2_1) { return false; }
        
        // Extension capability checks
        //   Extension                                         OpenGL dependency version
        //   ==========                                        =========================
        if (!GL_ARB_framebuffer_object)    { return false; }   // OpenGL 1.1

        return true;
    }

    void CheckError(const char* Expr, const char* File, int Line)
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
        
        char Buffer[256];
        snprintf(Buffer, 256, "OpenGL Error: %s in %s:%d\n", Str, File, Line);
        Platform::Print(Buffer);
        snprintf(Buffer, 256, "    --- Expression: %s\n", Expr);
        Platform::Print(Buffer);
    }

    internal void PrintCompileErrors(uint32 Handle, const char* Type, const char* File, int Line)
    {
        int32 Status;
        GLCHK( glGetShaderiv(Handle, GL_COMPILE_STATUS, &Status) );
        if (Status != GL_TRUE)
        {
            char Buffer[256];
            snprintf(Buffer, 256, "Compilation error in %s shader in %s:%d\n", Type, File, Line);
            Platform::Print(Buffer);

            char ErrorLog[4096];
            int32 OutLength;
            GLCHK( glGetShaderInfoLog(Handle, 4096, &OutLength, ErrorLog) );
            Platform::Print(ErrorLog);
            Platform::Print("\n");
        }
    }

    void CompileShader(ShaderInfo& Shader, const char* File, int Line, const char* Vert, const char* Frag, int32 AttribCount, int32 UniformCount, ...)
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
                char Buffer[256];
                snprintf(Buffer, 256, "Attribute %s not found in shader at %s:%d\n", Name, File, Line);
                Platform::Print(Buffer);
            }
        }

        for (int32 i = 0; i < UniformCount; i++)
        {
            const char* Name = va_arg(Args, const char*);
            Shader.Uniforms[i] = GLCHK( glGetUniformLocation(Shader.Handle, Name) );

            if (Shader.Uniforms[i] == -1)
            {
                char Buffer[256];
                snprintf(Buffer, 256, "Uniform %s not found in shader at %s:%d\n", Name, File, Line);
                Platform::Print(Buffer);
            }
        }
        va_end(Args);
    }

    void SetVertexAttribs(const ShaderInfo& Shader)
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

    void InitQuad(MeshInfo& Mesh, Vec2 Pos, Vec2 Size, GLenum Usage)
    {
        GLCHK( glGenBuffers  (1, &Mesh.VboHandle) );
        GLCHK( glBindBuffer  (GL_ARRAY_BUFFER, Mesh.VboHandle) );

        ImDrawVert Verts[6];
        Verts[0].pos = Vec2(Pos.x, Pos.y);                   Verts[0].uv = Vec2(0.0f, 1.0f); Verts[0].col = 0xffffffff;
        Verts[1].pos = Vec2(Size.x + Pos.x, Pos.y);          Verts[1].uv = Vec2(1.0f, 1.0f); Verts[1].col = 0xffffffff;
        Verts[2].pos = Vec2(Size.x + Pos.x, Size.y + Pos.y); Verts[2].uv = Vec2(1.0f, 0.0f); Verts[2].col = 0xffffffff;
        Verts[3].pos = Vec2(Pos.x, Pos.y);                   Verts[3].uv = Vec2(0.0f, 1.0f); Verts[3].col = 0xffffffff;
        Verts[4].pos = Vec2(Size.x + Pos.x, Size.y + Pos.y); Verts[4].uv = Vec2(1.0f, 0.0f); Verts[4].col = 0xffffffff;
        Verts[5].pos = Vec2(Pos.x, Size.y + Pos.y);          Verts[5].uv = Vec2(0.0f, 0.0f); Verts[5].col = 0xffffffff;

        GLCHK( glBufferData(GL_ARRAY_BUFFER, sizeof(Verts), Verts, Usage) );
    }

    void TransformQuad(const MeshInfo& Mesh, Vec2 Pos, Vec2 Size)
    {
        ImDrawVert Verts[6];
        {
            Verts[0].pos = Vec2(Pos.x, Pos.y);                    Verts[0].uv = Vec2(0.0f, 0.0f); Verts[0].col = 0xffffffff;
            Verts[1].pos = Vec2(Size.x + Pos.x, Pos.y);           Verts[1].uv = Vec2(1.0f, 0.0f); Verts[1].col = 0xffffffff;
            Verts[2].pos = Vec2(Size.x + Pos.x, Size.y + Pos.y);  Verts[2].uv = Vec2(1.0f, 1.0f); Verts[2].col = 0xffffffff;
            Verts[3].pos = Vec2(Pos.x, Pos.y);                    Verts[3].uv = Vec2(0.0f, 0.0f); Verts[3].col = 0xffffffff;
            Verts[4].pos = Vec2(Size.x + Pos.x, Size.y + Pos.y);  Verts[4].uv = Vec2(1.0f, 1.0f); Verts[4].col = 0xffffffff;
            Verts[5].pos = Vec2(Pos.x, Size.y + Pos.y);           Verts[5].uv = Vec2(0.0f, 1.0f); Verts[5].col = 0xffffffff;
        }
        GLCHK( glBindBuffer(GL_ARRAY_BUFFER, Mesh.VboHandle) );
        GLCHK( glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Verts), Verts) );
    }

    void DrawQuad(const MeshInfo& Mesh, const ShaderInfo& Shader, int32 UniformCount, ...)
    {
        GLint last_program, last_texture;
        GLCHK( glGetIntegerv  (GL_CURRENT_PROGRAM, &last_program) );
        GLCHK( glGetIntegerv  (GL_TEXTURE_BINDING_2D, &last_texture) );
        GLCHK( glEnable       (GL_BLEND) );
        GLCHK( glBlendEquation(GL_FUNC_ADD) );
        GLCHK( glBlendFunc    (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) );
        GLCHK( glDisable      (GL_CULL_FACE) );
        GLCHK( glDisable      (GL_DEPTH_TEST) );
        GLCHK( glEnable       (GL_SCISSOR_TEST) );

        GLCHK( glBindBuffer  (GL_ARRAY_BUFFER, Mesh.VboHandle) );
        GLCHK( glUseProgram   (Shader.Handle) );

        // Uniforms
        {
            va_list Args;
            va_start(Args, UniformCount);
            for (int32 i = 0; i < UniformCount; i++)
            {
                switch (va_arg(Args, UniformType_))
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

        GLCHK( glDrawArrays(GL_TRIANGLES, 0, 6) );

        // Restore modified state
        GLCHK( glBindBuffer (GL_ARRAY_BUFFER, 0) );
        GLCHK( glUseProgram (last_program) );                // TODO: Necessary?
        GLCHK( glDisable    (GL_SCISSOR_TEST) );             //
        GLCHK( glDisable    (GL_BLEND) );                    //
        GLCHK( glBindTexture(GL_TEXTURE_2D, last_texture) ); //
    }
}
