/*
 *
 * io dma adapter for cc2652 udma 
 *
 */
#ifndef cc2652rb_udma_H_
#define cc2652rb_udma_H_

typedef struct PACK_STRUCTURE cc2652_io_dma_channel {
	IO_DMA_CHANNEL_STRUCT_MEMBERS

	struct PACK_STRUCTURE {
		uint32_t dma_block:2;			// 1|2
		uint32_t channel_number:6;		//
		uint32_t peripheral_id:8;		// ??
		uint32_t priority:2;				// 0 .. 3
		uint32_t direction:1;			//
		uint32_t circular:1;				//
		uint32_t :12;						//
	} bit;
	uint32_t peripheral_address;
} cc2652_io_dma_channel_t;

#define cc2652_io_dma_channel_number(d) (d)->bit.channel_number


bool	initialise_io_cpu_dma (io_t*,io_cpu_clock_pointer_t);
void	cc2652_register_dma_channel (io_t*,io_dma_channel_t*);

extern EVENT_DATA io_dma_channel_implementation_t cc2652_dma_channel_implementation;
#include <ti/driverlib/udma.h>

#ifdef IMPLEMENT_IO_CPU_DMA
//-----------------------------------------------------------------------------
//
// Implementation
//
//-----------------------------------------------------------------------------
#include <ti/driverlib/udma.c>

typedef struct {
    volatile uint32_t source_address;
    volatile uint32_t destination_address;
    volatile uint32_t ui32Control;
    volatile uint32_t ui32Spare;
} cc2652_dma_control_t;

__attribute__ ((section(".dma_control_blocks")))
cc2652_dma_control_t dma_control[UDMA_NUM_CHANNELS];

//
// only valid for memory-to-memory transfers
//
static void
cc2652_dma_done_interrupt (void *user_value) {
	io_cc2652_cpu_t *this = user_value;
	cc2652_io_dma_channel_t *channel = (cc2652_io_dma_channel_t*) (
		this->dma_channel_list
	);

	while (channel) {
		uint32_t mask = (1 << cc2652_io_dma_channel_number(channel));
		if (uDMAIntStatus(UDMA0_BASE) & mask) {
			uDMAIntClear(UDMA0_BASE,mask);
			io_enqueue_event (user_value,&channel->complete);
		}
		channel = (cc2652_io_dma_channel_t*) channel->next_channel;
	}

}

static void
cc2652_dma_error_interrupt (void *user_value) {
	while(1);
}

bool
initialise_io_cpu_dma (io_t *io,io_cpu_clock_pointer_t dma_clock) {
	cc2652_dma_control_t *dma = dma_control;
	cc2652_dma_control_t *end = dma + UDMA_NUM_CHANNELS;
	while (dma < end) {
		dma->ui32Control = 0;
		dma++;
	}

	io_cpu_clock_start (io,dma_clock);

	HWREG (UDMA0_BASE + UDMA_O_CLEARCHANNELEN) = (1 << UDMA_NUM_CHANNELS) - 1;
	HWREG (UDMA0_BASE + UDMA_O_CTRL) = (uint32_t) dma_control;

	register_io_interrupt_handler (
		io,INT_DMA_DONE_COMB,cc2652_dma_done_interrupt,io
	);

	register_io_interrupt_handler (
		io,INT_DMA_ERR,cc2652_dma_error_interrupt,io
	);

	IntPrioritySet(INT_DMA_DONE_COMB,NORMAL_INTERRUPT_PRIORITY);
	IntPrioritySet(INT_DMA_ERR,NORMAL_INTERRUPT_PRIORITY);

	IntPendClear (INT_DMA_DONE_COMB);
	IntPendClear (INT_DMA_ERR);

	IntEnable (INT_DMA_DONE_COMB);
	IntEnable (INT_DMA_ERR);

	uDMAEnable (UDMA0_BASE);

	return true;
}

void
cc2652_register_dma_channel (io_t *io,io_dma_channel_t *channel) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;
	if (channel->next_channel == NULL) {
		channel->next_channel = this->dma_channel_list;
		this->dma_channel_list = channel;
	}
}

static void
cc2652_io_dma_transfer_from_peripheral (
	io_dma_channel_t *channel,void *dest,uint32_t length
) {

}

static void
cc2652_io_dma_transfer_to_peripheral (
	io_dma_channel_t *channel,void const *src,uint32_t size
) {
	cc2652_io_dma_channel_t *cc_channel = (cc2652_io_dma_channel_t*) channel;
	cc2652_dma_control_t *dma = dma_control + cc2652_io_dma_channel_number(cc_channel);

	if (size <= UDMA_XFER_SIZE_MAX) {
		uint32_t mask = (1 << cc2652_io_dma_channel_number(cc_channel));

		dma->source_address = ((uint32_t) src) + size - 1;
		dma->destination_address = (uint32_t) cc_channel->peripheral_address;
		dma->ui32Control = (
				UDMA_SRC_INC_8
			|	UDMA_DST_INC_NONE
			|	UDMA_SIZE_8
			|	UDMA_ARB_4
			|	UDMA_MODE_BASIC
			|	(size << UDMA_XFER_SIZE_S)
		);

		HWREG (UDMA0_BASE + UDMA_O_CLEARREQMASK) = mask;
		HWREG (UDMA0_BASE + UDMA_O_SETCHANNELEN) = mask;
	} else {
		// ??
	}
}

static void
cc2652_io_dma_channel_transfer_complete (io_t *io,io_dma_channel_t *channel) {
	cc2652_io_dma_channel_t *cc_channel = (cc2652_io_dma_channel_t*) channel;
	volatile uint32_t mask = (1 << cc2652_io_dma_channel_number(cc_channel));
	uDMAIntClear(UDMA0_BASE,mask);
	io_enqueue_event (io,&channel->complete);
}

EVENT_DATA io_dma_channel_implementation_t cc2652_dma_channel_implementation = {
	SPECIALISE_IO_DMA_CHANNEL_IMPLEMENTATION(&dma_channel_implementation)
	.transfer_from_peripheral = cc2652_io_dma_transfer_from_peripheral,
	.transfer_to_peripheral = cc2652_io_dma_transfer_to_peripheral,
	.transfer_complete = cc2652_io_dma_channel_transfer_complete,
};


#endif /* IMPLEMENT_IO_CPU_DMA */
#endif
