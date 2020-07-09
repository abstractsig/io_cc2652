/*
 *
 * cc2652 radio socket
 *
 */
#ifndef cc2652rb_radio_H_
#define cc2652rb_radio_H_
#include <ti/drivers/rf/RFQueue.h>
#include <ti/driverlib/rf_common_cmd.h>

#define CC2652_RADIO_SOCKET_LOG_LEVEL 			IO_INFO_LOG_LEVEL

#define IO_CC2652_RADIO_SOCKET_STATE_STRUCT_MEMBERS \
	IO_SOCKET_STATE_STRUCT_MEMBERS \
	io_socket_state_t const* (*command_acknowledge) (io_socket_t*); \
	io_socket_state_t const* (*command_done) (io_socket_t*); \
	/**/

typedef struct PACK_STRUCTURE io_cc2652_radio_socket_state {
	IO_CC2652_RADIO_SOCKET_STATE_STRUCT_MEMBERS
} io_cc2652_radio_socket_state_t;

io_socket_state_t const* radio_event_no_action (io_socket_t*);

#define SPECIALISE_CC2652_RADIO_SOCKET_STATE(S) \
	SPECIALISE_IO_SOCKET_STATE(S) \
	.command_acknowledge = radio_event_no_action, \
	.command_done = radio_event_no_action, \
	/**/

// patch functions
typedef struct PACK_STRUCTURE  {
    void (*cpe)(void);
    void (*mce)(void);
    void (*rfe)(void);
} radio_patch_functions_t;

#define CC2652RB_RADIO_SOCKET_STRUCT_MEMBERS \
	IO_MULTIPLEX_SOCKET_STRUCT_MEMBERS \
	io_cpu_clock_pointer_t peripheral_clock; \
	radio_patch_functions_t const *patch; \
	rfc_CMD_RADIO_SETUP_t *radio_setup; \
	rfc_radioOp_t *active_command; \
	io_event_t command_acknowledge; \
	io_event_t command_done; \
	io_event_t receive_event; \
	dataQueue_t rf_receive_queue; \
	/**/

typedef struct PACK_STRUCTURE cc2652rb_radio_socket {
	CC2652RB_RADIO_SOCKET_STRUCT_MEMBERS
} cc2652rb_radio_socket_t;

io_socket_t* cc2652rb_radio_initialise (io_socket_t*,io_t*,io_settings_t const*);
bool cc2652rb_radio_send_message (io_socket_t*,io_encoding_t*);
size_t cc2652rb_radio_mtu (io_socket_t const*);

extern EVENT_DATA io_socket_implementation_t cc2652rb_radio_socket_implementation;
extern EVENT_DATA io_socket_implementation_t cc2652rb_ble5_socket_implementation;

#define SPECIALISE_CC2652RB_RADIO_SOCKET_IMPLEMENTATION(S) \
	SPECIALISE_IO_MULTIPLEX_SOCKET_IMPLEMENTATION(S) \
	.initialise = cc2652rb_radio_initialise,	\
	.reference = io_virtual_socket_increment_reference,	\
	.send_message = cc2652rb_radio_send_message,	\
	.mtu = cc2652rb_radio_mtu,	\
	/**/

INLINE_FUNCTION void
write_to_radio_command_register (uint32_t rawCmd) {
	RFC_DOOR_BELL->CMDR.register_value = rawCmd;
}

#include <ti/driverlib/rf_common_cmd.h>
#include <ti/driverlib/rf_mailbox.h>
#include <ti/driverlib/rf_prop_cmd.h>


#ifdef IMPLEMENT_IO_CPU
//-----------------------------------------------------------------------------
//
// Implementation
//
//-----------------------------------------------------------------------------


#define CC2652_CPE0_INTERRUPTS (\
			RFC_DBELL_RFCPEIFG_COMMAND_DONE	\
		|	RFC_DBELL_RFCPEIFG_LAST_COMMAND_DONE \
	)\
	/**/

#define CC2652_CPE1_INTERRUPTS (\
			RFC_DBELL_RFCPEIFG_RX_ENTRY_DONE	\
		|	RFC_DBELL_RFCPEIFG_RX_ABORTED\
		|	RFC_DBELL_RFCPEIFG_RX_BUF_FULL \
		|	RFC_DBELL_RFCPEIFG_RX_NOK \
		|	RFC_DBELL_RFCPEIFG_RX_OK \
	)\
	/**/

#define RF_CMD0									0x0607
#define RF_BOOT0									0xE0000011
#define RF_BOOT1									0x00000080
#define RF_PHY_BOOTUP_MODE						0
#define RF_PHY_SWITCHING_MODE					1
#define RF_PHY_BOOTUP_MODE						0
#define BLE_ADV_AA								0x8E89BED6
#define RF_HPOSC_OVERRIDE_PATTERN			HPOSC_OVERRIDE(0)
#define RF_HPOSC_OVERRIDE_MASK				0xFFFF
#define RF_OVERRIDE_SEARCH_DEPTH				80
// HPOSC temperature limit
#define RF_TEMP_LIMIT_3_DEGREES_CELSIUS	0x300

typedef enum {
	BLE5_PHY_1M = 0,
	BLE5_PHY_2M,
	BLE5_PHY_CODED
} BLE5_PHY_Mode;

#include <ti/drivers/rf/RFQueue.c>


io_socket_state_t const*
radio_event_no_action (io_socket_t *socket) {
	return socket->State;
}

rfc_radioOp_t*
mk_SYNC_START_RAT_command (io_byte_memory_t* bm,ratmr_t rat0) {
	rfc_CMD_SYNC_START_RAT_t *this = io_byte_memory_allocate (
		bm,sizeof(rfc_CMD_SYNC_START_RAT_t)
	);

	if(this) {
		this->commandNo = CMD_SYNC_START_RAT;
		this->status = IDLE;
		this->startTrigger.triggerType = TRIG_NOW;
		this->pNextOp = NULL;
		this->condition.rule = COND_NEVER;
		this->rat0 = rat0;
	}

	return (rfc_radioOp_t*) this;
}

#define free_active_command(this)	\
	io_byte_memory_free(io_socket_byte_memory(this),this->active_command),\
	this->active_command = NULL;


bool
is_cc2652_radio_timer_running(void) {
	bool status = false;

	if (PRCM0->PDSTAT0.bit.RFC_ON) {
		status = RFC_PWR->PWMCLKEN.bit.RAT_M;
	}

	return status;
}

static io_socket_state_t const*
cc2652rb_radio_error_enter (io_socket_t *socket) {
	#if defined(CC2652_RADIO_SOCKET_LOG_LEVEL)
	io_log (
		io_socket_io (socket),
		IO_ERROR_LOG_LEVEL,
		"%-*s%-*senter\n",
		DBP_FIELD1,"radio",
		DBP_FIELD2,"error state"
	);
	#endif
	return socket->State;
}

static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_radio_error = {
	SPECIALISE_CC2652_RADIO_SOCKET_STATE (&io_socket_state)
	.name = "error",
	.enter = cc2652rb_radio_error_enter,
};

static void
cc2652rb_radio_cpe0_interrupt (void *user_value) {
	uint32_t status = RFC_DOOR_BELL->RFCPEIFG.register_value;
	cc2652rb_radio_socket_t *this = user_value;

	RFC_DOOR_BELL->RFCPEIFG.register_value = ~(
			CC2652_CPE0_INTERRUPTS
	);

	if (status & RFC_DBELL_RFCPEIFG_COMMAND_DONE) {
		io_enqueue_event (io_socket_io(this),&this->command_done);
	}

}

static void
cc2652rb_radio_cpe1_interrupt (void *user_value) {
	uint32_t status = RFC_DOOR_BELL->RFCPEIFG.register_value;
	cc2652rb_radio_socket_t *this = user_value;

	RFC_DOOR_BELL->RFCPEIFG.register_value = ~(
			CC2652_CPE1_INTERRUPTS
	);

	if (status & RFC_DBELL_RFCPEIFG_RX_ENTRY_DONE) {
		io_enqueue_event (io_socket_io(this),&this->receive_event);
	}
}

#define RF_HW_INT_CPE_MASK			RFC_DBELL_RFHWIFG_MDMSOFT
#define RF_HW_INT_RAT_CH_MASK		(RFC_DBELL_RFHWIFG_RATCH7 | RFC_DBELL_RFHWIFG_RATCH6 | RFC_DBELL_RFHWIFG_RATCH5)

static void
cc2652rb_radio_hardware_interrupt (void *user_value) {
   uint32_t rfchwifg = RFCHwIntGetAndClear(RF_HW_INT_CPE_MASK | RF_HW_INT_RAT_CH_MASK);
   uint32_t rfchwien = RFC_DOOR_BELL->RFHWIEN.bit.MDMSOFT;// HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFHWIEN) & RF_HW_INT_CPE_MASK;
   uint32_t rathwien = HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFHWIEN) & RF_HW_INT_RAT_CH_MASK;


	// clear interrupt ...
	RFCHwIntClear((uint32_t) 0);

   UNUSED(rfchwifg);
	UNUSED(rfchwien);
	UNUSED(rathwien);
}

//
// happens "When the command is processed by the radio CPU"
//
static void
cc2652rb_radio_door_bell_interrupt (void *user_value) {
	cc2652rb_radio_socket_t *this = user_value;
	RFC_DOOR_BELL->RFACKIFG.register_value = 0;
	io_enqueue_event (io_socket_io(this),&this->command_acknowledge);
}

#define CC2652RB_MAX_RECEIVE_LENGTH			257  // Max 8-bit length + two byte BLE header
#define CC2652RB_RECEIVE_QUEUE_LENGTH		2    // NOTE: Only two data entries supported at the moment
#define CC2652RB_RECEIVE_APPENDED_SIZE		1    // Prepended length byte

static void
cc2652rb_radio_command_acknowledge_event (io_event_t *ev) {
	io_socket_t *socket = ev->user_value;
	io_socket_call_state (
		socket,((io_cc2652_radio_socket_state_t const*) socket->State)->command_acknowledge
	);
}

static void
cc2652rb_radio_command_done_event (io_event_t *ev) {
	io_socket_t *socket = ev->user_value;
	io_socket_call_state (
		socket,((io_cc2652_radio_socket_state_t const*) socket->State)->command_done
	);
}

static void
cc2652rb_radio_receive_event (io_event_t *ev) {
	io_socket_t *socket = ev->user_value;
	io_socket_call_state (socket,socket->State->outer_receive_event);
}

io_socket_t*
cc2652rb_radio_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	cc2652rb_radio_socket_t *this = (cc2652rb_radio_socket_t*) socket;
	uint32_t size_of_rx_buffer = RF_QUEUE_DATA_ENTRY_BUFFER_SIZE (
		CC2652RB_RECEIVE_QUEUE_LENGTH,
		CC2652RB_MAX_RECEIVE_LENGTH,
		CC2652RB_RECEIVE_APPENDED_SIZE
	);

	initialise_io_multiplex_socket (socket,io,C);

	this->active_command = NULL;

	RFQueue_defineQueue (
		&this->rf_receive_queue,
		io_byte_memory_allocate (io_get_byte_memory(io),size_of_rx_buffer),
		size_of_rx_buffer,
		CC2652RB_RECEIVE_QUEUE_LENGTH,
		CC2652RB_MAX_RECEIVE_LENGTH + CC2652RB_RECEIVE_APPENDED_SIZE
	);

	initialise_io_event (
		&this->command_acknowledge,cc2652rb_radio_command_acknowledge_event,this
	);

	initialise_io_event (
		&this->command_done,cc2652rb_radio_command_done_event,this
	);

	initialise_io_event (
		&this->receive_event,cc2652rb_radio_receive_event,this
	);

	register_io_interrupt_handler (
		io,INT_RFC_CPE_0,cc2652rb_radio_cpe0_interrupt,this
	);
	register_io_interrupt_handler (
		io,INT_RFC_CPE_1,cc2652rb_radio_cpe1_interrupt,this
	);
	register_io_interrupt_handler (
		io,INT_RFC_HW_COMB,cc2652rb_radio_hardware_interrupt,this
	);

	register_io_interrupt_handler (
		io,INT_RFC_CMD_ACK,cc2652rb_radio_door_bell_interrupt,this
	);

	{
		int32_t irqn = CMSIS_IRQn(INT_RFC_CMD_ACK);
		NVIC_SetPriority (irqn,NORMAL_INTERRUPT_PRIORITY);
		NVIC_ClearPendingIRQ (irqn);
		NVIC_EnableIRQ (irqn);

		irqn = CMSIS_IRQn(INT_RFC_CPE_0);
		NVIC_SetPriority (irqn,NORMAL_INTERRUPT_PRIORITY);
		NVIC_ClearPendingIRQ (irqn);
		NVIC_EnableIRQ (irqn);

		irqn = CMSIS_IRQn(INT_RFC_CPE_1);
		NVIC_SetPriority (irqn,NORMAL_INTERRUPT_PRIORITY);
		NVIC_ClearPendingIRQ (irqn);
		NVIC_EnableIRQ (irqn);

		irqn = CMSIS_IRQn(INT_RFC_HW_COMB);
		NVIC_SetPriority (irqn,NORMAL_INTERRUPT_PRIORITY);
		NVIC_ClearPendingIRQ (irqn);
		NVIC_EnableIRQ (irqn);
	}

	return socket;
}

bool
cc2652rb_radio_send_message (io_socket_t *socket,io_encoding_t *encoding) {
	return false;
}

size_t
cc2652rb_radio_mtu (io_socket_t const *socket) {
	return 256;
}

EVENT_DATA io_socket_implementation_t
cc2652rb_radio_socket_implementation = {
	SPECIALISE_CC2652RB_RADIO_SOCKET_IMPLEMENTATION (
		&io_multiplex_socket_implementation
	)
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
