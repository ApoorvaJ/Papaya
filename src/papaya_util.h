#pragma once


enum TimerScope_
{
    TimerScope_1,
    TimerScope_2,
    TimerScope_COUNT
};

struct PapayaDebugTimer
{
    uint64 StartCycleCount,   StopCycleCount,   CyclesElapsed;
    int64 StartMilliseconds, StopMilliseconds;
    double MillisecondsElapsed;
};

struct PapayaDebugMemory
{
    int64 Time;             //
    int64 TicksPerSecond;   // TODO: Put in PapayaMemory so that we can track this during shipping builds as well
    PapayaDebugTimer Timers[TimerScope_COUNT];
};

namespace Util
{
    // Timer
    void StartTime(TimerScope_ TimerScope, PapayaDebugMemory* DebugMemory)
    {
        int32 i = TimerScope;
        DebugMemory->Timers[i].StartMilliseconds = Platform::GetMilliseconds();
        DebugMemory->Timers[i].StartCycleCount = __rdtsc();
    }

    void StopTime(TimerScope_ TimerScope, PapayaDebugMemory* DebugMemory)
    {
        int32 i = TimerScope;
        DebugMemory->Timers[i].StopCycleCount = __rdtsc();
        DebugMemory->Timers[i].StopMilliseconds = Platform::GetMilliseconds();
        DebugMemory->Timers[i].CyclesElapsed = DebugMemory->Timers[i].StopCycleCount - DebugMemory->Timers[i].StartCycleCount;
        DebugMemory->Timers[i].MillisecondsElapsed = (double)(DebugMemory->Timers[i].StopMilliseconds - DebugMemory->Timers[i].StartMilliseconds) *
                                                     1000.0 / (double)DebugMemory->TicksPerSecond;
    }

    void ClearTimes()
    {

    }

    void DisplayTimes(PapayaDebugMemory* DebugMemory)
    {
        ImGui::SetNextWindowPos(Vec2(0, ImGui::GetIO().DisplaySize.y - 75));
        ImGui::SetNextWindowSize(Vec2(300, 75));
        ImGui::Begin("Profiler");
        for (int32 i = 0; i < TimerScope_COUNT; i++)
        {
            ImGui::Text("%d: Cycles: %I64u, MS: %f", i, DebugMemory->Timers[i].CyclesElapsed, DebugMemory->Timers[i].MillisecondsElapsed);
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
