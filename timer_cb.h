#pragma once
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

#define NANOSECONDS_IN_SECOND (1000 * 1000 * 1000)

typedef struct Timer_Callback_t {
    const char* name;
    timer_t     id;
    uint64_t    interval_ns;
    void      (*on_timer)(struct Timer_Callback_t*);
    void*       data;
} Timer_Callback;

int      timer_cb_create(Timer_Callback* timer, const char* name, uint64_t interval_ns, void(*on_timer)(Timer_Callback*));
int      timer_cb_delete(Timer_Callback* timer);
int      timer_cb_set_interval(Timer_Callback* timer, uint64_t interval_ns);
int      timer_cb_reset(Timer_Callback* timer);
uint64_t timer_cb_time_until_next(Timer_Callback* timer);

static void
timer_cb_internal_handler(int sig, siginfo_t *si, void *uc)
{
    Timer_Callback* timer = (Timer_Callback*)si->_sifields._rt.si_sigval.sival_ptr;
    timer->on_timer(timer);
}

int
timer_cb_set_interval(Timer_Callback* timer, uint64_t interval_ns)
{
    struct itimerspec its = {0};
    its.it_value.tv_sec = interval_ns / NANOSECONDS_IN_SECOND;
    its.it_value.tv_nsec = interval_ns % NANOSECONDS_IN_SECOND;
    its.it_interval.tv_sec = interval_ns / NANOSECONDS_IN_SECOND;
    its.it_interval.tv_nsec = interval_ns % NANOSECONDS_IN_SECOND;

    int result = timer_settime(timer->id, 0, &its, 0);
    if(result != 0) return -1;

    timer->interval_ns = interval_ns;
    return 0;
}

int
timer_cb_create(Timer_Callback* timer, const char* name, uint64_t interval_ns, void(*on_timer)(Timer_Callback*))
{
    timer->on_timer = on_timer;
    timer->name = name;

    struct sigevent sev = { 0 };
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = timer;

    int result = timer_create(CLOCK_REALTIME, &sev, &timer->id);

    if(result != 0) return -1;

    struct sigaction sa = { 0 };
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_cb_internal_handler;

    sigemptyset(&sa.sa_mask);

    result = sigaction(SIGRTMIN, &sa, 0);
    if (result == -1) return -1;

    return timer_cb_set_interval(timer, interval_ns);
}

uint64_t
timer_cb_time_until_next(Timer_Callback* timer)
{
    struct itimerspec its = {0};
    timer_gettime(timer->id, &its);
    return its.it_value.tv_nsec + its.it_value.tv_sec * NANOSECONDS_IN_SECOND;
}

int
timer_cb_reset(Timer_Callback* timer)
{
    timer_cb_set_interval(timer, (uint64_t)NANOSECONDS_IN_SECOND * 5);
    return 0;
}

int
timer_cb_delete(Timer_Callback* timer)
{
    return timer_delete(timer->id);
}