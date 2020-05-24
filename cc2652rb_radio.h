/*
 *
 * cc2652 radio socket
 *
 */
#ifndef cc2652rb_radio_H_
#define cc2652rb_radio_H_

#include <ti/drivers/rf/RFQueue.h>
#include <ti/driverlib/rf_ble_cmd.h>

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

typedef struct PACK_STRUCTURE cc2652rb_radio_socket {
	IO_MULTIPLEX_SOCKET_STRUCT_MEMBERS

	//
	// device parameters declared in io_device.h
	//
	io_cpu_clock_pointer_t peripheral_clock;
	radio_patch_functions_t const *patch;
	rfc_CMD_RADIO_SETUP_t *radio_setup;
	//

	rfc_radioOp_t *active_command;

	rfc_bleGenericRxOutput_t receive_stats;
	dataQueue_t rf_receive_queue;

	io_event_t command_acknowledge;
	io_event_t command_done;
	io_event_t receive_event;

} cc2652rb_radio_socket_t;

extern EVENT_DATA io_socket_implementation_t cc2652rb_radio_socket_implementation;

#ifdef IMPLEMENT_IO_CPU
//-----------------------------------------------------------------------------
//
// Implementation
//
//-----------------------------------------------------------------------------
#include <ti/driverlib/rf_common_cmd.h>
#include <ti/driverlib/rf_mailbox.h>
#include <ti/driverlib/rf_prop_cmd.h>
#include <ti/drivers/rf/RFQueue.c>


/*-------------- Internal RF constants ---------------*/

#define RF_CMD0                                0x0607
#define RF_BOOT0                               0xE0000011
#define RF_BOOT1                               0x00000080
#define RF_PHY_BOOTUP_MODE                     0
#define RF_PHY_SWITCHING_MODE                  1
#define RF_PHY_BOOTUP_MODE					0
#define BLE_ADV_AA							0x8E89BED6

typedef enum {
	PHY_1M = 0,
	PHY_2M,
	PHY_CODED
} PHY_Mode;

typedef struct {
    uint32_t timestamp; // microseconds
    uint16_t length;
    int16_t rssi;
    uint16_t channel;
    uint16_t phy;
    uint8_t *pData;
} BLE_Frame;

INLINE_FUNCTION void
write_to_radio_command_register (uint32_t rawCmd) {
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDR) = rawCmd;
}

// Structure for CMD_BLE5_GENERIC_RX.pParam
rfc_bleGenericRxPar_t bleGenericRxPar = {
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
	.phyMode.mainMode = PHY_1M,
	.phyMode.coding = 0x0,
	.rangeDelay = 0x00,
	.txPower = 0x0000,
	.pParams = &bleGenericRxPar,
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
	cc2652rb_radio_socket_t *this,
	PHY_Mode phy,
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
   cc2652rb_radio_generic_receive.pParams->rxConfig.bAppendRssi = 0;
   cc2652rb_radio_generic_receive.pParams->rxConfig.bAppendStatus = 0;
   cc2652rb_radio_generic_receive.pParams->rxConfig.bAppendTimestamp = 0;

}

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

/*
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 *
 * radio socket states
 *
 *               <cpu reset>
 *                 |
 *                 v
 *               cc2652rb_radio_power_off
 *           <open>|
 *                 v
 *               cc2652rb_radio_system_bus_request
 *                 |
 *                 v
 *               cc2652rb_radio_setup_radio
 *                 |
 *                 v
 *               cc2652rb_radio_start_radio_timer
 *                 |
 *                 v
 *               cc2652rb_radio_power_on
 *
 *
 *     <any> --> cc2652rb_radio_error
 *
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */
static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_radio_power_off;
static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_radio_system_bus_request;
static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_radio_start_radio_timer;
static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_radio_setup_radio;
static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_radio_power_on;
static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_radio_error;

//
// 
//
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

bool
RF_ratIsRunning(void) {
    /* Assume by default that the RAT is not available. */
    bool status = false;

    /* If the RF core power domain is ON, read the clock of the RAT. */
    if (HWREG(PRCM_BASE + PRCM_O_PDSTAT0) & PRCM_PDSTAT0_RFC_ON)
    {
        status = (bool)(HWREG(RFC_PWR_BASE + RFC_PWR_O_PWMCLKEN) & RFC_PWR_PWMCLKEN_RAT_M);
    }

    /* Return with the status of RAT. */
    return(status);
}

static io_socket_state_t const*
cc2652rb_radio_power_off_open (
	io_socket_t *socket,io_socket_open_flag_t flag
) {
	switch (flag) {
		case IO_SOCKET_OPEN_CONNECT:
		break;

		case IO_SOCKET_OPEN_LISTEN:
			// not supported

		default:
			return socket->State;
	}

	// Notes
	//
	// 1	PRCM:RFCBITS control boot process for radio CPU
	// 2	CMDR register can only be written while it reads 0
	// 	the radio CPU set it back to zero 'immediately'

	//  Enable output RTC clock for Radio Timer Synchronization
   HWREG(AON_RTC_BASE + AON_RTC_O_CTL) |= AON_RTC_CTL_RTC_UPD_EN_M;

	// Set the automatic bus request
	HWREG(PRCM_BASE + PRCM_O_RFCBITS) = RF_BOOT0;

	cc2652rb_radio_socket_t *this = (cc2652rb_radio_socket_t*) socket;
	if (io_cpu_clock_start (this->io,this->peripheral_clock)) {
	   uint32_t availableRfModes = HWREG(PRCM_BASE + PRCM_O_RFCMODEHWOPT);

	   if (availableRfModes & (1 << RF_MODE_BLE)) {
	   	cc2652rb_radio_socket_t *this = (cc2652rb_radio_socket_t*) socket;
	   	//
	   	// do stuff from power-up-state
	   	//
			// Set the RF mode in the PRCM register (already verified that it is valid)
	   	//
			HWREG(PRCM_BASE + PRCM_O_RFCMODESEL) = RF_MODE_AUTO;

			HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEISL) = CC2652_CPE1_INTERRUPTS;

			HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG) = 0;
			HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIEN) = (
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
					return (io_socket_state_t const*) &cc2652rb_radio_system_bus_request;	// wrong
				}
			}

			//
			// Use the RAT_SYNC command in SETUP chain to start RAT
			//

			// goto active
			return (io_socket_state_t const*) &cc2652rb_radio_system_bus_request;
	   }
	}

	return socket->State;
}

static io_socket_state_t const*
cc2652rb_radio_power_off_command_acknowledge (io_socket_t *socket) {
	return (io_socket_state_t const*) &cc2652rb_radio_system_bus_request;
}

static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_radio_power_off = {
	SPECIALISE_CC2652_RADIO_SOCKET_STATE (&io_socket_state)
	.name = "off",
	.open = cc2652rb_radio_power_off_open,
	.command_acknowledge = cc2652rb_radio_power_off_command_acknowledge,
};

static io_socket_state_t const*
cc2652rb_radio_system_bus_request_enter (io_socket_t *socket) {

	// clear interrupt ...
	RFCCpeIntClear((uint32_t) RFC_DBELL_RFCPEIFG_BOOT_DONE | RFC_DBELL_RFCPEIFG_MODULES_UNLOCKED);

	write_to_radio_command_register (CMDR_DIR_CMD_1BYTE(CMD_BUS_REQUEST, 1));
	return socket->State;
}

static io_socket_state_t const*
cc2652rb_radio_system_bus_request_acknowledge (io_socket_t *socket) {
	volatile uint32_t status = HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDSTA);
	if (status == 1) {
		return (io_socket_state_t const*) &cc2652rb_radio_setup_radio;
	} else {
		return (io_socket_state_t const*) &cc2652rb_radio_error;
	}
}

static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_radio_system_bus_request = {
	SPECIALISE_CC2652_RADIO_SOCKET_STATE (&io_socket_state)
	.name = "powering on",
	.enter = cc2652rb_radio_system_bus_request_enter,
	.command_acknowledge = cc2652rb_radio_system_bus_request_acknowledge,
};

static io_socket_state_t const*
cc2652rb_radio_start_radio_timer_enter (io_socket_t *socket) {
	write_to_radio_command_register (CMDR_DIR_CMD(CMD_START_RAT));
	return socket->State;
}

static io_socket_state_t const*
cc2652rb_radio_start_radio_timer_command_acknowledge (io_socket_t *socket) {
	volatile uint32_t status = HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDSTA);
	if (status == 1) {
		return (io_socket_state_t const*) &cc2652rb_radio_power_on;
	} else {
		return (io_socket_state_t const*) &cc2652rb_radio_error;
	}
}

static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_radio_start_radio_timer = {
	SPECIALISE_CC2652_RADIO_SOCKET_STATE (&io_socket_state)
	.name = "start timer",
	.enter = cc2652rb_radio_start_radio_timer_enter,
	.command_acknowledge = cc2652rb_radio_start_radio_timer_command_acknowledge,
};

void
RF_decodeOverridePointers (rfc_CMD_RADIO_SETUP_t* radioSetup,uint16_t** pTxPower, uint32_t** pRegOverride) {
   switch (radioSetup->commandNo)
   {
       case (CMD_RADIO_SETUP):
//           *pTxPower     = &radioSetup->common.txPower;
//          *pRegOverride = radioSetup->common.pRegOverride;
           break;
       case (CMD_BLE5_RADIO_SETUP):
           *pTxPower     = &((rfc_CMD_BLE5_RADIO_SETUP_t*) radioSetup)->txPower;
           *pRegOverride = ((rfc_CMD_BLE5_RADIO_SETUP_t*) radioSetup)->pRegOverrideCommon;
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

#define RF_HPOSC_OVERRIDE_PATTERN              HPOSC_OVERRIDE(0)
#define RF_HPOSC_OVERRIDE_MASK                 0xFFFF
/* Common defines for override handling*/
#define RF_OVERRIDE_SEARCH_DEPTH               80
/* Define for HPOSC temperature limits */
#define RF_TEMP_LIMIT_3_DEGREES_CELSIUS        0x300

static int32_t                  RF_currentHposcFreqOffset;


static io_socket_state_t const*
cc2652rb_radio_setup_radio_enter (io_socket_t *socket) {
	cc2652rb_radio_socket_t *this = (cc2652rb_radio_socket_t*) socket;

//   uint16_t* pTxPower = NULL;
   uint32_t* pRegOverride = ((rfc_CMD_BLE5_RADIO_SETUP_t*) this->radio_setup)->pRegOverrideCommon;

//   RF_decodeOverridePointers (this->radio_setup,&pTxPower,&pRegOverride);

   uint8_t index;
   index = RFCOverrideSearch(pRegOverride, RF_HPOSC_OVERRIDE_PATTERN, RF_HPOSC_OVERRIDE_MASK, RF_OVERRIDE_SEARCH_DEPTH);

   if (index < RF_OVERRIDE_SEARCH_DEPTH) {
   	int32_t tempDegC = AONBatMonTemperatureGetDegC();
   	int32_t relFreqOffset = OSC_HPOSCRelativeFrequencyOffsetGet(tempDegC);
   	RF_currentHposcFreqOffset = relFreqOffset;
   	int16_t relFreqOffsetConverted = OSC_HPOSCRelativeFrequencyOffsetToRFCoreFormatConvert(relFreqOffset);
   	pRegOverride[index] = HPOSC_OVERRIDE(relFreqOffsetConverted);
   }

	write_to_radio_command_register (
		(uint32_t) this->radio_setup
	);

	return socket->State;
}

static io_socket_state_t const*
cc2652rb_radio_setup_radio_command_acknowledge (io_socket_t *socket) {
	return socket->State;
}

static io_socket_state_t const*
cc2652rb_radio_setup_radio_command_done (io_socket_t *socket) {
	volatile uint32_t status = HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDSTA);
	if (status == 1) {
		return (io_socket_state_t const*) &cc2652rb_radio_start_radio_timer;
	} else {
		return (io_socket_state_t const*) &cc2652rb_radio_error;
	}
}

static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_radio_setup_radio = {
	SPECIALISE_CC2652_RADIO_SOCKET_STATE (&io_socket_state)
	.name = "setup radio",
	.enter = cc2652rb_radio_setup_radio_enter,
	.command_acknowledge = cc2652rb_radio_setup_radio_command_acknowledge,
	.command_done = cc2652rb_radio_setup_radio_command_done,
};

static io_socket_state_t const*
cc2652rb_radio_receive_frame_enter (io_socket_t *socket) {
	cc2652rb_radio_socket_t *this = (cc2652rb_radio_socket_t*) socket;
	modify_receive_command (this,PHY_1M,37,BLE_ADV_AA,0x555555);
	write_to_radio_command_register ((uint32_t) &cc2652rb_radio_generic_receive);
	return socket->State;
}

static io_socket_state_t const*
cc2652rb_radio_receive_frame_command_done (io_socket_t *socket) {
	volatile uint32_t status = HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDSTA);

	if (status == 1) {
		#if defined(CC2652_RADIO_SOCKET_LOG_LEVEL)
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
cc2652rb_radio_receive_frame_command_acknowledge (io_socket_t *socket) {
	volatile uint32_t status = HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDSTA);
	if (status == 1) {
		#if defined(CC2652_RADIO_SOCKET_LOG_LEVEL)
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

static io_socket_state_t const*
cc2652rb_radio_receive_frame_receive (io_socket_t *socket) {
	cc2652rb_radio_socket_t *this = (cc2652rb_radio_socket_t*) socket;
   BLE_Frame frame;
   static uint32_t count = 0;

   rfc_dataEntryGeneral_t *currentDataEntry;
   uint8_t *packetPointer;

   currentDataEntry = RFQueue_getDataEntry();
   packetPointer = (uint8_t *)(&currentDataEntry->data);

   frame.length = packetPointer[2] + 2;
   frame.pData = packetPointer + 1;

   /* 4 MHz clock, so divide by 4 to get microseconds */
   frame.timestamp = this->receive_stats.timeStamp >> 2;
   frame.rssi = this->receive_stats.lastRssi;

   frame.channel = 37;
   frame.phy = 0;

   RFQueue_nextEntry();

	#if defined(CC2652_RADIO_SOCKET_LOG_LEVEL)
	io_log (
		io_socket_io (socket),
		CC2652_RADIO_SOCKET_LOG_LEVEL,
		"%-*s%-*slength = %u\n",
		DBP_FIELD1,"radio",
		DBP_FIELD2,"rx",
		frame.length
	);
	#endif
	count ++;

	return socket->State;
}

static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_radio_receive_frame = {
	SPECIALISE_CC2652_RADIO_SOCKET_STATE (&io_socket_state)
	.name = "setup radio",
	.enter = cc2652rb_radio_receive_frame_enter,
	.command_acknowledge = cc2652rb_radio_receive_frame_command_acknowledge,
	.command_done = cc2652rb_radio_receive_frame_command_done,
	.receive = cc2652rb_radio_receive_frame_receive,
};

static io_socket_state_t const*
cc2652rb_radio_power_on_enter (io_socket_t *socket) {

   // RF core boot process is now finished
//   HWREG(PRCM_BASE + PRCM_O_RFCBITS) |= RF_BOOT1;

	#if defined(CC2652_RADIO_SOCKET_LOG_LEVEL)
	io_log (
		io_socket_io (socket),
		IO_DETAIL_LOG_LEVEL,
		"%-*s%-*senter (rat %u)\n",
		DBP_FIELD1,"radio",
		DBP_FIELD2,"on",RF_ratIsRunning()
	);
	#endif

	return (io_socket_state_t const*) &cc2652rb_radio_receive_frame;
//	return socket->State;
}

static EVENT_DATA io_cc2652_radio_socket_state_t cc2652rb_radio_power_on = {
	SPECIALISE_CC2652_RADIO_SOCKET_STATE (&io_socket_state)
	.name = "on",
	.enter = cc2652rb_radio_power_on_enter,
};

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

//-----------------------------------------------------------------------------
//
// the socket
//
//-----------------------------------------------------------------------------
/*
	 RFCCpeIntClear((uint32_t) 0);
	 RFCCpeIntEnable((uint32_t) 0);
	 RFCHwIntClear((uint32_t) 0);
	 RFCHwIntEnable((uint32_t) 0);
 */

static void
cc2652rb_radio_cpe0_interrupt (void *user_value) {
	uint32_t status = HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG);
	cc2652rb_radio_socket_t *this = user_value;

	RFCCpeIntClear(CC2652_CPE0_INTERRUPTS);

	if (status & RFC_DBELL_RFCPEIFG_COMMAND_DONE) {
		io_enqueue_event (io_socket_io(this),&this->command_done);
	}

}

static void
cc2652rb_radio_cpe1_interrupt (void *user_value) {
	uint32_t status = HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFCPEIFG);
	cc2652rb_radio_socket_t *this = user_value;

	RFCCpeIntClear(CC2652_CPE1_INTERRUPTS);

	if (status & RFC_DBELL_RFCPEIFG_RX_ENTRY_DONE) {
		io_enqueue_event (io_socket_io(this),&this->receive_event);
	}
}

#define RF_HW_INT_CPE_MASK			RFC_DBELL_RFHWIFG_MDMSOFT
#define RF_HW_INT_RAT_CH_MASK		(RFC_DBELL_RFHWIFG_RATCH7 | RFC_DBELL_RFHWIFG_RATCH6 | RFC_DBELL_RFHWIFG_RATCH5)

static void
cc2652rb_radio_hardware_interrupt (void *user_value) {
   uint32_t rfchwifg = RFCHwIntGetAndClear(RF_HW_INT_CPE_MASK | RF_HW_INT_RAT_CH_MASK);
   uint32_t rfchwien = HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFHWIEN) & RF_HW_INT_CPE_MASK;
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
	HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0;
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
	io_socket_call_state (
		socket,((io_cc2652_radio_socket_state_t const*) socket->State)->receive
	);
}

static io_socket_t*
cc2652rb_radio_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	cc2652rb_radio_socket_t *this = (cc2652rb_radio_socket_t*) socket;
	uint32_t size_of_rx_buffer = RF_QUEUE_DATA_ENTRY_BUFFER_SIZE (
		CC2652RB_RECEIVE_QUEUE_LENGTH,
		CC2652RB_MAX_RECEIVE_LENGTH,
		CC2652RB_RECEIVE_APPENDED_SIZE
	);

	initialise_io_multiplex_socket (socket,io,C);

	this->active_command = NULL;
   this->receive_stats.nRxOk = 0;                      //!<        Number of packets received with CRC OK
   this->receive_stats.nRxNok = 0;                     //!<        Number of packets received with CRC error
   this->receive_stats.nRxBufFull = 0;                 //!<        Number of packets that have been received and discarded due to lack of buffer space
   this->receive_stats.lastRssi = 0;                     //!<        The RSSI of the last received packet (signed)
   this->receive_stats.timeStamp = 0;                   //!<        Time stamp of the last received packet


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

	socket->State = (io_socket_state_t const*) &cc2652rb_radio_power_off;
	io_socket_enter_current_state (socket);

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
	cc2652rb_radio_socket_t *this = (cc2652rb_radio_socket_t*) socket;

	if (cpu_clock_is_derrived_from_hp_oscillator (this->peripheral_clock)) {
		io_event_t *ev = io_byte_memory_allocate (
			io_get_byte_memory (io_socket_io (socket)),sizeof(io_event_t)
		);

		initialise_io_event (ev,cc2652rb_radio_open_event,socket);
		io_enqueue_event (io_socket_io (socket),ev);

		return true;
	} else {
		return false;
	}
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
