/*
 *
 * cc2652 twi master
 *
 */
#ifndef cc2652_twi_master_H_
#define cc2652_twi_master_H_

#ifdef IMPLEMENT_IO_CPU
#ifndef IMPLEMENT_IO_TWI_LAYER
# define IMPLEMENT_IO_TWI_LAYER
#endif
#endif
#include <layers/io_twi_layer.h>

typedef struct PACK_STRUCTURE cc2652_twi_master {
	IO_MULTIPLEX_SOCKET_STRUCT_MEMBERS

	uint32_t register_base_address;
	uint32_t interrupt_number;
	io_cpu_clock_pointer_t peripheral_clock;

	cc2652_io_pin_t sda_pin;
	cc2652_io_pin_t scl_pin;

	uint32_t maximum_speed;

} cc2652_twi_master_t;

extern EVENT_DATA io_socket_implementation_t cc2652_twi_master_implementation;


#ifdef IMPLEMENT_IO_CPU
//-----------------------------------------------------------------------------
//
// Implementation
//
//-----------------------------------------------------------------------------

static void
cc2652_twi_master_interrupt (void *user_value) {
}

static io_socket_t*
cc2652_twi_master_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	cc2652_twi_master_t *this = (cc2652_twi_master_t*) socket;

	initialise_io_multiplex_socket (socket,io,C);

	register_io_interrupt_handler (
		io,this->interrupt_number,cc2652_twi_master_interrupt,this
	);

	return socket;
}

bool
TWI_master_is_eEnabled (uint32_t ui32Base) {
	return (HWREG(ui32Base + I2C_O_MCR) & I2C_MCR_MFE) != 0;
}

static bool
cc2652_twi_master_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	cc2652_twi_master_t *this = (cc2652_twi_master_t*) socket;

	if (io_cpu_clock_start (this->io,this->peripheral_clock)) {
		if (!TWI_master_is_eEnabled(this->register_base_address)) {

			io_set_pin_to_alternate (this->io,this->scl_pin.io);
			io_set_pin_to_alternate (this->io,this->sda_pin.io);

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
cc2652_twi_master_close (io_socket_t *socket) {
}


EVENT_DATA io_socket_implementation_t cc2652_twi_master_implementation = {
	SPECIALISE_IO_MULTIPLEX_SOCKET_IMPLEMENTATION (
		&io_multiplex_socket_implementation
	)
	.initialise = cc2652_twi_master_initialise,
	.open = cc2652_twi_master_open,
	.close = cc2652_twi_master_close,
};
#endif /* IMPLEMENT_IO_CPU */
#endif
