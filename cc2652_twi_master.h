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

#define SPECIALISE_CC2652_TWI_MASTER_SOCKET_STATE(S) \
		SPECIALISE_IO_SOCKET_STATE(S)\
		.transfer_complete = io_socket_state_ignore_event,\
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

static EVENT_DATA cc2652_twi_master_socket_state_t io_twi_master_socket_state_busy = {
	SPECIALISE_CC2652_TWI_MASTER_SOCKET_STATE (&io_socket_state)
	.name = "busy",
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
//   uint32_t command = I2C_MCTRL_RUN;

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
				if (io_twi_transfer_rx_length (current) == 0) {
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
	                /* RUN and generate ACK to slave */
	                command |= I2C_MCTRL_ACK;
	            }

	            /* RUN and generate a repeated START */
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
			UNUSED(byte);

			if (io_twi_transfer_rx_length (current) > 1) {
				command |= I2C_MCTRL_ACK;
			} else if (io_twi_transfer_rx_length (current) == 0) {
				goto stop;
			}

			I2CMasterControl(this->register_base_address, command);

		} else {
			goto stop;
		}
   }
   return;

stop:
	I2CMasterControl(this->register_base_address, I2C_MCTRL_STOP);
	// do i get another interrupt or is this the end?
	io_enqueue_event (io_socket_io(this),&this->transfer_complete);

	return;

}

#if 0
/*
 *  ======== I2CCC26XX_hwiFxn ========
 *  Hwi interrupt handler to service the I2C peripheral
 *
 *  The handler is a generic handler for a I2C object.
 */
static void I2CCC26XX_hwiFxn(uintptr_t arg)
{
    I2C_Handle handle = (I2C_Handle) arg;
    I2CCC26XX_Object *object = handle->object;
    I2CCC26XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    uint32_t command = I2C_MCTRL_RUN;

    /* Clear the interrupt */
    I2CMasterIntClear(hwAttrs->baseAddr);

    /*
     * Check if the Master is busy. If busy, the MSTAT is invalid as
     * the controller is still transmitting or receiving. In that case,
     * we should wait for the next interrupt.
     */
    if (I2CMasterBusy(hwAttrs->baseAddr)) {
        return;
    }

    uint32_t status = HWREG(I2C0_BASE + I2C_O_MSTAT);

    /* Current transaction is cancelled */
    if (object->currentTransaction->status == I2C_STATUS_CANCEL) {
        I2CMasterControl(hwAttrs->baseAddr, I2C_MCTRL_STOP);
        I2CCC26XX_completeTransfer(handle);
        return;
    }

    /* Handle errors. ERR bit is not set if arbitration lost */
    if (status & (I2C_MSTAT_ERR | I2C_MSTAT_ARBLST)) {
        /* Decode interrupt status */
        if (status & I2C_MSTAT_ARBLST) {
            object->currentTransaction->status = I2C_STATUS_ARB_LOST;
        }
        /*
         * The I2C peripheral has an issue where the first data byte
         * is always transmitted, regardless of the ADDR NACK. Therefore,
         * we should check this error condition first.
         */
        else if (status & I2C_MSTAT_ADRACK_N) {
            object->currentTransaction->status = I2C_STATUS_ADDR_NACK;
        }
        else {
            /* Last possible bit is the I2C_MSTAT_DATACK_N */
            object->currentTransaction->status = I2C_STATUS_DATA_NACK;
        }

        /*
         * The CC13X2 / CC26X2 I2C peripheral does not have an explicit STOP
         * interrupt bit. Therefore, if an error occurred, we send the STOP
         * bit and complete the transfer immediately.
         */
        I2CMasterControl(hwAttrs->baseAddr, I2C_MCTRL_STOP);
        I2CCC26XX_completeTransfer(handle);
    }
    else if (object->writeCount) {
        object->writeCount--;

        /* Is there more to transmit */
        if (object->writeCount) {
            I2CMasterDataPut(hwAttrs->baseAddr, *(object->writeBuf++));
        }
        /* If we need to receive */
        else if (object->readCount) {

            /* Place controller in receive mode */
            I2CMasterSlaveAddrSet(hwAttrs->baseAddr,
                object->currentTransaction->slaveAddress, true);

            if (object->readCount > 1) {
                /* RUN and generate ACK to slave */
                command |= I2C_MCTRL_ACK;
            }

            /* RUN and generate a repeated START */
            command |= I2C_MCTRL_START;
        }
        else {
            /* Send STOP */
            command = I2C_MCTRL_STOP;
        }

        I2CMasterControl(hwAttrs->baseAddr, command);
    }
    else if (object->readCount) {
        object->readCount--;

        /* Read data */
        *(object->readBuf++) = I2CMasterDataGet(hwAttrs->baseAddr);

        if (object->readCount > 1) {
            /* Send ACK and RUN */
            command |= I2C_MCTRL_ACK;
        }
        else if (object->readCount < 1) {
            /* Send STOP */
            command = I2C_MCTRL_STOP;
        }
        else {
            /* Send RUN */
        }

        I2CMasterControl(hwAttrs->baseAddr, command);
    }
    else {
        I2CMasterControl(hwAttrs->baseAddr, I2C_MCTRL_STOP);
        object->currentTransaction->status = I2C_STATUS_SUCCESS;
        I2CCC26XX_completeTransfer(handle);
    }

    return;
}
#endif

static io_socket_t*
cc2652_twi_master_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	cc2652_twi_master_t *this = (cc2652_twi_master_t*) socket;

	io_twi_master_socket_initialise (socket,io,C);
	this->encoding = C->encoding;

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

				I2CMasterSlaveAddrSet (
					this->register_base_address,
					io_twi_transfer_bus_address(cmd),
					false
				);

				io_encoding_get_content (next,&this->next_transmit_byte,&this->end);
				if (
						this->end - this->next_transmit_byte
					&& io_twi_transfer_tx_length(&this->current_transfer) > 0
				) {
					I2CMasterDataPut (
						this->register_base_address,*this->next_transmit_byte
					);
					I2CMasterControl (
						this->register_base_address,
						(I2C_MCTRL_START | I2C_MCTRL_RUN)
					);
				} else {
					// get and free ...
				}
#if 0
				io_log (
					io_socket_io (this),
					IO_INFO_LOG_LEVEL,
					"%-*s%-*sstatus = 0x%x\n",
					DBP_FIELD1,"twi",
					DBP_FIELD2,"send",
					HWREG(this->register_base_address + I2C_O_MSTAT)
				);
#endif

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
