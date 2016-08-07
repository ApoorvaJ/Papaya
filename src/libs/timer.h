
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

// TODO: No need to expose the Timer struct to the caller. Improve API.

struct Timer {
    uint64 start_cycles, stop_cycles, elapsed_cycles;
    double start_ms, stop_ms, elapsed_ms;
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

namespace timer {
    void init(double freq);
    double get_freq();
    void start(Timer* timer);
    void stop(Timer* timer);
}

#endif // TIMER_H

// =======================================================================================

#ifdef TIMER_IMPLEMENTATION

#include "papaya_platform.h" // TODO: Remove this dependency

double tick_freq;

void timer::init(double _tick_freq) {
    tick_freq = _tick_freq;
}

double timer::get_freq() {
    return tick_freq;
}

void timer::start(Timer* timer) {
    timer->start_ms = platform::get_milliseconds();
    timer->start_cycles = __rdtsc();
}

void timer::stop(Timer* timer) {
    timer->stop_cycles = __rdtsc();
    timer->stop_ms = platform::get_milliseconds();
    timer->elapsed_cycles = timer->stop_cycles - timer->start_cycles;
    timer->elapsed_ms = (timer->stop_ms - timer->start_ms) * tick_freq;
}

#endif // TIMER_IMPLEMENTATION

