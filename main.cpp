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