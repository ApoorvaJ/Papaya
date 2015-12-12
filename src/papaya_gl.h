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
        if (glewInit() != GLEW_OK)
        {
            return false;
        }

        // Core profile capability check
        if (!GL_VERSION_1_4)
        {
            return false;
        }
        
        // Extension capability checks
        //   Extension                       OpenGL dependency version
        //   ==========                      =========================
        if (!GL_ARB_vertex_buffer_object)   // OpenGL 1.4
        {
            return false;
        }
        if (!GL_ARB_vertex_program)         // OpenGL 1.3
        {
            return false;
        }
        if (!GL_ARB_fragment_program)       // OpenGL 1.3
        {
            return false;
        }
        if (!GL_ARB_shader_objects)         // OpenGL 1.0
        {
            return false;
        }

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

    void PrintGlShaderCompilationStatus(uint32 ShaderHandle)
    {
        int32 CompilationStatus;
        glGetShaderiv(ShaderHandle, GL_COMPILE_STATUS, &CompilationStatus);
        if (CompilationStatus == GL_TRUE)
        {
            Platform::Print("Compilation successful\n");
        }
        else
        {
            Platform::Print("Compilation failed\n");
            const int32 BufferLength = 4096;
            int32 OutLength;
            char ErrorLog[BufferLength];

            glGetShaderInfoLog(ShaderHandle, BufferLength, &OutLength, ErrorLog);
            Platform::Print(ErrorLog);
        }
    }

}
