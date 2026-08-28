// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024, NOVATEK MICROELECTRONIC CORPORATION. All rights reserved.
 *
 * Portions based on U-Boot's dwc_eth_qos.c.
 */

/*
 * This driver supports the Synopsys Designware Ethernet XGMAC (10G Ethernet
 * MAC) IP block. The IP supports multiple options for bus type, clocking/
 * reset structure, and feature list.
 *
 * The driver is written such that generic core logic is kept separate from
 * configuration-specific logic. Code that interacts with configuration-
 * specific resources is split out into separate functions to avoid polluting
 * common code. If/when this driver is enhanced to support multiple
 * configurations, the core code should be adapted to call all configuration-
 * specific functions through function pointers, with the definition of those
 * function pointers being supplied by struct udevice_id xgmac_ids[]'s .data
 * field.
 *
 * This configuration uses an AXI master/DMA bus, an AHB slave/register bus,
 * contains the DMA, MTL, and MAC sub-blocks, and supports a single RGMII PHY.
 * This configuration also has SW control over all clock and reset signals to
 * the HW block.
 */

#define LOG_CATEGORY UCLASS_ETH

#include <clk.h>
#include <cpu_func.h>
#include <dm.h>
#include <errno.h>
#include <eth_phy.h>
#include <log.h>
#include <malloc.h>
#include <memalign.h>
#include <miiphy.h>
#include <net.h>
#include <netdev.h>
#include <phy.h>
#include <reset.h>
#include <wait_bit.h>
#include <asm/cache.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/nvt_common.h>
#include <asm/nvt-common/nvt_reg.h>
#include <asm/arch/efuse_protected.h>
#include <dm/device_compat.h>
#include "eth_ns02401.h"

#define SOCFPGA_XGMAC_SYSCON_ARG_COUNT 2

/**
    PIN config for ETH

    @note For pinmux_init() for PIN_FUNC_ETH
*/
#define PIN_ETH_CFG_NONE                0x0

#define PIN_ETH_CFG_ETH_RGMII_1         0x1          ///< ETH_RGMII      (E_GPIO[2..15])
#define PIN_ETH_CFG_ETH_RMII_1          0x10         ///< ETH_RMII       (E_GPIO[2..5], E_GPIO[9..11], E_GPIO[14..15])
#define PIN_ETH_CFG_ETH_SGMII_1         0x100        ///< ETH_SGRMII     (E_GPIO[14..15])
#define PIN_ETH_CFG_ETH_EXTPHYCLK       0x1000       ///< ETH_EXTPHYCLK  (E_GPIO[0])

#define PIN_ETH_CFG_ETH2_RGMII_1        0x10000      ///< ETH2_RGMII     (E_GPIO[18..31])
#define PIN_ETH_CFG_ETH2_RMII_1         0x100000     ///< ETH2_RMII      (E_GPIO[18..21], E_GPIO[25..27], E_GPIO[30..31])
#define PIN_ETH_CFG_ETH2_SGMII_1        0x1000000    ///< ETH2_SGRMII    (E_GPIO[30..31])
#define PIN_ETH_CFG_ETH2_EXTPHYCLK      0x10000000   ///< ETH2_EXTPHYCLK (E_GPIO[16]

// Serdes PHY
#define IOADDR_SERDES_PHY_REG_BASE  (IOADDR_GLOBAL_BASE+0x00531000)
#define IOADDR_SERDES_PHY1_REG_BASE (IOADDR_GLOBAL_BASE+0x00541000)

// XPCS
#define IOADDR_XPCS_REG_BASE        (IOADDR_GLOBAL_BASE+0x005B0000)
#define IOADDR_XPCS1_REG_BASE       (IOADDR_GLOBAL_BASE+0x005C0000)

//#define debug(fmt, args...)   printf(fmt, ##args)
#define REG32_MASK          0x00000000FFFFFFFF
#define DEFAULT_MAC_ADDRESS {0x00, 0x80, 0x48, 0xBA, 0xD1, 0x30}
#define CH_DIS 3
#define XGMAC_DMA_AXI_CNRT_REFS_BASE 0x4040

u8 phyaddr = 0;
phy_interface_t ETH_PHY_INTF = 0;
//UINT ETH_QOS_INDEX; // index to MAC (0: MAC0, 1: MAC1);
#define EPHY_RETRY_COUNT 100000
static void mac0_clk_en(phy_interface_t phy_intf)
{

	UINT reg;

	reg = readl(IOADDR_CG_REG_BASE + 0x0);
	reg |= (1 << 7); // ENABLE PLL7
	writel(reg, IOADDR_CG_REG_BASE + 0x0);

	udelay(10);

	reg = readl(IOADDR_CG_REG_BASE + 0x7C);
	reg |= (1 << 4); // ENABLE ETH0 Module CLK
	writel(reg, IOADDR_CG_REG_BASE + 0x7C);

	udelay(10);

	reg = readl(IOADDR_CG_REG_BASE + 0x9C);
	reg |= (1 << 12); // realse eth0 rst
	writel(reg, IOADDR_CG_REG_BASE + 0x9C);

	udelay(10);

	if (phy_intf == PHY_INTERFACE_MODE_SGMII ||
			phy_intf == PHY_INTERFACE_MODE_2500BASEX) {
		reg = readl(IOADDR_CG_REG_BASE + 0x0);
		reg |= (1 << 18); // ENABLE PLL18
		writel(reg, IOADDR_CG_REG_BASE + 0x0);

		udelay(10);

		reg = readl(IOADDR_CG_REG_BASE + 0x10);
		reg &= ~(0x1 << 13); // COMBO_PHY2_CLKSEL to 0x0 pll18
		writel(reg, IOADDR_CG_REG_BASE + 0x10);

		udelay(10);

		reg = readl(IOADDR_CG_REG_BASE + 0x7C);
		reg |= (1 << 6); // ENABLE ETH_XPCS_MAC_TX_CLKEN
		writel(reg, IOADDR_CG_REG_BASE + 0x7C);

		udelay(10);

		reg = readl(IOADDR_CG_REG_BASE + 0x7C);
		reg |= (1 << 7); // ENABLE ETH_XPCS_MAC_TX_CLKEN
		writel(reg, IOADDR_CG_REG_BASE + 0x7C);

		udelay(10);
	}

}

static void mac1_clk_en(phy_interface_t phy_intf)
{
	UINT reg;

	reg = readl(IOADDR_CG_REG_BASE + 0x0);
	reg |= (1 << 7); // ENABLE PLL7
	writel(reg, IOADDR_CG_REG_BASE + 0x0);

	udelay(10);

	reg = readl(IOADDR_CG_REG_BASE + 0x7C);
	reg |= (1 << 8); // ENABLE ETH0 Module CLK
	writel(reg, IOADDR_CG_REG_BASE + 0x7C);

	udelay(10);

	reg = readl(IOADDR_CG_REG_BASE + 0x9C);
	reg |= (1 << 15); // realse eth0 rst
	writel(reg, IOADDR_CG_REG_BASE + 0x9C);

	udelay(10);

	if (phy_intf == PHY_INTERFACE_MODE_SGMII ||
			phy_intf == PHY_INTERFACE_MODE_2500BASEX) {
		reg = readl(IOADDR_CG_REG_BASE + 0x0);
		reg |= (1 << 18); // ENABLE PLL18
		writel(reg, IOADDR_CG_REG_BASE + 0x0);

		udelay(10);

		reg = readl(IOADDR_CG_REG_BASE + 0x10);
		reg &= ~(0x1 << 14); // COMBO_PHY3_CLKSEL to 0x0 pll18
		writel(reg, IOADDR_CG_REG_BASE + 0x10);

		udelay(10);

		reg = readl(IOADDR_CG_REG_BASE + 0x7C);
		reg |= (1 << 10); // ENABLE ETH2_XPCS_MAC_TX_CLKEN
		writel(reg, IOADDR_CG_REG_BASE + 0x7C);

		udelay(10);

		reg = readl(IOADDR_CG_REG_BASE + 0x7C);
		reg |= (1 << 11); // ENABLE ETH2_XPCS_MAC_TX_CLKEN
		writel(reg, IOADDR_CG_REG_BASE + 0x7C);

		udelay(10);
	}

}

static void extphy0_clk_en(phy_interface_t phy_intf, int phy_clk, int ref_clk_out, int pin_num)
{
	UINT reg;

	switch (phy_intf) {
	case PHY_INTERFACE_MODE_RMII:
		reg = readl(IOADDR_ETH_REG_BASE + 0x4014);
		// pre-assume refclk: phy--> mac

		reg |= 1 << 0;
		reg &= ~(1 << 4);
		reg |= (1 << 5);

		reg &= ~(1 << 2); // refclk referenced from pad
		reg &= ~(0x3 << 30); // TXD_SRC: RMII
		if (ref_clk_out) {
			reg &= ~(1 << 0);
			reg |= 1 << 4;
		}
		writel(reg, IOADDR_ETH_REG_BASE + 0x4014);

		reg = readl(IOADDR_ETH_REG_BASE + 0x4004);
		reg |= (1 << 4); // select external phy
		writel(reg, IOADDR_ETH_REG_BASE + 0x4004);

		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
		reg = readl(IOADDR_ETH_REG_BASE + 0x4014);
		reg &= (0x3 << 30);
		reg |= (0x1 << 30); // TXD_SRC: RGMII
		reg |= (3 << 4); // select external phy
		writel(reg, IOADDR_ETH_REG_BASE + 0x4014);

		reg = readl(IOADDR_ETH_REG_BASE + 0x4004);
		reg |= (1 << 4); // select external phy

		writel(reg, IOADDR_ETH_REG_BASE + 0x4004);
		writel(0x1FF, IOADDR_ETH_REG_BASE + 0x400C);

		break;
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_2500BASEX:
		reg = readl(IOADDR_ETH_REG_BASE + 0x4004);
		reg |= (1 << 4); // select external phy
		writel(reg, IOADDR_ETH_REG_BASE + 0x4004);

		break;
	default:
		/*      reg = readl(IOADDR_ETH_REG_BASE + 0x4004);
		        reg &= ~(1 << 8); // select embedded phy
		        writel(reg, IOADDR_ETH_REG_BASE + 0x4004);

		        eth_phy_poweron(IOADDR_ETH_REG_BASE);
		        phy_set_mdio_addr(IOADDR_ETH_REG_BASE, PHYAD);
		        // phy setting
		#ifndef CONFIG_NVT_FPGA_EMULATION
		        set_eth_phy_trim(IOADDR_ETH_REG_BASE);
		#endif
		        phy_sw_reset_enable(IOADDR_ETH_REG_BASE);
		        set_break_link_timer(IOADDR_ETH_REG_BASE);
		        set_led_blinking(IOADDR_ETH_REG_BASE);
		        mdelay(10);
		        phy_sw_reset_disable(IOADDR_ETH_REG_BASE);
		*/
		break;
	}

	if (phy_clk > 0) {
		reg = readl(IOADDR_CG_REG_BASE + 0x7C); // CG EXT PHY EN
		reg |= (1 << 5);
		writel(reg, IOADDR_CG_REG_BASE + 0x7C);

		reg = smc_readl(IOADDR_TOP_REG_BASE + 0xC); // TOP EXT PHY EN
		reg |= (1 << 8);
		smc_writel(reg, IOADDR_TOP_REG_BASE + 0xC);

		reg = smc_readl(IOADDR_TOP_REG_BASE + 0xB0); // TOP EXT PHY GPIO FUNCTION NORMAL
		reg &= ~(1 << 0);
		smc_writel(reg, IOADDR_TOP_REG_BASE + 0xB0);
	}

	if (pin_num == 1) {
		// reset external phy
		gpio_request(E_GPIO(1), "PHY0_RST");
		gpio_direction_output(E_GPIO(1), 0);
		mdelay(10); // assert low at least 10ms
		gpio_set_value(E_GPIO(1), 1);
		mdelay(60); // wait 50ms before access phy register
		gpio_free(E_GPIO(1));
	} else if (pin_num == 2) {
		// reset external phy
		gpio_request(E_GPIO(1), "PHY0_RST");
		gpio_direction_output(E_GPIO(1), 0);
		mdelay(10); // assert low at least 10ms
		gpio_set_value(E_GPIO(1), 1);
		mdelay(60); // wait 50ms before access phy register
		gpio_free(E_GPIO(1));
	}

}

static void extphy1_clk_en(phy_interface_t phy_intf, int phy_clk, int ref_clk_out, int pin_num)
{
	UINT reg;

	switch (phy_intf) {
	case PHY_INTERFACE_MODE_RMII:
		reg = readl(IOADDR_ETH1_REG_BASE + 0x4014);
		// pre-assume refclk: phy--> mac

		reg |= 1 << 0;
		reg &= ~(1 << 4);
		reg |= (1 << 5);

		reg &= ~(1 << 2); // refclk referenced from pad
		reg &= ~(0x3 << 30); // TXD_SRC: RMII
		if (ref_clk_out) {
			reg &= ~(1 << 0);
			reg |= 1 << 4;
		}
		writel(reg, IOADDR_ETH1_REG_BASE + 0x4014);

		reg = readl(IOADDR_ETH1_REG_BASE + 0x4004);
		reg |= (1 << 4); // select external phy
		writel(reg, IOADDR_ETH1_REG_BASE + 0x4004);

		break;
	// fall through
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
		reg = readl(IOADDR_ETH1_REG_BASE + 0x4014);
		reg &= (0x3 << 30);
		reg |= (0x1 << 30); // TXD_SRC: RGMII
		reg |= (3 << 4); // select external phy
		writel(reg, IOADDR_ETH1_REG_BASE + 0x4014);

		reg = readl(IOADDR_ETH1_REG_BASE + 0x4004);
		reg |= (1 << 4); // select external phy

		writel(reg, IOADDR_ETH1_REG_BASE + 0x4004);
		writel(0x1FF, IOADDR_ETH1_REG_BASE + 0x400C);

		break;
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_2500BASEX:
		reg = readl(IOADDR_ETH1_REG_BASE + 0x4004);
		reg |= (1 << 4); // select external phy
		writel(reg, IOADDR_ETH1_REG_BASE + 0x4004);

		break;

	default:
		/*
		reg = readl(IOADDR_ETH_REG_BASE + 0x3004);
		reg &= ~(1<<8);// select embedded phy
		writel(reg, IOADDR_ETH_REG_BASE + 0x3004);

		eth_phy_poweron(IOADDR_ETH_REG_BASE);
		        // phy setting
		    set_eth_phy_trim(IOADDR_ETH_REG_BASE);
		        phy_sw_reset_enable(IOADDR_ETH_REG_BASE);
		        set_break_link_timer(IOADDR_ETH_REG_BASE);
		        set_led_blinking(IOADDR_ETH_REG_BASE);
		        mdelay(10);
		        phy_sw_reset_disable(IOADDR_ETH_REG_BASE);
		*/
		break;
	}

	if (phy_clk > 0) {
		reg = readl(IOADDR_CG_REG_BASE + 0x7C); // CG EXT PHY EN
		reg |= (1 << 9);
		writel(reg, IOADDR_CG_REG_BASE + 0x7C);

		reg = smc_readl(IOADDR_TOP_REG_BASE + 0xC); // TOP EXT PHY EN
		reg |= (1 << 16);
		smc_writel(reg, IOADDR_TOP_REG_BASE + 0xC);

		reg = smc_readl(IOADDR_TOP_REG_BASE + 0xB0); // TOP EXT PHY GPIO FUNCTION NORMAL
		reg &= ~(1 << 16);
		smc_writel(reg, IOADDR_TOP_REG_BASE + 0xB0);
	}

	if (pin_num == 1) {
		// reset external phy
		gpio_request(E_GPIO(17), "PHY0_RST");
		gpio_direction_output(E_GPIO(17), 0);
		mdelay(10); // assert low at least 10ms
		gpio_set_value(E_GPIO(17), 1);
		mdelay(60); // wait 50ms before access phy register
		gpio_free(E_GPIO(17));
	} else if (pin_num == 2) {
		// reset external phy
		gpio_request(E_GPIO(17), "PHY0_RST");
		gpio_direction_output(E_GPIO(17), 0);
		mdelay(10); // assert low at least 10ms
		gpio_set_value(E_GPIO(17), 1);
		mdelay(60); // wait 50ms before access phy register
		gpio_free(E_GPIO(17));
	}

}

static void mac0_pinmux_en(phy_interface_t phy_intf, int pin_num)
{
	UINT reg;

	switch (phy_intf) {
	case PHY_INTERFACE_MODE_RMII:
		// set pad driving
		reg = readl(IOADDR_PAD_REG_BASE + 0x140);
		// PHYCLK
		reg &= ~(0xf << 0);
		reg |= (0x8 << 0);
		writel(reg, IOADDR_PAD_REG_BASE + 0x140);

		reg = readl(IOADDR_PAD_REG_BASE + 0x144);

		// TXCTL TXD0 TXD1
		reg &= ~((0xf << 12) | (0xf << 8) | (0xf << 4));
		reg |= (0x8 << 12) | (0x8 << 8) | (0x8 << 4);
		writel(reg, IOADDR_PAD_REG_BASE + 0x144);

		reg = readl(IOADDR_PAD_REG_BASE + 0x140);
		// RX CLK
		reg &= ~(0xf << 28);
		reg |= (0x8 << 28);
		writel(reg, IOADDR_PAD_REG_BASE + 0x140);

		// RMII mode
		reg = smc_readl(IOADDR_TOP_REG_BASE + 0x0C);
		reg &= ~(0xf << 0);
		smc_writel(reg | (0x2 << 0), IOADDR_TOP_REG_BASE + 0x0C);

		// GPIO EN
		reg = smc_readl(IOADDR_TOP_REG_BASE + 0xB0);
		reg &= ~(0xCE9C);
		smc_writel(reg, IOADDR_TOP_REG_BASE + 0xB0);

		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
		// set pad driving
		reg = readl(IOADDR_PAD_REG_BASE + 0x140);
		// PHYCLK
		reg &= ~(0xf << 0);
		reg |= (0x8 << 0);
		writel(reg, IOADDR_PAD_REG_BASE + 0x140);

		reg = readl(IOADDR_PAD_REG_BASE + 0x144);
		// TXCLK, TXCTL, TXD0..TXD3
		reg &= ~((0xf << 20) | (0xf << 16) | (0xf << 12) | (0xf << 8) | (0xf << 4) | (0xf << 0));
		reg |= (0x8 << 20) | (0x8 << 16) | (0x8 << 12) | (0x8 << 8) | (0x8 << 4) | (0x8 << 0);
		writel(reg, IOADDR_PAD_REG_BASE + 0x144);

		// RGMII mode
		reg = smc_readl(IOADDR_TOP_REG_BASE + 0x0C);
		reg &= ~(0xf << 0);
		smc_writel(reg | (0x1 << 0), IOADDR_TOP_REG_BASE + 0x0C);

		// GPIO EN
		reg = smc_readl(IOADDR_TOP_REG_BASE + 0xB0);
		reg &= ~(0xFFFC);
		smc_writel(reg, IOADDR_TOP_REG_BASE + 0xB0);

		break;
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_2500BASEX:
		// SGMII mode
		reg = smc_readl(IOADDR_TOP_REG_BASE + 0x0C);
		reg &= ~(0xf << 0);
		smc_writel(reg | (0x3 << 0), IOADDR_TOP_REG_BASE + 0x0C);

		// GPIO EN
		reg = smc_readl(IOADDR_TOP_REG_BASE + 0xB0);
		reg &= ~(0xC000);
		smc_writel(reg, IOADDR_TOP_REG_BASE + 0xB0);

		break;
	default: // embedded phy (MII)

		/*      reg = smc_readl(IOADDR_TOP_REG_BASE + 0xC8);
		        reg &= ~(0xF<<4);
		        reg |= 1<<0;    // connect EMB phy to MAC0
		        smc_writel(reg, IOADDR_TOP_REG_BASE + 0xC8);

		        printf("mac0 no sup embd phy");
		*/
		break;
	}
}

static void mac1_pinmux_en(phy_interface_t phy_intf, int pin_num)
{
	UINT reg;

	switch (phy_intf) {
	case PHY_INTERFACE_MODE_RMII:
		// set pad driving
		reg = readl(IOADDR_PAD_REG_BASE + 0x148);
		// PHYCLK
		reg &= ~(0xf << 0);
		reg |= (0x8 << 0);
		writel(reg, IOADDR_PAD_REG_BASE + 0x148);

		reg = readl(IOADDR_PAD_REG_BASE + 0x14C);

		// TXCTL TXD0 TXD1
		reg &= ~((0xf << 12) | (0xf << 8) | (0xf << 4));
		reg |= (0x8 << 12) | (0x8 << 8) | (0x8 << 4);
		writel(reg, IOADDR_PAD_REG_BASE + 0x14C);

		reg = readl(IOADDR_PAD_REG_BASE + 0x148);
		// RX CLK
		reg &= ~(0xf << 28);
		reg |= (0x8 << 28);
		writel(reg, IOADDR_PAD_REG_BASE + 0x148);

		// RMII mode
		reg = smc_readl(IOADDR_TOP_REG_BASE + 0x0C);
		reg &= ~(0xf << 4);
		smc_writel(reg | (0x2 << 4), IOADDR_TOP_REG_BASE + 0x0C);

		// GPIO EN
		reg = smc_readl(IOADDR_TOP_REG_BASE + 0xB0);
		reg &= ~(0xCE9C0000);
		smc_writel(reg, IOADDR_TOP_REG_BASE + 0xB0);

		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
		// set pad driving
		reg = readl(IOADDR_PAD_REG_BASE + 0x148);
		// PHYCLK
		reg &= ~(0xf << 0);
		reg |= (0x8 << 0);
		writel(reg, IOADDR_PAD_REG_BASE + 0x148);

		reg = readl(IOADDR_PAD_REG_BASE + 0x14C);
		// TXCLK, TXCTL, TXD0..TXD3
		reg &= ~((0xf << 20) | (0xf << 16) | (0xf << 12) | (0xf << 8) | (0xf << 4) | (0xf << 0));
		reg |= (0x8 << 20) | (0x8 << 16) | (0x8 << 12) | (0x8 << 8) | (0x8 << 4) | (0x8 << 0);
		writel(reg, IOADDR_PAD_REG_BASE + 0x14C);


		// RGMII mode
		reg = smc_readl(IOADDR_TOP_REG_BASE + 0x0C);
		reg &= ~(0xf << 4);
		smc_writel(reg | (0x1 << 4), IOADDR_TOP_REG_BASE + 0x0C);

		// GPIO EN
		reg = smc_readl(IOADDR_TOP_REG_BASE + 0xB0);
		reg &= ~(0xFFFC0000);
		smc_writel(reg, IOADDR_TOP_REG_BASE + 0xB0);

		break;
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_2500BASEX:
		// SGMII mode
		reg = smc_readl(IOADDR_TOP_REG_BASE + 0x0C);
		reg &= ~(0xf << 4);
		smc_writel(reg | (0x3 << 4), IOADDR_TOP_REG_BASE + 0x0C);

		// GPIO EN
		reg = smc_readl(IOADDR_TOP_REG_BASE + 0xB0);
		reg &= ~(0xC0000000);
		smc_writel(reg, IOADDR_TOP_REG_BASE + 0xB0);

		break;

	default: // embedded phy (MII)
		/*
		    reg = smc_readl(IOADDR_TOP_REG_BASE + 0xC8);
		    reg &= ~(0xF<<8);
		    reg &= ~(1<<0); // connect EMB phy to MAC1
		    smc_writel(reg, IOADDR_TOP_REG_BASE + 0xC8);
		*/
		printf("mac1 no sup embd phy");
		break;
	}
}

struct NVT_PLAT_INFO {
	unsigned long   base_pa;        // base physical address
	char    *dtb_name;
	void (*mac_pinmux_en)(phy_interface_t phy_intf, int pin_num);
	void (*mac_clk_en)(phy_interface_t phy_intf);
	void (*extphy_clk_en)(phy_interface_t phy_intf, int phy_clk, int ref_clk_out, int pin_num);
	char    *rmii_refclk_i_name;    // name of rmii_refclk_i
	char    *rxclk_i_name;
	char    *txclk_i_name;
	char    *ext_phy_clk_name;

	UINT    pinmux_ext_phy;

	UINT    pinmux_sgmii;

	UINT    pinmux_rgmii;
	UINT    pinmux_rgmii_2;

	UINT    pinmux_rmii;
	UINT    pinmux_rmii_2;

	UINT    pinmux_emb;

	UINT    gpio_reset;
	UINT    gpio_reset_2;
};

static const struct NVT_PLAT_INFO v_nvt_plat_info[2] = {
	// MAC0
	{
		IOADDR_ETH_REG_BASE,
		"eth0@4,f0510000",
		mac0_pinmux_en,
		mac0_clk_en,
		extphy0_clk_en,
		"rmii0_refclk_i",
		"eth0_rxclk_i",
		"eth0_txclk_i",
		"eth0_extphy_clk",

		PIN_ETH_CFG_ETH_EXTPHYCLK,

		PIN_ETH_CFG_ETH_SGMII_1,

		PIN_ETH_CFG_ETH_RGMII_1,
		PIN_ETH_CFG_NONE,

		PIN_ETH_CFG_ETH_RMII_1,
		PIN_ETH_CFG_NONE,

		PIN_ETH_CFG_NONE,

		E_GPIO(1),
		0,
	},
	// MAC1
	{
		IOADDR_ETH1_REG_BASE,
		"eth1@4,f0520000",
		mac1_pinmux_en,
		mac1_clk_en,
		extphy1_clk_en,
		"rmii1_refclk_i",
		"eth1_rxclk_i",
		"eth1_txclk_i",
		"eth1_extphy_clk",

		PIN_ETH_CFG_ETH2_EXTPHYCLK,

		PIN_ETH_CFG_ETH2_SGMII_1,

		PIN_ETH_CFG_ETH2_RGMII_1,
		PIN_ETH_CFG_NONE,

		PIN_ETH_CFG_ETH2_RMII_1,
		PIN_ETH_CFG_NONE,

		PIN_ETH_CFG_NONE,

		E_GPIO(17),
		0,
	},
};
/*
static int dwxgmac_novatek_do_setphy(struct udevice *dev, u32 modereg)
{
    struct xgmac_priv *xgmac = dev_get_priv(dev);
    int ret;

    u32 modemask = SYSMGR_EMACGRP_CTRL_PHYSEL_MASK <<
               xgmac->syscon_phy_regshift;

    if (!(IS_ENABLED(CONFIG_SPL_BUILD)) && IS_ENABLED(CONFIG_SPL_ATF)) {
        u32 index = ((u64)xgmac->syscon_phy - socfpga_get_sysmgr_addr() -
                 SYSMGR_SOC64_EMAC0) >> 2;

        u32 id = SOCFPGA_SECURE_REG_SYSMGR_SOC64_EMAC0 + index;

        ret = socfpga_secure_reg_update32(id,
                          modemask,
                          modereg <<
                          xgmac->syscon_phy_regshift);
        if (ret) {
            dev_err(dev, "Failed to set PHY register via SMC call\n");
            return ret;
        }

    } else {
        clrsetbits_le32(xgmac->phy, modemask, modereg);
    }

    return 0;
}
*/
static int xgmac_probe_resources_novatek(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	/*  struct regmap *reg_map;
	    struct ofnode_phandle_args args;
	    void *range;
	    phy_interface_t interface;
	    int ret;
	    u32 modereg;
	*/
	int nodeoffset;
	u32 *cell = NULL;
	char path[20] = {0};
	int len;
	int pin_num = 0;
//	u32 phy_intf, led_intf; no embd phy so do not control led.
	u32 phy_intf;
	u32 phy_clk = 0;
	u32 ref_clk_out = 0;
	const char *phy_mode;

	if (xgmac->regs == v_nvt_plat_info[0].base_pa) {
		xgmac->eth_idx = 0;
	} else {
		xgmac->eth_idx = 1;
	}

	printf("%s: get IO MEM 0x%llx\r\n", __func__, xgmac->regs);

	sprintf(path, "/top@4,%x/eth", ((u32)(REG32_MASK & IOADDR_TOP_REG_BASE)));

	nodeoffset = fdt_path_offset((const void *)nvt_fdt_buffer, path);
	if (nodeoffset < 0) {
		printf("%s(%d) nodeoffset < 0\n", __func__, __LINE__);
		printf("%s: path %s not found\n", __func__, path);
		return -1;
	}

	cell = (u32 *)fdt_getprop((const void *)nvt_fdt_buffer, nodeoffset, "pinmux", &len);

	if (!cell) {
		printf("%s: pinmux not found\n", __func__);
		return -1;
	}

	if (len == 0) {
		printf("%s(%d) len = 0\n", __func__, __LINE__);
		return -1;
	}
	phy_intf = __be32_to_cpu(cell[0]);

	sprintf(path, "/%s", v_nvt_plat_info[xgmac->eth_idx].dtb_name);
	nodeoffset = fdt_path_offset((const void *)nvt_fdt_buffer, path);
	if (nodeoffset < 0) {
		printf("%s(%d) nodeoffset < 0\n", __func__, __LINE__);
		printf("%s: path %s not found\n", __func__, path);
		return -1;
	}

	cell = (u32 *)fdt_getprop((const void *)nvt_fdt_buffer, nodeoffset, "sp-clk", &len);

	if (!cell) {
		printf("%s: sp-clk not found\n", __func__);
		return -1;
	}

	if (len != 0) {
		phy_clk = __be32_to_cpu(cell[0]);
	}

	cell = (u32 *)fdt_getprop((const void *)nvt_fdt_buffer, nodeoffset, "ref-clk-out", &len);

	if (!cell) {
		printf("%s: ref-clk-out not found\n", __func__);
		return -1;
	}

	if (len != 0) {
		ref_clk_out = __be32_to_cpu(cell[0]);
	}
	printf("%s: ref-clk-out %d\r\n", __func__, ref_clk_out);

	cell = (u32 *)fdt_getprop((const void *)nvt_fdt_buffer, nodeoffset, "snps,phy-addr", &len);

	if (!cell) {
		printf("%s: snps,phy-addr not found\n", __func__);
	} else {
		phyaddr = __be32_to_cpu(cell[0]);
		printf("%s: snps,phy-addr 0x%x\r\n", __func__, phyaddr);
	}
	ETH_PHY_INTF = -1;

	phy_mode = fdt_getprop((const void *)nvt_fdt_buffer, nodeoffset, "phy-mode", NULL);
	if (phy_mode) {
		ETH_PHY_INTF = phy_get_interface_by_name(phy_mode);
		printf("%s: phy-mode: %s\r\n", __func__, phy_mode);
	}
	if (ETH_PHY_INTF == -1) {
		debug("%s: Invalid PHY interface '%s'\n", __func__, phy_mode);
		return -EINVAL;
	}

	cell = (u32 *)fdt_getprop((const void *)nvt_fdt_buffer, nodeoffset, "phy-mode-oc", &len);
	if (!cell) {
		printf("%s: phy-mode-oc not found \r\n", __func__);
	} else {
		xgmac->phy_mode_oc = __be32_to_cpu(cell[0]);
		printf("%s: phy-mode-oc %d \r\n", __func__, xgmac->phy_mode_oc);
	}

	if (phy_intf & v_nvt_plat_info[xgmac->eth_idx].pinmux_ext_phy) {
		phy_clk = 1;
	}

	v_nvt_plat_info[xgmac->eth_idx].mac_pinmux_en(ETH_PHY_INTF, pin_num);
	v_nvt_plat_info[xgmac->eth_idx].mac_clk_en(ETH_PHY_INTF);    // also assert async reset
	v_nvt_plat_info[xgmac->eth_idx].extphy_clk_en(ETH_PHY_INTF, phy_clk, ref_clk_out, pin_num);

	return 0;
}

static int xgmac_get_enetaddr_novatek(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_plat(dev);
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	u32 hi_addr, lo_addr;

	debug("%s(dev=%p):\n", __func__, dev);

	/* Read the MAC Address from the hardawre */
	hi_addr = readl(&xgmac->mac_regs->address0_high);
	lo_addr = readl(&xgmac->mac_regs->address0_low);

	pdata->enetaddr[0] = lo_addr & 0xff;
	pdata->enetaddr[1] = (lo_addr >> 8) & 0xff;
	pdata->enetaddr[2] = (lo_addr >> 16) & 0xff;
	pdata->enetaddr[3] = (lo_addr >> 24) & 0xff;
	pdata->enetaddr[4] = hi_addr & 0xff;
	pdata->enetaddr[5] = (hi_addr >> 8) & 0xff;

	return !is_valid_ethaddr(pdata->enetaddr);
}

static int xgmac_start_resets_novatek(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	int ret;

	debug("%s(dev=%p):\n", __func__, dev);

	ret = reset_assert_bulk(&xgmac->reset_bulk);
	if (ret < 0) {
		pr_err("xgmac reset assert failed: %d", ret);
		return ret;
	}

	udelay(2);

	ret = reset_deassert_bulk(&xgmac->reset_bulk);
	if (ret < 0) {
		pr_err("xgmac reset de-assert failed: %d", ret);
		return ret;
	}

	return 0;
}

static struct xgmac_ops xgmac_novatek_ops = {
	.xgmac_inval_desc = xgmac_inval_desc_generic,
	.xgmac_flush_desc = xgmac_flush_desc_generic,
	.xgmac_inval_buffer = xgmac_inval_buffer_generic,
	.xgmac_flush_buffer = xgmac_flush_buffer_generic,
	.xgmac_probe_resources = xgmac_probe_resources_novatek,
	.xgmac_remove_resources = xgmac_null_ops,
	.xgmac_stop_resets = xgmac_null_ops,
	.xgmac_start_resets = xgmac_start_resets_novatek,
	.xgmac_stop_clks = xgmac_null_ops,
	.xgmac_start_clks = xgmac_null_ops,
	.xgmac_calibrate_pads = xgmac_null_ops,
	.xgmac_disable_calibration = xgmac_null_ops,
	.xgmac_get_enetaddr = xgmac_get_enetaddr_novatek,
};

struct xgmac_config __maybe_unused xgmac_novatek_config = {
	.reg_access_always_ok = false,
	.swr_wait = 5000,
	.config_mac = XGMAC_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB,
	.config_mac_mdio = XGMAC_MAC_MDIO_ADDRESS_CR_100_150,
	.axi_bus_width = XGMAC_AXI_WIDTH_128,
	.interface = 0,
	.ops = &xgmac_novatek_ops
};

static void *xgmac_alloc_descs(struct xgmac_priv *xgmac, unsigned int num)
{
#ifdef CONFIG_SYS_NONCACHED_MEMORY
	return (void *)noncached_alloc(XGMAC_DESCRIPTORS_SIZE,
								   XGMAC_BUFFER_ALIGN);
#else
	return memalign(ARCH_DMA_MINALIGN, num * xgmac->desc_size);
#endif
}

static void xgmac_free_descs(void *descs)
{
#ifdef CONFIG_SYS_NONCACHED_MEMORY
#else
	free(descs);
#endif
}

static struct xgmac_desc *xgmac_get_desc(struct xgmac_priv *xgmac,
		unsigned int num, bool rx)
{
	return (rx ? xgmac->rx_descs : xgmac->tx_descs) +
		   (num * xgmac->desc_size);
}

void xgmac_inval_desc_generic(void *desc)
{
#ifndef CONFIG_SYS_NONCACHED_MEMORY
	unsigned long start;
	unsigned long end;

	if (!desc) {
		pr_err("%s invalid input buffer\n", __func__);
		return;
	}

	start = (unsigned long)desc & ~(ARCH_DMA_MINALIGN - 1);
	end = ALIGN(start + sizeof(struct xgmac_desc),
				ARCH_DMA_MINALIGN);

	invalidate_dcache_range(start, end);
#endif
}

void xgmac_flush_desc_generic(void *desc)
{
#ifndef CONFIG_SYS_NONCACHED_MEMORY
	unsigned long start;
	unsigned long end;

	if (!desc) {
		pr_err("%s invalid input buffer\n", __func__);
		return;
	}

	start = (unsigned long)desc & ~(ARCH_DMA_MINALIGN - 1);
	end = ALIGN(start + sizeof(struct xgmac_desc),
				ARCH_DMA_MINALIGN);

	flush_dcache_range(start, end);
#endif
}

void xgmac_inval_buffer_generic(void *buf, size_t size)
{
	unsigned long start;
	unsigned long end;

	if (!buf) {
		pr_err("%s invalid input buffer\n", __func__);
		return;
	}

	start = (unsigned long)buf & ~(ARCH_DMA_MINALIGN - 1);
	end = ALIGN((unsigned long)buf + size,
				ARCH_DMA_MINALIGN);

	invalidate_dcache_range(start, end);
}

void xgmac_flush_buffer_generic(void *buf, size_t size)
{
	unsigned long start;
	unsigned long end;

	if (!buf) {
		pr_err("%s invalid input buffer\n", __func__);
		return;
	}

	start = (unsigned long)buf & ~(ARCH_DMA_MINALIGN - 1);
	end = ALIGN((unsigned long)buf + size,
				ARCH_DMA_MINALIGN);

	flush_dcache_range(start, end);
}

static int xgmac_mdio_wait_idle(struct xgmac_priv *xgmac)
{
	int i = 0;
	u32 reg;

	do {
		reg = readl(&xgmac->mac_regs->mdio_data);
		if (!(reg & XGMAC_MAC_MDIO_ADDRESS_SBUSY)) {
			return 0;
		}
		i++;
	} while (i < XGMAC_TIMEOUT_100MS);

	return -1;
}

static int xgmac_mdio_read(struct mii_dev *bus, int mdio_addr, int mdio_devad,
						   int mdio_reg)
{
	struct xgmac_priv *xgmac = bus->priv;
	u32 val;
	u32 hw_addr;
	int ret;

	debug("%s(dev=%p, addr=0x%x, reg=%d):\n", __func__, xgmac->dev, mdio_addr,
		  mdio_reg);

	ret = xgmac_mdio_wait_idle(xgmac);
	if (ret) {
		pr_err("MDIO not idle at entry: %d\n", ret);
		return ret;
	}

	/* Set clause 22 format */
	val = BIT(mdio_addr);
	writel(val, &xgmac->mac_regs->mdio_clause_22_port);

	hw_addr = (mdio_addr << XGMAC_MAC_MDIO_ADDRESS_PA_SHIFT) |
			  (mdio_reg & XGMAC_MAC_MDIO_REG_ADDR_C22P_MASK);

	val = xgmac->config->config_mac_mdio <<
		  XGMAC_MAC_MDIO_ADDRESS_CR_SHIFT;

	val |= XGMAC_MAC_MDIO_ADDRESS_SADDR |
		   XGMAC_MDIO_SINGLE_CMD_ADDR_CMD_READ |
		   XGMAC_MAC_MDIO_ADDRESS_SBUSY;

	ret = xgmac_mdio_wait_idle(xgmac);
	if (ret) {
		pr_err("MDIO not idle at entry: %d\n", ret);
		return ret;
	}

	writel(hw_addr, &xgmac->mac_regs->mdio_address);
	writel(val, &xgmac->mac_regs->mdio_data);

	ret = xgmac_mdio_wait_idle(xgmac);
	if (ret) {
		pr_err("MDIO read didn't complete: %d\n", ret);
		return ret;
	}

	val = readl(&xgmac->mac_regs->mdio_data);
	val &= XGMAC_MAC_MDIO_DATA_GD_MASK;

	debug("%s: val=0x%x\n", __func__, val);

	return val;
}

static int xgmac_mdio_write(struct mii_dev *bus, int mdio_addr, int mdio_devad,
							int mdio_reg, u16 mdio_val)
{
	struct xgmac_priv *xgmac = bus->priv;
	u32 val;
	u32 hw_addr;
	int ret;

	debug("%s(dev=%p, addr=0x%x, reg=%d, val=0x%x):\n", __func__, xgmac->dev,
		  mdio_addr, mdio_reg, mdio_val);

	ret = xgmac_mdio_wait_idle(xgmac);
	if (ret) {
		pr_err("MDIO not idle at entry: %d\n", ret);
		return ret;
	}

	/* Set clause 22 format */
	val = BIT(mdio_addr);
	writel(val, &xgmac->mac_regs->mdio_clause_22_port);

	hw_addr = (mdio_addr << XGMAC_MAC_MDIO_ADDRESS_PA_SHIFT) |
			  (mdio_reg & XGMAC_MAC_MDIO_REG_ADDR_C22P_MASK);

	hw_addr |= (mdio_reg >> XGMAC_MAC_MDIO_ADDRESS_PA_SHIFT) <<
			   XGMAC_MAC_MDIO_ADDRESS_DA_SHIFT;

	val = (xgmac->config->config_mac_mdio <<
		   XGMAC_MAC_MDIO_ADDRESS_CR_SHIFT);

	val |= XGMAC_MAC_MDIO_ADDRESS_SADDR |
		   mdio_val | XGMAC_MDIO_SINGLE_CMD_ADDR_CMD_WRITE |
		   XGMAC_MAC_MDIO_ADDRESS_SBUSY;

	ret = xgmac_mdio_wait_idle(xgmac);
	if (ret) {
		pr_err("MDIO not idle at entry: %d\n", ret);
		return ret;
	}

	writel(hw_addr, &xgmac->mac_regs->mdio_address);
	writel(val, &xgmac->mac_regs->mdio_data);

	ret = xgmac_mdio_wait_idle(xgmac);
	if (ret) {
		pr_err("MDIO write didn't complete: %d\n", ret);
		return ret;
	}

	return 0;
}

static int xgmac_set_full_duplex(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);

	debug("%s(dev=%p):\n", __func__, dev);

	clrbits_le32(&xgmac->mac_regs->mac_extended_conf, XGMAC_MAC_EXT_CONF_HD);

	return 0;
}

static int xgmac_set_half_duplex(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);

	debug("%s(dev=%p):\n", __func__, dev);

	setbits_le32(&xgmac->mac_regs->mac_extended_conf, XGMAC_MAC_EXT_CONF_HD);

	/* WAR: Flush TX queue when switching to half-duplex */
	setbits_le32(&xgmac->mtl_regs->txq0_operation_mode,
				 XGMAC_MTL_TXQ0_OPERATION_MODE_FTQ);

	return 0;
}
static int xgmac_set_xgmii_speed(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	u32 val;

	debug("%s(dev=%p):\n", __func__, dev);

	val = XGMAC_MAC_CONF_SS_2_5G_GMII << XGMAC_MAC_CONF_SS_SHIFT;
	writel(val, &xgmac->mac_regs->tx_configuration);

	return 0;
}

static int xgmac_set_gmii_speed(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	u32 val;

	debug("%s(dev=%p):\n", __func__, dev);

	val = XGMAC_MAC_CONF_SS_1G_GMII << XGMAC_MAC_CONF_SS_SHIFT;
	writel(val, &xgmac->mac_regs->tx_configuration);

	return 0;
}

static int xgmac_set_mii_speed_100(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	u32 val;

	debug("%s(dev=%p):\n", __func__, dev);

	val = XGMAC_MAC_CONF_SS_100M_MII << XGMAC_MAC_CONF_SS_SHIFT;
	writel(val, &xgmac->mac_regs->tx_configuration);

	return 0;
}

static int xgmac_set_mii_speed_10(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	u32 val;

	debug("%s(dev=%p):\n", __func__, dev);

	val = XGMAC_MAC_CONF_SS_2_10M_MII << XGMAC_MAC_CONF_SS_SHIFT;
	writel(val, &xgmac->mac_regs->tx_configuration);

	return 0;
}

static int xgmac_adjust_link(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	int ret;
	bool en_calibration;

	debug("%s(dev=%p):\n", __func__, dev);

	if (xgmac->phy->duplex) {
		ret = xgmac_set_full_duplex(dev);
	} else {
		ret = xgmac_set_half_duplex(dev);
	}
	if (ret < 0) {
		pr_err("xgmac_set_*_duplex() failed: %d\n", ret);
		return ret;
	}

	switch (xgmac->phy->speed) {
	case SPEED_2500:
		en_calibration = true;
		ret = xgmac_set_xgmii_speed(dev);
		break;
	case SPEED_1000:
		en_calibration = true;
		ret = xgmac_set_gmii_speed(dev);
		break;
	case SPEED_100:
		en_calibration = true;
		ret = xgmac_set_mii_speed_100(dev);
		break;
	case SPEED_10:
		en_calibration = false;
		ret = xgmac_set_mii_speed_10(dev);
		break;
	default:
		pr_err("invalid speed %d\n", xgmac->phy->speed);
		return -EINVAL;
	}
	if (ret < 0) {
		pr_err("xgmac_set_*mii_speed*() failed: %d\n", ret);
		return ret;
	}

	if (en_calibration) {
		ret = xgmac->config->ops->xgmac_calibrate_pads(dev);
		if (ret < 0) {
			pr_err("xgmac_calibrate_pads() failed: %d\n",
				   ret);
			return ret;
		}
	} else {
		ret = xgmac->config->ops->xgmac_disable_calibration(dev);
		if (ret < 0) {
			pr_err("xgmac_disable_calibration() failed: %d\n",
				   ret);
			return ret;
		}
	}

	return 0;
}

static void nvt_eth_phy_masked_write(uintptr_t addr, UINT32 data, UINT32 start, UINT32 width) {
	const UINT32 MASK = (1<<width) - 1;
	UINT32 data_tmp;

	data_tmp = readl(addr);
//	printk(" read addr 0x%p: 0x%x\r\n",  addr, data_tmp);
	data_tmp = (data_tmp & (~(MASK << start))) | ((data & MASK) << start);
//	printk(" update addr 0x%p: 0x%x\r\n",  addr, data_tmp);
	writel(data_tmp, addr);
}

static void nvt_eth_phy_trim(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	u32 trim = 0, addr;
	int code;
	int is_found;
	u32 internal_12k_trim = 0x8, tx_trim = 0x6, rx_trim = 0xc;
	u32 eth_internal_12k_registor, eth_tx_trim, eth_rx_trim;
	uintptr_t serdes_phy_ba;

	if (xgmac->eth_idx == 0) {
		addr = EFUSE_TRIM_DATA_SATA3_SGMII_1_PRI;
		serdes_phy_ba = IOADDR_SERDES_PHY_REG_BASE;
	} else {
		addr = EFUSE_TRIM_DATA_SATA3_SGMII_2_PRI;
		serdes_phy_ba = IOADDR_SERDES_PHY1_REG_BASE;
	}

	code = otp_key_manager(EFUSE_ID_1, addr);
	if (code == -33) {
		pr_err("Get Serdes-phy trim fail, use deault trim\n");
	} else {
		is_found = extract_trim_valid(code, (u32 *) &trim);
		debug("eth%d Trim data read success addr[0x%02x] = 0x%04x\r\n", xgmac->eth_idx, (int)addr, (int)trim);
		if (is_found) {
			eth_internal_12k_registor = trim & 0x1F;
			eth_tx_trim = ((trim >> 5) & 0xf);
			eth_rx_trim = ((trim >> 9) & 0xf);

			if (eth_internal_12k_registor > 0x14 || eth_internal_12k_registor < 0x1) {
				pr_err("ETH 12K registor Trim data error [0x%02x] = 0x%04x", (int)addr, (int)eth_internal_12k_registor);
			} else {
				internal_12k_trim = eth_internal_12k_registor;
			}

			if (eth_tx_trim > 0xE || eth_tx_trim < 0x2) {
				pr_err("ETH tx Trim data error [0x%02x] = 0x%04x", (int)addr, (int)eth_tx_trim);
			} else {
				tx_trim = eth_tx_trim;
			}


			if (eth_rx_trim > 0xE || eth_rx_trim < 0x2) {
				pr_err("ETH tx Trim data error [0x%02x] = 0x%04x", (int)addr, (int)eth_rx_trim);
			} else {
				rx_trim = eth_rx_trim;
			}
		} else {
			pr_err("trim error [0x%02x] = 0x%04x", (int)addr, (int)trim);
		}
	}

	nvt_eth_phy_masked_write(serdes_phy_ba + 0x3fc, 0x0, 0, 8);
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x680, 0x3, 5, 2);
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x680, internal_12k_trim, 0, 5);

	//Tx trim value
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x684, 0x0, 0, 8);
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x68c, 0x60, 0, 8);
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x40, tx_trim, 4, 4);

	// RX trim value
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x3fc, 0x2, 0, 8);
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x0, rx_trim, 0, 4);
}

static int xgmac_serdes_xpcs_config(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	u32 reg;
	int i = 0;
	uintptr_t serdes_phy_ba, xpcs_ba;

	debug("%s, %d, speed = %d, duplex = %d, link = %d, idx = %d\n", __func__, __LINE__, xgmac->phy->speed, xgmac->phy->duplex, xgmac->phy->link, xgmac->eth_idx);

	if (xgmac->eth_idx == 0) {
		serdes_phy_ba = IOADDR_SERDES_PHY_REG_BASE;
		xpcs_ba = IOADDR_XPCS_REG_BASE;
		nvt_eth_phy_masked_write(IOADDR_CG_REG_BASE + 0xA8, 0x0, 2, 1);
		nvt_eth_phy_masked_write(IOADDR_CG_REG_BASE + 0xA8, 0x0, 3, 1);
		nvt_eth_phy_masked_write(IOADDR_CG_REG_BASE + 0xA8, 0x1, 3, 1);
	} else {
		serdes_phy_ba = IOADDR_SERDES_PHY1_REG_BASE;
		xpcs_ba = IOADDR_XPCS1_REG_BASE;

		i = 0;
		do {
			reg = readl(IOADDR_CG_REG_BASE + 0x4);
			if (reg & (0x1 << 18)) {
				break;
			} else if (++i >= EPHY_RETRY_COUNT) {
				pr_err("MPLL18 is not ready yet, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);

		nvt_eth_phy_masked_write(IOADDR_CG_REG_BASE + 0xA8, 0x0, 4, 1);
		nvt_eth_phy_masked_write(IOADDR_CG_REG_BASE + 0xA8, 0x0, 5, 1);
		nvt_eth_phy_masked_write(IOADDR_CG_REG_BASE + 0xA8, 0x1, 5, 1);

		// ignore both COMBO_PHY2_CLKSE/COMBO_PHY3_CLKSE need to mux to 0.
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x0, 0, 8);//to bank 0
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x25c, 0x1, 0, 8);// #bit0:cfg_refclk_src_sel #bit1:cfg_RX_MPLL_src_sel
	}


	nvt_eth_phy_trim(dev);
//#SGMII common setting
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x00, 0, 8); //to bank 0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x18c, 0x0b, 0, 8); //bit[2:0] = u30p_PHYMODE
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x94c, 0x88, 0, 8); //bit3 = u30p_rxtermination, bit7 = u30p_rxtermination_md
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x500, 0x80, 0, 8); //bit7 = u30p_rxanaif_rx_ss_ready_mode
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x540, 0x05, 0, 8); //bit[3:2] = u30p_rxanaif_ss_ready_slow_mode
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x600, 0xF0, 0, 8); //bit[4:0] = u30p_rxanaif_stopWSS_mode[4:0] = 0x10
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x5B8, 0xF0, 0, 8); //bit[3:0] = u30p_rxanaif_reWSS_mode[3:0] = 0x0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x5BC, 0xFE, 0, 8); //bit[1] =u30p_rxanaif_core_ctrl[1] . u30p_rxanaif_reWSS_sq_pkdet_mode =0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x400, 0x28, 0, 8); //bit2 = u30p_rxanaif_rclk_inv
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x404, 0x00, 0, 8); //bit7 = u30p_anaif_tclk_inv

	nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x02, 0, 8); //to bank 2
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x824, 0x09, 0, 8); //bit[7] = 7 = cfg_sgmii_phy_rdy_sel = 0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x0A8, 0x80, 0, 8); //bit[5:4] = LDO_VLDO_SEL_DCO[1:0] = 2'b00 RX LDO = 0.85v
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x00, 0, 8); //to bank 0

	//#same setting used in NT98690 SATA PHY
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x9F0, 0x01, 0, 8); //bit 0 = u30p_pll_ready_sync_en =1
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x9CC, 0x0F, 0, 8); //bit[3:0] = u30p_tx_sp_en_dly_cnt[3:0] = 0xf
	//#postpone CDR_EN enable
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x518, 0x0A, 0, 8); //#bit[7:0] =  u30p_rxanaif_cdr_s0_dly[7:0]
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x544, 0x13, 0, 8); //bit2 = u30p_rxanaif_aeq_en_sel, bit4 = u30p_rxanaif_AEQ_FSM_disable_sel
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x540, 0x02, 0, 8); //#bit[1:0] = u30p_rxanaif_cdr_ki_en_snum[1:0] = 0x2
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x9EC, 0x51, 0, 8); //#u30p_ele2_dly_time #delay time = value *128us , default = 1ms
	//#extend  TXPLL settle time
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x524, 0xFF, 0, 8); //bit[7:0] = u30p_rxanaif_vco_rst_pwidth = 0xff
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x514, 0xFF, 0, 8); //bit[7:0] = u30p_rxanaif_cdr_init_dly

	nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x01, 0, 8); //to bank 1
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x5A0, 0xEF, 0, 8); //bit [3:0] = r_ofs_time_period[3:0]  = 4'b1111
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x420, 0x02, 0, 8); //bit[2] = r_lms1_en = 0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x680, 0x5F, 0, 8); //#bit[4:0] = r_ba_cnt_ftime[4:0]
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x688, 0xF4, 0, 8); //#bit[7:4] = r_ba_abj_time
	//#HDMI DPHY RX setting
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x01, 0, 8); //to bank 1
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x578, 0xFF, 0, 8); //bit[7:0] = r_aeq_timeout_period[7:0]
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x088, 0x00, 0, 8); //bit2 = r_eq_c_freerun_en =0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x5B0, 0x00, 0, 8); //bit[3:0] = r_ofs_sa_eq_cft[3:0]
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x5C0, 0x00, 0, 8); //bit[3:0] =  r_ofs_sa_eq_rct[3:0] ; bit[7:4] = r_ofs_sa_eq_rvga[3:0]
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x5DC, 0xB9, 0, 8); //bit[1:0] =  r_ofs_eq_eq_spsel0[1:0],bit[3:2] =  r_ofs_eq_eq_spsel1[1:0],bit[5:4] =  r_ofs_eq_eq_spsel2[1:0]
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x08C, 0x23, 0, 8); //bit[7:4] = r_aeq_cfine_man[3:0],bit[3:0] = r_aeq_ccoarse_man[3:0]
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x694, 0x1C, 0, 8); //bit[5:4] = r_ki_fctl[1:0] = 0x1,bit[2:0] = r_ki_frate[2:0] =0x4
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x168, 0x08, 0, 8); //bit3 = r_ki_clk_inv=1
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x2E4, 0xC0, 0, 8); //bit 7 = r_eom_offset_swen,bit 6 = r_ie_use_eom_offset,bit [5:0] =  r_eom_offset_sw
	//# HDMI�@APHY RX  setting
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x02, 0, 8); //to bank 2
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x0A0, 0x77, 0, 8); //bit[1:0] = BIAS_EN_CDR_CST;bit[3:2] = BIAS_CDR_SEL_DCO;bit[7:4] = BIAS_CDR_SEL_PI[3:0]
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x02C, 0x3B, 0, 8); //bit[1:0] EQ_RS_TUNE[1:0] = 0x3;bit[3:2] = EQ_SPSEL[1:0] = 0x2
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x00C, 0x21, 0, 8); //bit[2:0] EQ_EOC_R_SW[2:0] =0x1;bit[7:4] = EQ_EOC_VSEL[3:0] =0x2
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x008, 0xEC, 0, 8); //bit7 = EQ_EOC_STEP=1
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x078, 0x10, 0, 8); //bit4 = DFE_SUM_GAIN =1,bit5 = DFE_EN_SEL1_H1_X2 = 0,bit6 = DFE_EN_SEL3_H1_X = 0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x0CC, 0xA0, 0, 8); //bit5 = SM_EQ_RVGA =1, bit7 = SM_EQ_SPSE =1
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x028, 0x0F, 0, 8); //bit[7:4] = EQ_RVGA[3:0] =0x0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x024, 0x10, 0, 8); //bit[6:4] = EQ_RFB[2:0] =0x1
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x004, 0xC0, 0, 8); //bit[3:0] = EQ_CFB[3:0] = 0x0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x07C, 0x7C, 0, 8); // bit[6:4] = CDR_P_CTL[2:0] = 0x7
	//#  TX setting
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x00, 0, 8);//to bank 0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x160, 0x04, 0, 8);//bit 2 = #TX VCM stabling function for SATA TX OOB =1
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x0D0, 0xFC, 0, 8);//bit[1:0] = AMP_CTL manaul mode  = 0x3,bit[7:4] = DEC_EM_CTL[3:0] manual mode =0xf
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x050, 0x09, 0, 8);//bit[1:0] = AMP_CTL manaul value = 0x1,bit[7:4] = DEC_EM_CTL[3:0] manaul value = 0x0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x0B8, 0x08, 0, 8);//bit3 = TX_LP manual mode =1
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x038, 0x42, 0, 8);//bit3 = TX_LP manual value =0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x450, 0x03, 0, 8);//##3. Power mode change ready response time = 1 ms
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x814, 0x07, 0, 8);//#bit 0 = u30p_ec_en #bit 1 = u30p_ec_en_mode
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x01, 0, 8);//to bank 1
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x16c, 0x80, 0, 8);//r_ba_init

	// LOs deglitch setting
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x00, 0, 8);//to bank 0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x250, 0x03, 0, 8);//bit7 = cfg_rx_los_bypass , default 1, modify to 0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x254, 0x0f, 0, 8);//bit[5:0] : cfg_rx_los_filt_0default 0xf

	// Follow SATA & USB. SGMII not use
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x00, 0, 8);//to bank 0
	nvt_eth_phy_masked_write(serdes_phy_ba + 0x158, 0x74, 0, 8);//bit[2:0] = BIAS_VSQ_SEL[2:0] = 3'b100  (unused)bit3 = RX_EN_VOS_CAL = 1'b0 (unused)bit[5:4] = BIAS_VCH_SEL[1:0] = 2'b01 -> 2'b11bit[7:6] = RX_RS_TUNE[1:0] = 2'b01 (unsed)

	if (xgmac->phy->speed == SPEED_2500) {
		//printk("#SPEED_2500\n");
		//SGMII 3.125G
		//TXPLL setting
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x00, 0, 8);//to bank 0
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x1A8, 0x6e, 0, 8);//bit1 = SM_TXPLL_VCO_BAND_SEL =1;bit2 = SM_TXPLL_VCO_CAP_EN =1;bit3 = SM_TXPLL_CLKMULTCLK=1;bit5 = SM_TXPLL_DIV_PRE =1;bit6 = SM_TXPLL_DIV_CNT
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x194, 0xb9, 0, 8);//bit 3 =  TXPLL_VCO_BAND_SEL = 1'b0;bit 4 = TX_VCO_CAP_EN = 1'b1;bit[7:5] =  TXPLL_CLKMULT_CTL[2:0] = 3'b101
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x198, 0x40, 0, 8);//bit[7:5] = TXPLL_DIV_PRE[2:0] = 3'b010
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x19C, 0x18, 0, 8);//bit[4:0]  = TXPLL_DIV_CNT = 0x13
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x164, 0x01, 0, 8);//bit[0] = SM_TX_RESERVED0 = 1
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x160, 0x02, 0, 8);//bit[1:0] = TXPLL_DIV_POST[1:0] = 2'b01
		//RX setting
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x02, 0, 8);//to bank 2
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x0E0, 0x08, 0, 8);//bit3 = SM_CDR_MODE = 1
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x07C, 0x7c, 0, 8);//bit[2:0] = SW_CDR_MODE = 3'b010
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x00, 0, 8);//to bank 0
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x408, 0xf0, 0, 8);//anaif_pc_rst=1
		udelay(1);
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x408, 0x00, 0, 8);//anaif_pc_rst=0

//	} else if (speed == SPEED_1000) {
	} else {
		//SGMII 1.25G
		//printk("#SPEED_1000 100 10\n");
		//TXPLL setting
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x00, 0, 8);//to bank 0
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x1A8, 0x6e, 0, 8);//bit1 = SM_TXPLL_VCO_BAND_SEL =1;bit2 = SM_TXPLL_VCO_CAP_EN =1;bit3 = SM_TXPLL_CLKMULTCLK=1;bit5 = SM_TXPLL_DIV_PRE =1;bit6 = SM_TXPLL_DIV_CNT
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x194, 0xb1, 0, 8);//bit 3 =  TXPLL_VCO_BAND_SEL = 1'b0;bit 4 = TX_VCO_CAP_EN = 1'b1;bit[7:5] =  TXPLL_CLKMULT_CTL[2:0] = 3'b101
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x198, 0x40, 0, 8);//bit[7:5] = TXPLL_DIV_PRE[2:0] = 3'b010
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x19C, 0x13, 0, 8);//bit[4:0]  = TXPLL_DIV_CNT = 0x13
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x164, 0x01, 0, 8);//bit[0] = SM_TX_RESERVED0 = 1
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x160, 0x01, 0, 8);//bit[1:0] = TXPLL_DIV_POST[1:0] = 2'b01
		//RX setting
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x02, 0, 8);//to bank 2
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x0E0, 0x08, 0, 8);//bit3 = SM_CDR_MODE = 1
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x07C, 0x7a, 0, 8);//bit[2:0] = SW_CDR_MODE = 3'b010
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x0A0, 0xf7, 0, 8);//bit[1:0] = BIAS_EN_CDR_CST;bit[3:2] = BIAS_CDR_SEL_DCO;bit[7:4] = BIAS_CDR_SEL_PI[3:0]
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x3FC, 0x00, 0, 8);//to bank 0
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x408, 0xf0, 0, 8);//anaif_pc_rst=1
		udelay(1);
		nvt_eth_phy_masked_write(serdes_phy_ba + 0x408, 0x00, 0, 8);//anaif_pc_rst=0
	}

	if (xgmac->eth_idx == 0) {
		nvt_eth_phy_masked_write(IOADDR_CG_REG_BASE + 0xA8, 0x1, 2, 1);
		mdelay(10);
	} else {
		nvt_eth_phy_masked_write(IOADDR_CG_REG_BASE + 0xA8, 0x1, 4, 1);
		mdelay(10);
	}

//==============================xpcs init======================================
	// Wait 0x1F0000 [15] = 0
	writel(0x1f00, xpcs_ba + 0x3fc);
	reg = readl(xpcs_ba);
	writel(reg | (0x1 << 15), xpcs_ba);
	i = 0;
	do {
		reg = readl(xpcs_ba);
		if (!(reg & (0x1 << 15))) {
			break;
		} else if (++i >= EPHY_RETRY_COUNT) {
			pr_err("xpcs init fail, reg : 0x%x\n", reg);
		}

		udelay(10);
	} while (i < EPHY_RETRY_COUNT);

	if (xgmac->phy->speed == SPEED_2500) {
		if (xgmac->phy->interface == PHY_INTERFACE_MODE_SGMII) {
			// 0x1F8001 [2:1] = 0 #VR_MII_AN_CTRL[2:l]
			writel(0x1f80, xpcs_ba + 0x3fc);
			reg = readl(xpcs_ba + 0x4);
			reg &= ~(0x3 << 1);
			writel(reg | (0x2 << 1), xpcs_ba + 0x4);
			// 0x1F800A [15:0] = 0x7A1, SGMII 2.5G link timer
			reg = readl(xpcs_ba + 0x40);
			writel(0x07A1, xpcs_ba + 0x40);
			// 0x1F8000 [3], SGMII 2.5G over ride link timer
			reg = readl(xpcs_ba);
			writel(reg | (0x1 << 3), xpcs_ba);
		} else {
			// 0x1F8001 [2:1] = 2 #VR_MII_AN_CTRL[2:l]
			writel(0x1f80, xpcs_ba + 0x3fc);
			reg = readl(xpcs_ba + 0x4);
			writel(reg & ~(0x3 << 1), xpcs_ba + 0x4);
			if (xgmac->phy_mode_oc) {
				// 0x1F800A [15:0] = 0x2FAF, 1000BASE-X(OC) 2.5G link timer
				reg = readl(xpcs_ba + 0x40);
				writel(0x2FAF, xpcs_ba + 0x40);
				// 0x1F8000 [3], 1000BASE-X(OC) 2.5G over ride link timer
				reg = readl(xpcs_ba);
				writel(reg | (0x1 << 3), xpcs_ba);
			}
		}
	} else {
		// 0x1F8001 [2:1] = 2 SGMII mode(This includes speeds of 1000M/100M/10M)
		writel(0x1f80, xpcs_ba + 0x3fc);
		reg = readl(xpcs_ba + 0x4);
		reg &= ~(0x3 << 1);
		writel(reg | (0x2 << 1), xpcs_ba + 0x4);
	}

	// 0x1F0000 [12] = 1 #Clause37 auto-negotiation AN_ENABLE
	writel(0x1f00, xpcs_ba + 0x3fc);
	reg = readl(xpcs_ba);
	if (xgmac->phy->interface == PHY_INTERFACE_MODE_SGMII) {
		writel(reg | (0x1 << 12), xpcs_ba);
	} else {
		writel(reg & ~(0x1 << 12), xpcs_ba);
	}


	// Wait 0x1F0001 [2] = 1;  #link up
	i = 0;
	do {
		reg = readl(xpcs_ba + 0x4);
		if (reg & 0x4) {
			break;
		} else if (++i >= EPHY_RETRY_COUNT) {
			pr_err("link up fail, reg : 0x%x\n", reg);
			return -1;
		}

		udelay(10);
	} while (i < EPHY_RETRY_COUNT);

	debug("speed = %d, interface = %d, phy_mode_oc = %d\n", xgmac->phy->speed, xgmac->phy->interface, xgmac->phy_mode_oc);
	if (xgmac->phy->speed == SPEED_2500) {
		if (xgmac->phy->interface == PHY_INTERFACE_MODE_2500BASEX) {
			if (xgmac->phy_mode_oc) {
				printf("%s, %d\n", __func__, __LINE__);
				// 0x1f8000 [2] = 1;
				writel(0x1f80, xpcs_ba + 0x3fc);
				reg = readl(xpcs_ba);
				writel(reg | (0x1 << 2), xpcs_ba);

				// 0x1f8004 [3] = 1;
				reg = readl(xpcs_ba + 0x10);
				writel(reg & ~(0x1 << 3), xpcs_ba + 0x10);
			} else {
				printf("%s, %d\n", __func__, __LINE__);
				// 0x1f8000 [2] = 1;
				writel(0x1f80, xpcs_ba + 0x3fc);
				reg = readl(xpcs_ba);
				writel(reg & ~(0x1 << 2), xpcs_ba);

				// 0x1f8004 [3] = 1;
				reg = readl(xpcs_ba + 0x10);
				writel(reg | (0x1 << 3), xpcs_ba + 0x10);
			}
		} else {
			// 0x1f8000 [2] = 1;
			writel(0x1f80, xpcs_ba + 0x3fc);
			reg = readl(xpcs_ba);
			writel(reg | (0x1 << 2), xpcs_ba);

			// 0x1f8004 [3] = 0;
			reg = readl(xpcs_ba + 0x10);
			writel(reg & ~(0x1 << 3), xpcs_ba + 0x10);
		}
	} else {
		// 0x1f8000 [2] = 0;
		writel(0x1f80, xpcs_ba + 0x3fc);
		reg = readl(xpcs_ba);
		writel(reg & ~(0x1 << 2), xpcs_ba);

		// 0x1f8004 [3] = 0;
		reg = readl(xpcs_ba + 0x10);
		writel(reg & ~(0x1 << 3), xpcs_ba + 0x10);
	}

	if (xgmac->phy->speed == SPEED_10) {
		// 0x1f0000 [6] = 0;
		writel(0x1f00, xpcs_ba + 0x3fc);
		reg = readl(xpcs_ba);
		writel(reg & ~(0x1 << 6), xpcs_ba);
		// 0x1f0000 [13] = 0;
		reg = readl(xpcs_ba);
		writel(reg & ~(0x1 << 13), xpcs_ba);
	} else if (xgmac->phy->speed == SPEED_100) {
		// 0x1f0000 [6] = 0;
		writel(0x1f00, xpcs_ba + 0x3fc);
		reg = readl(xpcs_ba);
		writel(reg & ~(0x1 << 6), xpcs_ba);
		// 0x1f0000 [13] = 1;
		reg = readl(xpcs_ba);
		writel(reg | (0x1 << 13), xpcs_ba);
	} else if (xgmac->phy->speed == SPEED_1000) {
		// 0x1f0000 [6] = 1;
		writel(0x1f00, xpcs_ba + 0x3fc);
		reg = readl(xpcs_ba);
		writel(reg | (0x1 << 6), xpcs_ba);
		// 0x1f0000 [13] = 0;
		reg = readl(xpcs_ba);
		writel(reg & ~(0x1 << 13), xpcs_ba);
	} else if (xgmac->phy->speed == SPEED_2500) {
		// 0x1f0000 [6] = 1;
		writel(0x1f00, xpcs_ba + 0x3fc);
		reg = readl(xpcs_ba);
		writel(reg | (0x1 << 6), xpcs_ba);
		// 0x1f0000 [13] = 0;
		reg = readl(xpcs_ba);
		writel(reg & ~(0x1 << 13), xpcs_ba);
	}

	writel(0x1f00, xpcs_ba + 0x3fc);
	reg = readl(xpcs_ba);

	if (xgmac->phy->interface == PHY_INTERFACE_MODE_SGMII) {
	// Wait 0x1F0001 [5] = 1;  #an complete
		do {
			writel(0x1f00, xpcs_ba + 0x3fc);
			reg = readl(xpcs_ba + 0x4);
		        if (reg & 0x20) {
				break;
			} else if (++i >= EPHY_RETRY_COUNT) {
				pr_err("AN Complete fail, reg : 0x%x\n", reg);
				return -1;
			}

	        udelay(10);
		} while (i < EPHY_RETRY_COUNT);
	}

#ifdef CONFIG_NVT_FPGA_EMULATION
	if (xgmac->eth_idx == 0) {
		// Gth_serdes1_rstn
		reg = readl(IOADDR_CG_REG_BASE + 0xA8);
		reg &= ~(0x1 << 2);
		writel(reg, IOADDR_CG_REG_BASE + 0xA8);
		reg = readl(IOADDR_CG_REG_BASE + 0xA8);
		reg |= (0x1 << 2);
		writel(reg, IOADDR_CG_REG_BASE + 0xA8);
		// 0x4[1] = 1; # mmcm_clk_sw_ini_en = 1
		reg = readl(IOADDR_SERDES_PHY_REG_BASE + 0x10);
		reg |= (0x1 << 1);
		writel(reg, IOADDR_SERDES_PHY_REG_BASE + 0x10);
//		reg = readl(IOADDR_SERDES_PHY_REG_BASE + 0x4);
//		reg |= (0x1 << 1);
//		writel(reg, IOADDR_SERDES_PHY_REG_BASE + 0x4);

		// 0x0[1] = 0; # gth_rst_sw = 0
		reg = readl(IOADDR_SERDES_PHY_REG_BASE);
		reg &= ~(0x1 << 1);
		writel(reg, IOADDR_SERDES_PHY_REG_BASE);
		// Wait 0x[1] = 0;
		i = 0;
		do {
			reg = readl(IOADDR_SERDES_PHY_REG_BASE);
			if (!(reg & 0x1)) {
				break;
			} else if (++i >= EPHY_RETRY_COUNT) {
				pr_err("get_rst_sw = 0 fail, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);

		if (xgmac->phy->speed == SPEED_2500) {
			// 0x0[6:4] = 0x6; # clk_mode=1(2.5G), board_refclk1_en=1, board_refclk0_en=0
			reg = readl(IOADDR_SERDES_PHY_REG_BASE);
			reg &= ~(0x70);
			reg |= (0x6 << 4);
			writel(reg, IOADDR_SERDES_PHY_REG_BASE);
		} else if (xgmac->phy->speed == SPEED_1000) {
			// 0x0[6:4] = 0x1; # clk_mode=0(1G), board_refclk1_en=0, board_refclk0_en=1
			reg = readl(IOADDR_SERDES_PHY_REG_BASE);
			reg &= ~(0x70);
			reg |= (0x1 << 4);
			writel(reg, IOADDR_SERDES_PHY_REG_BASE);
		}

		// 0x0[1] = 1; # gth_rst_sw = 1
		reg = readl(IOADDR_SERDES_PHY_REG_BASE);
		reg |= (0x1 << 1);
		writel(reg, IOADDR_SERDES_PHY_REG_BASE);
		// Wait 0x[1] = 1
		i = 0;
		do {
			reg = readl(IOADDR_SERDES_PHY_REG_BASE);
			if (reg & 0x2) {
				break;
			} else if (++i >= EPHY_RETRY_COUNT) {
				pr_err("get_rst_sw = 1 fail, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);

		// Wait 0x[9:8] = 0x3; # wait mmcm_locked = 1, gth_reset_done = 1
		i = 0;
		do {
			reg = readl(IOADDR_SERDES_PHY_REG_BASE + 0x4);
			if (reg & 0x3) {
				break;
			} else if (++i >= EPHY_RETRY_COUNT) {
				pr_err("mmcm_lock, gth_reset_done fail, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);

/*		do {
			reg = readl(IOADDR_SERDES_PHY_REG_BASE);
			if (reg & 0x300) {
				break;
			} else if (++i >= EPHY_RETRY_COUNT) {
				pr_err("mmcm_lock, gth_reset_done fail, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);
*/
		writel(0x3, IOADDR_SERDES_PHY_REG_BASE + 0x40);
		writel(0x3f, IOADDR_SERDES_PHY_REG_BASE + 0x48);
//==============================xpcs init======================================
		// Wait 0x1F0000 [15] = 0
		writel(0x1f00, IOADDR_XPCS_REG_BASE + 0x3fc);
		i = 0;
		do {
			reg = readl(IOADDR_XPCS_REG_BASE);
			if (!(reg & (0x1 << 15))) {
				break;
			} else if (++i >= EPHY_RETRY_COUNT) {
				pr_err("xpcs init fail, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);

		if (xgmac->phy->interface == PHY_INTERFACE_MODE_1000BASEX) {
			// 0x1F8001 [2:1] = 0 #VR_MII_AN_CTRL[2:l]
			writel(0x1f80, IOADDR_XPCS_REG_BASE + 0x3fc);
			reg = readl(IOADDR_XPCS_REG_BASE + 0x4);
			writel(reg & ~(0x3 << 1), IOADDR_XPCS_REG_BASE + 0x4);
		} else {
			// 0x1F8001 [2:1] = 2 #VR_MII_AN_CTRL[2:l]
			writel(0x1f80, IOADDR_XPCS_REG_BASE + 0x3fc);
			reg = readl(IOADDR_XPCS_REG_BASE + 0x4);
			writel(reg | (0x1 << 2), IOADDR_XPCS_REG_BASE + 0x4);
		}

		// 0x1F0000 [12] = 1 #Clause37 auto-negotiation AN_ENABLE
		writel(0x1f00, IOADDR_XPCS_REG_BASE + 0x3fc);
		reg = readl(IOADDR_XPCS_REG_BASE);
		writel(reg | (0x1 << 12), IOADDR_XPCS_REG_BASE);

		// Wait 0x1F0001 [2] = 1;  #link up
		i = 0;
		do {
			reg = readl(IOADDR_XPCS_REG_BASE + 0x4);
			if (reg & 0x4) {
				break;
			} else if (++i >= EPHY_RETRY_COUNT) {
				pr_err("link up fail, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);

		// Wait 0x1F0001 [5] = 1;  #an complete
		/*  do {
		        writel(0x1f00, IOADDR_XPCS_REG_BASE + 0x3fc);
		        reg = readl(IOADDR_XPCS_REG_BASE + 0x4);
		        if (reg & 0x20) {
		            break;
		        } else if (++i >= EPHY_RETRY_COUNT)
		            pr_err("AN Complete fail, reg : 0x%x\n", reg);

		        udelay(10);
		    } while (i < EPHY_RETRY_COUNT);

		*/       if ((xgmac->phy->interface == PHY_INTERFACE_MODE_2500BASEX) &&
					 (xgmac->phy->speed == SPEED_2500)) {
			// 0x1f8000 [2] = 1;
			writel(0x1f80, IOADDR_XPCS_REG_BASE + 0x3fc);
			reg = readl(IOADDR_XPCS_REG_BASE);
			writel(reg | (0x1 << 2), IOADDR_XPCS_REG_BASE);

			// 0x1f8004 [3] = 1;
			reg = readl(IOADDR_XPCS_REG_BASE + 0x10);
			writel(reg | (0x1 << 3), IOADDR_XPCS_REG_BASE + 0x10);
		} else if ((xgmac->phy->interface != PHY_INTERFACE_MODE_2500BASEX) &&
				   (xgmac->phy->speed == SPEED_2500)) {
			// 0x1f8000 [2] = 1;
			writel(0x1f80, IOADDR_XPCS_REG_BASE + 0x3fc);
			reg = readl(IOADDR_XPCS_REG_BASE);
			writel(reg | (0x1 << 2), IOADDR_XPCS_REG_BASE);

			// 0x1f8004 [3] = 0;
			reg = readl(IOADDR_XPCS_REG_BASE + 0x10);
			writel(reg & ~(0x1 << 3), IOADDR_XPCS_REG_BASE + 0x10);
		} else {
			// 0x1f8000 [2] = 0;
			writel(0x1f80, IOADDR_XPCS_REG_BASE + 0x3fc);
			reg = readl(IOADDR_XPCS_REG_BASE);
			writel(reg & ~(0x1 << 2), IOADDR_XPCS_REG_BASE);

			// 0x1f8004 [3] = 0;
			reg = readl(IOADDR_XPCS_REG_BASE + 0x10);
			writel(reg & ~(0x1 << 3), IOADDR_XPCS_REG_BASE + 0x10);
		}

		if (xgmac->phy->interface == PHY_INTERFACE_MODE_SGMII) {
			if (xgmac->phy->speed == SPEED_10) {
				// 0x1f0000 [6] = 0;
				writel(0x1f00, IOADDR_XPCS_REG_BASE + 0x3fc);
				reg = readl(IOADDR_XPCS_REG_BASE);
				writel(reg & ~(0x1 << 6), IOADDR_XPCS_REG_BASE);
				// 0x1f0000 [13] = 0;
				reg = readl(IOADDR_XPCS_REG_BASE);
				writel(reg & ~(0x1 << 13), IOADDR_XPCS_REG_BASE);
			} else if (xgmac->phy->speed == SPEED_100) {
				// 0x1f0000 [6] = 0;
				writel(0x1f00, IOADDR_XPCS_REG_BASE + 0x3fc);
				reg = readl(IOADDR_XPCS_REG_BASE);
				writel(reg & ~(0x1 << 6), IOADDR_XPCS_REG_BASE);
				// 0x1f0000 [13] = 1;
				reg = readl(IOADDR_XPCS_REG_BASE);
				writel(reg | (0x1 << 13), IOADDR_XPCS_REG_BASE);
			} else if (xgmac->phy->speed == SPEED_1000) {
				// 0x1f0000 [6] = 1;
				writel(0x1f00, IOADDR_XPCS_REG_BASE + 0x3fc);
				reg = readl(IOADDR_XPCS_REG_BASE);
				writel(reg | (0x1 << 6), IOADDR_XPCS_REG_BASE);
				// 0x1f0000 [13] = 0;
				reg = readl(IOADDR_XPCS_REG_BASE);
				writel(reg & ~(0x1 << 13), IOADDR_XPCS_REG_BASE);
			} else if (xgmac->phy->speed == SPEED_2500) {
				// 0x1f0000 [6] = 1;
				writel(0x1f00, IOADDR_XPCS_REG_BASE + 0x3fc);
				reg = readl(IOADDR_XPCS_REG_BASE);
				writel(reg | (0x1 << 6), IOADDR_XPCS_REG_BASE);
				// 0x1f0000 [13] = 0;
				reg = readl(IOADDR_XPCS_REG_BASE);
				writel(reg & ~(0x1 << 13), IOADDR_XPCS_REG_BASE);
			}
		}

		// Wait 0x1F0001 [5] = 1;  #an complete
		do {
		        writel(0x1f00, IOADDR_XPCS_REG_BASE + 0x3fc);
		        reg = readl(IOADDR_XPCS_REG_BASE + 0x4);
		        if (reg & 0x20) {
		            break;
		        } else if (++i >= EPHY_RETRY_COUNT)
		            pr_err("AN Complete fail, reg : 0x%x\n", reg);

		        udelay(10);
		} while (i < EPHY_RETRY_COUNT);

	} else {
		reg = readl(IOADDR_CG_REG_BASE + 0xA8);
		reg &= ~(0x1 << 4);
		writel(reg, IOADDR_CG_REG_BASE + 0xA8);
		reg = readl(IOADDR_CG_REG_BASE + 0xA8);
		reg |= (0x1 << 4);
		writel(reg, IOADDR_CG_REG_BASE + 0xA8);
		reg = readl(IOADDR_SERDES_PHY1_REG_BASE + 0x10);
		reg |= (0x1 << 1);
		writel(reg, IOADDR_SERDES_PHY1_REG_BASE + 0x10);
//		reg = readl(IOADDR_SERDES_PHY1_REG_BASE + 0x4);
//		reg |= (0x1 << 1);
//		writel(reg, IOADDR_SERDES_PHY1_REG_BASE + 0x4);
		// 0x0[1] = 0; # gth_rst_sw = 0

		reg = readl(IOADDR_SERDES_PHY1_REG_BASE);
		reg &= ~(0x1 << 1);
		writel(reg, IOADDR_SERDES_PHY1_REG_BASE);
		// Wait 0x[1] = 0;
		i = 0;
		do {
			reg = readl(IOADDR_SERDES_PHY1_REG_BASE);
			if (!(reg & 0x1)) {
				break;
			} else if (++i >= EPHY_RETRY_COUNT) {
				pr_err("get_rst_sw = 0 fail, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);

		if (xgmac->phy->speed == SPEED_2500) {
			// 0x0[6:4] = 0x6; # clk_mode=1(2.5G), board_refclk1_en=1, board_refclk0_en=0
			reg = readl(IOADDR_SERDES_PHY1_REG_BASE);
			reg &= ~(0x70);
			reg |= (0x6 << 4);
			writel(reg, IOADDR_SERDES_PHY1_REG_BASE);
		} else if (xgmac->phy->speed == SPEED_1000) {
			// 0x0[6:4] = 0x1; # clk_mode=0(1G), board_refclk1_en=0, board_refclk0_en=1
			reg = readl(IOADDR_SERDES_PHY1_REG_BASE);
			reg &= ~(0x70);
			reg |= (0x1 << 4);
			writel(reg, IOADDR_SERDES_PHY1_REG_BASE);
		}

		// 0x0[1] = 1; # gth_rst_sw = 1
		reg = readl(IOADDR_SERDES_PHY1_REG_BASE);
		reg |= (0x1 << 1);
		writel(reg, IOADDR_SERDES_PHY1_REG_BASE);
		// Wait 0x[1] = 1
		i = 0;
		do {
			reg = readl(IOADDR_SERDES_PHY1_REG_BASE);
			if (reg & 0x2) {
				break;
			} else if (+i >= EPHY_RETRY_COUNT) {
				pr_err("get_rst_sw = 1 fail, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);

		// Wait 0x[9:8] = 0x3; # wait mmcm_locked = 1, gth_reset_done = 1
		i = 0;
		do {
			reg = readl(IOADDR_SERDES_PHY1_REG_BASE + 0x4);
			if (reg & 0x3) {
				break;
			} else if (+i >= EPHY_RETRY_COUNT) {
				pr_err("mmcm_lock, gth_reset_done fail, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);

/*		do {
			reg = readl(IOADDR_SERDES_PHY1_REG_BASE);
			if (reg & 0x300) {
				break;
			} else if (+i >= EPHY_RETRY_COUNT) {
				pr_err("mmcm_lock, gth_reset_done fail, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);
*/
		writel(0x3, IOADDR_SERDES_PHY1_REG_BASE + 0x40);
		writel(0x3f, IOADDR_SERDES_PHY1_REG_BASE + 0x48);

//==============================xpcs init======================================
		// Wait 0x1F0000 [15] = 0
		writel(0x1f00, IOADDR_XPCS1_REG_BASE + 0x3fc);
		i = 0;
		do {
			reg = readl(IOADDR_XPCS1_REG_BASE);
			if (!(reg & (0x1 << 15))) {
				break;
			} else if (+i >= EPHY_RETRY_COUNT) {
				pr_err("xpcs init fail, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);

		if (xgmac->phy->interface == PHY_INTERFACE_MODE_1000BASEX) {
			// 0x1F8001 [2:1] = 0 #VR_MII_AN_CTRL[2:l]
			writel(0x1f80, IOADDR_XPCS1_REG_BASE + 0x3fc);
			reg = readl(IOADDR_XPCS1_REG_BASE + 0x4);
			writel(reg & ~(0x3 << 1), IOADDR_XPCS1_REG_BASE + 0x4);
		} else {
			// 0x1F8001 [2:1] = 2 #VR_MII_AN_CTRL[2:l]
			writel(0x1f80, IOADDR_XPCS1_REG_BASE + 0x3fc);
			reg = readl(IOADDR_XPCS1_REG_BASE + 0x4);
			writel(reg | (0x1 << 2), IOADDR_XPCS1_REG_BASE + 0x4);
		}

		// 0x1F0000 [12] = 1 #Clause37 auto-negotiation AN_ENABLE
		writel(0x1f00, IOADDR_XPCS1_REG_BASE + 0x3fc);
		reg = readl(IOADDR_XPCS1_REG_BASE);
		writel(reg | (0x1 << 12), IOADDR_XPCS1_REG_BASE);

		// Wait 0x1F0001 [2] = 1;  #link up
		i = 0;
		do {
			reg = readl(IOADDR_XPCS1_REG_BASE + 0x4);
			if (reg & 0x4) {
				break;
			} else if (+i >= EPHY_RETRY_COUNT) {
				pr_err("link up fail, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);

		// Wait 0x1F0001 [5] = 1;  #an complete
/*		i = 0;
		do {
			reg = readl(IOADDR_XPCS1_REG_BASE + 0x4);
			if (reg & 0x20) {
				break;
			} else if (+i >= EPHY_RETRY_COUNT) {
				pr_err("AN Complete fail, reg : 0x%x\n", reg);
			}

			udelay(10);
		} while (i < EPHY_RETRY_COUNT);
*/
		if ((xgmac->phy->interface == PHY_INTERFACE_MODE_2500BASEX) &&
			(xgmac->phy->speed == SPEED_2500)) {
			// 0x1f8000 [2] = 1;
			writel(0x1f80, IOADDR_XPCS1_REG_BASE + 0x3fc);
			reg = readl(IOADDR_XPCS1_REG_BASE);
			writel(reg | (0x1 << 2), IOADDR_XPCS1_REG_BASE);

			// 0x1f8004 [3] = 1;
			reg = readl(IOADDR_XPCS1_REG_BASE + 0x10);
			writel(reg | (0x1 << 3), IOADDR_XPCS1_REG_BASE + 0x10);
		} else if ((xgmac->phy->interface != PHY_INTERFACE_MODE_2500BASEX) &&
				   (xgmac->phy->speed == SPEED_2500)) {
			// 0x1f8000 [2] = 1;
			writel(0x1f80, IOADDR_XPCS1_REG_BASE + 0x3fc);
			reg = readl(IOADDR_XPCS1_REG_BASE);
			writel(reg | (0x1 << 2), IOADDR_XPCS1_REG_BASE);

			// 0x1f8004 [3] = 0;
			reg = readl(IOADDR_XPCS1_REG_BASE + 0x10);
			writel(reg & ~(0x1 << 3), IOADDR_XPCS1_REG_BASE + 0x10);
		} else {
			// 0x1f8000 [2] = 0;
			writel(0x1f80, IOADDR_XPCS1_REG_BASE + 0x3fc);
			reg = readl(IOADDR_XPCS1_REG_BASE);
			writel(reg & ~(0x1 << 2), IOADDR_XPCS1_REG_BASE);

			// 0x1f8004 [3] = 0;
			reg = readl(IOADDR_XPCS1_REG_BASE + 0x10);
			writel(reg & ~(0x1 << 3), IOADDR_XPCS1_REG_BASE + 0x10);
		}

		if (xgmac->phy->interface == PHY_INTERFACE_MODE_SGMII) {
			if (xgmac->phy->speed == SPEED_10) {
				// 0x1f0000 [6] = 0;
				writel(0x1f00, IOADDR_XPCS1_REG_BASE + 0x3fc);
				reg = readl(IOADDR_XPCS1_REG_BASE);
				writel(reg & ~(0x1 << 6), IOADDR_XPCS1_REG_BASE);
				// 0x1f0000 [13] = 0;
				reg = readl(IOADDR_XPCS1_REG_BASE);
				writel(reg & ~(0x1 << 13), IOADDR_XPCS1_REG_BASE);
			} else if (xgmac->phy->speed == SPEED_100) {
				// 0x1f0000 [6] = 0;
				writel(0x1f00, IOADDR_XPCS1_REG_BASE + 0x3fc);
				reg = readl(IOADDR_XPCS1_REG_BASE);
				writel(reg & ~(0x1 << 6), IOADDR_XPCS1_REG_BASE);
				// 0x1f0000 [13] = 1;
				reg = readl(IOADDR_XPCS1_REG_BASE);
				writel(reg | (0x1 << 13), IOADDR_XPCS1_REG_BASE);
			} else if (xgmac->phy->speed == SPEED_1000) {
				// 0x1f0000 [6] = 1;
				writel(0x1f00, IOADDR_XPCS1_REG_BASE + 0x3fc);
				reg = readl(IOADDR_XPCS1_REG_BASE);
				writel(reg | (0x1 << 6), IOADDR_XPCS1_REG_BASE);
				// 0x1f0000 [13] = 0;
				reg = readl(IOADDR_XPCS1_REG_BASE);
				writel(reg & ~(0x1 << 13), IOADDR_XPCS1_REG_BASE);
			} else if (xgmac->phy->speed == SPEED_2500) {
				// 0x1f0000 [6] = 1;
				writel(0x1f00, IOADDR_XPCS1_REG_BASE + 0x3fc);
				reg = readl(IOADDR_XPCS1_REG_BASE);
				writel(reg | (0x1 << 6), IOADDR_XPCS1_REG_BASE);
				// 0x1f0000 [13] = 0;
				reg = readl(IOADDR_XPCS1_REG_BASE);
				writel(reg & ~(0x1 << 13), IOADDR_XPCS1_REG_BASE);
			}
		}
		//  Wait 0x1F0001 [5] = 1;  #an complete
		i = 0;
	        do {
			writel(0x1f00, IOADDR_XPCS1_REG_BASE + 0x3fc);
			reg = readl(IOADDR_XPCS1_REG_BASE + 0x4);
			if (reg & 0x20) {
				break;
			} else if (++i >= EPHY_RETRY_COUNT)
				pr_err("AN Complete fail, reg : 0x%x\n", reg);

			udelay(10);
	        } while (i < EPHY_RETRY_COUNT);
	}
#endif
	return 0;
}

static int xgmac_write_hwaddr(struct udevice *dev)
{
	struct eth_pdata *plat = dev_get_plat(dev);
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	u32 val;
	/*
	 * This function may be called before start() or after stop(). At that
	 * time, on at least some configurations of the XGMAC HW, all clocks to
	 * the XGMAC HW block will be stopped, and a reset signal applied. If
	 * any register access is attempted in this state, bus timeouts or CPU
	 * hangs may occur. This check prevents that.
	 *
	 * A simple solution to this problem would be to not implement
	 * write_hwaddr(), since start() always writes the MAC address into HW
	 * anyway. However, it is desirable to implement write_hwaddr() to
	 * support the case of SW that runs subsequent to U-Boot which expects
	 * the MAC address to already be programmed into the XGMAC registers,
	 * which must happen irrespective of whether the U-Boot user (or
	 * scripts) actually made use of the XGMAC device, and hence
	 * irrespective of whether start() was ever called.
	 *
	 */
	if (!xgmac->config->reg_access_always_ok && !xgmac->reg_access_ok) {
		return 0;
	}
	/* Update the MAC address */
	val = (plat->enetaddr[5] << 8) |
		  (plat->enetaddr[4]);
	writel(val, &xgmac->mac_regs->address0_high);
	val = (plat->enetaddr[3] << 24) |
		  (plat->enetaddr[2] << 16) |
		  (plat->enetaddr[1] << 8) |
		  (plat->enetaddr[0]);
	writel(val, &xgmac->mac_regs->address0_low);
	return 0;
}
#if 0
static int xgmac_read_rom_hwaddr(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_plat(dev);
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	int ret;

	ret = xgmac->config->ops->xgmac_get_enetaddr(dev);
	if (ret < 0) {
		return ret;
	}

	return !is_valid_ethaddr(pdata->enetaddr);
}

static int xgmac_get_phy_addr(struct xgmac_priv *priv, struct udevice *dev)
{
	struct ofnode_phandle_args phandle_args;
	int reg;

	if (dev_read_phandle_with_args(dev, "phy-handle", NULL, 0, 0,
								   &phandle_args)) {
		debug("Failed to find phy-handle");
		return -ENODEV;
	}

	priv->phy_of_node = phandle_args.node;

	reg = ofnode_read_u32_default(phandle_args.node, "reg", 0);

	if (!reg) {
		reg = phyaddr;
	}
	return reg;
}
#endif
static int xgmac_start(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	int ret = -1, i = 0;
	u32 val, tx_fifo_sz, rx_fifo_sz, tqs, rqs, pbl;
	ulong last_rx_desc;
	ulong desc_pad;

	struct xgmac_desc *tx_desc = NULL;
	struct xgmac_desc *rx_desc = NULL;
	int reg;

	debug("%s(dev=%p):\n", __func__, dev);

	xgmac->tx_desc_idx = 0;
	xgmac->rx_desc_idx = 0;

	xgmac->reg_access_ok = true;

	/*  setbits_le32(&xgmac->dma_regs->mode,
	                 XGMAC_DMA_MODE_SWR);

	    do {
	        reg = readl(&xgmac->dma_regs->mode);
	        ret = reg & XGMAC_DMA_MODE_SWR;
	        if (!ret) {
	            break;
	        }

	        i++;
	    } while (i < xgmac->config->swr_wait);

	    if (ret) {
	        pr_err("XGMAC_DMA_MODE_SWR stuck: %d\n", ret);
	//      goto err_stop_resets;
	    }
	*/
	/*
	 * if PHY was already connected and configured,
	 * don't need to reconnect/reconfigure again
	 */
	if (!xgmac->phy) {
//		addr = xgmac_get_phy_addr(xgmac, dev);
		xgmac->phy = phy_connect(xgmac->mii, phyaddr, dev,
								 ETH_PHY_INTF);
		if (!xgmac->phy) {
			pr_err("phy_connect() failed\n");
			goto err_stop_resets;
		}

		if (xgmac->max_speed) {
			ret = phy_set_supported(xgmac->phy, xgmac->max_speed);
			if (ret) {
				pr_err("phy_set_supported() failed: %d\n", ret);
				goto err_shutdown_phy;
			}
		}

		xgmac->phy->node = xgmac->phy_of_node;
		ret = phy_config(xgmac->phy);
		if (ret < 0) {
			pr_err("phy_config() failed: %d\n", ret);
			goto err_shutdown_phy;
		}
	}

	ret = phy_startup(xgmac->phy);
	if (ret < 0) {
		pr_err("phy_startup() failed: %d\n", ret);
		goto err_shutdown_phy;
	}

	if (!xgmac->phy->link) {
		pr_err("No link\n");
		goto err_shutdown_phy;
	}

	if (xgmac->phy->interface == PHY_INTERFACE_MODE_SGMII ||
			xgmac->phy->interface == PHY_INTERFACE_MODE_2500BASEX) {
		ret = xgmac_serdes_xpcs_config(dev);
		if (ret < 0) {
			pr_err("xgmac_serdes_xpcs_config() failed: %d\n", ret);
			goto err_shutdown_phy;
		}
	}

	setbits_le32(&xgmac->dma_regs->mode,
				 XGMAC_DMA_MODE_SWR);

	do {
		reg = readl(&xgmac->dma_regs->mode);
		ret = reg & XGMAC_DMA_MODE_SWR;
		if (!ret) {
			break;
		}

		i++;
	} while (i < xgmac->config->swr_wait);

	if (ret) {
		pr_err("XGMAC_DMA_MODE_SWR stuck: %d\n", ret);
//		goto err_stop_resets;
	}

	ret = xgmac_adjust_link(dev);
	if (ret < 0) {
		pr_err("xgmac_adjust_link() failed: %d\n", ret);
		goto err_shutdown_phy;
	}

	/*  if (xgmac->phy->interface == PHY_INTERFACE_MODE_SGMII) {
	        // Wait 0x1F0001 [5] = 1;  #an complete
	        i = 0;
	        do {
	            writel(0x1f00, IOADDR_XPCS_REG_BASE + 0x3fc);
	            reg = readl(IOADDR_XPCS_REG_BASE + 0x4);
	            if (reg & 0x20) {
	                break;
	            } else if (++i >= EPHY_RETRY_COUNT)
	                pr_err("AN Complete fail, reg : 0x%x\n", reg);

	            udelay(10);
	        } while (i < EPHY_RETRY_COUNT);
	    }
	*/
	/* Configure MTL */

	/* Enable Store and Forward mode for TX */
	/* Program Tx operating mode */
	setbits_le32(&xgmac->mtl_regs->txq0_operation_mode,
				 XGMAC_MTL_TXQ0_OPERATION_MODE_TSF |
				 (XGMAC_MTL_TXQ0_OPERATION_MODE_TXQEN_ENABLED <<
				  XGMAC_MTL_TXQ0_OPERATION_MODE_TXQEN_SHIFT));

//	setbits_le32(&xgmac->mtl_regs->mtl_rxq_dma_map0, 0x1);
	/* Transmit Queue weight */
	writel(0x10, &xgmac->mtl_regs->txq0_quantum_weight);

	/* Enable Store and Forward mode for RX, since no jumbo frame */
	setbits_le32(&xgmac->mtl_regs->rxq0_operation_mode,
				 XGMAC_MTL_RXQ0_OPERATION_MODE_RSF);

	/* Transmit/Receive queue fifo size; use all RAM for 1 queue */
	val = readl(&xgmac->mac_regs->hw_feature1);
	tx_fifo_sz = (val >> XGMAC_MAC_HW_FEATURE1_TXFIFOSIZE_SHIFT) &
				 XGMAC_MAC_HW_FEATURE1_TXFIFOSIZE_MASK;
	rx_fifo_sz = (val >> XGMAC_MAC_HW_FEATURE1_RXFIFOSIZE_SHIFT) &
				 XGMAC_MAC_HW_FEATURE1_RXFIFOSIZE_MASK;

	/*
	 * r/tx_fifo_sz is encoded as log2(n / 128). Undo that by shifting.
	 * r/tqs is encoded as (n / 256) - 1.
	 */
	tqs = (128 << tx_fifo_sz) / 256 - 1;
	rqs = (128 << rx_fifo_sz) / 256 - 1;

	clrsetbits_le32(&xgmac->mtl_regs->txq0_operation_mode,
					XGMAC_MTL_TXQ0_OPERATION_MODE_TQS_MASK <<
					XGMAC_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT,
					tqs << XGMAC_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT);
	clrsetbits_le32(&xgmac->mtl_regs->rxq0_operation_mode,
					XGMAC_MTL_RXQ0_OPERATION_MODE_RQS_MASK <<
					XGMAC_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT,
					rqs << XGMAC_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT);

//	setbits_le32(&xgmac->mtl_regs->rxq0_operation_mode,
//		     XGMAC_MTL_RXQ0_OPERATION_MODE_EHFC);

	/* Configure MAC */
	clrsetbits_le32(&xgmac->mac_regs->rxq_ctrl0,
					XGMAC_MAC_RXQ_CTRL0_RXQ0EN_MASK <<
					XGMAC_MAC_RXQ_CTRL0_RXQ0EN_SHIFT,
					xgmac->config->config_mac <<
					XGMAC_MAC_RXQ_CTRL0_RXQ0EN_SHIFT);


	/* Multicast and Broadcast Queue Enable */
//	setbits_le32(&xgmac->mac_regs->rxq_ctrl1,
//		     XGMAC_MAC_RXQ_CTRL1_MCBCQEN);


	/* enable promise mode and receive all mode */
	setbits_le32(&xgmac->mac_regs->mac_packet_filter,
				 XGMAC_MAC_PACKET_FILTER_RA |
				 XGMAC_MAC_PACKET_FILTER_PR);

	/* Set TX flow control parameters */
	/* Set Pause Time */
	setbits_le32(&xgmac->mac_regs->q0_tx_flow_ctrl,
				 0xffff << XGMAC_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT);

	/*  setbits_le32(&xgmac->mac_regs->q0_tx_flow_ctrl,
	            0x0 << XGMAC_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT);
	*/
	/*  setbits_le32(&xgmac->mac_regs->q0_tx_flow_ctrl,
	             XGMAC_MAC_Q0_TX_FLOW_CTRL_PT_MASK <<
	             XGMAC_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT);
	*/
	/* Assign priority for RX flow control */
	/*  clrbits_le32(&xgmac->mac_regs->rxq_ctrl2,
	             XGMAC_MAC_RXQ_CTRL2_PSRQ0_MASK <<
	             XGMAC_MAC_RXQ_CTRL2_PSRQ0_SHIFT);
	*/
	/* Enable flow control */
	setbits_le32(&xgmac->mac_regs->q0_tx_flow_ctrl,
				 XGMAC_MAC_Q0_TX_FLOW_CTRL_TFE);
	setbits_le32(&xgmac->mac_regs->rx_flow_ctrl,
				 XGMAC_MAC_RX_FLOW_CTRL_RFE);

	clrbits_le32(&xgmac->mac_regs->tx_configuration,
//	setbits_le32(&xgmac->mac_regs->tx_configuration,
				 XGMAC_MAC_CONF_JD);

	clrbits_le32(&xgmac->mac_regs->rx_configuration,
//	setbits_le32(&xgmac->mac_regs->rx_configuration,
				 XGMAC_MAC_CONF_JE |
				 XGMAC_MAC_CONF_GPSLCE |
				 XGMAC_MAC_CONF_WD);

	setbits_le32(&xgmac->mac_regs->rx_configuration,
				 XGMAC_MAC_CONF_ACS |
//		     XGMAC_MAC_CONF_LM |
				 XGMAC_MAC_CONF_CST);
	ret = xgmac_write_hwaddr(dev);
	if (ret < 0) {
		pr_err("xgmac_write_hwaddr() failed: %d\n", ret);
		goto err;
	}

	reg = readl(xgmac->dma_ch_regs);
	reg &= ~(0x1 << CH_DIS);
	writel(reg, xgmac->dma_ch_regs);
	/* Configure DMA */
	/*  clrsetbits_le32(&xgmac->dma_regs->sysbus_mode,
	            XGMAC_DMA_SYSBUS_MODE_AAL,
	            XGMAC_DMA_SYSBUS_MODE_EAME |
	            XGMAC_DMA_SYSBUS_MODE_UNDEF);
	*/
	/* Enable OSP mode */
	/*  setbits_le32(&xgmac->dma_regs->ch0_tx_control,
	             XGMAC_DMA_CH0_TX_CONTROL_TSE |
	             XGMAC_DMA_CH0_TX_CONTROL_OSP);
	*/
	/* RX buffer size. Must be a multiple of bus width */
	clrsetbits_le32(&xgmac->dma_regs->ch0_rx_control,
					XGMAC_DMA_CH0_RX_CONTROL_RBSZ_MASK <<
					XGMAC_DMA_CH0_RX_CONTROL_RBSZ_SHIFT,
					XGMAC_MAX_PACKET_SIZE <<
					XGMAC_DMA_CH0_RX_CONTROL_RBSZ_SHIFT);

	desc_pad = (xgmac->desc_size - sizeof(struct xgmac_desc)) /
			   xgmac->config->axi_bus_width;
	setbits_le32(&xgmac->dma_regs->ch0_control,
//		     XGMAC_DMA_CH0_CONTROL_PBLX8 |
				 (desc_pad << XGMAC_DMA_CH0_CONTROL_DSL_SHIFT));
	/*
	 * Burst length must be < 1/2 FIFO size.
	 * FIFO size in tqs is encoded as (n / 256) - 1.
	 * Each burst is n * 8 (PBLX8) * 16 (AXI width) == 128 bytes.
	 * Half of n * 256 is n * 128, so pbl == tqs, modulo the -1.
	 */
	pbl = tqs + 1;
	if (pbl > 32) {
		// coverity[assigned_value]
		pbl = 32;
	}

	clrsetbits_le32(&xgmac->dma_regs->ch0_tx_control,
					XGMAC_DMA_CH0_TX_CONTROL_TXPBL_MASK <<
					XGMAC_DMA_CH0_TX_CONTROL_TXPBL_SHIFT,
					16 << XGMAC_DMA_CH0_TX_CONTROL_TXPBL_SHIFT);

	clrsetbits_le32(&xgmac->dma_regs->ch0_rx_control,
					XGMAC_DMA_CH0_RX_CONTROL_RXPBL_MASK <<
					XGMAC_DMA_CH0_RX_CONTROL_RXPBL_SHIFT,
					16 << XGMAC_DMA_CH0_RX_CONTROL_RXPBL_SHIFT);

	/* DMA performance configuration */
	val = (7 << XGMAC_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT) |
		  (7 << XGMAC_DMA_SYSBUS_MODE_WR_OSR_LMT_SHIFT) |
//		XGMAC_DMA_SYSBUS_MODE_AALE |
//		XGMAC_DMA_SYSBUS_MODE_EAME |
		  XGMAC_DMA_SYSBUS_MODE_AAL |
		  XGMAC_DMA_SYSBUS_MODE_UNDEF;
	/*  val = (XGMAC_DMA_SYSBUS_MODE_RD_OSR_LMT_MASK <<
	           XGMAC_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT) |
	           (XGMAC_DMA_SYSBUS_MODE_WR_OSR_LMT_MASK <<
	           XGMAC_DMA_SYSBUS_MODE_WR_OSR_LMT_SHIFT) |
	           XGMAC_DMA_SYSBUS_MODE_EAME |
	           XGMAC_DMA_SYSBUS_MODE_BLEN16 |
	           XGMAC_DMA_SYSBUS_MODE_BLEN8 |
	           XGMAC_DMA_SYSBUS_MODE_BLEN4 |
	           XGMAC_DMA_SYSBUS_MODE_BLEN32;
	*/
	writel(val, &xgmac->dma_regs->sysbus_mode);

	/* Set up descriptors */

	memset(xgmac->tx_descs, 0, xgmac->desc_size * XGMAC_DESCRIPTORS_TX);
	memset(xgmac->rx_descs, 0, xgmac->desc_size * XGMAC_DESCRIPTORS_RX);

	for (i = 0; i < XGMAC_DESCRIPTORS_TX; i++) {
		tx_desc = (struct xgmac_desc *)xgmac_get_desc(xgmac, i, false);

		xgmac->config->ops->xgmac_flush_desc(tx_desc);
	}

	for (i = 0; i < XGMAC_DESCRIPTORS_RX; i++) {
		rx_desc = (struct xgmac_desc *)xgmac_get_desc(xgmac, i, true);

		rx_desc->des0 = (uintptr_t)(xgmac->rx_dma_buf +
				(i * XGMAC_MAX_PACKET_SIZE));
		rx_desc->des3 = XGMAC_DESC3_OWN;
		/* Flush the cache to the memory */
		mb();
		xgmac->config->ops->xgmac_flush_desc(rx_desc);
		xgmac->config->ops->xgmac_inval_buffer(xgmac->rx_dma_buf +
				(i * XGMAC_MAX_PACKET_SIZE), XGMAC_MAX_PACKET_SIZE);
	}

	writel(0, &xgmac->dma_regs->ch0_txdesc_list_haddress);
	writel((ulong)xgmac_get_desc(xgmac, 0, false),
		   &xgmac->dma_regs->ch0_txdesc_list_address);
	writel(XGMAC_DESCRIPTORS_TX - 1,
		   &xgmac->dma_regs->ch0_txdesc_ring_length);
	writel(0, &xgmac->dma_regs->ch0_rxdesc_list_haddress);
	writel((ulong)xgmac_get_desc(xgmac, 0, true),
		   &xgmac->dma_regs->ch0_rxdesc_list_address);
	writel(XGMAC_DESCRIPTORS_RX - 1,
		   &xgmac->dma_regs->ch0_rxdesc_ring_length);

	/* Enable everything */

	setbits_le32(&xgmac->mac_regs->tx_configuration,
				 XGMAC_MAC_CONF_TE);
	setbits_le32(&xgmac->mac_regs->rx_configuration,
				 XGMAC_MAC_CONF_RE);

	setbits_le32(&xgmac->dma_regs->ch0_tx_control,
				 XGMAC_DMA_CH0_TX_CONTROL_ST);
	setbits_le32(&xgmac->dma_regs->ch0_rx_control,
				 XGMAC_DMA_CH0_RX_CONTROL_SR);

	/* TX tail pointer not written until we need to TX a packet */
	/*
	 * Point RX tail pointer at last descriptor. Ideally, we'd point at the
	 * first descriptor, implying all descriptors were available. However,
	 * that's not distinguishable from none of the descriptors being
	 * available.
	 */
	last_rx_desc = (ulong)xgmac_get_desc(xgmac, XGMAC_DESCRIPTORS_RX - 1, true);
	writel(last_rx_desc, &xgmac->dma_regs->ch0_rxdesc_tail_pointer);

	xgmac->started = true;

	debug("%s: OK\n", __func__);
	return 0;

err_shutdown_phy:
	phy_shutdown(xgmac->phy);
err_stop_resets:
	xgmac->config->ops->xgmac_stop_resets(dev);
err:
	pr_err("FAILED: %d\n", ret);
	return ret;
}

static void xgmac_stop(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	unsigned long start_time;
	u32 val;
	u32 trcsts;
	u32 txqsts;
	u32 prxq;
	u32 rxqsts;

	return;
	debug("%s(dev=%p):\n", __func__, dev);

	// coverity[unreachable], xgmac->started set to true in xgmac_start()
	if (!xgmac->started) {
		return;
	}
	xgmac->started = false;
	xgmac->reg_access_ok = false;

	/* Disable TX DMA */
	clrbits_le32(&xgmac->dma_regs->ch0_tx_control,
				 XGMAC_DMA_CH0_TX_CONTROL_ST);

	/* Wait for TX all packets to drain out of MTL */
	start_time = get_timer(0);

	while (get_timer(start_time) < XGMAC_TIMEOUT_100MS) {
		val = readl(&xgmac->mtl_regs->txq0_debug);

		trcsts = (val >> XGMAC_MTL_TXQ0_DEBUG_TRCSTS_SHIFT) &
				 XGMAC_MTL_TXQ0_DEBUG_TRCSTS_MASK;

		txqsts = val & XGMAC_MTL_TXQ0_DEBUG_TXQSTS;

		if (trcsts != XGMAC_MTL_TXQ0_DEBUG_TRCSTS_READ_STATE && !txqsts) {
			break;
		}
	}

	/* Turn off MAC TX and RX */
	clrbits_le32(&xgmac->mac_regs->tx_configuration,
				 XGMAC_MAC_CONF_RE);
	clrbits_le32(&xgmac->mac_regs->rx_configuration,
				 XGMAC_MAC_CONF_RE);

	/* Wait for all RX packets to drain out of MTL */
	start_time = get_timer(0);

	while (get_timer(start_time) < XGMAC_TIMEOUT_100MS) {
		val = readl(&xgmac->mtl_regs->rxq0_debug);

		prxq = (val >> XGMAC_MTL_RXQ0_DEBUG_PRXQ_SHIFT) &
			   XGMAC_MTL_RXQ0_DEBUG_PRXQ_MASK;

		rxqsts = (val >> XGMAC_MTL_RXQ0_DEBUG_RXQSTS_SHIFT) &
				 XGMAC_MTL_RXQ0_DEBUG_RXQSTS_MASK;

		if (!prxq && !rxqsts) {
			break;
		}
	}

	/* Turn off RX DMA */
	clrbits_le32(&xgmac->dma_regs->ch0_rx_control,
				 XGMAC_DMA_CH0_RX_CONTROL_SR);

	if (xgmac->phy) {
		phy_shutdown(xgmac->phy);
	}

	xgmac->config->ops->xgmac_stop_resets(dev);

	debug("%s: OK\n", __func__);
}

static int xgmac_send(struct udevice *dev, void *packet, int length)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	struct xgmac_desc *tx_desc;
	unsigned long start_time;

	debug("%s(dev=%p, packet=%p, length=%d):\n", __func__, dev, packet,
		  length);

	memcpy(xgmac->tx_dma_buf, packet, length);
	xgmac->config->ops->xgmac_flush_buffer(xgmac->tx_dma_buf, length);

	tx_desc = xgmac_get_desc(xgmac, xgmac->tx_desc_idx, false);
	xgmac->tx_desc_idx++;
	xgmac->tx_desc_idx %= XGMAC_DESCRIPTORS_TX;

	tx_desc->des0 = (ulong)xgmac->tx_dma_buf;
	tx_desc->des1 = 0;
	tx_desc->des2 = length;
	/*
	 * Make sure that if HW sees the _OWN write below, it will see all the
	 * writes to the rest of the descriptor too.
	 */
	mb();
	tx_desc->des3 = XGMAC_DESC3_OWN | XGMAC_DESC3_FD | XGMAC_DESC3_LD | length;
	xgmac->config->ops->xgmac_flush_desc(tx_desc);

	debug("TX DESC: %p [0x%x][0x%x][0x%x][0x%x]\n", xgmac->tx_dma_buf, tx_desc->des0, tx_desc->des1, tx_desc->des2, tx_desc->des3);
	writel((ulong)xgmac_get_desc(xgmac, xgmac->tx_desc_idx, false),
		   &xgmac->dma_regs->ch0_txdesc_tail_pointer);
	debug("tx tail = 0x%lx\n", (ulong)xgmac_get_desc(xgmac, xgmac->tx_desc_idx, false));

	start_time = get_timer(0);

	while (get_timer(start_time) < XGMAC_TIMEOUT_100MS) {
		xgmac->config->ops->xgmac_inval_desc(tx_desc);
		debug("TX DESC inval: %p [0x%x][0x%x][0x%x][0x%x]\n", xgmac->tx_dma_buf, tx_desc->des0, tx_desc->des1, tx_desc->des2, tx_desc->des3);
		if (!(readl(&tx_desc->des3) & XGMAC_DESC3_OWN)) {
			return 0;
		}
	}
	debug("%s: TX timeout\n", __func__);

	return -ETIMEDOUT;
}

static int xgmac_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	struct xgmac_desc *rx_desc;
	int length;

	debug("%s(dev=%p, flags=0x%x):\n", __func__, dev, flags);

	rx_desc = xgmac_get_desc(xgmac, xgmac->rx_desc_idx, true);
	xgmac->config->ops->xgmac_inval_desc(rx_desc);
	if (rx_desc->des3 & XGMAC_DESC3_OWN) {
		debug("%s: RX packet not available\n", __func__);
		return -EAGAIN;
	}

	*packetp = xgmac->rx_dma_buf +
			   (xgmac->rx_desc_idx * XGMAC_MAX_PACKET_SIZE);
	length = rx_desc->des3 & XGMAC_RDES3_PKT_LENGTH_MASK;
	debug("%s: *packetp=%p, length=%d\n", __func__, *packetp, length);

	xgmac->config->ops->xgmac_inval_buffer(*packetp, length);

	return length;
}

static int xgmac_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	u32 idx, idx_mask = xgmac->desc_per_cacheline - 1;
	uchar *packet_expected;
	struct xgmac_desc *rx_desc;

	debug("%s(packet=%p, length=%d)\n", __func__, packet, length);

	packet_expected = xgmac->rx_dma_buf +
					  (xgmac->rx_desc_idx * XGMAC_MAX_PACKET_SIZE);
	if (packet != packet_expected) {
		debug("%s: Unexpected packet (expected %p)\n", __func__,
			  packet_expected);
		return -EINVAL;
	}

	xgmac->config->ops->xgmac_inval_buffer(packet, length);

	if ((xgmac->rx_desc_idx & idx_mask) == idx_mask) {
		for (idx = xgmac->rx_desc_idx - idx_mask;
			 idx <= xgmac->rx_desc_idx;
			 idx++) {
			rx_desc = xgmac_get_desc(xgmac, idx, true);
			rx_desc->des0 = 0;
			/* Flush the cache to the memory */
			mb();
			xgmac->config->ops->xgmac_flush_desc(rx_desc);
			xgmac->config->ops->xgmac_inval_buffer(packet, length);
			rx_desc->des0 = (u32)(ulong)(xgmac->rx_dma_buf +
										 (idx * XGMAC_MAX_PACKET_SIZE));
			rx_desc->des1 = 0;
			rx_desc->des2 = 0;
			/*
			 * Make sure that if HW sees the _OWN write below,
			 * it will see all the writes to the rest of the
			 * descriptor too.
			 */
			mb();
			rx_desc->des3 = XGMAC_DESC3_OWN;
			xgmac->config->ops->xgmac_flush_desc(rx_desc);
		}
		writel((ulong)rx_desc, &xgmac->dma_regs->ch0_rxdesc_tail_pointer);
	}

	xgmac->rx_desc_idx++;
	xgmac->rx_desc_idx %= XGMAC_DESCRIPTORS_RX;

	return 0;
}

static int xgmac_probe_resources_core(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	unsigned int desc_step;
	int ret;

	debug("%s(dev=%p):\n", __func__, dev);

	/* Maximum distance between neighboring descriptors, in Bytes. */
	desc_step = sizeof(struct xgmac_desc);

//	if (desc_step < ARCH_DMA_MINALIGN) {
	/*
	 * The hardware implementation cannot place one descriptor
	 * per cacheline, it is necessary to place multiple descriptors
	 * per cacheline in memory and do cache management carefully.
	 */
	/*      xgmac->desc_size = BIT(fls(desc_step) - 1);
	    } else {
	*/      xgmac->desc_size = ALIGN(sizeof(struct xgmac_desc),
									 (unsigned int)ARCH_DMA_MINALIGN);
//	}
	xgmac->desc_per_cacheline = ARCH_DMA_MINALIGN / xgmac->desc_size;

	xgmac->tx_descs = xgmac_alloc_descs(xgmac, XGMAC_DESCRIPTORS_TX);
	if (!xgmac->tx_descs) {
		debug("%s: xgmac_alloc_descs(tx) failed\n", __func__);
		ret = -ENOMEM;
		goto err;
	}

	xgmac->rx_descs = xgmac_alloc_descs(xgmac, XGMAC_DESCRIPTORS_RX);
	if (!xgmac->rx_descs) {
		debug("%s: xgmac_alloc_descs(rx) failed\n", __func__);
		ret = -ENOMEM;
		goto err_free_tx_descs;
	}

	xgmac->tx_dma_buf = memalign(XGMAC_BUFFER_ALIGN, XGMAC_MAX_PACKET_SIZE);
	if (!xgmac->tx_dma_buf) {
		debug("%s: memalign(tx_dma_buf) failed\n", __func__);
		ret = -ENOMEM;
		goto err_free_descs;
	}
	debug("%s: tx_dma_buf=%p\n", __func__, xgmac->tx_dma_buf);

	xgmac->rx_dma_buf = memalign(XGMAC_BUFFER_ALIGN, XGMAC_RX_BUFFER_SIZE);
	if (!xgmac->rx_dma_buf) {
		debug("%s: memalign(rx_dma_buf) failed\n", __func__);
		ret = -ENOMEM;
		goto err_free_tx_dma_buf;
	}
	debug("%s: rx_dma_buf=%p\n", __func__, xgmac->rx_dma_buf);

	xgmac->rx_pkt = malloc(XGMAC_MAX_PACKET_SIZE);
	if (!xgmac->rx_pkt) {
		debug("%s: malloc(rx_pkt) failed\n", __func__);
		ret = -ENOMEM;
		goto err_free_rx_dma_buf;
	}
	debug("%s: rx_pkt=%p\n", __func__, xgmac->rx_pkt);

	xgmac->config->ops->xgmac_inval_buffer(xgmac->rx_dma_buf,
										   XGMAC_MAX_PACKET_SIZE * XGMAC_DESCRIPTORS_RX);

	debug("%s: OK\n", __func__);
	return 0;

err_free_rx_dma_buf:
	free(xgmac->rx_dma_buf);
err_free_tx_dma_buf:
	free(xgmac->tx_dma_buf);
err_free_descs:
	xgmac_free_descs(xgmac->rx_descs);
err_free_tx_descs:
	xgmac_free_descs(xgmac->tx_descs);
err:

	debug("%s: returns %d\n", __func__, ret);
	return ret;
}

static int xgmac_remove_resources_core(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);

	debug("%s(dev=%p):\n", __func__, dev);

	free(xgmac->rx_pkt);
	free(xgmac->rx_dma_buf);
	free(xgmac->tx_dma_buf);
	xgmac_free_descs(xgmac->rx_descs);
	xgmac_free_descs(xgmac->tx_descs);

	debug("%s: OK\n", __func__);
	return 0;
}

/* board-specific Ethernet Interface initializations. */
__weak int board_interface_eth_init(struct udevice *dev,
									phy_interface_t interface_type)
{
	return 0;
}

static int xgmac_probe(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	struct eth_pdata *plat = dev_get_plat(dev);
	int ret;
	u32 phyval, i;

	printf("%s, %s\n", __func__, XGMAC_VER);

#ifndef FIXED_ETH_PARAMETER
	u8 default_mac_addr[6] = DEFAULT_MAC_ADDRESS;
#else
	u8 default_mac_addr[6] = CFG_ETHADDR;
	u8 default_mac1_addr[6] = CFG_ETH1ADDR;
#endif

	debug("%s(dev=%p):\n", __func__, dev);

	xgmac->dev = dev;
	xgmac->config = (void *)dev_get_driver_data(dev);

	xgmac->regs = dev_read_addr(dev);
	if (xgmac->regs == FDT_ADDR_T_NONE) {
		pr_err("dev_read_addr() failed\n");
		return -ENODEV;
	}
	xgmac->mac_regs = (void *)(xgmac->regs + XGMAC_MAC_REGS_BASE);
	xgmac->mtl_regs = (void *)(xgmac->regs + XGMAC_MTL_REGS_BASE);
	xgmac->dma_regs = (void *)(xgmac->regs + XGMAC_DMA_REGS_BASE);
	xgmac->dma_ch_regs = xgmac->regs + XGMAC_DMA_AXI_CNRT_REFS_BASE;

	xgmac->max_speed = dev_read_u32_default(dev, "max-speed", 0);

	ret = xgmac_probe_resources_core(dev);
	if (ret < 0) {
		pr_err("xgmac_probe_resources_core() failed: %d\n", ret);
		return ret;
	}

	ret = xgmac->config->ops->xgmac_probe_resources(dev);
	if (ret < 0) {
		pr_err("xgmac_probe_resources() failed: %d\n", ret);
		goto err_remove_resources_core;
	}

	ret = xgmac->config->ops->xgmac_start_clks(dev);
	if (ret < 0) {
		pr_err("xgmac_start_clks() failed: %d\n", ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_DM_ETH_PHY)) {
		xgmac->mii = eth_phy_get_mdio_bus(dev);
	}

	if (!xgmac->mii) {
		xgmac->mii = mdio_alloc();
		if (!xgmac->mii) {
			pr_err("mdio_alloc() failed\n");
			ret = -ENOMEM;
			goto err_stop_clks;
		}
		xgmac->mii->read = xgmac_mdio_read;
		xgmac->mii->write = xgmac_mdio_write;
		xgmac->mii->priv = xgmac;
		strncpy(xgmac->mii->name, dev->name, sizeof(xgmac->mii->name) - 1);

		ret = mdio_register(xgmac->mii);
		if (ret < 0) {
			pr_err("mdio_register() failed: %d\n", ret);
			goto err_free_mdio;
		}
	}

	if (xgmac->regs == IOADDR_ETH_REG_BASE) {
		memcpy(plat->enetaddr, default_mac_addr, 6);
	} else {
		memcpy(plat->enetaddr, default_mac1_addr, 6);
	}

	if (!phyaddr) {
		// scan phy address
#ifdef CONFIG_NVT_FPGA_EMULATION
		for (i = 0; i < 32; i++) {
#else
		for (i = 1; i < 32; i++) {
#endif
			phyval = xgmac_mdio_read(xgmac->mii, i, 0, MII_BMSR);
			debug("phyval = 0x%xn", phyval);
			if (phyval != 0x0000 && phyval != 0xffff) {
				phyaddr = i;
				break;
			}
		}
	}
	printf("%s, phyaddr = 0x%x\n", __func__, phyaddr);

	if (IS_ENABLED(CONFIG_DM_ETH_PHY)) {
		eth_phy_set_mdio_bus(dev, xgmac->mii);
	}

	xgmac->phy = phy_connect(xgmac->mii, phyaddr, dev,
							 ETH_PHY_INTF);

	if (!xgmac->phy) {
		pr_err("phy_connect() failed\n");
		goto err_free_mdio;
	}

	ret = phy_config(xgmac->phy);
	if (ret < 0) {
		pr_err("phy_config() failed: %d\n", ret);
		goto err_free_mdio;
	}
	debug("%s: OK\n", __func__);
	return 0;

err_free_mdio:
//	mdio_free(xgmac->mii);
err_stop_clks:
	xgmac->config->ops->xgmac_stop_clks(dev);
err_remove_resources_core:
	xgmac_remove_resources_core(dev);

	debug("%s: returns %d\n", __func__, ret);
	return ret;
}

static int xgmac_remove(struct udevice *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);

	debug("%s(dev=%p):\n", __func__, dev);

	mdio_unregister(xgmac->mii);
	mdio_free(xgmac->mii);
	xgmac->config->ops->xgmac_stop_clks(dev);
	xgmac->config->ops->xgmac_remove_resources(dev);

	xgmac_remove_resources_core(dev);

	debug("%s: OK\n", __func__);
	return 0;
}

int xgmac_null_ops(struct udevice *dev)
{
	return 0;
}

static const struct eth_ops xgmac_ops = {
	.start = xgmac_start,
	.stop = xgmac_stop,
	.send = xgmac_send,
	.recv = xgmac_recv,
	.free_pkt = xgmac_free_pkt,
	.write_hwaddr = xgmac_write_hwaddr,
//	.read_rom_hwaddr = xgmac_read_rom_hwaddr,
};

static const struct udevice_id xgmac_ids[] = {
	{
		.compatible = "nvt,synopsys_dwxgmac",
		.data = (ulong) &xgmac_novatek_config
	},
	{ }
};

U_BOOT_DRIVER(eth_xgmac) = {
	.name = "eth_xgmac",
	.id = UCLASS_ETH,
	.of_match = of_match_ptr(xgmac_ids),
	.probe = xgmac_probe,
	.remove = xgmac_remove,
	.ops = &xgmac_ops,
	.priv_auto = sizeof(struct xgmac_priv),
	.plat_auto = sizeof(struct eth_pdata),
};
