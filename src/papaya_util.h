#pragma once


enum TimerScope_
{
	TimerScope_CPU_BRESENHAM,
	TimerScope_CPU_EFLA,
	TimerScope_COUNT
};

struct PapayaDebugTimer
{
	uint64 StartCycleCount,   StopCycleCount,   CyclesElapsed;
	LARGE_INTEGER StartMilliseconds, StopMilliseconds;
	double MillisecondsElapsed;
};

struct PapayaDebugMemory
{
	int64 Time;				//
	int64 TicksPerSecond;   // TODO: Put in PapayaMemory so that we can track this during shipping builds as well
	PapayaDebugTimer Timers[TimerScope_COUNT];
};

namespace Util
{
	// Timer
	void StartTime(TimerScope_ TimerScope, PapayaDebugMemory* DebugMemory)
	{
		int32 i = TimerScope;
		QueryPerformanceCounter(&DebugMemory->Timers[i].StartMilliseconds);
		DebugMemory->Timers[i].StartCycleCount = __rdtsc();
	}

	void StopTime(TimerScope_ TimerScope, PapayaDebugMemory* DebugMemory)
	{
		int32 i = TimerScope;
		DebugMemory->Timers[i].StopCycleCount = __rdtsc();
		QueryPerformanceCounter(&DebugMemory->Timers[i].StopMilliseconds);
		DebugMemory->Timers[i].CyclesElapsed = DebugMemory->Timers[i].StopCycleCount   - DebugMemory->Timers[i].StartCycleCount;
		DebugMemory->Timers[i].MillisecondsElapsed = (double)(DebugMemory->Timers[i].StopMilliseconds.QuadPart - DebugMemory->Timers[i].StartMilliseconds.QuadPart) *
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
				OutputDebugString("No error\n");
			} break;

			case GL_INVALID_ENUM:
			{
				OutputDebugString("Invalid enum\n");
			} break;

			case GL_INVALID_VALUE:
			{
				OutputDebugString("Invalid value\n");
			} break;

			case GL_INVALID_OPERATION:
			{
				OutputDebugString("Invalid operation\n");
			} break;

			case GL_INVALID_FRAMEBUFFER_OPERATION:
			{
				OutputDebugString("Invalid framebuffer operation\n");
			} break;

			case GL_OUT_OF_MEMORY:
			{
				OutputDebugString("Out of memory\n");
			} break;

			case GL_STACK_UNDERFLOW:
			{
				OutputDebugString("Stack underflow\n");
			} break;

			case GL_STACK_OVERFLOW:
			{
				OutputDebugString("Stack overflow\n");
			} break;

			default:
			{
				OutputDebugString("Undefined error\n");
			} break;
		}
	}

	void PrintGlShaderCompilationStatus(uint32 ShaderHandle)
	{
		int32 CompilationStatus;
		glGetShaderiv(ShaderHandle, GL_COMPILE_STATUS, &CompilationStatus);
		if (CompilationStatus == GL_TRUE)
		{
			OutputDebugString("Compilation successful\n");
		}
		else
		{
			OutputDebugString("Compilation failed\n");
		}
	}
}