/*
 *
 * cc2652 ble5 radio
 *
 */
#ifndef cc2652_ble5_radio_H_
#define cc2652_ble5_radio_H_
#include <cc2652rb_radio.h>
#include <layers/io_ble5_layer.h>
#include <ti/driverlib/rf_ble_cmd.h>

#define CC2652_BLE5_SOCKET_LOG_LEVEL 			IO_INFO_LOG_LEVEL

typedef struct PACK_STRUCTURE cc2652rb_ble5_socket {
	CC2652RB_RADIO_SOCKET_STRUCT_MEMBERS

	rfc_bleGenericRxOutput_t receive_stats;

} cc2652rb_ble5_socket_t;

extern EVENT_DATA io_socket_implementation_t cc2652rb_ble5_socket_implementation;

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
 * ble5 state machine
 *
 *               <cpu reset>
 *                 |
 *                 v
 *               cc2652rb_ble5_power_off
 *           <open>|
 *                 v
 *               cc2652rb_ble5_system_bus_request
 *                 |
 *                 v
 *               cc2652rb_ble5_setup_radio
 *                 |
 *                 v
 *               cc2652rb_ble5_start_radio_timer
 *                 |
 *                 v
 *               cc2652rb_ble5_power_on
 *                 |
 *                 v
 *               cc2652rb_ble5_receive_frame
 *
 *
 *     <any> --> cc2652rb_radio_error
 *
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */
static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_ble5_power_off;
static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_ble5_system_bus_request;
static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_ble5_start_radio_timer;
static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_ble5_setup_radio;
static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_ble5_power_on;
static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_ble5_receive_frame;



// Structure for CMD_BLE5_GENERIC_RX.pParam
static rfc_bleGenericRxPar_t ble5_generic_receive_parameters = {
	.pRxQ = 0,
	.rxConfig.bAutoFlushIgnored = 0x0,
	.rxConfig.bAutoFlushCrcErr = 0x0,
	.rxConfig.bAutoFlushEmpty = 0x0,
	.rxConfig.bIncludeLenByte = 0x1,
	.rxConfig.bIncludeCrc = 0x1,
	.rxConfig.bAppendRssi = 0x1,
	.rxConfig.bAppendStatus = 0x1,
	.rxConfig.bAppendTimestamp = 0x0,
	.bRepeat = 0x01,
	.__dummy0 = 0x0000,
	.accessAddress = 0x8E89BED6,
	.crcInit0 = 0x55,
	.crcInit1 = 0x55,
	.crcInit2 = 0x55,
	.endTrigger.triggerType = TRIG_NEVER, // never end rx operation
	.endTrigger.bEnaCmd = 0x0,
	.endTrigger.triggerNo = 0x0,
	.endTrigger.pastTrig = 0x0,
	.endTime = 0
};

//
// Bluetooth 5 Generic Receiver Command
//
rfc_CMD_BLE5_GENERIC_RX_t cc2652rb_radio_generic_receive = {
	.commandNo = 0x1829,
	.status = 0x0000,
	.pNextOp = NULL,
	.startTime = 0x00000000,
	.startTrigger.triggerType = TRIG_NOW,
	.startTrigger.bEnaCmd = 0x0,
	.startTrigger.triggerNo = 0x0,
	.startTrigger.pastTrig = 0x0,
	.condition.rule = COND_NEVER,
	.condition.nSkip = 0x0,
	.channel = 0x8C,
	.whitening.init = 0x40 + 37,
	.whitening.bOverride = 0x1,
	.phyMode.mainMode = BLE5_PHY_1M,
	.phyMode.coding = 0x0,
	.rangeDelay = 0x00,
	.txPower = 0x0000,
	.pParams = &ble5_generic_receive_parameters,
	.pOutput = NULL,
	.tx20Power = 0x00000000
};

// Sniff/Receive BLE packets
//
// Arguments:
//  phy         PHY mode to use
//  chan        Channel to listen on
//  accessAddr  BLE access address of packet to listen for
//  crcInit     Initial CRC value of packets being listened for
//  timeout     When to stop listening (in radio ticks)
//  callback    Function to call when a packet is received
//
void
modify_receive_command (
	cc2652rb_ble5_socket_t *this,
	BLE5_PHY_Mode phy,
	uint32_t channel_number,
	uint32_t accessAddr,
	uint32_t crcInit
) {
	//
	// rx stats need to be attached to the received encoding, i.e allocated
	//

	cc2652rb_radio_generic_receive.pOutput = (void*) &this->receive_stats;

	cc2652rb_radio_generic_receive.channel = channel_number;
	cc2652rb_radio_generic_receive.whitening.init = 0x40 + channel_number;
	cc2652rb_radio_generic_receive.phyMode.mainMode = phy;
	cc2652rb_radio_generic_receive.pParams->pRxQ = &this->rf_receive_queue;
	cc2652rb_radio_generic_receive.pParams->accessAddress = accessAddr;
   cc2652rb_radio_generic_receive.pParams->crcInit0 = crcInit & 0xFF;
   cc2652rb_radio_generic_receive.pParams->crcInit1 = (crcInit >> 8) & 0xFF;
   cc2652rb_radio_generic_receive.pParams->crcInit2 = (crcInit >> 16) & 0xFF;
   cc2652rb_radio_generic_receive.pParams->bRepeat = 1; // 1 to receive multiple packets
   cc2652rb_radio_generic_receive.pParams->endTrigger.triggerType = 0x01; // never?
   cc2652rb_radio_generic_receive.pParams->endTime = 0; // never?

   cc2652rb_radio_generic_receive.pParams->rxConfig.bAutoFlushIgnored = 1;
   cc2652rb_radio_generic_receive.pParams->rxConfig.bAutoFlushCrcErr = 1;
   cc2652rb_radio_generic_receive.pParams->rxConfig.bAutoFlushEmpty = 1;
   cc2652rb_radio_generic_receive.pParams->rxConfig.bIncludeLenByte = 1;
   cc2652rb_radio_generic_receive.pParams->rxConfig.bIncludeCrc = 0;
   cc2652rb_radio_generic_receive.pParams->rxConfig.bAppendRssi = 1;		// where?
   cc2652rb_radio_generic_receive.pParams->rxConfig.bAppendStatus = 0;
   cc2652rb_radio_generic_receive.pParams->rxConfig.bAppendTimestamp = 0;
}

static io_socket_state_t const*
cc2652rb_ble5_power_off_open (
	io_socket_t *socket,io_socket_open_flag_t flag
) {
	cc2652rb_ble5_socket_t *this = (cc2652rb_ble5_socket_t*) socket;

	switch (flag) {
		case IO_SOCKET_OPEN_CONNECT:
		break;

		case IO_SOCKET_OPEN_LISTEN:
			// not supported

		default:
			return socket->State;
	}

	if (cpu_clock_is_derrived_from_hp_oscillator (this->peripheral_clock)) {
		// Notes
		//
		// 1	PRCM:RFCBITS control boot process for radio CPU
		// 2	CMDR register can only be written while it reads 0
		// 	the radio CPU set it back to zero 'immediately'

		//  output RTC clock for Radio Timer Synchronization
	   RTC->CTL.bit.RTC_UPD_EN = 1;

		// Set the automatic bus request
		PRCM0->RFCBITS.register_value = RF_BOOT0;

		if (io_cpu_clock_start (this->io,this->peripheral_clock)) {

		   if (PRCM0->RFCMODEHWOPT.part.AVAIL & (1 << RF_MODE_BLE)) {
		   	cc2652rb_ble5_socket_t *this = (cc2652rb_ble5_socket_t*) socket;
		   	RFC_Doorbell_t *db = RFC_DOOR_BELL;

		   	//
		   	// do stuff from power-up-state
		   	//
				// Set the RF mode in the PRCM register (already verified that it is valid)
		   	//
				PRCM0->RFCMODESEL.register_value = RF_MODE_AUTO;
				db->RFCPEISL.register_value = CC2652_CPE1_INTERRUPTS;

				db->RFCPEIFG.register_value = 0;

				db->RFCPEIEN.register_value = (
						CC2652_CPE0_INTERRUPTS
					|	CC2652_CPE1_INTERRUPTS
				);

		   	//
		   	// do stuff from setup-state
		   	//
				{
					radio_patch_functions_t const *patch = this->patch;
					if (patch->cpe != NULL) patch->cpe();
					if (patch->mce != NULL) patch->mce ();
					if (patch->rfe != NULL) patch->rfe();
					if ((patch->mce != NULL) || (patch->rfe != NULL)) {
						// Turn off additional clocks (this command is not documented)
						write_to_radio_command_register (CMDR_DIR_CMD_2BYTE(RF_CMD0, 0));
						return (io_socket_state_t const*) &cc2652rb_ble5_system_bus_request;	// wrong
					}
				}

				//
				// Use the RAT_SYNC command in SETUP chain to start RAT
				//

				// goto active
				return (io_socket_state_t const*) &cc2652rb_ble5_system_bus_request;
		   }
		}
	} else {
		// error?
	}

	return socket->State;
}

static io_socket_state_t const*
cc2652rb_ble5_power_off_command_acknowledge (io_socket_t *socket) {
	return (io_socket_state_t const*) &cc2652rb_ble5_system_bus_request;
}

static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_ble5_power_off = {
	SPECIALISE_CC2652_RADIO_SOCKET_STATE (&io_socket_state)
	.name = "off",
	.open = cc2652rb_ble5_power_off_open,
	.command_acknowledge = cc2652rb_ble5_power_off_command_acknowledge,
};

static io_socket_state_t const*
cc2652rb_ble5_system_bus_request_enter (io_socket_t *socket) {

	// clear interrupt ...
	RFC_DOOR_BELL->RFCPEIFG.register_value = ~(
			RFC_DBELL_RFCPEIFG_BOOT_DONE
		|	RFC_DBELL_RFCPEIFG_MODULES_UNLOCKED
	);

	write_to_radio_command_register (CMDR_DIR_CMD_1BYTE(CMD_BUS_REQUEST, 1));
	return socket->State;
}

static io_socket_state_t const*
cc2652rb_ble5_system_bus_request_acknowledge (io_socket_t *socket) {
	volatile uint32_t status = RFC_DOOR_BELL->CMDSTA.register_value;
	if (status == 1) {
		return (io_socket_state_t const*) &cc2652rb_ble5_setup_radio;
	} else {
		return (io_socket_state_t const*) &cc2652rb_radio_error;
	}
}

static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_ble5_system_bus_request = {
	SPECIALISE_CC2652_RADIO_SOCKET_STATE (&io_socket_state)
	.name = "powering on",
	.enter = cc2652rb_ble5_system_bus_request_enter,
	.command_acknowledge = cc2652rb_ble5_system_bus_request_acknowledge,
};

static io_socket_state_t const*
cc2652rb_ble5_start_radio_timer_enter (io_socket_t *socket) {
	write_to_radio_command_register (CMDR_DIR_CMD(CMD_START_RAT));
	return socket->State;
}

static io_socket_state_t const*
cc2652rb_ble5_start_radio_timer_command_acknowledge (io_socket_t *socket) {
	volatile uint32_t status = RFC_DOOR_BELL->CMDSTA.register_value;
	if (status == 1) {
		return (io_socket_state_t const*) &cc2652rb_ble5_power_on;
	} else {
		return (io_socket_state_t const*) &cc2652rb_radio_error;
	}
}

static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_ble5_start_radio_timer = {
	SPECIALISE_CC2652_RADIO_SOCKET_STATE (&io_socket_state)
	.name = "start timer",
	.enter = cc2652rb_ble5_start_radio_timer_enter,
	.command_acknowledge = cc2652rb_ble5_start_radio_timer_command_acknowledge,
};

void
RF_decodeOverridePointers (rfc_CMD_RADIO_SETUP_t* radioSetup,uint16_t** pTxPower, volatile uint32_t** pRegOverride) {
   switch (radioSetup->commandNo)
   {
       case (CMD_RADIO_SETUP):
//           *pTxPower     = &radioSetup->common.txPower;
//          *pRegOverride = radioSetup->common.pRegOverride;
           break;
       case (CMD_BLE5_RADIO_SETUP):
           *pTxPower     = &((rfc_CMD_BLE5_RADIO_SETUP_t*) radioSetup)->txPower;
           *pRegOverride = ((rfc_CMD_BLE5_RADIO_SETUP_t *) radioSetup)->pRegOverrideCommon;
           break;
       case (CMD_PROP_RADIO_SETUP):
//           *pTxPower     = &radioSetup->prop.txPower;
//           *pRegOverride = radioSetup->prop.pRegOverride;
           break;
       default:
//           *pTxPower     = &radioSetup->prop_div.txPower;
//           *pRegOverride = radioSetup->prop_div.pRegOverride;
           break;
   }

}

//static int32_t RF_currentHposcFreqOffset;


static io_socket_state_t const*
cc2652rb_ble5_setup_radio_enter (io_socket_t *socket) {
	cc2652rb_ble5_socket_t *this = (cc2652rb_ble5_socket_t*) socket;

	uint32_t* pRegOverride = ((rfc_CMD_BLE5_RADIO_SETUP_t*) this->radio_setup)->pRegOverrideCommon;

   uint8_t index;
   index = RFCOverrideSearch (
   	pRegOverride,
		RF_HPOSC_OVERRIDE_PATTERN,
		RF_HPOSC_OVERRIDE_MASK,
		RF_OVERRIDE_SEARCH_DEPTH
	);

   if (index < RF_OVERRIDE_SEARCH_DEPTH) {
   	int32_t tempDegC = AONBatMonTemperatureGetDegC();
   	int32_t relFreqOffset = OSC_HPOSCRelativeFrequencyOffsetGet(tempDegC);
//   	RF_currentHposcFreqOffset = relFreqOffset;
   	int16_t relFreqOffsetConverted = OSC_HPOSCRelativeFrequencyOffsetToRFCoreFormatConvert(relFreqOffset);
   	pRegOverride[index] = HPOSC_OVERRIDE(relFreqOffsetConverted);
   }

	write_to_radio_command_register (
		(uint32_t) this->radio_setup
	);

	return socket->State;
}

static io_socket_state_t const*
cc2652rb_ble5_setup_radio_command_acknowledge (io_socket_t *socket) {
	return socket->State;
}

static io_socket_state_t const*
cc2652rb_ble5_setup_radio_command_done (io_socket_t *socket) {
	volatile uint32_t status = RFC_DOOR_BELL->CMDSTA.register_value;
	if (status == 1) {
		return (io_socket_state_t const*) &cc2652rb_ble5_start_radio_timer;
	} else {
		return (io_socket_state_t const*) &cc2652rb_radio_error;
	}
}

static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_ble5_setup_radio = {
	SPECIALISE_CC2652_RADIO_SOCKET_STATE (&io_socket_state)
	.name = "setup radio",
	.enter = cc2652rb_ble5_setup_radio_enter,
	.command_acknowledge = cc2652rb_ble5_setup_radio_command_acknowledge,
	.command_done = cc2652rb_ble5_setup_radio_command_done,
};

static io_socket_state_t const*
cc2652rb_ble5_receive_frame_enter (io_socket_t *socket) {
	cc2652rb_ble5_socket_t *this = (cc2652rb_ble5_socket_t*) socket;
	modify_receive_command (this,BLE5_PHY_1M,37,BLE_ADV_AA,0x555555);
	write_to_radio_command_register ((uint32_t) &cc2652rb_radio_generic_receive);
	return socket->State;
}

static io_socket_state_t const*
cc2652rb_ble5_receive_frame_command_done (io_socket_t *socket) {
	volatile uint32_t status = RFC_DOOR_BELL->CMDSTA.register_value;

	if (status == 1) {
		#if defined(CC2652_BLE5_SOCKET_LOG_LEVEL)
		io_log (
			io_socket_io (socket),
			IO_DETAIL_LOG_LEVEL,
			"%-*s%-*sdone\n",
			DBP_FIELD1,"radio",
			DBP_FIELD2,"snooping"
		);
		#endif
	}

	return socket->State;
}

static io_socket_state_t const*
cc2652rb_ble5_receive_frame_command_acknowledge (io_socket_t *socket) {
	volatile uint32_t status = RFC_DOOR_BELL->CMDSTA.register_value;
	if (status == 1) {
		#if defined(CC2652_BLE5_SOCKET_LOG_LEVEL)
		io_log (
			io_socket_io (socket),
			IO_DETAIL_LOG_LEVEL,
			"%-*s%-*senter\n",
			DBP_FIELD1,"radio",
			DBP_FIELD2,"listen"
		);
		#endif
	}
	return socket->State;
}

io_encoding_t*
cc2652rb_ble5_new_receive_packet (io_socket_t *socket) {
	io_encoding_t *message = io_packet_encoding_new (
		io_get_byte_memory(io_socket_io (socket))
	);

	if (message != NULL) {
/*		io_layer_t *layer = push_ble_transmit_layer (message);
		if (layer != NULL) {
//			io_layer_set_source_address (layer,message,io_socket_address(socket));
//			io_layer_set_inner_address (layer,message,IO_NULL_LAYER_ID);
			reference_io_encoding (message);
		} else {
			io_panic (io_socket_io(socket),IO_PANIC_OUT_OF_MEMORY);
		}
*/
	}

	return message;
}

//
// receive ble5 frame
//
static io_socket_state_t const*
cc2652rb_ble5_receive_frame_receive (io_socket_t *socket) {
	cc2652rb_ble5_socket_t *this = (cc2652rb_ble5_socket_t*) socket;
   rfc_dataEntryGeneral_t *next;
   ble5_packet_t *packet;
   io_inner_binding_t *rx;

   next = RFQueue_getDataEntry();
   packet = (ble5_packet_t *)(&next->data + 1);
   rx = io_multiplex_socket_find_inner_binding (
   	(io_multiplex_socket_t*) this,def_io_u8_address(ble5_packet_type(packet))
	);

   {
   	static uint32_t count = 0;
   	memory_info_t info;
   	count++;
   	if ((count % 64) == 0) {
   		io_byte_memory_get_info (io_get_byte_memory(io_socket_io(socket)),&info);
   		io_log (
   			io_socket_io (socket),
   			IO_DETAIL_LOG_LEVEL,
   			"%-*s%-*stype %u 0x%02x %u (r %u)\n",
   			DBP_FIELD1,"radio",
   			DBP_FIELD2,"rx",
   			ble5_packet_type(packet),(&next->data)[1],ble5_packet_length(packet),info.used_bytes
   		);
   	} else {
   		io_log (
   			io_socket_io (socket),
   			IO_DETAIL_LOG_LEVEL,
   			"%-*s%-*stype %u 0x%02x %u\n",
   			DBP_FIELD1,"radio",
   			DBP_FIELD2,"rx",
   			ble5_packet_type(packet),(&next->data)[1],ble5_packet_length(packet)
   		);
   	}
		}

   if (rx) {
   	io_encoding_t *msg = reference_io_encoding (
   		cc2652rb_ble5_new_receive_packet (socket)
		);
   	io_layer_t *ble  = push_io_ble5_layer (msg);
   	io_encoding_append_bytes (
   		msg,(uint8_t*) packet,ble5_packet_length(packet)
		);
   	io_layer_load_header(ble,msg);

   	// load additional info into encoding
      // 4 MHz clock, so divide by 4 to get microseconds
      //frame.timestamp = this->receive_stats.timeStamp >> 2;
      //frame.rssi = this->receive_stats.lastRssi;

   	if (io_encoding_pipe_put_encoding (io_inner_binding_receive_pipe(rx),msg)) {
   		io_enqueue_event (io_socket_io (this),io_inner_binding_receive_event(rx));
   	}

   	unreference_io_encoding (msg);
   }

   // this uses global data
   RFQueue_nextEntry();

	return socket->State;
}

static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_ble5_receive_frame = {
	SPECIALISE_CC2652_RADIO_SOCKET_STATE (&io_socket_state)
	.name = "setup radio",
	.enter = cc2652rb_ble5_receive_frame_enter,
	.command_acknowledge = cc2652rb_ble5_receive_frame_command_acknowledge,
	.command_done = cc2652rb_ble5_receive_frame_command_done,
	.outer_receive_event = cc2652rb_ble5_receive_frame_receive,
};

static io_socket_state_t const*
cc2652rb_ble5_power_on_enter (io_socket_t *socket) {

   // RF core boot process is now finished
//   PRCM0->RFCBITS.register_value |= RF_BOOT1;

	#if defined(CC2652_BLE5_SOCKET_LOG_LEVEL)
	io_log (
		io_socket_io (socket),
		IO_DETAIL_LOG_LEVEL,
		"%-*s%-*senter (rat %u)\n",
		DBP_FIELD1,"radio",
		DBP_FIELD2,"on",is_cc2652_radio_timer_running()
	);
	#endif

	return (io_socket_state_t const*) &cc2652rb_ble5_receive_frame;
}

static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_ble5_power_on = {
	SPECIALISE_CC2652_RADIO_SOCKET_STATE (&io_socket_state)
	.name = "on",
	.enter = cc2652rb_ble5_power_on_enter,
};

io_encoding_t*
cc2652rb_radio_new_binary_message (io_socket_t *socket) {
	io_encoding_t *message = reference_io_encoding (
		io_packet_encoding_new (
				io_get_byte_memory(io_socket_io (socket))
		)
	);

	if (message != NULL) {
		io_layer_t *layer = push_io_binary_transmit_layer (message);
		if (layer == NULL) {
			io_panic (io_socket_io(socket),IO_PANIC_OUT_OF_MEMORY);
		}
	}

	return message;
}

static io_socket_t*
cc2652rb_ble5_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	cc2652rb_ble5_socket_t *this = (cc2652rb_ble5_socket_t*) socket;

	cc2652rb_radio_initialise (socket,io,C);

   this->receive_stats.nRxOk = 0;			// Number of packets received with CRC OK
   this->receive_stats.nRxNok = 0;			// Number of packets received with CRC error
   this->receive_stats.nRxBufFull = 0;		// Number of packets that have been received and discarded due to lack of buffer space
   this->receive_stats.lastRssi = 0;		// The RSSI of the last received packet (signed)
   this->receive_stats.timeStamp = 0;		// Time stamp of the last received packet

	socket->State = (io_socket_state_t const*) &cc2652rb_ble5_power_off;
	io_socket_enter_current_state (socket);

	return socket;
}

static bool
cc2652rb_ble5_is_closed (io_socket_t const *socket) {
	return socket->State == (io_socket_state_t*) &cc2652rb_ble5_power_off;
}

EVENT_DATA io_socket_implementation_t
cc2652rb_ble5_socket_implementation = {
	SPECIALISE_CC2652RB_RADIO_SOCKET_IMPLEMENTATION (
		&cc2652rb_radio_socket_implementation
	)
	.initialise = cc2652rb_ble5_initialise,	\
	.is_closed = cc2652rb_ble5_is_closed,	\
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
