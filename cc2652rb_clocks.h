/*
 *
 * power domains and clocks for the CC2652
 *
 */
#ifndef cc2652rb_clocks_H_
#define cc2652rb_clocks_H_

#include <ti/driverlib/prcm.h>

//
// power domains
//

typedef struct PACK_STRUCTURE io_cc2652_cpu_power_domain {
    IO_CPU_POWER_DOMAIN_STRUCT_MEMBERS
    uint32_t prcm_domain_identifier;
} io_cc2652_cpu_power_domain_t;

#define CLOCK_MEMORY

extern EVENT_DATA io_cc2652_cpu_power_domain_t cpu_core_power_domain;
extern CLOCK_MEMORY io_cc2652_cpu_power_domain_t peripheral_power_domain;
extern CLOCK_MEMORY io_cc2652_cpu_power_domain_t serial_power_domain;
extern CLOCK_MEMORY io_cc2652_cpu_power_domain_t radio_power_domain;

//
// clocks
//

extern EVENT_DATA io_cpu_clock_implementation_t cc2652_hf_rc_48_oscillator_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_hf_rc_24_oscillator_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_hf_xosc_oscillator_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_lf_rc_oscillator_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_hp_oscillator_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_lf_crystal_oscillator_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_sclk_lf_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_infrastructure_clock_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_core_clock_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_peripheral_clock_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_serial_clock_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_radio_clock_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_rtc_clock_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_dma_clock_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_wdt_clock_implementation;


typedef struct PACK_STRUCTURE cc2652_hf_rc_oscillator {
    IO_CPU_CLOCK_SOURCE_STRUCT_MEMBERS
} cc2652_hf_rc_oscillator_t, cc2652_xosc_oscillator_t;

typedef struct PACK_STRUCTURE cc2652_hf_oscillator {
    IO_CPU_CLOCK_SOURCE_STRUCT_MEMBERS
    float64_t frequency;
} cc2652_hf_oscillator_t;

typedef struct PACK_STRUCTURE cc2652_sclk_lf {
    IO_CPU_CLOCK_FUNCTION_STRUCT_MEMBERS
} cc2652_sclk_lf_t;

typedef struct PACK_STRUCTURE cc2652_infrastructure_clock {
	IO_CPU_CLOCK_FUNCTION_STRUCT_MEMBERS
	struct PACK_STRUCTURE {
		uint32_t run_mode:2;
		uint32_t sleep_mode:2;
		uint32_t deep_sleep_mode:2;
		uint32_t :26;
	} divider;
} cc2652_infrastructure_clock_t;

typedef struct cc2652_core_clock {
	IO_CPU_CLOCK_FUNCTION_STRUCT_MEMBERS
} cc2652_core_clock_t;

typedef struct cc2652_rtc_clock {
    IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS
} cc2652_rtc_clock_t;

typedef struct cc2652_peripheral_clock {
    IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS
    uint32_t prcm_peripheral_id;
} cc2652_peripheral_clock_t;

INLINE_FUNCTION bool
cc2652_clock_is_hp_oscillator (io_cpu_clock_pointer_t clock) {
    return io_cpu_clock_has_implementation (clock,&cc2652_hp_oscillator_implementation);
}

INLINE_FUNCTION bool
cpu_clock_is_cc2652_hf_rc_oscillator (io_cpu_clock_pointer_t clock) {
	return io_cpu_clock_has_implementation (clock,&cc2652_hf_rc_48_oscillator_implementation);
}

INLINE_FUNCTION bool
cpu_clock_is_cc2652_hp_oscillator (io_cpu_clock_pointer_t clock) {
	return io_cpu_clock_has_implementation (clock,&cc2652_hp_oscillator_implementation);
}

INLINE_FUNCTION bool
cpu_clock_is_cc2652_hf_crystal_oscillator (io_cpu_clock_pointer_t clock) {
	return io_cpu_clock_has_implementation (
		clock,&cc2652_hf_xosc_oscillator_implementation
	);
}

INLINE_FUNCTION bool
cpu_clock_is_cc2652_lf_crystal_oscillator (io_cpu_clock_pointer_t clock) {
	return io_cpu_clock_has_implementation (
   		clock,&cc2652_lf_crystal_oscillator_implementation
	);
}

INLINE_FUNCTION bool
cpu_clock_is_cc2652_lf_rc_oscillator (io_cpu_clock_pointer_t clock) {
	return io_cpu_clock_has_implementation (
   		clock,&cc2652_lf_rc_oscillator_implementation
	);
}

INLINE_FUNCTION bool
cpu_clock_is_derrived_from_hp_oscillator (
	io_cpu_clock_pointer_t clock
) {
	return io_cpu_clock_is_derrived_from (
		clock,&cc2652_hp_oscillator_implementation
	);
}

INLINE_FUNCTION bool
cpu_clock_is_derrived_from_hf_xosc_oscillator (
	io_cpu_clock_pointer_t clock
) {
	return io_cpu_clock_is_derrived_from (
		clock,&cc2652_hf_xosc_oscillator_implementation
	);
}


#ifdef IMPLEMENT_IO_CPU
//-----------------------------------------------------------------------------
//
// implementation
//
//-----------------------------------------------------------------------------
//
// power domains
//

static bool
controlled_power_domain_turn_on (io_cpu_power_domain_pointer_t pd) {
    io_cc2652_cpu_power_domain_t const *this = (io_cc2652_cpu_power_domain_t const *) (
        io_cpu_power_domain_ro_pointer (pd)
    );

    if (PRCMPowerDomainStatus(this->prcm_domain_identifier) == PRCM_DOMAIN_POWER_OFF) {
      PRCMPowerDomainOn (this->prcm_domain_identifier);
      while(PRCMPowerDomainStatus(this->prcm_domain_identifier) != PRCM_DOMAIN_POWER_ON);
      PRCMLoadSet();
    }

    if (io_cpu_power_domain_rw_pointer (pd)) {
        io_cpu_power_domain_rw_pointer(pd)->reference_count ++;
    }

    return true;
}

//
// we can only turn a PD off when all clocks in that domain are off,
// so we need to keep track of which clocks has requested the pd
// to turn on
//
EVENT_DATA io_cpu_power_domain_implementation_t
cpu_core_power_domain_implementation = {
    .turn_off = io_power_domain_no_operation,
    .turn_on = io_power_domain_no_operation,
};

EVENT_DATA io_cc2652_cpu_power_domain_t cpu_core_power_domain = {
    .implementation = &cpu_core_power_domain_implementation,
    .prcm_domain_identifier = PRCM_DOMAIN_MCU, //
};

static void
turn_peripheral_power_domain_on (io_t *io,io_cpu_power_domain_pointer_t pd) {
    controlled_power_domain_turn_on (pd);
}

EVENT_DATA io_cpu_power_domain_implementation_t
peripheral_power_domain_implementation = {
    .turn_off = io_power_domain_no_operation,
    .turn_on = turn_peripheral_power_domain_on,
};

io_cc2652_cpu_power_domain_t peripheral_power_domain = {
    .implementation = &peripheral_power_domain_implementation,
    .prcm_domain_identifier = PRCM_DOMAIN_PERIPH,
};

static void
turn_serial_power_domain_on (io_t *io,io_cpu_power_domain_pointer_t pd) {
    controlled_power_domain_turn_on (pd);
}

static EVENT_DATA io_cpu_power_domain_implementation_t
serial_power_domain_implementation = {
    .turn_off = io_power_domain_no_operation,
    .turn_on = turn_serial_power_domain_on,
};

io_cc2652_cpu_power_domain_t serial_power_domain = {
    .implementation = &serial_power_domain_implementation,
    .prcm_domain_identifier = PRCM_DOMAIN_SERIAL,
};

static void
turn_radio_power_domain_on (io_t *io,io_cpu_power_domain_pointer_t pd) {
    controlled_power_domain_turn_on (pd);
}

static EVENT_DATA io_cpu_power_domain_implementation_t
radio_power_domain_implementation = {
    .turn_off = io_power_domain_no_operation,
    .turn_on = turn_radio_power_domain_on,
};

io_cc2652_cpu_power_domain_t radio_power_domain = {
    .implementation = &radio_power_domain_implementation,
    .prcm_domain_identifier = PRCM_DOMAIN_RFCORE,
};

//
// work in progress
//

EVENT_DATA io_cpu_power_domain_implementation_t
bus_power_domain_implementation = {
    .turn_off = io_power_domain_no_operation,
    .turn_on = io_power_domain_no_operation,
};

EVENT_DATA io_cpu_power_domain_implementation_t
vims_power_domain_implementation = {
    .turn_off = io_power_domain_no_operation,
    .turn_on = io_power_domain_no_operation,
};

//
// clocks
//

static bool
cc2652_hf_rc_oscillator_start (io_t *io,io_cpu_clock_pointer_t this) {
    return true;
}

static float64_t
cc2652_hf_rc_48_oscillator_get_current_frequency (io_cpu_clock_pointer_t this) {
    return 48000000.0;
}

//
// oscillators seem to be outside any power domain
//
EVENT_DATA io_cpu_clock_implementation_t
cc2652_hf_rc_48_oscillator_implementation = {
    .specialisation_of = &io_cpu_clock_implementation,
    .get_current_frequency = cc2652_hf_rc_48_oscillator_get_current_frequency,
    .get_expected_frequency = cc2652_hf_rc_48_oscillator_get_current_frequency,
    .get_power_domain = get_always_on_io_power_domain,
    .start = cc2652_hf_rc_oscillator_start,
    .stop = NULL,
};

static float64_t
cc2652_hf_rc_24_oscillator_get_current_frequency (io_cpu_clock_pointer_t this) {
    return 24000000.0;
}

EVENT_DATA io_cpu_clock_implementation_t
cc2652_hf_rc_24_oscillator_implementation = {
	SPECIALISE_IO_CPU_CLOCK_IMPLEMENTATION(&io_cpu_clock_implementation)
	.get_current_frequency = cc2652_hf_rc_24_oscillator_get_current_frequency,
	.get_expected_frequency = cc2652_hf_rc_24_oscillator_get_current_frequency,
	.get_power_domain = get_always_on_io_power_domain,
	.start = cc2652_hf_rc_oscillator_start,
};

static float64_t
cc2652_hp_oscillator_get_current_frequency (io_cpu_clock_pointer_t this) {
    cc2652_hf_oscillator_t const *c = (cc2652_hf_oscillator_t const*) (
        io_cpu_clock_ro_pointer (this)
    );
    return c->frequency;
}

//
// hp really means the HF clock sourced from the XOSC
// but there seems no way to explicitly turn on the
// external oscillator independently of the HF clock
//
static bool
cc2652_hp_oscillator_start (io_t *io,io_cpu_clock_pointer_t this) {
	if (OSC_IsHPOSCEnabled()) {
		if (OSCClockSourceGet(OSC_SRC_CLK_HF) == OSC_XOSC_HF) {
			return true;
		} else {
			OSC_HPOSCInitializeFrequencyOffsetParameters();
			OSCHF_TurnOnXosc();
		}
		return true;
	} else {
		return false;
	}
}

EVENT_DATA io_cpu_clock_implementation_t
cc2652_hp_oscillator_implementation = {
	SPECIALISE_IO_CPU_CLOCK_IMPLEMENTATION (
		&io_cpu_clock_source_implementation
	)
	.get_current_frequency = cc2652_hp_oscillator_get_current_frequency,
	.get_expected_frequency = cc2652_hp_oscillator_get_current_frequency,
	.get_power_domain = get_always_on_io_power_domain,
	.start = cc2652_hp_oscillator_start,
};

static bool
cc2652_hf_xosc_oscillator_start (io_t *io,io_cpu_clock_pointer_t this) {
	if (OSCClockSourceGet(OSC_SRC_CLK_HF) == OSC_XOSC_HF) {
		return true;
	} else {
		OSCHF_TurnOnXosc();
		return true;
	}
}

EVENT_DATA io_cpu_clock_implementation_t
cc2652_hf_xosc_oscillator_implementation = {
	SPECIALISE_IO_CPU_CLOCK_IMPLEMENTATION (
		&io_cpu_clock_source_implementation
	)
	.get_current_frequency = cc2652_hp_oscillator_get_current_frequency,
	.get_expected_frequency = cc2652_hp_oscillator_get_current_frequency,
	.get_power_domain = get_always_on_io_power_domain,
	.start = cc2652_hf_xosc_oscillator_start,
};


//
// SCLK_LF derived from one of:
//		OSC_RCOSC_HF
//		OSC_XOSC_HF
//		OSC_RCOSC_LF
//		OSC_XOSC_LF
//
// This is established in ccfg so device clock tree needs to follow
// the ccfg device defines
//
static float64_t
cc2652_sclk_lf_get_current_frequency (io_cpu_clock_pointer_t this) {
    cc2652_sclk_lf_t const *c = (cc2652_sclk_lf_t const*) (
        io_cpu_clock_ro_pointer (this)
    );
    return io_cpu_clock_get_current_frequency (c->input);
}

static bool
cc2652_sclk_lf_start (io_t *io,io_cpu_clock_pointer_t clock) {
	if (io_cpu_dependant_clock_start_input (io,clock)) {
		cc2652_sclk_lf_t const *this = (cc2652_sclk_lf_t const*) (
			io_cpu_clock_ro_pointer (clock)
		);

		uint32_t current_source;

		UNUSED(current_source);
		current_source = OSCClockSourceGet(OSC_SRC_CLK_LF);
		if (current_source == OSC_XOSC_LF) {

		}

		if (cpu_clock_is_cc2652_hf_rc_oscillator (this->input)) {
			OSCClockSourceSet (OSC_SRC_CLK_LF,OSC_RCOSC_HF);
		} else if (cpu_clock_is_cc2652_hp_oscillator (this->input)) {
			OSCClockSourceSet (OSC_SRC_CLK_LF,OSC_XOSC_HF);
		} else if (cpu_clock_is_cc2652_lf_crystal_oscillator (this->input)) {
			//
			// not working yet
			//
			OSCClockSourceSet (OSC_SRC_CLK_LF,OSC_XOSC_LF);
			while (OSCClockSourceGet(OSC_SRC_CLK_LF) != OSC_XOSC_LF);
		} else if (cpu_clock_is_cc2652_lf_rc_oscillator (this->input)) {
			OSCClockSourceSet (OSC_SRC_CLK_LF,OSC_RCOSC_LF);
		} else {
			return false;
		}

		current_source = OSCClockSourceGet(OSC_SRC_CLK_LF);
		if (current_source == OSC_XOSC_LF) {

		}

		return true;
	} else {
	  return false;
	}
}

EVENT_DATA io_cpu_clock_implementation_t
cc2652_sclk_lf_implementation = {
	SPECIALISE_IO_CPU_CLOCK_IMPLEMENTATION(&io_cpu_clock_implementation)
	.get_current_frequency = cc2652_sclk_lf_get_current_frequency,
	.get_expected_frequency = cc2652_sclk_lf_get_current_frequency,
	.start = cc2652_sclk_lf_start,
};

static bool
cc2652_lf_crystal_oscillator_start (io_t *io,io_cpu_clock_pointer_t clock) {

	// just need to know it is configured

	// what does DDI_0_OSC_STAT0_XOSC_LF_EN mean

	return true;
}

static float64_t
cc2652_get_32khz (
	io_cpu_clock_pointer_t this
) {
    return (float64_t) (1UL << 15);
}

EVENT_DATA io_cpu_clock_implementation_t
cc2652_lf_crystal_oscillator_implementation = {
	SPECIALISE_IO_CPU_CLOCK_SOURCE_IMPLEMENTATION (
		&io_cpu_clock_source_implementation
	)
	.start = cc2652_lf_crystal_oscillator_start,
	.get_current_frequency = cc2652_get_32khz,
	.get_expected_frequency = cc2652_get_32khz,
};


EVENT_DATA io_cpu_clock_implementation_t
cc2652_lf_rc_oscillator_implementation = {
	SPECIALISE_IO_CPU_CLOCK_SOURCE_IMPLEMENTATION (
		&io_cpu_clock_source_implementation
	)
	.get_current_frequency = cc2652_get_32khz,
	.get_expected_frequency = cc2652_get_32khz,
};

static bool
cc2652_infrastructure_clock_start (io_t *io,io_cpu_clock_pointer_t clock) {
	if (io_cpu_dependant_clock_start_input (io,clock)) {
		cc2652_infrastructure_clock_t const *this = (
			(cc2652_infrastructure_clock_t const*) (
				io_cpu_clock_ro_pointer (clock)
			)
		);

		PRCM0->INFRCLKDIVR.bit.RATIO = this->divider.run_mode;
		PRCM0->INFRCLKDIVS.bit.RATIO = this->divider.sleep_mode;
		PRCM0->INFRCLKDIVDS .bit.RATIO = this->divider.deep_sleep_mode;

		return true;
	} else {
		return false;
	}
}

static float64_t
cc2652_infrastructure_clock_get_frequency (io_cpu_clock_pointer_t clock) {
	cc2652_infrastructure_clock_t const *this = (cc2652_infrastructure_clock_t const*) (
        io_cpu_clock_ro_pointer (clock)
    );
    float64_t f = io_cpu_clock_get_current_frequency (this->input);
    uint32_t div;

    // assume run mode ...
    switch (PRCM0->INFRCLKDIVR.bit.RATIO) {
    	 case 0:
   	 	 div = 1;
   	 break;
    	 case 1:
   	 	 div = 2;
   	 break;
    	 case 2:
   	 	 div = 8;
   	 break;
    	 case 3:
   	 	 div = 32;
   	 break;
    }

    return f / ((float64_t) (div));
}

EVENT_DATA io_cpu_clock_implementation_t
cc2652_infrastructure_clock_implementation = {
	SPECIALISE_IO_CPU_CLOCK_FUNCTION_IMPLEMENTATION(
		&io_cpu_clock_function_implementation
	)
	.start = cc2652_infrastructure_clock_start,
	.get_current_frequency = cc2652_infrastructure_clock_get_frequency,
	.get_expected_frequency = cc2652_infrastructure_clock_get_frequency,
};

static float64_t
cc2652_core_clock_get_current_frequency (io_cpu_clock_pointer_t clock) {
    cc2652_core_clock_t const *this = (cc2652_core_clock_t const*) (
        io_cpu_clock_ro_pointer (clock)
    );
    return io_cpu_clock_get_current_frequency (this->input);
}

static bool
cc2652_core_clock_start (io_t *io,io_cpu_clock_pointer_t clock) {
	if (io_cpu_dependant_clock_start_input (io,clock)) {
		cc2652_core_clock_t const *this = (cc2652_core_clock_t const*) (
			io_cpu_clock_ro_pointer (clock)
		);

		if (cpu_clock_is_cc2652_hf_rc_oscillator (this->input)) {
			OSCClockSourceSet(OSC_SRC_CLK_HF,OSC_RCOSC_HF);
			//or OSCHF_SwitchToRcOscTurnOffXosc().
		} else if (cpu_clock_is_cc2652_hp_oscillator (this->input)) {
			switch (OSCClockSourceGet(OSC_SRC_CLK_HF)) {
				case OSC_XOSC_HF:
					// already
				break;

				default:
					OSCClockSourceSet(OSC_SRC_CLK_HF,OSC_XOSC_HF);
					while (!OSCHfSourceReady());
					OSCHfSourceSwitch();
				break;
			}
		} else if (cpu_clock_is_cc2652_hf_crystal_oscillator (this->input)) {
			switch (OSCClockSourceGet(OSC_SRC_CLK_HF)) {
				case OSC_XOSC_HF:
					// already
				break;

				default:
					OSCClockSourceSet(OSC_SRC_CLK_HF,OSC_XOSC_HF);
					while (!OSCHfSourceReady());
					OSCHfSourceSwitch();
				break;
			}

			uint32_t f = OSCClockSourceGet(OSC_SRC_CLK_HF);
			UNUSED(f);
			return f == OSC_XOSC_HF;
		} else {
		  return false;
		}
		return true;
	} else {
		return false;
	}
}

static io_cpu_power_domain_pointer_t
cc2652_core_clock_get_power_domain (io_cpu_clock_pointer_t clock) {
    return def_io_cpu_power_domain_pointer (&cpu_core_power_domain);
}

EVENT_DATA io_cpu_clock_implementation_t
cc2652_core_clock_implementation = {
	SPECIALISE_IO_CPU_CLOCK_FUNCTION_IMPLEMENTATION (
		&io_cpu_clock_function_implementation
	)
	.get_current_frequency = cc2652_core_clock_get_current_frequency,
	.get_expected_frequency = cc2652_core_clock_get_current_frequency,
	.get_power_domain = cc2652_core_clock_get_power_domain,
	.start = cc2652_core_clock_start,
};

static bool
cc2652_peripheral_clock_start (io_t *io,io_cpu_clock_pointer_t clock) {
    if (io_cpu_dependant_clock_start_input (io,clock)) {
        cc2652_peripheral_clock_t const *this = (cc2652_peripheral_clock_t const*) (
            io_cpu_clock_ro_pointer (clock)
        );

        turn_on_io_power_domain (io,io_cpu_clock_power_domain (clock));

        //
        // enable in run-power-mode
        //
        PRCMPeripheralRunEnable(this->prcm_peripheral_id);
        //
        // enable in sleep-power-mode
        //
        PRCMPeripheralSleepEnable(this->prcm_peripheral_id);
        PRCMLoadSet();

        return true;
    } else {
        return false;
    }
}

static io_cpu_power_domain_pointer_t
cc2652_peripheral_clock_get_power_domain (io_cpu_clock_pointer_t clock) {
    return def_io_cpu_power_domain_pointer (&peripheral_power_domain);
}

EVENT_DATA io_cpu_clock_implementation_t
cc2652_peripheral_clock_implementation = {
	SPECIALISE_DEPENDANT_IO_CPU_CLOCK_IMPLEMENTATION(
		&io_dependent_clock_implementation
	)
	.get_power_domain = cc2652_peripheral_clock_get_power_domain,
	.start = cc2652_peripheral_clock_start,
};

static io_cpu_power_domain_pointer_t
cc2652_serial_clock_get_power_domain (io_cpu_clock_pointer_t clock) {
    return def_io_cpu_power_domain_pointer (&serial_power_domain);
}

EVENT_DATA io_cpu_clock_implementation_t
cc2652_serial_clock_implementation = {
	SPECIALISE_DEPENDANT_IO_CPU_CLOCK_IMPLEMENTATION(
		&io_dependent_clock_implementation
	)
	.get_power_domain = cc2652_serial_clock_get_power_domain,
	.start = cc2652_peripheral_clock_start,
};

static io_cpu_power_domain_pointer_t
cc2652_radio_clock_get_power_domain (io_cpu_clock_pointer_t clock) {
    return def_io_cpu_power_domain_pointer (&radio_power_domain);
}

static bool
cc2652_radio_clock_start (io_t *io,io_cpu_clock_pointer_t clock) {
    if (io_cpu_dependant_clock_start_input (io,clock)) {
        turn_on_io_power_domain (io,io_cpu_clock_power_domain (clock));
        // Turn on the clock to the RF core. Registers can be accessed afterwards.
        RFCClockEnable();
        return true;
    } else {
        return false;
    }
}

EVENT_DATA io_cpu_clock_implementation_t cc2652_radio_clock_implementation = {
	SPECIALISE_DEPENDANT_IO_CPU_CLOCK_IMPLEMENTATION(
		&io_dependent_clock_implementation
	)
	.get_power_domain = cc2652_radio_clock_get_power_domain,
	.start = cc2652_radio_clock_start,
};

//
// RTC clock is supplied from SCLK_LF
//
static bool
cc2652_rtc_clock_start (io_t *io,io_cpu_clock_pointer_t clock) {
	if (io_cpu_dependant_clock_start_input (io,clock)) {
		return true;
	} else {
		return false;
	}
}

static float64_t
cc2652_rtc_clock_get_current_frequency (io_cpu_clock_pointer_t clock) {
    cc2652_rtc_clock_t const *this = (cc2652_rtc_clock_t const*) (
        io_cpu_clock_ro_pointer (clock)
    );
    return io_cpu_clock_get_current_frequency (this->input);
}

EVENT_DATA io_cpu_clock_implementation_t cc2652_rtc_clock_implementation = {
	SPECIALISE_DEPENDANT_IO_CPU_CLOCK_IMPLEMENTATION(
		&io_dependent_clock_implementation
	)
	.get_current_frequency = cc2652_rtc_clock_get_current_frequency,
	.get_expected_frequency = cc2652_rtc_clock_get_current_frequency,
	.get_power_domain = get_always_on_io_power_domain,
	.start = cc2652_rtc_clock_start,
};

static float64_t
cc2652_wdt_clock_get_current_frequency (io_cpu_clock_pointer_t clock) {
    cc2652_rtc_clock_t const *this = (cc2652_rtc_clock_t const*) (
        io_cpu_clock_ro_pointer (clock)
    );
    //
    // there is a pre-scaller in the WD according to the ti driver
    //
    return io_cpu_clock_get_current_frequency (this->input) / (32.0);
}

EVENT_DATA io_cpu_clock_implementation_t
cc2652_wdt_clock_implementation = {
	SPECIALISE_DEPENDANT_IO_CPU_CLOCK_IMPLEMENTATION(
		&io_dependent_clock_implementation
	)
	.get_power_domain = get_always_on_io_power_domain,
	.start = cc2652_peripheral_clock_start,
	.get_current_frequency = cc2652_wdt_clock_get_current_frequency,
};


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
