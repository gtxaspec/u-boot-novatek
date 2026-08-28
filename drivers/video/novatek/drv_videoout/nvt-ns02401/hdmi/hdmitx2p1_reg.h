/*
    HDMI 2.1 register header file

    HDMI 2.1 register header file.

    @file       hdmitx2p1_reg.h
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2024.  All rights reserved.
*/

#ifndef _HDMITX2P1_REG_H
#define _HDMITX2P1_REG_H

#include "hdmi_port.h"

#include "hdmitx_int.h"

#include <asm/arch/IOAddress.h>
#include <asm/nvt-common/rcw_macro_bit.h>
#include <asm/arch/efuse_protected.h>





#define HTX_LINK_CTRL0 0x0					//Page 6A; 
#define HTX_LINK_CTRL1 0x1					//Page 6B;
#define HTX_LINK_CTRL2 0x2					//Page 60;
#define HTX_PHY_CTRL 0x7						//Page 87;
#define HTX_PPS_CTRL 0x3					//Page 32; 
#define HTX_EMP_CTRL 0x4					//Page 33; 
#define HTX_FRL_CTRL 0xF					//Page 34;

#define REG_MAP_2P1(x,y) ((x << 8) + y)<<2

#define BIT0 (1 << 0)
#define BIT1 (1 << 1)
#define BIT2 (1 << 2)
#define BIT3 (1 << 3)
#define BIT4 (1 << 4)
#define BIT5 (1 << 5)
#define BIT6 (1 << 6)
#define BIT7 (1 << 7)










void hdmitx2p1_write_reg(UINT32 id, UINT32 page, UINT32 ui_offset, UINT32 ui_value);
void hdmitx2p1_write_reg_bit(UINT32 id, UINT32 page, UINT32 ui_offset, UINT32 mask, UINT32 ui_value);
void hdmitx2p1_write_reg_mask(UINT32 id, UINT32 page, UINT32 ui_offset, UINT32 mask, UINT32 ui_value);
UINT32 hdmitx2p1_read_reg(UINT32 id, UINT32 page, UINT32 ui_offset);




//
//  HDMI TX 2.1 Module Registers
//

//HDMI_TX LINK (Page 6A/6D)
#define ADR_HDMI_SRST						(0x005)
#define BIT_HDMI_SRST_SWRST					(1 << 0)
#define BIT_HDMI_SRST_FIFORST				(1 << 1)
#define ADR_HDMI_SYS_CTRL1					(0x008)
#define BIT_HDMI_SYS_CTRL1_PWR				(1 << 0)
#define BIT_HDMI_SYS_CTRL1_EDGE				(1 << 1)
#define BIT_HDMI_SYS_CTRL1_BSEL				(1 << 2)
#define BIT_HDMI_SYS_CTRL1_HEN				(1 << 4)
#define BIT_HDMI_SYS_CTRL1_VEN				(1 << 5)
#define BIT_HDMI_SYS_CTRL1_VSYNC			(1 << 6)
#define ADR_HDMI_SYS_STAT					(0x009)
#define BIT_HDMI_SYS_STAT_P_STABLE			(1 << 0)
#define BIT_HDMI_SYS_STAT_HPD				(1 << 1)
#define BIT_HDMI_SYS_STAT_RSEN				(1 << 2)
#define BIT_HDMI_SYS_STAT_VLOW				(1 << 7)
#define ADR_HDMI_SYS_CTRL3					(0x00A)
#define ADR_HDMI_LEGACY1					(0x00B)
#define ADR_HDMI_SYS_CTRL4					(0x00C)
#define ADR_HDMI_DCTL						(0x00D)
#define BIT_HDMI_DCTL_HDCP_SEL				(1 << 0)
#define BIT_HDMI_DCTL_AUD_MUTE				(1 << 1)
#define BIT_HDMI_DCTL_VID_BLANK				(1 << 2)
#define ADR_HDMI_LEGACY3					(0x00E)
#define ADR_HDMI_HDCP_CTRL					(0x00F)
#define BIT_HDMI_HDCP_CTRL_ENC_EN			(1 << 0)
#define BIT_HDMI_HDCP_CTRL_RI_RDY			(1 << 1)
#define BIT_HDMI_HDCP_CTRL_CP_RESTN			(1 << 2)
#define BIT_HDMI_HDCP_CTRL_TX_ANSTOP		(1 << 3)
#define BIT_HDMI_HDCP_CTRL_RX_RPTR			(1 << 4)
#define BIT_HDMI_HDCP_CTRL_BKSV_ERR			(1 << 5)
#define BIT_HDMI_HDCP_CTRL_ENC_ON			(1 << 6)


#define ADR_HDMI_HDCP_BKSV1					(0x010)
#define ADR_HDMI_HDCP_BKSV2					(0x011)
#define ADR_HDMI_HDCP_BKSV3					(0x012)
#define ADR_HDMI_HDCP_BKSV4					(0x013)
#define ADR_HDMI_HDCP_BKSV5					(0x014)
#define ADR_HDMI_HDCP_AN1					(0x015)
#define ADR_HDMI_HDCP_AN2					(0x016)
#define ADR_HDMI_HDCP_AN3					(0x017)
#define ADR_HDMI_HDCP_AN4					(0x018)
#define ADR_HDMI_HDCP_AN5					(0x019)
#define ADR_HDMI_HDCP_AN6					(0x01A)
#define ADR_HDMI_HDCP_AN7					(0x01B)
#define ADR_HDMI_HDCP_AN8					(0x01C)
#define ADR_HDMI_HDCP_AKSV1					(0x01D)
#define ADR_HDMI_HDCP_AKSV2					(0x01E)
#define ADR_HDMI_HDCP_AKSV3					(0x01F)
#define ADR_HDMI_HDCP_AKSV4					(0x020)
#define ADR_HDMI_HDCP_AKSV5					(0x021)
#define ADR_HDMI_HDCP_RI1					(0x022)
#define ADR_HDMI_HDCP_RI2					(0x023)
#define ADR_HDMI_HDCP_RI_128_COMP			(0x024)
#define ADR_HDMI_HDCP_RI_CONT				(0x025)
#define ADR_HDMI_HDCP_RI_STAT				(0x026)
#define BIT_HDMI_RI_STAT_RI_STARTED			(1 << 0)

#define ADR_HDMI_HDCP_RI_CMD				(0x027)
#define BIT_HDMI_RI_CMD_RI_EN				(1 << 0)
#define BIT_HDMI_RI_CMD_BCAP_EN				(1 << 1)
#define BIT_HDMI_RI_CMD_1STAT_EN			(1 << 2)

#define ADR_HDMI_HDCP_RI_START				(0x028)
#define ADR_HDMI_HDCP_RI_RX_L				(0x029)
#define ADR_HDMI_HDCP_RI_RX_H				(0x02A)
#define ADR_HDMI_HDCP_RI_DEBUG				(0x02B)
#define BIT_HDMI_RI_DEBUG_RI_DBG_HOLD  (1 << 6)
#define BIT_HDMI_RI_DEBUG_RI_DBG_TRASH (1 << 7)

#define _HDMITXLINK_INPUT_HV_POL_		(0x033)
#define _HDMITXLINK_INPUT_HTOTAL_LO		(0x03A)
#define _HDMITXLINK_INPUT_HTOTAL_HI		(0x03B)
#define _HDMITXLINK_INPUT_VTOTAL_LO		(0x03C)
#define _HDMITXLINK_INPUT_VTOTAL_HI		(0x03D)
#define _HDMITXLINK_INTPUT_DET 			(0x03F)
#define _HDMITXLINK_HSYNC_POL			BIT0
#define _HDMITXLINK_VSYNC_POL			BIT1
#define _HDMITXLINK_ITERLACE_POL 		BIT2

#define _HDMITXLINK_CSCSEL				(0x048)
#define _HDMITXLINK_VID_ACEN			(0x049)
#define _HDMITXLINK_VID_MODE			(0x04A)
#define _HDMITXLINK_VID_BLANK1			(0x04B)
#define _HDMITXLINK_VID_BLANK2			(0x04C)
#define _HDMITXLINK_VID_BLANK3			(0x04D)
#define _HDMI_XWIN_REKEY_CNT			(0x056)
#define _HDMITXLINK_HDE_LO				(0x060)
#define _HDMITXLINK_HDE_HI				(0x061)
#define _HDMITXLINK_VDE_LO				(0x062)
#define _HDMITXLINK_VDE_HI				(0x063)
#define _HDMITXLINK_HSW_LO				(0x064)
#define _HDMITXLINK_HSW_HI				(0x065)
#define _HDMITXLINK_VSW_LO				(0x066)
#define _HDMITXLINK_VSW_HI				(0x067)
#define _HDMITXLINK_HFP_LO				(0x068)
#define _HDMITXLINK_HFP_HI				(0x069)
#define _HDMITXLINK_VFP_LO				(0x06A)
#define _HDMITXLINK_VFP_HI				(0x06B)
#define ADR_HDMI_INTR_STATE					(0x070)
#define ADR_HDMI_INTR1						(0x071)
#define ADR_HDMI_INTR2						(0x072)
#define ADR_HDMI_INTR3						(0x073)
#define ADR_HDMI_INT_UNMASK1					(0x075)
#define ADR_HDMI_INT_UNMASK2					(0x076)
#define ADR_HDMI_INT_UNMASK3					(0x077)
#define ADR_HDMI_INT_UNMASK4					(0x078)
#define ADR_HDMI_INT_CTRL						(0x079)
#define BIT_HDMI_INT_CTRL_POLARITY				(1 << 1)

//
#define OTPRAM_LD								(0x0FA)
#define BIT_LD_KSV								(1 << 5)
//
#define OTPRAM_DATA								(0x0FC)
#define OTPNAND_CTRL							(0x0FD)
#define OTPRAM_CTRL								(0x0FE)
#define _HDMITXLINK_REPETITION_Ctrl     (0x082)
//#define _HDMITXLINK_DPColor_Ctrl 		(0x085)
#define _HDMITXLINK_LineBuf_Ctrl 		(0x086)
#define _HDMITXLINK_TMDS_Ctrl8 			(0x088)
#define _HDMITXLINK_OUTPUT_HV_POL 		(0x0BA)
//---------------------------------- DDC Master Registers ----------------------------------//
#define ADR_HDMI_SHA_CTRL						(0x0CC)
#define ADR_HDMI_DDC_MAN						(0x0EC)
#define ADR_HDMI_DDC_ADDR						(0x0ED)
#define ADR_HDMI_DDC_SEGM 						(0x0EE)
#define ADR_HDMI_DDC_OFFSET 					(0x0EF)
#define ADR_HDMI_DDC_COUNT1 					(0x0F0)
#define ADR_HDMI_DDC_COUNT2 					(0x0F1)
#define BIT_HDMI_DDC_COUNT2						0x03
#define ADR_HDMI_DDC_STATUS 					(0x0F2)
#define BIT_HDMI_M0_RD_EN						(1 << 3)
#define BIT_HDMI_DDC_STATUS_FWT_USE				(1 << 0)
#define BIT_HDMI_DDC_STATUS_FRD_USE				(1 << 1)
#define BIT_HDMI_DDC_STATUS_FIFO_EMPTY			(1 << 2)
#define BIT_HDMI_DDC_STATUS_FIFO_FULL			(1 << 3)
#define BIT_HDMI_DDC_STATUS_IN_PROG				(1 << 4)
#define BIT_HDMI_DDC_STATUS_NO_ACK				(1 << 5)
#define BIT_HDMI_DDC_STATUS_BUS_LOW				(1 << 6)
#define ADR_HDMI_DDC_CMD 						(0x0F3)
#define BIT_HDMI_DDC_CMD								0x0F
#define BIT_HDMI_DDC_CMD_ABORT							0x0F
#define BIT_HDMI_DDC_CMD_CLEAR_FIFO						0x09
#define BIT_HDMI_DDC_CMD_RESET							0x0A
#define BIT_HDMI_DDC_CMD_CURRENT_ADDR_READ				0x00 //no ACK on last byte
#define BIT_HDMI_DDC_CMD_SEQUENTIAL_READ				0x02 //no ACK on last byte
#define BIT_HDMI_DDC_CMD_ENHANCED_DDC_READ				0x04 //no ACK on last byte
#define BIT_HDMI_DDC_CMD_SEQUENTIAL_WRITE_IGNORE_ACK	0x06 //ignore ACK on last byte
#define BIT_HDMI_DDC_CMD_SEQUENTIAL_WRITE				0x07 //require ACK on last byte
//
#define ADR_HDMI_DDC_DATA 						(0x0F4)
#define ADR_HDMI_DDC_FIFOCNT 					(0x0F5)
#define BIT_HDMI_DDC_FIFOCNT					(0x1F << 0)
#define _HDMITX_DDC_SCL_SPDLO 					(0x0F6)
#define _HDMITX_DDC_SCL_SPDHI 					(0x0F7)


//HDMI_TX LINK (Page 6B/6E)
//HDMI_TX LINK_AUDIO
#define _HDMITXLINK_N_CTS_Ctrl 			(0x01)
#define _HDMITXLINK_N_SVAL_LO 			(0x03)
#define _HDMITXLINK_N_SVAL_MI 			(0x04)
#define _HDMITXLINK_N_SVAL_HI 			(0x05)
#define _HDMITXLINK_CTS_SVAL_LO 		(0x06)
#define _HDMITXLINK_CTS_SVAL_MI 		(0x07)
#define _HDMITXLINK_CTS_SVAL_HI 		(0x08)
#define _HDMITXLINK_Audio_SD_Ctrl 		(0x14)
#define _HDMITXLINK_Audio_LEN_Ctrl 		(0x18)
#define _HDMITXLINK_Audio_I2S_Ctrl 		(0x1D)
#define _HDMITXLINK_ChannelStatus 		(0x1E)
#define _HDMITXLINK_ChannelStatus2 		(0x1F)
#define _HDMITXLINK_ChannelStatus3 		(0x20)
#define _HDMITXLINK_ChannelStatus4 		(0x21)
#define _HDMITXLINK_ChannelStatus5 		(0x22)
#define _HDMITXLINK_Audio_FRL_Mode   	(0x23)
#define _HDMITXLINK_Audio_I2S_Ctrl2 	(0x24)
#define _HDMITXLINK_Mode_DeepColor 		(0x2F)
#define _HDMITXLINK_SSCP_EN 			(0x30)

//
#define ADR_HDMI_DPD 					(0x3D)
#define BIT_HDMI_DPD_PWRTOT						(1 << 0)
#define BIT_HDMI_DPD_PWROSC						(1 << 1)
#define BIT_HDMI_DPD_PWRIDCK					(1 << 2)
#define BIT_HDMI_DPD_TCLKPHZ					(1 << 3)
//

//HDMI_TX LINK_PACKET
#define _HDMITXLINK_Packet_Ctrl_0 		(0x3E)
#define _HDMITXLINK_Packet_Ctrl_1 		(0x3F)

//AVI_Info Packet: 0x6A00/0x6D00 + 0x140 ~ 0x152
#define _HDMITXLINK_AVI_Type_Code 		(0x40)
#define _HDMITXLINK_AVI_HB1 			(0x41)
#define _HDMITXLINK_AVI_HB2 			(0x42)
#define _HDMITXLINK_AVI_HB3_SUM 		(0x43)
#define _HDMITXLINK_AVI_DB1 			(0x44)



//SPD Packet: 0x6A00/0x6D00 + 0x160 ~ 0x17E
#define _HDMITXLINK_SPD_HB0 			(0x60)
#define _HDMITXLINK_SPD_HB1 			(0x61)
#define _HDMITXLINK_SPD_HB2 			(0x62)
#define _HDMITXLINK_SPD_HB3_SUM 		(0x63)
#define _HDMITXLINK_SPD_DB1 			(0x64)

//Audio_Info Packet: 0x6A00/0x6D00 + 0x180 ~ 0x18D
#define _HDMITXLINK_AUDInfo_Type_Code 	(0x80)
#define _HDMITXLINK_AUDInfo_HB1 		(0x81)
#define _HDMITXLINK_AUDInfo_HB2 		(0x82)
#define _HDMITXLINK_AUDInfo_HB3_SUM 	(0x83)
#define _HDMITXLINK_AUDInfo_DB1 		(0x84)

//MPEG Packet: 0x6A00/0x6D00 + 0x1A0 ~ 0x1BE
#define _HDMITXLINK_MPEG_HB0 			(0xA0)
#define _HDMITXLINK_MPEG_HB1 			(0xA1)
#define _HDMITXLINK_MPEG_HB2 			(0xA2)
#define _HDMITXLINK_MPEG_HB3_SUM 		(0xA3)
#define _HDMITXLINK_MPEG_DB1 			(0xA4)


#define _HDMITXLINK_Gen_HB0 			(0xC0)
#define _HDMITXLINK_AvMute_Ctrl 		(0xDF)




//HDMI_TX_PHY (Page 87/88)
#define _HDMITXPHY_TTX_CLK_CFG 					(0x000)
#define _HDMITXPHY_TTX_RST_CFG 					(0x001)
#define _HDMITXPHY_TTX_SWAP_MODE 				(0x002)
#define _HDMITXPHY_TTX_SWAP_CFG1				(0x003)
#define _HDMITXPHY_TTX_PLL_CFG0					(0x004)
#define _HDMITXPHY_TTX_PLL_CFG1					(0x005)
#define _HDMITXPHY_TTX_PLL_CFG2					(0x006)
#define _HDMITXPHY_TTX_PLL_CFG3					(0x007)
#define _HDMITXPHY_TTX_PLL_CFG4					(0x008)
#define _HDMITXPHY_TTX_LDO_CFG0					(0x009)
#define _HDMITXPHY_TTX_AC_BOOST_CFG0 			(0x00A)
#define _HDMITXPHY_TTX_BIAS_CFG0 				(0x00B)
#define _HDMITXPHY_TTX_RESTRIM_CFG0				(0x00C)
#define _HDMITXPHY_TTX_RESTRIM_CFG1				(0x00D)
#define _HDMITXPHY_TTX_RESTRIM_CFG2				(0x00E)
#define _HDMITXPHY_TTX_PHYCTL_CFGA				(0x00F)
#define _HDMITXPHY_PG_CFG						(0x010)
#define _HDMITXPHY_PG_PG_CFG					(0x011)
#define _HDMITXPHY_PG_PG_CFG2					(0x012)
#define _HDMITXPHY_PG_PG_GRAY_L					(0x013)
#define _HDMITXPHY_PG_PG_GRAY_H					(0x014)
#define _HDMITXPHY_PG_SG_HTOT_L					(0x015)
#define _HDMITXPHY_PG_SG_HTOT_H					(0x016)
#define _HDMITXPHY_PG_SG_HSW_L					(0x017)
#define _HDMITXPHY_PG_SG_HSW_H					(0x018)
#define _HDMITXPHY_PG_SG_HAB_L					(0x019)
#define _HDMITXPHY_PG_SG_HAB_H					(0x01A)
#define _HDMITXPHY_PG_SG_HAW_L					(0x01B)
#define _HDMITXPHY_PG_SG_HAW_H					(0x01C)
#define _HDMITXPHY_PG_SG_VTOT_L					(0x01D)
#define _HDMITXPHY_PG_SG_VTOT_H					(0x01E)
#define _HDMITXPHY_PG_SG_VSW_L					(0x01F)
#define _HDMITXPHY_PG_SG_VSW_H					(0x020)
#define _HDMITXPHY_PG_SG_VAB_L					(0x021)
#define _HDMITXPHY_PG_SG_VAB_H					(0x022)
#define _HDMITXPHY_PG_SG_VAW_L					(0x023)
#define _HDMITXPHY_PG_SG_VAW_H					(0x024)
#define _HDMITXPHY_PG_SG_MUX					(0x025)

#define _HDMITXPHY_TTX_TG_CFG 					(0x030)
#define _HDMITXPHY_SCCP_HSYNC_L_DLY				(0x031)
#define _HDMITXPHY_SCCP_HSYNC_H_DLY				(0x032)
#define _HDMITXPHY_SCCP_PIXEL_L_DLY				(0x033)
#define _HDMITXPHY_SCCP_PIXEL_H_DLY				(0x034)
#define _HDMITXPHY_SYNC_SCCP_CFG 				(0x035)
#define _HDMITXPHY_TTX_PRTY_CFG0				(0x036)
												
#define _HDMITXPHY_TTX_PRTY_CFG1				(0x037)
#define _HDMITXPHY_TTX_PRBS_CFG0				(0x038)
#define _HDMITXPHY_TTX_FRL_CFG0					(0x039)
#define _HDMITXPHY_TTX_FRL_CFG1					(0x03A)
#define _HDMITXPHY_TTX_FRL_CFG2					(0x03B)
#define _HDMITXPHY_TTX_FRL_CFG3					(0x03C)
#define _HDMITXPHY_TTX_FRL_CFG4					(0x03D)
#define _HDMITXPHY_TTX_FRL_CFG5					(0x03E)
#define _HDMITXPHY_TTX_FRL_CFG6					(0x03F)

#define _HDMITXPHY_I2CS_CFG_CONTROL				(0x040)
#define _HDMITXPHY_I2CS_CFG2_CONTROL			(0x041)
#define _HDMITXPHY_I2CS_FLAG					(0x042)
#define _HDMITXPHY_I2CS_INTEN					(0x043)
#define _HDMITXPHY_I2CS_RWTO_NUM				(0x044)
#define _HDMITXPHY_I2CS_TSU_NUM					(0x045)

#define _HDMITXPHY_TTX_CHANNEL_TEST_CFG 		(0x050)
#define _HDMITXPHY_TTX_CH0_TEST_PATTERN_CFG0	(0x051)
#define _HDMITXPHY_TTX_CH0_TEST_PATTERN_CFG1	(0x052)
#define _HDMITXPHY_TTX_CH0_TEST_PATTERN_CFG2	(0x053)
#define _HDMITXPHY_TTX_CH0_TEST_PATTERN_CFG3	(0x054)
#define _HDMITXPHY_TTX_CH0_TEST_PATTERN_CFG4	(0x055)
#define _HDMITXPHY_TTX_CH1_TEST_PATTERN_CFG0	(0x056)
#define _HDMITXPHY_TTX_CH1_TEST_PATTERN_CFG1	(0x057)
#define _HDMITXPHY_TTX_CH1_TEST_PATTERN_CFG2	(0x058)
#define _HDMITXPHY_TTX_CH1_TEST_PATTERN_CFG3	(0x059)
#define _HDMITXPHY_TTX_CH1_TEST_PATTERN_CFG4	(0x05A)
#define _HDMITXPHY_TTX_CH2_TEST_PATTERN_CFG0	(0x05B)
#define _HDMITXPHY_TTX_CH2_TEST_PATTERN_CFG1	(0x05C)
#define _HDMITXPHY_TTX_CH2_TEST_PATTERN_CFG2	(0x05D)
#define _HDMITXPHY_TTX_CH2_TEST_PATTERN_CFG3	(0x05E)
#define _HDMITXPHY_TTX_CH2_TEST_PATTERN_CFG4	(0x05F)
#define _HDMITXPHY_TTX_CHC_TEST_PATTERN_CFG0	(0x060)
#define _HDMITXPHY_TTX_CHC_TEST_PATTERN_CFG1	(0x061)
#define _HDMITXPHY_TTX_CHC_TEST_PATTERN_CFG2	(0x062)
#define _HDMITXPHY_TTX_CHC_TEST_PATTERN_CFG3	(0x063)
#define _HDMITXPHY_TTX_CHC_TEST_PATTERN_CFG4	(0x064)
#define _HDMITXPHY_TTX_CH0_LFSR_SEED_CFG0		(0x065)
#define _HDMITXPHY_TTX_CH0_LFSR_SEED_CFG1		(0x066)
#define _HDMITXPHY_TTX_CH1_LFSR_SEED_CFG0		(0x067)
#define _HDMITXPHY_TTX_CH1_LFSR_SEED_CFG1		(0x068)
#define _HDMITXPHY_TTX_CH2_LFSR_SEED_CFG0		(0x069)
#define _HDMITXPHY_TTX_CH2_LFSR_SEED_CFG1		(0x06A)
#define _HDMITXPHY_TTX_CH3_LFSR_SEED_CFG0		(0x06B)
#define _HDMITXPHY_TTX_CH3_LFSR_SEED_CFG1		(0x06C)

#define _HDMITXPHY_TTX_SSB_LSB_PATCH_CFG0		(0x070)
#define _HDMITXPHY_TTX_SSB_LSB_PATCH_CFG1		(0x071)
#define _HDMITXPHY_TTX_SSB_MSB_PATCH_CFG0		(0x072)
#define _HDMITXPHY_TTX_SR_LSB_PATCH_CFG0		(0x073)
#define _HDMITXPHY_TTX_SR_LSB_PATCH_CFG1		(0x074)
#define _HDMITXPHY_TTX_SR_MSB_PATCH_CFG0		(0x075)
#define _HDMITXPHY_TTX_PRBS_CFG1				(0x076)
#define _HDMITXPHY_TTX_PRBS_CFG2				(0x077)
#define _HDMITXPHY_TTX_PRBS_CFG3				(0x078)
#define _HDMITXPHY_TTX_PRBS_CFG4				(0x079)

#define _HDMITXPHY_TTX_TGEN_CFG					(0x07A)
#define _HDMITXPHY_TTX_FRL_CFG7					(0x07B)
#define _HDMITXPHY_TTX_P2S_DRV_CFG1				(0x07C)
#define _HDMITXPHY_TTX_P2S_DRV_CFG2				(0x07D)
#define _HDMITXPHY_TTX_P2S_DRV_CFG3				(0x07E)
#define _HDMITXPHY_TTX_P2S_DRV_CFG4				(0x07F)
#define _HDMITXPHY_TTX_P2S_DRV_CFG5				(0x080)

#define _HDMITXPHY_TTX_PLL2_CFG0				(0x081)
#define _HDMITXPHY_TTX_PLL2_CFG1				(0x082)
#define _HDMITXPHY_TTX_PLL2_CFG2				(0x083)
#define _HDMITXPHY_TTX_PLL2_CFG3				(0x084)
#define _HDMITXPHY_TTX_PLL2_CFG4				(0x085)

#define _HDMITXPHY_TTX_FRL_CFG8					(0x091)
#define _HDMITXPHY_TTX_TXFFE_CFG0				(0x092)
#define _HDMITXPHY_TTX_TXFFE_CFG1				(0x093)
#define _HDMITXPHY_TTX_TXFFE_CFG2				(0x094)
#define _HDMITXPHY_TTX_TXFFE_CFG3				(0x095)
#define _HDMITXPHY_TTX_TXFFE_CFG4				(0x096)
#define _HDMITXPHY_TTX_TXFFE_CFG5				(0x097)
#define _HDMITXPHY_TTX_TXFFE_CFG6				(0x098)
#define _HDMITXPHY_TTX_TXFFE_CFG7				(0x099)
#define _HDMITXPHY_TTX_TXFFE_CFG8				(0x09A)
#define _HDMITXPHY_TTX_TXFFE_CFG9				(0x09B)
#define _HDMITXPHY_TTX_TXFFE_CFG10				(0x09C)

#define _HDMITXPHY_TTX_C_CFG1F					(0x0A0)
#define _HDMITXPHY_TTX_C_CFG20					(0x0A1)
#define _HDMITXPHY_TTX_C_CFG21					(0x0A2)
#define _HDMITXPHY_TTX_C_CFG22					(0x0A3)
#define _HDMITXPHY_TTX_D_CFG00					(0x0A4)
#define _HDMITXPHY_TTX_D_CFG01					(0x0A5)
#define _HDMITXPHY_TTX_C_CFG02					(0x0A6)
#define _HDMITXPHY_TTX_C_CFG03					(0x0A7)
#define _HDMITXPHY_TTX_C_CFG04					(0x0A8)
#define _HDMITXPHY_TTX_C_CFG05					(0x0A9)
#define _HDMITXPHY_TTX_C_CFG06					(0x0AA)

#if 0
#define _HDMITXPHY_DC_CONTROL_0					(0x0B0)
#define _HDMITXPHY_DC_CONTROL_1					(0x0B2)
#define _HDMITXPHY_DC_CONTROL_2					(0x0B3)
#define _HDMITXPHY_DC_CONTROL_3					(0x0B4)
#define _HDMITXPHY_DC_CONTROL_4					(0x0B5)
#define _HDMITXPHY_DC_CONTROL_5					(0x0B6)
#define _HDMITXPHY_DC_CONTROL_6					(0x0B7)

#define _HDMITXPHY_APHY_CONTROL_0				(0x0C0)
#define _HDMITXPHY_APHY_CONTROL_1				(0x0C1)
#define _HDMITXPHY_APHY_CONTROL_2				(0x0C2)
#define _HDMITXPHY_APHY_CONTROL_3				(0x0C3)
#define _HDMITXPHY_APHY_CONTROL_4				(0x0C4)
#define _HDMITXPHY_APHY_CONTROL_5				(0x0C5)
#define _HDMITXPHY_APHY_CONTROL_6				(0x0C6)
#define _HDMITXPHY_APHY_CONTROL_7				(0x0C7)
#define _HDMITXPHY_APHY_CONTROL_8				(0x0C8)
#define _HDMITXPHY_APHY_CONTROL_9				(0x0C9)
#define _HDMITXPHY_APHY_CONTROL_A				(0x0CA)
#endif

#define _HDMITXPHY_ADDR_MPLL_FD_CFG				(0x0D0)
#define _HDMITXPHY_ADDR_MPLL_FD_PERIOD_L		(0x0D1)
#define _HDMITXPHY_ADDR_MPLL_FD_PERIOD_M		(0x0D2)
#define _HDMITXPHY_ADDR_MPLL_FD_PERIOD_H		(0x0D3)
#define _HDMITXPHY_ADDR_MPLL_FD_FCNT_L			(0x0D4)
#define _HDMITXPHY_ADDR_MPLL_FD_FCNT_M			(0x0D5)
#define _HDMITXPHY_ADDR_MPLL_FD_FCNT_H			(0x0D6)
#define _HDMITXPHY_ADDR_MPLL_FD_FNUM_L			(0x0D7)
#define _HDMITXPHY_ADDR_MPLL_FD_FNUM_M			(0x0D8)
#define _HDMITXPHY_ADDR_MPLL_FD_FNUM_H			(0x0D9)
#define _HDMITXPHY_APHY_ADDR_MPLL_FD_FTH		(0x0DA)


//HDMI_TX_PPS (Page 32/35)
#define _HDMITXPPS_CTRL_CFG				(0x00)
#define _HDMITXPPS_START_LB				(0x01)
#define _HDMITXPPS_START_HB				(0x02)
#define _HDMITXPPS_PPS0_PB0				(0x03)	//0x03 ~ 0x21
#define _HDMITXPPS_PPS1_PB0				(0x22)	//0x22 ~ 0x40
#define _HDMITXPPS_PPS2_PB0				(0x41)	//0x41 ~ 0x5F
#define _HDMITXPPS_PPS3_PB0				(0x60)	//0x60 ~ 0x7E
#define _HDMITXPPS_PPS4_PB0				(0x7F)	//0x7F ~ 0x9D
#define _HDMITXPPS_PPS5_PB0				(0x9E)	//0x9E ~ 0xBC
#define _HDMITXPPS_PPS_LOAD				(0xBD)
#define _HDMITXDSCCRC_CHUNKSIZE_L		(0xBE)
#define _HDMITXDSCCRC_CHUNKSIZE_H		(0xBF)
#define _HDMITXPCLKCRC_CTRL_CFG			(0xE0)
#define _HDMITXPCLKCRC_R_L				(0xE2)
#define _HDMITXPCLKCRC_R_H				(0xE3)
#define _HDMITXPCLKCRC_G_L				(0xE4)
#define _HDMITXPCLKCRC_G_H				(0xE5)
#define _HDMITXPCLKCRC_B_L				(0xE6)
#define _HDMITXPCLKCRC_B_H				(0xE7)
#define _HDMITXDSCCRC_CTRL_CFG			(0xE8)
#define _HDMITXDSCCRC_FRAME_CNT			(0xE9) //RO
#define _HDMITXDSCCRC_R_L				(0xEA)
#define _HDMITXDSCCRC_R_H				(0xEB)
#define _HDMITXDSCCRC_G_L				(0xEC)
#define _HDMITXDSCCRC_G_H				(0xED)
#define _HDMITXDSCCRC_B_L				(0xEE)
#define _HDMITXDSCCRC_B_H				(0xEF)

#define _HDMITXTCLKCRC_CTRL_CFG			(0xF0)
#define _HDMITXTCLKCRC_R_L				(0xF2)
#define _HDMITXTCLKCRC_R_H				(0xF3)
#define _HDMITXTCLKCRC_G_L				(0xF4)
#define _HDMITXTCLKCRC_G_H				(0xF5)
#define _HDMITXTCLKCRC_B_L				(0xF6)
#define _HDMITXTCLKCRC_B_H				(0xF7)
#define _HDMITXDSCCRC_VDE_L				(0xFD)
#define _HDMITXDSCCRC_VDE_H				(0xFE)



//HDMI_TX_EMP (Page 33/36)
#define _HDMITXEMP_CTRL_CFG				(0x00)
#define _HDMITXEMP_START_LB				(0x01)
#define _HDMITXEMP_START_HB				(0x02)
#define _HDMITXEMP_EMP0_HB0				(0x03)
#define _HDMITXEMP_EMP0_PB0				(0x06)

#define _HDMITXEMP_EMP_LOAD				(0xBD)
#define _HDMITXEMP_EMP_MASK_LB			(0xBE)
#define _HDMITXEMP_EMP_MASK_HB			(0xBF)

#define _HDMITXEMP_Port_CFG				(0x0F1)
#define _HDMITXEMP_Port_CFG2			(0x0F2)

#define _HDMITX_HS_VS_DLY				(0x0F3)
#define _HDMITX_DATA_DE_DLY				(0x0F4)
#define _HDMITX_CLOCK_DET_CTRL			(0x0F5)
#define _HDMITX_CPU_SCALE				(0x0F6)
#define _HDMITX_PCLK_THRHOLD			(0x0F7)
#define _HDMITX_TCLK_THRHOLD			(0x0F8)
#define _HDMITX_FCLK_THRHOLD			(0x0F9)
#define _HDMITX_CLOCK_DET_OUTPUT_L		(0x0FA)
#define _HDMITX_CLOCK_DET_OUTPUT_H		(0x0FB)
#define _HDMITX_SYS_Ctrl_0 				(0xFC)
#define _HDMITX_SYS_Ctrl_1				(0xFD)
	


//HDMI_TX_FRL (Page 34/37)
#define _HDMITXFRL_LANE_PORT_CFG		(0x00)
#define _HDMITXFRL_SSB_SR_THLD			(0x08)
#define _HDMITXFRL_SSB_CNT				(0x0D)
#define _HDMITXFRL_SR_CNT				(0x0E)
#define _HDMITXFRL_TP_CODE_0_1			(0xE0)
#define _HDMITXFRL_TP_CODE_2_3			(0xE1)
#define _HDMITXFRL_REG_TXFFE_0_1		(0xE2)
#define _HDMITXFRL_REG_TXFFE_2_3		(0xE3)
#define _HDMITXFRL_1Port_DeepColor_ECO	(0x28)
#define _33F2							(0xF2)

#define _HDMITXFRL_RC_UPPDER_LO			(0x14)
#define _HDMITXFRL_RC_UPPDER_HI			(0x15)
#define _HDMITXFRL_RC_LOWER_LO			(0x16)
#define _HDMITXFRL_RC_LOWER_HI			(0x17)
#define _HDMITXFRL_RC_UPPDER_IN_ADV02_LO	(0x1C)
#define _HDMITXFRL_RC_UPPDER_IN_ADV02_HI	(0x1D)
#define _HDMITXFRL_RC_UPPDER_IN_ADV1_LO		(0x1E)
#define _HDMITXFRL_RC_UPPDER_IN_ADV1_HI		(0x1F)
#define _HDMITXFRL_RC_UPPDER_IN_ADV3_LO		(0x20)
#define _HDMITXFRL_RC_UPPDER_IN_ADV3_HI		(0x21)






#endif
