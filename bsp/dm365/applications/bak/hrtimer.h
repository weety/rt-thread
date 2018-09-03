/*
 * File      : hrtimer.h
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

#ifndef __HRTIMER_H__
#define __HRTIMER_H__

#include <rtthread.h>
#include <sys/time.h>
#include "rt_rbtree.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INT_MAX		((int)(~0U>>1))
#define INT_MIN		(-INT_MAX - 1)
#define UINT_MAX	(~0U)
#define LONG_MAX	((long)(~0UL>>1))
#define LONG_MIN	(-LONG_MAX - 1)
#define ULONG_MAX	(~0UL)
#define LLONG_MAX	(~0ULL>>1)
#define LLONG_MIN	(-LLONG_MAX - 1)
#define ULLONG_MAX	(~0ULL)

/* Parameters used to convert the timespec values: */
#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define FSEC_PER_SEC	1000000000000000LL

/*
 * Mode arguments of xxx_hrtimer functions:
 */
enum rt_hrtimer_mode
{
	RT_HRTIMER_MODE_ABS = 0x0,		/* Time value is absolute */
	RT_HRTIMER_MODE_REL = 0x1,		/* Time value is relative to now */
};

/*
 * Return values for the callback function
 */
enum rt_hrtimer_restart
{
	RT_HRTIMER_NORESTART,	/* Timer is not restarted */
	RT_HRTIMER_RESTART,	/* Timer must be restarted */
};

enum rt_clocksource_mode
{
	RT_CLOCKSOURCE_UP,    /* counter mode is up */
	RT_CLOCKSOURCE_DOWN,  /* counter mode is down */
};

typedef signed long long int64_t;
typedef unsigned long long uint64_t;

typedef uint64_t  rt_cycle_t;
typedef int64_t	rt_ktime_t;

struct rt_clocksource_device
{
	rt_uint32_t freq;
	/* read current counter value */
	rt_cycle_t (*read)(struct rt_clocksource_device *device);
	rt_cycle_t mask;
	rt_cycle_t cycle_last;
	rt_uint32_t  mult;
	rt_uint32_t  shift;
	rt_uint32_t  maxadj;
	enum rt_clocksource_mode mode;
	uint64_t max_idle_ns;
	uint64_t max_idle_ns_half;
	uint64_t max_cycles;
	
	rt_ktime_t time_base;
};

struct rt_clockevent_device
{
	void (*event_handler)(struct rt_clockevent_device *);
	int (*set_next_event)(struct rt_clockevent_device *, unsigned long cycle);
	unsigned long min_delta_cycles;
	unsigned long max_delta_cycles;
	unsigned long min_delta_ns;
	unsigned long max_delta_ns;
	rt_uint32_t  mult;
	rt_uint32_t  shift;
	//int  rate;
};

/**
 * Absolute time, in microsecond units.
 *
 * Absolute time is measured from some arbitrary epoch shortly after
 * system startup.  It should never wrap or go backwards.
 */
//typedef uint64_t	rt_ktime_t;

//typedef int64_t	rt_ktime_t;


/**
 * Callout function type.
 *
 * Note that callouts run in the timer interrupt context, so
 * they are serialised with respect to each other, and must not
 * block.
 */
//typedef enum rt_hrtimer_restart (* rt_hrtimer_callback)(void *arg);

/**
 * Callout record.
 */
typedef struct rt_hrtimer
{
	struct rt_rb_node  node;

	rt_ktime_t      deadline;
	unsigned long   state;
#define RT_HRTIMER_STATE_INACTIVE	0x00
#define RT_HRTIMER_STATE_ENQUEUED	0x01
#define RT_HRTIMER_STATE_CALLBACK	0x02
	enum rt_hrtimer_mode    mode;
	enum rt_hrtimer_restart (* function)(void *);
	void  *arg;
} *rt_hrtimer_t;

struct rt_hrtimer_head 
{
	struct rt_rb_root head;
	struct rt_hrtimer *next;
	struct rt_clockevent_device *event;
	struct rt_clocksource_device *cs;
};

#define RT_HRTIMER_IS_QUEUED(timer) ((timer)->state & RT_HRTIMER_STATE_ENQUEUED)
#define RT_HRTIMER_IS_CALLBACK_RUNNING(timer) ((timer)->state & RT_HRTIMER_STATE_CALLBACK)
#define RT_HRTIMER_IS_ACTIVE(timer) ((timer)->state != RT_HRTIMER_STATE_INACTIVE)


/**
 * Get absolute time.
 */
rt_ktime_t rt_clocksource_absolute_time(void);

/**
 * Convert a timespec to absolute time.
 */
rt_ktime_t rt_ts_to_abstime(struct timespec *ts);

/**
 * Convert absolute time to a timespec.
 */
void	rt_abstime_to_ts(struct timespec *ts, rt_ktime_t abstime);

/**
 * Compute the delta between a timestamp taken in the past
 * and now.
 *
 * This function is safe to use even if the timestamp is updated
 * by an interrupt during execution.
 */
rt_ktime_t rt_hrtimer_elapsed_time(const volatile rt_ktime_t *then);

/**
 * Store the absolute time in an interrupt-safe fashion.
 *
 * This function ensures that the timestamp cannot be seen half-written by an interrupt handler.
 */
rt_ktime_t rt_hrtimer_store_absolute_time(volatile rt_ktime_t *now);

/**
 * Forward the timer expiry so it will expire in the future.
 */
int rt_hrtimer_forward(struct rt_hrtimer *timer, rt_ktime_t now, rt_ktime_t interval);

/* Forward a hrtimer so it expires after the hrtimer's current now */
int rt_hrtimer_forward_now(struct rt_hrtimer *timer, rt_ktime_t interval);

int rt_hrtimer_start(struct rt_hrtimer *timer, rt_ktime_t time, enum rt_hrtimer_mode mode);

/**
 * Remove the entry from the callout list.
 */
int	rt_hrtimer_cancel(struct rt_hrtimer *timer);

/**
 * Initialise a rt_hrtimer structure
 */
void rt_hrtimer_init(struct rt_hrtimer *timer,
                 enum rt_hrtimer_restart (* function)(void *),
                 void *arg,
                 rt_ktime_t time,
                 enum rt_hrtimer_mode mode);

/*
 * delay a rt_hrtimer_every() periodic call by the given number of
 * microseconds. This should be called from within the callout to
 * cause the callout to be re-scheduled for a later time. The periodic
 * callouts will then continue from that new base time at the
 * previously specified period.
 */
void rt_hrtimer_delay(struct rt_hrtimer *entry, rt_ktime_t delay);

void rt_clockevent_device_register(struct rt_clockevent_device *event_device, rt_uint32_t freq);
void rt_clocksource_device_register_hz(struct rt_clocksource_device *cs, rt_uint32_t hz);
void rt_clocksource_device_register_khz(struct rt_clocksource_device *cs, rt_uint32_t khz);

void
rt_clocks_calc_mult_shift(rt_uint32_t *mult, rt_uint32_t *shift, rt_uint32_t from, rt_uint32_t to, rt_uint32_t maxsec);

static inline void
rt_clockevents_calc_mult_shift(struct rt_clockevent_device *event, rt_uint32_t freq, rt_uint32_t minsec)
{
	rt_clocks_calc_mult_shift(&event->mult, &event->shift, NSEC_PER_SEC,
					  freq, minsec);
}

static inline uint64_t rt_clocksource_cyc2ns(rt_cycle_t cycles, rt_uint32_t mult, rt_uint32_t shift)
{
	return ((uint64_t) cycles * mult) >> shift;
}

void rt_systick_hrtimer_init(void);

#ifdef __cplusplus
}
#endif

#endif
