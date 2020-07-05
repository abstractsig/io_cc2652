/*
 *
 */
#ifndef io_cpu_H_
#define io_cpu_H_
#include <io_core.h>
#define __RFC_STRUCT PACK_STRUCTURE
#define __RFC_STRUCT_ATTR
#include <cc2652rb.h>

void cc2652_do_gc (io_t*,int32_t);
io_cpu_clock_pointer_t cc2652_get_core_clock (io_t*);
io_uid_t const* cc2652_get_uid (io_t*);
void cc2652_signal_event_pending (io_t*);
void cc2652_wait_for_event (io_t*);
void cc2652_wait_for_all_events (io_t*);
void cc2652_set_io_pin_to_output (io_t*,io_pin_t);
void cc2652_set_io_pin_to_input (io_t*,io_pin_t);
void cc2652_set_io_pin_to_alternate (io_t*,io_pin_t);
void cc2652_release_io_pin (io_t*,io_pin_t);
int32_t cc2652_read_io_input_pin (io_t*,io_pin_t);
void cc2652_toggle_io_pin (io_t*,io_pin_t);
bool cc2652_io_pin_is_valid (io_t *io,io_pin_t rpin);
uint32_t cc2652_get_prbs_random_u32 (io_t *io);
bool cc2652_enter_critical_section (io_t*);
void cc2652_exit_critical_section (io_t*,bool);
bool cc2652_is_in_event_thread (io_t*);
io_time_t cc2652_get_time (io_t*);
void cc2652_register_interrupt_handler (io_t*,int32_t,io_interrupt_action_t,void*);
bool	cc2652_unregister_interrupt_handler (io_t*,int32_t,io_interrupt_action_t);
void cc2652_log (io_t*,char const*,va_list);
void cc2652_time_clock_enqueue_alarm (io_t*,io_alarm_t*);
void cc2652_time_clock_dequeue_alarm (io_t*,io_alarm_t*);
bool cc2652_is_first_run (io_t*);
bool cc2652_clear_first_run (io_t*);
void cc2652_flush_log (io_t*);
void cc2652_stack_usage_info (io_t*,memory_info_t*);

#define SPECIALISE_IO_CPU_IMPLEMENTATION(S) \
	SPECIALISE_IO_IMPLEMENTATION(S) \
	.do_gc = cc2652_do_gc,\
	.get_stack_usage_info = cc2652_stack_usage_info,\
	.signal_event_pending = cc2652_signal_event_pending,\
	.uid = cc2652_get_uid,\
	.is_first_run = cc2652_is_first_run,\
	.clear_first_run = cc2652_clear_first_run,\
	.wait_for_event = cc2652_wait_for_event,\
	.wait_for_all_events = cc2652_wait_for_all_events,\
	.enqueue_alarm = cc2652_time_clock_enqueue_alarm,\
	.dequeue_alarm = cc2652_time_clock_dequeue_alarm,\
	.get_time = cc2652_get_time,\
	.set_io_pin_output = cc2652_set_io_pin_to_output,\
	.set_io_pin_input = cc2652_set_io_pin_to_input,\
	.set_io_pin_alternate = cc2652_set_io_pin_to_alternate,\
	.set_io_pin_interrupt = cc2652_set_io_pin_interrupt,\
	.read_from_io_pin = cc2652_read_io_input_pin,\
	.write_to_io_pin = cc2652_write_to_io_pin,\
	.toggle_io_pin = cc2652_toggle_io_pin,\
	.valid_pin = cc2652_io_pin_is_valid,\
	.release_io_pin = cc2652_release_io_pin,\
	.get_next_prbs_u32 = cc2652_get_prbs_random_u32,\
	.enter_critical_section = cc2652_enter_critical_section,\
	.exit_critical_section = cc2652_exit_critical_section,\
	.in_event_thread = cc2652_is_in_event_thread,\
	.register_interrupt_handler = cc2652_register_interrupt_handler,\
	.unregister_interrupt_handler = cc2652_unregister_interrupt_handler,\
	.log = cc2652_log,\
	.flush_log = cc2652_flush_log,\
   /**/

typedef struct PACK_STRUCTURE cc2652_time_clock {
    io_cpu_clock_pointer_t clock;
    io_event_t alarm;
    io_t *io;
} cc2652_time_clock_t;

#define CC2652_IO_CPU_STRUCT_MEMBERS \
	IO_STRUCT_MEMBERS               \
	uint32_t in_event_thread;\
	io_value_pipe_t *tasks;\
	io_cpu_clock_pointer_t gpio_clock; \
	uint32_t prbs_state[4]; \
	cc2652_time_clock_t rtc;\
	uint32_t first_run;\
	io_dma_channel_t *dma_channel_list;\
	/**/

typedef struct PACK_STRUCTURE io_cc2652_cpu {
    CC2652_IO_CPU_STRUCT_MEMBERS
} io_cc2652_cpu_t;

void	initialise_io_cpu (io_t*);
void	cc2652_start_gpio_clock (io_t*);
void	start_time_clock (io_cc2652_cpu_t*);

#include <cc2652rb_clocks.h>
#include <cc2652rb_pins.h>
#include <cc2652rb_uart.h>
#include <cc2652rb_radio.h>
#include <cc2652_twi_master.h>

#ifdef IMPLEMENT_IO_CPU
//-----------------------------------------------------------------------------
//
// implementation
//
//-----------------------------------------------------------------------------
#include <cc2652rb_time.h>

void
cc2652_start_gpio_clock (io_t *io) {
    io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;
    io_cpu_clock_start (io,this->gpio_clock);
}

INLINE_FUNCTION uint32_t prbs_rotl(const uint32_t x, int k) {
    return (x << k) | (x >> (32 - k));
}

uint32_t
cc2652_get_prbs_random_u32 (io_t *io) {
    io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;
    uint32_t *s = this->prbs_state;
    const uint32_t result = prbs_rotl (s[0] + s[3], 7) + s[0];

    const uint32_t t = s[1] << 9;

    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];

    s[2] ^= t;

    s[3] = prbs_rotl (s[3], 11);

    return result;
}

static PERSISTANT_MEMORY_SECTION io_persistant_state_t io_config = {
    .first_run_flag = IO_FIRST_RUN_SET,
    .power_cycles = 0,
    .uid = {{0}},
    .secret = {{0}},
    .shared = {{0}},
};

/*
 *
 *  When updating the Flash, the VIMS (Vesatile Instruction Memory System)
 *  mode must be set to GPRAM or OFF, before programming, and both VIMS
 *  flash line buffers must be set to disabled.
 *
 */
static uint8_t
disableFlashCache(void) {
    uint8_t mode = VIMSModeGet(VIMS_BASE);

    VIMSLineBufDisable(VIMS_BASE);

    if (mode != VIMS_MODE_DISABLED) {
        VIMSModeSet(VIMS_BASE, VIMS_MODE_DISABLED);
        while (VIMSModeGet(VIMS_BASE) != VIMS_MODE_DISABLED);
    }

    return (mode);
}

static void
restoreFlashCache(uint8_t mode) {
    if (mode != VIMS_MODE_DISABLED) {
        VIMSModeSet(VIMS_BASE, VIMS_MODE_ENABLED);
    }

    VIMSLineBufEnable(VIMS_BASE);
}

bool
cc2652_clear_first_run (io_t *io) {
    if (io_config.first_run_flag == IO_FIRST_RUN_SET) {
        io_persistant_state_t new_ioc = io_config;
        uint8_t mode = disableFlashCache ();
        uint32_t sector_address = (uint32_t) &io_config;

        DISABLE_INTERRUPTS;

        new_ioc.first_run_flag = IO_FIRST_RUN_CLEAR;

        if (FlashProtectionGet (sector_address) == FLASH_NO_PROTECT) {
            FlashSectorErase (sector_address);
            FlashProgram (
                (uint8_t*) &new_ioc,sector_address,sizeof(io_persistant_state_t)
            );
        }

        sector_address = FlashCheckFsmForError ();

        restoreFlashCache (mode);

        ENABLE_INTERRUPTS;

        return memcmp (&new_ioc,&io_config,sizeof(io_persistant_state_t)) == 0;
    } else {
        return true;
    }
}

static bool
cc2652_io_config_read_first_run (void) {
    bool first = (io_config.first_run_flag == IO_FIRST_RUN_SET);
//    cc2652_clear_first_run ();
    return first;
}

static io_interrupt_handler_t cpu_interrupts[NUMBER_OF_INTERRUPT_VECTORS];

void
cc2652_register_interrupt_handler (
	io_t *io,int32_t number,io_interrupt_action_t handler,void *user_value
) {
	io_interrupt_handler_t *i = cpu_interrupts + number;
	i->action = handler;
	i->user_value = user_value;
}

static void
null_interrupt_handler (void *w) {
	while(1);
}

bool
cc2652_unregister_interrupt_handler (
	io_t *io,int32_t number,io_interrupt_action_t handler
) {
	io_interrupt_handler_t *i = cpu_interrupts + number;
	if (i->action == handler) {
		i->action = null_interrupt_handler;
		i->user_value = io;
		return true;
	} else {
		return false;
	}
}

static void
hard_fault (void *io) {
	DISABLE_INTERRUPTS;
	while(1);
}

static void
event_thread (void *io) {
	io_cc2652_cpu_t *this = io;
	this->in_event_thread = true;
	while (next_io_event (io));
	this->in_event_thread = false;
}

void
initialise_io_cpu (io_t *io) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;

	this->in_event_thread = false;
	this->first_run = cc2652_io_config_read_first_run ();
	this->dma_channel_list = &null_dma_channel;

	io_cpu_clock_start (io,io_get_core_clock(io));

	this->prbs_state[0] = io_get_random_u32(io);
	this->prbs_state[1] = 0xf542d2d3;
	this->prbs_state[2] = 0x6fa035c3;
	this->prbs_state[3] = 0x77f2db5b;
	register_io_interrupt_handler (io,INT_PENDSV,event_thread,io);
	register_io_interrupt_handler (io,INT_HARD_FAULT,hard_fault,io);

	start_time_clock (this);
}

void
cc2652_do_gc (io_t *io,int32_t count) {
    io_value_memory_do_gc (io_get_short_term_value_memory (io),count);
}

void
cc2652_signal_event_pending (io_t *io) {
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void
cc2652_wait_for_event (io_t *io) {
    __WFI();
}

void
cc2652_wait_for_all_events (io_t *io) {
	io_event_t *event;
	io_alarm_t *alarm;
	do {
		ENTER_CRITICAL_SECTION(io);
		event = io->events;
		alarm = io->alarms;
		EXIT_CRITICAL_SECTION(io);
	} while (
			event != &s_null_io_event
		||	alarm != &s_null_io_alarm
	);
}

io_uid_t const*
cc2652_get_uid (io_t *io) {
    return &io_config.uid;
}

bool
cc2652_is_first_run (io_t *io) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;
	return this->first_run;
}

bool
cc2652_enter_critical_section (io_t *io) {
	uint32_t interrupts_are_enabled = !(__get_PRIMASK() & 0x1);
	DISABLE_INTERRUPTS;
	return interrupts_are_enabled;
}

void
cc2652_exit_critical_section (io_t *io,bool were_enabled) {
	if (were_enabled) {
		ENABLE_INTERRUPTS;
	}
}

bool
cc2652_is_in_event_thread (io_t *io) {
	return ((io_cc2652_cpu_t*) io)->in_event_thread;
}

void
cc2652_log (io_t *io,char const *fmt,va_list va) {
	io_socket_t *print = io_get_socket (io,IO_PRINTF_SOCKET);
	if (print) {
		io_encoding_t *msg = io_socket_new_message (print);
		if (msg) {
			io_encoding_print (msg,fmt,va);
			io_socket_send_message (print,msg);
		} else {
			io_panic (io,IO_PANIC_OUT_OF_MEMORY);
		}
	}
}

void
cc2652_flush_log (io_t *io) {
	io_socket_t *print = io_get_socket (io,IO_PRINTF_SOCKET);
	if (print) {
		io_socket_flush (print);
	}
}

static void
initialise_ram_interrupt_vectors (void) {
	io_interrupt_handler_t *i = cpu_interrupts;
	io_interrupt_handler_t *e = i + NUMBER_OF_INTERRUPT_VECTORS;
	while (i < e) {
		i->action = null_interrupt_handler;
		i->user_value = NULL;
		i++;
	}
}

void
cc2652_stack_usage_info (io_t *io,memory_info_t *info) {
   extern uint32_t ld_end_of_static_ram_allocations;
   extern uint32_t ld_top_of_c_stack;
   uint32_t* cursor = &ld_end_of_static_ram_allocations;
   uint32_t* end = &ld_top_of_c_stack;
   uint32_t count = 0;

   info->total_bytes = (end - cursor) * 4;

   while (cursor < end) {
   	if (*cursor != 0xdeadc0de) break;
   	count++;
   	cursor++;
   }

   info->free_bytes = count * 4;
   info->used_bytes = info->total_bytes - info->free_bytes;
}


static void
initialise_c_runtime (void) {
	extern uint32_t ld_start_of_sdata_in_flash;
	extern uint32_t ld_start_of_sdata_in_ram,ld_end_of_sdata_in_ram;
	extern uint32_t ld_start_of_bss,ld_end_of_bss;

	uint32_t *src = &ld_start_of_sdata_in_flash;
	uint32_t *dest = &ld_start_of_sdata_in_ram;

	while(dest < &ld_end_of_sdata_in_ram) *dest++ = *src++;
	dest = &ld_start_of_bss;
	while(dest < &ld_end_of_bss) *dest++ = 0;

	// fill stack/heap region of RAM with a pattern
	extern uint32_t ld_end_of_static_ram_allocations;
	uint32_t *end = (uint32_t*) __get_MSP();
	dest = &ld_end_of_static_ram_allocations;
	while (dest < end) *dest++ = 0xdeadc0de;

	initialise_ram_interrupt_vectors();
}

int main(void);
extern const void* s_flash_vector_table[];
void
cc2652_core_reset (void) {
	#if defined (__ARM_ARCH_7EM__) && defined(__VFP_FP__) && !defined(__SOFTFP__)
	volatile uint32_t * pui32Cpacr = (uint32_t *) (CPU_SCS_BASE + CPU_SCS_O_CPACR); 
	*pui32Cpacr |= (0xF << 20);
	#endif

	DISABLE_INTERRUPTS;

	volatile uint32_t *vtor = (uint32_t *) (CPU_SCS_BASE + CPU_SCS_O_VTOR); // 
	*vtor = (uint32_t) s_flash_vector_table;

	SetupTrimDevice();
	initialise_c_runtime();


	ENABLE_INTERRUPTS;

	main();
	while(1);
}

void
handle_io_cpu_interrupt (void) {
	uint32_t index = (
		SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk
	);
	io_interrupt_handler_t const *interrupt = &cpu_interrupts[index];
	interrupt->action(interrupt->user_value);
}

void resetISR(void);
extern uint32_t ld_top_of_c_stack;
__attribute__ ((section(".isr_vectors")))
const void* s_flash_vector_table[NUMBER_OF_INTERRUPT_VECTORS] = {
    (void const*) &ld_top_of_c_stack,
	 resetISR,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,

    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
    handle_io_cpu_interrupt,
};

//
// we arrive here from Ti's ROM code so the standard cortex
// reset conditions do nt apply unfortunately
//
void __attribute__((naked)) resetISR(void)
{
    __asm__ __volatile__ (
        " movw r0, #:lower16:s_flash_vector_table\n"
        " movt r0, #:upper16:s_flash_vector_table\n"
        " ldr r0, [r0]\n"
        " mov sp, r0\n"
        " bl cc2652_core_reset"
    );
}

#endif /* IMPLEMENT_IO_CPU */
#endif
