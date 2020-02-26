/*
 *
 * io cpu
 *
 */
#ifndef io_cpu_H_
#define io_cpu_H_
#include <io_core.h>

typedef struct PACK_STRUCTURE cc2652_main_rc_oscillator {
	IO_CPU_CLOCK_SOURCE_STRUCT_MEMBERS
	float64_t frequency;
} cc2652_main_rc_oscillator_t;

extern EVENT_DATA io_cpu_clock_implementation_t cc2652_main_rc_oscillator_implementation;

typedef struct cc2652_core_clock {
	IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS
} cc2652_core_clock_t;

extern EVENT_DATA io_cpu_clock_implementation_t cc2652_core_clock_implementation;


#define CC2652_IO_CPU_STRUCT_MEMBERS \
	IO_STRUCT_MEMBERS				\
	io_value_memory_t *vm;\
	io_byte_memory_t *bm;\
	uint32_t in_event_thread;\
	io_value_pipe_t *tasks;\
	/**/

typedef struct PACK_STRUCTURE io_cc2652_cpu {
	CC2652_IO_CPU_STRUCT_MEMBERS
} io_cc2652_cpu_t;

void	initialise_cpu_io (io_t*);


#ifdef IMPLEMENT_IO_CPU
//-----------------------------------------------------------------------------
//
// cc2652 Implementtaion
//
//-----------------------------------------------------------------------------
#include <cc2652rb.h>
typedef int32_t IRQn_Type;

#define SysTick_IRQn (INT_SYSTICK - 16)

#include <core_cm4.h>

#define NUMBER_OF_ARM_INTERRUPT_VECTORS	16L
#define NUMBER_OF_NRF_INTERRUPT_VECTORS	NUM_INTERRUPTS
#define NUMBER_OF_INTERRUPT_VECTORS	(NUMBER_OF_ARM_INTERRUPT_VECTORS + NUMBER_OF_NRF_INTERRUPT_VECTORS)

static io_interrupt_handler_t cpu_interrupts[NUMBER_OF_INTERRUPT_VECTORS];

#define ENABLE_INTERRUPTS	\
	do {	\
		__DMB();	\
		__enable_irq();	\
	} while (0)

#define DISABLE_INTERRUPTS	\
	do {	\
		__disable_irq();	\
		__DMB();	\
	} while (0)


static void
null_interrupt_handler (void *w) {
	while(1);
}

static float64_t
cc2652_main_rc_oscillator_get_frequency (io_cpu_clock_pointer_t this) {
	cc2652_main_rc_oscillator_t const *c = (cc2652_main_rc_oscillator_t const*) (
		io_cpu_clock_ro_pointer (this)
	);
	return c->frequency;
}

static bool
cc2652_main_rc_oscillator_start (io_cpu_clock_pointer_t this) {
	return true;
/*
	if (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == 0) {
		RCC_HSEConfig(RCC_HSE_ON);
		if (RCC_WaitForHSEStartUp () == SUCCESS) {
			return true;
		} else {
			return false;
		}
	} else {
		
	}
*/
}

EVENT_DATA io_cpu_clock_implementation_t cc2652_main_rc_oscillator_implementation = {
	.specialisation_of = &io_cpu_clock_implementation,
	.get_frequency = cc2652_main_rc_oscillator_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = cc2652_main_rc_oscillator_start,
	.stop = NULL,
};

bool
cc2652_clock_is_main_rc (io_cpu_clock_pointer_t clock) {
	return io_cpu_clock_has_implementation (clock,&cc2652_main_rc_oscillator_implementation);
}

static float64_t
cc2652_core_clock_get_frequency (io_cpu_clock_pointer_t clock) {
	cc2652_core_clock_t const *this = (cc2652_core_clock_t const*) (
		io_cpu_clock_ro_pointer (clock)
	);
	return io_cpu_clock_get_frequency (this->input);
}

static bool
cc2652_core_clock_start (io_cpu_clock_pointer_t clock) {
	if (io_cpu_dependant_clock_start_input (clock)) {

		return true;
	} else {
		return false;
	}
}

EVENT_DATA io_cpu_clock_implementation_t cc2652_core_clock_implementation = {
	.specialisation_of = &io_cpu_clock_implementation,
	.get_frequency = cc2652_core_clock_get_frequency,
	.link_input_to_output = NULL,
	.link_output_to_input = NULL,
	.start = cc2652_core_clock_start,
	.stop = NULL,
};

//
// pins
//

//
// io methods
//
static io_byte_memory_t*
cc2652_io_get_byte_memory (io_t *io) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;
	return this->bm;
}

static io_value_memory_t*
cc2652_io_get_stvm (io_t *io) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;
	return this->vm;
}

static void
cc2652_do_gc (io_t *io,int32_t count) {
	io_value_memory_do_gc (io_get_short_term_value_memory (io),count);
}

static uint32_t
cc2652_get_random_u32 (io_t *io) {
	uint32_t r = 0;
	return r;
}

static void
cc2652_signal_task_pending (io_t *io) {
	// no action required
}

static void
cc2652_signal_event_pending (io_t *io) {
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

static bool
cc2652_enqueue_task (io_t *io,vref_t r_task) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;
	return io_value_pipe_put_value (this->tasks,r_task);
}

static bool
cc2652_enter_critical_section (io_t *env) {
	uint32_t interrupts_are_enabled = !(__get_PRIMASK() & 0x1);
	DISABLE_INTERRUPTS;
	return interrupts_are_enabled;
}

void
cc2652_exit_critical_section (io_t *env,bool were_enabled) {
	if (were_enabled) {
		ENABLE_INTERRUPTS;
	}
}

static bool
cc2652_is_in_event_thread (io_t *io) {
	return ((io_cc2652_cpu_t*) io)->in_event_thread;
}

static void
cc2652_wait_for_event (io_t *io) {
	__WFI();
}

static void
cc2652_for_all_events (io_t *io) {
	io_event_t *event;
	io_alarm_t *alarm;
	do {
		ENTER_CRITICAL_SECTION(io);
		event = io->events;
		alarm = io->alarms;
		EXIT_CRITICAL_SECTION(io);
	} while (
			event != &s_null_io_event
		&&	alarm != &s_null_io_alarm
	);
}

static void	
cc2652_register_interrupt_handler (
	io_t *io,int32_t number,io_interrupt_action_t handler,void *user_value
) {
	io_interrupt_handler_t *i = cpu_interrupts + number;
	i->action = handler;
	i->user_value = user_value;
}

static bool	
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
cc2652_panic (io_t *io,int code) {
	DISABLE_INTERRUPTS;
	while (1);
}

static void
cc2652_log (io_t *io,char const *fmt,va_list va) {
	// ...
}

void
add_io_implementation_cpu_methods (io_implementation_t *io_i) {
	add_io_implementation_core_methods (io_i);

	io_i->get_byte_memory = cc2652_io_get_byte_memory;
	io_i->get_short_term_value_memory = cc2652_io_get_stvm;
	io_i->do_gc = cc2652_do_gc;
	io_i->get_random_u32 = cc2652_get_random_u32;
	io_i->signal_task_pending = cc2652_signal_task_pending;
	io_i->enqueue_task = cc2652_enqueue_task;
//	io_i->do_next_task = cc2652_do_next_task;
	io_i->signal_event_pending = cc2652_signal_event_pending;
	io_i->enter_critical_section = cc2652_enter_critical_section;
	io_i->exit_critical_section = cc2652_exit_critical_section;
	io_i->in_event_thread = cc2652_is_in_event_thread;
	io_i->wait_for_event = cc2652_wait_for_event;
//	io_i->get_time = cc2652_get_time,
//	io_i->enqueue_alarm = nrf_time_clock_enqueue_alarm;
//	io_i->dequeue_alarm = nrf_time_clock_dequeue_alarm;
	io_i->register_interrupt_handler = cc2652_register_interrupt_handler;
	io_i->unregister_interrupt_handler = cc2652_unregister_interrupt_handler;
	io_i->wait_for_all_events = cc2652_for_all_events;
//	io_i->set_io_pin_output = cc2652_set_io_pin_to_output,
//	io_i->set_io_pin_input = cc2652_set_io_pin_to_input,
//	io_i->set_io_pin_interrupt = cc2652_set_io_pin_interrupt,
//	io_i->set_io_pin_alternate = io_pin_nop,
//	io_i->read_from_io_pin = cc2652_read_io_input_pin,
//	io_i->write_to_io_pin = cc2652_write_to_io_pin,
//	io_i->toggle_io_pin = cc2652_toggle_io_pin,
//	io_i->valid_pin = cc2652_io_pin_is_valid,
	io_i->panic = cc2652_panic;
	io_i->log = cc2652_log;
}

static void
event_thread (void *io) {
	io_cc2652_cpu_t *this = io;
	this->in_event_thread = true;
	while (next_io_event (io));
	this->in_event_thread = false;
}

static void
hard_fault (void *io) {
	DISABLE_INTERRUPTS;
	while(1);
}

void
initialise_cpu_io (io_t *io) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;

	this->in_event_thread = false;

	io_cpu_clock_start (io_get_core_clock(io));

	register_io_interrupt_handler (io,INT_PENDSV,event_thread,io);
	register_io_interrupt_handler (io,INT_HARD_FAULT,hard_fault,io);
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
	while (dest < end) {
		*dest++ = 0xdeadc0de;
	}
	
	initialise_ram_interrupt_vectors ();
}

static void
tune_cpu (void) {
	// see GPNVM, TCM is disabled?
	
	#if (__FPU_USED == 1)
	/* enable FPU if available and used */
	SCB->CPACR |= ((3UL << 10*2) |             /* set CP10 Full Access               */
					  (3UL << 11*2)  );           /* set CP11 Full Access               */
	#endif
}

int main(void);

void
cc2652_core_reset (void) {
	initialise_c_runtime ();
	tune_cpu ();
	main ();
	while (1);
}

static void
handle_io_cpu_interrupt (void) {
	io_interrupt_handler_t const *interrupt = &cpu_interrupts[
		SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk
	];
	interrupt->action(interrupt->user_value);
}

extern uint32_t ld_top_of_c_stack;
__attribute__ ((section(".isr_vector")))
const void* s_flash_vector_table[NUMBER_OF_INTERRUPT_VECTORS] = {
	&ld_top_of_c_stack,
	cc2652_core_reset,
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


#endif /* IMPLEMENT_IO_CPU */
#ifdef IMPLEMENT_VERIFY_IO_CPU

#define IO_CPU_UNIT_TESTS \
	/**/
#else
#define IO_CPU_UNIT_TESTS
#endif /* IMPLEMENT_VERIFY_IO_CPU */
#endif
