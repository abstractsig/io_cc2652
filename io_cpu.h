/*
 *
 * io cpu
 *
 */
#ifndef io_cpu_H_
#define io_cpu_H_
#include <io_core.h>

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

typedef struct PACK_STRUCTURE cc2652_hp_oscillator {
	IO_CPU_CLOCK_SOURCE_STRUCT_MEMBERS
	float64_t frequency;
} cc2652_hp_oscillator_t;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_hp_oscillator_implementation;

typedef struct cc2652_core_clock {
	IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS
} cc2652_core_clock_t;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_core_clock_implementation;

typedef struct cc2652_peripheral_clock {
	IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS
	uint32_t prcm_peripheral_id;
} cc2652_peripheral_clock_t;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_peripheral_clock_implementation;
extern EVENT_DATA io_cpu_clock_implementation_t cc2652_serial_clock_implementation;

//
// pins
//

typedef union PACK_STRUCTURE {
	io_pin_t io;
	uint32_t u32;
	struct PACK_STRUCTURE {
		uint32_t	number:6;
		uint32_t ioc_port_id:6;
		uint32_t active_level:1;
		uint32_t initial_state:1;
		uint32_t pull_mode:2;
		uint32_t drive_level:3;
		uint32_t hysteresis:1;
		uint32_t :12;
	} cc;
} cc2652_io_pin_t;

#define cc2652_io_pin_number(pin)				(pin).cc.number
#define cc2652_io_pin_pull_mode(pin)			(pin).cc.pull_mode
#define cc2652_io_pin_active_level(pin)		(pin).cc.active_level
#define cc2652_io_pin_initial_state(pin)		(pin).cc.initial_state
#define cc2652_io_pin_ioc_port_id(pin)			(pin).cc.ioc_port_id

#define IO_PIN_ACTIVE_LEVEL_HIGH		1
#define IO_PIN_ACTIVE_LEVEL_LOW		0

#define IO_PIN_LEVEL_ACTIVE			1
#define IO_PIN_LEVEL_INACTIVE			0

#define IO_PIN_NO_PULL		0		//	IOC_NO_IOPULL
#define IO_PIN_PULL_UP		1		// IOC_IOPULL_UP
#define IO_PIN_PULL_DOWN	2		// IOC_IOPULL_DOWN

#define def_cc2652_io_input_pin(pin_number,active,pull) (cc2652_io_pin_t) {\
		.cc.number = pin_number,\
		.cc.active_level = active,\
		.cc.initial_state = IO_PIN_LEVEL_INACTIVE,\
		.cc.pull_mode = pull,\
		.cc.drive_level = 0,\
		.cc.hysteresis = 0,\
		.cc.ioc_port_id = 0,\
	}

#define def_cc2652_io_output_pin(pin_number,active,initial) (cc2652_io_pin_t) {\
		.cc.number = pin_number,\
		.cc.active_level = active,\
		.cc.initial_state = initial,\
		.cc.pull_mode = IO_PIN_NO_PULL,\
		.cc.drive_level = 0,\
		.cc.hysteresis = 0,\
		.cc.ioc_port_id = 0,\
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

#define CC2652_INVALID_PIN_NUMBER	0x3f

#define def_cc2652_null_io_pin() (cc2652_io_pin_t) {\
		.cc.number = CC2652_INVALID_PIN_NUMBER,\
		.cc.active_level = IO_PIN_ACTIVE_LEVEL_HIGH,\
		.cc.initial_state = IO_PIN_LEVEL_INACTIVE,\
		.cc.pull_mode = IO_PIN_NO_PULL,\
		.cc.drive_level = 0,\
		.cc.hysteresis = 0,\
		.cc.ioc_port_id = 0,\
	}

//
// cpu
//
#define CC2652_IO_CPU_STRUCT_MEMBERS \
	IO_STRUCT_MEMBERS				\
	io_value_memory_t *vm;\
	io_byte_memory_t *bm;\
	uint32_t in_event_thread;\
	io_value_pipe_t *tasks;\
	io_cpu_clock_pointer_t gpio_clock; \
	/**/

typedef struct PACK_STRUCTURE io_cc2652_cpu {
	CC2652_IO_CPU_STRUCT_MEMBERS
} io_cc2652_cpu_t;

void	initialise_cpu_io (io_t*);

//
// sockets
//

typedef struct PACK_STRUCTURE cc2652_uart {
	IO_SOCKET_STRUCT_MEMBERS
	
	io_t *io;
	io_encoding_implementation_t const *encoding;
	io_cpu_clock_pointer_t peripheral_clock;

	io_encoding_pipe_t *tx_pipe;
	io_event_t transmit_complete;
	io_byte_pipe_t *rx_pipe;

	cc2652_io_pin_t tx_pin;
	cc2652_io_pin_t rx_pin;
	cc2652_io_pin_t rts_pin;
	cc2652_io_pin_t cts_pin;
	
	uint32_t register_base_address;
	int32_t interrupt_number;
	uint32_t baud_rate;

} cc2652_uart_t;

#ifdef IMPLEMENT_IO_CPU
//-----------------------------------------------------------------------------
//
// cc2652 Implementtaion
//
//-----------------------------------------------------------------------------
#include <cc2652rb.h>
typedef int32_t IRQn_Type;

#define SysTick_IRQn (INT_SYSTICK - 16)

#include <core_cm4.h>

#define NUMBER_OF_ARM_INTERRUPT_VECTORS	16L
#define NUMBER_OF_NRF_INTERRUPT_VECTORS	NUM_INTERRUPTS
#define NUMBER_OF_INTERRUPT_VECTORS	(NUMBER_OF_ARM_INTERRUPT_VECTORS + NUMBER_OF_NRF_INTERRUPT_VECTORS)

static io_interrupt_handler_t cpu_interrupts[NUMBER_OF_INTERRUPT_VECTORS];

#define ENABLE_INTERRUPTS	\
	do {	\
		__DMB();	\
		__enable_irq();	\
	} while (0)

#define DISABLE_INTERRUPTS	\
	do {	\
		__disable_irq();	\
		__DMB();	\
	} while (0)


static void
null_interrupt_handler (void *w) {
	while(1);
}

//
// power domains
//
/*
 *-----------------------------------------------------------------------------
 *
 * power domains
 *
 *-----------------------------------------------------------------------------
 */

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

static void
turn_power_domain_off_nop (io_t *io,io_cpu_power_domain_pointer_t pd) {
}

static void
turn_power_domain_on_nop (io_t *io,io_cpu_power_domain_pointer_t pd) {
}

EVENT_DATA io_cpu_power_domain_implementation_t
always_on_power_domain_implementation = {
	.turn_off = turn_power_domain_off_nop,
	.turn_on = turn_power_domain_on_nop,
};

EVENT_DATA io_cpu_power_domain_implementation_t
cpu_core_power_domain_implementation = {
	.turn_off = turn_power_domain_off_nop,
	.turn_on = turn_power_domain_on_nop,
};

EVENT_DATA io_cpu_power_domain_implementation_t
bus_power_domain_implementation = {
	.turn_off = turn_power_domain_off_nop,
	.turn_on = turn_power_domain_on_nop,
};

static void
turn_peripheral_power_domain_on (io_t *io,io_cpu_power_domain_pointer_t pd) {
	controlled_power_domain_turn_on (pd);
}

EVENT_DATA io_cpu_power_domain_implementation_t
peripheral_power_domain_implementation = {
	.turn_off = turn_power_domain_off_nop,
	.turn_on = turn_peripheral_power_domain_on,
};

EVENT_DATA io_cpu_power_domain_implementation_t
radio_power_domain_implementation = {
	.turn_off = turn_power_domain_off_nop,
	.turn_on = turn_power_domain_on_nop,
};

static void
turn_serial_power_domain_on (io_t *io,io_cpu_power_domain_pointer_t pd) {
	controlled_power_domain_turn_on (pd);
}

static EVENT_DATA io_cpu_power_domain_implementation_t
serial_power_domain_implementation = {
	.turn_off = turn_power_domain_off_nop,
	.turn_on = turn_serial_power_domain_on,
};

EVENT_DATA io_cpu_power_domain_implementation_t
vims_power_domain_implementation = {
	.turn_off = turn_power_domain_off_nop,
	.turn_on = turn_power_domain_on_nop,
};

EVENT_DATA io_cc2652_cpu_power_domain_t cpu_core_power_domain = {
	.implementation = &cpu_core_power_domain_implementation,
	.prcm_domain_identifier = PRCM_DOMAIN_MCU, // no?
};

io_cc2652_cpu_power_domain_t peripheral_power_domain = {
	.implementation = &peripheral_power_domain_implementation,
	.prcm_domain_identifier = PRCM_DOMAIN_PERIPH,
};

io_cc2652_cpu_power_domain_t serial_power_domain = {
	.implementation = &serial_power_domain_implementation,
	.prcm_domain_identifier = PRCM_DOMAIN_SERIAL,
};

//
// clocks
//

static bool
cc2652_hf_rc_oscillator_start (io_t *io,io_cpu_clock_pointer_t this) {
	return true;
}

static float64_t
cc2652_hf_rc_48_oscillator_get_frequency (io_cpu_clock_pointer_t this) {
	return 48000000.0;
}

EVENT_DATA io_cpu_clock_implementation_t
cc2652_hf_rc_48_oscillator_implementation = {
	.specialisation_of = &io_cpu_clock_implementation,
	.get_frequency = cc2652_hf_rc_48_oscillator_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = cc2652_hf_rc_oscillator_start,
	.stop = NULL, // alwary on
};

static float64_t
cc2652_hf_rc_24_oscillator_get_frequency (io_cpu_clock_pointer_t this) {
	return 24000000.0;
}

EVENT_DATA io_cpu_clock_implementation_t
cc2652_hf_rc_24_oscillator_implementation = {
	.specialisation_of = &io_cpu_clock_implementation,
	.get_frequency = cc2652_hf_rc_24_oscillator_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = cc2652_hf_rc_oscillator_start,
	.stop = NULL, // alwary on
};


static float64_t
cc2652_hp_oscillator_get_frequency (io_cpu_clock_pointer_t this) {
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
		
	if ((HWREG (AUX_DDI0_OSC_BASE + DDI_0_OSC_O_CTL0) & DDI_0_OSC_CTL0_HPOSC_MODE_EN_M) == 0) {
		
		HWREG (AUX_DDI0_OSC_BASE + DDI_0_OSC_O_CTL0) = (
				(HWREG (AUX_DDI0_OSC_BASE + DDI_0_OSC_O_CTL0) & ~DDI_0_OSC_CTL0_HPOSC_MODE_EN_M) 
			| 	(DDI_0_OSC_CTL0_HPOSC_MODE_EN << DDI_0_OSC_CTL0_HPOSC_MODE_EN_S)
		);
		while ((HWREG (AUX_DDI0_OSC_BASE + DDI_0_OSC_O_STAT1) & DDI_0_OSC_STAT1_SCLK_HF_GOOD) == 0);

		return true;
	}
	
	return false;
}

EVENT_DATA io_cpu_clock_implementation_t cc2652_hp_oscillator_implementation = {
	.specialisation_of = &io_cpu_clock_implementation,
	.get_frequency = cc2652_hp_oscillator_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = cc2652_hp_oscillator_start,
	.stop = NULL,
};

bool
cc2652_clock_is_hp_oscillator (io_cpu_clock_pointer_t clock) {
	return io_cpu_clock_has_implementation (clock,&cc2652_hp_oscillator_implementation);
}

static float64_t
cc2652_core_clock_get_frequency (io_cpu_clock_pointer_t clock) {
	cc2652_core_clock_t const *this = (cc2652_core_clock_t const*) (
		io_cpu_clock_ro_pointer (clock)
	);
	return io_cpu_clock_get_frequency (this->input);
}

static bool
cc2652_core_clock_start (io_t *io,io_cpu_clock_pointer_t clock) {
	if (io_cpu_dependant_clock_start_input (io,clock)) {

		// ???
		
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
	.get_frequency = cc2652_core_clock_get_frequency,
	.get_power_domain = cc2652_core_clock_get_power_domain,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
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
	.get_frequency = io_dependant_cpu_clock_get_frequency,
	.get_power_domain = cc2652_peripheral_clock_get_power_domain,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = cc2652_peripheral_clock_start,
	.stop = NULL,
};

static io_cpu_power_domain_pointer_t
cc2652_serial_clock_get_power_domain (io_cpu_clock_pointer_t clock) {
	return def_io_cpu_power_domain_pointer (&serial_power_domain);
}

EVENT_DATA io_cpu_clock_implementation_t cc2652_serial_clock_implementation = {
	.specialisation_of = &io_cpu_clock_implementation,
	.get_frequency = io_dependant_cpu_clock_get_frequency,
	.get_power_domain = cc2652_serial_clock_get_power_domain,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = cc2652_peripheral_clock_start,
	.stop = NULL,
};

//
// pins
//

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
cc2652_configure_io_pin_as_output (cc2652_io_pin_t pin) {
	IOCIOPortPullSet (cc2652_io_pin_number(pin),IOC_NO_IOPULL);
	IOCPinTypeGpioOutput(cc2652_io_pin_number(pin));
}

void
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
cc2652_configure_io_pin_as_alternate (cc2652_io_pin_t pin) {

	IOCPinSetIoCfgMux (
		cc2652_io_pin_number (pin),
		cc2652_io_pin_ioc_port_id (pin)
	);
}

//
// sockets
//

static bool
cc2652_uart_output_next_buffer (cc2652_uart_t *this) {
	io_encoding_t *next;
	if (
			UARTisEnabled (this->register_base_address)
		&& io_encoding_pipe_peek (this->tx_pipe,&next)
	) {
		const uint8_t *byte,*end;
		io_encoding_get_ro_bytes (next,&byte,&end);
		
		while (byte < end) {
			UARTCharPut (this->register_base_address,*byte++);
		}
		
		io_enqueue_event (this->io,&this->transmit_complete);
		return true;
	} else {
		return false;
	}
}

static void
cc2652_uart_output_event_handler (io_event_t *ev) {
	cc2652_uart_t *this = ev->user_value;
	io_encoding_t *next;
	
	if (io_encoding_pipe_get_encoding (this->tx_pipe,&next)) {
		unreference_io_encoding (next);
	} else {
		io_panic (this->io,IO_PANIC_SOMETHING_BAD_HAPPENED);
	}
	
	if (
			!cc2652_uart_output_next_buffer (this)
		&&	UARTisEnabled (this->register_base_address)
		&& io_event_is_valid (io_pipe_event (this->tx_pipe))
	) {
		io_enqueue_event (this->io,io_pipe_event (this->tx_pipe));
	}
}

void
cc2652_uart_interrupt (void *user_value) {
	cc2652_uart_t *this = user_value;
	uint32_t status = HWREG(this->register_base_address + UART_O_MIS);

	if (status & ( UART_MIS_RTMIS | UART_MIS_RXMIS)) {		
		// clear interrupt
		HWREG (this->register_base_address + UART_O_ICR) &= ~UART_ICR_RXIC;

		while (!(HWREG(this->register_base_address + UART_O_FR) & UART_FR_RXFE)) {
			uint8_t byte = HWREG (this->register_base_address + UART_O_DR);
			io_byte_pipe_put_byte (this->rx_pipe,byte);
		}
		io_enqueue_event (this->io,io_pipe_event (this->rx_pipe));
	}

	if (status & UART_MIS_TXMIS) {		
		// clear interrupt
		HWREG (this->register_base_address + UART_O_ICR) &= ~UART_ICR_TXIC;
/*
		while (!(HWREG(base + UART_O_FR) & UART_FR_TXFF)) {
			if (!pipe_get_element (this->tx_buffer,byte)) {
				// tx done, disable tx interrupt
				HWREG (base + UART_O_IMSC) &= ~UART_IMSC_TXIM;
				ccm3_uart_tx_is_busy (this) = 0;
				break;
			}
		}
*/
	}

	if (HWREG(this->register_base_address + UART_O_MIS) != 0) {
		//panic("uart error");
	}

}

// has 32byte rx and tx fifos
static io_socket_t*
cc2652_uart_initialise (
	io_socket_t *socket,io_t *io,io_socket_constructor_t const *C
) {
	cc2652_uart_t *this = (cc2652_uart_t*) socket;
	this->io = io;

	this->tx_pipe = mk_io_encoding_pipe (io_get_byte_memory(io),C->transmit_pipe_length);
	//initialise_io_event (&this->signal_transmit_available,NULL,this);

	this->rx_pipe = mk_io_byte_pipe (
		io_get_byte_memory(io),io_socket_constructor_receive_pipe_length(C)
	);

	initialise_io_event (
		&this->transmit_complete,cc2652_uart_output_event_handler,this
	);

	// enable interrupts
	
	register_io_interrupt_handler (
		io,this->interrupt_number,cc2652_uart_interrupt,this
	);
	
	return socket;
}

static io_t*
cc2652_uart_get_io (io_socket_t *socket) {
	cc2652_uart_t *this = (cc2652_uart_t*) socket;
	return this->io;
}

void
cc2652_uart_set_baud_rate (cc2652_uart_t *this) {
	
	float64_t freq = io_cpu_clock_get_frequency (this->peripheral_clock);
	uint32_t clock = (uint32_t) freq;
	uint32_t base = this->register_base_address;
	uint32_t div = (((clock * 8) / this->baud_rate) + 1) / 2;//(clock << 6) / (this->baud_rate * 16);
	
	HWREG(base + UART_O_IBRD) = div / 64;
	HWREG(base + UART_O_FBRD) = div % 64;
}

static bool
cc2652_uart_open (io_socket_t *socket) {
	cc2652_uart_t *this = (cc2652_uart_t*) socket;
	
	if (io_cpu_clock_start (this->io,this->peripheral_clock)) {
		if (!UARTisEnabled (this->register_base_address)) {
		
			io_set_pin_to_output (this->io,this->tx_pin.io);
			io_set_pin_to_alternate (this->io,this->tx_pin.io);
			io_set_pin_to_input (this->io,this->rx_pin.io);
			io_set_pin_to_alternate (this->io,this->rx_pin.io);
			
			cc2652_uart_set_baud_rate (this);
			HWREG(this->register_base_address + UART_O_LCRH) = (
					UART_LCRH_WLEN_8
				|	UART_LCRH_STP1
				|	UART_LCRH_NO_PARITY
			);

			/*
			HWREG(this->register_base_address + UART_O_IFLS) = (
					UART_IFLS_RXSEL_7_8
				|	UART_IFLS_TXSEL_1_8
			);
			*/
			
			HWREG (this->register_base_address + UART_O_IMSC) |= (
					UART_IMSC_RXIM
			//	|	UART_IMSC_TXIM
			);
			
			// see UART_O_DMACTL
			
			UARTEnable (this->register_base_address);
			
			{
				int32_t irqn = CMSIS_IRQn(this->interrupt_number);
				NVIC_SetPriority (irqn,NORMAL_INTERRUPT_PRIORITY);
				NVIC_ClearPendingIRQ (irqn);
				NVIC_EnableIRQ (irqn);
			}
		}
		return true;
	}

	return false;
}

static void
cc2652_uart_close (io_socket_t *socket) {
}

static io_event_t*
cc2652_uart_bindr (io_socket_t *socket,io_event_t *rx) {
	return NULL;
}

static io_pipe_t*
cc2652_uart_bindt (io_socket_t *socket,io_event_t *ev) {
	return NULL;
}

static io_encoding_t*
cc2652_uart_new_message (io_socket_t *socket) {
	cc2652_uart_t *this = (cc2652_uart_t*) socket;
	return reference_io_encoding (
		new_io_encoding (this->encoding,io_get_byte_memory(this->io))
	);
}

static bool
cc2652_uart_send_message (io_socket_t *socket,io_encoding_t *encoding) {
	if (is_io_binary_encoding (encoding)) {
		cc2652_uart_t *this = (cc2652_uart_t*) socket;
		if (io_encoding_pipe_put_encoding (this->tx_pipe,encoding)) {
			if (io_encoding_pipe_count_occupied_slots (this->tx_pipe) == 1) {
				cc2652_uart_output_next_buffer (this);
			}
			return true;
		} else {
			unreference_io_encoding (encoding);
			return false;
		}
	} else {
		return false;
	}
}

static size_t
cc2652_uart_mtu (io_socket_t const *socket) {
	return 1024;
}	

EVENT_DATA io_socket_implementation_t cc2652_uart_implementation = {
	.specialisation_of = NULL,
	.initialise = cc2652_uart_initialise,
	.free = NULL,
	.get_io = cc2652_uart_get_io,
	.open = cc2652_uart_open,
	.close = cc2652_uart_close,
	.bindr = cc2652_uart_bindr,
	.bindt = cc2652_uart_bindt,
	.new_message = cc2652_uart_new_message,
	.send_message = cc2652_uart_send_message,
	.iterate_inner_sockets = NULL,
	.iterate_outer_sockets = NULL,
	.mtu = cc2652_uart_mtu,
};

//
// io methods
//
static io_byte_memory_t*
cc2652_io_get_byte_memory (io_t *io) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;
	return this->bm;
}

static io_value_memory_t*
cc2652_io_get_stvm (io_t *io) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;
	return this->vm;
}

static void
cc2652_do_gc (io_t *io,int32_t count) {
	io_value_memory_do_gc (io_get_short_term_value_memory (io),count);
}

static void
cc2652_signal_task_pending (io_t *io) {
	// no action required
}

static void
cc2652_signal_event_pending (io_t *io) {
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

static bool
cc2652_enqueue_task (io_t *io,vref_t r_task) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;
	return io_value_pipe_put_value (this->tasks,r_task);
}

static bool
cc2652_enter_critical_section (io_t *env) {
	uint32_t interrupts_are_enabled = !(__get_PRIMASK() & 0x1);
	DISABLE_INTERRUPTS;
	return interrupts_are_enabled;
}

void
cc2652_exit_critical_section (io_t *env,bool were_enabled) {
	if (were_enabled) {
		ENABLE_INTERRUPTS;
	}
}

static bool
cc2652_is_in_event_thread (io_t *io) {
	return ((io_cc2652_cpu_t*) io)->in_event_thread;
}

static void
cc2652_wait_for_event (io_t *io) {
	__WFI();
}

static void
cc2652_for_all_events (io_t *io) {
	io_event_t *event;
	io_alarm_t *alarm;
	do {
		ENTER_CRITICAL_SECTION(io);
		event = io->events;
		alarm = io->alarms;
		EXIT_CRITICAL_SECTION(io);
	} while (
			event != &s_null_io_event
		&&	alarm != &s_null_io_alarm
	);
}

static void	
cc2652_register_interrupt_handler (
	io_t *io,int32_t number,io_interrupt_action_t handler,void *user_value
) {
	io_interrupt_handler_t *i = cpu_interrupts + number;
	i->action = handler;
	i->user_value = user_value;
}

static bool	
cc2652_unregister_interrupt_handler (
	io_t *io,int32_t number,io_interrupt_action_t handler
) {
	io_interrupt_handler_t *i = cpu_interrupts + number;
	if (i->action == handler) {
		i->action = null_interrupt_handler;
		i->user_value = io;
		return true;
	} else {
		return false;
	}
}

//
// need power domain and clock enabled ...
// same for all pins
//
static void
cc2652_start_gpio_clock (io_t *io) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;
	io_cpu_clock_start (io,this->gpio_clock);
}

static void
cc2652_set_io_pin_to_output (io_t *io,io_pin_t rpin) {
	cc2652_io_pin_t pin = {rpin};
	cc2652_start_gpio_clock (io);
	cc2652_write_to_io_pin (io,rpin,cc2652_io_pin_initial_state(pin));	
	cc2652_configure_io_pin_as_output (pin);
}

static void
cc2652_set_io_pin_to_input (io_t *io,io_pin_t rpin) {
	cc2652_io_pin_t pin = {rpin};
	cc2652_start_gpio_clock (io);
	cc2652_configure_io_pin_as_input (pin);
}

static void
cc2652_set_io_pin_to_alternate (io_t *io,io_pin_t rpin) {
	cc2652_io_pin_t pin = {rpin};
	cc2652_start_gpio_clock (io);
	cc2652_configure_io_pin_as_alternate (pin);
}


//
// so we can turn clock and power off
//
static void
cc2652_release_io_pin (io_t *io,io_pin_t rpin) {
}

static bool
cc2652_io_pin_is_valid (io_t *io,io_pin_t rpin) {
	cc2652_io_pin_t pin = {rpin};
	return cc2652_io_pin_number(pin) != CC2652_INVALID_PIN_NUMBER;
}

static void
cc2652_set_io_pin_interrupt (io_t *io,io_pin_t rpin) {
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

static void
cc2652_panic (io_t *io,int code) {
	DISABLE_INTERRUPTS;
	while (1);
}

static void
cc2652_log (io_t *io,char const *fmt,va_list va) {
	// ...
}

void
add_io_implementation_cpu_methods (io_implementation_t *io_i) {
	add_io_implementation_core_methods (io_i);

	io_i->get_byte_memory = cc2652_io_get_byte_memory;
	io_i->get_short_term_value_memory = cc2652_io_get_stvm;
	io_i->do_gc = cc2652_do_gc;
	io_i->signal_task_pending = cc2652_signal_task_pending;
	io_i->enqueue_task = cc2652_enqueue_task;
//	io_i->do_next_task = cc2652_do_next_task;
	io_i->signal_event_pending = cc2652_signal_event_pending;
	io_i->enter_critical_section = cc2652_enter_critical_section;
	io_i->exit_critical_section = cc2652_exit_critical_section;
	io_i->in_event_thread = cc2652_is_in_event_thread;
	io_i->wait_for_event = cc2652_wait_for_event;
//	io_i->get_time = cc2652_get_time,
//	io_i->enqueue_alarm = nrf_time_clock_enqueue_alarm;
//	io_i->dequeue_alarm = nrf_time_clock_dequeue_alarm;
	io_i->register_interrupt_handler = cc2652_register_interrupt_handler;
	io_i->unregister_interrupt_handler = cc2652_unregister_interrupt_handler;
	io_i->wait_for_all_events = cc2652_for_all_events;
	io_i->set_io_pin_output = cc2652_set_io_pin_to_output,
	io_i->set_io_pin_input = cc2652_set_io_pin_to_input,
	io_i->set_io_pin_alternate = cc2652_set_io_pin_to_alternate,
	io_i->set_io_pin_interrupt = cc2652_set_io_pin_interrupt,
//	io_i->read_from_io_pin = cc2652_read_io_input_pin,
	io_i->write_to_io_pin = cc2652_write_to_io_pin,
//	io_i->toggle_io_pin = cc2652_toggle_io_pin,
	io_i->valid_pin = cc2652_io_pin_is_valid,
	io_i->release_io_pin = cc2652_release_io_pin,
	io_i->panic = cc2652_panic;
	io_i->log = cc2652_log;
}

static void
event_thread (void *io) {
	io_cc2652_cpu_t *this = io;
	this->in_event_thread = true;
	while (next_io_event (io));
	this->in_event_thread = false;
}

static void
hard_fault (void *io) {
	DISABLE_INTERRUPTS;
	while(1);
}

void
initialise_cpu_io (io_t *io) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;

	this->in_event_thread = false;

	io_cpu_clock_start (io,io_get_core_clock(io));

	register_io_interrupt_handler (io,INT_PENDSV,event_thread,io);
	register_io_interrupt_handler (io,INT_HARD_FAULT,hard_fault,io);
}

static void
initialise_ram_interrupt_vectors (void) {
	io_interrupt_handler_t *i = cpu_interrupts;
	io_interrupt_handler_t *e = i + NUMBER_OF_INTERRUPT_VECTORS;
	while (i < e) {
		i->action = null_interrupt_handler;
		i->user_value = NULL;
		i++;
	}
}

static void
initialise_c_runtime (void) {
	extern uint32_t ld_start_of_sdata_in_flash;
	extern uint32_t ld_start_of_sdata_in_ram,ld_end_of_sdata_in_ram;
	extern uint32_t ld_start_of_bss,ld_end_of_bss;

	uint32_t *src = &ld_start_of_sdata_in_flash;
	uint32_t *dest = &ld_start_of_sdata_in_ram;

	while(dest < &ld_end_of_sdata_in_ram) *dest++ = *src++;
	dest = &ld_start_of_bss;
	while(dest < &ld_end_of_bss) *dest++ = 0;

	// fill stack/heap region of RAM with a pattern
	extern uint32_t ld_end_of_static_ram_allocations;
	uint32_t *end = (uint32_t*) __get_MSP();
	dest = &ld_end_of_static_ram_allocations;
	while (dest < end) {
		*dest++ = 0xdeadc0de;
	}
	
	initialise_ram_interrupt_vectors ();
}

static void
tune_cpu (void) {
	// see GPNVM, TCM is disabled?
	
	#if (__FPU_USED == 1)
	/* enable FPU if available and used */
	SCB->CPACR |= ((3UL << 10*2) |             /* set CP10 Full Access               */
					  (3UL << 11*2)  );           /* set CP11 Full Access               */
	#endif
}

int main(void);

void
cc2652_core_reset (void) {
	initialise_c_runtime ();
	tune_cpu ();
	main ();
	while (1);
}

static void
handle_io_cpu_interrupt (void) {
	io_interrupt_handler_t const *interrupt = &cpu_interrupts[
		SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk
	];
	interrupt->action(interrupt->user_value);
}

extern uint32_t ld_top_of_c_stack;
__attribute__ ((section(".isr_vector")))
const void* s_flash_vector_table[NUMBER_OF_INTERRUPT_VECTORS] = {
	&ld_top_of_c_stack,
	cc2652_core_reset,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,

	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,
	handle_io_cpu_interrupt,

};


#endif /* IMPLEMENT_IO_CPU */
#ifdef IMPLEMENT_VERIFY_IO_CPU
static void
test_io_events_1_ev (io_event_t *ev) {
	*((uint32_t*) ev->user_value) = 1;
}

TEST_BEGIN(test_io_events_1) {
	volatile uint32_t a = 0;
	io_event_t ev;
		
	initialise_io_event (&ev,test_io_events_1_ev,(void*) &a);

	io_enqueue_event (TEST_IO,&ev);
	while (a == 0);
	VERIFY (a == 1,NULL);
}
TEST_END

UNIT_SETUP(setup_io_cpu_unit_test) {
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_cpu_unit_test) {
}

void
io_cpu_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_events_1,
		0
	};
	unit->name = "io cpu";
	unit->description = "io cpu unit test";
	unit->tests = tests;
	unit->setup = setup_io_cpu_unit_test;
	unit->teardown = teardown_io_cpu_unit_test;
}

#define IO_CPU_UNIT_TESTS \
	io_cpu_unit_test,\
	/**/
#else
#define IO_CPU_UNIT_TESTS
#endif /* IMPLEMENT_VERIFY_IO_CPU */
#endif
