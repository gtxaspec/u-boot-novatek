/*
    Pinmux module internal header file

    @file       top_reg.h
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2021.  All rights reserved
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#ifndef _PINMUX_INT_H
#define _PINMUX_INT_H

#include <asm/nvt-common/rcw_macro.h>

#define ENUM_DUMMY4WORD(name)   E_##name = 0x10000000

#define MUX_0                    0x00
#define MUX_1                    0x01
#define MUX_2                    0x02
#define MUX_3                    0x03
#define MUX_4                    0x04
#define MUX_5                    0x05
#define MUX_6                    0x06
#define MUX_7                    0x07
#define MUX_8                    0x08
#define MUX_9                    0x09
#define MUX_10                   0x0A

/*
    GPIO_ID_ENUM
*/
typedef enum {
	GPIO_ID_EMUM_FUNC,                  //< pinmux is mapping to function
	GPIO_ID_EMUM_GPIO,                  //< pinmux is mapping to gpio

	ENUM_DUMMY4WORD(GPIO_ID_ENUM)
} GPIO_ID_ENUM;

// 0x00 TOP Control Register 0 (BOOT)
#define TOP_REG0_OFS                        0x00
union TOP_REG0 {
	uint32_t reg;
	struct {
		unsigned int BOOT_SRC: 4;               // Boot Source Selection
		unsigned int EJTAG_SEL: 1;              // EJTAG select
		unsigned int DSI_PROT_EN: 1;
		unsigned int DEBUG_MODE_EN: 1;          // debug mode enable
		unsigned int MPLL_CLKSELL: 1;           // PLL clock output mux
		unsigned int DEBUG_MODE_SEL: 2;         // MIPS debug mode select
		unsigned int reserved0: 22;
	} bit;
};

// 0x04 TOP Control Register 1 (Storage)
#define TOP_REG1_OFS                        0x04
union TOP_REG1 {
	uint32_t reg;
	struct {
		unsigned int reserved0: 4;
		unsigned int FSPI_CS1_EN: 1;
		unsigned int ME_SHUT_IN: 2;
		unsigned int ME_SHUT_OUT: 2;
		unsigned int FLASH_TRIGA: 2;			// FLASH triga enable
		unsigned int FLASH_8BIT_DQS_EN: 1;
		unsigned int FSPI_EN: 2;                // FSPI enable control
		unsigned int SDIO_EN: 1;                // SDIO enable control
		unsigned int reserved1: 1;
		unsigned int EXTROM_EN: 1;              // EXTROM enable control
		unsigned int SDIO3_EN: 1;               // SDIO3 enable control
		unsigned int SDIO2_EN: 2;               // SDIO2 enable control
		unsigned int EJTAG_EN: 1;               // EJTAG enable control
		unsigned int EJTAG_CS: 1;               // EJTAG channel select
		unsigned int reserved2: 8;
		unsigned int SDIO3_DS_EN: 1;            // Select SDIO3 data strobe
		unsigned int SDIO3_BUS_WIDTH: 1;        // Select SDIO3 bus width
	} bit;
};

// 0x08 TOP Control Register 2 (LCD)
#define TOP_REG2_OFS                        0x08
union TOP_REG2 {
	uint32_t reg;
	struct {
		unsigned int LCD_TYPE: 4;               // Pinmux of LCD interface
		unsigned int reserved0: 2;
		unsigned int PLCD_DE: 1;                // Pinmux of DE (for parallel LCD)
		unsigned int CCIR_DATA_WIDTH: 1;        // CCIR data width
		unsigned int reserved1: 2;
		unsigned int CCIR_FIELD: 1;             // CCIR FIELD select
		unsigned int TE_SEL:1;                  // Memory Interface TE select
		unsigned int CCIR_HVLD: 2;         		// CCIR HVLD select
		unsigned int CCIR_VVLD: 2;        		// CCIR VVLD select
		unsigned int reserved2: 16;
	} bit;
};

// 0x0C TOP Control Register 3 (SENSOR)
#define TOP_REG3_OFS                        0x0C
union TOP_REG3 {
	uint32_t reg;
	struct {
		unsigned int SENSOR: 4;                 // Pinmux of SN interface
		unsigned int SENSOR2: 2;                // Pinmux of SN2 interface
		unsigned int SN_CCIR_VSHS: 2;          // Pinmux of SN2 CCIR VS/HS/FIELD
		unsigned int SN_MCLK: 2;                // Pinmux of SN MCLK
		unsigned int SN_VSHS: 2;                // Pinmux of SN VSHS
		unsigned int SN2_MCLK: 2;                // Pinmux of SN2 MCLK
		unsigned int SN2_VSHS: 2;                // Pinmux of SN2 VSHS
		unsigned int SN3_MCLK: 2;                // Pinmux of SN3 MCLK
		unsigned int SP_CLK: 2;
		unsigned int SN2_MCLK_SRC: 1;           // Pinmux of SN2_MCLK_SRC (from CG)
		unsigned int SN2_XVSHS_SRC: 1;          // Pinmux of SN2_XVSHS_SRC (TG)
		unsigned int SN_TIMER_SET_MUX:3;        // Pinmux of SN_TIMER_SET
		unsigned int SN_XVSXHS: 2;              // Pinmux of SN XVS/XHS
		unsigned int SN2_XVSXHS: 2;              // Pinmux of SN2 XVS/XHS
		unsigned int reserved1: 3;
	} bit;
};

// 0x10 TOP Control Register 4 (MI)
#define TOP_REG4_OFS                        0x10
union TOP_REG4 {
        uint32_t reg;
        struct {
        unsigned int MEMIF_TYPE:2;
        unsigned int MEMIF_SEL:2;
        unsigned int SMEMIF_DATA_WIDTH:1;
        unsigned int PMEMIF_DATA_WIDTH:2;
        unsigned int MEMIF_TE_SEL:1;
        unsigned int reserved0:24;
        } bit;
};


// 0x14 TOP Control Register 5 (I2C/SPI/SIF/ETH)
#define TOP_REG5_OFS                        0x14
union TOP_REG5 {
	uint32_t reg;
	struct {
		unsigned int I2C: 2;
		unsigned int ETH_MDIO_MUX: 2;
		unsigned int I2C2: 2;
		unsigned int SDP: 2;
		unsigned int SPI: 2;
		unsigned int SPI2: 2;
		unsigned int SPI3: 2;
		unsigned int SPI3_RDY: 2;
		unsigned int SPI_BUS_WIDTH: 1;
		unsigned int SPI2_BUS_WIDTH: 1;
		unsigned int SPI3_BUS_WIDTH: 1;
		unsigned int reserved0: 1;
		unsigned int SIF0: 1;
		unsigned int SIF1: 2;
		unsigned int SIF2: 3;
		unsigned int SIF3: 1;
		unsigned int I2C3: 3;
		unsigned int ETH: 2;
	} bit;
};

// 0x18 TOP Control Register 6 (AUDIO/PWM[1/2])
#define TOP_REG6_OFS                        0x18
union TOP_REG6 {
	uint32_t reg;
	struct {
		unsigned int I2S: 2;
		unsigned int I2S_MCLK: 2;
		unsigned int EXT_EAC_MCLK: 1;
		unsigned int DMIC_DATA0: 1;
		unsigned int DMIC_DATA1: 1;
		unsigned int REMOTE: 2;
		unsigned int RTC_CLK: 1;
		unsigned int SP_CLK2:2;
		unsigned int RTC_EXT_CLK:1;
		unsigned int ETH_LED: 2;
		unsigned int RTC_DIV_OUT:1;
		unsigned int PWM8: 3;
		unsigned int PWM9: 3;
		unsigned int PWM10: 3;
		unsigned int PWM11: 3;
		unsigned int ETH_EXT_PHY_CLK: 1;
		unsigned int DMIC: 3;
	} bit;
};

// 0x1C TOP Control Register 7 (PWM[2/2])
#define TOP_REG7_OFS                        0x1C
union TOP_REG7 {
	uint32_t reg;
	struct {
		unsigned int PWM0: 3;
		unsigned int PWM1: 3;
		unsigned int PWM2: 3;
		unsigned int PWM3: 3;
		unsigned int PWM4: 3;
		unsigned int PWM5: 3;
		unsigned int PWM6: 3;
		unsigned int PWM7: 3;
		unsigned int REMOTE_EXT: 2;
		unsigned int PICNT: 2;
		unsigned int PICNT2: 2;
		unsigned int PICNT3: 2;
	} bit;
};

// 0x20 TOP Control Register 8 (System)
#define TOP_REG8_OFS                        0x20
union TOP_REG8 {
	uint32_t reg;
	struct {
		unsigned int RAM_SRC_SELECT: 1;             // RAM SRC Select[0]
		unsigned int reserved0: 14;                 // bit[14..1] reserved
		unsigned int FAST_BOOT_CH_SEL: 2;           // Fast boot channel Select [16..15]
		unsigned int reserved1: 3;                  //bit[19.17]
		unsigned int FAST_BOOT_SCE_DIRMODE_EN: 1;   //bit[20]
		unsigned int FAST_BOOT_HASH_DIRMODE_EN: 1;  //bit[21]
		unsigned int FAST_BOOT_GZIP_DIRMODE_EN: 1;  //bit[22]
		unsigned int GZIP_SRAM_OWNER: 2;            //bit[24..23]
		unsigned int reserved2: 2;
		unsigned int VBUS_MONITOR: 1;
		unsigned int reserved3: 4;
	} bit;
};

// 0x24 TOP Control Register 9 (UART)
#define TOP_REG9_OFS                       0x24
union TOP_REG9 {
	uint32_t reg;
	struct {
		unsigned int UART:1;
		unsigned int UART2:3;
		unsigned int UART3:3;
		unsigned int reserved0: 1;
		unsigned int UART2_RTSCTS:2;
		unsigned int UART3_RTSCTS:2;
		unsigned int reserved1: 20;
	} bit;
};

// 0x28 TOP Control Register 10 (MISC)
#define TOP_REG10_OFS                       0x28
union TOP_REG10 {
	uint32_t reg;
	struct {
		unsigned int CPU_AXI_BRG: 1;
		unsigned int IP_AXI_BRG: 1;
		unsigned int IPE_DMA_CH: 1;
		unsigned int reserved0: 29;
	} bit;
};

// 0x2C TOP Control Register 11 (WEIGAID)
#define TOP_REG11_OFS                       0x2C
union TOP_REG11 {
	uint32_t reg;
	struct {
		unsigned int SPI_WG:3;
		unsigned int reserved0: 29;
	} bit;
};

// 0xA0 CGPIO Control Register 0 (C_GPIO)
#define TOP_REGCGPIO0_OFS                   0xA0
union TOP_REGCGPIO0 {
	uint32_t reg;
	struct {
		unsigned int CGPIO_0: 1;
		unsigned int CGPIO_1: 1;
		unsigned int CGPIO_2: 1;
		unsigned int CGPIO_3: 1;
		unsigned int CGPIO_4: 1;
		unsigned int CGPIO_5: 1;
		unsigned int CGPIO_6: 1;
		unsigned int CGPIO_7: 1;
		unsigned int CGPIO_8: 1;
		unsigned int CGPIO_9: 1;
		unsigned int CGPIO_10: 1;
		unsigned int CGPIO_11: 1;
		unsigned int CGPIO_12: 1;
		unsigned int CGPIO_13: 1;
		unsigned int CGPIO_14: 1;
		unsigned int CGPIO_15: 1;
		unsigned int CGPIO_16: 1;
		unsigned int CGPIO_17: 1;
		unsigned int CGPIO_18: 1;
		unsigned int CGPIO_19: 1;
		unsigned int CGPIO_20: 1;
		unsigned int CGPIO_21: 1;
		unsigned int CGPIO_22: 1;
		unsigned int CGPIO_23: 1;
		unsigned int CGPIO_24: 1;
		unsigned int reserved0: 7;
	} bit;
};

// 0xA4 Reserved

// 0xA8 PGPIO Control Register 0 (P_GPIO[1/2])
#define TOP_REGPGPIO0_OFS                   0xA8
union TOP_REGPGPIO0 {
	uint32_t reg;
	struct {
		unsigned int PGPIO_0: 1;
		unsigned int PGPIO_1: 1;
		unsigned int PGPIO_2: 1;
		unsigned int PGPIO_3: 1;
		unsigned int PGPIO_4: 1;
		unsigned int PGPIO_5: 1;
		unsigned int PGPIO_6: 1;
		unsigned int PGPIO_7: 1;
		unsigned int PGPIO_8: 1;
		unsigned int PGPIO_9: 1;
		unsigned int PGPIO_10: 1;
		unsigned int PGPIO_11: 1;
		unsigned int PGPIO_12: 1;
		unsigned int PGPIO_13: 1;
		unsigned int PGPIO_14: 1;
		unsigned int PGPIO_15: 1;
		unsigned int PGPIO_16: 1;
		unsigned int PGPIO_17: 1;
		unsigned int PGPIO_18: 1;
		unsigned int PGPIO_19: 1;
		unsigned int PGPIO_20: 1;
		unsigned int PGPIO_21: 1;
		unsigned int PGPIO_22: 1;
		unsigned int PGPIO_23: 1;
		unsigned int PGPIO_24: 1;
		unsigned int reserved0: 7;
	} bit;
};


// 0xAC Reserved

// 0xB0 DGPIO Control Register 0 (D_GPIO)
#define TOP_REGDGPIO0_OFS                   0xB0
union TOP_REGDGPIO0 {
	uint32_t reg;
	struct {
		unsigned int DGPIO_0: 1;
		unsigned int DGPIO_1: 1;
		unsigned int DGPIO_2: 1;
		unsigned int DGPIO_3: 1;
		unsigned int DGPIO_4: 1;
		unsigned int DGPIO_5: 1;
		unsigned int DGPIO_6: 1;
		unsigned int DGPIO_7: 1;
		unsigned int reserved0: 24;
	} bit;
};

// 0xB4 Reserved

// 0xB8 SGPIO Control Register 0 (S_GPIO)
#define TOP_REGSGPIO0_OFS                    0xB8
union TOP_REGSGPIO0 {
        uint32_t reg;
        struct {
        unsigned int SGPIO_0:1;
        unsigned int SGPIO_1:1;
        unsigned int SGPIO_2:1;
        unsigned int SGPIO_3:1;
        unsigned int SGPIO_4:1;
        unsigned int SGPIO_5:1;
        unsigned int SGPIO_6:1;
        unsigned int SGPIO_7:1;
        unsigned int SGPIO_8:1;
        unsigned int SGPIO_9:1;
        unsigned int SGPIO_10:1;
        unsigned int SGPIO_11:1;
        unsigned int SGPIO_12:1;
		unsigned int reserved0: 19;
        } bit;
};

// 0xBC Reserved

// 0xC0 HSIGPIO Control Register 0 (HSI_GPIO)
#define TOP_REGHSIGPIO0_OFS                  0xC0
union TOP_REGHSIGPIO0 {
	uint32_t reg;
	struct {
		unsigned int HSIGPIO_0: 1;
		unsigned int HSIGPIO_1: 1;
		unsigned int HSIGPIO_2: 1;
		unsigned int HSIGPIO_3: 1;
		unsigned int HSIGPIO_4: 1;
		unsigned int HSIGPIO_5: 1;
		unsigned int HSIGPIO_6: 1;
		unsigned int HSIGPIO_7: 1;
		unsigned int HSIGPIO_8: 1;
		unsigned int HSIGPIO_9: 1;
		unsigned int HSIGPIO_10: 1;
		unsigned int HSIGPIO_11: 1;
		unsigned int reserved0: 20;
	} bit;
};

// 0xC4 Reserved

// 0xC8 DSIGPIO Control Register 0 (DSI_GPIO)
#define TOP_REGDSIGPIO0_OFS                  0xC8
union TOP_REGDSIGPIO0 {
        uint32_t reg;
        struct {
        unsigned int DSIGPIO_0:1;
        unsigned int DSIGPIO_1:1;
        unsigned int DSIGPIO_2:1;
        unsigned int DSIGPIO_3:1;
        unsigned int DSIGPIO_4:1;
        unsigned int DSIGPIO_5:1;
        unsigned int DSIGPIO_6:1;
        unsigned int DSIGPIO_7:1;
        unsigned int DSIGPIO_8:1;
        unsigned int DSIGPIO_9:1;
        unsigned int DSIGPIO_10:1;
        unsigned int reserved0:21;
        } bit;
};

// 0xD0 LGPIO Control Register 0 (L_GPIO)
#define TOP_REGLGPIO0_OFS                    0xD0
union TOP_REGLGPIO0 {
        uint32_t reg;
        struct {
        unsigned int LGPIO_0:1;
        unsigned int LGPIO_1:1;
        unsigned int LGPIO_2:1;
        unsigned int LGPIO_3:1;
        unsigned int LGPIO_4:1;
        unsigned int LGPIO_5:1;
        unsigned int LGPIO_6:1;
        unsigned int LGPIO_7:1;
        unsigned int LGPIO_8:1;
        unsigned int LGPIO_9:1;
        unsigned int reserved0:22;
        } bit;
};

// 0xD4 Reserved

// 0xD8 AGPIO Control Register 0 (A_GPIO)
#define TOP_REGAGPIO0_OFS                    0xD8
union TOP_REGAGPIO0 {
        uint32_t reg;
        struct {
        unsigned int AGPIO_0:1;
        unsigned int AGPIO_1:1;
        unsigned int AGPIO_2:1;
        unsigned int AGPIO_3:1;
        unsigned int reserved0:28;
        } bit;
};

// 0xDC ~ 0xEC Reserved

// 0xF0 Version Code Register
#define TOP_VERSION_REG_OFS                  0xF0
union TOP_VERSION_REG {
	uint32_t reg;
	struct {
		unsigned int reserved0: 16;
		unsigned int CHIP_ID: 16;                // Chip ID
	} bit;
};

// 0xFC Debug Port Register
#define TOP_REG_DEBUG_OFS                    0xFC
union TOP_REG_DEBUG {
	uint32_t reg;
	struct {
		unsigned int DEBUG_SEL: 7;               // Debug Port Selection
		unsigned int reserved0:1;
		unsigned int DEBUG_OUTSEL: 2;            // Debug Port output select
		unsigned int reserved1: 22;
	} bit;
};

#endif