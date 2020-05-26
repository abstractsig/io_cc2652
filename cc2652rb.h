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

//
// ti SDK does not use cmsis convention for interrupt numbering
//
typedef int32_t IRQn_Type;
#define SysTick_IRQn (INT_SYSTICK - 16)
#define CMSIS_IRQn(N)   ((int32_t)(N) - 16)

#define __CM4_REV                 0x0001
#define __MPU_PRESENT                  1
#define __NVIC_PRIO_BITS               3
#define __Vendor_SysTickConfig         0
#define __FPU_PRESENT                  1

#define NUMBER_OF_ARM_INTERRUPT_VECTORS 16L
#define NUMBER_OF_NRF_INTERRUPT_VECTORS NUM_INTERRUPTS
#define NUMBER_OF_INTERRUPT_VECTORS (NUMBER_OF_ARM_INTERRUPT_VECTORS + NUMBER_OF_NRF_INTERRUPT_VECTORS)

#define HIGHEST_INTERRUPT_PRIORITY          0
#define HIGH_INTERRUPT_PRIORITY             1
#define NORMAL_INTERRUPT_PRIORITY           2
#define LOW_INTERRUPT_PRIORITY              3
#define LOWEST_INTERRUPT_PRIORITY           ((1 << __NVIC_PRIO_BITS) - 1)
#define EVENT_LOOP_INTERRUPT_PRIORITY       LOWEST_INTERRUPT_PRIORITY


#include <core_cm4.h>
#include <ti/driverlib/trng.h>

#define ENABLE_INTERRUPTS   \
    do {    \
        __DMB();    \
        __enable_irq(); \
    } while (0)

#define DISABLE_INTERRUPTS  \
    do {    \
        __disable_irq();    \
        __DMB();    \
    } while (0)


#ifdef IMPLEMENT_IO_CPU
//-----------------------------------------------------------------------------
//
// implementation
//
//-----------------------------------------------------------------------------
#include <ti/driverlib/aux_sysif.c>
#include <ti/driverlib/chipinfo.c>
#include <ti/driverlib/flash.c>
#include <ti/driverlib/setup.c>
#include <ti/driverlib/setup_rom.c>
#include <ti/driverlib/osc.c>
#include <ti/driverlib/ddi.c>
#include <ti/driverlib/aon_batmon.c>
#include <ti/driverlib/aon_event.c>
#include <ti/driverlib/ioc.c>
#include <ti/driverlib/prcm.c>
#include <ti/driverlib/vims.c>
#include <ti/driverlib/uart.c>
#include <ti/driverlib/interrupt.c>
#include <ti/driverlib/rfc.c>
#include <ti/driverlib/sys_ctrl.c>
#include <ti/driverlib/cpu.c>

#endif /* IMPLEMENT_IO_CPU */
#endif
