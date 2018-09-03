/*
 * File      : hrtimer.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009 RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-11-21     weety      first implementation
 */

#include <rtdef.h>
#include "hrtimer.h"

#ifndef min
#define min(x,y) (x<y?x:y)
#endif
#ifndef max
#define max(x,y) (x<y?y:x)
#endif

static struct rt_hrtimer_head hrtimer_queue = {RT_RB_ROOT, RT_NULL, RT_NULL, RT_NULL};
/* update system abs time, avoid clocksource timer overrun */
static struct rt_hrtimer update_clock_timer;
static struct rt_hrtimer systick_timer;

/* timer-specific functions */
static void rt_hrtimer_handler(void);
static void rt_hrtimer_enqueue(struct rt_hrtimer *node);
static void rt_hrtimer_dequeue(struct rt_hrtimer *node);
static void rt_hrtimer_reschedule(void);
static void rt_hrtimer_invoke(void);

static void rt_hrtimer_insert(struct rt_hrtimer_head *head, struct rt_hrtimer *node)
{
  	struct rt_rb_node **new = &(head->head.rt_rb_node);
	struct rt_rb_node *parent = NULL;
	struct rt_hrtimer *this;

	/* Make sure we don't add nodes that are already added */
	RT_ASSERT(RT_RB_EMPTY_NODE(&node->node));

  	/* Figure out where to put new node */
  	while (*new)
	{
		parent = *new;
		this = rt_rb_entry(parent, struct rt_hrtimer, node);
		
  		if (node->deadline < this->deadline)
  			new = &((*new)->rt_rb_left);
  		else
  			new = &((*new)->rt_rb_right);
  	}

  	/* Add new node and rebalance tree. */
  	rt_rb_link_node(&node->node, parent, new);
  	rt_rb_insert_color(&node->node, &head->head);

	if (!head->next || node->deadline < head->next->deadline)
		head->next = node;

}

static void rt_hrtimer_remove(struct rt_hrtimer_head *head, struct rt_hrtimer *node)
{
	RT_ASSERT(!RT_RB_EMPTY_NODE(&node->node));

	/* update next pointer */
	if (head->next == node)
	{
		struct rt_rb_node *rbn = rt_rb_next(&node->node);

		head->next = rbn ?
			rt_rb_entry(rbn, struct rt_hrtimer, node) : RT_NULL;
	}
	rt_rb_erase(&node->node, &head->head);
	RT_RB_CLEAR_NODE(&node->node);
}


/**
 * Handle the compare interrupt by calling the callback dispatcher
 * and then re-scheduling the next deadline.
 */
static void rt_hrtimer_handler(void)
{
	/* run any callbacks that have met their deadline */
	rt_hrtimer_invoke();

	/* and schedule the next interrupt */
	rt_hrtimer_reschedule();
}


/**
 * Convert a timespec to absolute time
 */
rt_ktime_t rt_ts_to_abstime(struct timespec *ts)
{
	rt_ktime_t	result;

	result = (rt_ktime_t)(ts->tv_sec) * NSEC_PER_SEC;
	result += ts->tv_nsec;

	return result;
}

/**
 * Convert absolute time to a timespec.
 */
void rt_abstime_to_ts(struct timespec *ts, rt_ktime_t abstime)
{
	ts->tv_sec = abstime / NSEC_PER_SEC;
	abstime -= ts->tv_sec * NSEC_PER_SEC;
	ts->tv_nsec = abstime;
}

/**
 * Compare a time value with the current time.
 */
rt_ktime_t rt_hrtimer_elapsed_time(const volatile rt_ktime_t *then)
{
	rt_base_t flags = rt_hw_interrupt_disable();

	rt_ktime_t delta = rt_clocksource_absolute_time() - *then;

	rt_hw_interrupt_enable(flags);

	return delta;
}

/**
 * Store the absolute time in an interrupt-safe fashion
 */
rt_ktime_t rt_hrtimer_store_absolute_time(volatile rt_ktime_t *now)
{
	rt_base_t flags = rt_hw_interrupt_disable();

	rt_ktime_t ts = rt_clocksource_absolute_time();

	rt_hw_interrupt_enable(flags);

	return ts;
}


/**
 * rt_hrtimer_forward - forward the timer expiry
 * @timer:	hrtimer to forward
 * @now:	forward past this time
 * @interval:	the interval to forward
 *
 * Forward the timer expiry so it will expire in the future.
 * Returns the number of overruns.
 */
int rt_hrtimer_forward(struct rt_hrtimer *timer, rt_ktime_t now, rt_ktime_t interval)
{
	int overrun = 1;
	rt_ktime_t delta;

	delta = now - timer->deadline;

	if (delta < 0)
		return 0;

	if (delta >= interval)
	{
		overrun += (delta + interval - 1) / interval;
		timer->deadline = now + hrtimer_queue.event->min_delta_ns; /* FIX UP */
		return overrun;
	}

	if (timer->deadline <= now)
	{
		timer->deadline = timer->deadline + interval;
	}

	return overrun;
}

/* Forward a hrtimer so it expires after the hrtimer's current now */
int rt_hrtimer_forward_now(struct rt_hrtimer *timer, rt_ktime_t interval)
{
	return rt_hrtimer_forward(timer, rt_clocksource_absolute_time(), interval);
}

/**
 * rt_hrtimer_start - (re)start an hrtimer on the current CPU
 * @timer:	the timer to be added
 * @time:	expiry time, only for RELTIME MODE
 * @mode:	expiry mode: absolute (HRTIMER_MODE_ABS) or
 *		relative (HRTIMER_MODE_REL)
 *
 * Returns:
 *  0 on success
 *  1 when the timer was active
 * -1 failed
 */
int rt_hrtimer_start(struct rt_hrtimer *timer, rt_ktime_t time, enum rt_hrtimer_mode mode)
{
	int ret = 0;
	rt_ktime_t	now = rt_clocksource_absolute_time();
	if (timer->mode != mode)
		return -1;
	if ((timer->mode == RT_HRTIMER_MODE_ABS) && (time <= now))
		return -1;
	rt_base_t flags = rt_hw_interrupt_disable();

	/* if the entry is currently queued, remove it */
	/* note that we are using a potentially uninitialised
	   entry->link here, but it is safe as sq_rem() doesn't
	   dereference the passed node unless it is found in the
	   list. So we potentially waste a bit of time searching the
	   queue for the uninitialised entry->link but we don't do
	   anything actually unsafe.
	*/
	if (RT_HRTIMER_IS_QUEUED(timer))
	{
		rt_hrtimer_dequeue(timer);
		ret = 1;
	}

	if (timer->mode == RT_HRTIMER_MODE_REL)
	{
		timer->deadline = now + time;
	}
	else
	{
		timer->deadline = time;
	}

	rt_hrtimer_enqueue(timer);
	if (hrtimer_queue.next == timer)
		rt_hrtimer_reschedule();

	rt_hw_interrupt_enable(flags);

	return ret;
}

/**
 * rt_hrtimer_cancel - cancel a timer and wait for the handler to finish.
 * @timer:	the timer to be cancelled
 *
 * Returns:
 *  0 when the timer was not active
 *  1 when the timer was active
 */
int rt_hrtimer_cancel(struct rt_hrtimer *timer)
{
	rt_bool_t need_reschedule = RT_FALSE;
	if (!RT_HRTIMER_IS_QUEUED(timer))
		return 0;
	rt_base_t flags = rt_hw_interrupt_disable();

	if (hrtimer_queue.next == timer)
		need_reschedule = RT_TRUE;
	rt_hrtimer_dequeue(timer);
	if (need_reschedule)
		rt_hrtimer_reschedule();

	rt_hw_interrupt_enable(flags);

	return 1;
}

/**
 * rt_hrtimer_init - initialize a timer to the given clock
 * @timer:	the timer to be initialized
 * @mode:	timer mode abs/rel
 */
void rt_hrtimer_init(struct rt_hrtimer *timer,
                 enum rt_hrtimer_restart (* function)(void *),
                 void *arg,
                 rt_ktime_t time,
                 enum rt_hrtimer_mode mode)
{
	rt_memset(timer, 0, sizeof(struct rt_hrtimer));
	timer->function = function;
	timer->arg = arg;
	timer->mode = mode;
	if (timer->mode == RT_HRTIMER_MODE_REL)
	{
		timer->deadline = rt_clocksource_absolute_time() + time;
	}
	else
	{
		timer->deadline = time;
	}
	timer->state = RT_HRTIMER_STATE_INACTIVE;
	RT_RB_CLEAR_NODE(&timer->node);
}

static void rt_hrtimer_enqueue(struct rt_hrtimer *node)
{
	rt_hrtimer_insert(&hrtimer_queue, node);
	node->state |= RT_HRTIMER_STATE_ENQUEUED;
}

static void rt_hrtimer_dequeue(struct rt_hrtimer *node)
{
	rt_hrtimer_remove(&hrtimer_queue, node);
	node->state &= ~RT_HRTIMER_STATE_ENQUEUED;
}

static void rt_hrtimer_invoke(void)
{
	struct rt_hrtimer	*node;
	int ret = RT_HRTIMER_NORESTART;

	while (RT_TRUE)
	{
		/* get the current time */
		rt_ktime_t now = rt_clocksource_absolute_time();

		node = hrtimer_queue.next;

		if (node == RT_NULL)
		{
			break;
		}

		if (node->deadline > now)
		{
			break;
		}

		rt_hrtimer_dequeue(node);

		/* invoke the callback (if there is one) */
		if (node->function)
		{
			/* hrtimer_dbg("call %p: %p(%p)\n", call, call->callback, call->arg); */
			node->state |= RT_HRTIMER_STATE_CALLBACK;
			ret = node->function(node->arg);
			node->state &= ~ RT_HRTIMER_STATE_CALLBACK;
		}

		/* if the callback has a non-zero period, it has to be re-entered */
		if (ret == RT_HRTIMER_RESTART)
		{
			rt_hrtimer_enqueue(node);
		}
	}
}

/**
 * Reschedule the next timer interrupt.
 *
 * This routine must be called with interrupts disabled.
 */
static void rt_hrtimer_reschedule()
{
	struct rt_hrtimer *next = RT_NULL;
	struct rt_clockevent_device *event = hrtimer_queue.event;
	int64_t delta;
	rt_cycle_t cycle;

	next = hrtimer_queue.next;

	/*
	 * Determine what the next deadline will be.
	 *
	 * Note that we ensure that this will be within the counter
	 * period, so that when we truncate all but the low 16 bits
	 * the next time the compare matches it will be the deadline
	 * we want.
	 *
	 * It is important for accurate timekeeping that the compare
	 * interrupt fires sufficiently often that the base_time update in
	 * rt_clocksource_absolute_time runs at least once per timer period.
	 */
	if (next != RT_NULL)
	{
		delta = next->deadline - rt_clocksource_absolute_time();
		if (delta <= 0)
			delta = event->min_delta_ns;

		delta = min(delta, (int64_t) event->max_delta_ns);
		delta = max(delta, (int64_t) event->min_delta_ns);
		
		cycle = ((rt_cycle_t) delta * event->mult) >> event->shift;
		event->set_next_event(event, (unsigned long) cycle);
	}
	else
	{
		rt_hrtimer_init(&update_clock_timer, RT_NULL, RT_NULL, hrtimer_queue.cs->max_idle_ns, RT_HRTIMER_MODE_REL);
		rt_hrtimer_enqueue(&update_clock_timer);
	}

	//hrtimer_dbg("schedule for %u at %u\n", (unsigned)(deadline & 0xffffffff), (unsigned)(now & 0xffffffff));

}


/*void rt_hrtimer_delay(struct rt_hrtimer *entry, rt_ktime_t delay)
{
	entry->deadline = rt_clocksource_absolute_time() + delay;
}*/

/**
 * clocks_calc_mult_shift - calculate mult/shift factors for scaled math of clocks
 * @mult:	pointer to mult variable
 * @shift:	pointer to shift variable
 * @from:	frequency to convert from
 * @to:		frequency to convert to
 * @maxsec:	guaranteed runtime conversion range in seconds
 *
 * The function evaluates the shift/mult pair for the scaled math
 * operations of clocksources and clockevents.
 *
 * @to and @from are frequency values in HZ. For clock sources @to is
 * NSEC_PER_SEC == 1GHz and @from is the counter frequency. For clock
 * event @to is the counter frequency and @from is NSEC_PER_SEC.
 *
 * The @maxsec conversion range argument controls the time frame in
 * seconds which must be covered by the runtime conversion with the
 * calculated mult and shift factors. This guarantees that no 64bit
 * overflow happens when the input value of the conversion is
 * multiplied with the calculated mult factor. Larger ranges may
 * reduce the conversion accuracy by chosing smaller mult and shift
 * factors.
 */
void
rt_clocks_calc_mult_shift(rt_uint32_t *mult, rt_uint32_t *shift, rt_uint32_t from, rt_uint32_t to, rt_uint32_t maxsec)
{
	uint64_t tmp;
	rt_uint32_t sft, sftacc= 32;

	/*
	 * Calculate the shift factor which is limiting the conversion
	 * range:
	 */
	tmp = ((uint64_t)maxsec * from) >> 32;
	while (tmp)
	{
		tmp >>=1;
		sftacc--;
	}

	/*
	 * Find the conversion shift/mult pair which has the best
	 * accuracy and fits the maxsec conversion range:
	 */
	for (sft = 32; sft > 0; sft--)
	{
		tmp = (uint64_t) to << sft;
		tmp += from / 2;
		//do_div(tmp, from);
		tmp = tmp / from;
		if ((tmp >> sftacc) == 0)
			break;
	}
	*mult = tmp;
	*shift = sft;
}

/**
 * rt_clocksource_max_adjustment- Returns max adjustment amount
 * @cs:         Pointer to clocksource
 *
 */
static rt_uint32_t rt_clocksource_max_adjustment(struct rt_clocksource_device *cs)
{
	uint64_t ret;
	/*
	 * We won't try to correct for more than 11% adjustments (110,000 ppm),
	 */
	ret = (uint64_t)cs->mult * 11;
	//do_div(ret,100);
	ret = ret / 100;
	return (rt_uint32_t)ret;
}

/**
 * rt_clocks_calc_max_nsecs - Returns maximum nanoseconds that can be converted
 * @mult:	cycle to nanosecond multiplier
 * @shift:	cycle to nanosecond divisor (power of two)
 * @maxadj:	maximum adjustment value to mult (~11%)
 * @mask:	bitmask for two's complement subtraction of non 64 bit counters
 * @max_cyc:	maximum cycle value before potential overflow (does not include
 *		any safety margin)
 *
 * NOTE: This function includes a safety margin of 50%, in other words, we
 * return half the number of nanoseconds the hardware counter can technically
 * cover. This is done so that we can potentially detect problems caused by
 * delayed timers or bad hardware, which might result in time intervals that
 * are larger then what the math used can handle without overflows.
 */
uint64_t rt_clocks_calc_max_nsecs(rt_uint32_t mult, rt_uint32_t shift, rt_uint32_t maxadj, uint64_t mask, uint64_t *max_cyc)
{
	uint64_t max_nsecs, max_cycles;

	/*
	 * Calculate the maximum number of cycles that we can pass to the
	 * cyc2ns() function without overflowing a 64-bit result.
	 */
	max_cycles = ULLONG_MAX;
	//do_div(max_cycles, mult+maxadj);
	max_cycles = max_cycles / (mult + maxadj);

	/*
	 * The actual maximum number of cycles we can defer the clocksource is
	 * determined by the minimum of max_cycles and mask.
	 * Note: Here we subtract the maxadj to make sure we don't sleep for
	 * too long if there's a large negative adjustment.
	 */
	max_cycles = min(max_cycles, mask);
	max_nsecs = rt_clocksource_cyc2ns(max_cycles, mult - maxadj, shift);

	/* return the max_cycles value as well if requested */
	if (max_cyc)
		*max_cyc = max_cycles;

	/* Return 50% of the actual maximum, so we can detect bad values */
	max_nsecs >>= 1;

	return max_nsecs;
}

/**
 * __clocksource_updatefreq_scale - Used update clocksource with new freq
 * @cs:		clocksource to be registered
 * @scale:	Scale factor multiplied against freq to get clocksource hz
 * @freq:	clocksource frequency (cycles per second) divided by scale
 *
 * This should only be called from the clocksource->enable() method.
 *
 * This *SHOULD NOT* be called directly! Please use the
 * clocksource_updatefreq_hz() or clocksource_updatefreq_khz helper functions.
 */
void __rt_clocksource_updatefreq_scale(struct rt_clocksource_device *cs, rt_uint32_t scale, rt_uint32_t freq)
{
	uint64_t sec;
	/*
	 * Calc the maximum number of seconds which we can run before
	 * wrapping around. For clocksources which have a mask > 32bit
	 * we need to limit the max sleep time to have a good
	 * conversion precision. 10 minutes is still a reasonable
	 * amount. That results in a shift value of 24 for a
	 * clocksource with mask >= 40bit and f >= 4GHz. That maps to
	 * ~ 0.06ppm granularity for NTP. We apply the same 12.5%
	 * margin as we do in clocksource_max_deferment()
	 */
	sec = (cs->mask - (cs->mask >> 3));
	//do_div(sec, freq);
	//do_div(sec, scale);
	sec = sec / freq;
	sec = sec / scale;
	if (!sec)
		sec = 1;
	else if (sec > 600 && cs->mask > UINT_MAX)
		sec = 600;

	rt_clocks_calc_mult_shift(&cs->mult, &cs->shift, freq,
			       NSEC_PER_SEC / scale, sec * scale);

	/*
	 * for clocksources that have large mults, to avoid overflow.
	 * Since mult may be adjusted by ntp, add an safety extra margin
	 *
	 */
	cs->maxadj = rt_clocksource_max_adjustment(cs);
	while ((cs->mult + cs->maxadj < cs->mult)
		|| (cs->mult - cs->maxadj > cs->mult))
	{
		cs->mult >>= 1;
		cs->shift--;
		cs->maxadj = rt_clocksource_max_adjustment(cs);
	}

	cs->max_idle_ns = rt_clocks_calc_max_nsecs(cs->mult, cs->shift,
						cs->maxadj, cs->mask,
						&cs->max_cycles);
	cs->max_idle_ns_half = cs->max_idle_ns >> 1;
	rt_kprintf("clocksource: mask: 0x%llx max_cycles: 0x%llx, max_idle_ns: %lld ns\n",
			cs->mask, cs->max_cycles, cs->max_idle_ns);
}

static uint64_t _rt_clockevent_delta2ns(unsigned long cycles, struct rt_clockevent_device *event,
			rt_bool_t ismax)
{
	uint64_t ns = (uint64_t) cycles << event->shift;
	uint64_t fixed;

	RT_ASSERT(event->mult);
	fixed = (uint64_t) event->mult - 1;

	/*
	 * Upper bound sanity check. If the backwards conversion is
	 * not equal latch, we know that the above shift overflowed.
	 */
	if ((ns >> event->shift) != (uint64_t)cycles)
		ns = ~0ULL;

	/*
	 * Scaled math oddities:
	 *
	 * For mult <= (1 << shift) we can safely add mult - 1 to
	 * prevent integer rounding loss. So the backwards conversion
	 * from nsec to device ticks will be correct.
	 *
	 * For mult > (1 << shift), i.e. device frequency is > 1GHz we
	 * need to be careful. Adding mult - 1 will result in a value
	 * which when converted back to device ticks can be larger
	 * than latch by up to (mult - 1) >> shift. For the min_delta
	 * calculation we still want to apply this in order to stay
	 * above the minimum device ticks limit. For the upper limit
	 * we would end up with a latch value larger than the upper
	 * limit of the device, so we omit the add to stay below the
	 * device upper boundary.
	 *
	 * Also omit the add if it would overflow the u64 boundary.
	 */
	if ((~0ULL - ns > fixed) &&
	    (!ismax || event->mult <= (1ULL << event->shift)))
		ns += fixed;

	ns = ns / event->mult;

	/* Deltas less than 1usec are pointless noise */
	return ns > 1000 ? ns : 1000;
}

void rt_clockevents_config(struct rt_clockevent_device *dev, rt_uint32_t freq)
{
	uint64_t sec;

	/*
	 * Calculate the maximum number of seconds we can sleep. Limit
	 * to 10 minutes for hardware which can program more than
	 * 32bit ticks so we still get reasonable conversion values.
	 */
	sec = dev->max_delta_cycles;
	sec = sec / freq;
	if (!sec)
		sec = 1;
	else if (sec > 600 && dev->max_delta_cycles > UINT_MAX)
		sec = 600;

	rt_clockevents_calc_mult_shift(dev, freq, sec);
	dev->min_delta_ns = _rt_clockevent_delta2ns(dev->min_delta_cycles, dev, RT_FALSE);
	dev->max_delta_ns = _rt_clockevent_delta2ns(dev->max_delta_cycles, dev, RT_TRUE);
}

static void rt_clockevent_device_handler(struct rt_clockevent_device *dev)
{
	rt_hrtimer_handler();
}

void rt_clockevent_device_register(struct rt_clockevent_device *event_device, rt_uint32_t freq)
{
	hrtimer_queue.event = event_device;
	hrtimer_queue.event->event_handler = rt_clockevent_device_handler;
	rt_clockevents_config(event_device, freq);
}

void rt_clocksource_device_register_hz(struct rt_clocksource_device *cs, rt_uint32_t hz)
{
	hrtimer_queue.cs = cs;
	__rt_clocksource_updatefreq_scale(cs, 1, hz);
}

void rt_clocksource_device_register_khz(struct rt_clocksource_device *cs, rt_uint32_t khz)
{
	hrtimer_queue.cs = cs;
	__rt_clocksource_updatefreq_scale(cs, 1000, khz);
}

/**
 * Fetch a never-wrapping absolute time value in microseconds from
 * some arbitrary epoch shortly after system start.
 */
rt_ktime_t rt_clocksource_absolute_time(void)
{
	struct rt_clocksource_device *cs = hrtimer_queue.cs;
	rt_cycle_t cycle_now, cycle_delta;
	uint64_t ns_offset;
	volatile rt_ktime_t time_base;
	rt_base_t flags;

	/*
	 * Counter state.  Marked volatile as they may change
	 * inside this routine but outside the irqsave/restore
	 * pair.  Discourage the compiler from moving loads/stores
	 * to these outside of the protected range.
	 */

	/* prevent re-entry */
	flags = rt_hw_interrupt_disable();

	/* get the current counter value */
	cycle_now = cs->read(cs);

	if (cs->mode == RT_CLOCKSOURCE_UP)
		cycle_delta = (cycle_now - cs->cycle_last) & cs->mask;
	else
		cycle_delta = (cs->cycle_last - cycle_now) & cs->mask;
	ns_offset = rt_clocksource_cyc2ns(cycle_delta, cs->mult, cs->shift);

	time_base = cs->time_base;
	time_base += ns_offset;
	if (ns_offset > cs->max_idle_ns_half)
	{
		cs->cycle_last = cycle_now;	
		cs->time_base = time_base;
	}

	rt_hw_interrupt_enable(flags);

	return time_base;
}

static enum rt_hrtimer_restart rt_systick_hrtimer_handler(void *param)
{
	struct rt_hrtimer *timer = (struct rt_hrtimer *)param;
	rt_tick_increase();
	rt_hrtimer_forward_now(timer, NSEC_PER_SEC / RT_TICK_PER_SECOND);
	return RT_HRTIMER_RESTART;
}

void rt_systick_hrtimer_init(void)
{
	rt_hrtimer_init(&systick_timer, 
		rt_systick_hrtimer_handler, 
		&systick_timer, 
		NSEC_PER_SEC / RT_TICK_PER_SECOND, 
		RT_HRTIMER_MODE_REL);
	rt_hrtimer_start(&systick_timer, 
		NSEC_PER_SEC / RT_TICK_PER_SECOND, 
		RT_HRTIMER_MODE_REL);
}

