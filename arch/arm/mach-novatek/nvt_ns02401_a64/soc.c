/**
	Provide basic CPU info

    @file       soc.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <common.h>
#include <asm/io.h>
//#include <asm/arch/soc.h>
#include <asm/arch/clock.h>
#include <asm/nvt-common/nvt_ivot_optee.h>
//#include <asm/arch/gpio.h>
#include <asm/arch/ns02401_regs.h>
#include <asm/arch/IOAddress.h>
#include <asm/arch/hardware.h>
#include <linux/sizes.h>
#include <linux/compiler.h>
#include <ns16550.h>
#include <dm.h>
#include <asm/nvt-common/nvt_common.h>
#include <command.h>

static int do_nvt_ddr_remap(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	printf("635 DDR remapping is not supported \n");
	return 0;
}

U_BOOT_CMD(
	nvt_ddr_remap, 2, 1, do_nvt_ddr_remap,
	"Represent DRAMx select",
	"\n"
);

#ifdef CONFIG_NVT_IVOT_DDR_SLOW_DOWN_SUPPORT
extern int nvt_ddr_slow_down(void);
#endif

DECLARE_GLOBAL_DATA_PTR;

#if !CONFIG_IS_ENABLED(OF_CONTROL)
static struct ns16550_platdata ns16550_com1_pdata = {
	.base = CONFIG_SYS_NS16550_COM1,
	.reg_shift = 2,
	.clock = 48000000,
};

static struct ns16550_platdata ns16550_com2_pdata = {
	.base = CONFIG_SYS_NS16550_COM2,
	.reg_shift = 2,
	.clock = 48000000,
};

static struct ns16550_platdata ns16550_com3_pdata = {
	.base = CONFIG_SYS_NS16550_COM3,
	.reg_shift = 2,
	.clock = 48000000,
};

static struct ns16550_platdata ns16550_com4_pdata = {
	.base = CONFIG_SYS_NS16550_COM4,
	.reg_shift = 2,
	.clock = 48000000,
};

static struct ns16550_platdata ns16550_com5_pdata = {
	.base = CONFIG_SYS_NS16550_COM5,
	.reg_shift = 2,
	.clock = 48000000,
};

U_BOOT_DEVICE(ns16550_com1) = {
	"ns16550_serial", &ns16550_com1_pdata
#ifdef CONFIG_SYS_NS16550_COM2
	"ns16550_serial", &ns16550_com2_pdata
#endif
#ifdef CONFIG_SYS_NS16550_COM3
	"ns16550_serial", &ns16550_com3_pdata
#endif
#ifdef CONFIG_SYS_NS16550_COM4
	"ns16550_serial", &ns16550_com4_pdata
#endif
#ifdef CONFIG_SYS_NS16550_COM5
	"ns16550_serial", &ns16550_com5_pdata
#endif
};
#endif



void enable_caches(void)
{
	dcache_enable();
	icache_enable();
}


#ifdef CONFIG_ARCH_MISC_INIT
#ifdef CONFIG_NOVATEK_PAD
extern void nvt_pad_init(void);
#endif
int arch_misc_init(void)
{
#ifdef CONFIG_NOVATEK_PAD
	nvt_pad_init();
#endif
	return 0;
}
#endif

/*
 * Print board information
 */
int checkboard(void)
{
	return 0;
}

/*
 * Print CPU information
 */
#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
	printf("CPU:   Novatek %s @ %lu MHz\n", _CHIP_NAME_, get_cpu_clk()/1000);

	return 0;
}
#endif

void s_init(void)
{
	//u32 val;

	/* Do we want to combine ARM_UART_B and STBC_UART_1 on EVB board?
	 * NO! So we need set fc040250[bit0] to "0"
	 */
	// val = *(volatile unsigned int *)0xfc040250;
	// if ((val & 0x01) == 0x01) {
		// val &= ~0x01;
		// writel(0x0, 0xfc040250);
	// }

	gd->cpu_clk = get_cpu_clk() * 1000;
}

void cpu_cache_initialization(void)
{
	return;
}

u32 nvt_get_chip_id(void)
{
	unsigned int chip_id;

	chip_id = (smc_readl(IOADDR_TOP_REG_BASE + TOP_VERSION_REG_OFS) >> 16) & 0xFFFF;

	return chip_id;
}

//
// ROM code version number
//
#define TOP_ROMVER_A            0x00000102
#define TOP_ROMVER_B            0x00000103
#define TOP_GETROMVER()         readl(0x2F0007FFC)
u32 nvt_get_chip_ver(void)
{
	unsigned int chip_ver;

	chip_ver = TOP_GETROMVER();
    if(chip_ver == TOP_ROMVER_A) {
        return CHIPVER_A;
    } else if(chip_ver == TOP_ROMVER_B) {
        return CHIPVER_B;
    } else {
		printf("force to Ver B\n");
        return CHIPVER_B;
    }

	return chip_ver;
}

