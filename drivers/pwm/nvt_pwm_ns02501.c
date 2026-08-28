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
	/* Use pinmux driver */
}

int do_nvt_pwm_set_pinmux(int pwm_id)
{
	/* Use pinmux driver */
	return 0;
}

void nvt_pwm_set_div(int pwm_id, u32 div)
{
	void __iomem *cg_base = (void __iomem *)IOADDR_CG_REG_BASE;

	if (pwm_id < 4) {
		nvt_pwm_masked_write(13, 0, div, cg_base + 0x50);
	} else if (pwm_id < 8) {
		nvt_pwm_masked_write(29, 16, div, cg_base + 0x50);
	} else if (pwm_id == 8) {
		nvt_pwm_masked_write(13, 0, div, cg_base + 0x54);
	} else if (pwm_id == 9) {
		nvt_pwm_masked_write(29, 16, div, cg_base + 0x54);
	} else if (pwm_id == 10) {
		nvt_pwm_masked_write(13, 0, div, cg_base + 0x58);
	} else if (pwm_id == 11) {
		nvt_pwm_masked_write(29, 16, div, cg_base + 0x58);
	} else {
		printf("nvt_pwm_set_div: invalid pwm_id%d\r\n", pwm_id);
	}
}

void pwm_enable_clk(int pwm_id)
{
	/* Enable clk */
	if (pwm_id < 12) {
		void __iomem *cg_base = (void __iomem *)IOADDR_CG_REG_BASE;
		nvt_pwm_masked_write(pwm_id, pwm_id, 1, cg_base + 0x7C);
		nvt_pwm_masked_write(8, 8, 1, cg_base + 0x88);
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