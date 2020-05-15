/*
 *
 *
 *
 */
#ifndef cc2652rb_clocks_H_
#define cc2652rb_clocks_H_

//
// power domain
//

typedef struct PACK_STRUCTURE io_cc2652_cpu_power_domain {
    IO_CPU_POWER_DOMAIN_STRUCT_MEMBERS
    uint32_t prcm_domain_identifier;
} io_cc2652_cpu_power_domain_t;

extern io_cc2652_cpu_power_domain_t peripheral_power_domain;

//
// inline io power domain implementation
//
INLINE_FUNCTION void
turn_on_io_power_domain (io_t* io,io_cpu_power_domain_pointer_t pd) {
    return io_cpu_power_domain_ro_pointer(pd)->implementation->turn_on(io,pd);
}

INLINE_FUNCTION void
turn_off_io_power_domain (io_t* io,io_cpu_power_domain_pointer_t pd) {
    return io_cpu_power_domain_ro_pointer(pd)->implementation->turn_off(io,pd);
}

//
// clocks
//
typedef struct PACK_STRUCTURE cc2652_hf_rc_oscillator {
    IO_CPU_CLOCK_SOURCE_STRUCT_MEMBERS
} cc2652_hf_rc_oscillator_t;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_hf_rc_48_oscillator_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_hf_rc_24_oscillator_implementation;

INLINE_FUNCTION bool
cpu_clock_is_cc2652_hf_rc_oscillator (io_cpu_clock_pointer_t clock) {
    return io_cpu_clock_has_implementation (clock,&cc2652_hf_rc_48_oscillator_implementation);
}

typedef struct PACK_STRUCTURE cc2652_hp_oscillator {
    IO_CPU_CLOCK_SOURCE_STRUCT_MEMBERS
    float64_t frequency;
} cc2652_hp_oscillator_t;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_hp_oscillator_implementation;

INLINE_FUNCTION bool
cpu_clock_is_cc2652_hp_oscillator (io_cpu_clock_pointer_t clock) {
    return io_cpu_clock_has_implementation (clock,&cc2652_hp_oscillator_implementation);
}

typedef struct PACK_STRUCTURE cc2652_sclk_lf {
    IO_CPU_CLOCK_FUNCTION_STRUCT_MEMBERS
} cc2652_sclk_lf_t;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_sclk_lf_implementation;

typedef struct cc2652_core_clock {
    IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS
} cc2652_core_clock_t;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_core_clock_implementation;

typedef struct cc2652_rtc_clock {
    IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS
} cc2652_rtc_clock_t;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_rtc_clock_implementation;

typedef struct cc2652_peripheral_clock {
    IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS
    uint32_t prcm_peripheral_id;
} cc2652_peripheral_clock_t;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_peripheral_clock_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_serial_clock_implementation;


bool
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

EVENT_DATA io_cpu_power_domain_implementation_t
cpu_core_power_domain_implementation = {
    .turn_off = io_power_domain_no_operation,
    .turn_on = io_power_domain_no_operation,
};

EVENT_DATA io_cpu_power_domain_implementation_t
bus_power_domain_implementation = {
    .turn_off = io_power_domain_no_operation,
    .turn_on = io_power_domain_no_operation,
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

static void
turn_serial_power_domain_on (io_t *io,io_cpu_power_domain_pointer_t pd) {
    controlled_power_domain_turn_on (pd);
}

static EVENT_DATA io_cpu_power_domain_implementation_t
serial_power_domain_implementation = {
    .turn_off = io_power_domain_no_operation,
    .turn_on = turn_serial_power_domain_on,
};

EVENT_DATA io_cpu_power_domain_implementation_t
vims_power_domain_implementation = {
    .turn_off = io_power_domain_no_operation,
    .turn_on = io_power_domain_no_operation,
};

EVENT_DATA io_cc2652_cpu_power_domain_t always_on_power_domain = {
    .implementation = &cpu_core_power_domain_implementation,
    .prcm_domain_identifier = 0, // none
};

EVENT_DATA io_cc2652_cpu_power_domain_t cpu_core_power_domain = {
    .implementation = &cpu_core_power_domain_implementation,
    .prcm_domain_identifier = PRCM_DOMAIN_MCU, //
};

io_cc2652_cpu_power_domain_t peripheral_power_domain = {
    .implementation = &peripheral_power_domain_implementation,
    .prcm_domain_identifier = PRCM_DOMAIN_PERIPH,
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
    .specialisation_of = &io_cpu_clock_implementation,
    .get_current_frequency = cc2652_hf_rc_24_oscillator_get_current_frequency,
    .get_expected_frequency = cc2652_hf_rc_24_oscillator_get_current_frequency,
    .get_power_domain = get_always_on_io_power_domain,
    .start = cc2652_hf_rc_oscillator_start,
    .stop = NULL,
};

static float64_t
cc2652_hp_oscillator_get_current_frequency (io_cpu_clock_pointer_t this) {
    cc2652_hp_oscillator_t const *c = (cc2652_hp_oscillator_t const*) (
        io_cpu_clock_ro_pointer (this)
    );
    return c->frequency;
}

//
// oscilator registers are accessed via this DDI thing
//
static bool
cc2652_hp_oscillator_start (io_t *io,io_cpu_clock_pointer_t this) {

    return OSC_IsHPOSCEnabled();
/*


    if (!OSC_IsHPOSCEnabled()) {

        OSC_HPOSCInitializeFrequencyOffsetParameters ();


        return true;
    }

    return false;
*/
}

EVENT_DATA io_cpu_clock_implementation_t cc2652_hp_oscillator_implementation = {
    .specialisation_of = &io_cpu_clock_implementation,
    .get_current_frequency = cc2652_hp_oscillator_get_current_frequency,
    .get_expected_frequency = cc2652_hp_oscillator_get_current_frequency,
    .get_power_domain = get_always_on_io_power_domain,
    .start = cc2652_hp_oscillator_start,
    .stop = NULL,
};

bool
cc2652_clock_is_hp_oscillator (io_cpu_clock_pointer_t clock) {
    return io_cpu_clock_has_implementation (clock,&cc2652_hp_oscillator_implementation);
}

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

        // need to use DDI_0_OSC_CTL0_SCLK_LF_SRC_SEL to select
        // connect input clock configuration

        if (cpu_clock_is_cc2652_hf_rc_oscillator (this->input)) {
            OSCClockSourceSet (OSC_SRC_CLK_LF,OSC_RCOSC_HF);
//          HWREG (AUX_DDI0_OSC_BASE + DDI_0_OSC_O_CTL0) &= ~DDI_0_OSC_CTL0_SCLK_LF_SRC_SEL_M;
//          HWREG (AUX_DDI0_OSC_BASE + DDI_0_OSC_O_CTL0) |= DDI_0_OSC_CTL0_SCLK_LF_SRC_SEL_RCOSCHFDLF;
        } else if (cpu_clock_is_cc2652_hp_oscillator (this->input)) {
            OSCClockSourceSet (OSC_SRC_CLK_LF,OSC_XOSC_HF);
        } else {
            return false;
        }

        return true;
    } else {
        return false;
    }
}

EVENT_DATA io_cpu_clock_implementation_t cc2652_sclk_lf_implementation = {
    .specialisation_of = &io_cpu_clock_implementation,
    .get_current_frequency = cc2652_sclk_lf_get_current_frequency,
    .get_expected_frequency = cc2652_sclk_lf_get_current_frequency,
    .get_power_domain = get_always_on_io_power_domain,
    .start = cc2652_sclk_lf_start,
    .stop = NULL,
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
        cc2652_sclk_lf_t const *this = (cc2652_sclk_lf_t const*) (
            io_cpu_clock_ro_pointer (clock)
        );

        if (cpu_clock_is_cc2652_hf_rc_oscillator (this->input)) {
            OSCClockSourceSet(OSC_SRC_CLK_HF,OSC_RCOSC_HF);
            //or OSCHF_SwitchToRcOscTurnOffXosc().
        } else if (cpu_clock_is_cc2652_hp_oscillator (this->input)) {
            OSCClockSourceSet(OSC_SRC_CLK_HF,OSC_XOSC_HF);
            while (!OSCHfSourceReady());
            OSCHfSourceSwitch();
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

EVENT_DATA io_cpu_clock_implementation_t cc2652_core_clock_implementation = {
    .specialisation_of = &io_cpu_clock_implementation,
    .get_current_frequency = cc2652_core_clock_get_current_frequency,
    .get_expected_frequency = cc2652_core_clock_get_current_frequency,
    .get_power_domain = cc2652_core_clock_get_power_domain,
    .start = cc2652_core_clock_start,
    .stop = NULL,
};

static bool
cc2652_peripheral_clock_start (io_t *io,io_cpu_clock_pointer_t clock) {
    if (io_cpu_dependant_clock_start_input (io,clock)) {
        cc2652_peripheral_clock_t const *this = (cc2652_peripheral_clock_t const*) (
            io_cpu_clock_ro_pointer (clock)
        );

        turn_on_io_power_domain (io,io_cpu_clock_power_domain (clock));

        PRCMPeripheralRunEnable(this->prcm_peripheral_id);
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

EVENT_DATA io_cpu_clock_implementation_t cc2652_peripheral_clock_implementation = {
    .specialisation_of = &io_cpu_clock_implementation,
    .get_current_frequency = io_dependant_cpu_clock_get_current_frequency,
    .get_expected_frequency = io_dependant_cpu_clock_get_current_frequency,
    .get_power_domain = cc2652_peripheral_clock_get_power_domain,
    .start = cc2652_peripheral_clock_start,
    .stop = NULL,
};

static io_cpu_power_domain_pointer_t
cc2652_serial_clock_get_power_domain (io_cpu_clock_pointer_t clock) {
    return def_io_cpu_power_domain_pointer (&serial_power_domain);
}

EVENT_DATA io_cpu_clock_implementation_t
cc2652_serial_clock_implementation = {
    .specialisation_of = &io_dependent_clock_implementation,
    .get_current_frequency = io_dependant_cpu_clock_get_current_frequency,
    .get_expected_frequency = io_dependant_cpu_clock_get_current_frequency,
    .get_power_domain = cc2652_serial_clock_get_power_domain,
    .start = cc2652_peripheral_clock_start,
    .stop = NULL,
};

static io_cpu_power_domain_pointer_t
cc2652_radio_clock_get_power_domain (io_cpu_clock_pointer_t clock) {
    return def_io_cpu_power_domain_pointer (&radio_power_domain);
}

static bool
cc2652_radio_clock_start (io_t *io,io_cpu_clock_pointer_t clock) {
    if (io_cpu_dependant_clock_start_input (io,clock)) {
        turn_on_io_power_domain (io,io_cpu_clock_power_domain (clock));
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

static float64_t
cc2652_rtc_clock_get_current_frequency (io_cpu_clock_pointer_t clock) {
    cc2652_rtc_clock_t const *this = (cc2652_rtc_clock_t const*) (
        io_cpu_clock_ro_pointer (clock)
    );
    return io_cpu_clock_get_current_frequency (this->input);
}

static bool
cc2652_rtc_clock_start (io_t *io,io_cpu_clock_pointer_t clock) {
    if (io_cpu_dependant_clock_start_input (io,clock)) {
        return true;
    } else {
        return false;
    }
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


#endif
