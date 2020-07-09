/*
 *
 * provide io compatible interface to cc2652 pins
 *
 */
#ifndef cc2652rb_pins_H_
#define cc2652rb_pins_H_

#include <ti/driverlib/ioc.h>

typedef union PACK_STRUCTURE {
    io_pin_t io;
    uint32_t u32;
    struct PACK_STRUCTURE {
        uint32_t number:6;
        uint32_t ioc_port_id:6;
        uint32_t direction:1;
        uint32_t io_mode:3;
        uint32_t active_level:1;
        uint32_t initial_state:1;
        uint32_t pull_mode:2;
        uint32_t io_drive_current:2;
        uint32_t io_drive_source:2;
        uint32_t :8;
    } cc;
} cc2652_io_pin_t;

#define cc2652_io_pin_number(pin)				(pin).cc.number
#define cc2652_io_pin_direction(pin)			(pin).cc.direction
#define cc2652_io_pin_mode(pin)					(pin).cc.io_mode
#define cc2652_io_pin_pull_mode(pin)			(pin).cc.pull_mode
#define cc2652_io_pin_active_level(pin)		(pin).cc.active_level
#define cc2652_io_pin_initial_state(pin)		(pin).cc.initial_state
#define cc2652_io_pin_ioc_port_id(pin)			(pin).cc.ioc_port_id

void cc2652_set_io_pin_interrupt (io_t*,io_pin_t,io_interrupt_handler_t*);
void cc2652_write_to_io_pin (io_t*,io_pin_t,int32_t);

#define IO_PIN_ACTIVE_LEVEL_HIGH        1
#define IO_PIN_ACTIVE_LEVEL_LOW     0

#define IO_PIN_LEVEL_ACTIVE         1
#define IO_PIN_LEVEL_INACTIVE           0

#define def_cc2652_io_input_pin(pin_number,active,pull) (cc2652_io_pin_t) {\
        .cc.number = pin_number,\
		  .cc.direction = 1,\
        .cc.active_level = active,\
        .cc.initial_state = IO_PIN_LEVEL_INACTIVE,\
        .cc.pull_mode = pull,\
        .cc.io_drive_current = 0,\
        .cc.io_drive_source = 0,\
        .cc.ioc_port_id = IOC_PORT_GPIO,\
    }

#define def_cc2652_io_output_pin(pin_number,active,initial) (cc2652_io_pin_t) {\
        .cc.number = pin_number,\
		  .cc.direction = 0,\
		  .cc.io_mode = IO_PIN_MODE_NORMAL,\
        .cc.active_level = active,\
        .cc.initial_state = initial,\
        .cc.pull_mode = IO_PIN_NO_PULL,\
        .cc.io_drive_current = 0,\
        .cc.io_drive_source = 0,\
        .cc.ioc_port_id = IOC_PORT_GPIO,\
    }

#define def_cc2652_io_alternate_pin(pin_number,dir,mode,pull,active,IOC) (cc2652_io_pin_t) {\
        .cc.number = pin_number,\
		  .cc.direction = dir,\
		  .cc.io_mode = mode,\
        .cc.active_level = active,\
        .cc.initial_state = IO_PIN_LEVEL_INACTIVE,\
        .cc.pull_mode = pull,\
        .cc.io_drive_current = 0,\
        .cc.io_drive_source = 0,\
        .cc.ioc_port_id = IOC,\
    }

#define CC2652_INVALID_PIN_NUMBER   0x3f

#define def_cc2652_null_io_pin() (cc2652_io_pin_t) {\
        .cc.number = CC2652_INVALID_PIN_NUMBER,\
        .cc.active_level = IO_PIN_ACTIVE_LEVEL_HIGH,\
        .cc.initial_state = IO_PIN_LEVEL_INACTIVE,\
        .cc.pull_mode = IO_PIN_NO_PULL,\
        .cc.io_drive_current = 0,\
        .cc.io_drive_source = 0,\
        .cc.ioc_port_id = IOC_PORT_GPIO,\
    }


#define IO_PIN_MODE(m)							((m) >> 24)
#define IO_PIN_MODE_NORMAL						IO_PIN_MODE(IOC_IOMODE_NORMAL)
#define IO_PIN_MODE_INVERTED					IO_PIN_MODE(IOC_IOMODE_INV)
#define IO_PIN_MODE_OPEN_DRAIN_NORMAL		IO_PIN_MODE(IOC_IOMODE_OPEN_DRAIN_NORMAL)
#define IO_PIN_MODE_OPEN_DRAIN_INVERTED	IO_PIN_MODE(IOC_IOMODE_OPEN_DRAIN_INV)
#define IO_PIN_MODE_OPEN_SOURCE_NORMAL		IO_PIN_MODE(IOC_IOMODE_OPEN_SRC_NORMAL)
#define IO_PIN_MODE_OPEN_SOURCE_INVERTED	IO_PIN_MODE(IOC_IOMODE_OPEN_SRC_INV)

#define IO_PIN_CURRENT(m)						((m) >> 10)
#define IO_PIN_CURRENT_2MA						IO_PIN_CURRENT(IOC_CURRENT_2MA)
#define IO_PIN_CURRENT_4MA						IO_PIN_CURRENT(IOC_CURRENT_4MA)
#define IO_PIN_CURRENT_8MA						IO_PIN_CURRENT(IOC_CURRENT_8MA)

#define IO_PIN_NO_PULL							(IOC_NO_IOPULL >> 13)
#define IO_PIN_PULL_UP							(IOC_IOPULL_UP >> 13)
#define IO_PIN_PULL_DOWN						(IOC_IOPULL_DOWN >> 13)

#define IO_PIN_DIRECTION_INPUT				1
#define IO_PIN_DIRECTION_OUTPUT				0

#ifdef IMPLEMENT_IO_CPU
//-----------------------------------------------------------------------------
//
// implementation
//
//-----------------------------------------------------------------------------

void
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
				(cc2652_io_pin_pull_mode (pin) << 13)
			|	(cc2652_io_pin_mode (pin) << 24)
			|	(IOC_CURRENT_2MA << 10)
			|	(IOC_STRENGTH_AUTO)
			|	(cc2652_io_pin_direction(pin) ? IOC_INPUT_ENABLE : 0)
			|	IOC_SLEW_DISABLE
			|	IOC_HYST_DISABLE
			|	IOC_NO_EDGE
			|	IOC_INT_DISABLE
			|	IOC_NO_WAKE_UP
	  )
	);
}

static void
cc2652_configure_io_pin_as_input (cc2652_io_pin_t pin) {
	IOCIOPortPullSet (
		cc2652_io_pin_number(pin),
		(cc2652_io_pin_pull_mode(pin) << 13)
	);
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
   cc2652_io_pin_t pin = {rpin};
	cc2652_io_pin_t unused = def_cc2652_io_input_pin (
		cc2652_io_pin_number(pin),IO_PIN_ACTIVE_LEVEL_LOW,IO_PIN_NO_PULL
	);
	cc2652_configure_io_pin_as_input (unused);
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
