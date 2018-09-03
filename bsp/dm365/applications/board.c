/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author		Notes
 * 2010-11-13     weety		first version
 */


#include <rtthread.h>
#include <rthw.h>
#include <mmu.h>
#include "board.h"

/**
 * @addtogroup dm365
 */
/*@{*/
#if defined(__CC_ARM)
	extern int Image$$ER_ZI$$ZI$$Base;
	extern int Image$$ER_ZI$$ZI$$Length;
	extern int Image$$ER_ZI$$ZI$$Limit;
#elif (defined (__GNUC__))
	rt_uint8_t _irq_stack_start[1024];
	rt_uint8_t _fiq_stack_start[1024];
	rt_uint8_t _undefined_stack_start[512];
	rt_uint8_t _abort_stack_start[512];
	rt_uint8_t _svc_stack_start[1024] SECTION(".nobss");
	extern unsigned char __bss_start;
	extern unsigned char __bss_end;
#endif

extern void rt_hw_clock_init(void);
extern void rt_hw_uart_init(void);

static struct mem_desc dm365_mem_desc[] = {
	{ 0x80000000, 0x88000000-1, 0x80000000, SECT_RW_CB, 0, SECT_MAPPED },       /* 128M cached SDRAM memory */
	{ 0xA0000000, 0xA8000000-1, 0x80000000, SECT_RW_NCNB, 0, SECT_MAPPED },     /* 128M No cached SDRAM memory */
	{ 0xFFFF0000, 0xFFFF1000-1, 0x80000000, SECT_TO_PAGE, PAGE_RO_CB, PAGE_MAPPED }, /* isr vector table */
	{ 0x01C00000, 0x02000000-1, 0x01C00000, SECT_RW_NCNB, 0, SECT_MAPPED },       /* CFG BUS peripherals */
	{ 0x02000000, 0x0A000000-1, 0x02000000, SECT_RW_NCNB, 0, SECT_MAPPED },       /* AEMIF */
};
#define RT_USING_HRTIMER
#ifdef RT_USING_HRTIMER
#include <hrtimer.h>

static struct rt_clocksource_device clocksource_dm365;
static struct rt_clockevent_device  clockevent_dm365;
#endif

#ifndef RT_USING_HRTIMER
/**
 * This function will handle rtos timer
 */
void rt_timer_handler(int vector, void *param)
{
	rt_tick_increase();
}
#endif

#ifdef RT_USING_HRTIMER
/*
 * clocksource
 */
static void dm365_clocksource_init(void)
{
	/* timer0, input clocks 24MHz */
	volatile timer_regs_t *regs =
		(volatile timer_regs_t*)DAVINCI_TIMER0_BASE;//DAVINCI_TIMER0_BASE;

	psc_change_state(DAVINCI_DM365_LPSC_TIMER0, 3);

	/*disable timer*/
	regs->tcr &= ~(0x3UL << 6);

	//TIMMODE 32BIT UNCHAINED MODE
	regs->tgcr |=(0x1UL << 2);

	/*not in reset timer */
	regs->tgcr |= (0x1UL << 0);

	//regs->tgcr &= ~(0x1UL << 1);

	/* set Period Registers */
	regs->prd12 = 0xFFFFFFFF;
	regs->tim12 = 0;

	/* Set enable mode */
	regs->tcr |= (0x2UL << 6); //period mode

}

static rt_cycle_t read_cycles(struct rt_clocksource_device *cs)
{
	volatile timer_regs_t *regs =
		(volatile timer_regs_t*)DAVINCI_TIMER0_BASE;
	return (rt_cycle_t)regs->tim12;
}

rt_ktime_t masure_time[20];
static int idx = 0;

void rt_timer_handler(int vector, void *param)
{
	rt_ktime_t now = rt_clocksource_absolute_time();
	clockevent_dm365.event_handler(&clockevent_dm365);
	masure_time[idx] = rt_clocksource_absolute_time() - now;
	idx++;
	if (idx >= 20)
		idx = 0;
}

static void dm365_clockevent_init(void)
{
	/* timer0, input clocks 24MHz */
	volatile timer_regs_t *regs =
		(volatile timer_regs_t*)DAVINCI_TIMER1_BASE;

	psc_change_state(DAVINCI_DM365_LPSC_TIMER1, 3);

	/*disable timer*/
	regs->tcr &= ~(0x3UL << 6);

	//TIMMODE 32BIT UNCHAINED MODE
	regs->tgcr |=(0x1UL << 2);

	/*not in reset timer */
	regs->tgcr |= (0x1UL << 0);

	//regs->tgcr &= ~(0x1UL << 1);

	/* set Period Registers */
	regs->prd12 = 0xFFFFFFFF;
	regs->tim12 = 0;

	/* Set enable mode */
	regs->tcr |= (0x1UL << 6); //once mode
	

	/* install interrupt handler */
	rt_hw_interrupt_install(IRQ_DM365_TINT2, rt_timer_handler,
							RT_NULL, "timer1_12");//IRQ_DM365_TINT0_TINT12
	rt_hw_interrupt_umask(IRQ_DM365_TINT2);//IRQ_DM365_TINT2

}

int dm365_set_next_event(struct rt_clockevent_device * cs, unsigned long cycle)
{
	volatile timer_regs_t *regs =
		(volatile timer_regs_t*)DAVINCI_TIMER1_BASE;
	/*disable timer*/
	regs->tcr &= ~(0x3UL << 6);
	regs->tim12 = 0;
	regs->prd12 = cycle;
	/* Set enable mode */
	regs->tcr |= (0x1UL << 6); //once mode

	return 0;
}

#endif

/**
 * This function will init timer0 for system ticks
 */
 void rt_hw_timer_init()
 {
 #ifdef RT_USING_HRTIMER
	dm365_clocksource_init();
	clocksource_dm365.read = read_cycles;
	clocksource_dm365.mask = 0xFFFFFFFF;
	clocksource_dm365.mode = RT_CLOCKSOURCE_UP;
	rt_clocksource_device_register_khz(&clocksource_dm365, 24000000/1000);

	dm365_clockevent_init();
	clockevent_dm365.set_next_event = dm365_set_next_event;
	clockevent_dm365.min_delta_cycles = 1;
	clockevent_dm365.max_delta_cycles = 0xFFFFFFFE;
	rt_clockevent_device_register(&clockevent_dm365, 24000000);

	rt_systick_hrtimer_init();
#else
	/* timer0, input clocks 24MHz */
	volatile timer_regs_t *regs =
		(volatile timer_regs_t*)DAVINCI_TIMER1_BASE;//DAVINCI_TIMER0_BASE;

	psc_change_state(DAVINCI_DM365_LPSC_TIMER0, 3);
	psc_change_state(DAVINCI_DM365_LPSC_TIMER1, 3);

	/*disable timer*/
	regs->tcr &= ~(0x3UL << 6);

	//TIMMODE 32BIT UNCHAINED MODE
	regs->tgcr |=(0x1UL << 2);

	/*not in reset timer */
	regs->tgcr |= (0x1UL << 0);

	//regs->tgcr &= ~(0x1UL << 1);

	/* set Period Registers */
	regs->prd12 = 24000000/RT_TICK_PER_SECOND;
	regs->tim12 = 0;

	/* Set enable mode */
	regs->tcr |= (0x2UL << 6); //period mode
	

	/* install interrupt handler */
	rt_hw_interrupt_install(IRQ_DM365_TINT2, rt_timer_handler,
							RT_NULL, "timer1_12");//IRQ_DM365_TINT0_TINT12
	rt_hw_interrupt_umask(IRQ_DM365_TINT2);//IRQ_DM365_TINT2
#endif
 }

#define LSR_DR		0x01		/* Data ready */
#define LSR_THRE	0x20		/* Xmit holding register empty */
#define BPS			115200	/* serial baudrate */

typedef struct uartport
{
	volatile rt_uint32_t rbr;
	volatile rt_uint32_t ier;
	volatile rt_uint32_t fcr;
	volatile rt_uint32_t lcr;
	volatile rt_uint32_t mcr;
	volatile rt_uint32_t lsr;
	volatile rt_uint32_t msr;
	volatile rt_uint32_t scr;
	volatile rt_uint32_t dll;
	volatile rt_uint32_t dlh;
	
	volatile rt_uint32_t res[2];
	volatile rt_uint32_t pwremu_mgmt;
	volatile rt_uint32_t mdr;
}uartport;

#define thr rbr
#define iir fcr

#define UART0	((struct uartport *)DAVINCI_UART0_BASE)

static void davinci_uart_putc(char c)
{
    while (!(UART0->lsr & LSR_THRE));
	UART0->thr = c;
}

/**
 * This function is used to display a string on console, normally, it's
 * invoked by rt_kprintf
 *
 * @param str the displayed string
 */
void rt_hw_console_output(const char* str)
{
	while (*str)
	{
		if (*str=='\n')
		{
			davinci_uart_putc('\r');
		}

		davinci_uart_putc(*str++);
	}
}

static void rt_hw_console_init(void)
{
	rt_uint32_t divisor;
	
	divisor = (24000000 + (BPS * (16 / 2))) / (16 * BPS);
	UART0->ier = 0;
	UART0->lcr = 0x83; //8N1
	UART0->dll = 0;
	UART0->dlh = 0;
	UART0->lcr = 0x03;
	UART0->mcr = 0x03; //RTS,CTS
	UART0->fcr = 0x07; //FIFO
	UART0->lcr = 0x83;
	UART0->dll = divisor & 0xff;
	UART0->dlh = (divisor >> 8) & 0xff;
	UART0->lcr = 0x03;
	UART0->mdr = 0; //16x over-sampling
	UART0->pwremu_mgmt = 0x6000;
}

/**
 * This function will init dm365 board
 */
void rt_hw_board_init()
{
	/* initialize console */
	rt_hw_console_init();

	/* initialize mmu */
	rt_hw_mmu_init(dm365_mem_desc, sizeof(dm365_mem_desc)/sizeof(dm365_mem_desc[0]));

	/* initialize hardware interrupt */
	rt_hw_interrupt_init();

	/* initialize the system clock */
	rt_hw_clock_init();

	/* initialize heap memory system */
#ifdef __CC_ARM
	rt_system_heap_init((void*)&Image$$ER_ZI$$ZI$$Limit, (void*)0x88000000);
#else
	rt_system_heap_init((void*)&__bss_end, (void*)0x88000000);
#endif

	/* initialize early device */
#ifdef RT_USING_COMPONENTS_INIT
	rt_components_board_init();
#endif
#ifdef RT_USING_CONSOLE
	rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

	/* initialize timer0 */
	rt_hw_timer_init();

}

/*@}*/
