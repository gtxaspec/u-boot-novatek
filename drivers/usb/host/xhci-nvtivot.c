/*
 * Copyright (c) 2021, Novatek Microelectronics Corp.
 * All rights reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <asm/io.h>
#include <errno.h>
#include <usb.h>

#include <usb/xhci.h>
#ifdef CONFIG_DM_USB
#include <dm.h>
#include <fdtdec.h>
#endif
#include <asm/arch/IOAddress.h>
#include <asm/arch/hardware.h>
#include <asm/gpio.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/nvt_common.h>
#include <linux/libfdt.h>
#include <linux/delay.h>
#if (defined(CONFIG_TARGET_NS02201_A64)|| defined(CONFIG_TARGET_NS02201) || defined(CONFIG_TARGET_NA51102) || defined(CONFIG_TARGET_NA51102_A64) || defined(CONFIG_TARGET_NS02302_A64) || defined(CONFIG_TARGET_NS02401_A64))
#include <asm/arch/efuse_protected.h>
#endif

#define DRV_VERSION                     "1.00.19"
#ifdef CONFIG_DM_USB
/* Declare global data pointer */
DECLARE_GLOBAL_DATA_PTR;

struct uboot_xhci_priv {
        struct xhci_ctrl ctrl; /* Needed by EHCI */
        struct xhci_hccr *hccr;
        struct xhci_hcor *hcor;
};
#endif

#if (defined(CONFIG_TARGET_NA51102) || defined(CONFIG_TARGET_NA51102_A64))
#define USB3PHY_SETREG(ofs,value)   writel((value), (volatile void __iomem *)(IOADDR_USB3CTRL_REG_BASE+0x2000+((ofs)<<2)))
#define USB3PHY_GETREG(ofs)         readl((volatile void __iomem *)(IOADDR_USB3CTRL_REG_BASE+0x2000+((ofs)<<2)))

#define USB3U2PHY_SETREG(ofs,value) writel((value), (volatile void __iomem *)(IOADDR_USB3CTRL_REG_BASE+0x1000+((ofs)<<2)))
#define USB3U2PHY_GETREG(ofs)       readl((volatile void __iomem *)(IOADDR_USB3CTRL_REG_BASE+0x1000+((ofs)<<2)))
#elif (defined(CONFIG_TARGET_NS02201) || defined(CONFIG_TARGET_NS02201_A64))

static u32 u3_channel = 0;
static u64 u3_ioaddr = IOADDR_USB3PHY_REG_BASE;

//#if USB3_CH2
//#define IOADDR_U3CTRL_REG_BASE		IOADDR_USB3_2PHY_REG_BASE
//#else
//#define IOADDR_U3CTRL_REG_BASE		IOADDR_USB3PHY_REG_BASE
//#endif

#define USB3PHY_SETREG(ofs,value)   writel((value), (volatile void __iomem *)(u3_ioaddr+0x2000+((ofs)<<2)))
#define USB3PHY_GETREG(ofs)         readl((volatile void __iomem *)(u3_ioaddr+0x2000+((ofs)<<2)))

#define USB3U2PHY_SETREG(ofs,value) writel((value), (volatile void __iomem *)(u3_ioaddr+0x1000+((ofs)<<2)))
#define USB3U2PHY_GETREG(ofs)       readl((volatile void __iomem *)(u3_ioaddr+0x1000+((ofs)<<2)))

#define USB_TRIM_INFO	0

/* NS02302 */
#elif defined(CONFIG_TARGET_NS02302_A64)
static u64 u3_ioaddr = IOADDR_USB3PHY_REG_BASE;
#define USB3PHY_SETREG(ofs,value)   writel((value), (volatile void __iomem *)(u3_ioaddr+0x2000+((ofs)<<2)))
#define USB3PHY_GETREG(ofs)         readl((volatile void __iomem *)(u3_ioaddr+0x2000+((ofs)<<2)))

#define USB3U2PHY_SETREG(ofs,value) writel((value), (volatile void __iomem *)(u3_ioaddr+0x1000+((ofs)<<2)))
#define USB3U2PHY_GETREG(ofs)       readl((volatile void __iomem *)(u3_ioaddr+0x1000+((ofs)<<2)))


/* NS02401 */
#elif defined(CONFIG_TARGET_NS02401_A64)
static u32 u3_channel = 0;
static u64 u2_ioaddr = IOADDR_USBPHY_REG_BASE;
static u64 u3_ioaddr = IOADDR_USB3PHY_REG_BASE;

#define USB3PHY_SETREG(ofs,value)   writel((value), (volatile void __iomem *)(u3_ioaddr+0x2000+((ofs)<<2)))
#define USB3PHY_GETREG(ofs)         readl((volatile void __iomem *)(u3_ioaddr+0x2000+((ofs)<<2)))

#define USB3U2PHY_SETREG(ofs,value) writel((value), (volatile void __iomem *)(u3_ioaddr+0x1000+((ofs)<<2)))
#define USB3U2PHY_GETREG(ofs)       readl((volatile void __iomem *)(u3_ioaddr+0x1000+((ofs)<<2)))

#define USB2PHY_SETREG(ofs,value) writel((value), (volatile void __iomem *)(u2_ioaddr+0x1000+((ofs)<<2)))
#define USB2PHY_GETREG(ofs)       readl((volatile void __iomem *)(u2_ioaddr+0x1000+((ofs)<<2)))

#define USB3PHY_OFS(ofs)            ((ofs - 0x2000) >> 2 )
#define USB3U2PHY_OFS(ofs)          ((ofs - 0x1000) >> 2 )
#define USB2PHY_OFS(ofs)          ((ofs - 0x1000) >> 2 )
#define USB_TRIM_INFO 0
#endif


static void nvtim_set_gpio_power(int index)
{
        /* Enable it after dts parsing ready*/
        int nodeoffset;
        u32 *cell = NULL;
        char path[20] = {0};
        int gpio_pin, gpio_value, err;

	#if  defined(CONFIG_TARGET_NS02401_A64)
        if (index) {
                //for two controller case
                sprintf(path,"/u3h2@%ld,%lx", (IOADDR_GLOBAL_BASE >> 32L), (IOADDR_USB3_2_REG_BASE & 0xFFFFFFFF));
        } else {
                sprintf(path,"/u3h@%ld,%lx", (IOADDR_GLOBAL_BASE >> 32L), (IOADDR_USB3_REG_BASE & 0xFFFFFFFF));
        }
	#elif (defined(CONFIG_TARGET_NS02201) || defined(CONFIG_TARGET_NS02201_A64))
	if (index) {
		//for two controller case
		sprintf(path,"/u3hst@%ld,%lx", (IOADDR_GLOBAL_BASE >> 32L), (IOADDR_USB3_2_REG_BASE & 0xFFFFFFFF));
	} else {
		sprintf(path,"/u3hst@%ld,%lx", (IOADDR_GLOBAL_BASE >> 32L), (IOADDR_USB3_REG_BASE & 0xFFFFFFFF));
	}

	#elif (defined(CONFIG_TARGET_NA51102) || defined(CONFIG_TARGET_NA51102_A64))
	sprintf(path,"/u3hst@%ld,%lx", (IOADDR_GLOBAL_BASE >> 32L), (IOADDR_USB3_REG_BASE & 0xFFFFFFFF));


	#elif defined(CONFIG_TARGET_NS02302_A64)
	sprintf(path,"/u3h@%ld,%lx", (IOADDR_GLOBAL_BASE >> 32L), (IOADDR_USB3_REG_BASE & 0xFFFFFFFF));
	#endif

        nodeoffset = fdt_path_offset((const void*)nvt_fdt_buffer, path);

        cell = (u32*)fdt_getprop((const void*)nvt_fdt_buffer, nodeoffset, "power_gpio", NULL);

        if (cell > 0) {
                gpio_pin = __be32_to_cpu(cell[0]);
                gpio_value = __be32_to_cpu(cell[1]);

                err = gpio_request(gpio_pin, "vbus");
                if (err) {
                        printf("#### failed to request vbus, %d\n", gpio_pin);
                        return;
                } else {
			printf("#### pass to request vbus, %d\n", gpio_pin);
		}

                gpio_direction_output(gpio_pin, !gpio_value);
                mdelay(10);

                gpio_direction_output(gpio_pin, gpio_value);
                mdelay(10);
                gpio_free(gpio_pin);
        } else {
                printf("no power gpio found\n");
        }
}

#ifdef CONFIG_TARGET_NS02401_A64

static void nvt_usb3_top_rst(int rst_mask)
{
	unsigned long tmpval = 0;

	tmpval = readl((volatile unsigned long *)(u3_ioaddr + USB3TOP_RSTCTRL_REG));
	tmpval &= ~(USB3TOP_RSTCTRL_MSK << USB3TOP_U3RSTCTRL_0_OFS);
	tmpval |= (rst_mask << USB3TOP_U3RSTCTRL_0_OFS);
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr + USB3TOP_RSTCTRL_REG));
}

static void nvt_u2phy_reset(void)
{
	unsigned long tmpval = 0;

	if (u3_channel == 1) {
		//printk("ch1 : %d\n", __LINE__);
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U2_2_RSTN_REG));
		tmpval &= ~(0x1 << CG_U2_2PHY_RSTN_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U2_2_RSTN_REG));
		udelay(100);
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U2_2_RSTN_REG));
		tmpval |= (0x1 << CG_U2_2PHY_RSTN_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U2_2_RSTN_REG));
	} else {
		//printk("ch0 : %d\n", __LINE__);
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U2_RSTN_REG));
		tmpval &= ~(0x1 << CG_U2PHY_RSTN_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U2_RSTN_REG));
		udelay(100);
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U2_RSTN_REG));
		tmpval |= (0x1 << CG_U2PHY_RSTN_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U2_RSTN_REG));
	}
}

static void nvt_u3phy_reset(void)
{
	unsigned long tmpval = 0;

	if (u3_channel == 1) {
		//printk("ch1 : %d\n", __LINE__);
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_2_RSTN_REG));
		tmpval &= ~((0x1 << CG_U3_2PHY_RSTN_OFS) | (0x1 << CG_U3_2GLUE_RSTN_OFS));
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_2_RSTN_REG));
		udelay(100);
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_2_RSTN_REG));
		tmpval |= ((0x1 << CG_U3_2PHY_RSTN_OFS) | (0x1 << CG_U3_2GLUE_RSTN_OFS));
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_2_RSTN_REG));
	} else {
		//printk("ch0 : %d\n", __LINE__);
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_RSTN_REG));
		tmpval &= ~((0x1 << CG_U3PHY_RSTN_OFS) | (0x1 << CG_U3GLUE_RSTN_OFS));
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_RSTN_REG));
		udelay(100);
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_RSTN_REG));
		tmpval |= ((0x1 << CG_U3PHY_RSTN_OFS) | (0x1 << CG_U3GLUE_RSTN_OFS));
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_RSTN_REG));
	}
}

#ifndef CONFIG_NVT_FPGA_EMULATION
static void nvt_usb3_ssc_en(void)
{
	writel(0xbe, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x4794));
	//SSC_mulfac (SSC:0.43%)
	writel(0x6, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x4790));
	//SSC_en (enable ssc down mode)
	writel(0x95, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x478c));
}
#endif

static void nvt_usb3_clk_init(void)
{
	unsigned long tmpval = 0;

	/* Enable PLL14 */
	tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE));
	tmpval |= (0x1 << CG_U3MPLL_OFS);
	writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE));

	udelay(10);

	/* Wait PLL14 ready */
	tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3MPLL_REG));
	tmpval = (tmpval >> CG_U3MPLL_OFS) & 0x1;
	while (tmpval == FALSE) {
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3MPLL_REG));
		tmpval = (tmpval >> CG_U3MPLL_OFS) & 0x1;
	}

	/* rstb controlled by u3 top */
	if (u3_channel == 1) {
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_PHY4_RSTB_CTRL_REG));
		tmpval &= ~(0x1 << CG_PHY4_RSTB_CTRL_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_PHY4_RSTB_CTRL_REG));
	} else {
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_PHY_RSTB_CTRL_REG));
		tmpval &= ~(0x1 << CG_PHY_RSTB_CTRL_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_PHY_RSTB_CTRL_REG));
	}

	/* COMBO phy pll source(mpll14) */
	if (u3_channel == 1) {
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_COMBOPHY_SRC_REG));
		tmpval &= ~(0x1 << CG_COMBOPHY_4_SEL_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_COMBOPHY_SRC_REG));
	} else {
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_COMBOPHY_SRC_REG));
		tmpval &= ~(0x1 << CG_COMBOPHY_1_SEL_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_COMBOPHY_SRC_REG));
	}
}

static void nvt_u3ctrl_rst(void)
{
	unsigned long tmpval = 0;

	if (u3_channel == 1) {
		//printk("ch1 : %d\n", __LINE__);
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_2_RSTN_REG));
		tmpval &= ~(0x1 << CG_U3_2_RSTN_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_2_RSTN_REG));
		udelay(100);
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_2_RSTN_REG));
		tmpval |= (0x1 << CG_U3_2_RSTN_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_2_RSTN_REG));
	} else {
		//printk("ch0 : %d\n", __LINE__);
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_RSTN_REG));
		tmpval &= ~(0x1 << CG_U3_RSTN_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_RSTN_REG));
		udelay(100);
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_RSTN_REG));
		tmpval |= (0x1 << CG_U3_RSTN_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_RSTN_REG));
	}
}

static void nvt_u3ctrl_clkinit(void)
{
	unsigned long tmpval = 0;

	if (u3_channel == 1) {
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_2_EN_REG));
		tmpval |= (0x1 << CG_U3_2_EN_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_2_EN_REG));
	} else {
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_EN_REG));
		tmpval |= (0x1 << CG_U3_EN_OFS);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE + CG_U3_EN_REG));
	}

}

static void nvt_u3top_init(void)
{
	unsigned long tmpval = 0;

	/* Close channel disable*/
	tmpval = readl((volatile unsigned long *)(u3_ioaddr + USB3TOP_CTRL1_REG));
	tmpval &= ~(0x1 << USB3TOP_CH_DIS_OFS);
	tmpval |= (0x1 << USB3TOP_RESP_MODE);
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr + USB3TOP_CTRL1_REG));

	tmpval = readl((volatile unsigned long *)(u3_ioaddr + USB3TOP_RSTCTRL_REG));
	tmpval &= ~(0x1 << USB3TOP_USB_ID_OFS); //host mode
	tmpval &= ~(0x1 << USB3TOP_RXTERM_EN_MSK_OFS); //unmask RXTERM_EN_MASK
	tmpval |= (0x1 << USB3TOP_VBUS_VALID_OFS); //VBUSVALID
	//tmpval |= (0x1 << USB3TOP_FORCE_HS_OFS); //HS
	tmpval &= ~(0x1 << USB3TOP_EL_BUF_OFS); //Clear ELASTICITY_BUF_MODE
	tmpval &= ~((0x7 << 16) | (0x7 << 24)); //Clear U3 RESET CTRL & U2 RESET CTRL
	tmpval |= ((0x4 << 16) | (0x4 << 24)); //U3 RESET CTRL(CTL1) + U2 RESET CTRL(CTL1)

	tmpval |= (0x1 << USB3TOP_U3_PM_MODE); //Support PM mode
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr + USB3TOP_RSTCTRL_REG));
}

#ifndef CONFIG_NVT_FPGA_EMULATION
static void bank_switch(int bank)
{
	USB3PHY_SETREG(0xFF, bank);
}

//parameter: (reg_addr, rangebit(H), rangebit(L), value)
static void u3phy_bitmask_set(unsigned int addr, unsigned char start_bit, unsigned char end_bit, unsigned int val)
{
	unsigned int mask;
	unsigned int temp;

	mask = (1 << (start_bit + 1)) - (1 << end_bit);

	temp = USB3PHY_GETREG(USB3PHY_OFS(addr));
	temp &= ~(mask);
	temp |= ((val << end_bit) & mask);
	USB3PHY_SETREG(USB3PHY_OFS(addr), temp);
}

static void u3u2phy_bitmask_set(unsigned int addr, unsigned char start_bit, unsigned char end_bit, unsigned int val)
{
	unsigned int mask;
	unsigned int temp;

	mask = (1 << (start_bit + 1)) - (1 << end_bit);

	temp = USB3U2PHY_GETREG(USB3U2PHY_OFS(addr));
	temp &= ~(mask);
	temp |= ((val << end_bit) & mask);
	USB3U2PHY_SETREG(USB3U2PHY_OFS(addr), temp);
}

static void nvt_u3phy_trim(void)
{
	//=====================================
	//======== Get Trim From Efuse  =======
	//=====================================
	u8 u3_trim_tx_val = 6;    //default TX(for SS)
	u8 u3_trim_rx_val = 0xC;  //default RX(for SS)
	u8 u3_trim_rint_sel = 0x8; //default 12K (for HS)
	u8 u2_trim_swctrl = 6, u2_trim_sqsel = 4;

	u8 TX_TRIM;
	u8 RX_TRIM;
	u8 Internal_12K_resistor;

	u32 trim;
	int code;
	bool is_found;
	s32 addr;

	// confirm channel
	if(u3_channel == 1) {
		addr = EFUSE_TRIM_DATA_SATA_3_USB3_2_PRI;
	} else {
		addr = EFUSE_TRIM_DATA_SATA3_USB3_1_PRI;
	}

	// get trim value of usb column
	code = otp_key_manager(EFUSE_ID_1, addr);
	if(code == -33) {
		//!!!Please apply default value here!!!
		printf("Read U3(TX/RX/12K) trim NULL, use default value\r\n");
	} else {
		// check if usb trim value is valid
		is_found = extract_trim_valid(code, (u32 *)&trim);

		if(is_found) {
			if (USB_TRIM_INFO) {
				printf(" ******* Trim data read success addr[0x%02x] = 0x%04x *****************\r\n", (int)addr, (int)trim);
				printf(" ******* (1) 12K_resistor[4..0] valid from 1(0x1) <-> 20(0x14)*********\r\n");
				printf(" ******* (2) TX trim[8..5] valid from 2(0x2) <-> 14(0xE) **************\r\n");
				printf(" ******* (3) RX trim[12..9] valid from 8(0x8) <-> 14(0xE)**************\r\n");
			}

			Internal_12K_resistor = trim & 0x1F;
			TX_TRIM = ((trim >> 5) & 0xF);
			RX_TRIM = ((trim >> 9) & 0xF);

			//(1) 12K resistor 1<= x <= 20(0x14)
			if (Internal_12K_resistor > 0x14) {
				printf(" 12K_resistor Trim data out range [0x%02x]=0x%04x > 20(0x14)\r\n", (int)addr, (int)Internal_12K_resistor);
			} else if (Internal_12K_resistor < 0x1) {
				printf(" 12K_resistor Trim data out range [0x%02x]=0x%04x < 1(0x1)\r\n", (int)addr, (int)Internal_12K_resistor);
			} else {
				if (USB_TRIM_INFO) {
					printf(" 12K_resistor Trim data range success 0x1 <= [0x%04x] <= 0x14\r\n", (int)Internal_12K_resistor);
				}
				u3_trim_rint_sel = Internal_12K_resistor;
			}

			//(2) TX term bit[8..5] 0x2 <= x <= 0xE
			if (TX_TRIM > 0xE) {
				printf(" TX Trim data error [0x%02x]=0x%04x > 0xE\r\n", (int)addr, (int)TX_TRIM);
			} else if (TX_TRIM < 0x2) {
				printf(" TX Trim data error [0x%02x]=0x%04x < 0x2\r\n", (int)addr, (int)TX_TRIM);
			} else {
				if (USB_TRIM_INFO) {
					printf(" TX Trim data range success 0x2 <= [0x%04x] <= 0xE\r\n", (int)TX_TRIM);
				}
				u3_trim_tx_val = TX_TRIM;
			}
			//(3) RX term bit[12..9] 0x8 <= x <= 0xE
			if (RX_TRIM > 0xE) {
				printf(" RX Trim data error [0x%02x]=0x%04x > 0xE\r\n", (int)addr, (int)RX_TRIM);
			} else if (RX_TRIM < 0x8) {
				printf(" RX Trim data error [0x%02x]=0x%04x < 0x8\r\n", (int)addr, (int)RX_TRIM);
			} else {
				if (USB_TRIM_INFO) {
					printf(" RX Trim data range success 0x8 <= [0x%04x] <= 0xE\r\n", (int)RX_TRIM);
				}
				u3_trim_rx_val = RX_TRIM;
			}


		} else {
			//!!!Please apply default value here!!!
			printf("is found [%d][ USB3] = 0x%08x\r\n", is_found, (int)code);
		}
	}


	//======================================================
	// USB30 12K & TX/RX trim bits
	//======================================================
	//12K src[b7](1: boundary 0: phy internal registers)
	bank_switch(0);
	u3phy_bitmask_set(0x2684, 7, 7, 0);

	USB3PHY_SETREG(USB3PHY_OFS(0x2680), 0x40 + u3_trim_rint_sel);
	USB3PHY_SETREG(USB3PHY_OFS(0x268C), 0x60);

	//TRIM: RX termination
	bank_switch(2);
	u3phy_bitmask_set(0x2000, 3, 0, u3_trim_rx_val);
	//TRIM: TX termination
	bank_switch(0);
	u3phy_bitmask_set(0x2040, 7, 4, u3_trim_tx_val);

	//======================================================
	// USB20 swing & sqsel
	//======================================================
	u3u2phy_bitmask_set(0x1018, 3, 1, u2_trim_swctrl);
	u3u2phy_bitmask_set(0x1014, 4, 2, u2_trim_sqsel);

	//R45 Calibration
	USB3U2PHY_SETREG(USB3U2PHY_OFS(0x1144), 0x20);
	udelay(24);
	USB3U2PHY_SETREG(USB3U2PHY_OFS(0x1144), 0x00);
	udelay(24);

}

static void nvt_u2phy_best_setting(void)
{
	//=====================================
	//=========== U2 Best setting =========
	//=====================================
	bank_switch(0);
	USB3PHY_SETREG(USB3PHY_OFS(0x2158), 0x74);
	USB3U2PHY_SETREG(USB3U2PHY_OFS(0x1024), 0xB0);
	USB2PHY_SETREG(USB2PHY_OFS(0x1024), 0xB0);
	USB3U2PHY_SETREG(USB3U2PHY_OFS(0x1048), 0x31);
	USB2PHY_SETREG(USB2PHY_OFS(0x1048), 0x31);
	USB3U2PHY_SETREG(USB3U2PHY_OFS(0x1018), 0x1C);
	USB2PHY_SETREG(USB2PHY_OFS(0x1018), 0x1C);
	//printk("nvt_u2phy_best_setting: done\n");

}

static void nvt_u3phy_best_setting(void)
{
	//=====================================
	//============ APHY setting ===========
	//=====================================
	bank_switch(0);
	USB3PHY_SETREG(USB3PHY_OFS(0x2524), 0xFF);
	USB3PHY_SETREG(USB3PHY_OFS(0x2514), 0xFF);
	bank_switch(1);
	USB3PHY_SETREG(USB3PHY_OFS(0x25A0), 0xEF);
	USB3PHY_SETREG(USB3PHY_OFS(0x2420), 0x2);
	USB3PHY_SETREG(USB3PHY_OFS(0x2680), 0x5F);
	USB3PHY_SETREG(USB3PHY_OFS(0x2688), 0xF4);
	USB3PHY_SETREG(USB3PHY_OFS(0x2578), 0xFF);
	USB3PHY_SETREG(USB3PHY_OFS(0x2088), 0x0);
	USB3PHY_SETREG(USB3PHY_OFS(0x25B0), 0x0);
	USB3PHY_SETREG(USB3PHY_OFS(0x25C0), 0x0);
	USB3PHY_SETREG(USB3PHY_OFS(0x25DC), 0xB9);
	USB3PHY_SETREG(USB3PHY_OFS(0x208C), 0x23);
	USB3PHY_SETREG(USB3PHY_OFS(0x2694), 0x1C);
	USB3PHY_SETREG(USB3PHY_OFS(0x2168), 0x8);
	USB3PHY_SETREG(USB3PHY_OFS(0x22E4), 0xC0);
	bank_switch(2);
	USB3PHY_SETREG(USB3PHY_OFS(0x20A0), 0xF7);
	USB3PHY_SETREG(USB3PHY_OFS(0x200C), 0x21);
	USB3PHY_SETREG(USB3PHY_OFS(0x2008), 0xEC);
	USB3PHY_SETREG(USB3PHY_OFS(0x2078), 0x10);
	USB3PHY_SETREG(USB3PHY_OFS(0x20CC), 0xA0);
	USB3PHY_SETREG(USB3PHY_OFS(0x2028), 0x0F);
	USB3PHY_SETREG(USB3PHY_OFS(0x202C), 0x3B);
	USB3PHY_SETREG(USB3PHY_OFS(0x207C), 0x7C);
	bank_switch(2);
	USB3PHY_SETREG(USB3PHY_OFS(0x20A8), 0x80);
	bank_switch(0);
	USB3PHY_SETREG(USB3PHY_OFS(0x2190), 0x44);
	USB3PHY_SETREG(USB3PHY_OFS(0x2194), 0x99);
	bank_switch(0);
	USB3PHY_SETREG(USB3PHY_OFS(0x2158), 0x74);
	USB3PHY_SETREG(USB3PHY_OFS(0x2050), 0x8A);
	USB3PHY_SETREG(USB3PHY_OFS(0x2048), 0xE8);
	USB3PHY_SETREG(USB3PHY_OFS(0x20D0), 0xFC);
	USB3PHY_SETREG(USB3PHY_OFS(0x2450), 0x04);
	USB3PHY_SETREG(USB3PHY_OFS(0x2188), 0x0D);
	//=====================================
	//============ U3 EXIT patch ==========
	//=====================================
	/* bit[3:0] = u30p_ele2_dly_time[3:0]; 1 = 128us , 4 = 512us, default = 0x8; */
	USB3PHY_SETREG(USB3PHY_OFS(0x29EC), 0x51);
	/* bit0 u30p_rxanaif_P1_skp_recalib_en */
	u3phy_bitmask_set(0x2504, 0, 0, 0x01);
	//temp = USB3PHY_GETREG(USB3PHY_OFS(0x2504));
	//temp |= 0x1;
	//USB3PHY_SETREG(USB3PHY_OFS(0x2504), temp);

	//=====================================
	//=========== U3 Best setting =========
	//=====================================
	/* #U3 : add mechanism to leave from WSS state, for small swing RX LFPS */
	bank_switch(0);
	u3phy_bitmask_set(0x2600, 4, 0, 0x08);

	/* #U3 : PIPE powerdown deglitch */
	bank_switch(0);
	//u3phy_bitmask_set(0x2954, 7, 7, 0x00);
	//USB3PHY_SETREG(USB3PHY_OFS(0x295C), 0x00);
	u3phy_bitmask_set(0x2954, 7, 7, 0x01);
	USB3PHY_SETREG(USB3PHY_OFS(0x295C), 0x10);


	/* #U3 : phystatus & rxstatus keeper */
	bank_switch(0);
	u3phy_bitmask_set(0x29DC, 7, 6, 0x02);

	/* #U3 : RX LFPS detect @ SS (for TD7.40) */
	bank_switch(0);
	u3phy_bitmask_set(0x28A4, 4, 4, 0x00);
	u3phy_bitmask_set(0x25B8, 3, 0, 0x00);
	u3phy_bitmask_set(0x25BC, 0, 0, 0x01);

	/* #U3 : go ELE1 state when P2/P3 */
	bank_switch(0);
	u3phy_bitmask_set(0x29F4, 1, 1, 0x00);

	/* #U3 : modification of rx_ss_ready asserted/deasserted machenism */
	bank_switch(0);
	u3phy_bitmask_set(0x25B8, 7, 4, 0x01);

	/* #U3 : reload EOC value after P1/P2/P3 -> P0 */
	bank_switch(0);
	u3phy_bitmask_set(0x25BC, 4, 4, 0x00);

	/* #U3 : elecidle follows rx_valid. To avoid rxelecidle abnorrmal, if link partner still transmit logic idle after LPMA */
	bank_switch(0);
	u3phy_bitmask_set(0x28A4, 5, 4, 0x00);
	u3phy_bitmask_set(0x28A4, 7, 7, 0x00);
	u3phy_bitmask_set(0x2900, 4, 4, 0x00);
	USB3PHY_SETREG(USB3PHY_OFS(0x2958), 0x00);

	/* #U3 : extend TX detect RX  delay */
	bank_switch(0);
	u3phy_bitmask_set(0x29FC, 6, 5, 0x00);

	/* PHY_bounary_reset (anaif_pc_rst)*/
	USB3PHY_SETREG(USB3PHY_OFS(0x2408), 0x01);
	mdelay(1);
	USB3PHY_SETREG(USB3PHY_OFS(0x2408), 0x00);

	//printk("nvt_u3phy_best_setting: done\n");
}

#if 0
static void hw_print(void)
{
	UINT32 u3_phy_base = 0;

	if(u3_channel == 1) {
		printk("hwdump: u3 channel 1\n");
		u3_phy_base = 0xF0A20000;
	} else {
		printk("hwdump: u3 channel 0\n");
		u3_phy_base = 0xF0490000;
	}

	printk("============================================\n");
	printk("============ usb2 trim setting  ============\n");
	printk("============================================\n");
	printk("[TRIM_S][U2] 0x%x = (0x%x)\n", (u3_phy_base + 0x2680), USB3PHY_GETREG(USB3PHY_OFS(0x2680)));

	printk("============================================\n");
	printk("============ USB3 trim setting  ============\n");
	printk("============================================\n");
	// TX trim
	bank_switch(0);
	printk("[TRIM_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2040), USB3PHY_GETREG(USB3PHY_OFS(0x2040)));
	// RX trim
	bank_switch(2);
	printk("[TRIM_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2000), USB3PHY_GETREG(USB3PHY_OFS(0x2000)));

	printk("============================================\n");
	printk("============ USB2 best setting  ============\n");
	printk("============================================\n");
	bank_switch(0);
	printk("[BEST_S][U2] 0x%x = (0x%x)\n", (u3_phy_base + 0x2158), USB3PHY_GETREG(USB3PHY_OFS(0x2158)));
	printk("[BEST_S][U2] 0x%x = (0x%x)\n", (u3_phy_base + 0x1024), USB3U2PHY_GETREG(USB3U2PHY_OFS(0x1024)));
	printk("[BEST_S][U2] 0x%x = (0x%x)\n", (u3_phy_base + 0x1048), USB3U2PHY_GETREG(USB3U2PHY_OFS(0x1048)));
	printk("[BEST_S][U2] 0x%x = (0x%x)\n", (u3_phy_base + 0x1018), USB3U2PHY_GETREG(USB3U2PHY_OFS(0x1018)));


	printk("============================================\n");
	printk("============ USB3 best setting  ============\n");
	printk("============================================\n");
	printk("[APHY setting]#############################################################################\n");
	bank_switch(0);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2524), USB3PHY_GETREG(USB3PHY_OFS(0x2524)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2514), USB3PHY_GETREG(USB3PHY_OFS(0x2514)));
	bank_switch(1);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x25A0), USB3PHY_GETREG(USB3PHY_OFS(0x25A0)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2420), USB3PHY_GETREG(USB3PHY_OFS(0x2420)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2680), USB3PHY_GETREG(USB3PHY_OFS(0x2680)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2688), USB3PHY_GETREG(USB3PHY_OFS(0x2688)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2578), USB3PHY_GETREG(USB3PHY_OFS(0x2578)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2088), USB3PHY_GETREG(USB3PHY_OFS(0x2088)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x25B0), USB3PHY_GETREG(USB3PHY_OFS(0x25B0)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x25C0), USB3PHY_GETREG(USB3PHY_OFS(0x25C0)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x25DC), USB3PHY_GETREG(USB3PHY_OFS(0x25DC)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x208C), USB3PHY_GETREG(USB3PHY_OFS(0x208C)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2694), USB3PHY_GETREG(USB3PHY_OFS(0x2694)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2168), USB3PHY_GETREG(USB3PHY_OFS(0x2168)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x22E4), USB3PHY_GETREG(USB3PHY_OFS(0x22E4)));
	bank_switch(2);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x20A0), USB3PHY_GETREG(USB3PHY_OFS(0x20A0)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x200C), USB3PHY_GETREG(USB3PHY_OFS(0x200C)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2008), USB3PHY_GETREG(USB3PHY_OFS(0x2008)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2078), USB3PHY_GETREG(USB3PHY_OFS(0x2078)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x20CC), USB3PHY_GETREG(USB3PHY_OFS(0x20CC)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2028), USB3PHY_GETREG(USB3PHY_OFS(0x2028)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x202C), USB3PHY_GETREG(USB3PHY_OFS(0x202C)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x207C), USB3PHY_GETREG(USB3PHY_OFS(0x207C)));
	bank_switch(2);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x20A8), USB3PHY_GETREG(USB3PHY_OFS(0x20A8)));
	bank_switch(0);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2190), USB3PHY_GETREG(USB3PHY_OFS(0x2190)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2194), USB3PHY_GETREG(USB3PHY_OFS(0x2194)));
	bank_switch(0);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2158), USB3PHY_GETREG(USB3PHY_OFS(0x2158)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2050), USB3PHY_GETREG(USB3PHY_OFS(0x2050)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2048), USB3PHY_GETREG(USB3PHY_OFS(0x2048)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x20D0), USB3PHY_GETREG(USB3PHY_OFS(0x20D0)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2450), USB3PHY_GETREG(USB3PHY_OFS(0x2450)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2188), USB3PHY_GETREG(USB3PHY_OFS(0x2188)));

	printk("[U3 EXIT patch]############################################################################\n");
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x29EC), USB3PHY_GETREG(USB3PHY_OFS(0x29EC)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2504), USB3PHY_GETREG(USB3PHY_OFS(0x2504)));

	printk("[U3 Best setting ]#########################################################################\n");
	bank_switch(0);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2600), USB3PHY_GETREG(USB3PHY_OFS(0x2600)));
	bank_switch(0);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2954), USB3PHY_GETREG(USB3PHY_OFS(0x2954)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x295C), USB3PHY_GETREG(USB3PHY_OFS(0x295C)));
	bank_switch(0);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x29DC), USB3PHY_GETREG(USB3PHY_OFS(0x29DC)));
	bank_switch(0);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x28A4), USB3PHY_GETREG(USB3PHY_OFS(0x28A4)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x25B8), USB3PHY_GETREG(USB3PHY_OFS(0x25B8)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x25BC), USB3PHY_GETREG(USB3PHY_OFS(0x25BC)));
	bank_switch(0);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x29F4), USB3PHY_GETREG(USB3PHY_OFS(0x29F4)));
	bank_switch(0);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x25B8), USB3PHY_GETREG(USB3PHY_OFS(0x25B8)));
	bank_switch(0);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x25BC), USB3PHY_GETREG(USB3PHY_OFS(0x25BC)));
	bank_switch(0);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x28A4), USB3PHY_GETREG(USB3PHY_OFS(0x28A4)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x28A4), USB3PHY_GETREG(USB3PHY_OFS(0x28A4)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2900), USB3PHY_GETREG(USB3PHY_OFS(0x2900)));
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x2958), USB3PHY_GETREG(USB3PHY_OFS(0x2958)));
	bank_switch(0);
	printk("[BEST_S][U3] 0x%x = (0x%x)\n", (u3_phy_base + 0x29FC), USB3PHY_GETREG(USB3PHY_OFS(0x29FC)));

}
#endif
#endif //CONFIG_NVT_FPGA_EMULATION

#endif //CONFIG_TARGET_NS02401_A64


#ifndef CONFIG_NVT_FPGA_EMULATION
#ifndef CONFIG_TARGET_NS02401_A64
static void nvt_phyinit(void)
{
#if (defined(CONFIG_TARGET_NA51102) || defined(CONFIG_TARGET_NA51102_A64))
	unsigned long tmpval = 0, temp = 0;
	u8 tx_term_trim_val = 6;
	u8 rx_term_trim_val = 12;
	u8 u3_trim_rint_sel=8;
	u32 trim;
	int code;
	bool is_found;

	code = otp_key_manager(EFUSE_USB_SATA_TRIM_DATA);
	if (code == -33) {
		printf("Read usb trim error\n");
		printf("[%d] data = NULL\n", 5);
	} else {
		is_found = extract_trim_valid(code, (u32 *)&trim);
		if (is_found) {
			printf("  USB(SATA) Trim data = 0x%04x\r\n", (int)trim);
			u3_trim_rint_sel = trim & 0x1F;
			tx_term_trim_val = ((trim >> 5) & 0xF);
			rx_term_trim_val = ((trim >> 9) & 0xF);
			if (u3_trim_rint_sel > 0x14) {
				u3_trim_rint_sel=8;
				printf("12K_resistor Trim data error 0x%04x > 20(0x14)\r\n", (int)u3_trim_rint_sel);
			} else if (u3_trim_rint_sel < 0x1) {
				u3_trim_rint_sel=8;
				printf("12K_resistor Trim data error 0x%04x < 1(0x1)\r\n", (int)u3_trim_rint_sel);
			} else {
				printf("  *12K_resistor Trim data range success 0x1 <= [0x%04x] <= 0x14\r\n", (int)u3_trim_rint_sel);
			}
			//TX term bit[8..5] 0x2 <= x <= 0xE
			if (tx_term_trim_val > 0xE) {
				tx_term_trim_val = 6;
				printf("USB(SATA) TX Trim data error 0x%04x > 0xE\r\n", (int)tx_term_trim_val);
			} else if (tx_term_trim_val < 0x2) {
				tx_term_trim_val = 6;
				printf("USB(SATA) TX Trim data error 0x%04x < 0x2\r\n", (int)tx_term_trim_val);
			} else {
				printf("  *USB(SATA) TX Trim data range success 0x2 <= [0x%04x] <= 0xE\r\n", (int)tx_term_trim_val);
			}
			//RX term bit[12..9] 0x8 <= x <= 0xE
			if (rx_term_trim_val > 0xE) {
				rx_term_trim_val = 12;
				printf("USB(SATA) RX Trim data error 0x%04x > 0xE\r\n", (int)rx_term_trim_val);
			} else if (rx_term_trim_val < 0x8) {
				rx_term_trim_val = 12;
				printf("USB(SATA) RX Trim data error 0x%04x < 0x8\r\n", (int)rx_term_trim_val);
			} else {
				printf("  *USB(SATA) RX Trim data 0x8 <= [0x%04x] <= 0xE\r\n", (int)rx_term_trim_val);
			}
		} else {
			//!!!Please apply default value here!!!
			//printf("is found [%d][USB trim] = 0x%08x\r\n", is_found, (int)code);
		}
	}

	//======================================================
	//(1) PHY RINT enable										[NEW]
	//======================================================
	// PHY_RES = 0xD
	tmpval = readl((volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x38));
	tmpval |= 0xD01;
	writel(tmpval, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x38));

	// Boundary /PHY control
	// Bank 0
	USB3PHY_SETREG(0xFF, 0x00);

	// RINT_EN bias enable internal control selection
	// 0 : from boundary
	// 1 : from PHY internal register
	tmpval = readl((volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x2680));
	tmpval &= ~0x20;
	writel(tmpval, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x2680));
	//======================================================
	//(2) USB30 PHY 12k trim bits mux selection  0X1a1[7] = 0		[NEW]
	//======================================================
	tmpval = readl((volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x2684));
	tmpval &= ~0x80;
	writel(tmpval, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x2684));

	USB3PHY_SETREG(0x1A0, 0x40+u3_trim_rint_sel);
	USB3PHY_SETREG(0x1A3, 0x60);
	//======================================================
	//(3) USB20 swing & sqsel trim bits removed in INT98530
	//======================================================
	//========================================================
	//add additonal setting for 0.85V
	//========================================================
	// Bank 2
	USB3PHY_SETREG(0xFF, 0x02);
	// bit[5:4]  = LDO_VLDO_SEL_DCO[1:0] ; 00 = 0.85v
	USB3PHY_SETREG(0x2A, 0x80);
	// Bank 0
	USB3PHY_SETREG(0xFF, 0x00);
	// bit[1:0]  = LDO_SEL_18[1:0] ; 00 = 0.85v
	USB3PHY_SETREG(0x64, 0x44);
	// bit[1:0] = TXPLL_CP[1:0] = 01
	USB3PHY_SETREG(0x65, 0X99);

	//========================================================
	//swing
	//========================================================
	// Bank 0
	USB3PHY_SETREG(0xFF, 0x00);
	// bit[5:4] = V_VCHSEL[1:0] = 11
	USB3PHY_SETREG(0x56, 0X74);
	// TX_AMP_CTL=2, TX_DEC_EM_CTL=8
	USB3PHY_SETREG(0x14, 0X8A);
	// bit[5:4] = TX_AMP_CTL_LFPS[1:0] (from 10 -> 11)
	//USB3PHY_SETREG(0x12, 0XE8);
	USB3PHY_SETREG(0x12, 0XF8);
	// bit3 : TX_DE_EN manual mode
	// bit[7:4]: TX_DEC_EM_CTL manual mode
	USB3PHY_SETREG(0x34, 0Xfc);

	//======================================================
	//(4) TX termination trim bits								[NEW]
	//======================================================
	// 0xF05F2040[7:4] = tx_term_trim_val[3:0]
	// Bank 0
	USB3PHY_SETREG(0xFF, 0x00);

	temp = USB3PHY_GETREG(0x10);
	temp &= ~(0xF0);
	temp |= (tx_term_trim_val<<4);
	USB3PHY_SETREG(0x10 , temp);

	//======================================================
	//(5) RX termination trim bits								[NEW]
	//======================================================
	// Bank 2
	USB3PHY_SETREG(0xFF, 0x02);

	temp = USB3PHY_GETREG(0x0);
	temp &= ~(0xF);
	temp |= (rx_term_trim_val);
	USB3PHY_SETREG(0x00 , temp);

	// Bank 0
	USB3PHY_SETREG(0xFF, 0x00);

	//======================================================
	//(6) T12 USB30 APHY setting								[NEW]
	//======================================================
	USB3PHY_SETREG(0xFF ,0x00);
	USB3PHY_SETREG(0x149,0xFF);
	USB3PHY_SETREG(0x145,0xFF);
	USB3PHY_SETREG(0xFF ,0x01);
	USB3PHY_SETREG(0x168,0xEF);
	USB3PHY_SETREG(0x108,0x02);
	USB3PHY_SETREG(0x1A0,0x5F);
	USB3PHY_SETREG(0x1A2,0xF4);
	USB3PHY_SETREG(0x15E,0xFF);
	USB3PHY_SETREG(0x22 ,0x00);
	USB3PHY_SETREG(0x16C,0x00);
	USB3PHY_SETREG(0x170,0x00);
	USB3PHY_SETREG(0x174,0xB9);
	USB3PHY_SETREG(0x23 ,0x13);
	USB3PHY_SETREG(0x1A5,0x2C);
	USB3PHY_SETREG(0x5A ,0x08);
	USB3PHY_SETREG(0xB9 ,0xC0);
	USB3PHY_SETREG(0xFF ,0x02);
	USB3PHY_SETREG(0x28 ,0xF7);
	USB3PHY_SETREG(0x3  ,0x21);
	USB3PHY_SETREG(0x2  ,0xEC);
	USB3PHY_SETREG(0x1E ,0x10);
	USB3PHY_SETREG(0x33 ,0xA0);
	USB3PHY_SETREG(0xA  ,0x0F);
	USB3PHY_SETREG(0xB  ,0x3B);
	USB3PHY_SETREG(0x1F ,0x7C);
	//USB3PHY_SETREG(0x2A ,0x90);
	USB3PHY_SETREG(0xFF ,0x00);

	/* asd_mode=1 (toggle rate) */
	USB3PHY_SETREG(0x198, 0x04);
	/* RX_ICTRL's offset = 0 */
	//USB3PHY_SETREG(0x1BF, 0x40);
	/* TX_AMP_CTL=2, TX_DEC_EM_CTL=8 */
	USB3PHY_SETREG(0x014, 0x8a);
	/* TX_LFPS_AMP_CTL = 1 */
	USB3PHY_SETREG(0x034, 0xfc);//default 0xfc
	/* PHY Power Mode Change ready reponse time. (3 is 1ms.)(4 is 1.3ms.) */

	//USB3PHY_SETREG(0x114, 0x0B);
	USB3PHY_SETREG(0x114, 0x04);
	//USB3PHY_SETREG(0x152, 0x2E);
	//USB3PHY_SETREG(0x153, 0x01);
	//USB3PHY_SETREG(0x1B0, 0xC0);
	//USB3PHY_SETREG(0x1B1, 0x91);
	//USB3PHY_SETREG(0x1B2, 0x00);
	//USB3PHY_SETREG(0x135, 0x88);
	//USB3PHY_SETREG(0x12A, 0x50);
	//USB3PHY_SETREG(0x1F0, 0x80);
	//USB3PHY_SETREG(0x1F5, 0x01|(u3_trim_icdr<<4));//0xB1
	//USB3PHY_SETREG(0x105, 0x01);
	USB3PHY_SETREG(0x056, 0x74);//disconnect level to 0x3
	udelay(2);

	USB3PHY_SETREG(0x102, 0xF0);
	udelay(10);
	USB3PHY_SETREG(0x102, 0x00);
	udelay(300);

	//USB3PHY_SETREG(0x103, 0x01);
	//udelay(100);

	USB3U2PHY_SETREG(0x51, 0x20);
	udelay(2);
	USB3U2PHY_SETREG(0x51, 0x00);
	udelay(2);

	if(1) {
		temp = USB3PHY_GETREG(0x011);
		temp |= 0x1;
		USB3PHY_SETREG(0x011, temp);

		temp = USB3PHY_GETREG(0x031);
		temp |= 0x1;
		USB3PHY_SETREG(0x031, temp);
	}

	USB3PHY_SETREG(0x029, 0x1);

	USB3U2PHY_SETREG(0x09, 0xb0);
	USB3U2PHY_SETREG(0x12, 0x31);
#elif (defined(CONFIG_TARGET_NS02201) || defined(CONFIG_TARGET_NS02201_A64))
	unsigned long tmpval = 0, temp = 0;
	u8 tx_term_trim_val = 6;
	u8 rx_term_trim_val = 12;
	u8 u3_trim_rint_sel=8;
	u8 u3_2_rint_sel=8;

	u32 trim;
	int code;
	bool is_found;
	//s32       addr;

	if(u3_channel == 1)
	{
		code = otp_key_manager(EFUSE_TRIM_DATA_USB_SATA_PRI);
		//addr = 0x05;
	} else {
		code = otp_key_manager(EFUSE_TRIM_DATA_USB3_1_USB2_DUAL_PHY_PRI);
		//addr = 0x2A;
	}

	if (code == -33) {
		//printf("Read usb trim error\n");
		//printf("[%d] data = NULL\n", 5);
		printf("USB param default\n");
	} else {
		is_found = extract_trim_valid(code, (u32 *)&trim);
		if (is_found) {
#if USB_TRIM_INFO
			printf("  USB Trim data = 0x%04x\r\n", (int)trim);
#endif
			u3_trim_rint_sel = trim & 0x1F;
			tx_term_trim_val = ((trim >> 5) & 0xF);
			rx_term_trim_val = ((trim >> 9) & 0xF);
			if (u3_trim_rint_sel > 0x14) {
				u3_trim_rint_sel=8;
				printf("12K_resistor Trim data error 0x%04x > 20(0x14)\r\n", (int)u3_trim_rint_sel);
			} else if (u3_trim_rint_sel < 0x1) {
				u3_trim_rint_sel=8;
				printf("12K_resistor Trim data error 0x%04x < 1(0x1)\r\n", (int)u3_trim_rint_sel);
			} else {
#if USB_TRIM_INFO
				printf("  *12K_resistor Trim data range success 0x1 <= [0x%04x] <= 0x14\r\n", (int)u3_trim_rint_sel);
#endif
			}
			//TX term bit[8..5] 0x2 <= x <= 0xE
			if (tx_term_trim_val > 0xE) {
				tx_term_trim_val = 6;
				printf("USB(SATA) TX Trim data error 0x%04x > 0xE\r\n", (int)tx_term_trim_val);
			} else if (tx_term_trim_val < 0x2) {
				tx_term_trim_val = 6;
				printf("USB(SATA) TX Trim data error 0x%04x < 0x2\r\n", (int)tx_term_trim_val);
			} else {
#if USB_TRIM_INFO
				printf("  *USB(SATA) TX Trim data range success 0x2 <= [0x%04x] <= 0xE\r\n", (int)tx_term_trim_val);
#endif
			}
			//RX term bit[12..9] 0x8 <= x <= 0xE
			if (rx_term_trim_val > 0xE) {
				rx_term_trim_val = 12;
				printf("USB(SATA) RX Trim data error 0x%04x > 0xE\r\n", (int)rx_term_trim_val);
			} else if (rx_term_trim_val < 0x8) {
				rx_term_trim_val = 12;
				printf("USB(SATA) RX Trim data error 0x%04x < 0x8\r\n", (int)rx_term_trim_val);
			} else {
#if USB_TRIM_INFO
				printf("  *USB(SATA) RX Trim data 0x8 <= [0x%04x] <= 0xE\r\n", (int)rx_term_trim_val);
#endif
			}
		} else {
			//!!!Please apply default value here!!!
			//printf("is found [%d][USB trim] = 0x%08x\r\n", is_found, (int)code);
		}
	}

	if(u3_channel == 1)
	{
		code = otp_key_manager(EFUSE_TRIM_USB_LDO_PRI);
		//addr = 0x39;

		if (code == -33) {
			printf("USB param default\n");
			//printf("[%d] data = NULL\n", 5);
		} else {
			is_found = extract_trim_valid(code, (u32 *)&trim);
			if (is_found) {
#if USB_TRIM_INFO
				printf("  USB3_2 single port Trim data = 0x%04x\r\n", (int)trim);
#endif
				u3_2_rint_sel = trim & 0x1F;
				if (u3_2_rint_sel > 0x14) {
					u3_2_rint_sel=8;
					printf("12K_resistor Trim data error 0x%04x > 20(0x14)\r\n", (int)u3_2_rint_sel);
				} else if (u3_2_rint_sel < 0x1) {
					u3_2_rint_sel=8;
					printf("12K_resistor Trim data error 0x%04x < 1(0x1)\r\n", (int)u3_2_rint_sel);
				} else {
#if USB_TRIM_INFO
					printf("  *12K_resistor Trim data range success 0x1 <= [0x%04x] <= 0x14\r\n", (int)u3_2_rint_sel);
#endif
				}
			} else {
				//!!!Please apply default value here!!!
				//printf("is found [%d][USB trim] = 0x%08x\r\n", is_found, (int)code);
			}
		}
#if 0
		USB3U2PHY_SETREG(0x00 , 0x88);
		USB3U2PHY_SETREG(0x04 , 0x08);
		USB3U2PHY_SETREG(0x37 , 0x6e);
		USB3U2PHY_SETREG(0x35 , 0x35);
		USB3U2PHY_SETREG(0x34 , 0xff);
		USB3U2PHY_SETREG(0x52 , 0x60+u3_2_rint_sel);
		USB3U2PHY_SETREG(0x50 , 0x13);
		udelay(60);
		USB3U2PHY_SETREG(0x50 , 0x17);
		udelay(60);
		temp = USB3U2PHY_GETREG(0x52);
		printf("12K_resistor Trim data [0xF0B11148]=0x%04x\r\n", (int)temp);
		//Rintcal enable (set 1 to enable , set 0 to reset)
		USB3U2PHY_SETREG(0x50 , 0x14);
		udelay(30);
#else
		USB3U2PHY_SETREG(0x51 , 0x20);
		USB3U2PHY_SETREG(0x50 , 0x30);
		USB3U2PHY_SETREG(0x52 , 0x60+u3_2_rint_sel);
		USB3U2PHY_SETREG(0x51 , 0x00);
#endif
		//Port0 VLDOSEL = 00
		USB3U2PHY_SETREG(0x00 , 0x00);
		//Port0 PLL_RG_LEVEL = 0
		USB3U2PHY_SETREG(0x54 , 0x00);
	}

	//======================================================
	//(1) PHY RINT enable										[NEW]
	//======================================================
	// PHY_RES = 0xD
	tmpval = readl((volatile unsigned long *)(u3_ioaddr+0x38));
	tmpval |= 0xD01;
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr+0x38));

	// Boundary /PHY control
	// Bank 0
	USB3PHY_SETREG(0xFF, 0x00);

	// RINT_EN bias enable internal control selection
	// 0 : from boundary
	// 1 : from PHY internal register
	tmpval = readl((volatile unsigned long *)(u3_ioaddr+0x2680));
	tmpval &= ~0x20;
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr+0x2680));
	//======================================================
	//(2) USB30 PHY 12k trim bits mux selection  0X1a1[7] = 0		[NEW]
	//======================================================
	tmpval = readl((volatile unsigned long *)(u3_ioaddr+0x2684));
	tmpval &= ~0x80;
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr+0x2684));

	USB3PHY_SETREG(0x1A0, 0x40+u3_trim_rint_sel);
	USB3PHY_SETREG(0x1A3, 0x60);
	//======================================================
	//(3) USB20 swing & sqsel trim bits removed in INT98530
	//======================================================
	//========================================================
	//add additonal setting for 0.85V
	//========================================================
	// Bank 2
	USB3PHY_SETREG(0xFF, 0x02);
	// bit[5:4]  = LDO_VLDO_SEL_DCO[1:0] ; 00 = 0.85v
	USB3PHY_SETREG(0x2A, 0x80);
	// Bank 0
	USB3PHY_SETREG(0xFF, 0x00);
	// bit[1:0]  = LDO_SEL_18[1:0] ; 00 = 0.85v
	USB3PHY_SETREG(0x64, 0x44);
	// bit[1:0] = TXPLL_CP[1:0] = 01
	USB3PHY_SETREG(0x65, 0X99);

	//========================================================
	//swing
	//========================================================
	// Bank 0
	USB3PHY_SETREG(0xFF, 0x00);
	// bit[5:4] = V_VCHSEL[1:0] = 11
	USB3PHY_SETREG(0x56, 0X74);
	// TX_AMP_CTL=2, TX_DEC_EM_CTL=8
	USB3PHY_SETREG(0x14, 0X8A);
	// bit[5:4] = TX_AMP_CTL_LFPS[1:0] (from 10 -> 11)
	//USB3PHY_SETREG(0x12, 0XE8);
	USB3PHY_SETREG(0x12, 0XE8);
	// bit3 : TX_DE_EN manual mode
	// bit[7:4]: TX_DEC_EM_CTL manual mode
	USB3PHY_SETREG(0x34, 0Xfc);

	//======================================================
	//(4) TX termination trim bits								[NEW]
	//======================================================
	// 0xF05F2040[7:4] = tx_term_trim_val[3:0]
	// Bank 0
	USB3PHY_SETREG(0xFF, 0x00);

	temp = USB3PHY_GETREG(0x10);
	temp &= ~(0xF0);
	temp |= (tx_term_trim_val<<4);
	USB3PHY_SETREG(0x10 , temp);

	//======================================================
	//(5) RX termination trim bits								[NEW]
	//======================================================
	// Bank 2
	USB3PHY_SETREG(0xFF, 0x02);

	temp = USB3PHY_GETREG(0x0);
	temp &= ~(0xF);
	temp |= (rx_term_trim_val);
	USB3PHY_SETREG(0x00 , temp);

	// Bank 0
	USB3PHY_SETREG(0xFF, 0x00);

	//======================================================
	//(6) T12 USB30 APHY setting								[NEW]
	//======================================================
	USB3PHY_SETREG(0xFF ,0x00);
	USB3PHY_SETREG(0x149,0xFF);
	USB3PHY_SETREG(0x145,0xFF);
	USB3PHY_SETREG(0xFF ,0x01);
	USB3PHY_SETREG(0x168,0xEF);
	USB3PHY_SETREG(0x108,0x02);
	USB3PHY_SETREG(0x1A0,0x5F);
	USB3PHY_SETREG(0x1A2,0xF4);
	USB3PHY_SETREG(0x15E,0xFF);
	USB3PHY_SETREG(0x22 ,0x00);
	USB3PHY_SETREG(0x16C,0x00);
	USB3PHY_SETREG(0x170,0x00);
	USB3PHY_SETREG(0x174,0xB9);
	USB3PHY_SETREG(0x23 ,0x13);
	USB3PHY_SETREG(0x1A5,0x2C);
	USB3PHY_SETREG(0x5A ,0x08);
	USB3PHY_SETREG(0xB9 ,0xC0);
	USB3PHY_SETREG(0xFF ,0x02);
	USB3PHY_SETREG(0x28 ,0xF7);
	USB3PHY_SETREG(0x3  ,0x21);
	USB3PHY_SETREG(0x2  ,0xEC);
	USB3PHY_SETREG(0x1E ,0x10);
	USB3PHY_SETREG(0x33 ,0xA0);
	USB3PHY_SETREG(0xA  ,0x0F);
	USB3PHY_SETREG(0xB  ,0x3B);
	USB3PHY_SETREG(0x1F ,0x7C);
	//USB3PHY_SETREG(0x2A ,0x90);
	USB3PHY_SETREG(0xFF ,0x00);

	/* asd_mode=1 (toggle rate) */
	USB3PHY_SETREG(0x198, 0x04);
	/* RX_ICTRL's offset = 0 */
	//USB3PHY_SETREG(0x1BF, 0x40);
	/* TX_AMP_CTL=2, TX_DEC_EM_CTL=8 */
	USB3PHY_SETREG(0x014, 0x8a);
	/* TX_LFPS_AMP_CTL = 1 */
	USB3PHY_SETREG(0x034, 0xfc);//default 0xfc
	/* PHY Power Mode Change ready reponse time. (3 is 1ms.)(4 is 1.3ms.) */

	//////////////////////////////////// suspend <-> resume begine
	//USB3PHY_SETREG(0x114, 0x0B);
	USB3PHY_SETREG(0x114, 0x04);
	// //bit[3:0] = u30p_ele2_dly_time[3:0]
	//   1 = 128us , 4 = 512us, default = 0x8
	USB3PHY_SETREG(0x27b, 0x51);

	//bit3 u30p_rxanaif_all_skp_recalib_en
	//0xf05f2504[0] = 1
	temp = USB3PHY_GETREG(0x141);
	temp |= 0x1;
	USB3PHY_SETREG(0x141, temp);

	//phystatus_tmr_en (timer enable)
	//(0xf05f2504[4] )
	//phystatus_period[2:0] , 1 = 1us
	//(0xf05f2504[7:5] )
	temp = USB3PHY_GETREG(0x141);
	temp |= 0x50;
	USB3PHY_SETREG(0x141, temp);

	//bit7 : u30p_rxanaif_ss_ready_mode=1
	temp = USB3PHY_GETREG(0x140);
	temp |= 0x80;
	USB3PHY_SETREG(0x140, temp);
	//////////////////////////////////// suspend <-> resume end
	//USB3PHY_SETREG(0x152, 0x2E);
	//USB3PHY_SETREG(0x153, 0x01);
	//USB3PHY_SETREG(0x1B0, 0xC0);
	//USB3PHY_SETREG(0x1B1, 0x91);
	//USB3PHY_SETREG(0x1B2, 0x00);
	//USB3PHY_SETREG(0x135, 0x88);
	//USB3PHY_SETREG(0x12A, 0x50);
	//USB3PHY_SETREG(0x1F0, 0x80);
	//USB3PHY_SETREG(0x1F5, 0x01|(u3_trim_icdr<<4));//0xB1
	//USB3PHY_SETREG(0x105, 0x01);
	USB3PHY_SETREG(0x056, 0x74);//disconnect level to 0x3
	udelay(2);

	USB3PHY_SETREG(0x102, 0xF0);
	udelay(10);
	USB3PHY_SETREG(0x102, 0x00);
	udelay(300);

	//USB3PHY_SETREG(0x103, 0x01);
	//udelay(100);

	USB3U2PHY_SETREG(0x51, 0x20);
	udelay(2);
	USB3U2PHY_SETREG(0x51, 0x00);
	udelay(2);

	if(1) {
		temp = USB3PHY_GETREG(0x011);
		temp |= 0x1;
		USB3PHY_SETREG(0x011, temp);

		temp = USB3PHY_GETREG(0x031);
		temp |= 0x1;
		USB3PHY_SETREG(0x031, temp);
	}

	USB3PHY_SETREG(0x029, 0x1);
	if(u3_channel == 1) {
		USB3U2PHY_SETREG(0x5, 0xd3);
	}

	USB3U2PHY_SETREG(0x09, 0xb0);
	USB3U2PHY_SETREG(0x12, 0x31);
	// tx swing  & de-emphasis swing  (690 add)
	USB3U2PHY_SETREG(0x6, 0x1c);

#elif defined(CONFIG_TARGET_NS02302_A64)
	u8 u3_trim_swctrl = 4, u3_trim_sqsel = 4;
	u8 u3_trim_rint_sel = 8;
	u32 temp;
	u8 tx_term_trim_val = 0x6;

	//======================================================
	// Get USB30 trim
	//======================================================
	#if 1
	{
		u32 trim;
		int code;
		bool is_found;

		code = otp_key_manager(EFUSE_TRIM_DATA_USB_PRI);
		if (code == -33) {
			printf("Read usb trim error\n");
			printf("[%d] data = NULL\n", EFUSE_TRIM_DATA_USB_PRI);
		} else {
			is_found = extract_trim_valid(code, (u32 *)&trim);
			if (is_found) {
				printf("  USB(SATA) Trim data = 0x%04x\r\n", (int)trim);
				u3_trim_rint_sel = trim & 0x1F;
				tx_term_trim_val = ((trim >> 5) & 0xF);

				if (u3_trim_rint_sel > 0x14) {
					u3_trim_rint_sel=8;
					printf("12K_resistor Trim data error 0x%04x > 20(0x14)\r\n", (int)u3_trim_rint_sel);
				} else if (u3_trim_rint_sel < 0x1) {
					u3_trim_rint_sel=8;
					printf("12K_resistor Trim data error 0x%04x < 1(0x1)\r\n", (int)u3_trim_rint_sel);
				} else {
					printf("  *12K_resistor Trim data range success 0x1 <= [0x%04x] <= 0x14\r\n", (int)u3_trim_rint_sel);
				}
				//TX term bit[8..5] 0x2 <= x <= 0xE
				if (tx_term_trim_val > 0xE) {
					tx_term_trim_val = 6;
					printf("USB(SATA) TX Trim data error 0x%04x > 0xE\r\n", (int)tx_term_trim_val);
				} else if (tx_term_trim_val < 0x2) {
					tx_term_trim_val = 6;
					printf("USB(SATA) TX Trim data error 0x%04x < 0x2\r\n", (int)tx_term_trim_val);
				} else {
					printf("  *USB(SATA) TX Trim data range success 0x2 <= [0x%04x] <= 0xE\r\n", (int)tx_term_trim_val);
				}
			} else {
				//!!!Please apply default value here!!!
				printf("is found [%d][USB trim] = 0x%08x\r\n", is_found, (int)code);
				u3_trim_rint_sel=8;
				tx_term_trim_val = 6;
			}
		}
	}
	#endif //USB3_CONFIG_TRIM END

	//======================================================
	// USB30 12K & TX trim bits
	//======================================================

	temp = USB3PHY_GETREG(0x102);
	pr_info("U3 internal block reset(0x2408): 0x%d\n", temp);

	temp = USB3PHY_GETREG(0x115);
	pr_info("U3 RX/TX termination(0x2454): 0x%d\n", temp);

	// step1: 12K trim set
	USB3PHY_SETREG(0x1A3, 0x60);

	temp = USB3PHY_GETREG(0x1A1);
	temp &= ~(0x80);
	USB3PHY_SETREG(0x1A1 , temp);

	USB3PHY_SETREG(0x1A0, 0x40 + u3_trim_rint_sel);

	// step2: tx trim
	//bit[7:4] :TX_ZTX_CTL force mode
	USB3PHY_SETREG(0x30, 0xF0);
	//0x40[7:4] = tx_term_trim_val[3:0]
	temp = USB3PHY_GETREG(0x10);
	temp &= ~(0xF0);
	temp |= (tx_term_trim_val<<4);
	USB3PHY_SETREG(0x10 , temp);

	//======================================================
	// USB20 swing & sqsel trim bits
	//======================================================
	temp = USB3U2PHY_GETREG(0x06);
	temp &= ~(0x7<<1);
	temp |= (u3_trim_swctrl<<1);
	USB3U2PHY_SETREG(0x06, temp);

	temp = USB3U2PHY_GETREG(0x05);
	temp &= ~(0x7<<2);
	temp |= (u3_trim_sqsel<<2);
	USB3U2PHY_SETREG(0x05, temp);

	//R45 Calibration
	USB3U2PHY_SETREG(0x51, 0x20);
	udelay(2);
	USB3U2PHY_SETREG(0x51, 0x00);
	udelay(2);

	//======================================================
	// USB20 Best setting on NT98538
	//======================================================
	//host mode
	USB3U2PHY_SETREG(0x60, 0x01);
	USB3U2PHY_SETREG(0xC, 0x20);
	USB3U2PHY_SETREG(0xB, 0x20);
	USB3U2PHY_SETREG(0x63, 0x60);
	USB3PHY_SETREG(0x56, 0x74);
	USB3U2PHY_SETREG(0x12, 0x31); //RX clk inverse

	//======================================================
	// USB30 Best setting on NT98538
	//======================================================
	USB3PHY_SETREG(0x1BF, 0x40); //EOC RX_ICTRL's offset = 0
	USB3PHY_SETREG(0x12, 0x28);  //bit[5:4] : TX_AMP_CTL_LFPS[1:0]
	USB3PHY_SETREG(0x14, 0x8A);  //Follow 336 for TX CTS
	USB3PHY_SETREG(0x34, 0xFC);
	USB3PHY_SETREG(0x114, 0x04); //PHY_PWR_MODE_CHG_RDY_RSP = 1.3ms
	USB3PHY_SETREG(0x152, 0x2E); //LB of VBS TUNE FREQ CNT
	USB3PHY_SETREG(0x153, 0x01); //UB of VBS TUNE FREQ CNT
	USB3PHY_SETREG(0x1B0, 0xC0); //EOC 2-data sampler
	USB3PHY_SETREG(0x1B1, 0x91); //LB of EOC THR
	USB3PHY_SETREG(0x1B2, 0x00); //UB of EOC THR
	USB3PHY_SETREG(0x135, 0x88); //SET VGA = FORCE MODE
	USB3PHY_SETREG(0x12A, 0x50); //SET VGA = 5
	USB3PHY_SETREG(0x105, 0x0); //Remove RX_ZRX OFS
	USB3PHY_SETREG(0x27B, 0x51);
	USB3PHY_SETREG(0x141, 0x51);
	USB3PHY_SETREG(0x140, 0x80);
	USB3PHY_SETREG(0x56, 0x74);  //bit[5:4] = BIAS_VCH_SEL[1:0] = 2'b11
	USB3PHY_SETREG(0x11, 0x11);  //bit[0] TX_PDB
	USB3PHY_SETREG(0x31, 0x1);   //bit[0] SM_TX_PDB
	USB3PHY_SETREG(0x13, 0x14);  //bit[2] TX_DET1_EN force value = 1
	USB3PHY_SETREG(0x102, 0x20); //ANAIF_PC_RST = 1
	udelay(1);
	USB3PHY_SETREG(0x102, 0x00); //ANAIF_PC_RST = 0
#endif
	//printf("nvt_phyinit done\r\n");
}
#endif //n CONFIG_TARGET_NS02401_A64
#endif //n CONFIG_NVT_FPGA_EMULATION

#ifndef CONFIG_DM_USB
/*
 * Create the appropriate control structures to manage a new XHCI host
 * controller.
 */
int xhci_hcd_init(int index, struct xhci_hccr **ret_hccr,
		  struct xhci_hcor **ret_hcor)
{
	struct xhci_hccr *hccr;
	struct xhci_hcor *hcor;
	int len;
	unsigned long tmpval, temp;
	u8 u3_trim_rint_sel=8,u3_trim_swctrl=4, u3_trim_sqsel=4,u3_trim_icdr=0xB;
#if defined(CONFIG_TARGET_NA51102_A64)
        unsigned long tmpval2 = 0;
#endif

#if (defined(CONFIG_TARGET_NA51102) || defined(CONFIG_TARGET_NA51102_A64))
	/* Enable PLL20 */
	tmpval = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE));
	tmpval |= (0x1<<20);
	writel(tmpval, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE));

	udelay(10);

	tmpval2 = readl((volatile unsigned long *)(IOADDR_USB3PHY_REG_BASE+0x28));

	//Reset u2/u3 phy
	tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
	tmpval &= ~(0x5<<5);
	writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
	udelay(10);

        /* Disable Reset*/
	tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
	tmpval |= 0x5<<5;
	writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));

	/* reset u2 phy */
	if (tmpval2 & 0x80) {
		//do nothing, since xhci has do the phy reset
	} else {
		// enable phy reset
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
		tmpval &= ~(0x1<<7);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));

		udelay(10);

		//disable phy reset
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
		tmpval |= (0x1<<7);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));

		//mark the reset flag
		tmpval2 |= 0x80;
		writel(tmpval2, (volatile unsigned long *)(IOADDR_USB3PHY_REG_BASE+0x28));
	}

       /* Enable USB3 clock*/
	tmpval = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x140));
	tmpval |= (0x1<<5);
	writel(tmpval, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x140));

	mdelay(10);

       /* Close channel disable*/
	tmpval = readl((volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x14));
	tmpval &= 0xFFFFFDFF;
	writel(tmpval, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x14));

	tmpval = readl((volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x10));
	tmpval &= ~0x2;
	tmpval &= ~(0x1 << 11);
	tmpval |= (0x1 << 0);
	tmpval &= ~((0x7 << 16) | (0x7 << 24) | (0x1 << 4));
	tmpval |= ((0x4 << 16) | (0x4 << 24));
	writel(tmpval, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x10));

	udelay(100);

#elif (defined(CONFIG_TARGET_NS02201) || defined(CONFIG_TARGET_NS02201_A64))
	/* Enable PLL14 */
	tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE));
	tmpval |= (0x1<<14);
	writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE));

	udelay(10);

	//SSC_period (Fssc:31.6kHz)
	writel(0xbe, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x4794));
	//SSC_mulfac (SSC:0.43%)
	writel(0x6, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x4790));
	//SSC_en (enable ssc down mode)
	writel(0x95, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x478c));

	 /* Enable USB3 clock*/
	tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x70));
	if(u3_channel == 1)
		tmpval |= (0x1<<7);
	else
		tmpval |= (0x1<<6);

	writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x70));

	mdelay(10);

       /* Close channel disable*/
	tmpval = readl((volatile unsigned long *)(u3_ioaddr+0x14));
	tmpval &= 0xFFFFFDFF;
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr+0x14));

	tmpval = readl((volatile unsigned long *)(u3_ioaddr+0x10));
	tmpval &= ~0x2;
	tmpval &= ~(0x1 << 11);
	tmpval |= (0x1 << 0);
	tmpval &= ~((0x7 << 16) | (0x7 << 24) | (0x1 << 4));
	tmpval |= ((0x4 << 16) | (0x4 << 24));
	//Support PM mode
	tmpval |= (0x1 << 21);
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr+0x10));

	udelay(100);
#elif defined(CONFIG_TARGET_NS02302_A64)
	/* Enable PLL14 */
	tmpval = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE));
	tmpval |= (0x1<<14);
	writel(tmpval, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE));

	udelay(10);

	//SSC_period (Fssc:31.6kHz)
	writel(0xbe, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x4594));
	//SSC_mulfac (SSC:0.43%)
	writel(0x6, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x4590));
	//SSC_en (enable ssc down mode)
	writel(0x95, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x458c));

	udelay(10);

	//ctrl reset
	tmpval = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	tmpval &= ~(0x1<<15);
	writel(tmpval, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	udelay(10);

	/* Disable Reset*/
	tmpval = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	tmpval |= 0x1<<15;
	writel(tmpval, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	udelay(10);

	/* USB3_CLKEN */
	tmpval = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x70));
	tmpval |= (0x1<<2);
	writel(tmpval, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x70));

	mdelay(10);

	/* Close channel disable & open resp mode*/
	tmpval = readl((volatile unsigned long *)(u3_ioaddr+0x14));
	tmpval &= ~(0x1 << 9); //close channel disable
	tmpval |= (0x1 << 1); // open response mode
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr+0x14));

	tmpval = readl((volatile unsigned long *)(u3_ioaddr+0x10));
	tmpval &= ~(0x1 << 1); // USB_ID as host
	tmpval &= ~(0x1 << 11); //unmask RXTERM_EN_MASK
	tmpval |= (0x1 << 0); //USB VBUS high
	tmpval &= ~((0x7 << 16) | (0x7 << 24) | (0x1 << 4)); //Clear U3 RESET CTRL & U2 RESET CTRL & ELASTICITY_BUF_MODE
	tmpval |= ((0x4 << 16) | (0x4 << 24)); //U3 RESET CTRL(CTL1) + U2 RESET CTRL(CTL1)
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr+0x10));
#endif

	#ifndef CONFIG_NVT_FPGA_EMULATION
	nvt_phyinit();
	#endif

	hccr = (struct xhci_hccr *)IOADDR_USB3_REG_BASE;
	len = HC_LENGTH(xhci_readl(&hccr->cr_capbase));
	hcor = (struct xhci_hcor *)((unsigned long)hccr + len);

	printk("XHCI init hccr 0x%lx and hcor 0x%lx hc_length 0x%x ver %s\n",
		(unsigned long)hccr, (unsigned long)hcor, len, DRV_VERSION);

	*ret_hccr = hccr;
	*ret_hcor = hcor;

	return 0;
}

/*
 * Destroy the appropriate control structures corresponding * to the XHCI host
 * controller
 */
void xhci_hcd_stop(int index)
{
#if defined(CONFIG_TARGET_NA51102_A64)
	unsigned long tmpval2 = 0;

	/* clear reset flag*/
	tmpval2 = readl((volatile unsigned long *)(IOADDR_USB3PHY_REG_BASE+0x28));
	if (tmpval2 & 0x80) {
		tmpval2 &= ~(0x80);
		writel(tmpval2, (volatile unsigned long *)(IOADDR_USB3PHY_REG_BASE+0x28));
	} else {
		//do nothing.
	}
#endif

}


#else
/*
 * Create the appropriate control structures to manage a new XHCI host
 * controller.
 */
int dm_xhci_hcd_init(int index, struct xhci_hccr **ret_hccr,
		struct xhci_hcor **ret_hcor)
{
	struct xhci_hccr *hccr;
	struct xhci_hcor *hcor;
	int len;
	#ifndef CONFIG_TARGET_NS02401_A64
	unsigned long tmpval = 0;
	#endif

	#if defined(CONFIG_TARGET_NA51102_A64)
        unsigned long tmpval2 = 0;
	#endif


#if (defined(CONFIG_TARGET_NA51102) || defined(CONFIG_TARGET_NA51102_A64))
	/* Enable PLL20 */
	tmpval = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE));
	tmpval |= (0x1<<20);
	writel(tmpval, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE));

	udelay(10);

	tmpval2 = readl((volatile unsigned long *)(IOADDR_USB3PHY_REG_BASE+0x28));

	//Reset u2/u3 phy
	tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
	tmpval &= ~(0x5<<5);
	writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
	udelay(10);

        /* Disable Reset*/
	tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
	tmpval |= 0x5<<5;
	writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));

	/* reset u2 phy */
	if (tmpval2 & 0x80) {
		//do nothing, since xhci has do the phy reset
	} else {
		// enable phy reset
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
		tmpval &= ~(0x1<<7);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));

		udelay(10);

		//disable phy reset
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
		tmpval |= (0x1<<7);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));

		//mark the reset flag
		tmpval2 |= 0x80;
		writel(tmpval2, (volatile unsigned long *)(IOADDR_USB3PHY_REG_BASE+0x28));
	}

       /* Enable USB3 clock*/
	tmpval = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x140));
	tmpval |= (0x1<<5);
	writel(tmpval, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x140));

	mdelay(10);

       /* Close channel disable*/
	tmpval = readl((volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x14));
	tmpval &= 0xFFFFFDFF;
	writel(tmpval, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x14));

	tmpval = readl((volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x10));
	tmpval &= ~0x2;
	tmpval &= ~(0x1 << 11);
	tmpval |= (0x1 << 0);
	tmpval &= ~((0x7 << 16) | (0x7 << 24) | (0x1 << 4));
	tmpval |= ((0x4 << 16) | (0x4 << 24));
	writel(tmpval, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x10));

	udelay(100);
#elif (defined(CONFIG_TARGET_NS02201) || defined(CONFIG_TARGET_NS02201_A64))
	//printf("u3_ioaddr  = 0x%llx\n",u3_ioaddr);

	/* Enable PLL14 */
	tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE));
	tmpval |= (0x1<<14);
	writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE));

	udelay(10);

	//ctrl reset
	if(u3_channel == 1){
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));
		tmpval &= ~(0x1<<6);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));
		udelay(10);

	        /* Disable Reset*/
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));
		tmpval |= 0x1<<6;
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));
	} else {
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x94));
		tmpval &= ~(0x1<<26);
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x94));
		udelay(10);

	        /* Disable Reset*/
		tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x94));
		tmpval |= 0x1<<26;
		writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x94));
	}
	udelay(10);

	//SSC_period (Fssc:31.6kHz)
	writel(0xbe, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x4794));
	//SSC_mulfac (SSC:0.43%)
	writel(0x6, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x4790));
	//SSC_en (enable ssc down mode)
	writel(0x95, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x478c));

	 /* Enable USB3 clock*/
	tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x70));
	if(u3_channel == 1)
		tmpval |= (0x1<<7);
	else
		tmpval |= (0x1<<6);

	writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x70));

	mdelay(10);

       /* Close channel disable*/
	tmpval = readl((volatile unsigned long *)(u3_ioaddr+0x14));
	tmpval &= 0xFFFFFDFF;
	tmpval |= 0x2;
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr+0x14));

	tmpval = readl((volatile unsigned long *)(u3_ioaddr+0x10));
	tmpval &= ~0x2;
	tmpval &= ~(0x1 << 11);
	tmpval |= (0x1 << 0);
	tmpval &= ~((0x7 << 16) | (0x7 << 24) | (0x1 << 4));
	tmpval |= ((0x4 << 16) | (0x4 << 24));

	//Support PM mode
	tmpval |= (0x1 << 21);
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr+0x10));
	
	udelay(100);

#elif defined(CONFIG_TARGET_NS02302_A64)
	//printf("u3_ioaddr  = 0x%llx\n",u3_ioaddr);

	/* Enable PLL14 */
	tmpval = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE));
	tmpval |= (0x1<<14);
	writel(tmpval, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE));

	udelay(10);

	//SSC_period (Fssc:31.6kHz)
	writel(0xbe, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x4594));
	//SSC_mulfac (SSC:0.43%)
	writel(0x6, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x4590));
	//SSC_en (enable ssc down mode)
	writel(0x95, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x458c));

	udelay(10);

	//ctrl reset
	tmpval = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	tmpval &= ~(0x1<<15);
	writel(tmpval, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	udelay(10);

	/* Disable Reset*/
	tmpval = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	tmpval |= 0x1<<15;
	writel(tmpval, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	udelay(10);

	/* USB3_CLKEN */
	tmpval = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x70));
	tmpval |= (0x1<<2);
	writel(tmpval, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x70));

	mdelay(10);

	/* Close channel disable & open resp mode*/
	tmpval = readl((volatile unsigned long *)(u3_ioaddr+0x14));
	tmpval &= ~(0x1 << 9); //close channel disable
	tmpval |= (0x1 << 1); // open response mode
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr+0x14));

	tmpval = readl((volatile unsigned long *)(u3_ioaddr+0x10));
	tmpval &= ~(0x1 << 1); // USB_ID as host
	tmpval &= ~(0x1 << 11); //unmask RXTERM_EN_MASK
	tmpval |= (0x1 << 0); //USB VBUS high
	tmpval &= ~((0x7 << 16) | (0x7 << 24) | (0x1 << 4)); //Clear U3 RESET CTRL & U2 RESET CTRL & ELASTICITY_BUF_MODE
	tmpval |= ((0x4 << 16) | (0x4 << 24)); //U3 RESET CTRL(CTL1) + U2 RESET CTRL(CTL1)
	writel(tmpval, (volatile unsigned long *)(u3_ioaddr+0x10));

	udelay(100);


#elif defined(CONFIG_TARGET_NS02401_A64)
	//======================================================
	// [phy_init][0] PHY reset
	//======================================================
	/* u2 phy reset */
	nvt_u2phy_reset();

	/* u3 phy reset */
	nvt_u3phy_reset();

	//======================================================
	// [phy_init][1] MPLL14 enable 500Mhz
	//======================================================
	nvt_usb3_clk_init();

	//======================================================
	// [phy_init][2] U3 top mask
	//======================================================
	nvt_usb3_top_rst(0); //(U3PHY_RESET_CTR0/U3PHY_RESET_CTR1/U3PHY_RESET_CTR2) = (0, 0 ,0)

	//======================================================
	// [phy_init][3] PHY trim & best setting
	//======================================================
	#ifndef CONFIG_NVT_FPGA_EMULATION
	nvt_u3phy_trim();
	nvt_u2phy_best_setting();
	nvt_u3phy_best_setting();
	#endif

	//======================================================
	// [phy_init][4] U3 top mask
	//======================================================
	nvt_usb3_top_rst(0x7); // (U3PHY_RESET_CTR0/U3PHY_RESET_CTR1/U3PHY_RESET_CTR2) = (1, 1 ,1)

	/* SSC enable */
	#ifndef CONFIG_NVT_FPGA_EMULATION
	nvt_usb3_ssc_en();
	#endif

	//======================================================
	// [probe][NVT] clock setting
	//======================================================
	nvt_u3ctrl_rst();
	nvt_u3ctrl_clkinit();

	//======================================================
	// [probe][NVT] controller setting
	//======================================================

	nvt_u3top_init();

	udelay(100);

	#if 0 //debug_dump
	hw_print();
	#endif
#endif

	#ifndef CONFIG_NVT_FPGA_EMULATION
	#ifndef CONFIG_TARGET_NS02401_A64
	nvt_phyinit();
	#endif
	#endif

#if (defined(CONFIG_TARGET_NS02201) || defined(CONFIG_TARGET_NS02201_A64) || defined(CONFIG_TARGET_NS02401_A64))
	if(u3_channel == 1)
		hccr = (struct xhci_hccr *)IOADDR_USB3_2_REG_BASE;
	else
		hccr = (struct xhci_hccr *)IOADDR_USB3_REG_BASE;

	//printf("hccr  = 0x%llx\n",(u64)hccr);
#else
	hccr = (struct xhci_hccr *)IOADDR_USB3_REG_BASE;
#endif
	len = HC_LENGTH(xhci_readl(&hccr->cr_capbase));
	hcor = (struct xhci_hcor *)((unsigned long)hccr + len);

	printk("XHCI init hccr 0x%lx and hcor 0x%lx hc_length 0x%x ver %s\n",
		(unsigned long)hccr, (unsigned long)hcor, len, DRV_VERSION);

	*ret_hccr = hccr;
	*ret_hcor = hcor;

	return 0;
}

static int xhci_usb_probe(struct udevice *dev)
{
	struct uboot_xhci_priv *ctx = dev_get_priv(dev);
	int ret = 0;
	#if (defined(CONFIG_TARGET_NS02201) || defined(CONFIG_TARGET_NS02201_A64) || defined(CONFIG_TARGET_NS02401_A64))
	unsigned long base_addr = dev_read_addr(dev);

	printk("%s base =  0x%lx \n", __func__, 	base_addr);
	if ((unsigned int)base_addr == (unsigned int)IOADDR_USB3_REG_BASE) {
		u3_channel = 0;
		u3_ioaddr = IOADDR_USB3PHY_REG_BASE;
		#if CONFIG_TARGET_NS02401_A64
		u2_ioaddr = IOADDR_USBPHY_REG_BASE;
		#endif
	}else {
		u3_channel = 1;
		u3_ioaddr = IOADDR_USB3_2PHY_REG_BASE;
		#if CONFIG_TARGET_NS02401_A64
		u2_ioaddr = IOADDR_USB2PHY_REG_BASE;
		#endif
	}
	//printk("%s CH =  0x%lx , 0x%lx\n", __func__, 	u3_channel, u3_ioaddr);

	#elif defined(CONFIG_TARGET_NS02302_A64)
	unsigned long base_addr = dev_read_addr(dev);

	printk("%s base =  0x%lx , u3_ioaddr is 0x%llx\n", __func__,         base_addr, u3_ioaddr);
	#endif


	#if (defined(CONFIG_TARGET_NA51102_A64) || defined(CONFIG_TARGET_NA51102) || defined(CONFIG_TARGET_NS02302_A64))
	nvtim_set_gpio_power(0); //one channel case
	#else
	nvtim_set_gpio_power(u3_channel); //two channel case
	#endif

	ret = dm_xhci_hcd_init(0 , &ctx->hccr, &ctx->hcor);
	if (ret) {
		puts("XHCI: failed to initialize controller\n");
		return -EINVAL;
	}

	debug("%s hccr 0x%lx and hcor 0x%lx hc_length %ld\n", __func__,
			(unsigned long)ctx->hccr, (unsigned long)ctx->hcor,
			(unsigned long)HC_LENGTH(xhci_readl(&ctx->hccr->cr_capbase)));

	return xhci_register(dev, ctx->hccr, ctx->hcor);
}

static int xhci_usb_remove(struct udevice *dev)
{
	int ret;

#if defined(CONFIG_TARGET_NA51102_A64)
	unsigned long tmpval2 = 0;

	/* clear reset flag*/
	tmpval2 = readl((volatile unsigned long *)(IOADDR_USB3PHY_REG_BASE+0x28));
	if (tmpval2 & 0x80) {
		tmpval2 &= ~(0x80);
		writel(tmpval2, (volatile unsigned long *)(IOADDR_USB3PHY_REG_BASE+0x28));
	} else {
		//do nothing.
	}

	/* Reset u2/u3 phy */
	tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
	tmpval2 &= ~(0x5<<5);
	writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));

	udelay(10);

	tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
	tmpval2 |= (0x5<<5);
	writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));

        /* Reset U3 controller */
        tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x94));
        tmpval2 &= ~(0x1<<26);
        writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x94));

        udelay(10);

        tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x94));
        tmpval2 |= (0x1<<26);
        writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x94));

#elif (defined(CONFIG_TARGET_NS02201) || defined(CONFIG_TARGET_NS02201_A64))
	unsigned long tmpval2 = 0;
	unsigned long base_addr = dev_read_addr(dev);
	u32 u3_ch = 0;

	if ((unsigned int)base_addr == (unsigned int)IOADDR_USB3_REG_BASE) {
		u3_ch = 0;
	}else {
		u3_ch = 1;
	}

	if(u3_ch == 1) {
		/* Reset u2/u3 phy */
		tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));
		tmpval2 &= ~(0x1<<15);
		writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));

		udelay(10);

		tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));
		tmpval2 |= (0x1<<15);
		writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));

		/* Reset U3_2 controller */
		tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));
		tmpval2 &= ~(0x1<<6);
		writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));

		udelay(10);

		tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));
		tmpval2 |= (0x1<<6);
		writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));
	} else {
		/* Reset u2/u3 phy */
		tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));
		tmpval2 &= ~(0x1<<14);
		writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));

		udelay(10);

		tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));
		tmpval2 |= (0x1<<14);
		writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0xA0));

		tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
		tmpval2 &= ~(0x1<<7);
		writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));

		udelay(10);

		tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));
		tmpval2 |= (0x1<<7);
		writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x9C));

		/* Reset U3_1 controller */
		tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x94));
		tmpval2 &= ~(0x1<<26);
		writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x94));

		udelay(10);

		tmpval2 = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x94));
		tmpval2 |= (0x1<<26);
		writel(tmpval2, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x94));
	}

#elif defined(CONFIG_TARGET_NS02302_A64)
	unsigned long tmpval2 = 0;

	/* Reset u2/u3 phy */
	tmpval2 = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	tmpval2 &= ~(0xD<<16);
	writel(tmpval2, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));

	udelay(10);

	tmpval2 = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	tmpval2 |= (0xD<<16);
	writel(tmpval2, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));

	/* Reset glue logic */
	tmpval2 = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	tmpval2 &= ~(0x1<<17);
	writel(tmpval2, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));

	udelay(10);

	tmpval2 = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	tmpval2 |= (0x1<<17);
	writel(tmpval2, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));


	/* Reset U3 controller */
	tmpval2 = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	tmpval2 &= ~(0x1<<15);
	writel(tmpval2, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));

	udelay(10);

	tmpval2 = readl((volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));
	tmpval2 |= (0x1<<15);
	writel(tmpval2, (volatile unsigned long *)(IOADDR_STBC_CG_REG_BASE+0x90));

#elif defined(CONFIG_TARGET_NS02401_A64)
	unsigned long base_addr = dev_read_addr(dev);

	if ((unsigned int)base_addr == (unsigned int)IOADDR_USB3_REG_BASE) {
		u3_channel = 0;
	}else {
		u3_channel = 1;
	}

	nvt_u2phy_reset();
	nvt_u3phy_reset();
	nvt_u3ctrl_rst();
	mdelay(200);
#else

#endif
	ret = xhci_deregister(dev);
	if (ret)
		return ret;

	return 0;
}

static const struct udevice_id xhci_usb_ids[] = {
	{ .compatible = "nvt,nvt_usb3xhci" },
	{ }
};

U_BOOT_DRIVER(usb_xhci) = {
	.name   = "xhci_nvtivot",
	.id     = UCLASS_USB,
	.of_match = xhci_usb_ids,
	.probe = xhci_usb_probe,
	.remove = xhci_usb_remove,
	.ops    = &xhci_usb_ops,
	.plat_auto = sizeof(struct usb_plat),
	.priv_auto = sizeof(struct uboot_xhci_priv),
	.flags  = DM_FLAG_ALLOC_PRIV_DMA,
};

#endif
