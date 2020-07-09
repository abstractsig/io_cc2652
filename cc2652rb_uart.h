/*
 *
 * cc2652 uart socket operating in byte-mode only
 *
 */
#ifndef cc2652rb_uart_H_
#define cc2652rb_uart_H_
#include <cc2652rb_udma.h>

typedef struct PACK_STRUCTURE cc2652_uart {
	IO_SOCKET_STRUCT_MEMBERS

	io_encoding_implementation_t const *encoding;
	io_cpu_clock_pointer_t peripheral_clock;

	// for single inner binding
	io_event_t *signal_transmit_available;
	io_event_t *signal_receive_data_available;

	cc2652_io_dma_channel_t tx_dma_channel;
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

extern EVENT_DATA io_socket_implementation_t cc2652_uart_implementation;

#include <ti/driverlib/uart.h>

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
 *               cc2652_uart_state_closed
 *                 |        ^
 *          <open> |        | <close>
 *                 v        |
 *               cc2652_uart_state_open
 *
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */
static EVENT_DATA io_socket_state_t cc2652_uart_state_closed;
static EVENT_DATA io_socket_state_t cc2652_uart_state_open;

bool
UARTisEnabled (uint32_t ui32Base) {
	return (HWREG(ui32Base + UART_O_CTL) & UART_CTL_UARTEN) != 0;
}

static bool
cc2652_uart_output_next_buffer (cc2652_uart_t *this) {
	io_encoding_t *next;
	if (
			UARTisEnabled (this->register_base_address)
		&& io_encoding_pipe_peek (this->tx_pipe,&next)
	) {
		const uint8_t *byte,*end;
		io_encoding_get_content (next,&byte,&end);

		io_dma_transfer_to_peripheral (
			(io_dma_channel_t*) &this->tx_dma_channel,byte,end - byte
		);
		HWREG (this->register_base_address + UART_O_DMACTL) |= (
			UART_DMACTL_TXDMAE
		);

		/*
		// keep just is case we need to test without DMA
		while (byte < end) {
			UARTCharPut (this->register_base_address,*byte++);
		}
		io_enqueue_event (this->io,&this->transmit_complete);
		*/

		return true;
	} else {
		return false;
	}
}

static void
cc2652_uart_output_event_handler (io_event_t *ev) {
	cc2652_uart_t *this = ev->user_value;

	if (!io_encoding_pipe_pop_encoding (this->tx_pipe)) {
		io_panic (this->io,IO_PANIC_SOMETHING_BAD_HAPPENED);
	}

	if (
			!cc2652_uart_output_next_buffer (this)
		&&	UARTisEnabled (this->register_base_address)
		&& this->signal_transmit_available
	) {
		io_enqueue_event (this->io,this->signal_transmit_available);
	}
}

static void
cc2652_uart_interrupt (void *user_value) {
	cc2652_uart_t *this = user_value;
	uint32_t status = HWREG(this->register_base_address + UART_O_MIS);
	if (status == 0 ) {
		volatile uint32_t status = HWREG (UDMA0_BASE + UDMA_O_REQDONE);
		volatile uint32_t mask = (1 << cc2652_io_dma_channel_number(&this->tx_dma_channel));
		if (status & mask) {
			HWREG (this->register_base_address + UART_O_DMACTL) &= ~(
				UART_DMACTL_TXDMAE
			);
			uDMAIntClear(UDMA0_BASE,mask);
			io_dma_transfer_complete (io_socket_io(this),(io_dma_channel_t*)&this->tx_dma_channel);
		} else {
			io_panic (io_socket_io(this),IO_PANIC_SOMETHING_BAD_HAPPENED);
		}

	} else {
		if (status & ( UART_MIS_RTMIS | UART_MIS_RXMIS)) {
			// clear interrupt
			HWREG (this->register_base_address + UART_O_ICR) &= ~UART_ICR_RXIC;

			while (!(HWREG(this->register_base_address + UART_O_FR) & UART_FR_RXFE)) {
				uint8_t byte = HWREG (this->register_base_address + UART_O_DR);
				io_byte_pipe_put_byte (this->rx_pipe,byte);
			}
			//io_enqueue_event (this->io,this->receieve_event);
			if (this->signal_receive_data_available) {
				io_enqueue_event (
					io_socket_io(this),this->signal_receive_data_available
				);
			}
		}

		if (status & UART_MIS_TXMIS) {
			// clear interrupt
			HWREG (this->register_base_address + UART_O_ICR) &= ~UART_ICR_TXIC;
		}
	}
}

static void
cc2652_uart_tx_dma_error (io_event_t *ev) {

}

static bool cc2652_uart_open (io_socket_t*,io_socket_open_flag_t);

io_socket_state_t const*
cc2652_uart_state_closed_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	if (cc2652_uart_open (socket,flag)) {
		return &cc2652_uart_state_open;
	} else {
		return socket->State;
	}
}

io_socket_state_t const*
cc2652_uart_state_open_close (io_socket_t *socket) {
	return socket->State;
}

static EVENT_DATA io_socket_state_t cc2652_uart_state_closed = {
	SPECIALISE_IO_SOCKET_STATE (&io_socket_state)
	.name = "closed",
	.open = cc2652_uart_state_closed_open,
};

static EVENT_DATA io_socket_state_t cc2652_uart_state_open = {
	SPECIALISE_IO_SOCKET_STATE (&io_socket_state)
	.name = "open",
	.close = cc2652_uart_state_open_close,
};

// has 32byte rx and tx fifos
static io_socket_t*
cc2652_uart_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	cc2652_uart_t *this = (cc2652_uart_t*) socket;
	initialise_io_socket (socket,io);
	this->State = &cc2652_uart_state_closed;
	
	this->tx_pipe = mk_io_encoding_pipe (
		io_get_byte_memory(io),C->transmit_pipe_length
	);
	this->signal_transmit_available = NULL;
	this->signal_receive_data_available = NULL;

	this->rx_pipe = mk_io_byte_pipe (
		io_get_byte_memory(io),io_settings_receive_pipe_length(C)
	);

	initialise_io_event (
		&this->transmit_complete,cc2652_uart_output_event_handler,this
	);

	initialise_io_event (
		&this->tx_dma_channel.complete,cc2652_uart_output_event_handler,this
	);
	initialise_io_event (
		&this->tx_dma_channel.error,cc2652_uart_tx_dma_error,this
	);

	cc2652_register_dma_channel (io,(io_dma_channel_t*)&this->tx_dma_channel);

	register_io_interrupt_handler (
		io,this->interrupt_number,cc2652_uart_interrupt,this
	);

	return socket;
}

void
cc2652_uart_set_baud_rate (cc2652_uart_t *this) {

	float64_t freq = io_cpu_clock_get_current_frequency (this->peripheral_clock);
	uint32_t clock = (uint32_t) freq;
	uint32_t base = this->register_base_address;
	uint32_t div = (((clock * 8) / this->baud_rate) + 1) / 2;//(clock << 6) / (this->baud_rate * 16);

	HWREG(base + UART_O_IBRD) = div / 64;
	HWREG(base + UART_O_FBRD) = div % 64;
}

#define UART_LCRH_STP1 0
#define UART_LCRH_NO_PARITY 0

static bool
cc2652_uart_open (io_socket_t *socket,io_socket_open_flag_t flag) {
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

			//UARTEnable (this->register_base_address);
		   //HWREG(this->register_base_address + UART_O_LCRH) |= UART_LCRH_FEN;

		    // Enable RX, TX, and the UART.
		    HWREG(this->register_base_address + UART_O_CTL) |= (
		   	 	UART_CTL_UARTEN
				|	UART_CTL_TXE
				|	UART_CTL_RXE
		    );

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

static io_encoding_t*
cc2652_uart_new_message (io_socket_t *socket) {
	cc2652_uart_t *this = (cc2652_uart_t*) socket;
	return reference_io_encoding (
		new_io_encoding (this->encoding,io_get_byte_memory(this->io))
	);
}

static bool
cc2652_uart_send_message (io_socket_t *socket,io_encoding_t *encoding) {
	bool ok = false;

	if (is_io_binary_encoding (encoding)) {
		cc2652_uart_t *this = (cc2652_uart_t*) socket;
		if (io_encoding_pipe_put_encoding (this->tx_pipe,encoding)) {
			if (io_encoding_pipe_count_occupied_slots (this->tx_pipe) == 1) {
				cc2652_uart_output_next_buffer (this);
			}
			ok = true;
		}
	}

	unreference_io_encoding (encoding);
	return ok;
}


static bool
cc2652_uart_bind_to_inner (io_socket_t *socket,io_address_t a,io_event_t *tx,io_event_t *rx) {
	cc2652_uart_t *this = (cc2652_uart_t*) socket;

	this->signal_transmit_available = tx;
	this->signal_receive_data_available = rx;

	return true;
}

io_pipe_t*
cc2652_uart_get_receive_pipe (io_socket_t *socket,io_address_t address) {
	cc2652_uart_t *this = (cc2652_uart_t*) socket;
	return (io_pipe_t*) this->rx_pipe;
}

void
cc2652_uart_flush (io_socket_t *socket) {
	cc2652_uart_t *this = (cc2652_uart_t*) socket;
	while (io_encoding_pipe_count_occupied_slots (this->tx_pipe) > 0);
}

static size_t
cc2652_uart_mtu (io_socket_t const *socket) {
	return 1024;
}

EVENT_DATA io_socket_implementation_t cc2652_uart_implementation = {
	SPECIALISE_IO_SOCKET_IMPLEMENTATION (
		&io_physical_socket_implementation
	)
	.initialise = cc2652_uart_initialise,
	.open = cc2652_uart_open,
	.close = cc2652_uart_close,
	.new_message = cc2652_uart_new_message,
	.send_message = cc2652_uart_send_message,
	.bind_inner = cc2652_uart_bind_to_inner,
	.get_receive_pipe = cc2652_uart_get_receive_pipe,
	.flush = cc2652_uart_flush,
	.mtu = cc2652_uart_mtu,
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
