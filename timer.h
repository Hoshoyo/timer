#pragma once
#include <stdint.h>

#if !defined(__cplusplus)
typedef int8_t bool
#define true 1
#define false 0
#endif

typedef enum {
	TIMER_FLAG_RESET_WHEN_CHECKING = (1 << 0), // The timer will reset the start time when the timer_check/timer_has_elapsed function is called
	TIMER_FLAG_ADJUST_AFTER_RESET = (1 << 1), // The timer will discount the elapsed time over the interval after a reset
} Timer_Flags;

typedef struct {
	uint64_t frequency;
	uint64_t start_time;
	uint64_t interval_ns;
	uint8_t  flags;
} Timer;

uint64_t timer_clock_now(Timer* timer);
void     timer_create(Timer* timer, uint64_t interval_ns);
bool     timer_has_elapsed(Timer* timer, uint64_t interval);
bool     timer_check(Timer* timer);
void     timer_reset(Timer* timer);
uint64_t timer_elapsed_ns(Timer* timer);

// Conversions
uint64_t timer_ms_to_ns(uint64_t ms);
uint64_t timer_us_to_ns(uint64_t us);
uint64_t timer_s_to_ns(uint64_t s);
double   timer_ns_to_ms(uint64_t ns);
double   timer_ns_to_us(uint64_t ns);
double   timer_ns_to_s(uint64_t ns);

#ifdef TIMER_IMPLEMENTATION

#ifdef _DEBUG
#include <assert.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#define NOGDI             // All GDI defines and routines
#define NOUSER            // All USER defines and routines
#include <windows.h>

void
os_timer_init(Timer* timer)
{
	LARGE_INTEGER li = { 0 };
	QueryPerformanceFrequency(&li);
	timer->frequency = li.QuadPart;
}

uint64_t
timer_clock_now(Timer* timer)
{
	LARGE_INTEGER li = { 0 };
	QueryPerformanceCounter(&li);
	return li.QuadPart * 1000000000;
}

#elif defined(__linux__)
#include <time.h>
void
os_timer_init(Timer* timer)
{
	timer->frequency = 1;
}

uint64_t
timer_clock_now(Timer* timer)
{
	struct timespec t_spec;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t_spec);
	return t_spec.tv_nsec + 1000000000 * t_spec.tv_sec;
}
#endif
#include <stdio.h>

void
timer_create(Timer* timer, uint64_t interval_ns)
{
	os_timer_init(timer);
	timer->start_time = timer_clock_now(timer);
	timer->interval_ns = interval_ns;
	timer->flags = TIMER_FLAG_RESET_WHEN_CHECKING | TIMER_FLAG_ADJUST_AFTER_RESET;
}

bool
timer_has_elapsed(Timer* timer, uint64_t interval)
{
	uint64_t now = timer_clock_now(timer);

#ifdef _DEBUG
	// This needs to be true in order to perform the subtraction,
	// otherwise the start time is in the future (multithread problem?)
	assert(now >= timer->start_time);
#endif

	uint64_t elapsed_ns = (now - timer->start_time) / timer->frequency;
	bool result = elapsed_ns > interval;

	if (result && timer->flags & TIMER_FLAG_RESET_WHEN_CHECKING)
	{
		timer->start_time = now;
		if (timer->flags & TIMER_FLAG_ADJUST_AFTER_RESET)
			timer->start_time -= (elapsed_ns - interval);
	}
	return result;
}

bool
timer_check(Timer* timer)
{
	return timer_has_elapsed(timer, timer->interval_ns);
}

void
timer_reset(Timer* timer)
{
	timer->start_time = timer_clock_now(timer);
}

uint64_t
timer_elapsed_ns(Timer* timer)
{
	uint64_t now = timer_clock_now(timer);
#ifdef _DEBUG
	// This needs to be true in order to perform the subtraction,
	// otherwise the start time is in the future (multithread problem?)
	assert(now >= timer->start_time);
#endif

	return (now - timer->start_time) / timer->frequency;
}

uint64_t
timer_ms_to_ns(uint64_t ms)
{
	return ms * 1000 * 1000;
}

uint64_t
timer_us_to_ns(uint64_t us)
{
	return us * 1000;
}

uint64_t
timer_s_to_ns(uint64_t s)
{
	return s * 1000 * 1000 * 1000;
}

double
timer_ns_to_ms(uint64_t ns)
{
	return ns / (1000.0 * 1000.0);
}

double
timer_ns_to_us(uint64_t ns)
{
	return ns / 1000.0;
}

double
timer_ns_to_s(uint64_t ns)
{
	return ns / (1000.0 * 1000.0 * 1000.0);
}

#endif // TIMER_IMPLEMENTATION

#if !defined(__cplusplus)
#undef true
#undef false
#endif