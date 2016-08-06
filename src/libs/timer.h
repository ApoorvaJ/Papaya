
#ifndef TIMER_H
#define TIMER_H

// TODO: Move rdtsc code into platform layer?
#ifdef __linux__
#include <x86intrin.h>
#elif defined(__APPLE__)
    #if defined(__x86_64__)
        static __inline__ unsigned long long __rdtsc()
        {
            unsigned Hi, Lo;
            __asm__ __volatile__("rdtsc" : "=a"(Lo), "=d"(Hi));
            return ((unsigned long long)Lo) | (((unsigned long long)Hi) << 32);
        }
    #elif defined(__i386__)
        static __inline__ unsigned long long __rdtsc()
        {
            unsigned long long int Result;
            __asm__ volatile(".byte 0x0f, 0x31" : "=A"(Result));
            return Result;
        }
    #else
        #error "No __rdtsc implementation"
    #endif
#endif // __APPLE__

// TODO: No need to expose the TimerInfo struct to the caller. Improve API.

struct TimerInfo
{
    uint64 StartCycles, StopCycles, ElapsedCycles;
    double StartMs, StopMs, ElapsedMs;
};

#define FOREACH_TIMER(TIMER)    \
        TIMER(Startup)          \
        TIMER(Frame)            \
        TIMER(Sleep)            \
        TIMER(ImageOpen)        \
        TIMER(GetUndoImage)     \
        TIMER(COUNT)            \

#define GENERATE_ENUM(ENUM) Timer_##ENUM,
#define GENERATE_STRING(STRING) #STRING,

enum Timer_ {
    FOREACH_TIMER(GENERATE_ENUM)
};

static const char* TimerNames[] = {
    FOREACH_TIMER(GENERATE_STRING)
};

#undef FOREACH_TIMER
#undef GENERATE_ENUM
#undef GENERATE_STRING

namespace Timer
{
    void Init(double TickFrequency);
    double GetFrequency();
    void StartTime(TimerInfo* Timer);
    void StopTime(TimerInfo* Timer);
}

#endif // TIMER_H

// =======================================================================================

#ifdef TIMER_IMPLEMENTATION

#include "papaya_platform.h" // TODO: Remove this dependency

double TickFrequency;

void Timer::Init(double _TickFrequency)
{
    TickFrequency = _TickFrequency;
}

double Timer::GetFrequency()
{
    return TickFrequency;
}

void Timer::StartTime(TimerInfo* Timer)
{
    Timer->StartMs = platform::get_milliseconds();
    Timer->StartCycles = __rdtsc();
}

void Timer::StopTime(TimerInfo* Timer)
{
    Timer->StopCycles = __rdtsc();
    Timer->StopMs = platform::get_milliseconds();
    Timer->ElapsedCycles = Timer->StopCycles - Timer->StartCycles;
    Timer->ElapsedMs     = (Timer->StopMs - Timer->StartMs) * TickFrequency;
}

// void Timer::DisplayTimes(TimerInfo* Timers, int32 Count)
// {
//     ImGui::SetNextWindowPos(Vec2(0, ImGui::GetIO().DisplaySize.y - 75));
//     ImGui::SetNextWindowSize(Vec2(300, 75));
//     ImGui::Begin("Profiler");
//     for (int32 i = 0; i < Timer_COUNT; i++)
//     {
//         ImGui::Text("%d: Cycles: %lu, MS: %f", i, Mem->Debug.Timers[i].CyclesElapsed, Mem->Debug.Timers[i].ElapsedMs);
//     }
//     ImGui::End();
// }

#endif // TIMER_IMPLEMENTATION

