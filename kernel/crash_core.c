// SPDX-License-Identifier: GPL-2.0-only
/*
 * crash.c - kernel crash support code.
 * Copyright (C) 2002-2004 Eric Biederman  <ebiederm@xmission.com>
 */

#include <linux/crash_core.h>
#include <linux/utsname.h>
#include <linux/vmalloc.h>
#include <linux/memblock.h>
#include <linux/swiotlb.h>

#ifdef CONFIG_KEXEC_CORE
#include <asm/kexec.h>
#endif

#include <asm/page.h>
#include <asm/sections.h>

#include <crypto/sha1.h>

/* vmcoreinfo stuff */
unsigned char *vmcoreinfo_data;
size_t vmcoreinfo_size;
u32 *vmcoreinfo_note;

/* trusted vmcoreinfo, e.g. we can make a copy in the crash memory */
static unsigned char *vmcoreinfo_data_safecopy;

/* Location of the reserved area for the crash kernel */
struct resource crashk_res = {
	.name  = "Crash kernel",
	.start = 0,
	.end   = 0,
	.flags = IORESOURCE_BUSY | IORESOURCE_SYSTEM_RAM,
	.desc  = IORES_DESC_CRASH_KERNEL
};
struct resource crashk_low_res = {
	.name  = "Crash kernel",
	.start = 0,
	.end   = 0,
	.flags = IORESOURCE_BUSY | IORESOURCE_SYSTEM_RAM,
	.desc  = IORES_DESC_CRASH_KERNEL
};

/*
 * parsing the "crashkernel" commandline
 *
 * this code is intended to be called from architecture specific code
 */


/*
 * This function parses command lines in the format
 *
 *   crashkernel=ramsize-range:size[,...][@offset]
 *
 * The function returns 0 on success and -EINVAL on failure.
 */
static int __init parse_crashkernel_mem(char *cmdline,
					unsigned long long system_ram,
					unsigned long long *crash_size,
					unsigned long long *crash_base)
{
	char *cur = cmdline, *tmp;

	/* for each entry of the comma-separated list */
	do {
		unsigned long long start, end = ULLONG_MAX, size;

		/* get the start of the range */
		start = memparse(cur, &tmp);
		if (cur == tmp) {
			pr_warn("crashkernel: Memory value expected\n");
			return -EINVAL;
		}
		cur = tmp;
		if (*cur != '-') {
			pr_warn("crashkernel: '-' expected\n");
			return -EINVAL;
		}
		cur++;

		/* if no ':' is here, than we read the end */
		if (*cur != ':') {
			end = memparse(cur, &tmp);
			if (cur == tmp) {
				pr_warn("crashkernel: Memory value expected\n");
				return -EINVAL;
			}
			cur = tmp;
			if (end <= start) {
				pr_warn("crashkernel: end <= start\n");
				return -EINVAL;
			}
		}

		if (*cur != ':') {
			pr_warn("crashkernel: ':' expected\n");
			return -EINVAL;
		}
		cur++;

		size = memparse(cur, &tmp);
		if (cur == tmp) {
			pr_warn("Memory value expected\n");
			return -EINVAL;
		}
		cur = tmp;
		if (size >= system_ram) {
			pr_warn("crashkernel: invalid size\n");
			return -EINVAL;
		}

		/* match ? */
		if (system_ram >= start && system_ram < end) {
			*crash_size = size;
			break;
		}
	} while (*cur++ == ',');

	if (*crash_size > 0) {
		while (*cur && *cur != ' ' && *cur != '@')
			cur++;
		if (*cur == '@') {
			cur++;
			*crash_base = memparse(cur, &tmp);
			if (cur == tmp) {
				pr_warn("Memory value expected after '@'\n");
				return -EINVAL;
			}
		}
	} else
		pr_info("crashkernel size resulted in zero bytes\n");

	return 0;
}

/*
 * That function parses "simple" (old) crashkernel command lines like
 *
 *	crashkernel=size[@offset]
 *
 * It returns 0 on success and -EINVAL on failure.
 */
static int __init parse_crashkernel_simple(char *cmdline,
					   unsigned long long *crash_size,
					   unsigned long long *crash_base)
{
	char *cur = cmdline;

	*crash_size = memparse(cmdline, &cur);
	if (cmdline == cur) {
		pr_warn("crashkernel: memory value expected\n");
		return -EINVAL;
	}

	if (*cur == '@')
		*crash_base = memparse(cur+1, &cur);
	else if (*cur != ' ' && *cur != '\0') {
		pr_warn("crashkernel: unrecognized char: %c\n", *cur);
		return -EINVAL;
	}

	return 0;
}

#define SUFFIX_HIGH 0
#define SUFFIX_LOW  1
#define SUFFIX_NULL 2
static __initdata char *suffix_tbl[] = {
	[SUFFIX_HIGH] = ",high",
	[SUFFIX_LOW]  = ",low",
	[SUFFIX_NULL] = NULL,
};

/*
 * That function parses "suffix"  crashkernel command lines like
 *
 *	crashkernel=size,[high|low]
 *
 * It returns 0 on success and -EINVAL on failure.
 */
static int __init parse_crashkernel_suffix(char *cmdline,
					   unsigned long long	*crash_size,
					   const char *suffix)
{
	char *cur = cmdline;

	*crash_size = memparse(cmdline, &cur);
	if (cmdline == cur) {
		pr_warn("crashkernel: memory value expected\n");
		return -EINVAL;
	}

	/* check with suffix */
	if (strncmp(cur, suffix, strlen(suffix))) {
		pr_warn("crashkernel: unrecognized char: %c\n", *cur);
		return -EINVAL;
	}
	cur += strlen(suffix);
	if (*cur != ' ' && *cur != '\0') {
		pr_warn("crashkernel: unrecognized char: %c\n", *cur);
		return -EINVAL;
	}

	return 0;
}

static __init char *get_last_crashkernel(char *cmdline,
			     const char *name,
			     const char *suffix)
{
	char *p = cmdline, *ck_cmdline = NULL;

	/* find crashkernel and use the last one if there are more */
	p = strstr(p, name);
	while (p) {
		char *end_p = strchr(p, ' ');
		char *q;

		if (!end_p)
			end_p = p + strlen(p);

		if (!suffix) {
			int i;

			/* skip the one with any known suffix */
			for (i = 0; suffix_tbl[i]; i++) {
				q = end_p - strlen(suffix_tbl[i]);
				if (!strncmp(q, suffix_tbl[i],
					     strlen(suffix_tbl[i])))
					goto next;
			}
			ck_cmdline = p;
		} else {
			q = end_p - strlen(suffix);
			if (!strncmp(q, suffix, strlen(suffix)))
				ck_cmdline = p;
		}
next:
		p = strstr(p+1, name);
	}

	if (!ck_cmdline)
		return NULL;

	return ck_cmdline;
}

static int __init __parse_crashkernel(char *cmdline,
			     unsigned long long system_ram,
			     unsigned long long *crash_size,
			     unsigned long long *crash_base,
			     const char *name,
			     const char *suffix)
{
	char	*first_colon, *first_space;
	char	*ck_cmdline;

	BUG_ON(!crash_size || !crash_base);
	*crash_size = 0;
	*crash_base = 0;

	ck_cmdline = get_last_crashkernel(cmdline, name, suffix);

	if (!ck_cmdline)
		return -EINVAL;

	ck_cmdline += strlen(name);

	if (suffix)
		return parse_crashkernel_suffix(ck_cmdline, crash_size,
				suffix);
	/*
	 * if the commandline contains a ':', then that's the extended
	 * syntax -- if not, it must be the classic syntax
	 */
	first_colon = strchr(ck_cmdline, ':');
	first_space = strchr(ck_cmdline, ' ');
	if (first_colon && (!first_space || first_colon < first_space))
		return parse_crashkernel_mem(ck_cmdline, system_ram,
				crash_size, crash_base);

	return parse_crashkernel_simple(ck_cmdline, crash_size, crash_base);
}

/*
 * That function is the entry point for command line parsing and should be
 * called from the arch-specific code.
 */
int __init parse_crashkernel(char *cmdline,
			     unsigned long long system_ram,
			     unsigned long long *crash_size,
			     unsigned long long *crash_base)
{
	return __parse_crashkernel(cmdline, system_ram, crash_size, crash_base,
					"crashkernel=", NULL);
}

int __init parse_crashkernel_high(char *cmdline,
			     unsigned long long system_ram,
			     unsigned long long *crash_size,
			     unsigned long long *crash_base)
{
	return __parse_crashkernel(cmdline, system_ram, crash_size, crash_base,
				"crashkernel=", suffix_tbl[SUFFIX_HIGH]);
}

int __init parse_crashkernel_low(char *cmdline,
			     unsigned long long system_ram,
			     unsigned long long *crash_size,
			     unsigned long long *crash_base)
{
	return __parse_crashkernel(cmdline, system_ram, crash_size, crash_base,
				"crashkernel=", suffix_tbl[SUFFIX_LOW]);
}

/*
 * --------- Crashkernel reservation ------------------------------
 */

#ifdef CONFIG_ARCH_WANT_RESERVE_CRASH_KERNEL
bool crash_low_mem_page_map __initdata;
static bool crash_high_mem_reserved __initdata;
static struct resource crashk_res_high;

static int __init reserve_crashkernel_low(void)
{
#ifdef CONFIG_64BIT
	unsigned long long base, low_base = 0, low_size = 0;
	unsigned long low_mem_limit;
	int ret;

	low_mem_limit = min(memblock_phys_mem_size(), CRASH_ADDR_LOW_MAX);

	/* crashkernel=Y,low */
	ret = parse_crashkernel_low(boot_command_line, low_mem_limit, &low_size, &base);
	if (ret) {
		/*
		 * two parts from kernel/dma/swiotlb.c:
		 * -swiotlb size: user-specified with swiotlb= or default.
		 *
		 * -swiotlb overflow buffer: now hardcoded to 32k. We round it
		 * to 8M for other buffers that may need to stay low too. Also
		 * make sure we allocate enough extra low memory so that we
		 * don't run out of DMA buffers for 32-bit devices.
		 */
		low_size = max(swiotlb_size_or_default() + (8UL << 20), 256UL << 20);
	} else {
		/* passed with crashkernel=0,low ? */
		if (!low_size)
			return 0;
	}

	low_base = memblock_find_in_range(CRASH_ALIGN, CRASH_ADDR_LOW_MAX,
			low_size, CRASH_ALIGN);
	if (!low_base) {
		pr_err("Cannot reserve %ldMB crashkernel low memory, please try smaller size.\n",
		       (unsigned long)(low_size >> 20));
		return -ENOMEM;
	}

	ret = memblock_reserve(low_base, low_size);
	if (ret) {
		pr_err("%s: Error reserving crashkernel low memblock.\n", __func__);
		return ret;
	}

	pr_info("Reserving %ldMB of low memory at %ldMB for crashkernel (low RAM limit: %ldMB)\n",
		(unsigned long)(low_size >> 20),
		(unsigned long)(low_base >> 20),
		(unsigned long)(low_mem_limit >> 20));

	crashk_low_res.start = low_base;
	crashk_low_res.end   = low_base + low_size - 1;
#endif
	return 0;
}

void __init reserve_crashkernel_high(void)
{
	unsigned long long crash_base, crash_size;
	char *cmdline = boot_command_line;
	int ret;

	if (!IS_ENABLED(CONFIG_KEXEC_CORE))
		return;

	/* crashkernel=X[@offset] */
	ret = parse_crashkernel(cmdline, memblock_phys_mem_size(),
				&crash_size, &crash_base);
	if (ret || !crash_size) {
		ret = parse_crashkernel_high(cmdline, 0, &crash_size, &crash_base);
		if (ret || !crash_size)
			return;
	} else if (!crash_base) {
		crash_low_mem_page_map = true;
	}

	crash_size = PAGE_ALIGN(crash_size);

	/*
	 * For the case crashkernel=X, may fall back to reserve memory above
	 * 4G, make reservations here in advance. It will be released later if
	 * the region is successfully reserved under 4G.
	 */
	if (!crash_base) {
		crash_base = memblock_phys_alloc_range(crash_size, CRASH_ALIGN,
						       crash_base, CRASH_ADDR_HIGH_MAX);
		if (!crash_base)
			return;

		crash_high_mem_reserved = true;
	}

	/* Mark the memory range that requires page-level mappings */
	crashk_res.start = crash_base;
	crashk_res.end   = crash_base + crash_size - 1;
}

static void __init hand_over_reserved_high_mem(void)
{
	crashk_res_high.start = crashk_res.start;
	crashk_res_high.end   = crashk_res.end;

	crashk_res.start = 0;
	crashk_res.end   = 0;
}

static void __init take_reserved_high_mem(unsigned long long *crash_base,
					  unsigned long long *crash_size)
{
	*crash_base = crashk_res_high.start;
	*crash_size = resource_size(&crashk_res_high);
}

static void __init free_reserved_high_mem(void)
{
	memblock_free(crashk_res_high.start, resource_size(&crashk_res_high));
}

static bool __init within_low_mem(unsigned long long crash_base,
				  unsigned long long crash_size)
{
	return crash_base < CRASH_ADDR_LOW_MAX &&
		CRASH_ADDR_LOW_MAX - crash_base >= crash_size;
}

/*
 * reserve_crashkernel() - reserves memory for crash kernel
 *
 * This function reserves memory area given in "crashkernel=" kernel command
 * line parameter. The memory reserved is used by dump capture kernel when
 * primary kernel is crashing.
 */
void __init reserve_crashkernel(void)
{
	unsigned long long crash_size, crash_base, total_mem;
	bool high = false;
	int ret;

	total_mem = memblock_phys_mem_size();

	hand_over_reserved_high_mem();

	/* crashkernel=XM */
	ret = parse_crashkernel(boot_command_line, total_mem, &crash_size, &crash_base);
	if (ret != 0 || crash_size <= 0) {
		/* crashkernel=X,high */
		ret = parse_crashkernel_high(boot_command_line, total_mem,
					     &crash_size, &crash_base);
		if (ret != 0 || crash_size <= 0)
			return;
		high = true;

		if (crash_high_mem_reserved) {
			take_reserved_high_mem(&crash_base, &crash_size);
			if (within_low_mem(crash_base, crash_size))
				goto reserve_ok;
			goto reserve_low;
		}
	}

	/* 0 means: find the address automatically */
	if (!crash_base) {
		/*
		 * Set CRASH_ADDR_LOW_MAX upper bound for crash memory,
		 * crashkernel=x,high reserves memory over CRASH_ADDR_LOW_MAX,
		 * also allocates 256M extra low memory for DMA buffers
		 * and swiotlb.
		 * But the extra memory is not required for all machines.
		 * So try low memory first and fall back to high memory
		 * unless "crashkernel=size[KMG],high" is specified.
		 */
		if (!high) {
			crash_base = memblock_find_in_range(CRASH_ALIGN,
					CRASH_ADDR_LOW_MAX, crash_size,
					CRASH_ALIGN);
			if (!crash_base && crash_high_mem_reserved) {
				take_reserved_high_mem(&crash_base, &crash_size);
				if (within_low_mem(crash_base, crash_size))
					goto reserve_ok;
				goto reserve_low;
			}
		}
		if (!crash_base)
			crash_base = memblock_find_in_range(CRASH_ALIGN,
					CRASH_ADDR_HIGH_MAX, crash_size,
					CRASH_ALIGN);
		if (!crash_base) {
			pr_info("crashkernel reservation failed - No suitable area found.\n");
			return;
		}
	} else {
		/* User specifies base address explicitly. */
		unsigned long long start;

		if (!IS_ALIGNED(crash_base, CRASH_ALIGN)) {
			pr_warn("cannot reserve crashkernel: base address is not %ldMB aligned\n",
				(unsigned long)CRASH_ALIGN >> 20);
			return;
		}

		start = memblock_find_in_range(crash_base,
				crash_base + crash_size, crash_size,
				CRASH_ALIGN);
		if (start != crash_base) {
			pr_info("crashkernel reservation failed - memory is in use.\n");
			return;
		}
	}
	ret = memblock_reserve(crash_base, crash_size);
	if (ret) {
		pr_err("%s: Error reserving crashkernel memblock.\n", __func__);
		return;
	}

	if ((crash_base >= CRASH_ADDR_LOW_MAX) || high) {
reserve_low:
		if (reserve_crashkernel_low()) {
			memblock_free(crash_base, crash_size);
			return;
		}
	} else if (crash_high_mem_reserved) {
		/*
		 * The crash memory is successfully allocated under 4G, and the
		 * previously reserved high memory is no longer required.
		 */
		free_reserved_high_mem();
	}

reserve_ok:
	pr_info("Reserving %ldMB of memory at %ldMB for crashkernel (System RAM: %ldMB)\n",
		(unsigned long)(crash_size >> 20),
		(unsigned long)(crash_base >> 20),
		(unsigned long)(total_mem >> 20));

	crashk_res.start = crash_base;
	crashk_res.end   = crash_base + crash_size - 1;
}
#endif /* CONFIG_ARCH_WANT_RESERVE_CRASH_KERNEL */

#ifdef CONFIG_PIN_MEMORY
int __init parse_pin_memory(char *cmdline,
		unsigned long long system_ram,
		unsigned long long *pin_size,
		unsigned long long *pin_base)
{
	return __parse_crashkernel(cmdline, system_ram, pin_size, pin_base,
		"pinmemory=", NULL);
}
#endif

Elf_Word *append_elf_note(Elf_Word *buf, char *name, unsigned int type,
			  void *data, size_t data_len)
{
	struct elf_note *note = (struct elf_note *)buf;

	note->n_namesz = strlen(name) + 1;
	note->n_descsz = data_len;
	note->n_type   = type;
	buf += DIV_ROUND_UP(sizeof(*note), sizeof(Elf_Word));
	memcpy(buf, name, note->n_namesz);
	buf += DIV_ROUND_UP(note->n_namesz, sizeof(Elf_Word));
	memcpy(buf, data, data_len);
	buf += DIV_ROUND_UP(data_len, sizeof(Elf_Word));

	return buf;
}

void final_note(Elf_Word *buf)
{
	memset(buf, 0, sizeof(struct elf_note));
}

static void update_vmcoreinfo_note(void)
{
	u32 *buf = vmcoreinfo_note;

	if (!vmcoreinfo_size)
		return;
	buf = append_elf_note(buf, VMCOREINFO_NOTE_NAME, 0, vmcoreinfo_data,
			      vmcoreinfo_size);
	final_note(buf);
}

void crash_update_vmcoreinfo_safecopy(void *ptr)
{
	if (ptr)
		memcpy(ptr, vmcoreinfo_data, vmcoreinfo_size);

	vmcoreinfo_data_safecopy = ptr;
}

void crash_save_vmcoreinfo(void)
{
	if (!vmcoreinfo_note)
		return;

	/* Use the safe copy to generate vmcoreinfo note if have */
	if (vmcoreinfo_data_safecopy)
		vmcoreinfo_data = vmcoreinfo_data_safecopy;

	vmcoreinfo_append_str("CRASHTIME=%lld\n", ktime_get_real_seconds());
	update_vmcoreinfo_note();
}

void vmcoreinfo_append_str(const char *fmt, ...)
{
	va_list args;
	char buf[0x50];
	size_t r;

	va_start(args, fmt);
	r = vscnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	r = min(r, (size_t)VMCOREINFO_BYTES - vmcoreinfo_size);

	memcpy(&vmcoreinfo_data[vmcoreinfo_size], buf, r);

	vmcoreinfo_size += r;
}

/*
 * provide an empty default implementation here -- architecture
 * code may override this
 */
void __weak arch_crash_save_vmcoreinfo(void)
{}

phys_addr_t __weak paddr_vmcoreinfo_note(void)
{
	return __pa(vmcoreinfo_note);
}
EXPORT_SYMBOL(paddr_vmcoreinfo_note);

#define NOTES_SIZE (&__stop_notes - &__start_notes)
#define BUILD_ID_MAX SHA1_DIGEST_SIZE
#define NT_GNU_BUILD_ID 3

struct elf_note_section {
	struct elf_note	n_hdr;
	u8 n_data[];
};

/*
 * Add build ID from .notes section as generated by the GNU ld(1)
 * or LLVM lld(1) --build-id option.
 */
static void add_build_id_vmcoreinfo(void)
{
	char build_id[BUILD_ID_MAX * 2 + 1];
	int n_remain = NOTES_SIZE;

	while (n_remain >= sizeof(struct elf_note)) {
		const struct elf_note_section *note_sec =
			&__start_notes + NOTES_SIZE - n_remain;
		const u32 n_namesz = note_sec->n_hdr.n_namesz;

		if (note_sec->n_hdr.n_type == NT_GNU_BUILD_ID &&
		    n_namesz != 0 &&
		    !strcmp((char *)&note_sec->n_data[0], "GNU")) {
			if (note_sec->n_hdr.n_descsz <= BUILD_ID_MAX) {
				const u32 n_descsz = note_sec->n_hdr.n_descsz;
				const u8 *s = &note_sec->n_data[n_namesz];

				s = PTR_ALIGN(s, 4);
				bin2hex(build_id, s, n_descsz);
				build_id[2 * n_descsz] = '\0';
				VMCOREINFO_BUILD_ID(build_id);
				return;
			}
			pr_warn("Build ID is too large to include in vmcoreinfo: %u > %u\n",
				note_sec->n_hdr.n_descsz,
				BUILD_ID_MAX);
			return;
		}
		n_remain -= sizeof(struct elf_note) +
			ALIGN(note_sec->n_hdr.n_namesz, 4) +
			ALIGN(note_sec->n_hdr.n_descsz, 4);
	}
}

static int __init crash_save_vmcoreinfo_init(void)
{
	vmcoreinfo_data = (unsigned char *)get_zeroed_page(GFP_KERNEL);
	if (!vmcoreinfo_data) {
		pr_warn("Memory allocation for vmcoreinfo_data failed\n");
		return -ENOMEM;
	}

	vmcoreinfo_note = alloc_pages_exact(VMCOREINFO_NOTE_SIZE,
						GFP_KERNEL | __GFP_ZERO);
	if (!vmcoreinfo_note) {
		free_page((unsigned long)vmcoreinfo_data);
		vmcoreinfo_data = NULL;
		pr_warn("Memory allocation for vmcoreinfo_note failed\n");
		return -ENOMEM;
	}

	VMCOREINFO_OSRELEASE(init_uts_ns.name.release);
	add_build_id_vmcoreinfo();
	VMCOREINFO_PAGESIZE(PAGE_SIZE);

	VMCOREINFO_SYMBOL(init_uts_ns);
	VMCOREINFO_SYMBOL(node_online_map);
#ifdef CONFIG_MMU
	VMCOREINFO_SYMBOL_ARRAY(swapper_pg_dir);
#endif
	VMCOREINFO_SYMBOL(_stext);
	VMCOREINFO_SYMBOL(vmap_area_list);

#ifndef CONFIG_NEED_MULTIPLE_NODES
	VMCOREINFO_SYMBOL(mem_map);
	VMCOREINFO_SYMBOL(contig_page_data);
#endif
#ifdef CONFIG_SPARSEMEM
	VMCOREINFO_SYMBOL_ARRAY(mem_section);
	VMCOREINFO_LENGTH(mem_section, NR_SECTION_ROOTS);
	VMCOREINFO_STRUCT_SIZE(mem_section);
	VMCOREINFO_OFFSET(mem_section, section_mem_map);
	VMCOREINFO_NUMBER(SECTION_SIZE_BITS);
	VMCOREINFO_NUMBER(MAX_PHYSMEM_BITS);
#endif
	VMCOREINFO_STRUCT_SIZE(page);
	VMCOREINFO_STRUCT_SIZE(pglist_data);
	VMCOREINFO_STRUCT_SIZE(zone);
	VMCOREINFO_STRUCT_SIZE(free_area);
	VMCOREINFO_STRUCT_SIZE(list_head);
	VMCOREINFO_SIZE(nodemask_t);
	VMCOREINFO_OFFSET(page, flags);
	VMCOREINFO_OFFSET(page, _refcount);
	VMCOREINFO_OFFSET(page, mapping);
	VMCOREINFO_OFFSET(page, lru);
	VMCOREINFO_OFFSET(page, _mapcount);
	VMCOREINFO_OFFSET(page, private);
	VMCOREINFO_OFFSET(page, compound_dtor);
	VMCOREINFO_OFFSET(page, compound_order);
	VMCOREINFO_OFFSET(page, compound_head);
	VMCOREINFO_OFFSET(pglist_data, node_zones);
	VMCOREINFO_OFFSET(pglist_data, nr_zones);
#ifdef CONFIG_FLAT_NODE_MEM_MAP
	VMCOREINFO_OFFSET(pglist_data, node_mem_map);
#endif
	VMCOREINFO_OFFSET(pglist_data, node_start_pfn);
	VMCOREINFO_OFFSET(pglist_data, node_spanned_pages);
	VMCOREINFO_OFFSET(pglist_data, node_id);
	VMCOREINFO_OFFSET(zone, free_area);
	VMCOREINFO_OFFSET(zone, vm_stat);
	VMCOREINFO_OFFSET(zone, spanned_pages);
	VMCOREINFO_OFFSET(free_area, free_list);
	VMCOREINFO_OFFSET(list_head, next);
	VMCOREINFO_OFFSET(list_head, prev);
	VMCOREINFO_OFFSET(vmap_area, va_start);
	VMCOREINFO_OFFSET(vmap_area, list);
	VMCOREINFO_LENGTH(zone.free_area, MAX_ORDER);
	log_buf_vmcoreinfo_setup();
	VMCOREINFO_LENGTH(free_area.free_list, MIGRATE_TYPES);
	VMCOREINFO_NUMBER(NR_FREE_PAGES);
	VMCOREINFO_NUMBER(PG_lru);
	VMCOREINFO_NUMBER(PG_private);
	VMCOREINFO_NUMBER(PG_swapcache);
	VMCOREINFO_NUMBER(PG_swapbacked);
	VMCOREINFO_NUMBER(PG_slab);
#ifdef CONFIG_MEMORY_FAILURE
	VMCOREINFO_NUMBER(PG_hwpoison);
#endif
	VMCOREINFO_NUMBER(PG_head_mask);
#define PAGE_BUDDY_MAPCOUNT_VALUE	(~PG_buddy)
	VMCOREINFO_NUMBER(PAGE_BUDDY_MAPCOUNT_VALUE);
#ifdef CONFIG_HUGETLB_PAGE
	VMCOREINFO_NUMBER(HUGETLB_PAGE_DTOR);
#define PAGE_OFFLINE_MAPCOUNT_VALUE	(~PG_offline)
	VMCOREINFO_NUMBER(PAGE_OFFLINE_MAPCOUNT_VALUE);
#endif

	arch_crash_save_vmcoreinfo();
	update_vmcoreinfo_note();

	return 0;
}

subsys_initcall(crash_save_vmcoreinfo_init);
