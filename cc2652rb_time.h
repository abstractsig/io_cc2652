/*
 *
 *
 *
 */
#ifndef cc2652rb_time_H_
#define cc2652rb_time_H_
#ifdef IMPLEMENT_IO_CPU
//-----------------------------------------------------------------------------
//
// implementation
//
//-----------------------------------------------------------------------------

static io_time_t
cc2652_time_clock_get_time (io_t *io,cc2652_time_clock_t *rtc) {
	int64_t sec,frac;

	{
		bool h = enter_io_critical_section (io);
		sec = AONRTCSecGet ();
		frac = AONRTCFractionGet ();

		exit_io_critical_section (io,h);
	}
	//
	// q32.32
	// fraction part is 1000000000/(1 << 32) = 0.2328 ns per unit
	// multiply by (1 << 16) to use integer multiplication
	//
	return (io_time_t) {
		.nanoseconds = (sec * 1000000000LL) + ((frac * 15259LL) >> 16LL),
	};
};

io_time_t
cc2652_get_time (io_t *io) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;
	return cc2652_time_clock_get_time (io,&this->rtc);
}

//
// expects next alarm time to be greater than current time
//
static bool
set_time_clock_alarm_time (io_cc2652_cpu_t *this) {
	if (this->alarms != &s_null_io_alarm) {
		uint32_t sec = this->alarms->when.ns/1000000000LL;
		uint32_t frac = ((this->alarms->when.ns - (sec * 1000000000LL)) * 281474LL) >> 16;

		AONRTCCompareValueSet (
			AON_RTC_CH0,
			((sec & 0xffff) << 16L) + ((frac & 0xffff0000) >> 16)
		);

		return true;
	} else {
		return false;
	}
}

static bool
process_next_alarm (io_cc2652_cpu_t *this) {
	if (this->alarms != &s_null_io_alarm) {
		volatile io_time_t t = cc2652_time_clock_get_time ((io_t*) this,&this->rtc);

		if (t.ns >= this->alarms->when.ns) {
			io_alarm_t *alarm = this->alarms;
			this->alarms = this->alarms->next_alarm;
			alarm->next_alarm = NULL;
			alarm->at->event_handler (alarm->at);
			//
			// add tollerance check ...
			//
			return true;
		} else {
			//while(1) {}
		}
	}
	return false;
}

static void
process_alarm_queue (io_event_t *ev) {
	io_cc2652_cpu_t *this = ev->user_value;
	uint32_t count = 0;

	while (process_next_alarm (this)) {
		count++;
	}

	if (count) {
		set_time_clock_alarm_time (this);
	}
}

void
cc2652_time_clock_enqueue_alarm (io_t *io,io_alarm_t *alarm) {
	io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;

	ENTER_CRITICAL_SECTION(io);

	if (alarm->when.ns < this->alarms->when.ns) {
		alarm->next_alarm = this->alarms;
		this->alarms = alarm;
		if (!set_time_clock_alarm_time (this)) {
			io_panic (io,IO_PANIC_TIME_CLOCK_ERROR);
		}
	} else {
		io_alarm_t *pre = this->alarms;
		while (alarm->when.ns > pre->when.ns) {
			if (pre->next_alarm == &s_null_io_alarm) {
				break;
			}
			pre = pre->next_alarm;
		}
		alarm->next_alarm = pre->next_alarm;
		pre->next_alarm = alarm;
	}

	EXIT_CRITICAL_SECTION(io);
}

void
cc2652_time_clock_dequeue_alarm (io_t *io,io_alarm_t *alarm) {
	if (alarm->next_alarm != NULL) {
		ENTER_CRITICAL_SECTION (io);
		if (alarm == io->alarms) {
			io_cc2652_cpu_t *this = (io_cc2652_cpu_t*) io;
			io->alarms = io->alarms->next_alarm;
			set_time_clock_alarm_time (this);
		} else {
			io_alarm_t *pre = io->alarms;
			while (pre) {
				if (alarm == pre->next_alarm) {
					pre->next_alarm = alarm->next_alarm;
					break;
				}
				pre = pre->next_alarm;
			}
		}
		alarm->next_alarm = NULL;
		EXIT_CRITICAL_SECTION (io);
	}
}

void
cc2652_time_clock_interrupt (void *user_value) {
	io_cc2652_cpu_t *this = user_value;
	AONRTCEventClear(AON_RTC_CH0);
	io_enqueue_event (user_value,&this->rtc.alarm);
}


void
start_time_clock (io_cc2652_cpu_t *this) {

	this->rtc.io = (io_t*) this;

	if (io_cpu_clock_start ((io_t*) this,this->rtc.clock)) {

		AONRTCReset();

		AONRTCDelayConfig (AON_RTC_CONFIG_DELAY_NODELAY);

		// compare channel 0, will also wakeup the cpu core
		AONEventMcuWakeUpSet(AON_EVENT_MCU_WU0, AON_EVENT_RTC0);

		AONRTCChannelEnable(AON_RTC_CH0);
		AONRTCCombinedEventConfig(AON_RTC_CH0);

		AONRTCEnable ();

		SysCtrlAonSync ();

		initialise_io_event (&this->rtc.alarm,process_alarm_queue,this);

		register_io_interrupt_handler (
			(io_t*) this,INT_AON_RTC_COMB,cc2652_time_clock_interrupt,this
		);

		{
			int32_t irqn = CMSIS_IRQn(INT_AON_RTC_COMB);
			NVIC_SetPriority (irqn,NORMAL_INTERRUPT_PRIORITY);
			NVIC_EnableIRQ (irqn);
		}

	}
}

#endif /* IMPLEMENT_IO_CPU */
#endif
