#ifndef __PINMUX_DRV_H__
#define __PINMUX_DRV_H__

/*
#include <plat/top_reg.h>
#include <plat/hardware.h>
#include <plat/top.h>
#include <plat/nvt-gpio.h>
#include <plat/pad.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/delay.h>
*/

#include "top_reg.h"
#include "top.h"
#include <asm/nvt-common/nvt_common.h>
#include <asm/arch/hardware.h>


#define DRV_VERSION "1.00.05"

#if IS_ENABLED(CONFIG_OPTEE) && defined(CONFIG_NVT_IVOT_PLAT_NS02401)
#include <linux/arm-smccc.h>
#define NVT_TOP_REG_READ        (0xC200FF08)
#define NVT_TOP_REG_WRITE		(0xC200FF09)
#else	/*IS_ENABLED(CONFIG_OPTEE)*/
#define TOP_SETREG(info, ofs,value)    OUTW(info->top_base+(ofs),(value))
#define TOP_GETREG(info, ofs)          INW(info->top_base+(ofs))
#endif

#define TGE_REG_ADDR(ofs)              (NVT_TGE_BASE_VIRT+(ofs))
#define TGE_GETREG(ofs)                INW(TGE_REG_ADDR(ofs))
#define TGE_SETREG(ofs,value)          OUTW(TGE_REG_ADDR(ofs), (value))
#define TGE_CONTROL_OFS                0x0

#define MAX_PAD_NUM  137

#define GPIO_SETREG(info, ofs,value)   OUTW(info->gpio_base+(ofs),(value))
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
	C_GPIO_all,
	JGPIO_0,
	JGPIO_1,
	JGPIO_2,
	JGPIO_3,
	JGPIO_4,
	JGPIO_5,
	J_GPIO_all,
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
	PGPIO_25,
	PGPIO_26,
	PGPIO_27,
	PGPIO_28,
	PGPIO_29,
	PGPIO_30,
	PGPIO_31,
	PGPIO_32,
	PGPIO_33,
	PGPIO_34,
	PGPIO_35,
	PGPIO_36,
	PGPIO_37,
	PGPIO_38,
	PGPIO_39,
	PGPIO_40,
	PGPIO_41,
	PGPIO_42,
	PGPIO_43,
	PGPIO_44,
	PGPIO_45,
	PGPIO_46,
	PGPIO_47,
	P_GPIO_all,
	EGPIO_0,
	EGPIO_1,
	EGPIO_2,
	EGPIO_3,
	EGPIO_4,
	EGPIO_5,
	EGPIO_6,
	EGPIO_7,
	EGPIO_8,
	EGPIO_9,
	EGPIO_10,
	EGPIO_11,
	EGPIO_12,
	EGPIO_13,
	EGPIO_14,
	EGPIO_15,
	EGPIO_16,
	EGPIO_17,
	EGPIO_18,
	EGPIO_19,
	EGPIO_20,
	EGPIO_21,
	EGPIO_22,
	EGPIO_23,
	EGPIO_24,
	EGPIO_25,
	EGPIO_26,
	EGPIO_27,
	EGPIO_28,
	EGPIO_29,
	EGPIO_30,
	EGPIO_31,
	E_GPIO_all,
	DGPIO_0,
	DGPIO_1,
	DGPIO_2,
	DGPIO_3,
	DGPIO_4,
	DGPIO_5,
	DGPIO_6,
	DGPIO_7,
	DGPIO_8,
	DGPIO_9,
	DGPIO_10,
	DGPIO_11,
	D_GPIO_all,
	BGPIO_0,
	BGPIO_1,
	BGPIO_2,
	BGPIO_3,
	BGPIO_4,
	BGPIO_5,
	BGPIO_6,
	BGPIO_7,
	BGPIO_8,
	BGPIO_9,
	BGPIO_10,
	BGPIO_11,
	BGPIO_12,
	BGPIO_13,
	BGPIO_14,
	BGPIO_15,
	BGPIO_16,
	B_GPIO_all,
	AGPIO_0,
	AGPIO_1,
	AGPIO_2,
	AGPIO_3,
	A_GPIO_all,
	GPIO_total,

	// backward compatible
	/*CGPIO_18,
	CGPIO_19,
	CGPIO_20,
	CGPIO_21,
	CGPIO_22,
	CGPIO_23,
	CGPIO_24,
	CGPIO_25,
	DGPIO_12,
	DGPIO_13,
	DGPIO_14,*/

	ENUM_DUMMY4WORD(GPIO_ALL)
} GPIO_ALL;

typedef enum {
	/* [0x04] Storage */
	func_FSPI = 1,
	func_SDIO,
	func_SDIO_2,
	func_SDIO_3,
	func_SDIO2,
	func_EJTAG,
	func_EXTROM,

	/* [0x0C] ETH */
	func_ETH,
	func_ETH2,

	/* [0x10] I2C */
	func_I2C,
	func_I2C2,
	func_I2C3,
	func_I2C3_2,
	func_I2C3_3,
	func_I2C3_4,
	func_I2C4,
	func_I2C4_2,
	func_I2C4_3,
	func_I2C4_4,
	func_I2C5,
	func_I2C5_2,
	func_I2C5_3,
	func_I2C5_4,
	func_I2C6,
	func_I2C7,
	func_I2C8,

	/* [0x18] PWM */
	func_PWM,
	func_PWM2,
	func_PWM3,
	func_PWM4,
	func_PWM5,
	func_PWM6,

	/* [0x30] I2S */
	func_I2S,
	func_I2S_MCLK,
	func_I2S_2,
	func_I2S_2_MCLK,
	func_I2S2,
	func_I2S2_MCLK,
	func_I2S2_2,
	func_I2S2_2_MCLK,
	func_I2S2_3,
	func_I2S2_3_MCLK,
	func_I2S3,
	func_I2S3_MCLK,
	func_I2S4,
	func_I2S4_MCLK,
	func_HDMI_CEC,
	func_HDMI2_CEC,
	func_HDMI3_CEC,

	/* [0x34] UART */
	func_UART,
	func_UART_2,
	func_UART2,
	func_UART2_2,
	func_UART3,
	func_UART3_2,
	func_UART4,
	func_UART4_2,
	func_UART4_3,
	func_UART5,
	func_UART5_2,
	func_UART_RTSCTS,
	func_UART_2_RTSCTS,
	func_UART2_RTSCTS,
	func_UART2_2_RTSCTS,
	func_UART3_RTSCTS,
	func_UART3_2_RTSCTS,

	/* [0x40] Remote / SDP */
	func_Remote,
	func_SDP,
	func_SDP_2,
	func_SDP_RDY,
	func_SDP_2_RDY,

	/* [0x44] SPI */
	func_SPI,
	func_SPI2,
	func_SPI2_2,
	func_SPI2_3,
	func_SPI2_4,

	/* [0x54] SATA_LED */
	func_SATA4_LED,
	func_SATA5_LED,
	func_SATA6_LED,
	func_SATA3_LED,
	func_SATA_LED,
	func_SATA2_LED,

	func_MISC,

	/* [0x08] LCD */
	func_LCD,

	FUNC_total,

	ENUM_DUMMY4WORD(FUNC_ALL)
} FUNC_ALL;

struct nvt_pinctrl_info {
	void __iomem *top_base;
	void __iomem *pad_base;
	void __iomem *gpio_base;
	PIN_GROUP_CONFIG top_pinmux[PIN_FUNC_MAX];
	PIN_GROUP_CONFIG sec_pinmux[PIN_FUNC_MAX];	// secure pinmux
	BOOL sec_pinmux_valid;
	struct nvt_pad_info pad[MAX_PAD_NUM];
};

#define MAX_MODULE_NAME 10

#if IS_ENABLED(CONFIG_OPTEE) && defined(CONFIG_NVT_IVOT_PLAT_NS02401)
static unsigned long __invoke_nvt_top_reg_fn_smc(unsigned long function_id,
        unsigned long arg0, unsigned long arg1,
        unsigned long arg2)
{
    struct arm_smccc_res res;
    arm_smccc_smc(function_id, arg0, arg1, arg2, 0, 0, 0, 0, &res);
    return res.a0;
}

static inline void TOP_SETREG(struct nvt_pinctrl_info *info, int ofs, u32 value)
{
    __invoke_nvt_top_reg_fn_smc(NVT_TOP_REG_WRITE, ofs, value, 0);
}

static inline u32 TOP_GETREG(struct nvt_pinctrl_info *info, int ofs)
{
    return __invoke_nvt_top_reg_fn_smc(NVT_TOP_REG_READ, ofs, 0, 0);
}
#endif
ER pinmux_init(struct nvt_pinctrl_info *info);
ER pad_init(struct nvt_pinctrl_info *info, unsigned long nr_pad);
void gpio_init(struct nvt_gpio_info *gpio, int nr_gpio, struct nvt_pinctrl_info *info);
ER power_init(struct nvt_power_info *power, int nr_power, struct nvt_pinctrl_info *info);
void pinmux_parsing(struct nvt_pinctrl_info *info);
ER pinmux_parsing_i2c(uint32_t config);
ER pinmux_parsing_i2cII(uint32_t config);
ER pinmux_parsing_i2cIV(uint32_t config);
ER pinmux_parsing_i2cIII(uint32_t config);
int nvt_pinmux_proc_init(void);
void pinmux_preset(struct nvt_pinctrl_info *info);
void pad_preset(struct nvt_pinctrl_info *info);
void pinmux_gpio_parsing(struct nvt_pinctrl_info *info);
int pinmux_set_config(PINMUX_FUNC_ID id, u32 pinmux);

void pad_show(struct nvt_pinctrl_info *info);
int pad_suspend(struct nvt_pinctrl_info *info);
int pad_resume(struct nvt_pinctrl_info *info);

static inline int pinmux_set_host(struct nvt_pinctrl_info *info, PINMUX_FUNC_ID id, u32 pinmux)
{
	return 0;
};
#endif /* __PINCTRL_NVT_H */
