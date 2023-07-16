// SPDX-License-Identifier: GPL-2.0
#include <asm/platform.h>
#include <asm/sw64_init.h>
#include <linux/reboot.h>

static void vt_mode_kill_arch(int mode)
{
	hcall(HCALL_SET_CLOCKEVENT, 0, 0, 0);

	switch (mode) {
	case LINUX_REBOOT_CMD_RESTART:
		hcall(HCALL_RESTART, 0, 0, 0);
		mb();
		break;
	case LINUX_REBOOT_CMD_HALT:
	case LINUX_REBOOT_CMD_POWER_OFF:
		hcall(HCALL_SHUTDOWN, 0, 0, 0);
		mb();
		break;
	default:
		break;
	}
}

extern void cpld_write(uint8_t slave_addr, uint8_t reg, uint8_t data);

static void xuelang_kill_arch(int mode)
{
	if (is_in_host()) {
		switch (mode) {
		case LINUX_REBOOT_CMD_RESTART:
			cpld_write(0x64, 0x00, 0xc3);
			mb();
			break;
		case LINUX_REBOOT_CMD_HALT:
		case LINUX_REBOOT_CMD_POWER_OFF:
			cpld_write(0x64, 0x00, 0xf0);
			mb();
			break;
		default:
			break;
		}
	} else {
		vt_mode_kill_arch(mode);
	}
}

static inline void __iomem *xuelang_ioportmap(unsigned long addr)
{
	unsigned long io_offset;

	if (addr < 0x100000) {
		io_offset = is_in_host() ? LPC_LEGACY_IO : PCI_VT_LEGACY_IO;
		addr = addr | io_offset;
	}

	return (void __iomem *)(addr | PAGE_OFFSET);
}

struct sw64_platform_ops xuelang_ops = {
	.kill_arch	= xuelang_kill_arch,
	.ioportmap	= xuelang_ioportmap,
	.ops_fixup	= sw64_init_noop,
};
