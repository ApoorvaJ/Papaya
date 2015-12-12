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
}
