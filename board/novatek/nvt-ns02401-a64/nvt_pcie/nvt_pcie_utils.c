#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <stdlib.h>
#include <asm/arch/IOAddress.h>
#include <asm/nvt-common/nvt_common.h>
#include <asm/arch/nvt_pcie.h>
#include <linux/bitfield.h>


#define EP_BAR0_SIZE			0x800000
#define EP_BAR2_SIZE			0x400000
#define BUS_ADDR			0xC0000000
#define PCIE_MAPPING_BASE		0x4000000000
#define PCIE_SRAM_BASE			0x4F0900000

#define EP_LOADER_SIZE			0x18000
#define OUTBOUND_SIZE			0x2000000

#define CC_RC_EP_BOOT_COMM_REG		0x4F0110120
#define EP_CC_OFFSET			0x120


#define PCIE_STATUS_IDLE		0x544F4F42
#define PCIE_STATUS_DRAM_DONE		0x5944524C

static int nvt_change_ep_inbound(unsigned long dest, unsigned long size)
{
	if (size > OUTBOUND_SIZE) {
		nvt_dbg(MSG, "Size is over %x\n", OUTBOUND_SIZE);
		return -1;
	}
	writel(0x0, 0x104 + EP_BAR0_SIZE + EP_BAR2_SIZE + PCIE_MAPPING_BASE);
	writel(0x2000, 0x100 + EP_BAR0_SIZE + EP_BAR2_SIZE + PCIE_MAPPING_BASE);
	writel(BUS_ADDR, 0x108 + EP_BAR0_SIZE + EP_BAR2_SIZE + PCIE_MAPPING_BASE);
	writel(0x0, 0x10C + EP_BAR0_SIZE + EP_BAR2_SIZE + PCIE_MAPPING_BASE);
	writel(BUS_ADDR + EP_BAR2_SIZE - 1, 0x110 + EP_BAR0_SIZE + EP_BAR2_SIZE + PCIE_MAPPING_BASE);
	writel(dest & GENMASK(31,0), 0x114 + EP_BAR0_SIZE + EP_BAR2_SIZE + PCIE_MAPPING_BASE);
	writel(dest >> 32, 0x118 + EP_BAR0_SIZE + EP_BAR2_SIZE + PCIE_MAPPING_BASE);
	writel(0x0, 0x11C + EP_BAR0_SIZE + EP_BAR2_SIZE + PCIE_MAPPING_BASE);
	writel(0x0, 0x120 + EP_BAR0_SIZE + EP_BAR2_SIZE + PCIE_MAPPING_BASE);
	writel(0x80000000, 0x104 + EP_BAR0_SIZE + EP_BAR2_SIZE + PCIE_MAPPING_BASE);

	return 0;
}

static int do_nvt_pcie_copy_sram(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned int eploader_size = EP_LOADER_SIZE;
	unsigned int *p_src = (unsigned int *)simple_strtoul(argv[1], NULL, 16);
	unsigned int *p_dst = (unsigned int *)(PCIE_MAPPING_BASE);

	if (nvt_change_ep_inbound(PCIE_SRAM_BASE, eploader_size))
		return -1;

	memcpy_toio(p_dst, p_src, eploader_size);
	return 0;
}

U_BOOT_CMD(
	nvt_pcie_copy_sram, 2,    1,      do_nvt_pcie_copy_sram,
	"nvt_pcie_copy_sram [eploader addre]",
	""
);

static int do_nvt_pcie_boot(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	return 0;
}

U_BOOT_CMD(
	nvt_pcie_boot, 1,    1,      do_nvt_pcie_boot,
	"trigger pcie CPU",
	""
);

static char epstatus_help_text[] =
	"nvt_pcie_epstatus get - get current ep status\n"
	"nvt_pcie_epstatus set idle - set communication register to idle\n"
	"nvt_pcie_epstatus set start [epfdt address] - set ep fdt address to communication register\n"
	"";

static int do_nvt_pcie_epstatus(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = -1;
	u32 value;
	u32 status;
	enum {
		IDLE = 0,
		DRAM_DONE,
	};

	if (nvt_change_ep_inbound(CC_RC_EP_BOOT_COMM_REG, 0x1000))
		return -1;

	if (argc == 2) {
		status = readl(PCIE_MAPPING_BASE + EP_CC_OFFSET);
		switch (status) {
		case PCIE_STATUS_IDLE :
			nvt_dbg(MSG, "EP Idle \n");
			ret = IDLE;
			break;
		case PCIE_STATUS_DRAM_DONE :
			nvt_dbg(MSG, "EP Dram config done \n");
			ret = DRAM_DONE;
			break;
		default:
			nvt_dbg(MSG, "EP on unknow status\n");
			ret = -1;
			break;
		}
	} else if (argc == 3) {
		if (strcmp(argv[2], "idle") == 0) {
			writel(PCIE_STATUS_IDLE, PCIE_MAPPING_BASE + EP_CC_OFFSET);
			ret = 0;
		}
	} else if (argc == 4) {
		if (strcmp(argv[2], "start") == 0) {
			value = (u32)simple_strtoul(argv[3], NULL, 16);
			nvt_dbg(MSG, "write start value 0x%x\n", value);
			writel(value, PCIE_MAPPING_BASE + EP_CC_OFFSET);
			ret = 0;
		}
	}

	return ret;
}

U_BOOT_CMD(
	nvt_pcie_epstatus, 4,    1,      do_nvt_pcie_epstatus,
	"get or set epstatus on boot ep flow",
	epstatus_help_text
);

static int do_nvt_pcie_copy(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned long *src = (unsigned long *)simple_strtoul(argv[1], NULL, 16);
	unsigned long *ep_dest = (unsigned long *)(simple_strtoul(argv[2], NULL, 16) + PCIE_MAPPING_BASE);
	unsigned long dest = (unsigned long )(simple_strtoul(argv[2], NULL, 16));
	unsigned long size = (unsigned long)simple_strtoul(argv[3], NULL, 16);
	u32 status;

	if (!size) {
		nvt_dbg(MSG, "Wrong size = 0x%lx\n", size);
	}

	if (nvt_change_ep_inbound(CC_RC_EP_BOOT_COMM_REG, 0x1000))
		return -1;
	status = readl(PCIE_MAPPING_BASE + EP_CC_OFFSET);
	switch (status) {
	case PCIE_STATUS_DRAM_DONE :
		nvt_dbg(MSG, "EP Dram config done \n");
		break;
	default:
		nvt_dbg(MSG, "EP Dram is not ready\n");
		return -1;
	}

	if (nvt_change_ep_inbound(dest, size))
		return -1;

	nvt_dbg(MSG, "src : 0x%lx, dest : 0x%lx, size : 0x%lx\n", (unsigned long)src, (unsigned long)ep_dest, size);
	memcpy((void *)PCIE_MAPPING_BASE, src, size);
	// coverity[overflow_const]: size will not be zero, it is checked at the beginning of the function
	flush_cache(round_down((unsigned long)PCIE_MAPPING_BASE, CONFIG_SYS_CACHELINE_SIZE), round_up(size, CONFIG_SYS_CACHELINE_SIZE));
	return 0;
}

U_BOOT_CMD(
	nvt_pcie_copy, 4,    1,      do_nvt_pcie_copy,
	"nvt_pcie_copy [source data] [ep address] [size]",
	""
);

static int do_nvt_pcie_init(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	return 0;
}

U_BOOT_CMD(
	nvt_pcie_init, 1,    1,      do_nvt_pcie_init,
	"nvt_pcie_init",
	""
);
