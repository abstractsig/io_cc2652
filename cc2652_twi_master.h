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

#define CC2652_TWI_MASTER_LOG_LEVEL		IO_INFO_LOG_LEVEL

typedef struct PACK_STRUCTURE cc2652_twi_master {
	IO_TWI_MASTER_SOCKET_STRUCT_MEMBERS

	io_twi_transfer_t current_transfer;
	io_event_t transfer_complete;
	
	const uint8_t *next_byte,*end;
	io_encoding_pipe_t *rx_pipe;

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
/*
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 *
 * socket states
 *
 *               io_twi_master_socket_state_closed
 *                 |        ^
 *          <open> |        | <close>
 *                 v        |
 *               io_twi_master_socket_state_open
 *
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */
static EVENT_DATA io_socket_state_t io_twi_master_socket_state_closed;
static EVENT_DATA io_socket_state_t io_twi_master_socket_state_open;

static bool cc2652_twi_master_open (io_socket_t*,io_socket_open_flag_t);

io_socket_state_t const*
io_twi_master_socket_state_closed_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	if (cc2652_twi_master_open (socket,flag)) {
		return &io_twi_master_socket_state_open;
	} else {
		return socket->State;
	}
}

io_socket_state_t const*
io_twi_master_socket_state_closed_open_for_inner (
	io_socket_t *socket,io_address_t inner,io_socket_open_flag_t flag
) {
	return io_twi_master_socket_state_closed_open (socket,flag);
}

static EVENT_DATA io_socket_state_t io_twi_master_socket_state_closed = {
	SPECIALISE_IO_SOCKET_STATE (&io_socket_state)
	.name = "closed",
	.open_for_inner = io_twi_master_socket_state_closed_open_for_inner,
};

io_socket_state_t const*
io_twi_master_socket_state_open_close (io_socket_t *socket) {
	return &io_twi_master_socket_state_closed;
}

io_socket_state_t const*
io_twi_master_socket_state_open_inner_closed (io_socket_t *socket,io_address_t inner) {
	return socket->State;
}

static EVENT_DATA io_socket_state_t io_twi_master_socket_state_open = {
	SPECIALISE_IO_SOCKET_STATE (&io_socket_state)
	.name = "open",
	.close = io_twi_master_socket_state_open_close,
	.inner_closed = io_twi_master_socket_state_open_inner_closed,
};

static void
cc2652_twi_master_interrupt (void *user_value) {
	cc2652_twi_master_t *this = user_value;

	I2CMasterIntClear (this->register_base_address);
}

static io_socket_t*
cc2652_twi_master_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	cc2652_twi_master_t *this = (cc2652_twi_master_t*) socket;

	io_twi_master_socket_initialise (socket,io,C);
	this->encoding = C->encoding;

	register_io_interrupt_handler (
		io,this->interrupt_number,cc2652_twi_master_interrupt,this
	);

	this->State = &io_twi_master_socket_state_closed;
	io_socket_enter_current_state (socket);

	return socket;
}

bool
TWI_master_is_enabled (uint32_t ui32Base) {
	return (HWREG(ui32Base + I2C_O_MCR) & I2C_MCR_MFE) != 0;
}

static bool
cc2652_twi_master_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	cc2652_twi_master_t *this = (cc2652_twi_master_t*) socket;

	if (io_cpu_clock_start (this->io,this->peripheral_clock)) {
		if (!TWI_master_is_enabled (this->register_base_address)) {
			float64_t freq = io_cpu_clock_get_current_frequency (this->peripheral_clock);

			io_set_pin_to_alternate (this->io,this->scl_pin.io);
			io_set_pin_to_alternate (this->io,this->sda_pin.io);

			I2CMasterInitExpClk (
				this->register_base_address,
				(uint32_t) freq,
			   true //fast
			);

			I2CMasterIntEnable (this->register_base_address);

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
	cc2652_twi_master_t *this = (cc2652_twi_master_t*) socket;
	if (TWI_master_is_enabled (this->register_base_address)) {

		I2CMasterIntDisable (this->register_base_address);
		I2CMasterDisable (this->register_base_address);

		{
			int32_t irqn = CMSIS_IRQn(this->interrupt_number);
			NVIC_DisableIRQ (irqn);
		}

		release_io_pin (this->io,this->scl_pin.io);
		release_io_pin (this->io,this->sda_pin.io);
	}
}


static bool
cc2652_twi_master_output_next_buffer (cc2652_twi_master_t *this) {
	io_encoding_t *next;
	if (io_encoding_pipe_peek (this->tx_pipe,&next)) {
		io_twi_transfer_t *cmd = io_encoding_get_layer (next,&io_twi_layer_implementation);

		this->current_transfer = *cmd;
		I2CMasterSlaveAddrSet (this->register_base_address,io_twi_transfer_bus_address(cmd),false);

		io_encoding_get_content (next,&this->next_byte,&this->end);
		if (
				this->end - this->next_byte
			&& io_twi_transfer_tx_length(&this->current_transfer) > 0
		) {
//			this->registers->TASKS_STARTTX = 1;
//			this->registers->TXD = *(this->next_byte)++;
			io_twi_transfer_tx_length(&this->current_transfer)--;
		} else {
			// get and free ...
		}
		return true;
	} else {
		return false;
	}
}

static bool
cc2652_twi_master_send_message (io_socket_t *socket,io_encoding_t *encoding) {
	bool ok = false;

	if (is_io_twi_encoding (encoding)) {
		cc2652_twi_master_t *this = (cc2652_twi_master_t*) socket;
		if (io_encoding_pipe_put_encoding (this->tx_pipe,encoding)) {
			if (io_encoding_pipe_count_occupied_slots (this->tx_pipe) == 1) {
				cc2652_twi_master_output_next_buffer (this);
			}
			ok = true;
		}
	}
	unreference_io_encoding (encoding);
	return ok;
}

EVENT_DATA io_socket_implementation_t cc2652_twi_master_implementation = {
		SPECIALISE_IO_TWI_MASTER_SOCKET_IMPLEMENTATION (
		&io_multiplex_socket_implementation
	)
	.initialise = cc2652_twi_master_initialise,
	.open = io_socket_try_open,
	.close = cc2652_twi_master_close,
	.send_message = cc2652_twi_master_send_message,
};
#endif /* IMPLEMENT_IO_CPU */
#endif
