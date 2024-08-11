#pragma once
#include <stdint.h>

#define NANOSECONDS_IN_SECOND (1000 * 1000 * 1000)

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#define NOGDI             // All GDI defines and routines
#include <windows.h>
typedef UINT_PTR timer_t;
#endif

typedef struct Timer_Callback_t {
    const char* name;
    timer_t     id;
    uint64_t    interval_ns;
    void      (*on_timer)(struct Timer_Callback_t*);
    void*       data;

#if defined(_WIN32) || defined(_WIN64)
    DWORD last_trigger_time;
#endif
} Timer_Callback;

int      timer_cb_create(Timer_Callback* timer, const char* name, uint64_t interval_ns, void(*on_timer)(Timer_Callback*));
int      timer_cb_delete(Timer_Callback* timer);
int      timer_cb_set_interval(Timer_Callback* timer, uint64_t interval_ns);
int      timer_cb_reset(Timer_Callback* timer);
uint64_t timer_cb_time_until_next(Timer_Callback* timer);

#if defined(TIMER_CALLBACK_IMPLEMENTATION)

#if defined(_WIN32) || defined(_WIN64)
static volatile HWND internal_global_timer_handle;

void 
internal_timer_proc(HWND window, UINT message, UINT_PTR id, DWORD system_time)
{
    Timer_Callback* timer = (Timer_Callback*)id;
    timer->last_trigger_time = system_time;
    timer->on_timer(timer);
}

LRESULT
internal_message_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
    return DefWindowProcA(window, msg, wparam, lparam);
}

int
timer_cb_create(Timer_Callback* timer, const char* name, uint64_t interval_ns, void(*on_timer)(Timer_Callback*))
{
    if (!internal_global_timer_handle)
    {
        static const char* class_name = "Timer_Class";
        WNDCLASSEXA wx = {};
        wx.cbSize = sizeof(WNDCLASSEX);
        wx.lpfnWndProc = internal_message_proc;
        wx.hInstance = 0;
        wx.lpszClassName = class_name;
        if (RegisterClassExA(&wx))
        {
            HWND window = CreateWindowExA(0, class_name, "timer_window", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
            if (window != INVALID_HANDLE_VALUE)
            {
                uint64_t result = InterlockedCompareExchange((volatile uint64_t*)&internal_global_timer_handle, (uint64_t)window, 0);
                if (result != 0)
                {
                    // Already initialized
                    DestroyWindow(window);
                }
            }
            else
                return -1; // Could not create a window to receive timer callbacks
        }
        else
            return -1; // Could not register a class to create a window
    }

    timer->on_timer = on_timer;
    timer->name = name;
    timer->last_trigger_time = GetTickCount();
    timer->interval_ns = interval_ns;

    UINT_PTR timer_id = SetTimer(internal_global_timer_handle, (UINT_PTR)timer, (UINT)(interval_ns / 1000000), internal_timer_proc);
    timer->id = timer_id;
    return 0;
}

int
timer_cb_delete(Timer_Callback* timer)
{
    if (KillTimer(internal_global_timer_handle, timer->id))
        return 0;
    else
        return -1;
}

int
timer_cb_set_interval(Timer_Callback* timer, uint64_t interval_ns)
{
    if (SetTimer(internal_global_timer_handle, (UINT_PTR)timer, (UINT)(interval_ns / 1000000), internal_timer_proc))
        return 0;
    return -1;
}

int
timer_cb_reset(Timer_Callback* timer)
{
    return timer_cb_set_interval(timer, timer->interval_ns);
}

uint64_t
timer_cb_time_until_next(Timer_Callback* timer)
{    
    uint64_t result = (GetTickCount64() - (uint64_t)timer->last_trigger_time) * 1000000;
    return timer->interval_ns - result;
}

#elif defined(__linux__)

#include <time.h>

static void
timer_cb_internal_handler(int sig, siginfo_t* si, void* uc)
{
    Timer_Callback* timer = (Timer_Callback*)si->_sifields._rt.si_sigval.sival_ptr;
    timer->on_timer(timer);
}

int
timer_cb_set_interval(Timer_Callback* timer, uint64_t interval_ns)
{
    struct itimerspec its = { 0 };
    its.it_value.tv_sec = interval_ns / NANOSECONDS_IN_SECOND;
    its.it_value.tv_nsec = interval_ns % NANOSECONDS_IN_SECOND;
    its.it_interval.tv_sec = interval_ns / NANOSECONDS_IN_SECOND;
    its.it_interval.tv_nsec = interval_ns % NANOSECONDS_IN_SECOND;

    int result = timer_settime(timer->id, 0, &its, 0);
    if (result != 0) return -1;

    timer->interval_ns = interval_ns;
    return 0;
}

int
timer_cb_create(Timer_Callback* timer, const char* name, uint64_t interval_ns, void(*on_timer)(Timer_Callback*))
{
    timer->on_timer = on_timer;
    timer->name = name;

    struct sigevent sev = { 0 };
    sev.sigev_notify = SIGEV_SIGNAL | SIGEV_THREAD_ID;
    sev.sigev_signo = SIGRTMIN;
    sev._sigev_un._tid = gettid();
    sev.sigev_value.sival_ptr = timer;

    int result = timer_create(CLOCK_REALTIME, &sev, &timer->id);

    if (result != 0) return -1;

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
    struct itimerspec its = { 0 };
    timer_gettime(timer->id, &its);
    return its.it_value.tv_nsec + its.it_value.tv_sec * NANOSECONDS_IN_SECOND;
}

int
timer_cb_reset(Timer_Callback* timer)
{
    timer_cb_set_interval(timer, timer->interval_ns);
    return 0;
}

int
timer_cb_delete(Timer_Callback* timer)
{
    return timer_delete(timer->id);
}
#endif

#endif // TIMER_CALLBACK_IMPLEMENTATION
