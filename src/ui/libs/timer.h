
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

struct Timer {
    uint64_t start_cycles, stop_cycles, elapsed_cycles;
    double start_ms, stop_ms, elapsed_ms;
};

#define TIMER_LIST \
    /* timer name */ \
    TME(Startup) \
    TME(Frame) \
    TME(Sleep) \
    TME(ImageOpen) \
    TME(GetUndoImage) \
    TME(COUNT) \
    /* end */

enum Timer_ {
#define TME(name) Timer_##name,
    TIMER_LIST
#undef TME
};

namespace timer {
    void init();
    double get_freq();
    double get_milliseconds();
    void start(Timer_ timer);
    void stop(Timer_ timer);
}

const char* get_timer_name(int idx);

extern Timer timers[Timer_COUNT];
#endif // TIMER_H

// =======================================================================================

#ifdef TIMER_IMPLEMENTATION

Timer timers[Timer_COUNT];
double tick_freq;

// TODO: Add things needed to implement rdtsc on Windows
#if defined(_WIN32)

    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>

#elif defined(__linux__)

    #include <x86intrin.h>
    #include <time.h>

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

void timer::init()
{
#if defined (__linux__)

    // TODO: Verify whether this is correct
    tick_freq = 1.0;

#elif defined (_WIN32)

    int64_t ticks_per_second;
    QueryPerformanceFrequency((LARGE_INTEGER *)&ticks_per_second);
    tick_freq = 1000.0 / ticks_per_second;

#endif
}

double timer::get_freq()
{
    return tick_freq;
}

double timer::get_milliseconds()
{
#if defined(__linux__)

    timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (double)(time.tv_sec * 1000 + time.tv_nsec / 1000000);

#elif defined(_WIN32)

    LARGE_INTEGER ms;
    QueryPerformanceCounter(&ms);
    return (double)ms.QuadPart;

#else
    #error "Clock millisecond API not implemented on this platform"
#endif
}


void timer::start(Timer_ timer)
{
    timers[timer].start_ms = get_milliseconds();
    timers[timer].start_cycles = __rdtsc();
}

void timer::stop(Timer_ timer)
{
    timers[timer].stop_cycles = __rdtsc();
    timers[timer].stop_ms = get_milliseconds();
    timers[timer].elapsed_cycles = timers[timer].stop_cycles -
                                   timers[timer].start_cycles;
    timers[timer].elapsed_ms = (timers[timer].stop_ms - timers[timer].start_ms) *
                               tick_freq;
}

static const char* timer_names[] = {
#define TME(name) #name,
    TIMER_LIST
#undef TME
};

const char* get_timer_name(int idx)
{
    return timer_names[idx];
}

#endif // TIMER_IMPLEMENTATION

