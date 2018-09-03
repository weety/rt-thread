/*
 * hrtimer test
 */

#include <rtdevice.h>
#include <hrtimer.h>

static struct gpio_hrtimer
{
	struct rt_hrtimer gpio_timer;
	rt_ktime_t period;
	rt_base_t pin;
	int indx;
	rt_ktime_t time_deadline[10];
	rt_ktime_t time_invok[10];
	rt_ktime_t time_use[10];
};
static struct gpio_hrtimer gpio1_timer;
static struct gpio_hrtimer gpio2_timer;

static enum rt_hrtimer_restart rt_gpio_hrtimer_handler(void *param)
{
	int ret = 0;
	rt_ktime_t now = rt_clocksource_absolute_time();
	struct gpio_hrtimer *timer = (struct gpio_hrtimer *)param;
	//rt_pin_write(timer->pin, rt_pin_read(timer->pin) ^ 0x1);
	while ((rt_clocksource_absolute_time() - now) < 5000);
	ret = rt_hrtimer_forward_now(&timer->gpio_timer, timer->period);
	if (timer->indx < 10)
	{
		timer->time_deadline[timer->indx] = timer->gpio_timer.deadline;
		timer->time_invok[timer->indx] = rt_clocksource_absolute_time();
		timer->time_use[timer->indx] = rt_clocksource_absolute_time() - now;
		timer->indx++;
	}
	//if (ret > 1)
	//	rt_kprintf("overrun=%d\n", ret);
	return RT_HRTIMER_RESTART;
}


int hrtimer_test(rt_bool_t start, long gpio1_ns, long gpio2_ns)
{
	if (start)
	{
		rt_kprintf("hrtimer test start, gpio1ns=%d, gpio2ns=%d\n", gpio1_ns, gpio2_ns);
		gpio1_timer.period = gpio1_ns;
		gpio1_timer.pin = 80;
		gpio1_timer.indx = 0;
		gpio2_timer.period = gpio2_ns;
		gpio2_timer.pin = 83;
		gpio2_timer.indx = 0;

		//rt_pin_mode(gpio1_timer.pin, PIN_MODE_OUTPUT);
		//rt_pin_mode(gpio2_timer.pin, PIN_MODE_OUTPUT);
		
		rt_hrtimer_init(&gpio1_timer.gpio_timer, 
			rt_gpio_hrtimer_handler, 
			&gpio1_timer, 
			gpio1_ns, 
			RT_HRTIMER_MODE_REL);
		rt_hrtimer_start(&gpio1_timer.gpio_timer, 
			gpio1_ns, 
			RT_HRTIMER_MODE_REL);

		rt_hrtimer_init(&gpio2_timer.gpio_timer, 
			rt_gpio_hrtimer_handler, 
			&gpio2_timer, 
			gpio2_ns, 
			RT_HRTIMER_MODE_REL);
		rt_hrtimer_start(&gpio2_timer.gpio_timer, 
			gpio2_ns, 
			RT_HRTIMER_MODE_REL);
	}
	else
	{
		rt_kprintf("hrtimer test stop\n");
		rt_hrtimer_cancel(&gpio1_timer.gpio_timer);
		rt_hrtimer_cancel(&gpio2_timer.gpio_timer);
	}

    return 0;
}
#ifdef RT_USING_FINSH
#include <finsh.h>

FINSH_FUNCTION_EXPORT(hrtimer_test, test hrtimer);

static void usage(void)
{
	rt_kprintf("Please use: hrtimer_test <start/stop> gpio1_ns gpio2_ns\n");
}

int cmd_hrtimer_test(int argc, char **argv)
{
	rt_bool_t start;
    if (argc != 4)
    {
        usage();
    }
    else
    {
    	if (!rt_strncmp(argv[1], "start", 5))
			start = RT_TRUE;
		else if (!rt_strncmp(argv[1], "stop", 4))
			start = RT_FALSE;
		else
			usage();
        hrtimer_test(start, strtoul(argv[2], RT_NULL, RT_NULL), strtoul(argv[3], RT_NULL, RT_NULL));
    }

    return 0;
}

FINSH_FUNCTION_EXPORT_ALIAS(cmd_hrtimer_test, __cmd_hrtimer_test, hrtimer test);


int cmd_hrtimer_dump(int argc, char **argv)
{
	int indx = 0;

	printf("gpio1_timer\n");
	for (indx = 0; indx < 10; indx++)
	{
		printf("%lld, %lld, %lld\n", gpio1_timer.time_deadline[indx], gpio1_timer.time_invok[indx], gpio1_timer.time_use[indx]);
	}

	printf("gpio2_timer\n");
	for (indx = 0; indx < 10; indx++)
	{
		printf("%lld, %lld, %lld\n", gpio2_timer.time_deadline[indx], gpio2_timer.time_invok[indx], gpio2_timer.time_use[indx]);
	}
}

FINSH_FUNCTION_EXPORT_ALIAS(cmd_hrtimer_dump, __cmd_hrtimer_dump, hrtimer dump);

extern rt_ktime_t masure_time[20];
int cmd_masure_dump(int argc, char **argv)
{
	int indx = 0;

	printf("masure_timer\n");
	for (indx = 0; indx < 20; indx++)
	{
		printf("%lld\n", masure_time[indx]);
	}
}

FINSH_FUNCTION_EXPORT_ALIAS(cmd_masure_dump, __cmd_masure_dump, masure dump);

int cmd_clocksource_test(int argc, char **argv)
{
	int value;
	rt_ktime_t now = 0, last = 0;

	printf("masure_timer\n");
	for (now = rt_clocksource_absolute_time();  (rt_clocksource_absolute_time() - now) < 10000000000; )
	{
		last = rt_clocksource_absolute_time();
		//value = rt_pin_read(83);
		if (rt_clocksource_absolute_time() <= last)
			printf("clocksource hang detect, %d\n", value);
	}
	printf("clocksource test pass\n");

	return value;
}

FINSH_FUNCTION_EXPORT_ALIAS(cmd_clocksource_test, __cmd_clocksource_test, test clocksource);

#endif
