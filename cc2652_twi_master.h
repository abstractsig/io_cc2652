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
	
	const uint8_t *next_transmit_byte,*end;
	io_encoding_t *current_receive_message;

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
typedef struct cc2652_twi_master_socket_state {
	IO_SOCKET_STATE_STRUCT_MEMBERS
	io_socket_state_t const* (*transfer_complete) (io_socket_t*);
} cc2652_twi_master_socket_state_t;

io_socket_state_t const*
io_twi_master_socket_state_ignore_transfer_complete (io_socket_t *socket) {
	cc2652_twi_master_t *this = (cc2652_twi_master_t*) socket;
	if (this->current_receive_message != NULL) {
		io_encoding_free (this->current_receive_message);
		this->current_receive_message = NULL;
	}
	return socket->State;
}

#define SPECIALISE_CC2652_TWI_MASTER_SOCKET_STATE(S) \
		SPECIALISE_IO_SOCKET_STATE(S)\
		.transfer_complete = io_twi_master_socket_state_ignore_transfer_complete,\
		/**/

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
 *                 |        ^
 *                 v        |
 *               io_twi_master_socket_state_busy
 *
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */
static EVENT_DATA cc2652_twi_master_socket_state_t io_twi_master_socket_state_closed;
static EVENT_DATA cc2652_twi_master_socket_state_t io_twi_master_socket_state_open;
static EVENT_DATA cc2652_twi_master_socket_state_t io_twi_master_socket_state_busy;

static bool cc2652_twi_master_open (io_socket_t*,io_socket_open_flag_t);
static void	cc2652_twi_master_transfer_complete (io_event_t*);
static bool cc2652_twi_master_output_next_buffer (cc2652_twi_master_t*);


io_socket_state_t const*
io_twi_master_socket_state_closed_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	if (cc2652_twi_master_open (socket,flag)) {
		return (io_socket_state_t const*) &io_twi_master_socket_state_open;
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

static EVENT_DATA cc2652_twi_master_socket_state_t io_twi_master_socket_state_closed = {
	SPECIALISE_CC2652_TWI_MASTER_SOCKET_STATE (&io_socket_state)
	.name = "closed",
	.open_for_inner = io_twi_master_socket_state_closed_open_for_inner,
};

//
// on entry, see if we have stuff to send
// if not signal tx available
//

io_socket_state_t const*
io_twi_master_socket_state_open_send (io_socket_t *socket) {
	cc2652_twi_master_t *this = (cc2652_twi_master_t*) socket;

	if (cc2652_twi_master_output_next_buffer(this)) {
		return (io_socket_state_t const*) &io_twi_master_socket_state_busy;
	} else {
		return socket->State;
	}
}

io_socket_state_t const*
io_twi_master_socket_state_open_close (io_socket_t *socket) {
	return (io_socket_state_t const*) &io_twi_master_socket_state_closed;
}

io_socket_state_t const*
io_twi_master_socket_state_open_inner_closed (io_socket_t *socket,io_address_t inner) {
	return socket->State;
}

static EVENT_DATA cc2652_twi_master_socket_state_t io_twi_master_socket_state_open = {
	SPECIALISE_CC2652_TWI_MASTER_SOCKET_STATE (&io_socket_state)
	.name = "open",
	.inner_send = io_twi_master_socket_state_open_send,
	.close = io_twi_master_socket_state_open_close,
	.inner_closed = io_twi_master_socket_state_open_inner_closed,
};

io_socket_state_t const*
io_twi_master_socket_state_busy_end (io_socket_t *socket) {
	cc2652_twi_master_t *this = (cc2652_twi_master_t*) socket;

	if (this->current_receive_message != NULL) {
		io_layer_t *layer = get_twi_layer (this->current_receive_message);

		if (layer == NULL) {
			io_panic (io_socket_io (this),IO_PANIC_SOMETHING_BAD_HAPPENED);
		}

		io_inner_binding_t *inner = io_multiplex_socket_find_inner_binding (
			(io_multiplex_socket_t*) this,
			io_layer_get_source_address (layer,this->current_receive_message)
		);
		if (inner != NULL) {
			io_encoding_pipe_put_encoding (
				io_inner_binding_receive_pipe (inner),this->current_receive_message
			);
			io_enqueue_event (io_socket_io(this),io_inner_binding_receive_event(inner));
		}

		unreference_io_encoding (this->current_receive_message);
		this->current_receive_message = NULL;
	}

	return (io_socket_state_t const*) &io_twi_master_socket_state_open;
}

static EVENT_DATA cc2652_twi_master_socket_state_t io_twi_master_socket_state_busy = {
	SPECIALISE_CC2652_TWI_MASTER_SOCKET_STATE (&io_socket_state)
	.name = "busy",
	.transfer_complete = io_twi_master_socket_state_busy_end,
};

static void
cc2652_twi_master_interrupt (void *user_value) {
	cc2652_twi_master_t *this = user_value;

	I2CMasterIntClear (this->register_base_address);

   if (I2CMasterBusy(this->register_base_address)) {
       return;
   }

   //
   // could be end of a stop?
   //

   uint32_t status = HWREG(this->register_base_address + I2C_O_MSTAT);

   if (status & (I2C_MSTAT_ERR | I2C_MSTAT_ARBLST)) {

      I2CMasterControl(this->register_base_address, I2C_MCTRL_STOP);

      //io_enqueue_event (io_socket_io(this),&this->transfer_error);

   } else {
   	io_twi_transfer_t *current = &this->current_transfer;

		if (io_twi_transfer_tx_length (current) > 0) {
			io_twi_transfer_tx_length(current)--;
			this->next_transmit_byte ++;

			if (io_twi_transfer_tx_length (current) > 0) {
				I2CMasterDataPut (
					this->register_base_address,*this->next_transmit_byte
				);
			} else {
				//
				// single byte transmit
				//
				if (io_twi_transfer_rx_length (current) == 0) {
					//
					// no response, so stop
					//
					goto stop;
				} else {
					uint32_t command = (I2C_MCTRL_START | I2C_MCTRL_RUN);
					// repeat-start
	            I2CMasterSlaveAddrSet (
	            	this->register_base_address,
						io_twi_transfer_bus_address(&this->current_transfer),
						true
					);

	            if (io_twi_transfer_rx_length (current) > 1) {
	                // acknowledge received byte
	                command |= I2C_MCTRL_ACK;
	            }

	            // run with repeated START
	            command |= I2C_MCTRL_START;
	            I2CMasterControl(
	            	this->register_base_address, command
					);
				}
			}
		} else if (io_twi_transfer_rx_length (current) > 0) {
			uint32_t command = I2C_MCTRL_RUN;
			volatile uint8_t byte;

			io_twi_transfer_rx_length (current) --;
			byte = I2CMasterDataGet(this->register_base_address);
			io_encoding_append_byte (this->current_receive_message,byte);

			if (io_twi_transfer_rx_length (current) > 1) {
				command |= I2C_MCTRL_ACK;
			} else if (io_twi_transfer_rx_length (current) == 0) {
				goto stop;
			}

			I2CMasterControl(this->register_base_address, command);

		} else {

			//

			goto stop;
		}
   }
   return;

stop:
	if (I2CMasterBusBusy (this->register_base_address)) {
		I2CMasterControl(this->register_base_address, I2C_MCTRL_STOP);
	} else {
		io_enqueue_event (io_socket_io(this),&this->transfer_complete);
	}
	return;

}

static io_socket_t*
cc2652_twi_master_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	cc2652_twi_master_t *this = (cc2652_twi_master_t*) socket;

	io_twi_master_socket_initialise (socket,io,C);
	this->encoding = C->encoding;
	this->current_receive_message = NULL;

	initialise_io_event (
		&this->transfer_complete,cc2652_twi_master_transfer_complete,this
	);

	register_io_interrupt_handler (
		io,this->interrupt_number,cc2652_twi_master_interrupt,this
	);

	this->State = (io_socket_state_t const*) &io_twi_master_socket_state_closed;
	io_socket_enter_current_state (socket);

	return socket;
}

static void
cc2652_twi_master_transfer_complete (io_event_t *ev) {
	io_socket_t *socket = ev->user_value;
	io_socket_call_state (
		socket,((cc2652_twi_master_socket_state_t const*) socket->State)->transfer_complete
	);
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
			   this->maximum_speed > 100000
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
	io_inner_binding_t *tx = io_multiplex_socket_get_next_transmit_binding (
		(io_multiplex_socket_t*) this
	);

	if (tx) {
		io_encoding_t *next;
		if (io_encoding_pipe_peek (io_inner_binding_transmit_pipe(tx),&next)) {
			io_twi_transfer_t *cmd = get_twi_layer (next);
			if (cmd) {

				this->current_transfer = *cmd;

				if (io_twi_transfer_rx_length(&this->current_transfer) > 0) {
					// we need a receive message
					io_encoding_t *message  = new_io_encoding (
						this->encoding,io_get_byte_memory (io_socket_io (this))
					);
					if (message != NULL) {
						io_layer_t *layer = push_io_twi_receive_layer (message);
						if (layer != NULL) {
							io_layer_set_source_address (layer,message,io_inner_binding_address(tx));
						} else {
							io_panic (io_socket_io (this),IO_PANIC_OUT_OF_MEMORY);
						}
						this->current_receive_message = reference_io_encoding (message);
					} else {
						io_panic (io_socket_io (this),IO_PANIC_OUT_OF_MEMORY);
					}
				}
				I2CMasterSlaveAddrSet (
					this->register_base_address,
					io_twi_transfer_bus_address(cmd),
					false
				);

				if (io_twi_transfer_tx_length (&this->current_transfer) > 0) {
					io_encoding_get_content (next,&this->next_transmit_byte,&this->end);

					I2CMasterDataPut (
						this->register_base_address,*this->next_transmit_byte
					);
					I2CMasterControl (
						this->register_base_address,
						(I2C_MCTRL_START | I2C_MCTRL_RUN)
					);

				} else if (io_twi_transfer_rx_length(&this->current_transfer) > 0) {
					// receive only to be supported soon...
				} else {
					// nothing to do
				}

				return true;
			}
		}
	}

	return false;
}

static bool
cc2652_twi_master_send_message (io_socket_t *socket,io_encoding_t *encoding) {
	bool ok = false;

	io_layer_t *twi = get_twi_layer (encoding);
	if (twi) {
		io_address_t addr = io_layer_get_destination_address(twi,encoding);
		io_inner_binding_t *inner = io_multiplex_socket_find_inner_binding (
			(io_multiplex_socket_t*) socket,addr
		);
		if (inner) {
			if (io_encoding_pipe_put_encoding (io_inner_binding_transmit_pipe(inner),encoding)) {
				io_socket_call_state (socket,socket->State->inner_send);
			}
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
