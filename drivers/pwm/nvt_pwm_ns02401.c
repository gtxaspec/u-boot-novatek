#include "nvt_pwm.h"
#include <asm/nvt-common/nvt_reg.h>

#define NUM_PWM                 6

#define PIN_PWM_CFG_NONE                0x0

#define PIN_PWM_CFG_PWM_1               0x1         ///< PWM   (P_GPIO[28])
#define PIN_PWM_CFG_PWM2_1              0x10        ///< PWM2  (P_GPIO[29])
#define PIN_PWM_CFG_PWM3_1              0x100       ///< PWM3  (P_GPIO[30])
#define PIN_PWM_CFG_PWM4_1              0x1000      ///< PWM4  (P_GPIO[31])
#define PIN_PWM_CFG_PWM5_1              0x10000     ///< PWM5  (D_GPIO[0])
#define PIN_PWM_CFG_PWM6_1              0x100000    ///< PWM6  (D_GPIO[5])

static void nvt_pwm_masked_write(u32 hi, u32 lo, u32 val, void __iomem *phy_reg)
{
	u32 mask, tmp;

	mask = GENMASK(hi, lo);
	tmp = smc_readl((uintptr_t)phy_reg);

	tmp &= ~(mask);
	tmp |= ((val << lo) & mask);
	smc_writel(tmp, (uintptr_t)phy_reg);
}

void nvt_pwm_set_pinmux(int pwm_id, u32 pwm_pinmux)
{
	printf("pwm_id(%d) pwm_pinmux(0x%x)\n", pwm_id, pwm_pinmux);
	void __iomem *top_base = (void __iomem *)IOADDR_TOP_REG_BASE;

	if (pwm_id == 0) {
		if (pwm_pinmux & PIN_PWM_CFG_PWM_1) {
			nvt_pwm_masked_write(3, 0, 1, top_base + 0x18);
			nvt_pwm_masked_write(28, 28, 0, top_base + 0xA8);
		} else {
            DBG_ERR("!!Caution!!>>using pwm_id(%d) but pwm_pinmux(0x%x) not configured(check nvt-top.dtsi)\n", pwm_id, pwm_pinmux);
		}
	} else if (pwm_id == 1) {
		if (pwm_pinmux & PIN_PWM_CFG_PWM2_1) {
			nvt_pwm_masked_write(7, 4, 1, top_base + 0x18);
			nvt_pwm_masked_write(29, 29, 0, top_base + 0xA8);
		} else {
			DBG_ERR("!!Caution!!>>using pwm_id(%d) but pwm_pinmux(0x%x) not configured(check nvt-top.dtsi)\n", pwm_id, pwm_pinmux);
		}
	} else if (pwm_id == 2) {
		if (pwm_pinmux & PIN_PWM_CFG_PWM3_1) {
			nvt_pwm_masked_write(11, 8, 1, top_base + 0x18);
			nvt_pwm_masked_write(30, 30, 0, top_base + 0xA8);
		} else {
			DBG_ERR("!!Caution!!>>using pwm_id(%d) but pwm_pinmux(0x%x) not configured(check nvt-top.dtsi)\n", pwm_id, pwm_pinmux);
		}
	} else if (pwm_id == 3) {
		if (pwm_pinmux & PIN_PWM_CFG_PWM4_1) {
			nvt_pwm_masked_write(15, 12, 1, top_base + 0x18);
			nvt_pwm_masked_write(31, 31, 0, top_base + 0xA8);
		} else {
			DBG_ERR("!!Caution!!>>using pwm_id(%d) but pwm_pinmux(0x%x) not configured(check nvt-top.dtsi)\n", pwm_id, pwm_pinmux);
		}
	} else if (pwm_id == 4) {
		if (pwm_pinmux & PIN_PWM_CFG_PWM5_1) {
			nvt_pwm_masked_write(19, 16, 1, top_base + 0x18);
			nvt_pwm_masked_write(0, 0, 0, top_base + 0xB4);
		} else {
			DBG_ERR("!!Caution!!>>using pwm_id(%d) but pwm_pinmux(0x%x) not configured(check nvt-top.dtsi)\n", pwm_id, pwm_pinmux);
		}
	} else if (pwm_id == 5) {
		if (pwm_pinmux & PIN_PWM_CFG_PWM6_1) {
			nvt_pwm_masked_write(23, 20, 1, top_base + 0x18);
			nvt_pwm_masked_write(5, 5, 0, top_base + 0xB4);
		} else {
			DBG_ERR("!!Caution!!>>using pwm_id(%d) but pwm_pinmux(0x%x) not configured(check nvt-top.dtsi)\n", pwm_id, pwm_pinmux);
		}
	} else {
		printf("nvt_pwm_set_div: invalid pwm_id%d\r\n", pwm_id);
	}
}

int do_nvt_pwm_set_pinmux(int pwm_id)
{
	char pwm[10] = {0};
	char path[20] = {0};
	int nodeoffset, len;
    u32 pwm_pinmux = 0;
	u32 *cell = NULL;

	if (pwm_id < NUM_PWM) {
		sprintf(pwm, "pwm");
	} else {
		printf("%s(%d) invalid pwm:%d\n", __func__, __LINE__, pwm_id);
	}

    sprintf(path, "/top@4,f0010000/%s", pwm);

	nodeoffset = fdt_path_offset((const void *)nvt_fdt_buffer, path);
	if (nodeoffset < 0) {
		printf("%s(%d) nodeoffset < 0\n", __func__, __LINE__);
		return -1;
	}

	cell = (u32 *)fdt_getprop((const void *)nvt_fdt_buffer, nodeoffset, "pinmux", &len);
	if (cell == NULL) {
		printf("%s(%d) cell = NULL\n", __func__, __LINE__);
		return -1;
	}

	if (len == 0) {
		printf("%s(%d) len = 0\n", __func__, __LINE__);
		return -1;
	}

	pwm_pinmux = __be32_to_cpu(cell[0]);
	printf("%s_pinmux = 0x%x\n", pwm, pwm_pinmux);
	nvt_pwm_set_pinmux(pwm_id, pwm_pinmux);

	return 0;
}

void nvt_pwm_set_div(int pwm_id, u32 div)
{
	void __iomem *cg_base = (void __iomem *)IOADDR_CG_REG_BASE;

	if (pwm_id == 0) {
		nvt_pwm_masked_write(13, 0, div, cg_base + 0x30);
	} else if (pwm_id == 1) {
		nvt_pwm_masked_write(29, 16, div, cg_base + 0x30);
	} else if (pwm_id == 2) {
		nvt_pwm_masked_write(13, 0, div, cg_base + 0x34);
	} else if (pwm_id == 3) {
		nvt_pwm_masked_write(29, 16, div, cg_base + 0x34);
	} else if (pwm_id == 4) {
		nvt_pwm_masked_write(13, 0, div, cg_base + 0x38);
	} else if (pwm_id == 5) {
		nvt_pwm_masked_write(29, 16, div, cg_base + 0x38);
	} else {
		printf("nvt_pwm_set_div: invalid pwm_id%d\r\n", pwm_id);
	}
}

void pwm_enable_clk(int pwm_id)
{
	/* Enable clk */
	if (pwm_id < 12) {
		void __iomem *cg_base = (void __iomem *)IOADDR_CG_REG_BASE;
		nvt_pwm_masked_write(pwm_id + 17, pwm_id + 17, 1, cg_base + 0x78);
		nvt_pwm_masked_write(17, 17, 1, cg_base + 0x98);
	} else {
		printf("pwm_enable_clk: invalid pwm_id%d\r\n", pwm_id);
	}
}

bool pwmid_is_valid(int pwm_id)
{
    if (pwm_id < 0 || pwm_id >= NUM_PWM) {
        return false;
    }

    return true;
}