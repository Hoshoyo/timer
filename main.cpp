#if 0
#define TIMER_IMPLEMENTATION
#include "timer.h"
#include <stdio.h>

int main()
{
	Timer timer;
	timer_create(&timer, timer_s_to_ns(1));

	Timer s;
	timer_create(&s, timer_s_to_ns(1));

	while (true)
	{
		uint64_t ns_elapsed = timer_elapsed_ns(&s);
		if (timer_has_elapsed(&timer, timer_s_to_ns(1)))
		{
			printf("Elapsed %lld ns\n", ns_elapsed);
		}

		//Sleep(100);
		//Sleep(1000);
	}

	return 0;
}
#endif

#define TIMER_CALLBACK_IMPLEMENTATION
#include "timer_cb.h"
#include <stdio.h>

uint64_t
timer_ms_to_ns(uint64_t ms)
{
    return ms * 1000 * 1000;
}

typedef struct {
    int count;
} Timer_Data;

void
on_timer(Timer_Callback* timer)
{
    printf("Timer called %s\n", timer->name);

    Timer_Data* data = (Timer_Data*)timer->data;
    data->count++;

    // After 3 times, reduce the timer interval in half
    if (data->count == 3)
    {
        timer_cb_set_interval(timer, timer_ms_to_ns(500));
    }
}

int main()
{
    Timer_Data data = { 0 };
    Timer_Callback timer;
    timer.data = &data;

    timer_cb_create(&timer, "teste", timer_ms_to_ns(1000), on_timer);

    while (true)
    {
#if 1 // uncomment this to print the time until the next timer in nanoseconds
        uint64_t next_ns = timer_cb_time_until_next(&timer);
        printf("Time until next (ns): %lld\n", next_ns);
#endif
#if defined(__linux__)
        usleep(1000 * 10);
#else
        MSG msg = { 0 };
        while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        Sleep(10);
#endif
    }

    return 0;
}