// SPDX-License-Identifier: GPL-2.0+
/*
 * Faraday USB 3.0 OTG Controller
 *
 * (C) Copyright 2010 Faraday Technology
 * Dante Su <dantesu@faraday-tech.com>
 */

#include <common.h>
#include <command.h>
#include <config.h>
#include <net.h>
#include <malloc.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <asm/arch/efuse_protected.h>
#include <asm/arch/IOAddress.h>
#include <linux/delay.h>

#include <usb/fotg330.h>

#define DRV_VERSION                     "1.00.05"

// result of setup packet
#define CX_IDLE		0
#define CX_FINISH	1
#define CX_STALL	       2

#define CFG_NUM_ENDPOINTS       4
#define CFG_EP0_MAX_PACKET_SIZE	64
#define CFG_EPX_MAX_PACKET_SIZE	512

struct nvt_udc_chip;

struct nvt_udc_ep {
	struct usb_ep ep;

	uint maxpacket;
	uint id;
	uint stopped;

	struct list_head                      queue;
	struct nvt_udc_chip                  *chip;
	const struct usb_endpoint_descriptor *desc;
};

struct nvt_udc_request {
	struct usb_request req;
	struct list_head   queue;
	struct nvt_udc_ep *ep;
};

struct nvt_udc_chip {
	struct usb_gadget         gadget;
	struct usb_gadget_driver *driver;
	struct nvt_udc_ext_regs  *ext_regs;
	uint8_t                   irq;
	uint16_t                  addr;
	int                       pullup;
	enum usb_device_state     state;
	struct nvt_udc_ep         ep[1 + CFG_NUM_ENDPOINTS];
};


static struct usb_endpoint_descriptor ep0_desc = {
	.bLength = sizeof(struct usb_endpoint_descriptor),
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_CONTROL,
};


static const char * const epname[] = {
	"ep0"
};

//#define USB3_CH2					DISABLE

#if defined(CONFIG_TARGET_NA51102_A64)

#define USB3PHY_SETREG(ofs,value)   writel((value), (volatile void __iomem *)(IOADDR_USB3PHY_REG_BASE+0x2000+((ofs)<<2)))
#define USB3PHY_GETREG(ofs)         readl((volatile void __iomem *)(IOADDR_USB3PHY_REG_BASE+0x2000+((ofs)<<2)))
#define USB3U2PHY_SETREG(ofs,value) writel((value), (volatile void __iomem *)(IOADDR_USB3PHY_REG_BASE+0x1000+((ofs)<<2)))
#define USB3U2PHY_GETREG(ofs)       readl((volatile void __iomem *)(IOADDR_USB3PHY_REG_BASE+0x1000+((ofs)<<2)))

#define  IOADDR_USB3_DEVMODE_BASE    (IOADDR_USB3_REG_BASE + 0x2000)

#elif defined(CONFIG_TARGET_NS02201_A64)
#if defined(CONFIG_USB3_2)
#define IOADDR_USB3CTL_REG_BASE		IOADDR_USB3_2PHY_REG_BASE
#define IOADDR_U3_REG_BASE			IOADDR_USB3_2_REG_BASE
#else
#define IOADDR_USB3CTL_REG_BASE		IOADDR_USB3PHY_REG_BASE
#define IOADDR_U3_REG_BASE			IOADDR_USB3_REG_BASE
#endif

#define USB3PHY_SETREG(ofs,value)   writel((value), (volatile void __iomem *)(IOADDR_USB3CTL_REG_BASE+0x2000+((ofs)<<2)))
#define USB3PHY_GETREG(ofs)         readl((volatile void __iomem *)(IOADDR_USB3CTL_REG_BASE+0x2000+((ofs)<<2)))
#define USB3U2PHY_SETREG(ofs,value) writel((value), (volatile void __iomem *)(IOADDR_USB3CTL_REG_BASE+0x1000+((ofs)<<2)))
#define USB3U2PHY_GETREG(ofs)       readl((volatile void __iomem *)(IOADDR_USB3CTL_REG_BASE+0x1000+((ofs)<<2)))

#define  IOADDR_USB3_DEVMODE_BASE    (IOADDR_U3_REG_BASE + 0x2000)
#endif

static unsigned fotg330_dbg = 0;

#if (defined(CONFIG_TARGET_NA51102) || defined(CONFIG_TARGET_NA51102_A64))
static void nvt_phyinit(void)
{
	unsigned long tmpval = 0, temp = 0;
	u8 tx_term_trim_val = 6;
	u8 rx_term_trim_val = 12;
	u8 u3_trim_rint_sel=8;
	u32 trim;
	int code;
	bool is_found;

	if(fotg330_dbg)
		printf("nvt_udc: na51102_phyinit - start\n");
	code = -33;//otp_key_manager(EFUSE_USB_SATA_TRIM_DATA);
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
			printf("is found [%d][USB trim] = 0x%08x\r\n", is_found, (int)code);
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
	if(fotg330_dbg)
		printf("nvt_udc: na51102_phyinit - end\n");
}
#elif defined(CONFIG_TARGET_NS02201_A64)
static void nvt_phyinit(void)
{
	u8 u3_trim_rint_sel=8;
#if defined(CONFIG_USB3_2)
	u8 u3_2_rint_sel=8;
#endif
	u32 temp;
	u8 tx_term_trim_val = 0x6;
	u8 rx_term_trim_val = 0xC;
	u8 u2_trim_swctrl = 4;
	//u8 u3_trim_swctrl;

	if(fotg330_dbg)
		pr_info("690 usb3_u2u3phyinit\r\n");

	// Bank 0
	USB3PHY_SETREG(0xFF, 0x00);

	temp = USB3PHY_GETREG(0x97);
	temp |= 0x3;
	USB3PHY_SETREG(0x97 , temp);

#if 1//USB3_CONFIG_TRIM
	{
		s32       trim;
		//s32       addr;
		//u16      value;
		int     code;
		BOOL is_found;

		#if defined(CONFIG_USB3_2)
			code = otp_key_manager(EFUSE_TRIM_DATA_USB_SATA_PRI);
			//addr = 0x05;
		#else
			code = otp_key_manager(EFUSE_TRIM_DATA_USB3_1_USB2_DUAL_PHY_PRI);
			//addr = 0x2A;
		#endif

		if (code == -33) {
			//!!!Please apply default value here!!!
			printf("Read usb trim error\n");
			printf("[%d] data = NULL\n", 5);
		} else {
			is_found = extract_trim_valid(code, (u32 *)&trim);
			if (is_found) {
				printf("  USB Trim data = 0x%04x\r\n", (int)trim);
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
				printf("is found [%d][USB trim] = 0x%08x\r\n", is_found, (int)code);
			}
		}
		#if defined(CONFIG_USB3_2)
		code = otp_key_manager(EFUSE_TRIM_USB_LDO_PRI);
		//addr = 0x39;

		if (code == -33) {
			printf("Read usb trim error\n");
			printf("[%d] data = NULL\n", 5);
		} else {
			is_found = extract_trim_valid(code, (u32 *)&trim);
			if (is_found) {
				printf("  USB3_2 single port Trim data = 0x%04x\r\n", (int)trim);
				u3_2_rint_sel = trim & 0x1F;
				if (u3_2_rint_sel > 0x14) {
					u3_2_rint_sel=8;
					printf("12K_resistor Trim data error 0x%04x > 20(0x14)\r\n", (int)u3_2_rint_sel);
				} else if (u3_2_rint_sel < 0x1) {
					u3_2_rint_sel=8;
					printf("12K_resistor Trim data error 0x%04x < 1(0x1)\r\n", (int)u3_2_rint_sel);
				} else {
					printf("  *12K_resistor Trim data range success 0x1 <= [0x%04x] <= 0x14\r\n", (int)u3_2_rint_sel);
				}
			} else {
				//!!!Please apply default value here!!!
				printf("is found [%d][USB trim] = 0x%08x\r\n", is_found, (int)code);
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

		#endif

		//(1) select PHY internal rsel value 0x2684 bit 7
		temp = USB3PHY_GETREG(0x1A1);
		temp &= ~(0x80);
		USB3PHY_SETREG(0x1A1 , temp);
		//(2) R12K trim setting  if 0x2680[3..0] == 0 setting
		temp = USB3PHY_GETREG(0x1A0);
		if ((temp & 0xF) == 0) {
			USB3PHY_SETREG(0x1A0, 0x40+u3_trim_rint_sel);
			USB3PHY_SETREG(0x1A3, 0x60);
		}
		//======================================================
		//(3) USB20 swing & sqsel trim bits removed in INT98530
		//======================================================
		//temp = USB3U2PHY_GETREG(0x06);
		//temp &= ~(0x7<<1);
		//temp |= (u3_trim_swctrl<<1);
		//USB3U2PHY_SETREG(0x06, temp);

		//temp = USB3U2PHY_GETREG(0x05);
		//temp &= ~(0x7<<2);
		//temp |= (u3_trim_sqsel<<2);
		//USB3U2PHY_SETREG(0x05, temp);
	}
#else
	temp = USB3PHY_GETREG(0x1A1);
	temp &= ~(0x80);
	USB3U2PHY_SETREG(0x1A1 , temp);

		temp = USB3PHY_GETREG(0x1A0);
		if ((temp & 0xF) == 0) {
			USB3PHY_SETREG(0x1A0, 0x40+USB3_Internal_12K_resistor);
			USB3PHY_SETREG(0x1A3, 0x60);
		}
#endif
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

	// Bank 0
	USB3PHY_SETREG(0xFF, 0x00);
	// bit[5:4] = V_VCHSEL[1:0] = 11
	USB3PHY_SETREG(0x56, 0X74);
	// TX_AMP_CTL=2, TX_DEC_EM_CTL=8
	USB3PHY_SETREG(0x14, 0X8A);
	// bit[5:4] = TX_AMP_CTL_LFPS[1:0]
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
#if 0
	if ((tx_swing >= 0) && (tx_swing <= 3)) {
		u3_trim_swctrl = tx_swing;
		pr_info("u3_trim_swctrl = 0x%x\n",u3_trim_swctrl);

		temp = USB3PHY_GETREG(0x14);
		temp &= ~(0x3);
		temp |= (u3_trim_swctrl);
		pr_info("temp = 0x%x\n",temp);
		USB3PHY_SETREG(0x14 , temp);
	}
#endif
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
	//disconnect level to 0x3
	USB3PHY_SETREG(0x056, 0x74);
	udelay(2);

	//USB3PHY_SETREG(0x102, 0xFF);
	USB3PHY_SETREG(0x102, 0xF0);
	udelay(10);
	USB3PHY_SETREG(0x102, 0x00);
	udelay(300);

	/* disable_power_saving */
	//Checking this one
	USB3U2PHY_SETREG(0xE, 0x04);

	udelay(50);

	//USB3PHY_SETREG(0x103, 0x01);
	//udelay(100);

	//R45 Calibration
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

	//======================================================
	// U2 best setting												[NEW]
	//======================================================
	//U3 vchsel = 11
	USB3PHY_SETREG(0x56, 0x74);
	#if defined(CONFIG_USB3_2)
		USB3U2PHY_SETREG(0x5, 0xd3);
	#endif
	//Port2 ctrl_ls = 11
	USB3U2PHY_SETREG(0x9, 0xb0);
	//Port2 dout inverter
	USB3U2PHY_SETREG(0x12, 0x31);
	// tx swing  & de-emphasis swing  (690 add)
	USB3U2PHY_SETREG(0x6, 0x1c);

	//if ((u2_tx_swing >= 0) && (u2_tx_swing <= 7)) {
	//	u2_trim_swctrl = u2_tx_swing;
	//}

	//Set u2 phy swing
	temp = USB3U2PHY_GETREG(0x6);
	temp &= ~(0x7 << 1);
	temp |= (u2_trim_swctrl << 0x1);
	USB3U2PHY_SETREG(0x6, temp);

	if(fotg330_dbg)
		printf("nvt_udc: 690 phyinit - end\n");
}
#endif


static void nvtim_init_usbhc(void)
{
//	unsigned long usbbase, usbphybase;
	unsigned long tmpval = 0;
#if defined(CONFIG_TARGET_NA51102_A64)
	unsigned long tmpval2 = 0;
#endif
	if(fotg330_dbg)
		printf("nvt_udc: nvtim_init_usbhc\n");
#if defined(CONFIG_TARGET_NA51102_A64)
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

	//mdelay(10);

       /* Close channel disable*/
	tmpval = readl((volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x14));
	tmpval &= 0xFFFFFDFF;
	writel(tmpval, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x14));

	tmpval = readl((volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x10));
	//tmpval &= ~0x2;
	tmpval &= ~(0x1 << 11);
	tmpval |= (0x3 << 0); //VBUSVALID & dev mode
	tmpval |= (0x8 << 0); //HS
	tmpval &= ~((0x7 << 16) | (0x7 << 24) | (0x1 << 4));
	tmpval |= ((0x4 << 16) | (0x4 << 24));
	writel(tmpval, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x10));

	udelay(100);
#elif defined(CONFIG_TARGET_NS02201_A64)

	/* Enable PLL14 */
	tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE));
	tmpval |= (0x1<<14);
	writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE));

	udelay(10);


	tmpval = readl((volatile unsigned long *)(IOADDR_CG_REG_BASE+0x70));
#if defined(CONFIG_USB3_2)
	tmpval |= (0x1<<7);
#else
	tmpval |= (0x1<<6);
#endif
	writel(tmpval, (volatile unsigned long *)(IOADDR_CG_REG_BASE+0x70));

	//mdelay(10);
       /* Close channel disable*/
	tmpval = readl((volatile unsigned long *)(IOADDR_USB3CTL_REG_BASE+0x14));
	tmpval &= 0xFFFFFDFF;
	writel(tmpval, (volatile unsigned long *)(IOADDR_USB3CTL_REG_BASE+0x14));

	tmpval = readl((volatile unsigned long *)(IOADDR_USB3CTL_REG_BASE+0x10));
	//tmpval &= ~0x2;
	tmpval &= ~(0x1 << 11);
	tmpval |= (0x3 << 0); //VBUSVALID & dev mode
	tmpval |= (0x8 << 0); //HS
	tmpval &= ~((0x7 << 16) | (0x7 << 24) | (0x1 << 4));
	tmpval |= ((0x4 << 16) | (0x4 << 24));
	writel(tmpval, (volatile unsigned long *)(IOADDR_USB3CTL_REG_BASE+0x10));

	udelay(100);

#endif
	nvt_phyinit();
}



static int nvt_udc_reset(struct nvt_udc_chip *chip)
{
	unsigned long tmpval = 0;

	if(fotg330_dbg)
		printf("nvt_udc: nvt_udc_reset\n");
	nvtim_init_usbhc();
	
	/* enable all grp1 interrupt */
	writel(0xcfffff9f, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+ FUSB300_OFFSET_IGER1));

	if(fotg330_dbg)
	{
		tmpval = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+ FUSB300_OFFSET_IGER1));
		printf("nvt_udc: nvt_udc_reset 0x%lx\n",tmpval);
	}
	return 0;
}

static void fusb300_set_cxstall(void)
{
	u32 reg;
	reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_CSR));
	reg |= FUSB300_CSR_STL;
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_CSR));	
}

static void fusb300_set_cxdone(void)
{
	u32 reg;
	reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_CSR));
	reg |= FUSB300_CSR_DONE;
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_CSR));	
}

static void fusb330_set_dev_addr(u16 addr)
{
	u32 reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_DAR));

	reg &= ~FUSB300_DAR_DRVADDR_MSK;
	reg |= FUSB300_DAR_DRVADDR(addr);

	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_DAR));	
}

static void set_address(struct usb_ctrlrequest *ctrl)
{
	if (ctrl->wValue >= 0x0100)
		fusb300_set_cxstall();
	else {
		fusb330_set_dev_addr(ctrl->wValue);
		fusb300_set_cxdone();
	}
}


static void fusb330_set_configuration(void)
{
	u32 reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_DAR));

	reg |= FUSB300_DAR_SETCONFG;

	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_DAR));	
}


static void fusb330_clr_seqnum(u8 ep)
{
	u32 reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET0(ep)));

	reg |= FUSB300_EPSET0_CLRSEQNUM;

	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET0(ep)));	
}



/*
 * USB Gadget Layer
 */
static int nvt_udc_ep_enable(
	struct usb_ep *_ep, const struct usb_endpoint_descriptor *desc)
{
	

	return 0;
}

static int nvt_udc_ep_disable(struct usb_ep *_ep)
{
	
	return 0;
}

static struct usb_request *nvt_udc_ep_alloc_request(
	struct usb_ep *_ep, gfp_t gfp_flags)
{
	struct nvt_udc_request *req = malloc(sizeof(*req));

	if(fotg330_dbg)
		printf("nvt_udc_ep_alloc_request\n");
	if (req) {
		memset(req, 0, sizeof(*req));
		INIT_LIST_HEAD(&req->queue);
	}
	return &req->req;
}

static void nvt_udc_ep_free_request(
	struct usb_ep *_ep, struct usb_request *_req)
{
	struct nvt_udc_request *req;

	if(fotg330_dbg)	
		printf("nvt_udc_ep_free_request\n");
	req = container_of(_req, struct nvt_udc_request, req);
	free(req);
}

static void fotg330_set_cxlen(u32 length)
{
	u32 reg;

	reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_CSR));
	reg &= ~FUSB300_CSR_LEN_MSK;
	reg |= FUSB300_CSR_LEN(length);
	writel(reg, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_CSR);
}


/* write data to cx fifo */
static void fusb300_wrcxf(struct nvt_udc_ep *ep,
		   struct nvt_udc_request *req)
{
	int i = 0;
	u8 *tmp;
	u32 data;
	u32 length = req->req.length - req->req.actual;

	tmp = req->req.buf + req->req.actual;

	//printf( " fusb300_wrcxffusb300_wrcxffusb300_wrcxffusb300_wrcxf \n");

	writel(0x0F0F0002, (volatile unsigned long *)(IOADDR_USB3CTL_REG_BASE+0x14));
#if 0
	if (length > SS_CTL_MAX_PACKET_SIZE) {
		fusb300_set_cxlen(fusb300, SS_CTL_MAX_PACKET_SIZE);
		for (i = (SS_CTL_MAX_PACKET_SIZE >> 2); i > 0; i--) {
			data = *tmp | *(tmp + 1) << 8 | *(tmp + 2) << 16 |
				*(tmp + 3) << 24;
			writel(data, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_CXPORT);
			tmp += 4;
		}
		req->req.actual += SS_CTL_MAX_PACKET_SIZE;
	} 
	else 
#endif		
	{ /* length is less than max packet size */
		//printk(KERN_INFO " LEN   0x%x\n", length);
		fotg330_set_cxlen(length);
		for (i = length >> 2; i > 0; i--) {
			data = *tmp | *(tmp + 1) << 8 | *(tmp + 2) << 16 |
				*(tmp + 3) << 24;
			//printf( "    0x%x\n", data);
			writel(data, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_CXPORT);
			tmp = tmp + 4;
		}
		switch (length % 4) {
		case 1:
			data = *tmp;
			//printf( "  1  0x%x\n", data);
			writel(data, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_CXPORT);
			break;
		case 2:
			data = *tmp | *(tmp + 1) << 8;
			//printf( "  2  0x%x\n", data);
			writel(data, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_CXPORT);
			break;
		case 3:
			data = *tmp | *(tmp + 1) << 8 | *(tmp + 2) << 16;
			//printf( " 3   0x%x\n", data);
			writel(data, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_CXPORT);
			break;
		default:
			break;
		}
		req->req.actual += length;
	}
	writel(0x0F0F0003, (volatile unsigned long *)(IOADDR_USB3CTL_REG_BASE+0x14));
}


static void done(struct nvt_udc_ep *ep, struct nvt_udc_request *req,
		 int status)
{
#if 0
	list_del_init(&req->queue);

	/* don't modify queue heads during completion callback */
	if (ep->fusb300->gadget.speed == USB_SPEED_UNKNOWN)
		req->req.status = -ESHUTDOWN;
	else
		req->req.status = status;

	if(req->req.complete != NULL) {
		spin_unlock(&ep->fusb300->lock);
		usb_gadget_giveback_request(&ep->ep, &req->req);
		spin_lock(&ep->fusb300->lock);
	}

	if (ep->epnum) {
		disable_fifo_int(ep);
		if (!list_empty(&ep->queue))
			enable_fifo_int(ep);
	} else
#endif	
	if(req->req.complete != NULL) {
		req->req.complete(&ep->ep, &req->req);
	}

		fusb300_set_cxdone();
}

/* read data from cx fifo */
static u32 fusb330_rdcxf(u8 *buffer, u32 length)
{
	int i = 0;
	u8 *tmp;
	u32 data;

	//printf("fusb330_rdcxf len =%d\n",length);
	do{
		data = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_CSR));	
		//data = (data >>8)&0xFFFF;
		if((data&FUSB300_CSR_EMP) == 0)
			break;
		udelay(50);
	}while(1);

	data = (data >>8)&0xFFFF;

	if(length > data) {
		length = data;
		//printf("fusb330_rdcxf len reset to =0x%x\n",data);
	}

	writel(0x0F0F0002, (volatile unsigned long *)(IOADDR_USB3CTL_REG_BASE+0x14));
	
	tmp = buffer;

	for (i = (length >> 2); i > 0; i--) {
		data = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_CXPORT));
			
		//printf("    0x%08x\n", data);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		*(tmp + 3) = (data >> 24) & 0xFF;
		tmp = tmp + 4;
	}

	switch (length % 4) {
	case 1:
		data = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_CXPORT));
			//printf( "    0x%x\n", data);
		*tmp = data & 0xFF;
		break;
	case 2:
		data = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_CXPORT));
			//printf( "    0x%x\n", data);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		break;
	case 3:
		data = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_CXPORT));
			//printf( "    0x%x\n", data);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		break;
	default:
		break;
	}

	writel(0x0F0F0003, (volatile unsigned long *)(IOADDR_USB3CTL_REG_BASE+0x14));
	return length;
}


static void fusb330_ep0out(struct nvt_udc_ep *ep , struct nvt_udc_request *req)
{
	u32 reg;
	u32 donesize;
	u32 remain_size;
	u8 *buf_addr;	
	
	//struct nvt_udc_ep *ep = chip->ep;
	//struct nvt_udc_request *req;
	//int len;

	//if (ep->stopped || (ep->desc->bEndpointAddress & USB_DIR_IN)) {
	//	printf("fotg330: ep%d recv, invalid!\n", ep->id);
	//	return;
	//}


	//if (list_empty(&ep->queue)) {
	//	printf("fotg330: ep%d recv, drop!\n", ep->id);
	//	return;
	//}
	
	//printf("fotg330: req->req.length 0x%x!\n", req->req.length);

	//if (req->req.length)
	//	fusb330_rdcxf(req->req.buf,req->req.length);

	remain_size = req->req.length;
	donesize = 0;
	buf_addr = req->req.buf;
	
	do {
		donesize = fusb330_rdcxf(buf_addr,req->req.length);
		req->req.actual += donesize;
		remain_size -= donesize;
		//printf("fotg330: remain_size =  0x%x!\n", remain_size);
		buf_addr    += (uintptr_t)donesize;
	} while (remain_size);

	//printk(KERN_INFO "fusb330_rdcxf len Before done =%d\n",req->req.length);
	done(ep, req, 0);
	//fusb300_set_cxdone();
	//printk(KERN_INFO "fusb330_rdcxf len After done =%d\n",req->req.length);
	//reg = ioread32(fusb300->reg + FUSB300_OFFSET_IGER1);
	reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+ FUSB300_OFFSET_IGER1));
	reg &= ~FUSB300_IGER1_CX_OUT_INT;
	//printk(KERN_INFO "reg =%d\n",req);
	//iowrite32(reg, fusb300->reg + FUSB300_OFFSET_IGER1);
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+ FUSB300_OFFSET_IGER1));
	
	//printk(KERN_INFO "fusb330_ep0out done\n");
}


static void ep0_queue(struct nvt_udc_ep *ep, struct nvt_udc_request *req)
{
	unsigned long tmpval = 0;

	//printf("%s : req->req.length = 0x%x\n",__func__, req->req.length);
	//if (ep->fusb300->ep0_dir) { /* if IN */
	if (ep->desc->bEndpointAddress & USB_DIR_IN) { /* if IN */
		
		if (req->req.length) {
			fusb300_wrcxf(ep, req);
			//printf("%s : req->req.length1 = 0x%x\n",__func__, req->req.length);
		} 
		//else
			//printf("%s : req->req.length2 = 0x%x\n",__func__, req->req.length);

		if ((req->req.length == req->req.actual) ||
		    (req->req.actual < ep->ep.maxpacket)){
			//done(ep, req, 0);
			if(fotg330_dbg)
				printf("[NOTENOTENOTE1]: need check new DONE function here ?\n");

			if(req->req.complete != NULL) {
				req->req.complete(&ep->ep, &req->req);
			}
		}
	} else { /* OUT */
		if (!req->req.length){
			//done(ep, req, 0);
			if(fotg330_dbg)
				printf("[NOTENOTENOTE2]: need chcek new DONE function here ?\n");
		}
		else{
			if(fotg330_dbg)
				printf("%s : req->req.length3 = 0x%x\n",__func__, req->req.length);
			//fusb300_enable_bit(ep->fusb300, FUSB300_OFFSET_IGER1,
			//	FUSB300_IGER1_CX_OUT_INT);

			//fusb330_ep0out(struct nvt_udc_chip *chip);
			fusb330_ep0out(ep, req);
			tmpval = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_IGER1));
			tmpval |= FUSB300_IGER1_CX_OUT_INT;
			writel(tmpval, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_IGER1));			
		}	
	}
}

static int nvt_udc_ep_queue(
	struct usb_ep *_ep, struct usb_request *_req, gfp_t gfp_flags)
{
	struct nvt_udc_ep *ep = container_of(_ep, struct nvt_udc_ep, ep);
	//struct nvt_udc_chip *chip = ep->chip;
	//struct nvt_udc_regs *regs = chip->regs;
	struct nvt_udc_request *req;

	if(fotg330_dbg)
		printf("nvt_udc_ep_queue\n");

	req = container_of(_req, struct nvt_udc_request, req);
	if (!_req || !_req->complete || !_req->buf
		|| !list_empty(&req->queue)) {
		printf("fotg330: invalid request to ep%d\n", ep->id);
		return -EINVAL;
	}
	//printf("nvt_udc_ep_queue  len =0x%x\n",req->req.length );

	req->req.actual = 0;
	req->req.status = -EINPROGRESS;
	
	if (req->req.length == 0) {
		req->req.status = 0;
		if (req->req.complete){
			req->req.complete(&ep->ep, &req->req);
		}
		return 0;
	}

	if (ep->ep.name == epname[0]) /* ep0 */
		ep0_queue(ep, req);
	else
		printf("nvt_udc_ep_queue: invalid request to ep%d\n", ep->id);
	//else if (request && !ep->stall)
	//	enable_fifo_int(ep);

	return 0;
}

static int nvt_udc_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	if(fotg330_dbg)
		printf("nvt_udc_ep_dequeue\n");	

	return 0;
}

static int nvt_udc_ep_halt(struct usb_ep *_ep, int halt)
{
	struct nvt_udc_ep *ep = container_of(_ep, struct nvt_udc_ep, ep);
	//struct nvt_udc_chip *chip = ep->chip;
	int ret = -1;


	printf("nvt_udc: ep%d halt=%d\n", ep->id, halt);

	ret = 0;


	return ret;
}

/*
 * activate/deactivate link with host.
 */
static void pullup(struct nvt_udc_chip *chip, int is_on)
{
	//struct nvt_udc_ext_regs *regs = chip->ext_regs;
	unsigned long tmpval = 0;

	if (is_on) {
		if(fotg330_dbg)
			printf("pullup ON\n");

		tmpval = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EFCS));
		tmpval &= 0xFFFFF000;
		tmpval |= 0x200;//plug
		writel(tmpval, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EFCS));
	} else {
		if(fotg330_dbg)
			printf("pullup OFF\n");

		tmpval = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EFCS));
		tmpval &= 0xFFFFF000;
		tmpval |= 0x3FF; //un-plug
		writel(tmpval, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EFCS));
		
	}
	
}


static int nvt_udc_pullup(struct usb_gadget *_gadget, int is_on)
{
	struct nvt_udc_chip *chip;

	if(fotg330_dbg)
		printf("330 nvt_udc_pullup\n");

	chip = container_of(_gadget, struct nvt_udc_chip, gadget);
	pullup(chip, is_on);

	return 0;
}

static int nvt_udc_get_frame(struct usb_gadget *_gadget)
{
	if(fotg330_dbg)
		printf("330 nvt_udc_get_frame\n");
	return 0;
}


static struct usb_gadget_ops nvt_udc_gadget_ops = {
	.get_frame = nvt_udc_get_frame,
	.pullup = nvt_udc_pullup,
};

static struct usb_ep_ops nvt_udc_ep_ops = {
	.enable         = nvt_udc_ep_enable,
	.disable        = nvt_udc_ep_disable,
	.queue          = nvt_udc_ep_queue,
	.dequeue        = nvt_udc_ep_dequeue,
	.set_halt       = nvt_udc_ep_halt,
	.alloc_request  = nvt_udc_ep_alloc_request,
	.free_request   = nvt_udc_ep_free_request,
};


static struct nvt_udc_chip ext_controller = {
	.ext_regs = (void __iomem *)IOADDR_USB3_REG_BASE,
	.gadget = {
		.name = "nvt_udc",
		.ops = &nvt_udc_gadget_ops,
		.ep0 = &ext_controller.ep[0].ep,
		.speed = USB_SPEED_UNKNOWN,
		.is_dualspeed = 1,
		.is_otg = 0,
		.is_a_peripheral = 0,
		.b_hnp_enable = 0,
		.a_hnp_support = 0,
		.a_alt_hnp_support = 0,
	},
	.ep[0] = {
		.id = 0,
		.ep = {
			.name  = "ep0",
			.ops   = &nvt_udc_ep_ops,
		},
		.desc      = &ep0_desc,
		.chip      = &ext_controller,
		.maxpacket = CFG_EP0_MAX_PACKET_SIZE,
	},
	.ep[1] = {
		.id = 1,
		.ep = {
			.name  = "ep1",
			.ops   = &nvt_udc_ep_ops,
		},
		.chip      = &ext_controller,
		.maxpacket = CFG_EPX_MAX_PACKET_SIZE,
	},
	.ep[2] = {
		.id = 2,
		.ep = {
			.name  = "ep2",
			.ops   = &nvt_udc_ep_ops,
		},
		.chip      = &ext_controller,
		.maxpacket = CFG_EPX_MAX_PACKET_SIZE,
	},
	.ep[3] = {
		.id = 3,
		.ep = {
			.name  = "ep3",
			.ops   = &nvt_udc_ep_ops,
		},
		.chip      = &ext_controller,
		.maxpacket = CFG_EPX_MAX_PACKET_SIZE,
	},
	.ep[4] = {
		.id = 4,
		.ep = {
			.name  = "ep4",
			.ops   = &nvt_udc_ep_ops,
		},
		.chip      = &ext_controller,
		.maxpacket = CFG_EPX_MAX_PACKET_SIZE,
	},
};


static void nvt_udc_setup(struct nvt_udc_chip *chip)
{
	//int id;
	u8 i = 0;
	int ret = CX_IDLE;
	uint32_t tmp[2];
	struct usb_ctrlrequest *req = (struct usb_ctrlrequest *)tmp;
	//u32 reg;

	fusb330_rdcxf((u8 *)&tmp[0], 8);
	//printf("SETUP (0x%x)\n", tmp[0]);
	//printf("SETUP (0x%x)\n", tmp[1]);

	if (req->bRequestType & USB_DIR_IN){
		ep0_desc.bEndpointAddress = USB_DIR_IN;
		//printf("SETUP IN\n");
	}
	else{
		ep0_desc.bEndpointAddress = USB_DIR_OUT;
		//printf("SETUP OUT\n");		
	}

	ret = CX_IDLE;
	//printf("nvt_udc: req->bRequest (%d)\n", req->bRequest);
	if ((req->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) {
		//printf("(STD)\n");
		switch (req->bRequest) {
			case USB_REQ_SET_CONFIGURATION:
				//printf("nvt_udc: set_cfg(%d)\n", req->wValue & 0x00FF);
				if (!(req->wValue & 0x00FF)) {
					chip->state = USB_STATE_ADDRESS;
					//writel(chip->addr, &regs->dev_addr);
					fusb300_set_cxstall();
				} else {
					chip->state = USB_STATE_CONFIGURED;
					//writel(chip->addr | FUSB300_DAR_SETCONFG, &regs->dev_addr);
					fusb330_set_configuration();
					/* clear sequence number */
					for (i = 1; i <= FUSB330_MAX_NUM_EP; i++)
						fusb330_clr_seqnum(i);
				}
				ret = CX_IDLE;
				break;

			case USB_REQ_SET_ADDRESS:
				//printf("nvt_udc: set_addr(0x%04X)\n", req->wValue);
				chip->state = USB_STATE_ADDRESS;
				chip->addr  = req->wValue & FUSB300_DAR_DRVADDR_MSK;
				ret = CX_FINISH;
				//writel(chip->addr, &regs->dev_addr);
				set_address(req);
				break;

			case USB_REQ_CLEAR_FEATURE:
				printf("nvt_udc: clr_feature(%d, %d)\n",
						req->bRequestType & 0x03, req->wValue);
				break;

			case USB_REQ_SET_FEATURE:
				printf("nvt_udc: set_feature(%d, %d)\n",
						req->wValue, req->wIndex & 0xf);
				break;

			case USB_REQ_GET_STATUS:
				printf("nvt_udc: get_status\n");
				ret = CX_STALL;
				break;

			case USB_REQ_SET_DESCRIPTOR:
				printf("nvt_udc: set_descriptor\n");
				ret = CX_STALL;
				break;

			case USB_REQ_SYNCH_FRAME:
				printf("nvt_udc: sync frame\n");
				ret = CX_STALL;
				break;
		}
	} /* if ((req->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD) */	

	if (ret == CX_IDLE && chip->driver->setup) {
		if (chip->driver->setup(&chip->gadget, req) < 0)
			ret = CX_STALL;
		else
			ret = CX_FINISH;
	}

	switch (ret) {
	case CX_FINISH:
		fusb300_set_cxdone();
		break;

	case CX_STALL:
		fusb300_set_cxstall();
		printf("fotg330: cx_stall!\n");
		break;

	case CX_IDLE:
		printf("fotg330: cx_idle?\n");
	default:
		break;
	}
	

}


int usb_gadget_handle_interrupts(int index)
{
	struct nvt_udc_chip *chip = &ext_controller;
	//struct nvt_udc_ext_regs *regs = chip->ext_regs;
	//unsigned long id, st, isr, gisr;	
	u32 int_grp1 = readl(IOADDR_USB3_DEVMODE_BASE+ FUSB300_OFFSET_IGR1);
	u32 int_grp1_en = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGER1);
	u32 int_grp0 = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGR0);
	u32 int_grp0_en = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGER0);

	//printf("fusb330_irq [0x%x] [0x%x]\r\n",int_grp0,int_grp1);
	//printf("fusb330_irq_en [0x%x] [0x%x]\r\n",int_grp0_en,int_grp1_en);	

	int_grp1 &= int_grp1_en;
	int_grp0 &= int_grp0_en;

	if(!(int_grp0 || int_grp1))
		return 0;
	
	writel(int_grp0, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGR0);
	writel(int_grp1, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGR1);


	if (int_grp1 & FUSB300_IGR1_USBRST_INT) {	
		if(fotg330_dbg)
			printf(" FUSB300_IGR1_USBRST_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_SUSP_INT) {
		if(fotg330_dbg)
			printf(" FUSB300_IGR1_SUSP_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_CX_SETUP_INT) {
		if(fotg330_dbg)
			printf(" FUSB300_IGR1_CX_SETUP_INT\n");
		nvt_udc_setup(chip);
	}

	if (int_grp1 & FUSB300_IGR1_CX_OUT_INT) {
		//if(fotg330_dbg)
			printf("fusb330_cxout\n");
		//fusb330_ep0out(chip);
	}	
	
	return 0;
}


int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	int i, ret = 0;

	struct nvt_udc_chip *chip = &ext_controller;

	//printf("fotg330 usb_gadget_register_driver\n");

	if (!driver    || !driver->bind || !driver->setup) {
		puts("nvt_udc: bad parameter.\n");
		return -EINVAL;
	}

	INIT_LIST_HEAD(&chip->gadget.ep_list);
	for (i = 0; i < CFG_NUM_ENDPOINTS + 1; ++i) {
		struct nvt_udc_ep *ep = chip->ep + i;

		ep->ep.maxpacket = ep->maxpacket;
		INIT_LIST_HEAD(&ep->queue);

		if (ep->id == 0) {
			ep->stopped = 0;
		} else {
			ep->stopped = 1;
			list_add_tail(&ep->ep.ep_list, &chip->gadget.ep_list);
		}
	}

	if (nvt_udc_reset(chip)) {
		puts("nvt_udc: reset failed.\n");
		return -EINVAL;
	}

	ret = driver->bind(&chip->gadget);
	if (ret) {
		debug("nvt_udc: driver->bind() returned %d\n", ret);
		return ret;
	}
	chip->driver = driver;

	printf("fotg330 usb_gadget_register_driver ver %s\r\n",DRV_VERSION);

	return ret;
}


int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct nvt_udc_chip *chip = &ext_controller;


	driver->disconnect(&chip->gadget);
	driver->unbind(&chip->gadget);
	chip->driver = NULL;

	pullup(chip, 0);
	//printf("udc remove~~\n");

	return 0;
}


