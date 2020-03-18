/*
 *
 * collect stuff from ti SDK
 *
 */
#ifndef cc2652rb_H_
#define cc2652rb_H_
#include <ti/inc/hw_ccfg_simple_struct.h>
#include <ti/inc/hw_ints.h>
#include <ti/inc/hw_memmap.h>

#include <ti/driverlib/vims.h>
#include <ti/driverlib/flash.h>
#include <ti/driverlib/prcm.h>
#include <ti/driverlib/osc.h>
#include <ti/driverlib/gpio.h>
#include <ti/driverlib/ioc.h>
#include <ti/driverlib/uart.h>
#include <ti/driverlib/aon_event.h>
#include <ti/driverlib/aon_rtc.h>
#include <ti/driverlib/sys_ctrl.h>
#include <ti/driverlib/trng.h>
#include <ti/driverlib/setup.h>

//
// are these correct?
//
#define __CM4_REV                 0x0001            /*!< Cortex-M4 Core Revision                                               */
#define __MPU_PRESENT                  1            /*!< MPU present or not                                                    */
#define __NVIC_PRIO_BITS               3            /*!< Number of Bits used for Priority Levels                               */
#define __Vendor_SysTickConfig         0            /*!< Set to 1 if different SysTick Config is used                          */
#define __FPU_PRESENT                  1            /*!< FPU present or not                                                    */


#define HIGHEST_INTERRUPT_PRIORITY			0
#define HIGH_INTERRUPT_PRIORITY				1
#define NORMAL_INTERRUPT_PRIORITY			2
#define LOW_INTERRUPT_PRIORITY				3
#define LOWEST_INTERRUPT_PRIORITY			7
#define EVENT_LOOP_INTERRUPT_PRIORITY		LOWEST_INTERRUPT_PRIORITY

#ifdef IMPLEMENT_IO_CPU
//-----------------------------------------------------------------------------
//
// cc2652 Implementtaion
//
//-----------------------------------------------------------------------------
#undef ASSERT
#define ASSERT(cond)

#include <ti/driverlib/aux_sysif.c>
#include <ti/driverlib/chipinfo.c>
#include <ti/driverlib/flash.c>
#include <ti/driverlib/setup.c>

#undef ASSERT
#endif // IMPLEMENT_IO_CPU
#endif
