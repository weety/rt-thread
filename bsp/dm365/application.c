/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author		Notes
 * 2007-11-20     Yi.Qiu		add rtgui application
 * 2008-6-28      Bernard		no rtgui init
 */

/**
 * @addtogroup dm365
 */
/*@{*/

#include <rtthread.h>
//#include <rtdevice.h>

#ifdef RT_USING_DFS
/* dfs init */
#include <dfs_init.h>
/* dfs filesystem:ELM FatFs filesystem init */
#include <dfs_elm.h>
/* dfs Filesystem APIs */
#include <dfs_fs.h>
#ifdef RT_USING_DFS_UFFS
/* dfs filesystem:UFFS filesystem init */
#include <dfs_uffs.h>
#endif
#endif

#if defined(RT_USING_DFS_DEVFS)
#include <devfs.h>
#endif

#ifdef RT_USING_SDIO
#include <drivers/mmcsd_core.h>

#endif

#ifdef RT_USING_LWIP
#include <netif/ethernetif.h>
//#include <arch/sys_arch_init.h>
//#include "macb.h"
#endif

#ifdef RT_USING_LED
#include "led.h"
#endif

#define RT_INIT_THREAD_STACK_SIZE (2*1024)

#ifdef RT_USING_DFS_ROMFS
#include <dfs_romfs.h>
#endif

void rt_init_thread_entry(void* parameter)
{
	platform_init();
	
/* Filesystem Initialization */
#ifdef RT_USING_DFS
	{
		/* init the device filesystem */
		dfs_init();

#if defined(RT_USING_DFS_ELMFAT)
		/* init the elm chan FatFs filesystam*/
		elm_init();
#endif

#if defined(RT_USING_DFS_ROMFS)
		dfs_romfs_init();
		if (dfs_mount(RT_NULL, "/rom", "rom", 0, &romfs_root) == 0)
		{
			rt_kprintf("ROM File System initialized!\n");
		}
		else
			rt_kprintf("ROM File System initialzation failed!\n");
#endif

#if defined(RT_USING_DFS_DEVFS)
		devfs_init();
		if (dfs_mount(RT_NULL, "/dev", "devfs", 0, 0) == 0)
			rt_kprintf("Device File System initialized!\n");
		else
			rt_kprintf("Device File System initialzation failed!\n");

		#ifdef RT_USING_NEWLIB
		/* init libc */
		libc_system_init("uart0");
		#endif
#endif

#if defined(RT_USING_DFS_UFFS)
	{
		/* init the uffs filesystem */
		dfs_uffs_init();

		/* mount flash device as flash directory */
		if(dfs_mount("nand0", "/nand0", "uffs", 0, 0) == 0)
			rt_kprintf("UFFS File System initialized!\n");
		else
			rt_kprintf("UFFS File System initialzation failed!\n");
	}
#endif

#ifdef RT_USING_SDIO
	rt_mmcsd_core_init();
	rt_mmcsd_blk_init();
	#if 1
	rt_hw_mmcsd_init();
	rt_thread_delay(RT_TICK_PER_SECOND*2);
	/* mount sd card fat partition 1 as root directory */
		if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)
		{
			rt_kprintf("File System initialized!\n");
		}
		else
			rt_kprintf("File System initialzation failed!%d\n", rt_get_errno());
	#endif
#endif
	}
#endif

#ifdef RT_USING_LWIP
	{
		/* register ethernetif device */
		eth_system_device_init();
		//rt_hw_macb_init();
		/* re-init device driver */
		rt_device_init_all();
		/* init lwip system */
		lwip_sys_init();
	}
#endif

#ifdef RT_USING_I2C
	{
		rt_i2c_core_init();
		davinci_i2c_init("I2C1");
		pcf8563_init("I2C1", 0x51);
	}
#endif

}


void rt_led_thread_entry(void* parameter)
{
	while(1)
	{
		/* light on leds for one second */
		rt_thread_delay(100);

		/* light off leds for one second */
		rt_thread_delay(100);
	}
}

int rt_application_init()
{
	rt_thread_t init_thread;
#ifdef RT_USING_LED
	rt_thread_t led_thread;
#endif

#if (RT_THREAD_PRIORITY_MAX == 32)
	init_thread = rt_thread_create("init",
								rt_init_thread_entry, RT_NULL,
								RT_INIT_THREAD_STACK_SIZE, 8, 20);
#ifdef RT_USING_LED
	led_thread = rt_thread_create("led",
								rt_led_thread_entry, RT_NULL,
								512, 20, 20);
#endif
#else
	init_thread = rt_thread_create("init",
								rt_init_thread_entry, RT_NULL,
								RT_INIT_THREAD_STACK_SIZE, 80, 20);
#ifdef RT_USING_LED
	led_thread = rt_thread_create("led",
								rt_led_thread_entry, RT_NULL,
								512, 200, 20);
#endif
#endif

	if (init_thread != RT_NULL)
		rt_thread_startup(init_thread);
#ifdef RT_USING_LED
	if(led_thread != RT_NULL)
		rt_thread_startup(led_thread);
#endif

	return 0;
}

/* NFSv3 Initialization */
#if defined(RT_USING_DFS) && defined(RT_USING_LWIP) && defined(RT_USING_DFS_NFS)
#include <dfs_nfs.h>
void nfs_start(void)
{
	nfs_init();

	if (dfs_mount(RT_NULL, "/nfs", "nfs", 0, RT_NFS_HOST_EXPORT) == 0)
	{
		rt_kprintf("NFSv3 File System initialized!\n");
	}
	else
		rt_kprintf("NFSv3 File System initialzation failed!\n");
}

#include "finsh.h"
FINSH_FUNCTION_EXPORT(nfs_start, start net filesystem);
#endif

/*@}*/
