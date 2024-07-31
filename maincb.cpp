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
    if(data->count == 3)
    {
       timer_cb_set_interval(timer, timer_ms_to_ns(500));
    }
}

int main()
{
    Timer_Data data = {0};
    Timer_Callback timer;
    timer.data = &data;

    timer_cb_create(&timer, "teste", timer_ms_to_ns(1000), on_timer);

    while(true)
    {
        #if 0 // uncomment this to print the time until the next timer in nanoseconds
        uint64_t next_ns = timer_cb_time_until_next(&timer);
        printf("Time until next (ns): %lld\n", next_ns);
        #endif

        //sleep(1);
        usleep(1000 * 100);
    }

    return 0;
}