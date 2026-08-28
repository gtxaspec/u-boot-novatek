/**
    PAD controller header

    PAD controller header

    @file       pad.h
    @ingroup    mIDrvSys_PAD
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2021.  All rights reserved
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#ifndef _PAD_H
#define _PAD_H

#include <asm/nvt-common/nvt_types.h>

/*********** MAIN PAD ***********/
// PAD Pull UP/DOWN Register 0
#define PAD_PUPD0_REG_OFS                   0x00
#define PAD_PUPD1_REG_OFS                   0x04
#define PAD_PUPD2_REG_OFS                   0x08
#define PAD_PUPD3_REG_OFS                   0x0C
#define PAD_PUPD4_REG_OFS                   0x10
#define PAD_PUPD5_REG_OFS                   0x14
#define PAD_PUPD6_REG_OFS                   0x18
#define PAD_PUPD7_REG_OFS                   0x1C
#define PAD_PUPD8_REG_OFS                   0x20
#define PAD_PUPD9_REG_OFS                   0x24
#define PAD_DS_REG_OFS                      0x40
#define PAD_DS1_REG_OFS                     0x44
#define PAD_DS2_REG_OFS                     0x48
#define PAD_DS3_REG_OFS                     0x4C
#define PAD_DS4_REG_OFS                     0x50
#define PAD_DS5_REG_OFS                     0x54
#define PAD_DS6_REG_OFS                     0x58
#define PAD_DS7_REG_OFS                     0x5C
#define PAD_DS8_REG_OFS                     0x60
#define PAD_DS9_REG_OFS                     0x64
#define PAD_DS10_REG_OFS                    0x68
#define PAD_DS11_REG_OFS                    0x90

//0x78 PAD Power Control Register 0
#define PAD_PWR0_REG_OFS                     0x64

#define PAD_REG_TO_BASE(reg) (((reg)/(4))*(32))

#define PAD_CGPIO_BASE           PAD_REG_TO_BASE(0x00)      // 0x00~0x04
#define PAD_PGPIO_BASE           PAD_REG_TO_BASE(0x08)      // 0x08, 0x0C
#define PAD_DGPIO_BASE           PAD_REG_TO_BASE(0x10)      // 0x10
#define PAD_SGPIO_BASE           PAD_REG_TO_BASE(0x14)      // 0x14
#define PAD_HSIGPIO_BASE         PAD_REG_TO_BASE(0x18)      // 0x18
#define PAD_DSIGPIO_BASE         PAD_REG_TO_BASE(0x1C)      // 0x1C
#define PAD_LGPIO_BASE           PAD_REG_TO_BASE(0x20)      // 0x20
#define PAD_AGPIO_BASE           PAD_REG_TO_BASE(0x24)      // 0x24

/* 4 level */
#define PAD_4L_DS_CGPIO_BASE     PAD_REG_TO_BASE(0x2C)      // 0x2C
#define PAD_4L_DS_PGPIO_BASE     PAD_REG_TO_BASE(0x30)      // 0x30~0x34
#define PAD_4L_DS_DGPIO_BASE     PAD_REG_TO_BASE(0x38)      // 0x38
#define PAD_4L_DS_SGPIO_BASE     PAD_REG_TO_BASE(0x3C)      // 0x3C
#define PAD_4L_DS_HSIGPIO_BASE   PAD_REG_TO_BASE(0x40)      // 0x40
#define PAD_4L_DS_DSIGPIO_BASE   PAD_REG_TO_BASE(0x44)      // 0x44
#define PAD_4L_DS_LGPIO_BASE     PAD_REG_TO_BASE(0x48)      // 0x48
#define PAD_4L_DS_AGPIO_BASE     PAD_REG_TO_BASE(0x4C)      // 0x4C

/* 8 level */
#define PAD_8L_DS_CGPIO_BASE     PAD_REG_TO_BASE(0x50)      // 0x50~0x58
#define PAD_8L_DS_SGPIO_BASE     PAD_REG_TO_BASE(0x60)      // 0x60

/* 2 level */
#define PAD_2L_DS_HSIGPIO_BASE   PAD_REG_TO_BASE(0x5c)      // 0x48

typedef enum {
	PAD_NONE                    =    0x00,      ///< none of pull up/down
	PAD_PULLDOWN                =    0x01,      ///< pull down
	PAD_PULLUP                  =    0x02,      ///< pull up
	PAD_KEEPER                  =    0x03,      ///< keeper
	PAD_STRONG_PULLUP           =    0x03,      ///< strong pull up

	ENUM_DUMMY4WORD(PAD_PULL)
} PAD_PULL;

typedef enum {
	PAD_DRIVINGSINK_LEVEL_0     =    0x00,
	PAD_DRIVINGSINK_LEVEL_1     =    0x01,
	PAD_DRIVINGSINK_LEVEL_2     =    0x02,
	PAD_DRIVINGSINK_LEVEL_3     =    0x03,
	PAD_DRIVINGSINK_LEVEL_4     =    0x04,
	PAD_DRIVINGSINK_LEVEL_5     =    0x05,
	PAD_DRIVINGSINK_LEVEL_6     =    0x06,
	PAD_DRIVINGSINK_LEVEL_7     =    0x07,
	PAD_DRIVINGSINK_LEVEL_8     =    0x08,
	PAD_DRIVINGSINK_LEVEL_9     =    0x09,
	PAD_DRIVINGSINK_LEVEL_10    =    0x0A,
	PAD_DRIVINGSINK_LEVEL_11    =    0x0B,
	PAD_DRIVINGSINK_LEVEL_12    =    0x0C,
	PAD_DRIVINGSINK_LEVEL_13    =    0x0D,
	PAD_DRIVINGSINK_LEVEL_14    =    0x0E,
	PAD_DRIVINGSINK_LEVEL_15    =    0x0F,

	ENUM_DUMMY4WORD(PAD_DRIVINGSINK)
} PAD_DRIVINGSINK;

typedef enum {
	// CGPIO group
	PAD_PIN_CGPIO0      = (PAD_CGPIO_BASE + 0),        ///< C_GPIO_0
	PAD_PIN_CGPIO1      = (PAD_CGPIO_BASE + 2),        ///< C_GPIO_1
	PAD_PIN_CGPIO2      = (PAD_CGPIO_BASE + 4),        ///< C_GPIO_2
	PAD_PIN_CGPIO3      = (PAD_CGPIO_BASE + 6),        ///< C_GPIO_3
	PAD_PIN_CGPIO4      = (PAD_CGPIO_BASE + 8),        ///< C_GPIO_4
	PAD_PIN_CGPIO5      = (PAD_CGPIO_BASE + 10),       ///< C_GPIO_5
	PAD_PIN_CGPIO6      = (PAD_CGPIO_BASE + 12),       ///< C_GPIO_6
	PAD_PIN_CGPIO7      = (PAD_CGPIO_BASE + 14),       ///< C_GPIO_7
	PAD_PIN_CGPIO8      = (PAD_CGPIO_BASE + 16),       ///< C_GPIO_8
	PAD_PIN_CGPIO9      = (PAD_CGPIO_BASE + 18),       ///< C_GPIO_9
	PAD_PIN_CGPIO10     = (PAD_CGPIO_BASE + 20),       ///< C_GPIO_10
	PAD_PIN_CGPIO11     = (PAD_CGPIO_BASE + 22),       ///< C_GPIO_11
	PAD_PIN_CGPIO12     = (PAD_CGPIO_BASE + 24),       ///< C_GPIO_12
	PAD_PIN_CGPIO13     = (PAD_CGPIO_BASE + 26),       ///< C_GPIO_13
	PAD_PIN_CGPIO14     = (PAD_CGPIO_BASE + 28),       ///< C_GPIO_14
	PAD_PIN_CGPIO15     = (PAD_CGPIO_BASE + 30),       ///< C_GPIO_15
	PAD_PIN_CGPIO16     = (PAD_CGPIO_BASE + 32),       ///< C_GPIO_16
	PAD_PIN_CGPIO17     = (PAD_CGPIO_BASE + 34),       ///< C_GPIO_17
	PAD_PIN_CGPIO18     = (PAD_CGPIO_BASE + 36),       ///< C_GPIO_18
	PAD_PIN_CGPIO19     = (PAD_CGPIO_BASE + 38),       ///< C_GPIO_19
	PAD_PIN_CGPIO20     = (PAD_CGPIO_BASE + 40),       ///< C_GPIO_20
	PAD_PIN_CGPIO21     = (PAD_CGPIO_BASE + 42),       ///< C_GPIO_21
	PAD_PIN_CGPIO22     = (PAD_CGPIO_BASE + 44),       ///< C_GPIO_22
	PAD_PIN_CGPIO23     = (PAD_CGPIO_BASE + 46),       ///< C_GPIO_23
	PAD_PIN_CGPIO24     = (PAD_CGPIO_BASE + 48),       ///< C_GPIO_24

	// PGPIO group
	PAD_PIN_PGPIO0      = (PAD_PGPIO_BASE + 0),        ///< P_GPIO_0
	PAD_PIN_PGPIO1      = (PAD_PGPIO_BASE + 2),        ///< P_GPIO_1
	PAD_PIN_PGPIO2      = (PAD_PGPIO_BASE + 4),        ///< P_GPIO_2
	PAD_PIN_PGPIO3      = (PAD_PGPIO_BASE + 6),        ///< P_GPIO_3
	PAD_PIN_PGPIO4      = (PAD_PGPIO_BASE + 8),        ///< P_GPIO_4
	PAD_PIN_PGPIO5      = (PAD_PGPIO_BASE + 10),       ///< P_GPIO_5
	PAD_PIN_PGPIO6      = (PAD_PGPIO_BASE + 12),       ///< P_GPIO_6
	PAD_PIN_PGPIO7      = (PAD_PGPIO_BASE + 14),       ///< P_GPIO_7
	PAD_PIN_PGPIO8      = (PAD_PGPIO_BASE + 16),       ///< P_GPIO_8
	PAD_PIN_PGPIO9      = (PAD_PGPIO_BASE + 18),       ///< P_GPIO_9
	PAD_PIN_PGPIO10     = (PAD_PGPIO_BASE + 20),       ///< P_GPIO_10
	PAD_PIN_PGPIO11     = (PAD_PGPIO_BASE + 22),       ///< P_GPIO_11
	PAD_PIN_PGPIO12     = (PAD_PGPIO_BASE + 24),       ///< P_GPIO_12
	PAD_PIN_PGPIO13     = (PAD_PGPIO_BASE + 26),       ///< P_GPIO_13
	PAD_PIN_PGPIO14     = (PAD_PGPIO_BASE + 28),       ///< P_GPIO_14
	PAD_PIN_PGPIO15     = (PAD_PGPIO_BASE + 30),       ///< P_GPIO_15
	PAD_PIN_PGPIO16     = (PAD_PGPIO_BASE + 32),       ///< P_GPIO_16
	PAD_PIN_PGPIO17     = (PAD_PGPIO_BASE + 34),       ///< P_GPIO_17
	PAD_PIN_PGPIO18     = (PAD_PGPIO_BASE + 36),       ///< P_GPIO_18
	PAD_PIN_PGPIO19     = (PAD_PGPIO_BASE + 38),       ///< P_GPIO_19
	PAD_PIN_PGPIO20     = (PAD_PGPIO_BASE + 40),       ///< P_GPIO_20
	PAD_PIN_PGPIO21     = (PAD_PGPIO_BASE + 42),       ///< P_GPIO_21
	PAD_PIN_PGPIO22     = (PAD_PGPIO_BASE + 44),       ///< P_GPIO_22
	PAD_PIN_PGPIO23     = (PAD_PGPIO_BASE + 46),       ///< P_GPIO_23
	PAD_PIN_PGPIO24     = (PAD_PGPIO_BASE + 48),       ///< P_GPIO_24
	PAD_PIN_PGPIO25     = (PAD_PGPIO_BASE + 50),       ///< P_GPIO_25

	// DGPIO group
	PAD_PIN_DGPIO0      = (PAD_DGPIO_BASE + 0),        ///< D_GPIO_0
	PAD_PIN_DGPIO1      = (PAD_DGPIO_BASE + 2),        ///< D_GPIO_1
	PAD_PIN_DGPIO2      = (PAD_DGPIO_BASE + 4),        ///< D_GPIO_2
	PAD_PIN_DGPIO3      = (PAD_DGPIO_BASE + 6),        ///< D_GPIO_3
	PAD_PIN_DGPIO4      = (PAD_DGPIO_BASE + 8),        ///< D_GPIO_4
	PAD_PIN_DGPIO5      = (PAD_DGPIO_BASE + 10),       ///< D_GPIO_5
	PAD_PIN_DGPIO6      = (PAD_DGPIO_BASE + 12),       ///< D_GPIO_6
	PAD_PIN_DGPIO7      = (PAD_DGPIO_BASE + 14),       ///< D_GPIO_7

	// SGPIO group
	PAD_PIN_SGPIO0      = (PAD_SGPIO_BASE + 0),        ///< S_GPIO_0
	PAD_PIN_SGPIO1      = (PAD_SGPIO_BASE + 2),        ///< S_GPIO_1
	PAD_PIN_SGPIO2      = (PAD_SGPIO_BASE + 4),        ///< S_GPIO_2
	PAD_PIN_SGPIO3      = (PAD_SGPIO_BASE + 6),        ///< S_GPIO_3
	PAD_PIN_SGPIO4      = (PAD_SGPIO_BASE + 8),        ///< S_GPIO_4
	PAD_PIN_SGPIO5      = (PAD_SGPIO_BASE + 10),       ///< S_GPIO_5
	PAD_PIN_SGPIO6      = (PAD_SGPIO_BASE + 12),       ///< S_GPIO_6
	PAD_PIN_SGPIO7      = (PAD_SGPIO_BASE + 14),       ///< S_GPIO_7
	PAD_PIN_SGPIO8      = (PAD_SGPIO_BASE + 16),       ///< S_GPIO_8

	// HSIGPIO group
	PAD_PIN_HSIGPIO0      = (PAD_HSIGPIO_BASE + 0),        ///< HSI_GPIO_0
	PAD_PIN_HSIGPIO1      = (PAD_HSIGPIO_BASE + 2),        ///< HSI_GPIO_1
	PAD_PIN_HSIGPIO2      = (PAD_HSIGPIO_BASE + 4),        ///< HSI_GPIO_2
	PAD_PIN_HSIGPIO3      = (PAD_HSIGPIO_BASE + 6),        ///< HSI_GPIO_3
	PAD_PIN_HSIGPIO4      = (PAD_HSIGPIO_BASE + 8),        ///< HSI_GPIO_4
	PAD_PIN_HSIGPIO5      = (PAD_HSIGPIO_BASE + 10),       ///< HSI_GPIO_5
	PAD_PIN_HSIGPIO6      = (PAD_HSIGPIO_BASE + 12),       ///< HSI_GPIO_6
	PAD_PIN_HSIGPIO7      = (PAD_HSIGPIO_BASE + 14),       ///< HSI_GPIO_7
	PAD_PIN_HSIGPIO8      = (PAD_HSIGPIO_BASE + 16),       ///< HSI_GPIO_8
	PAD_PIN_HSIGPIO9      = (PAD_HSIGPIO_BASE + 18),       ///< HSI_GPIO_9
	PAD_PIN_HSIGPIO10     = (PAD_HSIGPIO_BASE + 20),       ///< HSI_GPIO_10
	PAD_PIN_HSIGPIO11     = (PAD_HSIGPIO_BASE + 22),       ///< HSI_GPIO_11

	// DSIGPIO group
	PAD_PIN_DSIGPIO0      = (PAD_DSIGPIO_BASE + 0),        ///< DSI_GPIO_0
	PAD_PIN_DSIGPIO1      = (PAD_DSIGPIO_BASE + 2),        ///< DSI_GPIO_1
	PAD_PIN_DSIGPIO2      = (PAD_DSIGPIO_BASE + 4),        ///< DSI_GPIO_2
	PAD_PIN_DSIGPIO3      = (PAD_DSIGPIO_BASE + 6),        ///< DSI_GPIO_3
	PAD_PIN_DSIGPIO4      = (PAD_DSIGPIO_BASE + 8),        ///< DSI_GPIO_4
	PAD_PIN_DSIGPIO5      = (PAD_DSIGPIO_BASE + 10),       ///< DSI_GPIO_5
	PAD_PIN_DSIGPIO6      = (PAD_DSIGPIO_BASE + 12),       ///< DSI_GPIO_6
	PAD_PIN_DSIGPIO7      = (PAD_DSIGPIO_BASE + 14),       ///< DSI_GPIO_7
	PAD_PIN_DSIGPIO8      = (PAD_DSIGPIO_BASE + 16),       ///< DSI_GPIO_8
	PAD_PIN_DSIGPIO9      = (PAD_DSIGPIO_BASE + 18),       ///< DSI_GPIO_9
	PAD_PIN_DSIGPIO10     = (PAD_DSIGPIO_BASE + 20),       ///< DSI_GPIO_10

	// LGPIO group
	PAD_PIN_LGPIO0      = (PAD_LGPIO_BASE + 0),        ///< L_GPIO_0
	PAD_PIN_LGPIO1      = (PAD_LGPIO_BASE + 2),        ///< L_GPIO_1
	PAD_PIN_LGPIO2      = (PAD_LGPIO_BASE + 4),        ///< L_GPIO_2
	PAD_PIN_LGPIO3      = (PAD_LGPIO_BASE + 6),        ///< L_GPIO_3
	PAD_PIN_LGPIO4      = (PAD_LGPIO_BASE + 8),        ///< L_GPIO_4
	PAD_PIN_LGPIO5      = (PAD_LGPIO_BASE + 10),       ///< L_GPIO_5
	PAD_PIN_LGPIO6      = (PAD_LGPIO_BASE + 12),       ///< L_GPIO_6
	PAD_PIN_LGPIO7      = (PAD_LGPIO_BASE + 14),       ///< L_GPIO_7
	PAD_PIN_LGPIO8      = (PAD_LGPIO_BASE + 16),       ///< L_GPIO_8
	PAD_PIN_LGPIO9      = (PAD_LGPIO_BASE + 18),       ///< L_GPIO_9

	// AGPIO group
	PAD_PIN_AGPIO0      = (PAD_AGPIO_BASE + 0),        ///< A_GPIO_0
	PAD_PIN_AGPIO1      = (PAD_AGPIO_BASE + 2),        ///< A_GPIO_1
	PAD_PIN_AGPIO2      = (PAD_AGPIO_BASE + 4),        ///< A_GPIO_2
	PAD_PIN_AGPIO3      = (PAD_AGPIO_BASE + 6),        ///< A_GPIO_3

	PAD_PIN_MAX         =     PAD_PIN_AGPIO3,

	ENUM_DUMMY4WORD(PAD_PIN)
} PAD_PIN;

typedef enum {
	// CGPIO group
	PAD_DS_CGPIO0      = (PAD_8L_DS_CGPIO_BASE + 0),        ///< C_GPIO_0
	PAD_DS_CGPIO1      = (PAD_8L_DS_CGPIO_BASE + 4),        ///< C_GPIO_1
	PAD_DS_CGPIO2      = (PAD_8L_DS_CGPIO_BASE + 8),        ///< C_GPIO_2
	PAD_DS_CGPIO3      = (PAD_8L_DS_CGPIO_BASE + 12),       ///< C_GPIO_3
	PAD_DS_CGPIO4      = (PAD_8L_DS_CGPIO_BASE + 16),       ///< C_GPIO_4
	PAD_DS_CGPIO5      = (PAD_8L_DS_CGPIO_BASE + 20),       ///< C_GPIO_5
	PAD_DS_CGPIO6      = (PAD_8L_DS_CGPIO_BASE + 24),       ///< C_GPIO_6
	PAD_DS_CGPIO7      = (PAD_8L_DS_CGPIO_BASE + 28),       ///< C_GPIO_7
	PAD_DS_CGPIO8      = (PAD_8L_DS_CGPIO_BASE + 32),       ///< C_GPIO_8
	PAD_DS_CGPIO9      = (PAD_8L_DS_CGPIO_BASE + 36),       ///< C_GPIO_9
	PAD_DS_CGPIO10     = (PAD_8L_DS_CGPIO_BASE + 40),       ///< C_GPIO_10
	PAD_DS_CGPIO11     = (PAD_8L_DS_CGPIO_BASE + 44),       ///< C_GPIO_11
	PAD_DS_CGPIO12     = (PAD_8L_DS_CGPIO_BASE + 48),       ///< C_GPIO_12
	PAD_DS_CGPIO13     = (PAD_8L_DS_CGPIO_BASE + 52),       ///< C_GPIO_13
	PAD_DS_CGPIO14     = (PAD_8L_DS_CGPIO_BASE + 56),       ///< C_GPIO_14
	PAD_DS_CGPIO15     = (PAD_8L_DS_CGPIO_BASE + 60),       ///< C_GPIO_15
	PAD_DS_CGPIO16     = (PAD_8L_DS_CGPIO_BASE + 64),       ///< C_GPIO_16
	PAD_DS_CGPIO17     = (PAD_8L_DS_CGPIO_BASE + 68),       ///< C_GPIO_17
	PAD_DS_CGPIO18     = (PAD_8L_DS_CGPIO_BASE + 72),       ///< C_GPIO_18
	PAD_DS_CGPIO19     = (PAD_8L_DS_CGPIO_BASE + 76),       ///< C_GPIO_19

	PAD_DS_CGPIO20     = (PAD_4L_DS_CGPIO_BASE + 8),       ///< C_GPIO_20
	PAD_DS_CGPIO21     = (PAD_4L_DS_CGPIO_BASE + 10),       ///< C_GPIO_21
	PAD_DS_CGPIO22     = (PAD_4L_DS_CGPIO_BASE + 12),       ///< C_GPIO_22
	PAD_DS_CGPIO23     = (PAD_4L_DS_CGPIO_BASE + 14),       ///< C_GPIO_23
	PAD_DS_CGPIO24     = (PAD_4L_DS_CGPIO_BASE + 16),       ///< C_GPIO_24

	// PGPIO group
	PAD_DS_PGPIO0      = (PAD_4L_DS_PGPIO_BASE + 0),        ///< P_GPIO_0
	PAD_DS_PGPIO1      = (PAD_4L_DS_PGPIO_BASE + 2),        ///< P_GPIO_1
	PAD_DS_PGPIO2      = (PAD_4L_DS_PGPIO_BASE + 4),        ///< P_GPIO_2
	PAD_DS_PGPIO3      = (PAD_4L_DS_PGPIO_BASE + 6),       ///< P_GPIO_3
	PAD_DS_PGPIO4      = (PAD_4L_DS_PGPIO_BASE + 8),       ///< P_GPIO_4
	PAD_DS_PGPIO5      = (PAD_4L_DS_PGPIO_BASE + 10),       ///< P_GPIO_5
	PAD_DS_PGPIO6      = (PAD_4L_DS_PGPIO_BASE + 12),       ///< P_GPIO_6
	PAD_DS_PGPIO7      = (PAD_4L_DS_PGPIO_BASE + 14),       ///< P_GPIO_7
	PAD_DS_PGPIO8      = (PAD_4L_DS_PGPIO_BASE + 16),       ///< P_GPIO_8
	PAD_DS_PGPIO9      = (PAD_4L_DS_PGPIO_BASE + 18),       ///< P_GPIO_9
	PAD_DS_PGPIO10     = (PAD_4L_DS_PGPIO_BASE + 20),       ///< P_GPIO_10
	PAD_DS_PGPIO11     = (PAD_4L_DS_PGPIO_BASE + 22),       ///< P_GPIO_11
	PAD_DS_PGPIO12     = (PAD_4L_DS_PGPIO_BASE + 24),       ///< P_GPIO_12
	PAD_DS_PGPIO13     = (PAD_4L_DS_PGPIO_BASE + 26),       ///< P_GPIO_13
	PAD_DS_PGPIO14     = (PAD_4L_DS_PGPIO_BASE + 28),       ///< P_GPIO_14
	PAD_DS_PGPIO15     = (PAD_4L_DS_PGPIO_BASE + 30),       ///< P_GPIO_15
	PAD_DS_PGPIO16     = (PAD_4L_DS_PGPIO_BASE + 32),       ///< P_GPIO_16
	PAD_DS_PGPIO17     = (PAD_4L_DS_PGPIO_BASE + 34),       ///< P_GPIO_17
	PAD_DS_PGPIO18     = (PAD_4L_DS_PGPIO_BASE + 36),       ///< P_GPIO_18
	PAD_DS_PGPIO19     = (PAD_4L_DS_PGPIO_BASE + 38),       ///< P_GPIO_19
	PAD_DS_PGPIO20     = (PAD_4L_DS_PGPIO_BASE + 40),       ///< P_GPIO_20
	PAD_DS_PGPIO21     = (PAD_4L_DS_PGPIO_BASE + 42),       ///< P_GPIO_21
	PAD_DS_PGPIO22     = (PAD_4L_DS_PGPIO_BASE + 44),       ///< P_GPIO_22
	PAD_DS_PGPIO23     = (PAD_4L_DS_PGPIO_BASE + 46),       ///< P_GPIO_23
	PAD_DS_PGPIO24     = (PAD_4L_DS_PGPIO_BASE + 48),       ///< P_GPIO_24
	PAD_DS_PGPIO25     = (PAD_4L_DS_PGPIO_BASE + 50),       ///< P_GPIO_25

	// DGPIO group
	PAD_DS_DGPIO0      = (PAD_4L_DS_DGPIO_BASE + 0),        ///< D_GPIO_0
	PAD_DS_DGPIO1      = (PAD_4L_DS_DGPIO_BASE + 2),        ///< D_GPIO_1
	PAD_DS_DGPIO2      = (PAD_4L_DS_DGPIO_BASE + 4),        ///< D_GPIO_2
	PAD_DS_DGPIO3      = (PAD_4L_DS_DGPIO_BASE + 6),       ///< D_GPIO_3
	PAD_DS_DGPIO4      = (PAD_4L_DS_DGPIO_BASE + 8),       ///< D_GPIO_4
	PAD_DS_DGPIO5      = (PAD_4L_DS_DGPIO_BASE + 10),       ///< D_GPIO_5
	PAD_DS_DGPIO6      = (PAD_4L_DS_DGPIO_BASE + 12),       ///< D_GPIO_6
	PAD_DS_DGPIO7      = (PAD_4L_DS_DGPIO_BASE + 14),       ///< D_GPIO_7
	PAD_DS_SYS_RST     = (PAD_4L_DS_DGPIO_BASE + 28),       ///< SYS_RST

	// SGPIO group
	PAD_DS_SGPIO0      = (PAD_8L_DS_SGPIO_BASE + 0),        ///< S_GPIO_0
	PAD_DS_SGPIO1      = (PAD_4L_DS_SGPIO_BASE + 2),        ///< S_GPIO_1
	PAD_DS_SGPIO2      = (PAD_4L_DS_SGPIO_BASE + 4),        ///< S_GPIO_2
	PAD_DS_SGPIO3      = (PAD_4L_DS_SGPIO_BASE + 6),       ///< S_GPIO_3
	PAD_DS_SGPIO4      = (PAD_4L_DS_SGPIO_BASE + 8),       ///< S_GPIO_4
	PAD_DS_SGPIO5      = (PAD_4L_DS_SGPIO_BASE + 10),       ///< S_GPIO_5
	PAD_DS_SGPIO6      = (PAD_4L_DS_SGPIO_BASE + 12),       ///< S_GPIO_6
	PAD_DS_SGPIO7      = (PAD_4L_DS_SGPIO_BASE + 14),       ///< S_GPIO_7
	PAD_DS_SGPIO8      = (PAD_4L_DS_SGPIO_BASE + 16),       ///< S_GPIO_8

	// HSIGPIO group
	PAD_DS_HSIGPIO0      = (PAD_2L_DS_HSIGPIO_BASE + 0),        ///< HSI_GPIO_0
	PAD_DS_HSIGPIO1      = (PAD_2L_DS_HSIGPIO_BASE + 1),        ///< HSI_GPIO_1
	PAD_DS_HSIGPIO2      = (PAD_2L_DS_HSIGPIO_BASE + 2),        ///< HSI_GPIO_2
	PAD_DS_HSIGPIO3      = (PAD_2L_DS_HSIGPIO_BASE + 3),       ///< HSI_GPIO_3
	PAD_DS_HSIGPIO4      = (PAD_2L_DS_HSIGPIO_BASE + 4),       ///< HSI_GPIO_4
	PAD_DS_HSIGPIO5      = (PAD_2L_DS_HSIGPIO_BASE + 5),       ///< HSI_GPIO_5
	PAD_DS_HSIGPIO6      = (PAD_2L_DS_HSIGPIO_BASE + 6),       ///< HSI_GPIO_6
	PAD_DS_HSIGPIO7      = (PAD_2L_DS_HSIGPIO_BASE + 7),       ///< HSI_GPIO_7
	PAD_DS_HSIGPIO8      = (PAD_2L_DS_HSIGPIO_BASE + 8),       ///< HSI_GPIO_8
	PAD_DS_HSIGPIO9      = (PAD_2L_DS_HSIGPIO_BASE + 9),       ///< HSI_GPIO_9
	PAD_DS_HSIGPIO10     = (PAD_4L_DS_HSIGPIO_BASE + 20),       ///< HSI_GPIO_10
	PAD_DS_HSIGPIO11     = (PAD_4L_DS_HSIGPIO_BASE + 22),       ///< HSI_GPIO_11

	// DSIGPIO group
	PAD_DS_DSIGPIO0      = (PAD_4L_DS_DSIGPIO_BASE + 0),        ///< DSI_GPIO_0
	PAD_DS_DSIGPIO1      = (PAD_4L_DS_DSIGPIO_BASE + 2),        ///< DSI_GPIO_1
	PAD_DS_DSIGPIO2      = (PAD_4L_DS_DSIGPIO_BASE + 4),        ///< DSI_GPIO_2
	PAD_DS_DSIGPIO3      = (PAD_4L_DS_DSIGPIO_BASE + 6),        ///< DSI_GPIO_3
	PAD_DS_DSIGPIO4      = (PAD_4L_DS_DSIGPIO_BASE + 8),        ///< DSI_GPIO_4
	PAD_DS_DSIGPIO5      = (PAD_4L_DS_DSIGPIO_BASE + 10),        ///< DSI_GPIO_5
	PAD_DS_DSIGPIO6      = (PAD_4L_DS_DSIGPIO_BASE + 12),        ///< DSI_GPIO_6
	PAD_DS_DSIGPIO7      = (PAD_4L_DS_DSIGPIO_BASE + 14),        ///< DSI_GPIO_7
	PAD_DS_DSIGPIO8      = (PAD_4L_DS_DSIGPIO_BASE + 16),        ///< DSI_GPIO_8
	PAD_DS_DSIGPIO9      = (PAD_4L_DS_DSIGPIO_BASE + 18),        ///< DSI_GPIO_9
	PAD_DS_DSIGPIO10     = (PAD_4L_DS_DSIGPIO_BASE + 20),       ///< DSI_GPIO_10

	// LGPIO group
	PAD_DS_LGPIO0      = (PAD_4L_DS_LGPIO_BASE + 0),        ///< L_GPIO_0
	PAD_DS_LGPIO1      = (PAD_4L_DS_LGPIO_BASE + 2),        ///< L_GPIO_1
	PAD_DS_LGPIO2      = (PAD_4L_DS_LGPIO_BASE + 4),        ///< L_GPIO_2
	PAD_DS_LGPIO3      = (PAD_4L_DS_LGPIO_BASE + 6),        ///< L_GPIO_3
	PAD_DS_LGPIO4      = (PAD_4L_DS_LGPIO_BASE + 8),        ///< L_GPIO_4
	PAD_DS_LGPIO5      = (PAD_4L_DS_LGPIO_BASE + 10),        ///< L_GPIO_5
	PAD_DS_LGPIO6      = (PAD_4L_DS_LGPIO_BASE + 12),        ///< L_GPIO_6
	PAD_DS_LGPIO7      = (PAD_4L_DS_LGPIO_BASE + 14),        ///< L_GPIO_7
	PAD_DS_LGPIO8      = (PAD_4L_DS_LGPIO_BASE + 16),        ///< L_GPIO_8
	PAD_DS_LGPIO9      = (PAD_4L_DS_LGPIO_BASE + 18),        ///< L_GPIO_9

	// AGPIO group
	PAD_DS_AGPIO0      = (PAD_4L_DS_AGPIO_BASE + 0),        ///< A_GPIO_0
	PAD_DS_AGPIO1      = (PAD_4L_DS_AGPIO_BASE + 2),        ///< A_GPIO_1
	PAD_DS_AGPIO2      = (PAD_4L_DS_AGPIO_BASE + 4),        ///< A_GPIO_2
	PAD_DS_AGPIO3      = (PAD_4L_DS_AGPIO_BASE + 6),        ///< A_GPIO_3

	PAD_DS_MAX = PAD_DS_SGPIO0,

	ENUM_DUMMY4WORD(PAD_DS)
} PAD_DS;

#define PAD_1P8V_SN_MAGIC       0x00005378
#define PAD_1P8V_SN2_MAGIC       0x00005378
#define PAD_1P8V_P1_MAGIC       0x00008031
#define PAD_1P8V_SD3_MAGIC      0x00534433

/**
    Pad power ID select

    Pad power ID for PAD_POWER_STRUCT.
*/
typedef enum {
	PAD_POWERID_MC0  =          0x00,      ///< Pad power id for MC0 LDO
	PAD_POWERID_MC1  =          0x01,      ///< Pad power id for MC1 LDO
	PAD_POWERID_ADC  =          0x02,      ///< Pad power id for ADC (backward compatible)
	PAD_POWERID_CSI  =          0x04,      ///< Pad power id for CSI (backward compatible)
	PAD_POWERID_SN   =          0x10,      ///< Pad power id for SN  REG
	PAD_POWERID_SN2  =          0x20,      ///< Pad power id for SN  REG
	PAD_POWERID_P1   =          0x40,      ///< Pad power id for P1  REG
	PAD_POWERID_SD3  =          0x80,      ///< Pad power id for SD3 REG

	ENUM_DUMMY4WORD(PAD_POWERID)
} PAD_POWERID;

/**
    Pad power select

    Pad power value for PAD_POWER_STRUCT.
*/
typedef enum {
	PAD_3P3V         =          0x00,      ///< Pad power is 1.8V
	PAD_1P8V         =          0x01,      ///< Pad power is 3.3V
	PAD_AVDD         =          0x00,      ///< Pad power is AVDD ( for PAD_POWERID_ADC use) (backward compatible)
	PAD_PAD_VAD      =          0x01,      ///< Pad power is PAD_ADC_VAD ( for PAD_POWERID_ADC use) (backward compatible)

	ENUM_DUMMY4WORD(PAD_POWER)
} PAD_POWER;

/**
    Pad power VAD for  PAD_POWERID_ADC

    Pad power VAD value for PAD_POWER_STRUCT.
*/
typedef enum {
	PAD_VAD_2P9V     =          0x00,      ///< Pad power VAD = 2.9V <ADC> (backward compatible)
	PAD_VAD_3P0V     =          0x01,      ///< Pad power VAD = 3.0V <ADC> (backward compatible)
	PAD_VAD_3P1V     =          0x03,      ///< Pad power VAD = 3.1V <ADC> (backward compatible)

	PAD_VAD_2P4V     =          0x100,     ///< Pad power VAD = 2.4V <CSI> (backward compatible)
	PAD_VAD_2P5V     =          0x101,     ///< Pad power VAD = 2.5V <CSI> (backward compatible)
	PAD_VAD_2P6V     =          0x103,     ///< Pad power VAD = 2.6V <CSI> (backward compatible)

	ENUM_DUMMY4WORD(PAD_VAD)
} PAD_VAD;

/**
    PAD power structure

    PAD power setting for pad_setPower()
*/
typedef struct {
	PAD_POWERID         pad_power_id;      ///< Pad power id
	PAD_POWER           pad_power;         ///< Pad power (for set)
	PAD_POWER           pad_status;        ///< Pad cell MS1/MS2 status (for get)
	BOOL                bias_current;      ///< Regulator bias current selection (backward compatible)
	///< - @b FALSE: disable
	///< - @b TRUE: enable
	BOOL                opa_gain;          ///< Regulator OPA gain/phase selection (backward compatible)
	///< - @b FALSE: disable
	///< - @b TRUE: enable
	BOOL                pull_down;         ///< Regulator output pull down control (backward compatible)
	///< - @b FALSE: none
	///< - @b TRUE: pull down enable
	BOOL                enable;            ///< Regulator enable (backward compatible)
	///< - @b FALSE: disable
	///< - @b TRUE: enable
	PAD_VAD             pad_vad;           ///< Pad VAD of PAD_POWERID_ADC when PAD_PAD_VAD = 1 (backward compatible)
} PAD_POWER_STRUCT;

extern ER pad_set_pull_updown(PAD_PIN pin, PAD_PULL pulltype);
extern ER pad_get_pull_updown(PAD_PIN pin, PAD_PULL *pulltype);
extern ER pad_set_drivingsink(PAD_DS name, PAD_DRIVINGSINK driving);
extern ER pad_get_drivingsink(PAD_DS name, PAD_DRIVINGSINK *driving);
extern ER pad_set_power(PAD_POWER_STRUCT *pad_power);
extern void pad_get_power(PAD_POWER_STRUCT *pad_power);

#endif
