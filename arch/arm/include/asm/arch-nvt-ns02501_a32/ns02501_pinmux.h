#ifndef __PINMUX_DRV_H__
#define __PINMUX_DRV_H__

#include <asm/arch-nvt-ns02501_a32/top_reg.h>
#include <asm/arch-nvt-ns02501_a32/top.h>
#include <asm/nvt-common/nvt_common.h>
#include <asm/arch/hardware.h>

typedef uint32_t u32;

#define DRV_VERSION "1.00.07"

#define TOP_SETREG(info, ofs,value)    OUTW(info->top_base+(ofs),(value))
#define TOP_GETREG(info, ofs)          INW(info->top_base+(ofs))

#define MAX_PAD_NUM  236

#define GPIO_SETREG(info, ofs,value)       OUTW(info->gpio_base+(ofs),(value))
#define GPIO_GETREG(info, ofs)         INW(info->gpio_base+(ofs))

struct nvt_pad_info {
	unsigned long pad_ds_pin;
	unsigned long driving;
	unsigned long pad_gpio_pin;
	unsigned long direction;
};

struct nvt_gpio_info {
	unsigned long gpio_pin;
	unsigned long direction;
	unsigned long level;
};

struct nvt_power_info {
	unsigned long pad_power_id;
	unsigned long pad_power;
};

struct nvt_pinctrl_info {
		void __iomem *top_base;
		void __iomem *pad_base;
		void __iomem *gpio_base;
        PIN_GROUP_CONFIG top_pinmux[PIN_FUNC_MAX];
        struct nvt_pad_info pad[MAX_PAD_NUM];
};

typedef enum {
	CGPIO_0,
	CGPIO_1,
	CGPIO_2,
	CGPIO_3,
	CGPIO_4,
	CGPIO_5,
	CGPIO_6,
	CGPIO_7,
	CGPIO_8,
	CGPIO_9,
	CGPIO_10,
	CGPIO_11,
	CGPIO_12,
	CGPIO_13,
	CGPIO_14,
	CGPIO_15,
	CGPIO_16,
	CGPIO_17,
	CGPIO_18,
	CGPIO_19,
	CGPIO_20,
	CGPIO_21,
	CGPIO_22,
	CGPIO_23,
	CGPIO_24,
	C_GPIO_all,
	PGPIO_0,
	PGPIO_1,
	PGPIO_2,
	PGPIO_3,
	PGPIO_4,
	PGPIO_5,
	PGPIO_6,
	PGPIO_7,
	PGPIO_8,
	PGPIO_9,
	PGPIO_10,
	PGPIO_11,
	PGPIO_12,
	PGPIO_13,
	PGPIO_14,
	PGPIO_15,
	PGPIO_16,
	PGPIO_17,
	PGPIO_18,
	PGPIO_19,
	PGPIO_20,
	PGPIO_21,
	PGPIO_22,
	PGPIO_23,
	PGPIO_24,
	P_GPIO_all,
	DGPIO_0,
	DGPIO_1,
	DGPIO_2,
	DGPIO_3,
	DGPIO_4,
	DGPIO_5,
	DGPIO_6,
	DGPIO_7,
	D_GPIO_all,
	SGPIO_0,
	SGPIO_1,
	SGPIO_2,
	SGPIO_3,
	SGPIO_4,
	SGPIO_5,
	SGPIO_6,
	SGPIO_7,
	SGPIO_8,
	SGPIO_9,
	SGPIO_10,
	SGPIO_11,
	SGPIO_12,
	S_GPIO_all,
	HSIGPIO_0,
	HSIGPIO_1,
	HSIGPIO_2,
	HSIGPIO_3,
	HSIGPIO_4,
	HSIGPIO_5,
	HSIGPIO_6,
	HSIGPIO_7,
	HSIGPIO_8,
	HSIGPIO_9,
	HSIGPIO_10,
	HSIGPIO_11,
	HSI_GPIO_all,
	DSIGPIO_0,
	DSIGPIO_1,
	DSIGPIO_2,
	DSIGPIO_3,
	DSIGPIO_4,
	DSIGPIO_5,
	DSIGPIO_6,
	DSIGPIO_7,
	DSIGPIO_8,
	DSIGPIO_9,
	DSIGPIO_10,
	DSI_GPIO_all,
	LGPIO_0,
	LGPIO_1,
	LGPIO_2,
	LGPIO_3,
	LGPIO_4,
	LGPIO_5,
	LGPIO_6,
	LGPIO_7,
	LGPIO_8,
	LGPIO_9,
	L_GPIO_all,
	AGPIO_0,
	AGPIO_1,
	AGPIO_2,
	AGPIO_3,
	A_GPIO_all,
	GPIO_total,

	ENUM_DUMMY4WORD(GPIO_ALL)
} GPIO_ALL; //for gpio_func_show


typedef enum {
	func_FSPI = 1,
	func_SDIO,
	func_SDIO2,
	func_SDIO3,
	func_EJTAG,
	func_EXTROM,
	func_ETH,
	func_ETH2,
	/*func_CCIR626,
	func_CCIR601,
	func_LCD310_RGB888,
	func_LCD210_1,
	func_LCD210_2,
	func_LCD310_DE,*/
	/*I2C*/
	func_I2C_1,
	func_I2C_2,
	func_I2C_3,
	func_I2C2_1,
	func_I2C2_2,
	func_I2C2_3,
	func_I2C3_1,
	func_I2C3_2,
	func_I2C3_3,
	func_I2C3_4,
	func_I2C3_5,
	func_I2C3_6,
	/*PWM*/
	func_PWM_1,
	func_PWM_2,
	func_PWM_3,
	func_PWM_4,
	func_PWM_5,
	func_PWM_6,
	func_PWM1_1,
	func_PWM1_2,
	func_PWM1_3,
	func_PWM1_4,
	func_PWM1_5,
	func_PWM1_6,
	func_PWM2_1,
	func_PWM2_2,
	func_PWM2_3,
	func_PWM2_4,
	func_PWM2_5,
	func_PWM3_1,
	func_PWM3_2,
	func_PWM3_3,
	func_PWM3_4,
	func_PWM3_5,
	func_PWM4_1,
	func_PWM4_2,
	func_PWM4_3,
	func_PWM4_4,
	func_PWM4_5,
	func_PWM5_1,
	func_PWM5_2,
	func_PWM5_3,
	func_PWM5_4,
	func_PWM5_5,
	func_PWM6_1,
	func_PWM6_2,
	func_PWM6_3,
	func_PWM6_4,
	func_PWM6_5,
	func_PWM7_1,
	func_PWM7_2,
	func_PWM7_3,
	func_PWM7_4,
	func_PWM7_5,
	func_PWM8_1,
	func_PWM8_2,
	func_PWM8_3,
	func_PWM8_4,
	func_PWM8_5,
	func_PWM9_1,
	func_PWM9_2,
	func_PWM9_3,
	func_PWM9_4,
	func_PWM9_5,
	func_PWM10_1,
	func_PWM10_2,
	func_PWM10_3,
	func_PWM10_4,
	func_PWM10_5,
	func_PWM11_1,
	func_PWM11_2,
	func_PWM11_3,
	func_PWM11_4,
	func_PWM11_5,
	/*CCNT*/
	func_CCNT_1,
	func_CCNT_2,
	func_CCNT2_1,
	func_CCNT2_2,
	func_CCNT3_1,
	func_CCNT3_2,
	/*SENSOR*/
	func_SENSOR,
	func_SENSOR2,
	func_SENSORMISC,
	func_SN1_VSHS,
	func_SN1_VSHS_2,
	func_SN2_VSHS,
	func_SN1_CCIR_VSHS,
	func_SN1_CCIR_VSHS_2,
	func_SN1_XVSXHS,
	func_SN2_XVSXHS,
	func_SN1_MCLK,
	func_SN2_MCLK,
	func_SN3_MCLK,
	/*Audio*/
	func_I2S_1,
	func_I2S_1_MCLK,
	func_I2S_2,
	func_I2S_2_MCLK,
	func_I2S_3,
	func_I2S_3_MCLK,
	func_AUDIO_DMIC,
	func_AUDIO_EXT_MCLK,
	/*UART*/
	func_UART,
	func_UART2_1,
	func_UART2_2,
	func_UART2_3,
	func_UART2_4,
	func_UART2_5,
	func_UART2_6,
	func_UART3_1,
	func_UART3_2,
	func_UART3_3,
	func_UART3_4,
	func_UART3_5,
	func_UART2_CTSRTS,
	func_UART2_DTROE,
	func_UART3_CTSRTS,
	func_UART3_DTROE,
	/*remote*/
	func_Remote,
	/*SDP*/
	func_SDP_1,
	func_SDP_2,
	/*SPI*/
	func_SPI_1,
	func_SPI_2,
	func_SPI_3,
	func_SPI2_1,
	func_SPI2_2,
	func_SPI3_1,
	func_SPI3_2,
	func_SPI3_3,
	func_SPI3_RDY,
	func_SPI3_RDY2,
	func_SPI3_RDY3,
	/*SIF*/
	func_SIF_1,
	func_SIF1_1,
	func_SIF1_2,
	func_SIF2_1,
	func_SIF2_2,
	func_SIF2_3,
	func_SIF2_4,
	func_SIF2_5,
	func_SIF3_1,
	/*MISC*/
	func_MISC,
	/*LCD*/
	func_LCD,
	/*Weigand*/
	func_WG_1,
	func_WG_2,
	func_WG_3,
	func_WG_4,
	func_WG_5,
	FUNC_total,

	ENUM_DUMMY4WORD(FUNC_ALL)
} FUNC_ALL;

#define MAX_MODULE_NAME 10

ER pinmux_init(struct nvt_pinctrl_info *info);
ER pad_init(struct nvt_pinctrl_info *info, unsigned long nr_pad);
void gpio_init(struct nvt_gpio_info *gpio, int nr_gpio, struct nvt_pinctrl_info *info);
ER power_init(struct nvt_power_info *power, int nr_power, struct nvt_pinctrl_info *info);
void pinmux_parsing(struct nvt_pinctrl_info *info);
int nvt_pinmux_proc_init(void);
void pinmux_preset(struct nvt_pinctrl_info *info);
void pad_preset(struct nvt_pinctrl_info *info);
void pinmux_gpio_parsing(struct nvt_pinctrl_info *info);
int pinmux_set_config(PINMUX_FUNC_ID id, u32 pinmux);

void pad_show(void);
int pad_suspend(struct nvt_pinctrl_info *info);
int pad_resume(struct nvt_pinctrl_info *info);
int pinmux_conflict_check(void);

static inline int pinmux_set_host(struct nvt_pinctrl_info *info, PINMUX_FUNC_ID id, u32 pinmux)
{
	return 0;
};
#endif /* __PINCTRL_NVT_H */