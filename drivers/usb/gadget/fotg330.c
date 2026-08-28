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

#include <usb/fotg330.h>


#define DRV_VERSION                     "1.00.04"

// result of setup packet
#define CX_IDLE		0
#define CX_FINISH	1
#define CX_STALL	       2

#define CFG_NUM_ENDPOINTS       4
#define CFG_EP0_MAX_PACKET_SIZE	64
#define CFG_EPX_MAX_PACKET_SIZE	512

#define UVC_FIFOENTRY_NUM	10
#define UVC_ISO_MAX_PKTNUM	32

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

#if defined(CONFIG_TARGET_NA51102_A64)

#define USB3PHY_SETREG(ofs,value)   writel((value), (volatile void __iomem *)(IOADDR_USB3PHY_REG_BASE+0x2000+((ofs)<<2)))
#define USB3PHY_GETREG(ofs)         readl((volatile void __iomem *)(IOADDR_USB3PHY_REG_BASE+0x2000+((ofs)<<2)))
#define USB3U2PHY_SETREG(ofs,value) writel((value), (volatile void __iomem *)(IOADDR_USB3PHY_REG_BASE+0x1000+((ofs)<<2)))
#define USB3U2PHY_GETREG(ofs)       readl((volatile void __iomem *)(IOADDR_USB3PHY_REG_BASE+0x1000+((ofs)<<2)))

#define  IOADDR_USB3_DEVMODE_BASE    (IOADDR_USB3_REG_BASE + 0x2000)
#endif


static unsigned fotg330_dbg = 0;
static unsigned fifo_entry_num = FUSB300_FIFO_ENTRY_NUM;
static unsigned fifo_entry_num_acc = 0;
static unsigned addrofs = 0;
static unsigned pio_on = 0;
static unsigned hbw_ep = 0;
static unsigned uvc_enable = 0;
spinlock_t	u3lock;
static unsigned showSpeed = 0;


#if (defined(CONFIG_TARGET_NA51102) || defined(CONFIG_TARGET_NA51102_A64))
static void na51102_phyinit(void)
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
	// bit[5:4] = TX_AMP_CTL_LFPS[1:0] (from 11 -> 10)
	USB3PHY_SETREG(0x12, 0XE8);
	//USB3PHY_SETREG(0x12, 0XF8);
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
#endif


static void nvtim_init_usbhc(void)
{
//	unsigned long usbbase, usbphybase;
	unsigned long tmpval = 0;
	unsigned long tmpval2 = 0;

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

	mdelay(10);

       /* Close channel disable*/
	tmpval = readl((volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x14));
	tmpval &= 0xFFFFFDFF;
	writel(tmpval, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x14));

	tmpval = readl((volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x10));
	//tmpval &= ~0x2;
	tmpval &= ~(0x1 << 11);
	tmpval |= (0x3 << 0);
	tmpval &= ~((0x7 << 16) | (0x7 << 24) | (0x1 << 4));
	tmpval |= ((0x4 << 16) | (0x4 << 24));
	writel(tmpval, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x10));

	udelay(100);

	na51102_phyinit();
#endif
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


static void fusb300_enable_bit(u32 offset,u32 value)
{
	u32 reg;

	reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+offset));
	reg |= value;
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+offset));
}

static void fusb300_disable_bit(u32 offset,u32 value)
{
	u32 reg;

	reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+offset));
	reg &= ~value;
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+offset));
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


static void fusb300_set_fifo_entry(u32 ep)
{
	u32 val = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(ep)));

	val &= ~FUSB300_EPSET1_FIFOENTRY_MSK;
	val |= FUSB300_EPSET1_FIFOENTRY(fifo_entry_num);
	writel(val, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(ep)));
}

static void fusb300_set_start_entry(u8 ep)
{
	u32 reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(ep)));
	u32 start_entry = fifo_entry_num_acc * fifo_entry_num;

	reg &= ~FUSB300_EPSET1_START_ENTRY_MSK	;
	reg |= FUSB300_EPSET1_START_ENTRY(start_entry);
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(ep)));

	if (fifo_entry_num_acc == FUSB300_MAX_FIFO_ENTRY) {
		fifo_entry_num_acc = 0;
		addrofs = 0;
		printf("fifo entry is over the maximum number!\n");
	} else
		fifo_entry_num_acc++;
}

/* set fusb300_set_start_entry first before fusb300_set_epaddrofs */
static void fusb300_set_epaddrofs( struct fusb300_ep_info info)
{
	u32 reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET2(info.epnum)));

	reg &= ~FUSB300_EPSET2_ADDROFS_MSK;
	reg |= FUSB300_EPSET2_ADDROFS(addrofs);
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET2(info.epnum)));

	addrofs += (info.maxpacket + (fifo_entry_num-1)) / fifo_entry_num * fifo_entry_num;
}

static void ep_fifo_setting(struct fusb300_ep_info info)
{
	fusb300_set_fifo_entry(info.epnum);
	fusb300_set_start_entry(info.epnum);
	fusb300_set_epaddrofs(info);
}

static void fusb300_set_eptype(struct fusb300_ep_info info)
{
	u32 reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(info.epnum)));

	reg &= ~FUSB300_EPSET1_TYPE_MSK;
	reg |= FUSB300_EPSET1_TYPE(info.type);
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(info.epnum)));
}

static void fusb300_set_epdir(struct fusb300_ep_info info)
{
	u32 reg;

	if (!info.dir_in)
		return;

	reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(info.epnum)));
	reg &= ~FUSB300_EPSET1_DIR_MSK;
	reg |= FUSB300_EPSET1_DIRIN;
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(info.epnum)));
}


static void fusb300_set_ep_active(u8 ep)
{
	u32 reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(ep)));

	reg |= FUSB300_EPSET1_ACTEN;
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(ep)));
}

static void fusb300_set_epmps(struct fusb300_ep_info info)
{
	u32 reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET2(info.epnum)));

	reg &= ~FUSB300_EPSET2_MPS_MSK;
	reg |= FUSB300_EPSET2_MPS(info.maxpacket);
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET2(info.epnum)));
}

static void fusb300_set_interval(struct fusb300_ep_info info)
{
	u32 reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(info.epnum)));

	reg &= ~FUSB300_EPSET1_INTERVAL(0x7);
	reg |= FUSB300_EPSET1_INTERVAL(info.interval);
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(info.epnum)));
}

#if 0
static void fusb300_set_isoinpkt(u8 epnum, u32 pktnum)
{
	u32 reg;

	if(pktnum>UVC_ISO_MAX_PKTNUM)
		pktnum = UVC_ISO_MAX_PKTNUM;

	reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(epnum)));

	reg &= ~FUSB300_EPSET1_ISOINPKT(0x3f);
	reg |= FUSB300_EPSET1_ISOINPKT(pktnum);
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(epnum)));
}
#endif

static void fusb300_set_bwnum(struct fusb300_ep_info info)
{
	u32 reg = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(info.epnum)));

	reg &= ~FUSB300_EPSET1_BWNUM(0x3);
	reg |= FUSB300_EPSET1_BWNUM(info.bw_num);
	writel(reg, (volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE+FUSB300_OFFSET_EPSET1(info.epnum)));
}


static void set_ep_reg(struct fusb300_ep_info info)
{
	fusb300_set_eptype(info);
	fusb300_set_epdir(info);
	fusb300_set_epmps(info);

	if (info.interval)
		fusb300_set_interval(info);

	if (info.bw_num)
		fusb300_set_bwnum(info);

	fusb300_set_ep_active(info.epnum);
}


//static void fusb300_ep_setting(struct nvt_udc_ep *ep,
//			       struct fusb300_ep_info info)
//{
//	ep->epnum = info.epnum;
//	ep->type = info.type;
//}



static int config_ep(struct nvt_udc_ep *ep,
		     const struct usb_endpoint_descriptor *desc)
{
	struct fusb300_ep_info info;
	u32 reg;
	u32 i;

	//printk(KERN_INFO "config_ep ======================= start\n");
	ep->ep.desc = desc;

	info.interval = 0;
	info.addrofs = 0;
	info.bw_num = 0;

	info.type = desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
	info.dir_in = (desc->bEndpointAddress & USB_ENDPOINT_DIR_MASK) ? 1 : 0;
	info.maxpacket = usb_endpoint_maxp(desc);
	info.epnum = desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;

	if ((info.type == USB_ENDPOINT_XFER_INT) ||
	   (info.type == USB_ENDPOINT_XFER_ISOC)) {
		info.interval = desc->bInterval;
		if (info.type == USB_ENDPOINT_XFER_ISOC)
			info.bw_num = usb_endpoint_maxp_mult(desc);
	}

	ep_fifo_setting(info);

	set_ep_reg(info);

	//fusb300_ep_setting(ep, info);

	if(fotg330_dbg){
		if(info.type == USB_ENDPOINT_XFER_INT)
			printf("--[INT]--\n");
		else if(info.type == USB_ENDPOINT_XFER_ISOC)
			printf("--[ISO]--\n");
		else if(info.type == USB_ENDPOINT_XFER_CONTROL)
			printf("--[CTRL]--\n");
		else if(info.type == USB_ENDPOINT_XFER_BULK)
			printf("--[BULKn");

		printk(KERN_INFO "info.epnum %d\n",info.epnum);
		printf("maxburst = [%d]--\n", ep->ep.maxburst);
		//printf("mult = [%d]--\n", ep->ep.mult);
	}

	//UVC SS need check this
	//if((info.type == USB_ENDPOINT_XFER_ISOC)&&(ep->fusb300->gadget.speed == USB_SPEED_SUPER)){
	//	fusb300_set_isoinpkt(fusb300, info.epnum, ep->ep.mult*ep->ep.maxburst);
	//}

	//fusb300->ep[info.epnum] = ep;
	//printk(KERN_INFO "config_ep =======================  end\n");
#if 0
	if(fotg330_dbg){
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
	for(i=1;i<=2;i++)
	{
		printf("========[EP%d]===========  \n",i);
		reg = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_EPSET0(i));
		printf("EP[%d] EPSET0 = 0x%x\n",i,reg);
		reg = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_EPSET1(i));
		printf("EP[%d] EPSET1 = 0x%x\n",i,reg);
		reg = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_EPSET2(i));
		printf("EP[%d] EPSET2 = 0x%x\n",i,reg);
		reg = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_EPFFR(i));
		printf("EP[%d] EPFFR = 0x%x\n",i,reg);
	}
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> \n");
	}
#endif
	return 0;
}


static void fusb300_clear_int(u32 offset,  u32 value)
{
	writel(value, IOADDR_USB3_DEVMODE_BASE + offset);
}

/*
 * USB Gadget Layer
 */
static int nvt_udc_ep_enable(
	struct usb_ep *_ep, const struct usb_endpoint_descriptor *desc)
{
	struct nvt_udc_ep *ep = container_of(_ep, struct nvt_udc_ep, ep);
	//struct nvt_udc_chip *chip = ep->chip;
	//int id = ep->id;
	//int in = (desc->bEndpointAddress & USB_DIR_IN) ? 1 : 0;
	if(fotg330_dbg)
		printf("nvt_udc_ep_enablenvt_udc_ep_enablenvt_udc_ep_enable ~~~~~\n");
	
	if (!_ep || !desc
		|| desc->bDescriptorType != USB_DT_ENDPOINT
		|| le16_to_cpu(desc->wMaxPacketSize) == 0) {
		printf("nvt_udc: bad ep or descriptor\n");
		return -EINVAL;
	}

	ep->desc = desc;
	ep->stopped = 0;

	return config_ep(ep, desc);
}

static int nvt_udc_ep_disable(struct usb_ep *_ep)
{
	printf("nvt_udc_ep_disable need imp. ~~~~~\n");	
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


static int enable_fifo_int(u32 ep)
{
	//printf("fotg330: enable_fifo_int ep%d\n", ep);
	if (ep) {
		fusb300_enable_bit( FUSB300_OFFSET_IGER0,
			FUSB300_IGER0_EEPn_FIFO_INT(ep));
	} else {
		printf("can't enable_fifo_int ep0\n");
		return -EINVAL;
	}
	return 0;
}


static int disable_fifo_int(u32 ep)
{
	//printf("fotg330: disable_fifo_int ep%d\n", ep);

	if (ep) {
		fusb300_disable_bit(FUSB300_OFFSET_IGER0,
			FUSB300_IGER0_EEPn_FIFO_INT(ep));
	} else {
		printf("can't disable_fifo_int ep0\n");
		return -EINVAL;
	}

	return 0;
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
	//printf("fusb300_wrcxf   req->req.actual = 0x%x\n",req->req.actual);

	writel(0x0F0F0002, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x14));

	if(fotg330_dbg)
	       printf("length = 0x%x\n",length);
#if 0
	if (length > SS_CTL_MAX_PACKET_SIZE) {
		fotg330_set_cxlen(SS_CTL_MAX_PACKET_SIZE);
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
	writel(0x0F0F0003, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x14));
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
	list_del_init(&req->queue);

	if(req->req.complete != NULL) {
		req->req.complete(&ep->ep, &req->req);
	}

	if (ep->id) {
		disable_fifo_int(ep->id);
		if (!list_empty(&ep->queue))
			enable_fifo_int(ep->id);
	} else
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

	writel(0x0F0F0002, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x14));
	
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
	
	writel(0x0F0F0003, (volatile unsigned long *)(IOADDR_USB3CTRL_REG_BASE+0x14));	
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

	req->req.status = 0;
	//printk(KERN_INFO "fusb330_rdcxf len Before done =%d\n",req->req.length);
	//done(ep, req, 0);
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
		//printf("IN  %s : req->req.length = 0x%x\n",__func__, req->req.length);
		if (req->req.length) {
			fusb300_wrcxf(ep, req);
			//printf("%s : req->req.length1 = 0x%x\n",__func__, req->req.length);
		} 
		//else
			//printf("%s : req->req.length2 = 0x%x\n",__func__, req->req.length);

		if ((req->req.length == req->req.actual) ||
		    (req->req.actual < ep->ep.maxpacket)){
			done(ep, req, 0);
			if(fotg330_dbg)
				printf("[NOTENOTENOTE1]: need check new DONE function here ?\n");
		}
	} else { /* OUT */
		//printf("OUT  %s : req->req.length = 0x%x\n",__func__, req->req.length);
		if (!req->req.length){
			done(ep, req, 0);
			if(fotg330_dbg)
				printf("[NOTENOTENOTE2]: need chcek new DONE function here ?\n");

			if(req->req.complete != NULL) {
				req->req.complete(&ep->ep, &req->req);
			}
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

#if 1
	//below for debug
	if(!showSpeed){
		tmpval = readl((volatile unsigned long *)(IOADDR_USB3_DEVMODE_BASE));
		if((tmpval&0x3) == 0x01)
			printf("nvt_udc: SS\n");
		else if((tmpval&0x3) == 0x02)
			printf("nvt_udc: HS\n");
		else if((tmpval&0x3) == 0x03)
			printf("nvt_udc: FS\n");
		else
			printf("nvt_udc: NO\n");

		showSpeed = 1;
	}
#endif

	
}

static int nvt_udc_ep_queue(
	struct usb_ep *_ep, struct usb_request *_req, gfp_t gfp_flags)
{
	struct nvt_udc_ep *ep = container_of(_ep, struct nvt_udc_ep, ep);
	struct nvt_udc_chip *chip = ep->chip;
	//struct nvt_udc_regs *regs = chip->regs;
	struct nvt_udc_request *req;

	unsigned long flags;

	if(fotg330_dbg)
		printf("==========================[nvt_udc_ep_queue]\n");

	req = container_of(_req, struct nvt_udc_request, req);
	//printf("[req contain] 0x%x\n",(u32)req);

	if (!_req || !_req->complete || !_req->buf
		|| !list_empty(&req->queue)) {
		printf("fotg330: invalid request to ep%d\n", ep->id);
		return -EINVAL;
	}

	spin_lock_irqsave(&u3lock, flags);

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

	//printf("==========================[nvt_udc_ep_queue] id = %d  name = %s \n", ep->id, ep->ep.name);
	//if (ep->ep.name == epname[0]) /* ep0 */
	if (ep->ep.desc == NULL)
		ep0_queue(ep, req);
	else{
		enable_fifo_int(ep->id);
		//if (ep->desc->bEndpointAddress & USB_DIR_IN){		
		//} else {
			//printf("[Add req contain] 0x%x\n",(u32)req);
			//printf("[Add ep contain] 0x%x\n",(u32)ep);
			list_add_tail(&req->queue, &ep->queue);
		//}
	}

	if (ep->id == 0 || (ep->desc->bEndpointAddress & USB_DIR_IN)) {
		if (req->req.complete)
			req->req.complete(&ep->ep, &req->req);
	}
	spin_unlock_irqrestore(&u3lock, flags);

	return 0;
}

static int nvt_udc_ep_dequeue(struct usb_ep *_ep, struct usb_request *_req)
{
	struct nvt_udc_ep *ep = container_of(_ep, struct nvt_udc_ep, ep);
	struct nvt_udc_request *req;

	if(fotg330_dbg)
		printf("nvt_udc_ep_dequeue\n");
#if 1
	/* make sure it's actually queued on this endpoint */
	list_for_each_entry(req, &ep->queue, queue) {
		if (&req->req == _req)
			break;
	}
	if (&req->req != _req)
		return -EINVAL;

	/* remove the request */
	list_del_init(&req->queue);

	/* update status & invoke complete callback */
	if (req->req.status == -EINPROGRESS) {
		req->req.status = -ECONNRESET;
		if (req->req.complete)
			req->req.complete(_ep, &req->req);
	}
#endif
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


static struct nvt_udc_chip ext_controller3 = {
	.ext_regs = (void __iomem *)IOADDR_USB3_REG_BASE,
	.gadget = {
		.name = "nvt_udc",
		.ops = &nvt_udc_gadget_ops,
		.ep0 = &ext_controller3.ep[0].ep,
		.speed = USB_SPEED_HIGH,
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
		.chip      = &ext_controller3,
		.maxpacket = CFG_EP0_MAX_PACKET_SIZE,
	},
	.ep[1] = {
		.id = 1,
		.ep = {
			.name  = "ep1",
			.ops   = &nvt_udc_ep_ops,
		},
		.chip      = &ext_controller3,
		.maxpacket = CFG_EPX_MAX_PACKET_SIZE,
	},
	.ep[2] = {
		.id = 2,
		.ep = {
			.name  = "ep2",
			.ops   = &nvt_udc_ep_ops,
		},
		.chip      = &ext_controller3,
		.maxpacket = CFG_EPX_MAX_PACKET_SIZE,
	},
	.ep[3] = {
		.id = 3,
		.ep = {
			.name  = "ep3",
			.ops   = &nvt_udc_ep_ops,
		},
		.chip      = &ext_controller3,
		.maxpacket = CFG_EPX_MAX_PACKET_SIZE,
	},
	.ep[4] = {
		.id = 4,
		.ep = {
			.name  = "ep4",
			.ops   = &nvt_udc_ep_ops,
		},
		.chip      = &ext_controller3,
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

	if(fotg330_dbg)
		printf("nvt_udc: req->bRequest (%d)\n", req->bRequest);

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


static void fusb300_fill_idma_prdtbl(struct nvt_udc_ep *ep, uint8_t *d,
		u32 len)
{
	u32 value;
	u32 reg;
	//u32 fifo_time;
	//u32 *data;
#if 0
	data = (u32 *)d;

	printf("  ep = %d ,  len   = 0x%08x  , len   = 0x%08x\n",ep->id, d , len);
	printf("  0x%08x\n", *data);
	printf("  0x%08x\n", *(data+1));
	printf("  0x%08x\n", *(data+2));
	printf("  0x%08x\n", *(data+3));
	printf("  0x%08x\n", *(data+4));
	printf("  0x%08x\n", *(data+5));
	printf("  0x%08x\n", *(data+6));
	printf("  0x%08x\n", *(data+7));
#endif	

	/* wait SW owner */
	do {
		reg = readl(IOADDR_USB3_DEVMODE_BASE +
			FUSB300_OFFSET_EPPRD_W0(ep->id));
		reg &= FUSB300_EPPRD0_H;
	} while (reg);


	if (ep->desc->bEndpointAddress & USB_DIR_IN)
		flush_dcache_range((ulong)d, (ulong)d + roundup(len, ARCH_DMA_MINALIGN));
	else
		invalidate_dcache_range((ulong)d, (ulong)d + roundup(len, ARCH_DMA_MINALIGN));	

	writel(virt_to_phys(d), IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_EPPRD_W1(ep->id));

	value = FUSB300_EPPRD0_BTC(len) | FUSB300_EPPRD0_H |
		FUSB300_EPPRD0_F | FUSB300_EPPRD0_L | FUSB300_EPPRD0_I;
	writel(value, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_EPPRD_W0(ep->id));

	writel(0x0, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_EPPRD_W2(ep->id));

	if((uvc_enable)&&(hbw_ep==ep->id))
	{
		printf("UVC func not supported\n");
#if 0	
		while(1){
			if(fusb300_chkEPEmpty(ep))
			{
				//printf("EP fifo full\n");
				break;
			}
		}

		// Set EP stall
		fusb300_enable_uvc(ep->fusb300, ep->epnum);
#endif		
	}

	fusb300_enable_bit(FUSB300_OFFSET_EPPRDRDY, FUSB300_EPPRDR_EP_PRD_RDY(ep->id));

	//pr_info("Len = 0x%x\r\n",len);
	if((uvc_enable)&&(hbw_ep==ep->id))
	{
#if 0	
		fifo_time = (ep->fusb300->gadget.speed = USB_SPEED_SUPER)? 2 : 9;
		if(len >= (1024*fifo_entry_num)) {
			while(1){
				if(fusb300_chkEPFull(ep))
				{
					//printf("EP fifo full\n");
					break;
				}
			}
		}else {
			udelay(fifo_time*fifo_entry_num);
		}
		// Release EP stall
		fusb300_disable_uvc(ep->fusb300, ep->epnum);
#endif		
	}
}

static void fusb300_wait_idma_finished(struct nvt_udc_ep *ep)
{
	u32 reg;

	do {
		reg = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGR1);
		if ((reg & FUSB300_IGR1_VBUS_CHG_INT) ||
		    (reg & FUSB300_IGR1_WARM_RST_INT) ||
		    (reg & FUSB300_IGR1_HOT_RST_INT) ||
		    (reg & FUSB300_IGR1_USBRST_INT)
		)
			goto IDMA_RESET;
		reg = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGR0);
		reg &= FUSB300_IGR0_EPn_PRD_INT(ep->id);
	} while (!reg);

	fusb300_clear_int(FUSB300_OFFSET_IGR0,
		FUSB300_IGR0_EPn_PRD_INT(ep->id));
	return;

IDMA_RESET:
	reg = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGER0);
	reg &= ~FUSB300_IGER0_EEPn_PRD_INT(ep->id);
	writel(reg, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGER0);
}


static void fusb300_set_idma(struct nvt_udc_ep *ep,
			struct nvt_udc_request *req)
{
	//int ret;
	//uint32_t *data;
	uint8_t *buf  = req->req.buf + req->req.actual;
	uint32_t len  = req->req.length - req->req.actual;
	dma_addr_t d = req->req.dma;

	//ret = usb_gadget_map_request(&ep->chip->gadget,
	//		&req->req, ep->ep.desc->bEndpointAddress& USB_DIR_IN);
	//if (ret)
	//	return;
	//printf("  buf   = 0x%08x  , len   = 0x%08x\n", buf , len);

	fusb300_enable_bit( FUSB300_OFFSET_IGER0,
		FUSB300_IGER0_EEPn_PRD_INT(ep->id));

	if(ep->ep.desc->bEndpointAddress& USB_DIR_IN) {
		//fusb300_fill_idma_prdtbl(ep, req->req.dma, req->req.length);
		fusb300_fill_idma_prdtbl(ep, buf, len);
	} else {
		//fusb300_fill_idma_prdtbl(ep, req->req.dma + req->req.actual, req->req.length);
		fusb300_fill_idma_prdtbl(ep, buf, len);
	}
	/* check idma is done */
	fusb300_wait_idma_finished(ep);


	//usb_gadget_unmap_request(&ep->chip->gadget,
	//		&req->req, ep->ep.desc->bEndpointAddress& USB_DIR_IN);
}


//static void fusb330_rdfifo(struct fusb300_ep *ep,
//			  struct fusb300_request *req,
//			  u32 length)
static void fusb330_rdfifo(u32 epnum, 
			struct nvt_udc_request *req,
			u32 length)
{
	int i = 0;
	u8 *tmp;
	u32 data, reg;
	//struct fusb300 *fusb300 = ep->fusb300;	

	tmp = req->req.buf + req->req.actual;
	req->req.actual += length;

	//printf("req->req.actual = %x\n",req->req.actual);
	//printf("req->req.buf= %x\n",req->req.buf);	

	if (req->req.actual > req->req.length)
		printf("req->req.actual > req->req.length\n");

	for (i = (length >> 2); i > 0; i--) {
		data = readl(IOADDR_USB3_DEVMODE_BASE +
			FUSB300_OFFSET_EPPORT(epnum));
		//printf("  0x%08x\n", data);
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		*(tmp + 3) = (data >> 24) & 0xFF;
		tmp = tmp + 4;
	}

	switch (length % 4) {
	case 1:
		data = readl(IOADDR_USB3_DEVMODE_BASE +
			FUSB300_OFFSET_EPPORT(epnum));
		*tmp = data & 0xFF;
		break;
	case 2:
		data = readl(IOADDR_USB3_DEVMODE_BASE +
			FUSB300_OFFSET_EPPORT(epnum));
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		break;
	case 3:
		data = readl(IOADDR_USB3_DEVMODE_BASE +
			FUSB300_OFFSET_EPPORT(epnum));
		*tmp = data & 0xFF;
		*(tmp + 1) = (data >> 8) & 0xFF;
		*(tmp + 2) = (data >> 16) & 0xFF;
		break;
	default:
		break;
	}

	do {
		reg = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGR1);
		reg &= FUSB300_IGR1_SYNF0_EMPTY_INT;
		if (i)
			printf("sync fifo is not empty!\n");
		i++;
	} while (!reg);
}

static void usb3dev_set_tx0byte(struct nvt_udc_ep *ep)
{
	u32 reg = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_EPFFR(ep->id));

	reg = reg|(FUSB300_TX0BYTE);
	writel(reg, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_EPFFR(ep->id));
}

static void in_ep_fifo_handler(u32 epnum, struct nvt_udc_ep *ep)
{
	//struct nvt_udc_request *req = list_entry(ep->queue.next, struct nvt_udc_request, queue);
	struct nvt_udc_request *req = list_first_entry(&ep->queue, struct nvt_udc_request, queue);

	if(fotg330_dbg)
		printf("in_ep_fifo_handler epnum=%d  fifo len = 0x%x\r\n",epnum, req->req.length);

	//req = container_of(ep->chip, struct nvt_udc_request, req);

	if (req->req.length)
		fusb300_set_idma(ep, req);
	else
	{
		//pr_info("TX0B ep->epnum [0x%x] \r\n",ep->epnum);
		usb3dev_set_tx0byte(ep);
	}
	req->req.status = 0;
	done(ep, req, 0);

	//printf("req = 0x%lx\r\n",req);
}

static void out_ep_fifo_handler(u32 epnum, struct nvt_udc_ep *ep)
{
	u32 reg = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_EPFFR(epnum));
	u32 length = reg & FUSB300_FFR_BYCNT;
	struct nvt_udc_request *req = list_entry(ep->queue.next, struct nvt_udc_request, queue);
	u32 i;

	if(fotg330_dbg)
		printf("out_ep_fifo_handler epnum=%d  fifo len = 0x%x\r\n",epnum, req->req.length);
	if(pio_on) {
		fusb330_rdfifo(epnum, req, length);
		//reg = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_EPPRD_W0(epnum));
		//req->req.actual += (req->req.length - (reg&FUSB300_FFR_BYCNT));
		//req->req.status = 0;
		//done(ep, req, 0);
		//if (req->req.complete)
		//	req->req.complete(&ep->ep, &req->req);
		//done(ep, req, 0);
		//printf("req->req.actual 0x%x  fifo len = 0x%x\r\n",req->req.actual, length);
		if ((req->req.length == req->req.actual) || (length < ep->ep.maxpacket)){
			req->req.status = 0;
			done(ep, req, 0);
		}
	} else {
		fusb300_set_idma(ep, req);
		reg = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_EPPRD_W0(epnum));
		req->req.actual += (req->req.length - (reg&FUSB300_FFR_BYCNT));

		req->req.status = 0;
		done(ep, req, 0);

		//if(req->req.complete != NULL) {
		//	req->req.complete(&ep->ep, &req->req);
		//}
	}
}

int usb_gadget_handle_interrupts(int index)
{
	struct nvt_udc_chip *chip = &ext_controller3;
	//struct nvt_udc_ext_regs *regs = chip->ext_regs;
	//unsigned long id, st, isr, gisr;	
	u32 int_grp1 = readl(IOADDR_USB3_DEVMODE_BASE+ FUSB300_OFFSET_IGR1);
	u32 int_grp1_en = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGER1);
	u32 int_grp0 = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGR0);
	u32 int_grp0_en = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGER0);
	int i;
	u32 reg;
	u8 in;
	struct nvt_udc_ep  *ep;

	spin_lock(&u3lock);

	//if((int_grp1!=0x10000000)||(int_grp0!=0))
		//printf("<<<<<<<<<<<<<<<<<<<<<<<fusb330_irq>>>>>>>>>> [0x%x] [0x%x]\r\n",int_grp0,int_grp1);
	//printf("fusb330_irq_en [0x%x] [0x%x]\r\n",int_grp0_en,int_grp1_en);	

	int_grp1 &= int_grp1_en;
	int_grp0 &= int_grp0_en;

	if(!(int_grp0 || int_grp1))
		return 0;
	
	writel(int_grp0, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGR0);
	writel(int_grp1, IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGR1);


	if (int_grp1 & FUSB300_IGR1_WARM_RST_INT) {	
		if(fotg330_dbg)
			printf(" FUSB300_IGR1_WARM_RST_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_HOT_RST_INT) {
		if(fotg330_dbg)
			printf(" FUSB300_IGR1_HOT_RST_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_USBRST_INT) {
		if(fotg330_dbg)
			printf(" FUSB300_IGR1_USBRST_INT\n");
	}
	/* COMABT_INT has a highest priority */
	if (int_grp1 & FUSB300_IGR1_CX_COMABT_INT) {
		if(fotg330_dbg)
			printf(" FUSB300_IGR1_CX_COMABT_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_VBUS_CHG_INT) {
		if(fotg330_dbg)
			printf(" FUSB300_IGR1_VBUS_CHG_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_U3_ENTRY_INT) {
		if(fotg330_dbg)
			printf(" FUSB300_IGR1_U3_ENTRY_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_U2_ENTRY_INT) {
		if(fotg330_dbg)
			printf(" FUSB300_IGR1_U2_ENTRY_INT\n");
	}

	if (int_grp1 & FUSB300_IGR1_U1_ENTRY_INT) {
		if(fotg330_dbg)
			printf(" FUSB300_IGR1_U1_ENTRY_INT\n");
	}


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
		if(fotg330_dbg)
			printf("fusb330_cxout\n");
		//fusb330_ep0out(chip);
	}

	if (int_grp1 & FUSB300_IGR1_CX_IN_INT) {
		if(fotg330_dbg)
			printf("fusb330_cxin\n");
		//fusb330_ep0in(fusb300);
	}

	if (int_grp0) {
		if(fotg330_dbg)
			printf(" FUSB300_IGR0_INT\n");
		#if 1
		for (i = 1; i <= FUSB330_MAX_NUM_EP; i++) {
			if (int_grp0 & FUSB300_IGR0_EPn_FIFO_INT(i)) {

				reg = readl(IOADDR_USB3_DEVMODE_BASE+ FUSB300_OFFSET_EPSET1(i));

				ep = &chip->ep[i];

				in = (reg & FUSB300_EPSET1_DIRIN) ? 1 : 0;
				if (in)
					in_ep_fifo_handler(i,ep);
				else
					out_ep_fifo_handler(i,ep);
			}
		}
		#endif
#if 0
		int_grp0 = readl(IOADDR_USB3_DEVMODE_BASE + FUSB300_OFFSET_IGR0);
		for (i = 1; i <= FUSB330_MAX_NUM_EP; i++) {
			//printf(" Loop test %d\n",i);
			if(int_grp0&(1<<i))
				enable_fifo_int(i);
		}
#endif
			
	}	
	spin_lock(&u3lock);
	return 0;
}


int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	int i, ret = 0;

	struct nvt_udc_chip *chip = &ext_controller3;

	//printf("fotg330 usb_gadget_register_driver\n");

	if (!driver    || !driver->bind || !driver->setup) {
		puts("nvt_udc: bad parameter.\n");
		return -EINVAL;
	}

	INIT_LIST_HEAD(&chip->gadget.ep_list);
	for (i = 0; i < CFG_NUM_ENDPOINTS + 1; ++i) {
		struct nvt_udc_ep *ep = chip->ep + i;

		ep->ep.maxpacket = ep->maxpacket;
		//printf("%d,  ep->ep.maxpacket =  %d\r\n",i,ep->ep.maxpacket);
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
	spin_lock_init(&u3lock);

	printf("fotg330 usb_gadget_register_driver ver %s\r\n",DRV_VERSION);

	return ret;
}


int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct nvt_udc_chip *chip = &ext_controller3;


	driver->disconnect(&chip->gadget);
	driver->unbind(&chip->gadget);
	chip->driver = NULL;

	pullup(chip, 0);
	//printf("udc remove~~\n");

	return 0;
}


