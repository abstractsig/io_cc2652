/*
 *
 * provide io compatible interface to cc2652 pins
 *
 */
#ifndef cc2652rb_pins_H_
#define cc2652rb_pins_H_

typedef union PACK_STRUCTURE {
    io_pin_t io;
    uint32_t u32;
    struct PACK_STRUCTURE {
        uint32_t number:6;
        uint32_t ioc_port_id:6;
        uint32_t active_level:1;
        uint32_t initial_state:1;
        uint32_t pull_mode:2;
        uint32_t drive_level:3;
        uint32_t hysteresis:1;
        uint32_t :12;
    } cc;
} cc2652_io_pin_t;

#define cc2652_io_pin_number(pin)               (pin).cc.number
#define cc2652_io_pin_pull_mode(pin)            (pin).cc.pull_mode
#define cc2652_io_pin_active_level(pin)     (pin).cc.active_level
#define cc2652_io_pin_initial_state(pin)        (pin).cc.initial_state
#define cc2652_io_pin_ioc_port_id(pin)          (pin).cc.ioc_port_id

#define IO_PIN_ACTIVE_LEVEL_HIGH        1
#define IO_PIN_ACTIVE_LEVEL_LOW     0

#define IO_PIN_LEVEL_ACTIVE         1
#define IO_PIN_LEVEL_INACTIVE           0

#define IO_PIN_NO_PULL      0       //  IOC_NO_IOPULL
#define IO_PIN_PULL_UP      1       // IOC_IOPULL_UP
#define IO_PIN_PULL_DOWN    2       // IOC_IOPULL_DOWN

#define def_cc2652_io_input_pin(pin_number,active,pull) (cc2652_io_pin_t) {\
        .cc.number = pin_number,\
        .cc.active_level = active,\
        .cc.initial_state = IO_PIN_LEVEL_INACTIVE,\
        .cc.pull_mode = pull,\
        .cc.drive_level = 0,\
        .cc.hysteresis = 0,\
        .cc.ioc_port_id = IOC_PORT_GPIO,\
    }

#define def_cc2652_io_output_pin(pin_number,active,initial) (cc2652_io_pin_t) {\
        .cc.number = pin_number,\
        .cc.active_level = active,\
        .cc.initial_state = initial,\
        .cc.pull_mode = IO_PIN_NO_PULL,\
        .cc.drive_level = 0,\
        .cc.hysteresis = 0,\
        .cc.ioc_port_id = IOC_PORT_GPIO,\
    }

#define def_cc2652_io_alternate_pin(pin_number,active,IOC) (cc2652_io_pin_t) {\
        .cc.number = pin_number,\
        .cc.active_level = active,\
        .cc.initial_state = IO_PIN_LEVEL_INACTIVE,\
        .cc.pull_mode = IO_PIN_NO_PULL,\
        .cc.drive_level = 0,\
        .cc.hysteresis = 0,\
        .cc.ioc_port_id = IOC,\
    }

#define CC2652_INVALID_PIN_NUMBER   0x3f

#define def_cc2652_null_io_pin() (cc2652_io_pin_t) {\
        .cc.number = CC2652_INVALID_PIN_NUMBER,\
        .cc.active_level = IO_PIN_ACTIVE_LEVEL_HIGH,\
        .cc.initial_state = IO_PIN_LEVEL_INACTIVE,\
        .cc.pull_mode = IO_PIN_NO_PULL,\
        .cc.drive_level = 0,\
        .cc.hysteresis = 0,\
        .cc.ioc_port_id = IOC_PORT_GPIO,\
    }



#ifdef IMPLEMENT_IO_CPU
//-----------------------------------------------------------------------------
//
// implementation
//
//-----------------------------------------------------------------------------

static void
cc2652_write_to_io_pin (io_t *io,io_pin_t rpin,int32_t state) {
    cc2652_io_pin_t pin = {rpin};
    if (state ^ cc2652_io_pin_active_level (pin)) {
         GPIO_writeDio (cc2652_io_pin_number(pin),0);
    } else {
         GPIO_writeDio (cc2652_io_pin_number(pin),1);
    }
}


static void
cc2652_configure_io_pin_as_alternate (cc2652_io_pin_t pin) {
    IOCPortConfigureSet (
        cc2652_io_pin_number (pin),
        cc2652_io_pin_ioc_port_id (pin),
        (
                IOC_NO_IOPULL
            |   0
        )
    );
}

static void
cc2652_configure_io_pin_as_input (cc2652_io_pin_t pin) {
    switch (cc2652_io_pin_pull_mode(pin)) {
        case IO_PIN_NO_PULL:
            IOCIOPortPullSet (cc2652_io_pin_number(pin),IOC_NO_IOPULL);
        break;

        case IO_PIN_PULL_UP:
            IOCIOPortPullSet (cc2652_io_pin_number(pin),IOC_IOPULL_UP);
        break;

        case IO_PIN_PULL_DOWN:
            IOCIOPortPullSet (cc2652_io_pin_number(pin),IOC_IOPULL_DOWN);
        break;
    }
    IOCPinTypeGpioInput(cc2652_io_pin_number(pin));
}

static void
cc2652_configure_io_pin_as_output (cc2652_io_pin_t pin) {
    IOCIOPortPullSet (cc2652_io_pin_number(pin),IOC_NO_IOPULL);
    IOCPinTypeGpioOutput(cc2652_io_pin_number(pin));
}


void
cc2652_toggle_io_pin (io_t *io,io_pin_t rpin) {
    cc2652_io_pin_t pin = {rpin};
    GPIO_toggleDio(cc2652_io_pin_number(pin));
}

int32_t
cc2652_read_io_input_pin (io_t *io,io_pin_t rpin) {
    cc2652_io_pin_t pin = {rpin};
    return GPIO_readDio(cc2652_io_pin_number(pin));
}

void
cc2652_set_io_pin_to_output (io_t *io,io_pin_t rpin) {
    cc2652_io_pin_t pin = {rpin};
    cc2652_start_gpio_clock (io);
    cc2652_write_to_io_pin (io,rpin,cc2652_io_pin_initial_state(pin));
    cc2652_configure_io_pin_as_output (pin);
}

void
cc2652_set_io_pin_to_input (io_t *io,io_pin_t rpin) {
    cc2652_io_pin_t pin = {rpin};
    cc2652_start_gpio_clock (io);
    cc2652_configure_io_pin_as_input (pin);
}

void
cc2652_set_io_pin_to_alternate (io_t *io,io_pin_t rpin) {
    cc2652_io_pin_t pin = {rpin};
    cc2652_start_gpio_clock (io);
    cc2652_configure_io_pin_as_alternate (pin);
}

//
// so we can turn clock and power off
//
void
cc2652_release_io_pin (io_t *io,io_pin_t rpin) {
}

bool
cc2652_io_pin_is_valid (io_t *io,io_pin_t rpin) {
    cc2652_io_pin_t pin = {rpin};
    return cc2652_io_pin_number(pin) != CC2652_INVALID_PIN_NUMBER;
}

void
cc2652_set_io_pin_interrupt (io_t *io,io_pin_t rpin,io_interrupt_handler_t *h) {
/*
    switch (base3_gpio_pin_config_interrupt(pin)) {
      case GPIO_INTERRUPT_RISING:
            IOCIOIntSet (
                 base3_gpio_pin_config_number(pin),IOC_INT_ENABLE,IOC_RISING_EDGE
            );
      break;
      case GPIO_INTERRUPT_FALLING:
            IOCIOIntSet (
                 base3_gpio_pin_config_number(pin),IOC_INT_ENABLE,IOC_FALLING_EDGE
            );
      break;
      case GPIO_INTERRUPT_BOTH:
            IOCIOIntSet (
                 base3_gpio_pin_config_number(pin),IOC_INT_ENABLE,IOC_BOTH_EDGES
            );
      break;
    }
*/
}

#endif /* IMPLEMENT_IO_CPU */
#endif
