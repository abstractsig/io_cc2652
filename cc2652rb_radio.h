/*
 *
 * cc2652 radio socket
 *
 */
#ifndef cc2652rb_radio_H_
#define cc2652rb_radio_H_

typedef struct PACK_STRUCTURE cc2652rb_radio_socket {
	IO_MULTIPLEX_SOCKET_STRUCT_MEMBERS


} cc2652rb_radio_socket_t;

extern EVENT_DATA io_socket_implementation_t cc2652rb_radio_socket_implementation;

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
 * radio socket states
 *
 *               cc2652rb_radio_power_off
 *           <open>|
 *                 v
 *
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */
static io_socket_state_t const*
cc2652rb_radio_power_off_open (
	io_socket_t *socket,io_socket_open_flag_t flag
) {
	switch (flag) {
		case IO_SOCKET_OPEN_CONNECT:

		case IO_SOCKET_OPEN_LISTEN:
			// not supported

		default:
			return socket->State;
	}
}

static EVENT_DATA io_socket_state_t cc2652rb_radio_power_off = {
	SPECIALISE_IO_SOCKET_STATE (&io_socket_state)
	.name = "off",
	.open = cc2652rb_radio_power_off_open,
};

//-----------------------------------------------------------------------------
//
// the socket
//
//-----------------------------------------------------------------------------

static void
cc2652rb_radio_cpe0_interrupt (void *user_value) {
}

static io_socket_t*
cc2652rb_radio_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	cc2652rb_radio_socket_t *this = (cc2652rb_radio_socket_t*) socket;

	initialise_io_multiplex_socket (socket,io,C);

	socket->State = &cc2652rb_radio_power_off;
	io_socket_enter_current_state (socket);

	register_io_interrupt_handler (
		io,INT_RFC_CPE_0,cc2652rb_radio_cpe0_interrupt,this
	);
	//INT_RFC_CPE_1
	//INT_RFC_HW_COMB
	//INT_RFC_CMD_ACK

	return socket;
}

static void
cc2652rb_radio_open_event (io_event_t *ev) {
	io_socket_t *this = ev->user_value;
	io_socket_call_open (this,0);
	io_byte_memory_free (io_get_byte_memory (io_socket_io (this)),ev);
}

static bool
cc2652rb_radio_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	io_event_t *ev = io_byte_memory_allocate (
		io_get_byte_memory (io_socket_io (socket)),sizeof(io_event_t)
	);

	initialise_io_event (ev,cc2652rb_radio_open_event,socket);
	io_enqueue_event (io_socket_io (socket),ev);

	//rfcore requires the crystal oscillator to be M3 core clock
	if (
		OSCClockSourceGet(OSC_SRC_CLK_HF) == OSC_XOSC_HF
	) {

	}

	// door bell interrupt
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFHWIFG) = 0;

	return true;
}

static void
cc2652rb_radio_close (io_socket_t *socket) {
}

static bool
cc2652rb_radio_is_closed (io_socket_t const *socket) {
	return false;
}

io_encoding_t*
cc2652rb_radio_new_message (io_socket_t *socket) {
	return NULL;
}

static bool
cc2652rb_radio_send_message (io_socket_t *socket,io_encoding_t *encoding) {
	return false;
}

static size_t
cc2652rb_radio_mtu (io_socket_t const *socket) {
	return 256;
}


EVENT_DATA io_socket_implementation_t cc2652rb_radio_socket_implementation = {
	SPECIALISE_IO_MULTIPLEX_SOCKET_IMPLEMENTATION (
		&io_multiplex_socket_implementation
	)
	.initialise = cc2652rb_radio_initialise,
	.reference = io_virtual_socket_increment_reference,
	.open = cc2652rb_radio_open,
	.close = cc2652rb_radio_close,
	.is_closed = cc2652rb_radio_is_closed,
	.new_message = cc2652rb_radio_new_message,
	.send_message = cc2652rb_radio_send_message,
	.mtu = cc2652rb_radio_mtu,
};


#endif /* IMPLEMENT_IO_CPU */
#endif
