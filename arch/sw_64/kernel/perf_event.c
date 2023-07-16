// SPDX-License-Identifier: GPL-2.0
/*
 * Performance events support for SW64 platforms.
 *
 * This code is based upon riscv and sparc perf event code.
 */

#include <linux/perf_event.h>

/* For tracking PMCs and the hw events they monitor on each CPU. */
struct cpu_hw_events {
	/* Number of events currently scheduled onto this cpu.
	 * This tells how many entries in the arrays below
	 * are valid.
	 */
	int			n_events;
	/* Track counter usage of each counter */
#define PMC_IN_USE  1
#define PMC_NOT_USE 0
	int			pmcs[MAX_HWEVENTS];
	/* Array of events current scheduled on this cpu. */
	struct perf_event	*event[MAX_HWEVENTS];
};

DEFINE_PER_CPU(struct cpu_hw_events, cpu_hw_events);

static void sw64_pmu_start(struct perf_event *event, int flags);
static void sw64_pmu_stop(struct perf_event *event, int flags);

struct sw64_perf_event {
	/* pmu index */
	int counter;
	/* events selector */
	int event;
};

/*
 * A structure to hold the description of the PMCs available on a particular
 * type of SW64 CPU.
 */
struct sw64_pmu_t {
	/* generic hw/cache events table */
	const struct sw64_perf_event *hw_events;
	const struct sw64_perf_event (*cache_events)[PERF_COUNT_HW_CACHE_MAX]
		[PERF_COUNT_HW_CACHE_OP_MAX]
		[PERF_COUNT_HW_CACHE_RESULT_MAX];

	/* method used to map hw/cache events */
	const struct sw64_perf_event *(*map_hw_event)(u64 config);
	const struct sw64_perf_event *(*map_cache_event)(u64 config);

	/* The number of entries in the hw_event_map */
	int  max_events;

	/* The number of counters on this pmu */
	int  num_pmcs;

	/*
	 * All PMC counters reside in the IBOX register PCTR.  This is the
	 * LSB of the counter.
	 */
	int  pmc_count_shift[MAX_HWEVENTS];

	/*
	 * The mask that isolates the PMC bits when the LSB of the counter
	 * is shifted to bit 0.
	 */
	unsigned long pmc_count_mask;

	/* The maximum period the PMC can count. */
	unsigned long pmc_max_period;

	/*
	 * The maximum value that may be written to the counter due to
	 * hardware restrictions is pmc_max_period - pmc_left.
	 */
	long pmc_left;

	/* Subroutine for checking validity of a raw event for this PMU. */
	bool (*raw_event_valid)(u64 config);
};

/*
 * The SW64 PMU description currently in operation.  This is set during
 * the boot process to the specific CPU of the machine.
 */
static const struct sw64_pmu_t *sw64_pmu;

/*
 * SW64 PMC event types
 *
 * There is no one-to-one mapping of the possible hw event types to the
 * actual codes that are used to program the PMCs hence we introduce our
 * own hw event type identifiers.
 */
#define SW64_OP_UNSUP {-1, -1}

/* Mapping of the hw event types to the perf tool interface */
static const struct sw64_perf_event core3_hw_event_map[] = {
	[PERF_COUNT_HW_CPU_CYCLES]		= {PERFMON_PC0, PC0_CPU_CYCLES},
	[PERF_COUNT_HW_INSTRUCTIONS]		= {PERFMON_PC0, PC0_INSTRUCTIONS},
	[PERF_COUNT_HW_CACHE_REFERENCES]	= {PERFMON_PC0,	PC0_SCACHE_REFERENCES},
	[PERF_COUNT_HW_CACHE_MISSES]		= {PERFMON_PC1, PC1_SCACHE_MISSES},
	[PERF_COUNT_HW_BRANCH_INSTRUCTIONS]	= {PERFMON_PC0, PC0_BRANCH_INSTRUCTIONS},
	[PERF_COUNT_HW_BRANCH_MISSES]		= {PERFMON_PC1, PC1_BRANCH_MISSES},
};

/* Mapping of the hw cache event types to the perf tool interface */
#define C(x) PERF_COUNT_HW_CACHE_##x
static const struct sw64_perf_event core3_cache_event_map
				[PERF_COUNT_HW_CACHE_MAX]
				[PERF_COUNT_HW_CACHE_OP_MAX]
				[PERF_COUNT_HW_CACHE_RESULT_MAX] = {
	[C(L1D)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= {PERFMON_PC0, PC0_DCACHE_READ},
			[C(RESULT_MISS)]	= {PERFMON_PC1, PC1_DCACHE_MISSES}
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},
	[C(L1I)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= {PERFMON_PC0, PC0_ICACHE_READ},
			[C(RESULT_MISS)]	= {PERFMON_PC1, PC1_ICACHE_READ_MISSES},
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},
	[C(LL)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},
	[C(DTLB)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= {PERFMON_PC0, PC0_DTB_READ},
			[C(RESULT_MISS)]	= {PERFMON_PC1, PC1_DTB_SINGLE_MISSES},
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},
	[C(ITLB)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= {PERFMON_PC0, PC0_ITB_READ},
			[C(RESULT_MISS)]	= {PERFMON_PC1, PC1_ITB_MISSES},
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},
	[C(BPU)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},
	[C(NODE)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= SW64_OP_UNSUP,
			[C(RESULT_MISS)]	= SW64_OP_UNSUP,
		},
	},

};

static const struct sw64_perf_event *core3_map_hw_event(u64 config)
{
	return &sw64_pmu->hw_events[config];
}

static const struct sw64_perf_event *core3_map_cache_event(u64 config)
{
	unsigned int cache_type, cache_op, cache_result;
	const struct sw64_perf_event *perf_event;

	cache_type = (config >> 0) & 0xff;
	if (cache_type >= PERF_COUNT_HW_CACHE_MAX)
		return ERR_PTR(-EINVAL);

	cache_op = (config >> 8) & 0xff;
	if (cache_op >= PERF_COUNT_HW_CACHE_OP_MAX)
		return ERR_PTR(-EINVAL);

	cache_result = (config >> 16) & 0xff;
	if (cache_result >= PERF_COUNT_HW_CACHE_RESULT_MAX)
		return ERR_PTR(-EINVAL);

	perf_event = &((*sw64_pmu->cache_events)[cache_type][cache_op][cache_result]);
	if (perf_event->counter == -1) /* SW64_OP_UNSUP */
		return ERR_PTR(-ENOENT);

	return perf_event;
}

/*
 * r0xx for counter0, r1yy for counter1.
 * According to the datasheet, 00 <= xx <= 0F, 00 <= yy <= 37
 */
static bool core3_raw_event_valid(u64 config)
{
	if ((config >= (PC0_RAW_BASE + PC0_MIN) && config <= (PC0_RAW_BASE + PC0_MAX)) ||
		(config >= (PC1_RAW_BASE + PC1_MIN) && config <= (PC1_RAW_BASE + PC1_MAX))) {
		return true;
	}

	pr_info("sw64 pmu: invalid raw event config %#llx\n", config);
	return false;
}

static const struct sw64_pmu_t core3_pmu = {
	.max_events = ARRAY_SIZE(core3_hw_event_map),
	.hw_events = core3_hw_event_map,
	.map_hw_event = core3_map_hw_event,
	.cache_events = &core3_cache_event_map,
	.map_cache_event = core3_map_cache_event,
	.num_pmcs = MAX_HWEVENTS,
	.pmc_count_mask = PMC_COUNT_MASK,
	.pmc_max_period = PMC_COUNT_MASK,
	.pmc_left = 4,
	.raw_event_valid = core3_raw_event_valid,
};

/*
 * Low-level functions: reading/writing counters
 */
static void sw64_write_pmc(int idx, unsigned long val)
{
	if (idx == PERFMON_PC0)
		wrperfmon(PERFMON_CMD_WRITE_PC0, val);
	else
		wrperfmon(PERFMON_CMD_WRITE_PC1, val);
}

static unsigned long sw64_read_pmc(int idx)
{
	unsigned long val;

	if (idx == PERFMON_PC0)
		val = wrperfmon(PERFMON_CMD_READ, PERFMON_READ_PC0);
	else
		val = wrperfmon(PERFMON_CMD_READ, PERFMON_READ_PC1);
	return val;
}

/* Set a new period to sample over */
static int sw64_perf_event_set_period(struct perf_event *event,
				struct hw_perf_event *hwc, int idx)
{
	long left = local64_read(&hwc->period_left);
	long period = hwc->sample_period;
	int ret = 0;

	if (unlikely(left <= -period)) {
		left = period;
		local64_set(&hwc->period_left, left);
		hwc->last_period = period;
		ret = 1;
	}

	if (unlikely(left <= 0)) {
		left += period;
		local64_set(&hwc->period_left, left);
		hwc->last_period = period;
		ret = 1;
	}

	if (left > (long)sw64_pmu->pmc_max_period)
		left = sw64_pmu->pmc_max_period;

	local64_set(&hwc->prev_count, (unsigned long)(-left));
	sw64_write_pmc(idx,  (unsigned long)(sw64_pmu->pmc_max_period - left));

	perf_event_update_userpage(event);

	return ret;
}

/*
 * Calculates the count (the 'delta') since the last time the PMC was read.
 *
 * As the PMCs' full period can easily be exceeded within the perf system
 * sampling period we cannot use any high order bits as a guard bit in the
 * PMCs to detect overflow as is done by other architectures.  The code here
 * calculates the delta on the basis that there is no overflow when ovf is
 * zero.  The value passed via ovf by the interrupt handler corrects for
 * overflow.
 *
 * This can be racey on rare occasions -- a call to this routine can occur
 * with an overflowed counter just before the PMI service routine is called.
 * The check for delta negative hopefully always rectifies this situation.
 */
static unsigned long sw64_perf_event_update(struct perf_event *event,
					struct hw_perf_event *hwc, int idx, long ovf)
{
	long prev_raw_count, new_raw_count;
	long delta;

again:
	prev_raw_count = local64_read(&hwc->prev_count);
	new_raw_count = sw64_read_pmc(idx);

	if (local64_cmpxchg(&hwc->prev_count, prev_raw_count,
			     new_raw_count) != prev_raw_count)
		goto again;

	delta = (new_raw_count - (prev_raw_count & sw64_pmu->pmc_count_mask)) + ovf;

	/* It is possible on very rare occasions that the PMC has overflowed
	 * but the interrupt is yet to come.  Detect and fix this situation.
	 */
	if (unlikely(delta < 0))
		delta += sw64_pmu->pmc_max_period + 1;

	local64_add(delta, &event->count);
	local64_sub(delta, &hwc->period_left);

	return new_raw_count;
}

/*
 * State transition functions:
 *
 * add()/del() & start()/stop()
 *
 */

/*
 * pmu->add: add the event to PMU.
 */
static int sw64_pmu_add(struct perf_event *event, int flags)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);
	struct hw_perf_event *hwc = &event->hw;
	int err = 0;
	unsigned long irq_flags;

	perf_pmu_disable(event->pmu);
	local_irq_save(irq_flags);

	if (cpuc->pmcs[hwc->idx] == PMC_IN_USE) {
		err = -ENOSPC;
		goto out;
	}

	cpuc->pmcs[hwc->idx] = PMC_IN_USE;
	cpuc->event[hwc->idx] = event;


	cpuc->n_events++;

	hwc->state = PERF_HES_STOPPED | PERF_HES_UPTODATE;
	if (flags & PERF_EF_START)
		sw64_pmu_start(event, PERF_EF_RELOAD);

	/* Propagate our changes to the userspace mapping. */
	perf_event_update_userpage(event);

out:
	local_irq_restore(irq_flags);
	perf_pmu_enable(event->pmu);

	return err;
}

/*
 * pmu->del: delete the event from PMU.
 */
static void sw64_pmu_del(struct perf_event *event, int flags)
{
	struct cpu_hw_events *cpuc = this_cpu_ptr(&cpu_hw_events);
	struct hw_perf_event *hwc = &event->hw;
	unsigned long irq_flags;

	perf_pmu_disable(event->pmu);
	local_irq_save(irq_flags);

	if (cpuc->event[hwc->idx] != event)
		goto out;

	cpuc->event[hwc->idx] = NULL;
	cpuc->pmcs[hwc->idx] = PMC_NOT_USE;
	cpuc->n_events--;

	sw64_pmu_stop(event, PERF_EF_UPDATE);

	/* Absorb the final count and turn off the event. */
	perf_event_update_userpage(event);

out:
	local_irq_restore(irq_flags);
	perf_pmu_enable(event->pmu);
}

/*
 * pmu->start: start the event.
 */
static void sw64_pmu_start(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	if (WARN_ON_ONCE(!(hwc->state & PERF_HES_STOPPED)))
		return;

	if (flags & PERF_EF_RELOAD) {
		WARN_ON_ONCE(!(hwc->state & PERF_HES_UPTODATE));
		sw64_perf_event_set_period(event, hwc, hwc->idx);
	}

	hwc->state = 0;

	/* counting in all modes, for both counters */
	wrperfmon(PERFMON_CMD_PM, 4);
	if (hwc->idx == PERFMON_PC0) {
		wrperfmon(PERFMON_CMD_EVENT_PC0, hwc->event_base);
		wrperfmon(PERFMON_CMD_ENABLE, PERFMON_ENABLE_ARGS_PC0);
	} else {
		wrperfmon(PERFMON_CMD_EVENT_PC1, hwc->event_base);
		wrperfmon(PERFMON_CMD_ENABLE, PERFMON_ENABLE_ARGS_PC1);
	}
}

/*
 * pmu->stop: stop the counter
 */
static void sw64_pmu_stop(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	if (!(hwc->state & PERF_HES_STOPPED)) {
		hwc->state |= PERF_HES_STOPPED;
		barrier();
	}

	if ((flags & PERF_EF_UPDATE) && !(hwc->state & PERF_HES_UPTODATE)) {
		sw64_perf_event_update(event, hwc, hwc->idx, 0);
		hwc->state |= PERF_HES_UPTODATE;
	}

	if (hwc->idx == 0)
		wrperfmon(PERFMON_CMD_DISABLE, PERFMON_DISABLE_ARGS_PC0);
	else
		wrperfmon(PERFMON_CMD_DISABLE, PERFMON_DISABLE_ARGS_PC1);

}

/*
 * pmu->read: read and update the counter
 */
static void sw64_pmu_read(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;

	sw64_perf_event_update(event, hwc, hwc->idx, 0);
}

static bool supported_cpu(void)
{
	return true;
}

static void hw_perf_event_destroy(struct perf_event *event)
{
	/* Nothing to be done! */
}

static int __hw_perf_event_init(struct perf_event *event)
{
	struct perf_event_attr *attr = &event->attr;
	struct hw_perf_event *hwc = &event->hw;
	const struct sw64_perf_event *event_type;


	/* SW64 do not have per-counter usr/os/guest/host bits */
	if (event->attr.exclude_user || event->attr.exclude_kernel ||
			event->attr.exclude_hv || event->attr.exclude_idle ||
			event->attr.exclude_host || event->attr.exclude_guest)
		return -EINVAL;

	/*
	 * SW64 does not support precise ip feature, and system hang when
	 * detecting precise_ip by perf_event_attr__set_max_precise_ip
	 * in userspace
	 */
	if (attr->precise_ip != 0)
		return -EOPNOTSUPP;

	/* SW64 has fixed counter for given event type */
	if (attr->type == PERF_TYPE_HARDWARE) {
		if (attr->config >= sw64_pmu->max_events)
			return -EINVAL;
		event_type = sw64_pmu->map_hw_event(attr->config);
		hwc->idx = event_type->counter;
		hwc->event_base = event_type->event;
	} else if (attr->type == PERF_TYPE_HW_CACHE) {
		event_type = sw64_pmu->map_cache_event(attr->config);
		if (IS_ERR(event_type))	/* */
			return PTR_ERR(event_type);
		hwc->idx = event_type->counter;
		hwc->event_base = event_type->event;
	} else { /* PERF_TYPE_RAW */
		if (!sw64_pmu->raw_event_valid(attr->config))
			return -EINVAL;
		hwc->idx = attr->config >> 8;	/* counter selector */
		hwc->event_base = attr->config & 0xff;	/* event selector */
	}

	hwc->config = attr->config;

	if (!is_sampling_event(event))
		pr_debug("not sampling event\n");

	event->destroy = hw_perf_event_destroy;

	if (!hwc->sample_period) {
		hwc->sample_period = sw64_pmu->pmc_max_period;
		hwc->last_period = hwc->sample_period;
		local64_set(&hwc->period_left, hwc->sample_period);
	}

	return 0;
}

/*
 * Main entry point to initialise a HW performance event.
 */
static int sw64_pmu_event_init(struct perf_event *event)
{
	int err;

	/* does not support taken branch sampling */
	if (has_branch_stack(event))
		return -EOPNOTSUPP;

	switch (event->attr.type) {
	case PERF_TYPE_RAW:
	case PERF_TYPE_HARDWARE:
	case PERF_TYPE_HW_CACHE:
		break;
	default:
		return -ENOENT;
	}

	if (!sw64_pmu)
		return -ENODEV;

	/* Do the real initialisation work. */
	err = __hw_perf_event_init(event);

	return err;
}

static struct pmu pmu = {
	.name		= "core3-base",
	.capabilities   = PERF_PMU_CAP_NO_NMI,
	.event_init	= sw64_pmu_event_init,
	.add		= sw64_pmu_add,
	.del		= sw64_pmu_del,
	.start		= sw64_pmu_start,
	.stop		= sw64_pmu_stop,
	.read		= sw64_pmu_read,
};

void perf_event_print_debug(void)
{
	unsigned long flags;
	unsigned long pcr0, pcr1;
	int cpu;

	if (!supported_cpu())
		return;

	local_irq_save(flags);

	cpu = smp_processor_id();

	pcr0 = wrperfmon(PERFMON_CMD_READ, PERFMON_READ_PC0);
	pcr1 = wrperfmon(PERFMON_CMD_READ, PERFMON_READ_PC1);

	pr_info("CPU#%d: PCTR0[%lx] PCTR1[%lx]\n", cpu, pcr0, pcr1);

	local_irq_restore(flags);
}

static void sw64_perf_event_irq_handler(unsigned long perfmon_num,
					struct pt_regs *regs)
{
	struct cpu_hw_events *cpuc;
	struct perf_sample_data data;
	struct perf_event *event;
	struct hw_perf_event *hwc;
	int idx;

	__this_cpu_inc(irq_pmi_count);
	cpuc = this_cpu_ptr(&cpu_hw_events);

	idx = perfmon_num;

	event = cpuc->event[idx];

	if (unlikely(!event)) {
		/* This should never occur! */
		irq_err_count++;
		pr_warn("PMI: No event at index %d!\n", idx);
		wrperfmon(PERFMON_CMD_ENABLE, idx == 0 ? PERFMON_DISABLE_ARGS_PC0 : PERFMON_DISABLE_ARGS_PC1);
		return;
	}

	hwc = &event->hw;
	sw64_perf_event_update(event, hwc, idx, sw64_pmu->pmc_max_period + 1);
	perf_sample_data_init(&data, 0, hwc->last_period);

	if (sw64_perf_event_set_period(event, hwc, idx)) {
		if (perf_event_overflow(event, &data, regs)) {
			/* Interrupts coming too quickly; "throttle" the
			 * counter, i.e., disable it for a little while.
			 */
			sw64_pmu_stop(event, 0);
		}
	}
}

bool valid_utext_addr(unsigned long addr)
{
	return addr >= current->mm->start_code && addr <= current->mm->end_code;
}

bool valid_dy_addr(unsigned long addr)
{
	bool ret = false;
	struct vm_area_struct *vma;
	struct mm_struct *mm = current->mm;

	if (addr > TASK_SIZE || addr < TASK_UNMAPPED_BASE)
		return ret;
	vma = find_vma(mm, addr);
	if (vma && vma->vm_start <= addr && (vma->vm_flags & VM_EXEC))
		ret = true;
	return ret;
}

void perf_callchain_user(struct perf_callchain_entry_ctx *entry,
		struct pt_regs *regs)
{
	unsigned long usp = current_user_stack_pointer();
	unsigned long user_addr;
	int err;

	perf_callchain_store(entry, regs->pc);

	while (entry->nr < entry->max_stack && usp < current->mm->start_stack) {
		if (!access_ok(usp, 8))
			break;
		pagefault_disable();
		err = __get_user(user_addr, (unsigned long *)usp);
		pagefault_enable();
		if (err)
			break;
		if (valid_utext_addr(user_addr) || valid_dy_addr(user_addr))
			perf_callchain_store(entry, user_addr);
		usp = usp + 8;
	}
}

void perf_callchain_kernel(struct perf_callchain_entry_ctx *entry,
			   struct pt_regs *regs)
{
	unsigned long *sp = (unsigned long *)current_thread_info()->pcb.ksp;
	unsigned long addr;

	perf_callchain_store(entry, regs->pc);

	while (!kstack_end(sp) && entry->nr < entry->max_stack) {
		addr = *sp++;
		if (__kernel_text_address(addr))
			perf_callchain_store(entry, addr);
	}
}

/*
 * Init call to initialise performance events at kernel startup.
 */
int __init init_hw_perf_events(void)
{
	if (!supported_cpu()) {
		pr_info("Performance events: Unsupported CPU type!\n");
		return 0;
	}

	pr_info("Performance events: Supported CPU type!\n");

	/* Override performance counter IRQ vector */

	perf_irq = sw64_perf_event_irq_handler;

	/* And set up PMU specification */
	sw64_pmu = &core3_pmu;

	perf_pmu_register(&pmu, "cpu", PERF_TYPE_RAW);

	return 0;
}
early_initcall(init_hw_perf_events);
