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

        va_list Args;
        va_start(Args, UniformCount);
        for (int32 i = 0; i < AttribCount; i++)
        {
            Shader.Attributes[i] = GLCHK( glGetAttribLocation(Shader.Handle, va_arg(Args, const char*)) );
        }

        for (int32 i = 0; i < UniformCount; i++)
        {
            Shader.Uniforms[i] = GLCHK( glGetUniformLocation(Shader.Handle, va_arg(Args, const char*)) );
        }
        va_end(Args);
    }
}
