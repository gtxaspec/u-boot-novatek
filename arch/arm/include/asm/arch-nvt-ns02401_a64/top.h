/*
	TOP controller header

	Sets the pinmux of each module.

	@file       top.h
	@ingroup    mIDrvSys_TOP
	@note       Refer NS02302 data sheet for PIN/PAD naming

	Copyright   Novatek Microelectronics Corp. 2023.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_NS02401_TOP_H
#define __ASM_ARCH_NS02401_TOP_H

#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/rcw_macro.h>
#include "nvt-ns02401-pinctrl.h"
#if !defined(__LINUX)
#include <stdio.h>
#endif

typedef u32  PIN_FUNC;                  ///< For top.h usage reference
typedef u32  PINMUX_LCD_SEL;            ///< For panel driver reference

/**
    Debug port select ID

    Debug port select value for pinmux_select_debugport().
*/
typedef enum {
	PINMUX_DEBUGPORT_GPIO =         0x0000,     ///< GPIO
	PINMUX_DEBUGPORT_CKG =          0x0001,     ///< CKGen
	PINMUX_DEBUGPORT_CPU =          0x0002,     ///< CPU
	PINMUX_DEBUGPORT_PAD =          0x0003,     ///< PAD
	PINMUX_DEBUGPORT_TIMER =        0x0004,     ///< Timer
	PINMUX_DEBUGPORT_HRTIMER  =     0x0005,     ///< HR timer
	PINMUX_DEBUGPORT_HRTIMER2 =     0x0006,     ///< HR timer 2
	PINMUX_DEBUGPORT_HRTIMER3 =     0x0007,     ///< HR timer 3

	PINMUX_DEBUGPORT_HRTIMER4 =     0x0008,     ///< HR timer 4
	PINMUX_DEBUGPORT_HRTIMER5 =     0x0009,     ///< HR timer 5
	PINMUX_DEBUGPORT_HRTIMER6 =     0x000A,     ///< HR timer 6
	PINMUX_DEBUGPORT_HRTIMER7 =     0x000B,     ///< HR timer 7
	PINMUX_DEBUGPORT_HRTIMER8 =     0x000C,     ///< HR timer 8
	PINMUX_DEBUGPORT_HRTIMER9 =     0x000D,     ///< HR timer 9
	PINMUX_DEBUGPORT_HRTIMER10 =    0x000E,     ///< HR timer 10
	PINMUX_DEBUGPORT_CC =           0x000F,     ///< CC

	PINMUX_DEBUGPORT_DRTC =         0x0010,     ///< DRTC
	PINMUX_DEBUGPORT_DOORBELL =     0x0011,     ///< Doorbell
	PINMUX_DEBUGPORT_MSI =          0x0012,     ///< MSI
	PINMUX_DEBUGPORT_TRNG =         0x0013,     ///< TRNG
	PINMUX_DEBUGPORT_PWM =          0x0014,     ///< PWM
	PINMUX_DEBUGPORT_RMT =          0x0015,     ///< RMT
	PINMUX_DEBUGPORT_UART =         0x0016,     ///< UART
	PINMUX_DEBUGPORT_UART2 =        0x0017,     ///< UART2

	PINMUX_DEBUGPORT_UART3 =        0x0018,     ///< UART3
	PINMUX_DEBUGPORT_UART4 =        0x0019,     ///< UART4
	PINMUX_DEBUGPORT_UART5 =        0x001A,     ///< UART5
	PINMUX_DEBUGPORT_UART6 =        0x001B,     ///< UART6
	PINMUX_DEBUGPORT_I2C =          0x001C,     ///< I2C
	PINMUX_DEBUGPORT_I2C2 =         0x001D,     ///< I2C2
	PINMUX_DEBUGPORT_I2C3 =         0x001E,     ///< I2C3
	PINMUX_DEBUGPORT_I2C4 =         0x001F,     ///< I2C4

	PINMUX_DEBUGPORT_I2C5 =         0x0020,     ///< I2C5
	PINMUX_DEBUGPORT_I2C6 =         0x0021,     ///< I2C6
	PINMUX_DEBUGPORT_I2C7 =         0x0022,     ///< I2C7
	PINMUX_DEBUGPORT_I2C8 =         0x0023,     ///< I2C8
	PINMUX_DEBUGPORT_SPI =          0x0024,     ///< SPI
	PINMUX_DEBUGPORT_SPI2 =         0x0025,     ///< SPI2
	PINMUX_DEBUGPORT_SPI3 =         0x0026,     ///< SPI3
	PINMUX_DEBUGPORT_SDP =          0x0027,     ///< SDP

	PINMUX_DEBUGPORT_DAI =          0x0028,     ///< DAI
	PINMUX_DEBUGPORT_DAI2 =         0x0029,     ///< DAI2
	PINMUX_DEBUGPORT_DAI3 =         0x002A,     ///< DAI3
	PINMUX_DEBUGPORT_DAI4 =         0x002B,     ///< DAI4
	PINMUX_DEBUGPORT_DAI5 =         0x002C,     ///< DAI5
	PINMUX_DEBUGPORT_GRAPHIC =      0x002D,     ///< GRAPHIC
	PINMUX_DEBUGPORT_GRAPHIC2 =     0x002E,     ///< GRAPHIC2
	PINMUX_DEBUGPORT_GRAPHIC3 =     0x002F,     ///< GRAPHIC3

	PINMUX_DEBUGPORT_GPE =          0x0030,     ///< GPENG
	PINMUX_DEBUGPORT_GPE2 =         0x0031,     ///< GPENG2
	PINMUX_DEBUGPORT_GPE3 =         0x0032,     ///< GPENG3
	PINMUX_DEBUGPORT_AGE =          0x0033,     ///< AGE
	PINMUX_DEBUGPORT_JPG =          0x0034,     ///< JPG
	PINMUX_DEBUGPORT_JPG2 =         0x0035,     ///< JPG2
	PINMUX_DEBUGPORT_MAU =          0x0036,     ///< MAU
	PINMUX_DEBUGPORT_MAU2 =         0x0037,     ///< MAU2

	PINMUX_DEBUGPORT_TZASC =        0x0038,     ///< TZASC
	PINMUX_DEBUGPORT_EFUSE =        0x0039,     ///< EFUSE
	PINMUX_DEBUGPORT_SCE =          0x003A,     ///< SCE
	PINMUX_DEBUGPORT_HASH =         0x003B,     ///< HASH
	PINMUX_DEBUGPORT_RSA =          0x003C,     ///< RSA
	PINMUX_DEBUGPORT_ECDSA =        0x003D,     ///< ECDSA
	PINMUX_DEBUGPORT_HVYLD =        0x003E,     ///< HVYLD
	PINMUX_DEBUGPORT_HVYLD2 =       0x003F,     ///< HVYLD2

	PINMUX_DEBUGPORT_HVYLD3 =       0x0040,     ///< HVYLD3
	PINMUX_DEBUGPORT_HWCP =         0x0041,     ///< HWCP
	PINMUX_DEBUGPORT_NUE2 =        	0x0042,     ///< NUE2
	PINMUX_DEBUGPORT_IVE =          0x0043,     ///< IVE
	PINMUX_DEBUGPORT_MDBC =         0x0044,     ///< MDBC
	PINMUX_DEBUGPORT_ISE =          0x0045,     ///< ISE
	PINMUX_DEBUGPORT_TSEN =         0x0046,     ///< TSEN
	PINMUX_DEBUGPORT_VENC =         0x0047,     ///< VENC

	PINMUX_DEBUGPORT_PCIE =         0x0048,     ///< PCIE
	PINMUX_DEBUGPORT_PCIE2 =        0x0049,     ///< PCIE2
	PINMUX_DEBUGPORT_PCIE3 =        0x004A,     ///< PCIE3
	PINMUX_DEBUGPORT_PCIE4 =        0x004B,     ///< PCIE4
	PINMUX_DEBUGPORT_LCD =          0x004C,     ///< LCD
	PINMUX_DEBUGPORT_LCD2 =         0x004D,     ///< LCD2
	PINMUX_DEBUGPORT_LCD3 =         0x004E,     ///< LCD3
	PINMUX_DEBUGPORT_DSC =          0x004F,     ///< DSC

	PINMUX_DEBUGPORT_HDMI =        	0x0050,     ///< HDMI
	PINMUX_DEBUGPORT_HDMI2 =        0x0051,     ///< HDMI2
	PINMUX_DEBUGPORT_HDMI3 =        0x0052,     ///< HDMI3
	PINMUX_DEBUGPORT_USB2 =         0x0053,     ///< USB2
	PINMUX_DEBUGPORT_USB3 =         0x0054,     ///< USB3
	PINMUX_DEBUGPORT_SATA =         0x0055,     ///< SATA
	PINMUX_DEBUGPORT_VPEL2 =        0x0056,     ///< VPEL2
	PINMUX_DEBUGPORT_XGMAC =        0x0057,     ///< XGMAC

	PINMUX_DEBUGPORT_XGMAC2 =       0x0058,     ///< XGMAC2
	PINMUX_DEBUGPORT_SATA2 =        0x0059,     ///< SATA2
	PINMUX_DEBUGPORT_SATA3 =        0x005A,     ///< SATA3
	PINMUX_DEBUGPORT_SDIO =         0x005B,     ///< SDIO
	PINMUX_DEBUGPORT_SDIO2 =        0x005C,     ///< SDIO2
	PINMUX_DEBUGPORT_SMC =          0x005D,     ///< SMC
	PINMUX_DEBUGPORT_RS =           0x005E,     ///< RS
	PINMUX_DEBUGPORT_RS2 =          0x005F,     ///< RS2

	PINMUX_DEBUGPORT_MALI =         0x0060,     ///< MALI
	PINMUX_DEBUGPORT_JM =           0x0061,     ///< JM
	PINMUX_DEBUGPORT_JMISP =        0x0062,     ///< JMISP
	PINMUX_DEBUGPORT_UTL =          0x0063,     ///< UTL
	PINMUX_DEBUGPORT_CAL =          0x0064,     ///< CAL
	PINMUX_DEBUGPORT_ROU =          0x0065,     ///< ROU
	PINMUX_DEBUGPORT_PPU =          0x0066,     ///< PPU
	PINMUX_DEBUGPORT_LSU =          0x0067,     ///< LSU

	PINMUX_DEBUGPORT_ROU2 =         0x0068,     ///< ROU2
	PINMUX_DEBUGPORT_CONV =         0x0069,     ///< CONV
	PINMUX_DEBUGPORT_VPE =          0x006A,     ///< VPE
	PINMUX_DEBUGPORT_USB2_2 =       0x006B,     ///< USB2_2
	PINMUX_DEBUGPORT_USB3_2 =       0x006C,     ///< USB3_2
	PINMUX_DEBUGPORT_SATA4 =        0x006D,     ///< SATA4
	PINMUX_DEBUGPORT_SATA5 =        0x006E,     ///< SATA5
	PINMUX_DEBUGPORT_SATA6 =        0x006F,     ///< SATA6

	PINMUX_DEBUGPORT_ADM =          0x0070,     ///< ADM
	PINMUX_DEBUGPORT_RTC =          0x0071,     ///< RTC
	PINMUX_DEBUGPORT_VPEL =         0x0072,     ///< VPEL
	PINMUX_DEBUGPORT_VDEC =         0x0073,     ///< VDEC
	PINMUX_DEBUGPORT_VDEC2 =        0x0074,     ///< VDEC2
	PINMUX_DEBUGPORT_VDEC3 =        0x0075,     ///< VDEC3
	PINMUX_DEBUGPORT_RS_2 =         0x0076,     ///< RS_2
	PINMUX_DEBUGPORT_RS_3 =         0x0077,     ///< RS_3

	PINMUX_DEBUGPORT_LSU2 =         0x0078,     ///< LSU2
	PINMUX_DEBUGPORT_WDT =          0x0079,     ///< WDT
	PINMUX_DEBUGPORT_EFUSE2 =       0x007A,     ///< EFUSE2
	PINMUX_DEBUGPORT_DEI =          0x007B,     ///< DEI

	PINMUX_DEBUGPORT_GROUP_NONE =   0x0000,     ///< No debug port is output
	PINMUX_DEBUGPORT_GROUP1 =       0x0100,     ///< Output debug port to B_GPIO[0..16], D_GPIO[6..7]
	PINMUX_DEBUGPORT_GROUP2 =       0x0200,     ///< Output debug port to E_GPIO[0..15], P_GPIO[22..24]

	ENUM_DUMMY4WORD(PINMUX_DEBUGPORT)
} PINMUX_DEBUGPORT;

/**
    PIN config of LCD modes

    @note For pinmux_init() with PIN_FUNC_LCD or PIN_FUNC_LCD2.\n
    For example, you can use {PIN_FUNC_LCD, PINMUX_DISPMUX_SEL_LCD | PINMUX_LCDMODE_RGB_SERIAL}
    to tell display object that PIN_FUNC_LCD is located on primary LCD pinmux,
    and it's LCD mode is RGB serial.
*/
typedef enum {
	PINMUX_LCDMODE_RGB_SERIAL           =    0,     ///< LCD MODE is RGB Serial or UPS051
	PINMUX_LCDMODE_RGB_PARALL           =    1,     ///< LCD MODE is RGB Parallel (888)
	PINMUX_LCDMODE_YUV640               =    2,     ///< LCD MODE is YUV640
	PINMUX_LCDMODE_YUV720               =    3,     ///< LCD MODE is YUV720
	PINMUX_LCDMODE_RGBD360              =    4,     ///< LCD MODE is RGB Dummy 360
	PINMUX_LCDMODE_RGBD320              =    5,     ///< LCD MODE is RGB Dummy 320
	PINMUX_LCDMODE_RGB_THROUGH          =    6,     ///< LCD MODE is RGB through mode
	PINMUX_LCDMODE_CCIR601              =    7,     ///< LCD MODE is CCIR601
	PINMUX_LCDMODE_CCIR656              =    8,     ///< LCD MODE is CCIR656
	PINMUX_LCDMODE_RGB_PARALL666        =    9,     ///< LCD MODE is RGB Parallel 666
	PINMUX_LCDMODE_RGB_PARALL565        =   10,     ///< LCD MODE is RGB Parallel 565
	PINMUX_LCDMODE_RGB_PARALL_DELTA     =   11,     ///< LCD MODE is RGB Parallel Delta
	PINMUX_LCDMODE_MIPI                 =   12,     ///< LCD MODE is MIPI Display

	PINMUX_LCDMODE_MI_OFFSET            = 32,       ///< Memory LCD MODE offset
	PINMUX_LCDMODE_MI_FMT0              = 32 + 0,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	PINMUX_LCDMODE_MI_FMT1              = 32 + 1,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	PINMUX_LCDMODE_MI_FMT2              = 32 + 2,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	PINMUX_LCDMODE_MI_FMT3              = 32 + 3,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	PINMUX_LCDMODE_MI_FMT4              = 32 + 4,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	PINMUX_LCDMODE_MI_FMT5              = 32 + 5,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	PINMUX_LCDMODE_MI_FMT6              = 32 + 6,   ///< LCD MODE is Memory(Parallel Interface) 8bits
	PINMUX_LCDMODE_MI_FMT7              = 32 + 7,   ///< LCD MODE is Memory(Parallel Interface) 9bits
	PINMUX_LCDMODE_MI_FMT8              = 32 + 8,   ///< LCD MODE is Memory(Parallel Interface) 16bits
	PINMUX_LCDMODE_MI_FMT9              = 32 + 9,   ///< LCD MODE is Memory(Parallel Interface) 16bits
	PINMUX_LCDMODE_MI_FMT10             = 32 + 10,  ///< LCD MODE is Memory(Parallel Interface) 18bits
	PINMUX_LCDMODE_MI_FMT11             = 32 + 11,  ///< LCD MODE is Memory(Parallel Interface)  8bits
	PINMUX_LCDMODE_MI_FMT12             = 32 + 12,  ///< LCD MODE is Memory(Parallel Interface) 16bits
	PINMUX_LCDMODE_MI_FMT13             = 32 + 13,  ///< LCD MODE is Memory(Parallel Interface) 16bits
	PINMUX_LCDMODE_MI_FMT14             = 32 + 14,  ///< LCD MODE is Memory(Parallel Interface) 16bits
	PINMUX_LCDMODE_MI_SERIAL_BI         = 32 + 20,  ///< LCD MODE is Serial Interface bi-direction
	PINMUX_LCDMODE_MI_SERIAL_SEP        = 32 + 21,  ///< LCD MODE is Serial Interface separation

	PINMUX_LCDMODE_AUTO_PINMUX          = 0x01 << 22, ///< Set display device to GPIO mode when display device is closed. Select this filed will inform display object to switch to GPIO when display device is closed.


	PINMUX_HDMIMODE_OFFSET              = 64,       ///< HDMI MODE offset (not support HDMI. Below is backward compatible)
	PINMUX_HDMIMODE_640X480P60          = 64 + 1,   ///< HDMI Video format is 640x480 & Progressive 60fps
	PINMUX_HDMIMODE_720X480P60          = 64 + 2,   ///< HDMI Video format is 720x480 & Progressive 60fps & 4:3
	PINMUX_HDMIMODE_720X480P60_16X9     = 64 + 3,   ///< HDMI Video format is 720x480 & Progressive 60fps & 16:9
	PINMUX_HDMIMODE_1280X720P60         = 64 + 4,   ///< HDMI Video format is 1280x720 & Progressive 60fps
	PINMUX_HDMIMODE_1920X1080I60        = 64 + 5,   ///< HDMI Video format is 1920x1080 & Interlaced 60fps
	PINMUX_HDMIMODE_720X480I60          = 64 + 6,   ///< HDMI Video format is 720x480 & Interlaced 60fps
	PINMUX_HDMIMODE_720X480I60_16X9     = 64 + 7,   ///< HDMI Video format is 720x480 & Interlaced 60fps & 16:9
	PINMUX_HDMIMODE_720X240P60          = 64 + 8,   ///< HDMI Video format is 720x240 & Progressive 60fps
	PINMUX_HDMIMODE_720X240P60_16X9     = 64 + 9,   ///< HDMI Video format is 720x240 & Progressive 60fps & 16:9
	PINMUX_HDMIMODE_1440X480I60         = 64 + 10,  ///< HDMI Video format is 1440x480 & Interlaced 60fps
	PINMUX_HDMIMODE_1440X480I60_16X9    = 64 + 11,  ///< HDMI Video format is 1440x480 & Interlaced 60fps & 16:9
	PINMUX_HDMIMODE_1440X240P60         = 64 + 12,  ///< HDMI Video format is 1440x240 & Progressive 60fps
	PINMUX_HDMIMODE_1440X240P60_16X9    = 64 + 13,  ///< HDMI Video format is 1440x240 & Progressive 60fps & 16:9
	PINMUX_HDMIMODE_1440X480P60         = 64 + 14,  ///< HDMI Video format is 1440x480 & Progressive 60fps
	PINMUX_HDMIMODE_1440X480P60_16X9    = 64 + 15,  ///< HDMI Video format is 1440x480 & Progressive 60fps & 16:9
	PINMUX_HDMIMODE_720X576P50          = 64 + 17,  ///< HDMI Video format is 720x576 & Progressive 50fps
	PINMUX_HDMIMODE_720X576P50_16X9     = 64 + 18,  ///< HDMI Video format is 720x576 & Progressive 50fps & 16:9
	PINMUX_HDMIMODE_1280X720P50         = 64 + 19,  ///< HDMI Video format is 1280x720 & Progressive 50fps
	PINMUX_HDMIMODE_1920X1080I50        = 64 + 20,  ///< HDMI Video format is 1920x1080 & Interlaced 50fps
	PINMUX_HDMIMODE_720X576I50          = 64 + 21,  ///< HDMI Video format is 720x576 & Interlaced 50fps
	PINMUX_HDMIMODE_720X576I50_16X9     = 64 + 22,  ///< HDMI Video format is 720x576 & Interlaced 50fps & 16:9
	PINMUX_HDMIMODE_720X288P50          = 64 + 23,  ///< HDMI Video format is 720x288 & Progressive 50fps
	PINMUX_HDMIMODE_720X288P50_16X9     = 64 + 24,  ///< HDMI Video format is 720x288 & Progressive 50fps & 16:9
	PINMUX_HDMIMODE_1440X576I50         = 64 + 25,  ///< HDMI Video format is 1440x576 & Interlaced 50fps
	PINMUX_HDMIMODE_1440X576I50_16X9    = 64 + 26,  ///< HDMI Video format is 1440x576 & Interlaced 50fps & 16:9
	PINMUX_HDMIMODE_1440X288P50         = 64 + 27,  ///< HDMI Video format is 1440x288 & Progressive 50fps
	PINMUX_HDMIMODE_1440X288P50_16X9    = 64 + 28,  ///< HDMI Video format is 1440x288 & Progressive 50fps & 16:9
	PINMUX_HDMIMODE_1440X576P50         = 64 + 29,  ///< HDMI Video format is 1440x576 & Progressive 50fps
	PINMUX_HDMIMODE_1440X576P50_16X9    = 64 + 30,  ///< HDMI Video format is 1440x576 & Progressive 50fps & 16:9
	PINMUX_HDMIMODE_1920X1080P50        = 64 + 31,  ///< HDMI Video format is 1920x1080 & Progressive 50fps
	PINMUX_HDMIMODE_1920X1080P24        = 64 + 32,  ///< HDMI Video format is 1920x1080 & Progressive 24fps
	PINMUX_HDMIMODE_1920X1080P25        = 64 + 33,  ///< HDMI Video format is 1920x1080 & Progressive 25fps
	PINMUX_HDMIMODE_1920X1080P30        = 64 + 34,  ///< HDMI Video format is 1920x1080 & Progressive 30fps
	PINMUX_HDMIMODE_1920X1080I50_VT1250 = 64 + 39,  ///< HDMI Video format is 1920x1080 & Interlaced  50fps & V-total is 1250 lines
	PINMUX_HDMIMODE_1920X1080I100       = 64 + 40,  ///< HDMI Video format is 1920x1080 & Interlaced  100fps
	PINMUX_HDMIMODE_1280X720P100        = 64 + 41,  ///< HDMI Video format is 1280X720  & Progressive 100fps
	PINMUX_HDMIMODE_720X576P100         = 64 + 42,  ///< HDMI Video format is 720X576   & Progressive 100fps
	PINMUX_HDMIMODE_720X576P100_16X9    = 64 + 43,  ///< HDMI Video format is 720X576   & Progressive 100fps & 16:9
	PINMUX_HDMIMODE_720X576I100         = 64 + 44,  ///< HDMI Video format is 720X576  & Interlaced 100fps
	PINMUX_HDMIMODE_720X576I100_16X9    = 64 + 45,  ///< HDMI Video format is 720X576  & Interlaced 100fps & 16:9
	PINMUX_HDMIMODE_1920X1080I120       = 64 + 46,  ///< HDMI Video format is 1920X1080 & Interlaced 120fps
	PINMUX_HDMIMODE_1280X720P120        = 64 + 47,  ///< HDMI Video format is 1280X720  & Progressive 120fps
	PINMUX_HDMIMODE_720X480P120         = 64 + 48,  ///< HDMI Video format is 720X480   & Progressive 120fps
	PINMUX_HDMIMODE_720X480P120_16X9    = 64 + 49,  ///< HDMI Video format is 720X480   & Progressive 120fps & 16:9
	PINMUX_HDMIMODE_720X480I120         = 64 + 50,  ///< HDMI Video format is 720X480  & Interlaced 120fps
	PINMUX_HDMIMODE_720X480I120_16X9    = 64 + 51,  ///< HDMI Video format is 720X480  & Interlaced 120fps & 16:9
	PINMUX_HDMIMODE_720X576P200         = 64 + 52,  ///< HDMI Video format is 720X576  & Progressive 200fps
	PINMUX_HDMIMODE_720X576P200_16X9    = 64 + 53,  ///< HDMI Video format is 720X576  & Progressive 200fps & 16:9
	PINMUX_HDMIMODE_720X576I200         = 64 + 54,  ///< HDMI Video format is 720X576  & Interlaced 200fps
	PINMUX_HDMIMODE_720X576I200_16X9    = 64 + 55,  ///< HDMI Video format is 720X576  & Interlaced 200fps & 16:9
	PINMUX_HDMIMODE_720X480P240         = 64 + 56,  ///< HDMI Video format is 720X480  & Progressive 240fps
	PINMUX_HDMIMODE_720X480P240_16X9    = 64 + 57,  ///< HDMI Video format is 720X480  & Progressive 240fps & 16:9
	PINMUX_HDMIMODE_720X480I240         = 64 + 58,  ///< HDMI Video format is 720X480  & Interlaced 240fps
	PINMUX_HDMIMODE_720X480I240_16X9    = 64 + 59,  ///< HDMI Video format is 720X480  & Interlaced 240fps & 16:9

	PINMUX_DSI_1_LANE_CMD_MODE_RGB565           = 128 + 0, ///< DSI command mode with RGB565 format   (not support DSI. Below is backward compatible)
	PINMUX_DSI_1_LANE_CMD_MODE_RGB666P          = 128 + 1, ///< DSI command mode with RGB666 packed
	PINMUX_DSI_1_LANE_CMD_MODE_RGB666L          = 128 + 2, ///< DSI command mode with RGB666 loosely
	PINMUX_DSI_1_LANE_CMD_MODE_RGB888           = 128 + 3, ///< DSI command mode with RGB888

	PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB565     = 128 + 4, ///< DSI video sync pulse mode with RGB565 format
	PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666P    = 128 + 5, ///< DSI video sync pulse mode with RGB666 packed
	PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666L    = 128 + 6, ///< DSI video sync pulse mode with RGB666 loosely
	PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB888     = 128 + 7, ///< DSI video sync pulse mode with RGB888

	PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB565     = 128 + 8, ///< DSI video sync event burst mode with RGB565 format
	PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666P    = 128 + 9, ///< DSI video sync event burst mode with RGB666 packed
	PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666L    = 128 + 10, ///< DSI video sync event burst mode with RGB666 loosely
	PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB888     = 128 + 11, ///< DSI video sync event burst mode with RGB888

	PINMUX_DSI_2_LANE_CMD_MODE_RGB565           = 128 + 12, ///< DSI command mode with RGB565 format
	PINMUX_DSI_2_LANE_CMD_MODE_RGB666P          = 128 + 13, ///< DSI command mode with RGB666 packed
	PINMUX_DSI_2_LANE_CMD_MODE_RGB666L          = 128 + 14, ///< DSI command mode with RGB666 loosely
	PINMUX_DSI_2_LANE_CMD_MODE_RGB888           = 128 + 15, ///< DSI command mode with RGB888

	PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB565     = 128 + 16, ///< DSI video sync pulse mode with RGB565 format
	PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB666P    = 128 + 17, ///< DSI video sync pulse mode with RGB666 packed
	PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB666L    = 128 + 18, ///< DSI video sync pulse mode with RGB666 loosely
	PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB888     = 128 + 19, ///< DSI video sync pulse mode with RGB888

	PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB565     = 128 + 20, ///< DSI video sync event burst mode with RGB565 format
	PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB666P    = 128 + 21, ///< DSI video sync event burst mode with RGB666 packed
	PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB666L    = 128 + 22, ///< DSI video sync event burst mode with RGB666 loosely
	PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB888     = 128 + 23, ///< DSI video sync event burst mode with RGB888

	PINMUX_DSI_4_LANE_CMD_MODE_RGB565           = 128 + 24, ///< DSI command mode with RGB565 format
	PINMUX_DSI_4_LANE_CMD_MODE_RGB666P          = 128 + 25, ///< DSI command mode with RGB666 packed
	PINMUX_DSI_4_LANE_CMD_MODE_RGB666L          = 128 + 26, ///< DSI command mode with RGB666 loosely
	PINMUX_DSI_4_LANE_CMD_MODE_RGB888           = 128 + 27, ///< DSI command mode with RGB888

	PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB565     = 128 + 28, ///< DSI video sync pulse mode with RGB565 format
	PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB666P    = 128 + 29, ///< DSI video sync pulse mode with RGB666 packed
	PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB666L    = 128 + 30, ///< DSI video sync pulse mode with RGB666 loosely
	PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB888     = 128 + 31, ///< DSI video sync pulse mode with RGB888

	PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB565     = 128 + 32, ///< DSI video sync event burst mode with RGB565 format
	PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB666P    = 128 + 33, ///< DSI video sync event burst mode with RGB666 packed
	PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB666L    = 128 + 34, ///< DSI video sync event burst mode with RGB666 loosely
	PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB888     = 128 + 35, ///< DSI video sync event burst mode with RGB888



	ENUM_DUMMY4WORD(PINMUX_LCDINIT)
} PINMUX_LCDINIT;

/**
    PIN config for TV/HDMI

    @note For pinmux_init() for PIN_FUNC_TV or PIN_FUNC_HDMI.
    For example, you can use {PIN_FUNC_HDMI, PINMUX_TV_HDMI_CFG_NORMAL|PINMUX_HDMIMODE_1280X720P60}
    to tell display object that HDMI activation will disable PANEL,
    and HDMI mode is 1280x720 P60.
*/
typedef enum {
	PINMUX_TV_HDMI_CFG_GPIO = 0x00,                 ///< TV activation will disable PINMUX to GPIO
	PINMUX_TV_HDMI_CFG_NORMAL = 0x00,               ///< TV activation will disable PANEL which shared the same IDE
	PINMUX_TV_HDMI_CFG_PINMUX_ON = 0x01 << 28,      ///< TV activation will keep PINMUX setting

	PINMUX_TV_HDMI_CFG_MASK = 0x03 << 28,
	ENUM_DUMMY4WORD(PINMUX_TV_HDMI_CFG)
} PINMUX_TV_HDMI_CFG;

/**
    PIN config for HDMI, not support (Backward compatible)

    @note For pinmux_init() for PIN_FUNC_HDMI.\n
            For example, you can use {PIN_FUNC_HDMI, PINMUX_HDMI_CFG_CEC|PINMUX_TV_HDMI_CFG_NORMAL}\n
            to declare HDMI CEC pinmux is enabled.
*/
typedef enum {
	PINMUX_HDMI_CFG_GPIO = 0x00,                    ///< HDMI specific PIN to GPIO
	PINMUX_HDMI_CFG_HOTPLUG = 0x01 << 26,           ///< HDMI HOTPLUG.
	PINMUX_HDMI_CFG_CEC = 0x02 << 26,               ///< HDMI CEC.

	PINMUX_HDMI_CFG_MASK = 0x03 << 26,
	ENUM_DUMMY4WORD(PINMUX_HDMI_CFG)
} PINMUX_HDMI_CFG;

/**
    PIN config for USB

    @note For pinmux_init() for PIN_FUNC_USB.\n
            For example, you can use {PIN_FUNC_USB, PINMUX_USB_CFG_DEVICE}\n
            to select USB as device.
*/
typedef enum {
	PINMUX_USB_CFG_NONE = 0x00,                     ///< USB as device
	PINMUX_USB_CFG_DEVICE = 0x00,                   ///< USB as device
	PINMUX_USB_CFG_HOST = 0x01,                     ///< USB as host

	ENUM_DUMMY4WORD(PINMUX_USB_CFG)
} PINMUX_USB_CFG;

/**
    Pinmux power domain isolated function enable/disable

    @note For pinmux_set_direct_channel()
*/
typedef enum {
	PINMUX_ISOFUNC_DISABLE = 0x00,               ///< DISABLE power domain isolated function
	PINMUX_ISOFUNC_ENABLE = 0x01,                ///< ENABLE  power domain isolated function
	ENUM_DUMMY4WORD(PINMUX_POWER_ISOFUNC)
} PINMUX_POWER_ISOFUNC;

/**
    Pinmux Function identifier

    @note For pinmux_getDispMode(), pinmux_setPinmux().
*/
typedef enum {
	PINMUX_FUNC_ID_LCD,                             ///< 1st Panel (LCD), pinmux can be:
	///< - @b PINMUX_LCD_SEL_GPIO
	///< - @b PINMUX_LCD_SEL_CCIR656
	///< - @b PINMUX_LCD_SEL_CCIR656_16BITS
	///< - @b PINMUX_LCD_SEL_CCIR601
	///< - @b PINMUX_LCD_SEL_CCIR601_16BITS
	///< - @b PINMUX_LCD_SEL_PARALLE_RGB565
	///< - @b PINMUX_LCD_SEL_PARALLE_RGB666
	///< - @b PINMUX_LCD_SEL_PARALLE_RGB888
	///< ORed with
	///< - @b PINMUX_LCD_SEL_DE_ENABLE
	///< - @b PINMUX_LCD_SEL_FIELD
	///< - @b PINMUX_LCD_SEL_NO_HVSYNC
	PINMUX_FUNC_ID_LCD2,                            ///< LCD2 not support.
	PINMUX_FUNC_ID_TV,                              ///< TV, pinmux can be:
	///< - @b PINMUX_LCD_SEL_GPIO
	PINMUX_FUNC_ID_HDMI,                            ///< HDMI, pinmux can be:
	///< - @b PINMUX_LCD_SEL_GPIO
	PINMUX_FUNC_ID_COUNT,                           //< Total function count

	ENUM_DUMMY4WORD(PINMUX_FUNC_ID)
} PINMUX_FUNC_ID;

/**
    Pinmux selection for Storage

    @note For PINMUX_FUNC_ID_SDIO, PINMUX_FUNC_ID_SPI, PINMUX_FUNC_ID_NAND
*/
typedef enum {
	PINMUX_STORAGE_SEL_INACTIVE,                    ///< Inactive storage
	PINMUX_STORAGE_SEL_ACTIVE,                      ///< Active storage
	PINMUX_STORAGE_SEL_INEXIST,                     ///< Inexist storage
	PINMUX_STORAGE_SEL_EXIST,                       ///< Exist storage
	ENUM_DUMMY4WORD(PINMUX_STORAGE_SEL)
} PINMUX_STORAGE_SEL;

/*
    Pinmux Function identifier for driver only

    @note For pinmux_setPinmux().
*/
typedef enum {
	PINMUX_FUNC_ID_SDIO = 0x8000000,                ///< SDIO, pinmux can be:
	///< - @b PINMUX_STORAGE_SEL_INACTIVE
	///< - @b PINMUX_STORAGE_SEL_ACTIVE
	PINMUX_FUNC_ID_SDIO2,                           ///< SDIO2, pinmux can be:
	///< - @b PINMUX_STORAGE_SEL_INACTIVE
	///< - @b PINMUX_STORAGE_SEL_ACTIVE
	PINMUX_FUNC_ID_SDIO3,                           ///< SDIO3, pinmux can be:
	///< - @b PINMUX_STORAGE_SEL_INACTIVE
	///< - @b PINMUX_STORAGE_SEL_ACTIVE
	PINMUX_FUNC_ID_SPI,                             ///< SPI, pinmux can be:
	///< - @b PINMUX_STORAGE_SEL_INACTIVE
	///< - @b PINMUX_STORAGE_SEL_ACTIVE
	PINMUX_FUNC_ID_NAND,                            ///< NAND, pinmux can be:
	///< - @b PINMUX_STORAGE_SEL_INACTIVE
	///< - @b PINMUX_STORAGE_SEL_ACTIVE
	PINMUX_FUNC_ID_BMC,                             ///< BMC, pinmux can be:
	///< - @b PINMUX_STORAGE_SEL_INACTIVE
	///< - @b PINMUX_STORAGE_SEL_ACTIVE
	PINMUX_FUNC_ID_USB_VBUSI,                       ///< USB VBUSI, pinmux can be:
	///< - @b PINMUX_USB_SEL_INACTIVE
	///< - @b PINMUX_USB_SEL_ACTIVE
	PINMUX_FUNC_ID_USB2_VBUSI,                      ///< USB2 VBUSI, pinmux can be:
	///< - @b PINMUX_USB_SEL_INACTIVE
	///< - @b PINMUX_USB_SEL_ACTIVE
	PINMUX_FUNC_ID_I2C1_1ST,                        ///< I2C channel1 1st pinmux, pinmux can be:
	///< - @b PINMUX_I2C_SEL_INACTIVE
	///< - @b PINMUX_I2C_SEL_ACTIVE
	PINMUX_FUNC_ID_I2C1_2ND,                        ///< I2C channel1 2nd pinmux, pinmux can be:
	///< - @b PINMUX_I2C_SEL_INACTIVE
	///< - @b PINMUX_I2C_SEL_ACTIVE
	PINMUX_FUNC_ID_I2C2_1ST,                        ///< I2C channel2 1st pinmux, pinmux can be:
	///< - @b PINMUX_I2C_SEL_INACTIVE
	///< - @b PINMUX_I2C_SEL_ACTIVE
	PINMUX_FUNC_ID_I2C2_2ND,                        ///< I2C channel2 2nd pinmux, pinmux can be:
	///< - @b PINMUX_I2C_SEL_INACTIVE
	///< - @b PINMUX_I2C_SEL_ACTIVE
	PINMUX_FUNC_ID_I2C3_1ST,                        ///< I2C channel3 1st pinmux, pinmux can be:
	///< - @b PINMUX_I2C_SEL_INACTIVE
	///< - @b PINMUX_I2C_SEL_ACTIVE
	PINMUX_FUNC_ID_I2C3_2ND,                        ///< I2C channel3 2nd pinmux, pinmux can be:
	///< - @b PINMUX_I2C_SEL_INACTIVE
	///< - @b PINMUX_I2C_SEL_ACTIVE
	PINMUX_FUNC_ID_I2C3_3RD,                        ///< I2C channel3 3rd pinmux, pinmux can be:
	///< - @b PINMUX_I2C_SEL_INACTIVE
	///< - @b PINMUX_I2C_SEL_ACTIVE
	PINMUX_FUNC_ID_I2C4_1ST,                        ///< I2C channel4 1st pinmux, pinmux can be:
	///< - @b PINMUX_I2C_SEL_INACTIVE
	///< - @b PINMUX_I2C_SEL_ACTIVE
	PINMUX_FUNC_ID_I2C4_2ND,                        ///< I2C channel4 2nd pinmux, pinmux can be:
	///< - @b PINMUX_I2C_SEL_INACTIVE
	///< - @b PINMUX_I2C_SEL_ACTIVE
	PINMUX_FUNC_ID_I2C4_3RD,                        ///< I2C channel4 3rd pinmux, pinmux can be:
	///< - @b PINMUX_I2C_SEL_INACTIVE
	///< - @b PINMUX_I2C_SEL_ACTIVE
	PINMUX_FUNC_ID_I2C4_4TH,                        ///< I2C channel4 4th pinmux, pinmux can be:
	///< - @b PINMUX_I2C_SEL_INACTIVE
	///< - @b PINMUX_I2C_SEL_ACTIVE
	PINMUX_FUNC_ID_I2C5_1ST,                        ///< I2C channel5 1st pinmux, pinmux can be:
	///< - @b PINMUX_I2C_SEL_INACTIVE
	///< - @b PINMUX_I2C_SEL_ACTIVE
	PINMUX_FUNC_ID_I2C5_2ND,                        ///< I2C channel5 2nd pinmux, pinmux can be:
	///< - @b PINMUX_I2C_SEL_INACTIVE
	///< - @b PINMUX_I2C_SEL_ACTIVE
	PINMUX_FUNC_ID_SN_VDHD,                         ///< SN VD/HD, pinmux can be:
	///< - @b PINMUX_SENSOR_SEL_INACTIVE
	///< - @b PINMUX_SENSOR_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_0,                           ///< PWM channel 0
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_1,                           ///< PWM channel 1
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_2,                           ///< PWM channel 2
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_3,                           ///< PWM channel 3
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_4,                           ///< PWM channel 4
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_5,                           ///< PWM channel 5
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_6,                           ///< PWM channel 6
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_7,                           ///< PWM channel 7
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_8,                           ///< PWM channel 8
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_9,                           ///< PWM channel 9
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_10,                          ///< PWM channel 10
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_11,                          ///< PWM channel 11
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_12,                          ///< PWM channel 12
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_13,                          ///< PWM channel 13
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_14,                          ///< PWM channel 14
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_15,                          ///< PWM channel 15
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_16,                          ///< PWM channel 16
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_17,                          ///< PWM channel 17
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_18,                          ///< PWM channel 18
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_PWM_19,                          ///< PWM channel 19
	///< - @b PINMUX_PWM_SEL_INACTIVE
	///< - @b PINMUX_PWM_SEL_ACTIVE
	PINMUX_FUNC_ID_SIF_0,                           ///< SIF channel 0
	///< - @b PINMUX_SIF_SEL_INACTIVE
	///< - @b PINMUX_SIF_SEL_ACTIVE
	PINMUX_FUNC_ID_SIF_1,                           ///< SIF channel 1
	///< - @b PINMUX_SIF_SEL_INACTIVE
	///< - @b PINMUX_SIF_SEL_ACTIVE
	PINMUX_FUNC_ID_SIF_2,                           ///< SIF channel 2
	///< - @b PINMUX_SIF_SEL_INACTIVE
	///< - @b PINMUX_SIF_SEL_ACTIVE
	PINMUX_FUNC_ID_SIF_3,                           ///< SIF channel 3
	///< - @b PINMUX_SIF_SEL_INACTIVE
	///< - @b PINMUX_SIF_SEL_ACTIVE
	PINMUX_FUNC_ID_SIF_4,                           ///< SIF channel 4
	///< - @b PINMUX_SIF_SEL_INACTIVE
	///< - @b PINMUX_SIF_SEL_ACTIVE
	PINMUX_FUNC_ID_SIF_5,                           ///< SIF channel 5
	///< - @b PINMUX_SIF_SEL_INACTIVE
	///< - @b PINMUX_SIF_SEL_ACTIVE
	PINMUX_FUNC_ID_SIF_6,                           ///< SIF channel 6
	///< - @b PINMUX_SIF_SEL_INACTIVE
	///< - @b PINMUX_SIF_SEL_ACTIVE
	PINMUX_FUNC_ID_SIF_7,                           ///< SIF channel 7
	///< - @b PINMUX_SIF_SEL_INACTIVE
	///< - @b PINMUX_SIF_SEL_ACTIVE
	PINMUX_FUNC_ID_UART_1,                          ///< UART channel 1
	///< - @b PINMUX_UART_SEL_INACTIVE
	///< - @b PINMUX_UART_SEL_ACTIVE
	PINMUX_FUNC_ID_UART_2,                          ///< UART channel 2
	///< - @b PINMUX_UART_SEL_INACTIVE
	///< - @b PINMUX_UART_SEL_ACTIVE
	PINMUX_FUNC_ID_UART_3,                          ///< UART channel 3
	///< - @b PINMUX_UART_SEL_INACTIVE
	///< - @b PINMUX_UART_SEL_ACTIVE
	PINMUX_FUNC_ID_UART_4,                          ///< UART channel 4
	///< - @b PINMUX_UART_SEL_INACTIVE
	///< - @b PINMUX_UART_SEL_ACTIVE
	PINMUX_FUNC_ID_I2S,                             ///< I2S
	///< - @b PINMUX_I2S_SEL_INACTIVE
	///< - @b PINMUX_I2S_SEL_ACTIVE
	PINMUX_FUNC_ID_I2S_MCLK,                        ///< I2S MCLK
	///< - @b PINMUX_I2S_SEL_INACTIVE
	///< - @b PINMUX_I2S_SEL_ACTIVE
	PINMUX_FUNC_ID_DSP_PERI_CH_EN,                  ///< DSP PERI channel enable
	///< - @b PINMUX_DSP_CH_INACTIVE
	///< - @b PINMUX_DSP_CH_ACTIVE
	PINMUX_FUNC_ID_DSP_IOP_CH_EN,                   ///< DSP IOP channel enable
	///< - @b PINMUX_DSP_CH_INACTIVE
	///< - @b PINMUX_DSP_CH_ACTIVE
	PINMUX_FUNC_ID_DSP2_PERI_CH_EN,                 ///< DSP2 PERI channel enable
	///< - @b PINMUX_DSP_CH_INACTIVE
	///< - @b PINMUX_DSP_CH_ACTIVE
	PINMUX_FUNC_ID_DSP2_IOP_CH_EN,                  ///< DSP2 IOP channel enable
	///< - @b PINMUX_DSP_CH_INACTIVE
	///< - @b PINMUX_DSP_CH_ACTIVE

	ENUM_DUMMY4WORD(PINMUX_FUNC_ID_DRV)
} PINMUX_FUNC_ID_DRV;

/**
    Pinmux selection for boot source

    @note For BOOT_SRC_ENUM
*/
typedef enum
{
	BOOT_SRC_SPINOR,                    //< Boot from SPI NOR
	BOOT_SRC_CARD,                      //< Boot from CARD
	BOOT_SRC_SPINAND,                   //< Boot from SPI NAND with on die ECC (2K page)
	BOOT_SRC_SPINAND_RS,                //< Boot from SPI NAND (Reed Solomon ECC) (2K page)
	BOOT_SRC_ETH,                       //< Boot from Ethernet
	BOOT_SRC_USB_HS,                    //< Boot from USB (High Speed)
	BOOT_SRC_SPINAND_4K,                //< Boot from SPI NAND with on die ECC (4K page)
	BOOT_SRC_RESERVED,                  //< RESERVED
	BOOT_SRC_EMMC_4BIT,                 //< Boot from eMMC 4-bit
	BOOT_SRC_EMMC_8BIT,                 //< Boot from eMMC 8-bit
	BOOT_SRC_SPINAND_RS_4K,             //< Boot from SPI NAND (Reed Solomon ECC) (4K page)
	BOOT_SRC_USB_FS,                    //< Boot from USB (Full Speed)
	ENUM_DUMMY4WORD(BOOT_SRC_ENUM)
} BOOT_SRC_ENUM;


/**
    Pinmux group

    @note For pinmux_init()
*/
typedef struct {
	PIN_FUNC    pin_function;           ///< PIN Function group
	UINT32      config;                 ///< Configuration for pinFunction
} PIN_GROUP_CONFIG;

/*
enum CHIP_ID {
	CHIP_NA51055 = 0x4821,
	CHIP_NA51084 = 0x5021,
	CHIP_NA51089 = 0x7021,
	CHIP_NA51090 = 0xBC21,
	CHIP_NA51102 = 0x5221,
	CHIP_NA51103 = 0x8B20,
	CHIP_NS02201 = 0xF221,
	CHIP_NS02301 = 0x7721,
	CHIP_NS02302 = 0x5A21,
	CHIP_NS02401 = 0xBB21,
	CHIP_NS02402 = 0x5B21,
};

enum CHIP_VER {
	CHIPVER_A,
	CHIPVER_B
};
*/
int nvt_pinmux_capture(PIN_GROUP_CONFIG *pinmux_config, int count);
int nvt_pinmux_update(PIN_GROUP_CONFIG *pinmux_config, int count);
int pinmux_set_config(PINMUX_FUNC_ID id, u32 pinmux);
int nvt_pinmux_probe(void);

/*
#ifdef CONFIG_PINCTRL_NS02401
UINT32 nvt_get_chip_id(void);
#else
static unsigned int inline nvt_get_chip_id(void) {
	return CHIP_NS02401;
}
#endif
*/
UINT32 nvt_get_chip_ver(void);
PINMUX_LCDINIT pinmux_get_dispmode(PINMUX_FUNC_ID id);
#endif
