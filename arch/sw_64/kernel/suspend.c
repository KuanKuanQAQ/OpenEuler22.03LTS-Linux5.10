// SPDX-License-Identifier: GPL-2.0
#include <linux/suspend.h>

#include <asm/suspend.h>
#include <asm/sw64_init.h>

struct processor_state suspend_state;

static int native_suspend_state_valid(suspend_state_t pm_state)
{
	switch (pm_state) {
	case PM_SUSPEND_ON:
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		return 1;
	default:
		return 0;
	}
}

void disable_local_timer(void)
{
	wrtimer(0);
}

/*
 * Boot Core will enter suspend stat here.
 */
void sw64_suspend_enter(void)
{
	/* boot processor will go to deep sleep mode from here
	 * After wake up  boot processor, pc will go here
	 */

	disable_local_timer();

#ifdef CONFIG_SW64_SUSPEND_DEEPSLEEP_BOOTCORE
	sw64_suspend_deep_sleep(&suspend_state);
#else
	mtinten();
	asm("halt");
#endif

	disable_local_timer();
}

static int native_suspend_enter(suspend_state_t state)
{
	/* processor specific suspend */
	sw64_suspend_enter();
	return 0;
}

static const struct platform_suspend_ops native_suspend_ops = {
	.valid = native_suspend_state_valid,
	.enter = native_suspend_enter,
};
static int __init sw64_pm_init(void)
{
	suspend_set_ops(&native_suspend_ops);
	return 0;
}
arch_initcall(sw64_pm_init);
