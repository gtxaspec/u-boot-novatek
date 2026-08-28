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

#include <asm/arch/ns02401_regs.h>
#include <asm/arch/ns02401_pinmux.h>
#include <asm/arch/top_reg.h>

#include <common.h>
#include <command.h>

#undef DEBUG_PRINT
// #define DEBUG_PRINT
#ifdef DEBUG_PRINT
#define dbg_printf printf
#else
#define dbg_printf(fmt...)
#endif

// CG
#define CG_UART_CLKDIV_REG_1    (IOADDR_CG_REG_BASE + 0x48)
#define UART2_CLKDIV_SHF		(8)
#define UART2_CLKDIV_MSK		(0xFF<<UART2_CLKDIV_SHF)
#define UART3_CLKDIV_SHF		(16)
#define UART3_CLKDIV_MSK		(0xFF<<UART3_CLKDIV_SHF)
#define UART4_CLKDIV_SHF		(24)
#define UART4_CLKDIV_MSK		(0xFF<<UART4_CLKDIV_SHF)

#define CG_UART_CLKDIV_REG_2	(IOADDR_CG_REG_BASE + 0x4c)
#define UART5_CLKDIV_SHF		(0)
#define UART5_CLKDIV_MSK		(0xFF<<UART5_CLKDIV_SHF)

#define CG_UART_CLKEN_REG		(IOADDR_CG_REG_BASE + 0x78)
#define UART1_CLKEN			    (1<<0)
#define UART2_CLKEN		    	(1<<1)
#define UART3_CLKEN			    (1<<2)
#define UART4_CLKEN			    (1<<3)
#define UART5_CLKEN			    (1<<4)

//TOP
#define TOP_UART_CTRL_REG		(IOADDR_TOP_REG_BASE + 0x34)
#define TOP_UART_SHT			(0)
#define TOP_UART_MSK			(0x1<<TOP_UART_SHT)
#define TOP_UART_EN				(0X1<<TOP_UART_SHT)

#define TOP_UART2_SHT			(4)
#define TOP_UART2_MSK			(0xf<<TOP_UART2_SHT)
#define TOP_UART2_1_EN			(0x1<<TOP_UART2_SHT)
#define TOP_UART2_2_EN			(0x2<<TOP_UART2_SHT)

#define TOP_UART3_SHT			(8)
#define TOP_UART3_MSK			(0xf<<TOP_UART3_SHT)
#define TOP_UART3_1_EN			(0x1<<TOP_UART3_SHT)
#define TOP_UART3_2_EN			(0x2<<TOP_UART3_SHT)

#define TOP_UART4_SHT			(12)
#define TOP_UART4_MSK			(0xf<<TOP_UART4_SHT)
#define TOP_UART4_1_EN			(0x1<<TOP_UART4_SHT)
#define TOP_UART4_2_EN			(0x2<<TOP_UART4_SHT)
#define TOP_UART4_3_EN			(0x3<<TOP_UART4_SHT)

#define TOP_UART5_SHT			(16)
#define TOP_UART5_MSK			(0xf<<TOP_UART5_SHT)
#define TOP_UART5_1_EN			(0x1<<TOP_UART5_SHT)
#define TOP_UART5_2_EN			(0x2<<TOP_UART5_SHT)


#define TOP_UART2_CTS_RTS_SHT   (22)
#define TOP_UART2_CTS_RTS_MSK	(0x3<<TOP_UART2_CTS_RTS_SHT)
#define TOP_UART2_CTS_RTS_1_EN  (0x1<<TOP_UART2_CTS_RTS_SHT)
#define TOP_UART2_CTS_RTS_2_EN  (0x2<<TOP_UART2_CTS_RTS_SHT)

#define TOP_UART3_CTS_RTS_SHT   (24)
#define TOP_UART3_CTS_RTS_MSK	(0x3<<TOP_UART3_CTS_RTS_SHT)
#define TOP_UART3_CTS_RTS_1_EN  (0x1<<TOP_UART3_CTS_RTS_SHT)
#define TOP_UART3_CTS_RTS_2_EN  (0x2<<TOP_UART3_CTS_RTS_SHT)

// GPIO
#define TOP_C_GPIO_PINMUX_REG		(IOADDR_TOP_REG_BASE + 0xA0)
#define TOP_P_GPIO_PINMUX_REG		(IOADDR_TOP_REG_BASE + 0xA8)
#define TOP_D_GPIO_PINMUX_REG		(IOADDR_TOP_REG_BASE + 0xB4)

#define PAD_PUPD_C_REG			(IOADDR_PAD_REG_BASE)
#define PAD_PUPD_P_REG			(IOADDR_PAD_REG_BASE + 0x08)
#define PAD_PUPD_D_REG			(IOADDR_PAD_REG_BASE + 0x18)


// UART
#define UART_RS485_REG_OFFSET     (0x20)
#define UART_RS485_ENABLE_SHT     (0)
#define UART_RS485_SETUP_TIME_SHT (4)
#define UART_RS485_SETUP_TIME_MSK (0xfff << UART_RS485_SETUP_TIME_SHT)
#define UART_RS485_HOLD_TIME_SHT  (20)
#define UART_RS485_HOLD_TIME_MSK  (0xfff << UART_RS485_HOLD_TIME_SHT)

#define NO_CTS_RTS ((u8)-1)

typedef enum {
	PAD_NONE                    =    0x00,      ///< none of pull up/down
	PAD_PULLDOWN                =    0x01,      ///< pull down
	PAD_PULLUP                  =    0x02,      ///< pull up
	PAD_KEEPER                  =    0x03,      ///< keeper
	PAD_STRONG_PULLUP           =    0x03,      ///< strong pull up

	ENUM_DUMMY4WORD(PAD_PULL)
} PAD_PULL;

#define PAD_MASK(pin)			(0x3 << (((pin)*2)&0x1F))
#define PAD_VAL(pin, setting)		((setting)<<(((pin)*2)&0x1F))

struct uart_pinmux_config {
	u32 pinmux;
	u8 gpio_tx;
	u8 gpio_rx;
	u8 gpio_rts;
	u8 gpio_cts;

	phys_addr_t pad_pupd_reg;
	phys_addr_t gpio_pinmux_reg;
	u32 uart_mask;
	u32 uart_enable;
};

struct uart_pinmux_config uart2_config[] = {
	{TOP_UART2_1_EN, 4, 5, 6, 7, PAD_PUPD_P_REG, TOP_P_GPIO_PINMUX_REG, TOP_UART2_MSK, TOP_UART2_1_EN}, // p_gpio(4, 5)
	{TOP_UART2_2_EN, 8, 9, NO_CTS_RTS, NO_CTS_RTS, PAD_PUPD_C_REG, TOP_C_GPIO_PINMUX_REG, TOP_UART2_MSK, TOP_UART2_2_EN}, // c_gpio(8, 9)
};

struct uart_pinmux_config uart3_config[] = {
	{TOP_UART3_1_EN, 8, 9, NO_CTS_RTS, NO_CTS_RTS, PAD_PUPD_P_REG, TOP_P_GPIO_PINMUX_REG, TOP_UART3_MSK, TOP_UART3_1_EN}, // p_gpio(8, 9)
	{TOP_UART3_2_EN, 1, 2, 8, 9, PAD_PUPD_D_REG, TOP_D_GPIO_PINMUX_REG, TOP_UART3_MSK, TOP_UART3_2_EN}, // d_gpio(1, 2)
};

struct uart_pinmux_config uart4_config[] = {
	{TOP_UART4_1_EN, 10, 11, NO_CTS_RTS, NO_CTS_RTS, PAD_PUPD_P_REG, TOP_P_GPIO_PINMUX_REG, TOP_UART4_MSK, TOP_UART4_1_EN}, // p_gpio(10, 11)
	{TOP_UART4_2_EN, 12, 13, NO_CTS_RTS, NO_CTS_RTS, PAD_PUPD_P_REG, TOP_P_GPIO_PINMUX_REG, TOP_UART4_MSK, TOP_UART4_2_EN}, // p_gpio(12, 13)
    {TOP_UART4_3_EN, 19, 20, NO_CTS_RTS, NO_CTS_RTS, PAD_PUPD_P_REG, TOP_P_GPIO_PINMUX_REG, TOP_UART4_MSK, TOP_UART4_3_EN}, // p_gpio(19, 20)
};

struct uart_pinmux_config uart5_config[] = {
	{TOP_UART5_1_EN, 8, 9, NO_CTS_RTS, NO_CTS_RTS, PAD_PUPD_D_REG, TOP_D_GPIO_PINMUX_REG, TOP_UART5_MSK, TOP_UART5_1_EN}, // p_gpio(8, 9)
	{TOP_UART5_2_EN, 14, 15, NO_CTS_RTS, NO_CTS_RTS, PAD_PUPD_D_REG, TOP_D_GPIO_PINMUX_REG, TOP_UART5_MSK, TOP_UART5_2_EN}, // p_gpio(14, 15)
};


struct uart_init_info {
	const char *fdt_path;

	phys_addr_t uart_reg_base;
	phys_addr_t pinmux_reg_base;
	phys_addr_t clkdiv_reg_base;
	phys_addr_t clken_reg_base;

	u32 pinmux_rtscts_mask;
	u32 pinmux_rtscts_enable;
	u32 pinmux_rs485_oe_enable;
	u32 clken;
	u32 clkdiv_mask;
	u32 clkdiv_shift;

	struct uart_pinmux_config *config;
	u32 count;
};

struct uart_init_info uart_info_list[] = {
	{
		.fdt_path = "/uart@4,f0131000", 
		.uart_reg_base = CONFIG_SYS_NS16550_COM2,
		.pinmux_reg_base = TOP_UART_CTRL_REG,
		.clkdiv_reg_base = CG_UART_CLKDIV_REG_1,
		.clken_reg_base = CG_UART_CLKEN_REG,

		.pinmux_rtscts_mask     = TOP_UART2_CTS_RTS_MSK,
		.pinmux_rtscts_enable   = TOP_UART2_CTS_RTS_1_EN,
		.pinmux_rs485_oe_enable = TOP_UART2_CTS_RTS_1_EN,

		.clken = UART2_CLKEN,
		.clkdiv_mask  = UART2_CLKDIV_MSK,
		.clkdiv_shift = UART2_CLKDIV_SHF,

		.config = uart2_config,
		.count = sizeof(uart2_config)/sizeof(struct uart_pinmux_config)
	},
	{
		.fdt_path = "/uart@4,f0132000", 
		.uart_reg_base = CONFIG_SYS_NS16550_COM3,
		.pinmux_reg_base = TOP_UART_CTRL_REG,
		.clkdiv_reg_base = CG_UART_CLKDIV_REG_1,
		.clken_reg_base = CG_UART_CLKEN_REG,

		.pinmux_rtscts_mask     = TOP_UART3_CTS_RTS_MSK,
		.pinmux_rtscts_enable   = TOP_UART3_CTS_RTS_1_EN,
		.pinmux_rs485_oe_enable = TOP_UART2_CTS_RTS_1_EN,

		.clken = UART3_CLKEN,
		.clkdiv_mask  = UART3_CLKDIV_MSK,
		.clkdiv_shift = UART3_CLKDIV_SHF,

		.config = uart3_config,
		.count = sizeof(uart3_config)/sizeof(struct uart_pinmux_config)
	},
	{
		.fdt_path = "/uart@4,f0133000", 
		.uart_reg_base = CONFIG_SYS_NS16550_COM4,
		.pinmux_reg_base = TOP_UART_CTRL_REG,
		.clkdiv_reg_base = CG_UART_CLKDIV_REG_1,
		.clken_reg_base = CG_UART_CLKEN_REG,

		.clken = UART4_CLKEN,
		.clkdiv_mask  = UART4_CLKDIV_MSK,
		.clkdiv_shift = UART4_CLKDIV_SHF,

		.config = uart4_config,
		.count = sizeof(uart4_config)/sizeof(struct uart_pinmux_config)
	},
	{
		.fdt_path = "/uart@4,f0134000", 
		.uart_reg_base = CONFIG_SYS_NS16550_COM5,
		.pinmux_reg_base = TOP_UART_CTRL_REG,
		.clkdiv_reg_base = CG_UART_CLKDIV_REG_1,
		.clken_reg_base = CG_UART_CLKEN_REG,

		.clken = UART5_CLKEN,
		.clkdiv_mask  = UART5_CLKDIV_MSK,
		.clkdiv_shift = UART5_CLKDIV_SHF,

		.config = uart5_config,
		.count = sizeof(uart5_config)/sizeof(struct uart_pinmux_config)
	}
};

static inline struct uart_pinmux_config *get_uart_pinmux_config(struct uart_init_info *uart_info, u32 pinmux)
{
	u32 i;

	for(i = 0; i < uart_info->count; i++) {
		if(pinmux & uart_info->config[i].pinmux) {
			return &uart_info->config[i];
		}
	}

	return NULL;
}

static inline struct uart_init_info *get_uart_info(phys_addr_t uart_reg_base)
{
	u32 i;
	
	for(i = 0; i < sizeof(uart_info_list)/sizeof(struct uart_init_info); i++) {
		if(uart_reg_base == uart_info_list[i].uart_reg_base) {
			return &uart_info_list[i];
		}
	}

	return NULL;
}

int nvt_serial_calc_divisor(struct ns16550 *port, int baudrate)
{
	u32 PSR;
	u32 lcr_val;
	u32 src_clk = 48000000;

	if (((phys_addr_t)port) == CONFIG_SYS_NS16550_COM1) {
		printf("%s: COM1\r\n", __func__);
		PSR = 0x01;
		src_clk = 24000000;
	} else {
		struct uart_init_info *uart_info;
		uart_info = get_uart_info((phys_addr_t)port);

		if(uart_info == NULL)
			return -EINVAL;;

		u32 reg = readl(uart_info->clkdiv_reg_base);
		reg = (reg & uart_info->clkdiv_mask) >> uart_info->clkdiv_shift;
		src_clk = 48000000 / (reg+1);

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

		lcr_val = readl(&port->lcr);	// re-read lcr
	}

	return (src_clk / 16 / PSR / baudrate);
}

int nvt_serial_plat_init(struct ns16550 *com_port, int baud_divisor)
{
	u32 reg;
	u32 pinmux_value = 0;
	int nodeoffset;
	char path[20] = {0};
	struct uart_init_info *uart_info;
	struct uart_pinmux_config *uart_config;
	const fdt32_t *prop;
	unsigned int rts_start_delay;
	unsigned int rts_stop_delay;

	// COM1 is always used as console and already setup, just skip it
	if((phys_addr_t)com_port == CONFIG_SYS_NS16550_COM1)
		return 0;

	// Get UART info from the list
	uart_info = get_uart_info((phys_addr_t)com_port);
	if(uart_info == NULL)
		return -EINVAL;

	// enable clock
	reg = readl(uart_info->clken_reg_base);
	reg |= uart_info->clken;
	writel(reg, uart_info->clken_reg_base);

	// setup pinmux
	sprintf(path, "/top/uart");
	nodeoffset = fdt_path_offset((const void *)gd->fdt_blob, path);
	prop = (u32 *)fdt_getprop((const void *)gd->fdt_blob, nodeoffset, "pinmux", NULL);
	if (( prop > 0 )) {
		pinmux_value = fdt32_to_cpu(prop[0]);
	}

	dbg_printf("base:%p pinmux:0x%x\r\n", com_port, pinmux_value);

	// Get UART pinmux config from the list
	uart_config = get_uart_pinmux_config(uart_info, pinmux_value);
	if(uart_config == NULL)
		return -EINVAL;

	dbg_printf("pinmux:0x%x, tx:0x%x rx:0x%x rts:0x%x cts:0x%x, MASK:0x%x, ENABLE:0x%x\r\n", 
		uart_config->pinmux, uart_config->gpio_tx, uart_config->gpio_rx, uart_config->gpio_rts, uart_config->gpio_cts,
	    uart_config->uart_mask, uart_config->uart_enable);

	// Set UART pins to normal function mode
	reg = readl(uart_config->gpio_pinmux_reg);
	reg &= ~ (1 << uart_config->gpio_tx | 1 << uart_config->gpio_rx);
	reg |= GPIO_ID_EMUM_FUNC << uart_config->gpio_tx | GPIO_ID_EMUM_FUNC << uart_config->gpio_rx;
	if(uart_config->gpio_rts != NO_CTS_RTS && uart_config->gpio_cts != NO_CTS_RTS) {
		reg &= ~ (1 << uart_config->gpio_rts | 1 << uart_config->gpio_cts);
		reg |= GPIO_ID_EMUM_FUNC << uart_config->gpio_rts | GPIO_ID_EMUM_FUNC << uart_config->gpio_cts;
	}
	writel(reg, uart_config->gpio_pinmux_reg);
	dbg_printf("pgpio-func(0x%x): 0x%x\n", uart_config->gpio_pinmux_reg, readl(uart_config->gpio_pinmux_reg));

	// // Set Tx to pull-up
	reg = readl(uart_config->pad_pupd_reg + 4*(uart_config->gpio_tx / 16));
	reg &= ~PAD_MASK(uart_config->gpio_tx);
	reg |= PAD_VAL(uart_config->gpio_tx, PAD_PULLUP);
	writel(reg, uart_config->pad_pupd_reg  + 4*(uart_config->gpio_tx / 16));

	// Set Rx to pull-up
	reg = readl(uart_config->pad_pupd_reg + 4*(uart_config->gpio_rx / 16));
	reg &= ~PAD_MASK(uart_config->gpio_rx);
	reg |= PAD_VAL(uart_config->gpio_rx, PAD_PULLUP);
	writel(reg, uart_config->pad_pupd_reg + 4*(uart_config->gpio_rx / 16));

	// Set RTS to pull-down
	if (uart_config->gpio_rts != NO_CTS_RTS) {
		reg = readl(uart_config->pad_pupd_reg + 4*(uart_config->gpio_rts / 16));
		reg &= ~PAD_MASK(uart_config->gpio_rts);
		reg |= PAD_VAL(uart_config->gpio_rts, PAD_PULLDOWN);
		writel(reg, uart_config->pad_pupd_reg + 4*(uart_config->gpio_rts / 16));
	}
	
	// Set CTS to pull-down
	if(uart_config->gpio_cts != NO_CTS_RTS) {
		reg = readl(uart_config->pad_pupd_reg + 4*(uart_config->gpio_cts / 16));
		reg &= ~PAD_MASK(uart_config->gpio_cts);
		reg |= PAD_VAL(uart_config->gpio_cts, PAD_PULLDOWN);
		writel(reg, uart_config->pad_pupd_reg + 4*(uart_config->gpio_cts / 16));
	}
	dbg_printf("pad(0x%x): 0x%x\n", uart_config->pad_pupd_reg, readl(uart_config->pad_pupd_reg));

	// Enable UART in top control register
	reg = readl(uart_info->pinmux_reg_base);
	reg &= ~uart_config->uart_mask;
	reg |= uart_config->uart_enable;
	writel(reg, uart_info->pinmux_reg_base);

	// Get UART device tree
    nodeoffset = fdt_path_offset(nvt_fdt_buffer, uart_info->fdt_path);
    if (nodeoffset < 0) {
        dbg_printf("Error: Node '%s' not found.\n", uart_info->fdt_path);
		dbg_printf("%s\n", nvt_fdt_buffer);
        return -ENODEV;
    }

	/*
	 * Set up RS485 configuration specified in the Linux device tree (DTS).
	 * Note: If both Linux and NVT custom RS485 properties exist, NVT custom
	 * properties will override to maintain compatibility with existing customer
	 * implementations.
	*/
	prop = (const u32 *)fdt_getprop(nvt_fdt_buffer, nodeoffset, "rs485_en", NULL);
    if (prop == NULL) {
		prop = (const u32 *)fdt_getprop(nvt_fdt_buffer, nodeoffset, "uboot,rs485-enabled-at-boot-time", NULL);
        if (!prop) {
        	return -ENODEV;
		}
    }
	dbg_printf("rs485_en: %u\n", fdt32_to_cpu(prop[0]));

	// Enable RS485
	if (prop) {
		prop = fdt_getprop(nvt_fdt_buffer, nodeoffset, "rs485_delay", NULL);
		if (!prop) {
			prop = fdt_getprop(nvt_fdt_buffer, nodeoffset, "rs485-rts-delay", NULL);
		}

		if (prop) {
			rts_start_delay = fdt32_to_cpu(prop[0]);
			rts_stop_delay  = fdt32_to_cpu(prop[1]);
		} else {
			rts_start_delay = 0;
			rts_stop_delay  = 0;
		}

		// Enable RS485 pinmux
		reg = readl(uart_info->pinmux_reg_base);
		reg &= ~uart_info->pinmux_rtscts_mask;
		reg |= uart_info->pinmux_rs485_oe_enable;
		writel(reg, uart_info->pinmux_reg_base);

		// Enable RS485 and set delay
		reg = 0;
		reg |= (1 << UART_RS485_ENABLE_SHT);
		reg |= ((rts_start_delay << UART_RS485_SETUP_TIME_SHT) & UART_RS485_SETUP_TIME_MSK);
		reg |= ((rts_stop_delay << UART_RS485_HOLD_TIME_SHT) & UART_RS485_HOLD_TIME_MSK);
		writel(reg, uart_info->uart_reg_base + UART_RS485_REG_OFFSET);

		dbg_printf("start_delay:%u stop_delay:%u\n", rts_start_delay, rts_stop_delay);
	}

	return 0;
}