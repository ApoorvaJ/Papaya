#pragma once


namespace Util
{
    // Timer
    void StartTime(Timer_ Timer, PapayaMemory* Mem)
    {
        int32 i = Timer;
        Mem->Debug.Timers[i].StartMilliseconds = Platform::GetMilliseconds();
        Mem->Debug.Timers[i].StartCycleCount = __rdtsc();
    }

    void StopTime(Timer_ Timer, PapayaMemory* Mem)
    {
        int32 i = Timer;
        Mem->Debug.Timers[i].StopCycleCount = __rdtsc();
        Mem->Debug.Timers[i].StopMilliseconds = Platform::GetMilliseconds();
        Mem->Debug.Timers[i].CyclesElapsed = Mem->Debug.Timers[i].StopCycleCount - Mem->Debug.Timers[i].StartCycleCount;
        Mem->Debug.Timers[i].MillisecondsElapsed = (double)(Mem->Debug.Timers[i].StopMilliseconds - Mem->Debug.Timers[i].StartMilliseconds) *
                                                     1000.0 / (double)Mem->Debug.TicksPerSecond;
    }

    void ClearTimes()
    {

    }

    void DisplayTimes(PapayaMemory* Mem)
    {
        ImGui::SetNextWindowPos(Vec2(0, ImGui::GetIO().DisplaySize.y - 75));
        ImGui::SetNextWindowSize(Vec2(300, 75));
        ImGui::Begin("Profiler");
        for (int32 i = 0; i < Timer_COUNT; i++)
        {
            ImGui::Text("%d: Cycles: %lu, MS: %f", i, Mem->Debug.Timers[i].CyclesElapsed, Mem->Debug.Timers[i].MillisecondsElapsed);
        }
        ImGui::End();
    }

    // OpenGL
    void PrintGlError()
    {
        GLenum Error = glGetError();
        switch (Error)
        {
            case GL_NO_ERROR:
            {
                Platform::Print("No error\n");
            } break;

            case GL_INVALID_ENUM:
            {
                Platform::Print("Invalid enum\n");
            } break;

            case GL_INVALID_VALUE:
            {
                Platform::Print("Invalid value\n");
            } break;

            case GL_INVALID_OPERATION:
            {
                Platform::Print("Invalid operation\n");
            } break;

            case GL_INVALID_FRAMEBUFFER_OPERATION:
            {
                Platform::Print("Invalid framebuffer operation\n");
            } break;

            case GL_OUT_OF_MEMORY:
            {
                Platform::Print("Out of memory\n");
            } break;

            case GL_STACK_UNDERFLOW:
            {
                Platform::Print("Stack underflow\n");
            } break;

            case GL_STACK_OVERFLOW:
            {
                Platform::Print("Stack overflow\n");
            } break;

            default:
            {
                Platform::Print("Undefined error\n");
            } break;
        }
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
