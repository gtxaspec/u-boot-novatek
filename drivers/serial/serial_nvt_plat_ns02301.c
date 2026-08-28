// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024 Novatek Microelectronics Corp
 */

#include <common.h>
#include <debug_uart.h>
#include <dm.h>
#include <dt-structs.h>
#include <ns16550.h>
#include <serial.h>
#include <dm/device-internal.h>
#include "serial_nvt.h"
#include <asm/arch/IOAddress.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/nvt_common.h>
#include <linux/libfdt.h>

#include <asm/arch/ns02301_regs.h>
#include <asm/arch/ns02301_pinmux.h>
#include <asm/arch/top_reg.h>

#define CG_UART_CLKDIV_REG		(IOADDR_CG_REG_BASE + 0x4C)
#define UART2_CLKDIV_SHF		(0)
#define UART2_CLKDIV_MSK		(0xFF<<UART2_CLKDIV_SHF)
#define UART3_CLKDIV_SHF		(8)
#define UART3_CLKDIV_MSK		(0xFF<<UART3_CLKDIV_SHF)
#define CG_UART_CLKEN_REG		(IOADDR_CG_REG_BASE + 0x74)
#define UART1_CLKEN_MSK			(1<<10)
#define UART2_CLKEN_MSK			(1<<11)
#define UART3_CLKEN_MSK			(1<<22)

#define TOP_UART_CTRL_REG		(IOADDR_TOP_REG_BASE + 0x10)
#define TOP_UART_SHT			(16)
#define TOP_UART_MSK			(0x7<<TOP_UART_SHT)
#define TOP_UART_EN			(0X1<<TOP_UART_SHT)
#define TOP_UART2_SHT			(19)
#define TOP_UART2_MSK			(0x7<<TOP_UART2_SHT)
#define TOP_UART2_1_EN			(0x1<<TOP_UART2_SHT)
#define TOP_UART2_2_EN			(0x2<<TOP_UART2_SHT)
#define TOP_UART2_3_EN			(0x3<<TOP_UART2_SHT)
#define TOP_UART2_4_EN			(0x4<<TOP_UART2_SHT)
#define TOP_UART3_SHT			(22)
#define TOP_UART3_MSK			(0x7<<TOP_UART3_SHT)
#define TOP_UART3_1_EN			(0x1<<TOP_UART3_SHT)
#define TOP_UART3_2_EN			(0x2<<TOP_UART3_SHT)
#define TOP_UART3_3_EN			(0x3<<TOP_UART3_SHT)
#define TOP_UART3_4_EN			(0x4<<TOP_UART3_SHT)
#define TOP_UART3_5_EN			(0x5<<TOP_UART3_SHT)
#define TOP_UART3_6_EN			(0x6<<TOP_UART3_SHT)

// main
#define TOP_CGPIO_PINMUX_REG		(IOADDR_TOP_REG_BASE + 0xA0)
#define TOP_PGPIO_PINMUX_REG		(IOADDR_TOP_REG_BASE + 0xA8)

#define PAD_PUPD1_REG			(IOADDR_PAD_REG_BASE + 0x04)	// CGPIO16~31
#define PAD_PUPD2_REG			(IOADDR_PAD_REG_BASE + 0x08)	// PGPIO0~15
#define PAD_PUPD3_REG			(IOADDR_PAD_REG_BASE + 0x10)	// PGPIO16~31

// pre-roll
#define TOP_SGPIO_PINMUX_REG		(IOADDR_PRTOP_REG_BASE + 0x28)

#define PR_PAD_PUPD2_REG		(IOADDR_PRPAD_REG_BASE + 0x08)	// SGPIO0~15

typedef enum {
	PAD_NONE                    =    0x00,      ///< none of pull up/down
	PAD_PULLDOWN                =    0x01,      ///< pull down
	PAD_PULLUP                  =    0x02,      ///< pull up
	PAD_KEEPER                  =    0x03,      ///< keeper
	PAD_STRONG_PULLUP           =    0x03,      ///< strong pull up

	ENUM_DUMMY4WORD(PAD_PULL)
} PAD_PULL;

#define PAD_MASK(pin)			(0x3<<(((pin)*2)&0x1F))
#define PAD_VAL(pin, setting)		((setting)<<(((pin)*2)&0x1F))

int nvt_serial_calc_divisor(struct ns16550 *port, int baudrate)
{
	u32 PSR;
	u32 lcr_val;
	u32 src_clk = 48000000;

	if (((phys_addr_t)port) == CONFIG_SYS_NS16550_COM1) {
//		printf("%s: COM1\r\n", __func__);
		PSR = 0x01;
		src_clk = 24000000;
	} else {
		u32 reg;

		reg = readl(CG_UART_CLKDIV_REG);
		if (((phys_addr_t)port) == CONFIG_SYS_NS16550_COM2) {
			reg = (reg&UART2_CLKDIV_MSK) >> UART2_CLKDIV_SHF;
			src_clk = 480000000 / (reg+1);
//			printf("%s: COM2 src clk %d\r\n", __func__, src_clk);
		} else if (((phys_addr_t)port) == CONFIG_SYS_NS16550_COM3) {
			reg = (reg&UART3_CLKDIV_MSK) >> UART3_CLKDIV_SHF;
			src_clk = 480000000 / (reg+1);
//			printf("%s: COM3 src clk %d\r\n", __func__, src_clk);
		}
		if (baudrate >= 9600) {
			PSR = 0x01;
		} else if (baudrate >= 2400 && baudrate < 9600) {
			PSR = 0x05;
		} else if (baudrate >= 1200 && baudrate < 2400) {
			PSR = 0x0a;
		} else if (baudrate >= 600 && baudrate < 1200) {
			PSR = 0x14;
		} else {
			PSR = 0x1f;
		}
		lcr_val = readl(&port->lcr);	// backup lcr

		writel(UART_LCR_BKSE | lcr_val, &port->lcr);
		writel(PSR & 0xff, &port->fcr);	// fcr is shadowed as psr when dlab is 1
		writel(lcr_val, &port->lcr);	// restore lcr

//		printf("%s: baud %d, PSR 0x%x\r\n", __func__, baudrate, PSR);
	}

//	printf("%s: div = %d\r\n", __func__, src_clk / 16 / PSR / baudrate);

	return (src_clk / 16 / PSR / baudrate);
}

int nvt_serial_plat_init(struct ns16550 *com_port, int baud_divisor)
{
	u32 reg;
	u32 pinmux_value = 0;
	int nodeoffset;
	u32 *cell = NULL;
	char path[20] = {0};
	/* GPIO */
	union TOP_REGCGPIO0    top_reg_cgpio0;
	union TOP_REGPGPIO0    top_reg_pgpio0;
	/* PR GPIO */
	union PR_TOP_REGSGPIO0 pr_top_reg_sgpio0;

	// enable clock
	reg = readl(CG_UART_CLKEN_REG);
	switch ((phys_addr_t)com_port) {
	case CONFIG_SYS_NS16550_COM1:
		reg |= UART1_CLKEN_MSK;
		return 0;	// COM1 is always used as console and already setup, just skip it
		break;
	case CONFIG_SYS_NS16550_COM2:
		reg |= UART2_CLKEN_MSK;
		break;
	case CONFIG_SYS_NS16550_COM3:
		reg |= UART3_CLKEN_MSK;
		break;
	default:
		return -EINVAL;
	};
	writel(reg, CG_UART_CLKEN_REG);

	// setup pinmux
	sprintf(path, "/top/uart");
	nodeoffset = fdt_path_offset((const void *)nvt_fdt_buffer, path);
	cell = (u32 *)fdt_getprop((const void *)nvt_fdt_buffer, nodeoffset, "pinmux", NULL);
	if (( cell > 0 )) {
		pinmux_value = __be32_to_cpu(cell[0]);
	}

	if (pinmux_value & PIN_UART_CFG_UART_1) {
		reg = readl(PAD_PUPD3_REG);
		reg &= ~(PAD_MASK(23)|PAD_MASK(24));	// mask PGPIO23, 24
		reg |= PAD_VAL(23, PAD_PULLUP) | PAD_VAL(24, PAD_PULLUP);
		writel(reg, PAD_PUPD3_REG);

		reg = readl(TOP_UART_CTRL_REG);
		reg &= ~TOP_UART_MSK;
		reg |= TOP_UART_EN;
		writel(reg, TOP_UART_CTRL_REG);

		top_reg_pgpio0.reg = readl(TOP_PGPIO_PINMUX_REG);
		top_reg_pgpio0.bit.PGPIO_23 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_24 = GPIO_ID_EMUM_FUNC;
		writel(top_reg_pgpio0.reg, TOP_PGPIO_PINMUX_REG);
	}

	if (pinmux_value & PIN_UART_CFG_UART2_1) {
		reg = readl(PAD_PUPD3_REG);
		reg &= ~(PAD_MASK(17)|PAD_MASK(18));	// mask PGPIO17, 18
		reg |= PAD_VAL(17, PAD_PULLUP) | PAD_VAL(18, PAD_PULLUP);
		writel(reg, PAD_PUPD3_REG);

		reg = readl(TOP_UART_CTRL_REG);
		reg &= ~TOP_UART2_MSK;
		reg |= TOP_UART2_1_EN;
		writel(reg, TOP_UART_CTRL_REG);

		top_reg_pgpio0.reg = readl(TOP_PGPIO_PINMUX_REG);
		top_reg_pgpio0.bit.PGPIO_17 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_18 = GPIO_ID_EMUM_FUNC;
		writel(top_reg_pgpio0.reg, TOP_PGPIO_PINMUX_REG);
	} else if (pinmux_value & PIN_UART_CFG_UART2_2) {
		reg = readl(PAD_PUPD1_REG);
		reg &= ~(PAD_MASK(17)|PAD_MASK(18));	// mask CGPIO17, 18
		reg |= PAD_VAL(17, PAD_PULLUP) | PAD_VAL(18, PAD_PULLUP);
		writel(reg, PAD_PUPD1_REG);

		reg = readl(TOP_UART_CTRL_REG);
		reg &= ~TOP_UART2_MSK;
		reg |= TOP_UART2_2_EN;
		writel(reg, TOP_UART_CTRL_REG);

		top_reg_cgpio0.reg = readl(TOP_CGPIO_PINMUX_REG);
		top_reg_cgpio0.bit.CGPIO_17 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_18 = GPIO_ID_EMUM_FUNC;
		writel(top_reg_cgpio0.reg, TOP_CGPIO_PINMUX_REG);
	} else if (pinmux_value & PIN_UART_CFG_UART2_3) {
		reg = readl(PAD_PUPD2_REG);
		reg &= ~(PAD_MASK(0)|PAD_MASK(1));	// mask PGPIO0, 1
		reg |= PAD_VAL(0, PAD_PULLUP) | PAD_VAL(1, PAD_PULLUP);
		writel(reg, PAD_PUPD2_REG);

		reg = readl(TOP_UART_CTRL_REG);
		reg &= ~TOP_UART2_MSK;
		reg |= TOP_UART2_3_EN;
		writel(reg, TOP_UART_CTRL_REG);

		top_reg_pgpio0.reg = readl(TOP_PGPIO_PINMUX_REG);
		top_reg_pgpio0.bit.PGPIO_0 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_1 = GPIO_ID_EMUM_FUNC;
		writel(top_reg_pgpio0.reg, TOP_PGPIO_PINMUX_REG);
	} else if (pinmux_value & PIN_UART_CFG_UART2_4) {
		reg = readl(PR_PAD_PUPD2_REG);
		reg &= ~(PAD_MASK(9)|PAD_MASK(10));	// mask SGPIO9, 10
		reg |= PAD_VAL(9, PAD_PULLUP) | PAD_VAL(10, PAD_PULLUP);
		writel(reg, PR_PAD_PUPD2_REG);

		reg = readl(TOP_UART_CTRL_REG);
		reg &= ~TOP_UART2_MSK;
		reg |= TOP_UART2_4_EN;
		writel(reg, TOP_UART_CTRL_REG);

		pr_top_reg_sgpio0.reg = readl(TOP_SGPIO_PINMUX_REG);
		pr_top_reg_sgpio0.bit.SGPIO_9 = GPIO_ID_EMUM_FUNC_MAIN;
		pr_top_reg_sgpio0.bit.SGPIO_10 = GPIO_ID_EMUM_FUNC_MAIN;
		writel(pr_top_reg_sgpio0.reg, TOP_SGPIO_PINMUX_REG);
	}

	if (pinmux_value & PIN_UART_CFG_UART3_1) {
		reg = readl(PAD_PUPD2_REG);
		reg &= ~(PAD_MASK(8)|PAD_MASK(9));	// mask PGPIO8, 9
		reg |= PAD_VAL(8, PAD_PULLUP) | PAD_VAL(9, PAD_PULLUP);
		writel(reg, PAD_PUPD2_REG);

		reg = readl(TOP_UART_CTRL_REG);
		reg &= ~TOP_UART3_MSK;
		reg |= TOP_UART3_1_EN;
		writel(reg, TOP_UART_CTRL_REG);

		top_reg_pgpio0.reg = readl(TOP_PGPIO_PINMUX_REG);
		top_reg_pgpio0.bit.PGPIO_8 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_9 = GPIO_ID_EMUM_FUNC;
		writel(top_reg_pgpio0.reg, TOP_PGPIO_PINMUX_REG);
	} else if (pinmux_value & PIN_UART_CFG_UART3_2) {
		reg = readl(PAD_PUPD1_REG);
		reg &= ~(PAD_MASK(23)|PAD_MASK(24));	// mask CGPIO23, 24
		reg |= PAD_VAL(23, PAD_PULLUP) | PAD_VAL(24, PAD_PULLUP);
		writel(reg, PAD_PUPD1_REG);

		reg = readl(TOP_UART_CTRL_REG);
		reg &= ~TOP_UART3_MSK;
		reg |= TOP_UART3_2_EN;
		writel(reg, TOP_UART_CTRL_REG);

		top_reg_cgpio0.reg = readl(TOP_CGPIO_PINMUX_REG);
		top_reg_cgpio0.bit.CGPIO_23 = GPIO_ID_EMUM_FUNC;
		top_reg_cgpio0.bit.CGPIO_24 = GPIO_ID_EMUM_FUNC;
		writel(top_reg_cgpio0.reg, TOP_CGPIO_PINMUX_REG);
	} else if (pinmux_value & PIN_UART_CFG_UART3_3) {
		reg = readl(PAD_PUPD2_REG);
		reg &= ~(PAD_MASK(4)|PAD_MASK(5));	// mask PGPIO4, 5
		reg |= PAD_VAL(4, PAD_PULLUP) | PAD_VAL(5, PAD_PULLUP);
		writel(reg, PAD_PUPD2_REG);

		reg = readl(TOP_UART_CTRL_REG);
		reg &= ~TOP_UART3_MSK;
		reg |= TOP_UART3_3_EN;
		writel(reg, TOP_UART_CTRL_REG);

		top_reg_pgpio0.reg = readl(TOP_PGPIO_PINMUX_REG);
		top_reg_pgpio0.bit.PGPIO_4 = GPIO_ID_EMUM_FUNC;
		top_reg_pgpio0.bit.PGPIO_5 = GPIO_ID_EMUM_FUNC;
		writel(top_reg_pgpio0.reg, TOP_PGPIO_PINMUX_REG);
	} else if (pinmux_value & PIN_UART_CFG_UART3_4) {
		reg = readl(PR_PAD_PUPD2_REG);
		reg &= ~(PAD_MASK(0)|PAD_MASK(1));	// mask SGPIO0, 1
		reg |= PAD_VAL(0, PAD_PULLUP) | PAD_VAL(1, PAD_PULLUP);
		writel(reg, PR_PAD_PUPD2_REG);

		reg = readl(TOP_UART_CTRL_REG);
		reg &= ~TOP_UART3_MSK;
		reg |= TOP_UART3_4_EN;
		writel(reg, TOP_UART_CTRL_REG);

		pr_top_reg_sgpio0.reg = readl(TOP_SGPIO_PINMUX_REG);
		pr_top_reg_sgpio0.bit.SGPIO_0 = GPIO_ID_EMUM_FUNC_MAIN;
		pr_top_reg_sgpio0.bit.SGPIO_1 = GPIO_ID_EMUM_FUNC_MAIN;
		writel(pr_top_reg_sgpio0.reg, TOP_SGPIO_PINMUX_REG);
	} else if (pinmux_value & PIN_UART_CFG_UART3_5) {
		reg = readl(PR_PAD_PUPD2_REG);
		reg &= ~(PAD_MASK(4)|PAD_MASK(5));	// mask SGPIO4, 5
		reg |= PAD_VAL(4, PAD_PULLUP) | PAD_VAL(5, PAD_PULLUP);
		writel(reg, PR_PAD_PUPD2_REG);

		reg = readl(TOP_UART_CTRL_REG);
		reg &= ~TOP_UART3_MSK;
		reg |= TOP_UART3_5_EN;
		writel(reg, TOP_UART_CTRL_REG);

		pr_top_reg_sgpio0.reg = readl(TOP_SGPIO_PINMUX_REG);
		pr_top_reg_sgpio0.bit.SGPIO_4 = GPIO_ID_EMUM_FUNC_MAIN;
		pr_top_reg_sgpio0.bit.SGPIO_5 = GPIO_ID_EMUM_FUNC_MAIN;
		writel(pr_top_reg_sgpio0.reg, TOP_SGPIO_PINMUX_REG);
	} else if (pinmux_value & PIN_UART_CFG_UART3_6) {
		reg = readl(PR_PAD_PUPD2_REG);
		reg &= ~(PAD_MASK(13)|PAD_MASK(14));	// mask SGPIO13, 14
		reg |= PAD_VAL(13, PAD_PULLUP) | PAD_VAL(14, PAD_PULLUP);
		writel(reg, PR_PAD_PUPD2_REG);

		reg = readl(TOP_UART_CTRL_REG);
		reg &= ~TOP_UART3_MSK;
		reg |= TOP_UART3_6_EN;
		writel(reg, TOP_UART_CTRL_REG);

		pr_top_reg_sgpio0.reg = readl(TOP_SGPIO_PINMUX_REG);
		pr_top_reg_sgpio0.bit.SGPIO_13 = GPIO_ID_EMUM_FUNC_MAIN;
		pr_top_reg_sgpio0.bit.SGPIO_14 = GPIO_ID_EMUM_FUNC_MAIN;
		writel(pr_top_reg_sgpio0.reg, TOP_SGPIO_PINMUX_REG);
	}

	return 0;
}
