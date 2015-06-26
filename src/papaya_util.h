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
		ImGui::SetNextWindowPos(Vec2(0, ImGui::GetIO().DisplaySize.y - 300));
		ImGui::SetNextWindowSize(Vec2(600, 300));
		ImGui::Begin("Profiler");
		for (int32 i = 0; i < TimerScope_COUNT; i++)
		{
			ImGui::Text("%d: Cycles: %I64u, MS: %f", i, DebugMemory->Timers[i].CyclesElapsed, DebugMemory->Timers[i].MillisecondsElapsed);
		}
		ImGui::End();
	}
}