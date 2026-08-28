/*
    PAD controller

    Sets the PAD control of each pin.

    @file       pad.c
    @ingroup    mIDrvSys_PAD
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include <asm/arch/pad.h>
#include <asm/nvt_pinmux.h>
#include <asm/nvt-common/rcw_macro.h>

#define __MODULE__    rtos_pad
// #include <kwrap/debug.h>
// unsigned int rtos_pad_debug_level = NVT_DBG_WRN;

//static u32 top_reg0;

static u32 TOP_PAD_REG_BASE;
#define PAD_GETREG(ofs)         INW(TOP_PAD_REG_BASE + (ofs))
#define PAD_SETREG(ofs,value)   OUTW(TOP_PAD_REG_BASE + (ofs), value)

// static DEFINE_SPINLOCK(pad_lock);
// #define loc_cpu(flags) spin_lock_irqsave(&pad_lock, flags)
// #define unl_cpu(flags) spin_unlock_irqrestore(&pad_lock, flags)

// #define pr_err(fmt, args...)         DBG_ERR(fmt, ##args)
#define msleep(x)                    vos_util_delay_ms(x)


/**
    Set a pin pull up/down/keeper.

    Set a pin pull up/down/keeper.

    @param[in]  pin Pin name. For example PAD_PIN_xxxx. See pad.h.
    @param[in]  pulltype PAD_NONE or PAD_PULLUP or PAD_PULLDOWN or PAD_KEEPER

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_set_pull_updown(PAD_PIN pin, PAD_PULL pulltype)
{
	UINT32 dw_offset, bit_offset;
	REGVALUE reg_data;

	if (pin > PAD_PIN_MAX) {
		pr_err("%s: Not Existed PAD_PIN %d\r\n", __func__, pin);
		return E_NOEXS;
	}

	bit_offset = pin & 0x1F;
	dw_offset = (pin >> 5);

	loc_cpu();
	reg_data = PAD_GETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2));
	reg_data &= ~(0x3 << bit_offset);
	PAD_SETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2), reg_data);
	reg_data |= (pulltype << bit_offset);
	PAD_SETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2), reg_data);
	unl_cpu();

	return E_OK;
}
// EXPORT_SYMBOL(pad_set_pull_updown);

/**
    Get a pin pull up/down/keeper

    Get a pin pull up/down/keeper

    @param[in]      pin         Pin name. For example PAD_PIN_xxxx. See pad.h.
    @param[out]     pulltype    PAD_NONE or PAD_PULLUP or PAD_PULLDOWN or PAD_KEEPER

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_get_pull_updown(PAD_PIN pin, PAD_PULL *pulltype)
{
	UINT32 dw_offset, bit_offset;
	REGVALUE reg_data;

	if (pulltype == NULL) {
        pr_err("%s: pulltype is NULL pointer\r\n", __func__);
        return E_NOEXS;
    }

	if (pin > PAD_PIN_MAX) {
		pr_err("%s: Not Existed PAD_PIN %d\r\n", __func__, pin);
		return E_NOEXS;
	}

	bit_offset = pin & 0x1F;
	dw_offset = (pin >> 5);

	reg_data = PAD_GETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2));
	*pulltype = ((reg_data >> bit_offset) & 0x3);

	return E_OK;
}
// EXPORT_SYMBOL(pad_get_pull_updown);

/**
    Set driving/sink.

    Set driving/sink.

    @param[in] name     For example PAD_DS_xxxx. See pad.h.
    @param[in] driving  PAD_DRIVINGSINK_xxMA. See pad.h.

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_set_drivingsink(PAD_DS name, PAD_DRIVINGSINK driving)
{
	UINT32 dw_offset, bit_offset;
	REGVALUE reg_data;

	if (name > PAD_DS_MAX) {
		pr_err("%s: Not Existed PAD_DS %d\r\n", __func__, name);
		return E_NOEXS;
	}

	bit_offset = name & 0x1F;
	dw_offset = (name >> 5);

	loc_cpu();
	reg_data = PAD_GETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2));

	if ((dw_offset << 5) == PAD_2L_DS_HSIGPIO_BASE) {
		/* 2 level case */
		reg_data &= ~(0x1 << bit_offset);
	} else if (((dw_offset << 5) >= PAD_8L_DS_CGPIO_BASE) && ((dw_offset << 5) <= PAD_8L_DS_SGPIO_BASE)) {
		/* 8 level case */
		reg_data &= ~(0xf << bit_offset);
	} else {
		/* 4 level case */
		reg_data &= ~(0x3 << bit_offset);
	}

	PAD_SETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2), reg_data);
	reg_data |= (driving << bit_offset);
	PAD_SETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2), reg_data);
	unl_cpu();

	return E_OK;
}
// EXPORT_SYMBOL(pad_set_drivingsink);

/**
    Get driving/sink.

    Get driving/sink.

    @param[in]  name        For example PAD_DS_xxxx. See pad.h.
    @param[out] driving     PAD_DRIVINGSINK_xxMA. See pad.h.

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_get_drivingsink(PAD_DS name, PAD_DRIVINGSINK *driving)
{
	UINT32 dw_offset, bit_offset;
	REGVALUE reg_data;

	if (name > PAD_DS_MAX) {
		pr_err("%s: Not Existed PAD_DS %d\r\n", __func__, name);
		return E_NOEXS;
	}

	bit_offset = name & 0x1F;
	dw_offset = (name >> 5);

	reg_data = PAD_GETREG(PAD_PUPD0_REG_OFS + (dw_offset << 2));

	if (((dw_offset << 5) == PAD_2L_DS_HSIGPIO_BASE)) {
		/* 2 level case */
		*driving = ((reg_data >> bit_offset) & 0x1);
	} else if (((dw_offset << 5) >= PAD_8L_DS_CGPIO_BASE) && ((dw_offset << 5) <= PAD_8L_DS_SGPIO_BASE)) {
		/* 8 level case */
		*driving = ((reg_data >> bit_offset) & 0xf);

	} else {
		/* 4 level case */
		*driving = ((reg_data >> bit_offset) & 0x3);
	}

	return E_OK;
}
// EXPORT_SYMBOL(pad_get_drivingsink);

#define ADC_POWER0_BIT 0
#define ADC_SEL_BIT 1
#define ADC_OPSEL_BIT 2
#define ADC_PULLDOWN_BIT 3
#define ADC_REGULATOR_EN_BIT 4
#define ADC_VAD_BIT 5
/**
    Set pad power.

    Set pad power.

    @param[in] pad_power  pointer to pad power struct

    @return
        - @b E_OK: Done with no error
        - Others: Error occured.
*/
ER pad_set_power_host(PAD_POWER_STRUCT *pad_power)
{
	REGVALUE pwr0_reg;

	if (pad_power == NULL) {
		pr_err("%s: Not Existed PAD_POWER_STRUCT\r\n", __func__);
		return E_NOEXS;
	}

	pwr0_reg = PAD_GETREG(PAD_PWR0_REG_OFS);

	if (pad_power->pad_power_id == PAD_POWERID_ADC) {
		if (pad_power->pad_vad & PAD_VAD_2P4V)
			return E_PAR;

		//ADC_POWER0
		pwr0_reg &= ~(0x1 << ADC_POWER0_BIT);
		pwr0_reg |= (pad_power->pad_power << ADC_POWER0_BIT);
		//ADC_SEL
		pwr0_reg &= ~(0x1 << ADC_SEL_BIT);
		pwr0_reg |= (pad_power->bias_current << ADC_SEL_BIT);

		//ADC_OPSEL
		pwr0_reg &= ~(0x1 << ADC_OPSEL_BIT);
		pwr0_reg |= (pad_power->opa_gain << ADC_OPSEL_BIT);

		//ADC_PULLDOWN
		pwr0_reg &= ~(0x1 << ADC_PULLDOWN_BIT);
		pwr0_reg |= (pad_power->pull_down << ADC_PULLDOWN_BIT);

		//ADC_REGULATOR_EN
		pwr0_reg &= ~(0x1 << ADC_REGULATOR_EN_BIT);
		pwr0_reg |= (pad_power->enable << ADC_REGULATOR_EN_BIT);

		//ADC_VAD
		pwr0_reg &= ~(0x3 << ADC_VAD_BIT);
		pwr0_reg |= (pad_power->pad_vad << ADC_VAD_BIT);

		loc_cpu();
		PAD_SETREG(PAD_PWR0_REG_OFS, pwr0_reg);
		unl_cpu();

		return E_OK;
	} else {
		pr_err("%s: Not Existed PAD_POWERID\r\n", __func__);
		return E_NOEXS;
	}
}
// EXPORT_SYMBOL(pad_set_power_host);

/**
    Get pad power.

    get pad power.

    @param[in] pad_power  pointer to pad power struct
*/
void pad_get_power(PAD_POWER_STRUCT *pad_power)
{
	REGVALUE pwr0_reg;

	pwr0_reg = PAD_GETREG(PAD_PWR0_REG_OFS);

	if (pad_power->pad_power_id == PAD_POWERID_ADC) {
		pwr0_reg = PAD_GETREG(PAD_PWR0_REG_OFS);

		pad_power->pad_power = (pwr0_reg >> ADC_POWER0_BIT) & 0x1;
		pad_power->bias_current = (pwr0_reg >> ADC_SEL_BIT) & 0x1;
		pad_power->opa_gain = (pwr0_reg >> ADC_OPSEL_BIT) & 0x1;
		pad_power->pull_down =  (pwr0_reg >> ADC_PULLDOWN_BIT) & 0x1;
		pad_power->enable = (pwr0_reg >> ADC_REGULATOR_EN_BIT) & 0x1;
		pad_power->pad_vad = (pwr0_reg >> ADC_VAD_BIT) & 0x3;;
	}
}
// EXPORT_SYMBOL(pad_get_power);

void pad_preset(struct nvt_pinctrl_info *info)
{
	if (info == NULL) {
		pr_err("%s: Not Existed nvt_pinctrl_info\r\n", __func__);
    }
	TOP_PAD_REG_BASE = (unsigned long) info->pad_base;
}

ER pad_init(struct nvt_pinctrl_info *info, unsigned long nr_pad)
{
	unsigned long cnt;
	int ret = 0;

	for (cnt = 0; cnt < nr_pad; cnt++) {
		ret = pad_set_pull_updown(info->pad[cnt].pad_gpio_pin, info->pad[cnt].direction);
		if (ret) {
			pr_err("%s: pad pin 0x%lx pull failed!\n", __func__, info->pad[cnt].pad_gpio_pin);
			return ret;
		}

		ret = pad_set_drivingsink(info->pad[cnt].pad_ds_pin, info->pad[cnt].driving);
		if (ret) {
			pr_err("%s: pad pin 0x%lx driving failed!\n", __func__, info->pad[cnt].pad_ds_pin);
			return ret;
		}
	}

	return E_OK;
}


void pad_show(void)
{

	CHAR pull_type[4][32] = {"\033[0;37mNone\033[0m", "\033[0;36mPullDown\033[0m", "\033[0;33mPullUp\033[0m", "\033[1;33mStrong PullUp\033[0m"};
	UINT32 index, pinMax, uiPullType = 0, uiPadLevel = 0;

	pr_err("%-12s | %-10s | %-15s\r\n", "Pin Name", "Pull", "Driving");
	pr_err("============================================\r\n");

	// C_GPIO
	pinMax = 19;
	for (index = 0; index <= pinMax; index++) {
		pad_get_drivingsink(PAD_DS_CGPIO0 + (4*index), &uiPadLevel);
		pad_get_pull_updown(PAD_PIN_CGPIO0 + (2*index), &uiPullType);
		pr_err("C_GPIO_%02d     | %-2d %-19s | 0x%-6x\r\n", index, uiPullType, pull_type[uiPullType], uiPadLevel);
	}
	pinMax = 4;
	for (index = 0; index <= pinMax; index++) {
		pad_get_drivingsink(PAD_DS_CGPIO20 + (2*index), &uiPadLevel);
		pad_get_pull_updown(PAD_PIN_CGPIO0 + (2*index), &uiPullType);
		pr_err("C_GPIO_%02d     | %-2d %-19s | 0x%-6x\r\n", index + 20, uiPullType, pull_type[uiPullType], uiPadLevel);
	}

    //HSI_GPIO
    pinMax = 9;
	for (index = 0; index <= pinMax; index++) {
		pad_get_drivingsink(PAD_DS_HSIGPIO0 + (1*index), &uiPadLevel);
		pad_get_pull_updown(PAD_PIN_HSIGPIO0 + (2*index), &uiPullType);
		pr_err("HSI_GPIO_%02d   | %-2d %-17s | 0x%-6x\r\n", index, uiPullType, pull_type[uiPullType], uiPadLevel);
	}
	pinMax = 1;
	for (index = 0; index <= pinMax; index++) {
		pad_get_drivingsink(PAD_DS_HSIGPIO10 + (2*index), &uiPadLevel);
		pad_get_pull_updown(PAD_PIN_HSIGPIO0 + (2*index), &uiPullType);
		pr_err("HSI_GPIO_%02d   | %-2d %-17s | 0x%-6x\r\n", index + 10, uiPullType, pull_type[uiPullType], uiPadLevel);
	}

	//DSI_GPIO
	pinMax = 10;
	for (index = 0; index <= pinMax; index++) {
		pad_get_drivingsink(PAD_DS_DSIGPIO0 + (1*index), &uiPadLevel);
		pad_get_pull_updown(PAD_PIN_DSIGPIO0 + (2*index), &uiPullType);
		pr_err("DSI_GPIO_%02d   | %-2d %-17s | 0x%-6x\r\n", index, uiPullType, pull_type[uiPullType], uiPadLevel);
	}

    //S_GPIO
	pad_get_drivingsink(PAD_DS_SGPIO0, &uiPadLevel);
	pad_get_pull_updown(PAD_PIN_SGPIO0 + (2*index), &uiPullType);
	pr_err("S_GPIO_%02d     | %-2d %-19s | 0x%-6x\r\n", 0, uiPullType, pull_type[uiPullType], uiPadLevel);
	pinMax = 7;
	for (index = 0; index <= pinMax; index++) {
		pad_get_drivingsink(PAD_DS_SGPIO0, &uiPadLevel);
		pad_get_pull_updown(PAD_PIN_SGPIO0 + (2*index), &uiPullType);
		pr_err("S_GPIO_%02d     | %-2d %-19s | 0x%-6x\r\n", index + 1, uiPullType, pull_type[uiPullType], uiPadLevel);
	}

    //L_GPIO
	pinMax = 9;
	for (index = 0; index <= pinMax; index++) {
		pad_get_drivingsink(PAD_DS_LGPIO0 + (1*index), &uiPadLevel);
		pad_get_pull_updown(PAD_PIN_LGPIO0 + (2*index), &uiPullType);
		pr_err("L_GPIO_%02d     | %-2d %-19s | 0x%-6x\r\n", index, uiPullType, pull_type[uiPullType], uiPadLevel);
	}

    // A_GPIO
	pinMax = 3;
	for (index = 0; index <= pinMax; index++) {
		pad_get_drivingsink(PAD_DS_AGPIO0 + (1*index), &uiPadLevel);
		pad_get_pull_updown(PAD_PIN_AGPIO0 + (2*index), &uiPullType);
		pr_err("A_GPIO_%02d     | %-2d %-19s | 0x%-6x\r\n", index, uiPullType, pull_type[uiPullType], uiPadLevel);
	}

	// P_GPIO
	pinMax = 24;
	for (index = 0; index <= pinMax; index++) {
		pad_get_drivingsink(PAD_DS_PGPIO0 + (2*index), &uiPadLevel);
		pad_get_pull_updown(PAD_PIN_PGPIO0 + (2*index), &uiPullType);
		pr_err("P_GPIO_%02d     | %-2d %-19s | 0x%-6x\r\n", index, uiPullType, pull_type[uiPullType], uiPadLevel);
	}

	// D_GPIO
	pinMax = 7;
	for (index = 0; index <= pinMax; index++) {
		pad_get_drivingsink(PAD_DS_DGPIO0 + (2*index), &uiPadLevel);
		pad_get_pull_updown(PAD_PIN_DGPIO0 + (2*index), &uiPullType);
		pr_err("D_GPIO_%02d     | %-2d %-19s | 0x%-6x\r\n", index, uiPullType, pull_type[uiPullType], uiPadLevel);
	}
}

ER power_init(struct nvt_power_info *power, int nr_power, struct nvt_pinctrl_info *info)
{
#if !defined(_NVT_FPGA_)
	if (info == NULL) {
		pr_err("%s: Not Existed nvt_pinctrl_info\r\n", __func__);
		return E_NOEXS;
	}
	if (power == NULL) {
		pr_err("%s: Not Existed nvt_power_info\r\n", __func__);
        return E_NOEXS;
    }

	PAD_POWER_STRUCT pad_power;
	int cnt;
	int ret = 0;

	for (cnt = 0; cnt < nr_power; cnt++) {
		pad_power.pad_power_id = power[cnt].pad_power_id;
		pad_power.pad_power = power[cnt].pad_power;

		ret = pad_set_power_host(&pad_power);
		if (ret) {
			pr_err("%s: pad_power_id(%lx) set pad_power(%lx) failed!\n", __func__, power[cnt].pad_power_id, power[cnt].pad_power);
			return ret;
		}
	}
#endif
	return E_OK;
}
