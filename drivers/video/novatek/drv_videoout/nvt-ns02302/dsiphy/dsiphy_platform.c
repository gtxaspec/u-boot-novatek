#ifdef __KERNEL__
// #include <linux/clk.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/arch/IOAddress.h>
#include <asm/nvt-common/rcw_macro_bit.h>
#else
#include "pll.h"
#include "pll_protected.h"
#include <kwrap/debug.h>
#endif
// #include <kwrap/type.h>
// #include <kwrap/util.h>
#include "include/dsiphy_platform.h"

#define INREG32(x)          			(*((volatile UINT32*)(x)))
#define OUTREG32(x, y)      			(*((volatile UINT32*)(x)) = (y))    ///< Write 32bits IO register
#define SETREG32(x, y)      			OUTREG32((x), INREG32(x) | (y))     ///< Set 32bits IO register
#define CLRREG32(x, y)      			OUTREG32((x), INREG32(x) & ~(y))    ///< Clear 32bits IO register
#define PLL17_OFFSET					(IOADDR_STBC_CG_REG_BASE + 0x47A0)

#define CSI_TX_PLAT_CLK_NAME_STR        "pll11"
#define CSI_TX_PLAT_PLL_ID              PLL_ID_11


void dsiphy_platform_clk_set_freq(unsigned long ui_config)
{
#if 0
#ifdef __KERNEL__
	struct clk *dsiphy_pll_clk;

	dsiphy_pll_clk = clk_get(NULL, CSI_TX_PLAT_CLK_NAME_STR);
	clk_prepare(dsiphy_pll_clk);
	clk_set_rate(dsiphy_pll_clk, ui_config); //unit: Hz
	clk_enable(dsiphy_pll_clk);
	clk_put(dsiphy_pll_clk);

#else
    UINT64 clock = ui_config;
	pll_setPLLEn(CSI_TX_PLAT_PLL_ID, FALSE);
	pll_setPLL(CSI_TX_PLAT_PLL_ID, (clock * 131072 / 12 / 1000000));
	pll_setPLLEn(CSI_TX_PLAT_PLL_ID, TRUE);
#endif
#endif
	REGVALUE reg_data;

	reg_data = ((ui_config / 12000000) * 131072);

	OUTREG32(PLL17_OFFSET, reg_data & 0xFF);
	OUTREG32(PLL17_OFFSET + 0x4, (reg_data >> 8) & 0xFF);
	OUTREG32(PLL17_OFFSET + 0x8, (reg_data >> 16) & 0xFF);
}

unsigned long dsiphy_platform_clk_get_freq(void)
{
#if 0
	UINT32 div;
	unsigned long csi_tx_src_clk;

#ifdef __KERNEL__
	struct clk *dsiphy_pll_clk;

	dsiphy_pll_clk = clk_get(NULL, CSI_TX_PLAT_CLK_NAME_STR);
	clk_prepare(dsiphy_pll_clk);
	csi_tx_src_clk = clk_get_rate(dsiphy_pll_clk);
	div = 1;
	return (csi_tx_src_clk / div);
#else
	csi_tx_src_clk = pll_getPLLFreq(CSI_TX_PLAT_PLL_ID);
	div = 1;
	return (csi_tx_src_clk / div);
#endif
#endif
	return 0;
}
