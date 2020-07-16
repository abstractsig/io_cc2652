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

//
// This started to help track down a volatile bug by creating a C
// alternative to the Ti HWREG macros.  It turned out that there
// was no problem but these structs are nice so I kept using them.
//
typedef struct {
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t RATIO:2;
			uint32_t :30;
		} bit;
	} INFRCLKDIVR;	// 0h Infrastructure Clock Division Factor For Run Mode Section 7.8.2.1
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t RATIO:2;
			uint32_t :30;
		} bit;
	} INFRCLKDIVS;//  4h Infrastructure Clock Division Factor For Sleep Mode Section 7.8.2.2
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t RATIO:2;
			uint32_t :30;
		} bit;
	} INFRCLKDIVDS;//  8h Infrastructure Clock Division Factor For DeepSleepMode Section 7.8.2.3
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} VDCTL;//  ch MCU Voltage Domain Control Section 7.8.2.4
	__IO uint32_t separator2[6];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} CLKLOADCTL;//  28h Load PRCM Settings To CLKCTRL Power Domain Section 7.8.2.5
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} RFCCLKG;//  2ch RFC Clock Gate Section 7.8.2.6
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} VIMSCLKG;//  30h VIMS Clock Gate Section 7.8.2.7
	__IO uint32_t separator3[2];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} SECDMACLKGR;// 3ch SEC (PKA And TRNG And CRYPTO) And UDMA Clock Gate For Run And All Modes Section 7.8.2.8
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} SECDMACLKGS;//  40h SEC (PKA And TRNG And CRYPTO) And UDMA Clock Gate For Sleep Mode Section 7.8.2.9
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} SECDMACLKGDS;//  44h SEC (PKA And TRNG and CRYPTO) And UDMA Clock Gate For Deep Sleep Mode Section 7.8.2.10
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} GPIOCLKGR;//  48h GPIO Clock Gate For Run And All Modes Section 7.8.2.11
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} GPIOCLKGS;//  4ch GPIO Clock Gate For Sleep Mode Section 7.8.2.12
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} GPIOCLKGDS;//  50h GPIO Clock Gate For Deep Sleep Mode Section 7.8.2.13
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} GPTCLKGR;//  54h GPT Clock Gate For Run And All Modes Section 7.8.2.14
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} GPTCLKGS;//  58h GPT Clock Gate For Sleep Mode Section 7.8.2.15
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} GPTCLKGDS;//  5ch GPT Clock Gate For Deep Sleep Mode Section 7.8.2.16
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} I2CCLKGR;//  60h I2C Clock Gate For Run And All Modes Section 7.8.2.17
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} I2CCLKGS;//  64h I2C Clock Gate For Sleep Mode Section 7.8.2.18
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} I2CCLKGDS;//  68h I2C Clock Gate For Deep Sleep Mode Section 7.8.2.19
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} UARTCLKGR;//  6ch UART Clock Gate For Run And All Modes Section 7.8.2.20
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} UARTCLKGS;//  70h UART Clock Gate For Sleep Mode Section 7.8.2.21
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} UARTCLKGDS;// 74h UART Clock Gate For Deep Sleep Mode Section 7.8.2.22
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} SSICLKGR;// 78h SSI Clock Gate For Run And All Modes Section 7.8.2.23
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} SSICLKGS;//  7ch SSI Clock Gate For Sleep Mode Section 7.8.2.24
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} SSICLKGDS;// 80h SSI Clock Gate For Deep Sleep Mode Section 7.8.2.25
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} I2SCLKGR;//  84h I2S Clock Gate For Run And All Modes Section 7.8.2.26
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} I2SCLKGS;//  88h I2S Clock Gate For Sleep Mode Section 7.8.2.27
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} I2SCLKGDS;// 8ch  I2S Clock Gate For Deep Sleep Mode Section 7.8.2.28
	__IO uint32_t separator4[10];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} SYSBUSCLKDIV;// b4h Internal Section 7.8.2.29
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} CPUCLKDIV;//  b8h Internal Section 7.8.2.30
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PERBUSCPUCLKDIV;//  bch Internal Section 7.8.2.31
	__IO uint32_t separator5[1];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PERDMACLKDIV;//  c4h Internal Section 7.8.2.32
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} I2SBCLKSEL;//  c8h I2S Clock Control Section 7.8.2.33
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} GPTCLKDIV;//  cch GPT Scalar Section 7.8.2.34
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} I2SCLKCTL;//  d0h I2S Clock Control Section 7.8.2.35
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} I2SMCLKDIV;// d4h  MCLK Division Ratio Section 7.8.2.36
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} I2SBCLKDIV;// d8h BCLK Division Ratio Section 7.8.2.37
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} I2SWCLKDIV;//  dch WCLK Division Ratio Section 7.8.2.38
	__IO uint32_t separator6[4];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} RESETSECDMA;//  f0h RESET For SEC (PKA And TRNG And CRYPTO) AndUDMA
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} RESETGPIO;//  f4h RESET For GPIO IPs Section 7.8.2.40
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} RESETGPT;//  f8h RESET For GPT Ips Section 7.8.2.41
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} RESETI2C;//  fch RESET For I2C IPs Section 7.8.2.42
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} RESETUART;//  100h RESET For UART IPs Section 7.8.2.43
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} RESETSSI;//  104h RESET For SSI IPs Section 7.8.2.44
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} RESETI2S;//  108h RESET For I2S IP Section 7.8.2.45
	__IO uint32_t separator7[9];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDCTL0;//  12ch Power Domain Control Section 7.8.2.46
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDCTL0RFC;// 130h RFC Power Domain Control Section 7.8.2.47
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDCTL0SERIAL;// 134h SERIAL Power Domain Control Section 7.8.2.48
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDCTL0PERIPH;// 138h PERIPH Power Domain Control Section 7.8.2.49
	__IO uint32_t separator8[1];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t RFC_ON:1;
			uint32_t SERIAL_ON:1;
			uint32_t PERIPH_ON:1;
			uint32_t :29;
		} bit;
	} PDSTAT0;// 140h Power Domain Status Section 7.8.2.50
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDSTAT0RFC;// 144h RFC Power Domain Status Section 7.8.2.51
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDSTAT0SERIAL;// 148h SERIAL Power Domain Status Section 7.8.2.52
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDSTAT0PERIPH;// 14ch  PERIPH Power Domain Status Section 7.8.2.53
	__IO uint32_t separator9[12];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDCTL1;//  17ch Power Domain Control Section 7.8.2.54
	__IO uint32_t separator10[1];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDCTL1CPU;// 184h CPU Power Domain Direct Control Section 7.8.2.55
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDCTL1RFC;// 188h RFC Power Domain Direct Control Section 7.8.2.56
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDCTL1VIMS;// 18ch VIMS Mode Direct Control Section 7.8.2.57
	__IO uint32_t separator11[1];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDSTAT1;//  194h Power Manager Status Section 7.8.2.58
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDSTAT1BUS;//  198h BUS Power Domain Direct Read Status Section 7.8.2.59
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDSTAT1RFC;// 19ch  RFC Power Domain Direct Read Status Section 7.8.2.60
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDSTAT1CPU;//  1a0h CPU Power Domain Direct Read Status Section 7.8.2.61
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PDSTAT1VIMS;// 1a4h  VIMS Mode Direct Read Status Section 7.8.2.62
	__IO uint32_t separator12[6];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} RFCBITS;//  1cch Control To RFC Section 7.8.2.63
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} RFCMODESEL;//  1d0h Selected RFC Mode Section 7.8.2.64
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t MODE_0:1;
			uint32_t MODE_1:1;
			uint32_t MODE_2:1;
			uint32_t MODE_3:1;
			uint32_t MODE_4:1;
			uint32_t MODE_5:1;
			uint32_t MODE_6:1;
			uint32_t MODE_7:1;
			uint32_t :24;
		} bit;
		struct {
			uint32_t AVAIL:8;
			uint32_t :24;
		} part;
	} RFCMODEHWOPT;// 1d4h Allowed RFC Modes Section 7.8.2.65
	__IO uint32_t separator13[2];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} PWRPROFSTAT;//  1e0h Power Profiler Register Section 7.8.2.66
	__IO uint32_t separator14[15];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} MCUSRAMCFG;// 21ch MCU SRAM configuration Section 7.8.2.67
	__IO uint32_t separator15[1];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} RAMRETEN;//  224h Memory Retention Control Section 7.8.2.68
	__IO uint32_t separator16[27];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} OSCIMSC;//  290h Oscillator Interrupt Mask Section 7.8.2.69
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} OSCRIS;// 294h  Oscillator Raw Interrupt Status Section 7.8.2.70
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} OSCICR;// 298h Oscillator Raw Interrupt Clear Section 7.8.2.7
} PRCM_registers_t;


typedef struct {
	__IO uint32_t RATCNT;
	__IO uint32_t separator1[31];
	__IO uint32_t RATCHVAL[8];
} RFC_RAT_registers_t;

typedef struct {
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :10;
			uint32_t RAT_M:1;
			uint32_t :21;
		} bit;
	} PWMCLKEN;
} RFC_PWR_registers_t;

typedef struct {
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} CMDR;
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} CMDSTA;
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :1;
			uint32_t FCSA:1;
			uint32_t MDMDONE:1;
			uint32_t MDMIN:1;
			uint32_t MDMOUT:1;
			uint32_t MDMSOFT:1;
			uint32_t TRCTK:1;
			uint32_t :1;
			uint32_t RFEDONE:1;
			uint32_t RFESOFT0:1;
			uint32_t RFESOFT1:1;
			uint32_t RFESOFT2:1;
			uint32_t RATCH0:1;
			uint32_t RATCH1:1;
			uint32_t RATCH2:1;
			uint32_t RATCH3:1;
			uint32_t RATCH4:1;
			uint32_t RATCH5:1;
			uint32_t RATCH6:1;
			uint32_t RATCH7:1;
			uint32_t :12;
		} bit;
	} RFHWIFG;
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :1;
			uint32_t FCSA:1;
			uint32_t MDMDONE:1;
			uint32_t MDMIN:1;
			uint32_t MDMOUT:1;
			uint32_t MDMSOFT:1;
			uint32_t TRCTK:1;
			uint32_t :1;
			uint32_t RFEDONE:1;
			uint32_t RFESOFT0:1;
			uint32_t RFESOFT1:1;
			uint32_t RFESOFT2:1;
			uint32_t RATCH0:1;
			uint32_t RATCH1:1;
			uint32_t RATCH2:1;
			uint32_t RATCH3:1;
			uint32_t RATCH4:1;
			uint32_t RATCH5:1;
			uint32_t RATCH6:1;
			uint32_t RATCH7:1;
			uint32_t :12;
		} bit;
	} RFHWIEN;
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t COMMAND_DONE:1;
			uint32_t LAST_COMMAND_DONE:1;
			uint32_t FG_COMMAND_DONE:1;
			uint32_t LAST_FG_COMMAND_DONE:1;
			uint32_t TX_DONE:1;
			uint32_t TX_ACK:1;
			uint32_t TX_CTRL:1;
			uint32_t TX_CTRL_ACK:1;
			uint32_t TX_CTRL_ACK_ACK:1;
			uint32_t TX_RETRANS:1;
			uint32_t TX_ENTRY_DONE:1;
			uint32_t TX_BUFFER_CHANGED:1;
			uint32_t COMMAND_STARTED:1;
			uint32_t FG_COMMAND_STARTED:1;
			uint32_t IRQ14:1;
			uint32_t IRQ15:1;
			uint32_t RX_OK:1;
			uint32_t RX_NOK:1;
			uint32_t RX_IGNORED:1;
			uint32_t RX_EMPTY:1;
			uint32_t RX_CTRL:1;
			uint32_t RX_CTRL_ACK:1;
			uint32_t RX_BUF_FULL:1;
			uint32_t RX_ENTRY_DONE:1;
			uint32_t RX_DATA_WRITTEN:1;
			uint32_t RX_N_DATA_WRITTEN:1;
			uint32_t RX_ABORTED:1;
			uint32_t IRQ27:1;
			uint32_t SYNTH_NO_LOCK:1;
			uint32_t MODULES_UNLOCKED:1;
			uint32_t BOOT_DONE:1;
			uint32_t INTERNAL_ERROR:1;
		} bit;
	} RFCPEIFG;
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t COMMAND_DONE:1;
			uint32_t LAST_COMMAND_DONE:1;
			uint32_t FG_COMMAND_DONE:1;
			uint32_t LAST_FG_COMMAND_DONE:1;
			uint32_t TX_DONE:1;
			uint32_t TX_ACK:1;
			uint32_t TX_CTRL:1;
			uint32_t TX_CTRL_ACK:1;
			uint32_t TX_CTRL_ACK_ACK:1;
			uint32_t TX_RETRANS:1;
			uint32_t TX_ENTRY_DONE:1;
			uint32_t TX_BUFFER_CHANGED:1;
			uint32_t COMMAND_STARTED:1;
			uint32_t FG_COMMAND_STARTED:1;
			uint32_t IRQ14:1;
			uint32_t IRQ15:1;
			uint32_t RX_OK:1;
			uint32_t RX_NOK:1;
			uint32_t RX_IGNORED:1;
			uint32_t RX_EMPTY:1;
			uint32_t RX_CTRL:1;
			uint32_t RX_CTRL_ACK:1;
			uint32_t RX_BUF_FULL:1;
			uint32_t RX_ENTRY_DONE:1;
			uint32_t RX_DATA_WRITTEN:1;
			uint32_t RX_N_DATA_WRITTEN:1;
			uint32_t RX_ABORTED:1;
			uint32_t IRQ27:1;
			uint32_t SYNTH_NO_LOCK:1;
			uint32_t MODULES_UNLOCKED:1;
			uint32_t BOOT_DONE:1;
			uint32_t INTERNAL_ERROR:1;
		} bit;
	} RFCPEIEN;
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t COMMAND_DONE:1;
			uint32_t LAST_COMMAND_DONE:1;
			uint32_t FG_COMMAND_DONE:1;
			uint32_t LAST_FG_COMMAND_DONE:1;
			uint32_t TX_DONE:1;
			uint32_t TX_ACK:1;
			uint32_t TX_CTRL:1;
			uint32_t TX_CTRL_ACK:1;
			uint32_t TX_CTRL_ACK_ACK:1;
			uint32_t TX_RETRANS:1;
			uint32_t TX_ENTRY_DONE:1;
			uint32_t TX_BUFFER_CHANGED:1;
			uint32_t COMMAND_STARTED:1;
			uint32_t FG_COMMAND_STARTED:1;
			uint32_t IRQ14:1;
			uint32_t IRQ15:1;
			uint32_t RX_OK:1;
			uint32_t RX_NOK:1;
			uint32_t RX_IGNORED:1;
			uint32_t RX_EMPTY:1;
			uint32_t RX_CTRL:1;
			uint32_t RX_CTRL_ACK:1;
			uint32_t RX_BUF_FULL:1;
			uint32_t RX_ENTRY_DONE:1;
			uint32_t RX_DATA_WRITTEN:1;
			uint32_t RX_N_DATA_WRITTEN:1;
			uint32_t RX_ABORTED:1;
			uint32_t IRQ27:1;
			uint32_t SYNTH_NO_LOCK:1;
			uint32_t MODULES_UNLOCKED:1;
			uint32_t BOOT_DONE:1;
			uint32_t INTERNAL_ERROR:1;
		} bit;
	} RFCPEISL;
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t ACKFLAG:1;
			uint32_t :31;
		} bit;
	} RFACKIFG;
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} SYSGPOCTL;
} RFC_Doorbell_t;

typedef struct {
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t EN:1;
			uint32_t RTC_UPD_EN:1;
			uint32_t RTC_4KHZ_EN:1;
			uint32_t :4;
			uint32_t RESET:1;
			uint32_t EV_DELAY:4;
			uint32_t :4;
			uint32_t COMB_EV_MASK:3;
			uint32_t :13;
		} bit;
	} CTL;// 0h Control Section 16.4.1.1
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} EVFLAGS;// 4h Event Flags, RTC Status Section 16.4.1.2
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} SEC;//  8h  Second Counter Value, Integer Part Section 16.4.1.3
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} SUBSEC;//  Ch  Second Counter Value, Fractional Part Section 16.4.1.4
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	};//  10h SUBSECINC Subseconds Increment Section 16.4.1.5
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} CHCTL;//  14h  Channel Configuration Section 16.4.1.6
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} CH0CMP;//  18h  Channel 0 Compare Value Section 16.4.1.7
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} CH1CMP;//  1Ch  Channel 1 Compare Value Section 16.4.1.8
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} CH2CMP;// 20h  Channel 2 Compare Value Section 16.4.1.9
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} CH2CMPINC;//  24h  Channel 2 Compare Value Auto-increment Section 16.4.1.10
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} CH1CAPT;//  28h  Channel 1 Capture Value Section 16.4.1.11
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} SYNC;// 2Ch  AON Synchronization Section 16.4.1.12
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} TIME;//  30h  Current Counter Value Section 16.4.1.13
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} SYNCLF;//  34h  Synchronization to SCLK_LF Section 16.4.1.1
} RTC_registers_t;

typedef struct {
	__IO uint32_t spacer1[7];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} STAT; // 1ch
	__IO uint32_t spacer2[1];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t DIS_IDLE:1;
			uint32_t DIS_STANDBY:1;
			uint32_t :1;
			uint32_t ENABLE_SWINTF:1;
			uint32_t DIS_READACCESS:1;
			uint32_t DIS_EFUSECLK:1;
			uint32_t :1;
			uint32_t :1;
			uint32_t :1;
			uint32_t :1;
			uint32_t :22;
		} bit;
	} CFG; // 24h
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} SYSCODE_START;	// 28h
} FLASH_registers_t;


typedef struct {
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t all:32;
		} bit;
	} LOAD; //  0x00
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t all:32;
		} bit;
	} VALUE; //  0x04
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t INTEN:1;
			uint32_t RESEN:1;
			uint32_t INTTYPE:1;
			uint32_t :29;
		} bit;
	} CTL; //  0x08
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t all:32;
		} bit;
	} ICR; // 0x0c
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t WDTRIS:1;
			uint32_t :31;
		} bit;
	} RIS; // 0x10
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} MIS; // 0x14
	__IO uint32_t spacer1[257];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t TEST_EN:1;
			uint32_t :7;
			uint32_t STALL:1;
			uint32_t :23;
		} bit;
	} TEST; // 0x418
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t :32;
		} bit;
	} INT_CAUS; // 0x41c
	__IO uint32_t spacer2[505];
	__IO union {
		uint32_t register_value;
		struct {
			uint32_t all:32;
		} bit;
	} LOCK; // 0xc00
} WDT_registers_t;

#define FLASH0					((FLASH_registers_t*) FLASH_BASE)

#define RTC						((RTC_registers_t*) AON_RTC_BASE)
#define PRCM0					((PRCM_registers_t*) PRCM_BASE)
#define RFC_DOOR_BELL		((RFC_Doorbell_t*) RFC_DBELL_BASE)
#define RFC_PWR				((RFC_PWR_registers_t*) RFC_PWR_BASE)
#define WDT0					((WDT_registers_t*) WDT_BASE)

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
#include <ti/driverlib/i2c.c>
//#include <ti/driverlib/interrupt.c>
#include <ti/driverlib/rfc.c>
#include <ti/driverlib/sys_ctrl.c>
#include <ti/driverlib/cpu.c>

static const uint32_t g_pui32Regs[] =
{
    0, NVIC_SYS_PRI1, NVIC_SYS_PRI2, NVIC_SYS_PRI3, NVIC_PRI0, NVIC_PRI1,
    NVIC_PRI2, NVIC_PRI3, NVIC_PRI4, NVIC_PRI5, NVIC_PRI6, NVIC_PRI7,
    NVIC_PRI8, NVIC_PRI9, NVIC_PRI10, NVIC_PRI11, NVIC_PRI12, NVIC_PRI13
};

void
IntPrioritySet(uint32_t ui32Interrupt, uint8_t ui8Priority)
{
    uint32_t ui32Temp;

    // Check the arguments.
    ASSERT((ui32Interrupt >= 4) && (ui32Interrupt < NUM_INTERRUPTS));
    ASSERT(ui8Priority <= INT_PRI_LEVEL7);

    // Set the interrupt priority.
    ui32Temp = HWREG(g_pui32Regs[ui32Interrupt >> 2]);
    ui32Temp &= ~(0xFF << (8 * (ui32Interrupt & 3)));
    ui32Temp |= ui8Priority << (8 * (ui32Interrupt & 3));
    HWREG(g_pui32Regs[ui32Interrupt >> 2]) = ui32Temp;
}

void
IntPendClear(uint32_t ui32Interrupt)
{
    // Check the arguments.
    ASSERT(ui32Interrupt < NUM_INTERRUPTS);

    // Determine the interrupt to unpend.
    if(ui32Interrupt == INT_PENDSV)
    {
        // Unpend the PendSV interrupt.
        HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_UNPEND_SV;
    }
    else if(ui32Interrupt == INT_SYSTICK)
    {
        // Unpend the SysTick interrupt.
        HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PENDSTCLR;
    }
    else if((ui32Interrupt >= 16) && (ui32Interrupt <= 47))
    {
        // Unpend the general interrupt.
        HWREG(NVIC_UNPEND0) = 1 << (ui32Interrupt - 16);
    }
    else if(ui32Interrupt >= 48)
    {
        // Unpend the general interrupt.
        HWREG(NVIC_UNPEND1) = 1 << (ui32Interrupt - 48);
    }
}

void
IntEnable(uint32_t ui32Interrupt)
{
    // Check the arguments.
    ASSERT(ui32Interrupt < NUM_INTERRUPTS);

    // Determine the interrupt to enable.
    if(ui32Interrupt == INT_MEMMANAGE_FAULT)
    {
        // Enable the MemManage interrupt.
        HWREG(NVIC_SYS_HND_CTRL) |= NVIC_SYS_HND_CTRL_MEM;
    }
    else if(ui32Interrupt == INT_BUS_FAULT)
    {
        // Enable the bus fault interrupt.
        HWREG(NVIC_SYS_HND_CTRL) |= NVIC_SYS_HND_CTRL_BUS;
    }
    else if(ui32Interrupt == INT_USAGE_FAULT)
    {
        // Enable the usage fault interrupt.
        HWREG(NVIC_SYS_HND_CTRL) |= NVIC_SYS_HND_CTRL_USAGE;
    }
    else if(ui32Interrupt == INT_SYSTICK)
    {
        // Enable the System Tick interrupt.
        HWREG(NVIC_ST_CTRL) |= NVIC_ST_CTRL_INTEN;
    }
    else if((ui32Interrupt >= 16) && (ui32Interrupt <= 47))
    {
        // Enable the general interrupt.
        HWREG(NVIC_EN0) = 1 << (ui32Interrupt - 16);
    }
    else if(ui32Interrupt >= 48)
    {
        // Enable the general interrupt.
        HWREG(NVIC_EN1) = 1 << (ui32Interrupt - 48);
    }
}

void cc2652_register_interrupt_handler (io_t*,int32_t number,io_interrupt_action_t handler,void *);

void
scif_interrupt (void *user_value) {
	((void (*)(void)) user_value)();
}

void
IntRegister(uint32_t ui32Interrupt, void (*pfnHandler)(void)) {
	//
	// This relies on cc2652 cpu not referencing io in it's implementation
	// of this method.
	//
	cc2652_register_interrupt_handler (NULL,ui32Interrupt,scif_interrupt,pfnHandler);
}

#endif /* IMPLEMENT_IO_CPU */
#endif
/*
Copyright 2020 Gregor Bruce

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice
and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
