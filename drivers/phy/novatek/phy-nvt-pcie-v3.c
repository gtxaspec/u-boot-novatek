// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Novatek
 */

#include <common.h>
#include <dm.h>
#include <generic-phy.h>
#include <asm/io.h>
#include <asm/arch/efuse_protected.h>
#include <linux/bitops.h>

#define DRV_VERSION                     "1.00.06"

enum nvt_pcie_phy_id {
	NVT_PHY_PCIE0 = 0,
	NVT_PHY_PCIE1 = 1,
	NVT_PHY_PCIE2 = 2,
	NVT_PHY_PCIE3 = 3,
	MAX_NUM_PHYS,
};

enum nvt_pcie_phy_mode {
	NVT_PHY_ONE_x4 = 0,
	NVT_PHY_TWO_x2 = 1,
	NVT_PHY_FOUR_x1 = 2,
	NVT_PHY_ONE_x2_TWO_x1 = 3,
	MAX_NUM_PHY_MODE,
};

#define TOP_BS_REG_OFS			0x00
#define TOP_BS_PCIE_LANE_MODE_MSK	(0x3<<14)
#define TOP_BS_PCIE_LANE_MODE_ONE_X4	(0)
#define TOP_BS_PCIE_LANE_MODE_TWO_X2	(1<<14)
#define TOP_BS_PCIE_LANE_MODE_FOUR_X1	(2<<14)
#define TOP_BS_PCIE_LANE_MODE_ONE_X2_TWO_X1	(3<<14)
#define TOP_BS_PCIE_REFCLK_MSK		(1<<16)


/* we have up to 8 PAXB based RC. The 9th one is always PAXC */
#define SR_NR_PCIE_PHYS               8

struct nvt_pcie_phy {
	struct nvt_pcie_phy_core *core;
	enum nvt_pcie_phy_id id;
//	struct phy *phy;

	u32 is_rint;	// 0: use rext, 1: use rint
	u32 rint;
	u32 tx_odt_p;
	u32 tx_odt_n;
	u32 tx_odt_s;

	u32 is_dcc_valid;
	u32 dcc_dec;
	u32 dcc_inc;
};

/**
 * struct nvt_pcie_phy_core - Novatek PCIe PHY core control
 *
 * @dev: pointer to device
 * @base: base register of PCIe SS
 * @cdru: CDRU base address
 * @pipemux: pipemuex strap
 */
struct nvt_pcie_phy_core {
	struct udevice *dev;
	void __iomem *v_base[MAX_NUM_PHYS];

	int is_phy_aggressive;
	enum nvt_pcie_phy_mode mode;
	struct nvt_pcie_phy phys[MAX_NUM_PHYS];
};

static void rc_apbm_srmw(void __iomem *addr, u32 data, u32 start, u32 width) {
	const u32 MASK = (1<<width) - 1;
	u32 data_tmp;

	data_tmp = readl(addr);
	data_tmp = (data_tmp & (~(MASK << start))) | ((data & MASK) << start);
	writel(data_tmp, addr);
}

static int pcie_phy_init(struct phy *phy)
{
	struct nvt_pcie_phy_core *core;
	int idx;

	if (phy == NULL) {
		printf("%s: input phy NULL\r\n", __func__);
		return -1;
	}

	if (phy->dev == NULL) {
		printf("%s: phy->dev NULL\r\n", __func__);
		return -1;
	}

	core = dev_get_priv(phy->dev);
	if (core == NULL) {
		printf("%s: phy core NULL\r\n", __func__);
		return -1;
	}

#if (!IS_ENABLED(CONFIG_NVT_FPGA_EMULATION))
	// realchip configuration
	if (core->phys[NVT_PHY_PCIE0].is_rint) {
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0]+0x5064, 0, 5, 1);	// CMM_CTRL_IREXT = 1'b1
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0]+0x5004, 0, 0, 1);	// 0: RINT, 1: REXT

		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0]+0x5004,
			core->phys[NVT_PHY_PCIE0].rint & 0x1f,
			1, 5); // RINT
	} else {
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0]+0x5064, 1, 5, 1);	// CMM_CTRL_IREXT = 1'b1
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0]+0x5004, 1, 0, 1);	// 0: RINT, 1: REXT
	}

	for(idx=0;idx<MAX_NUM_PHYS;idx++){
		//
		// Lane idx ODT Trim
		//
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0008,
			core->phys[NVT_PHY_PCIE0+idx].tx_odt_p & 0xf,
			8, 4); // TX_ODT_P
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0008,
			core->phys[NVT_PHY_PCIE0+idx].tx_odt_n & 0xf,
			4, 4); // TX_ODT_N

		//
		// Lane idx DCC ODC
		//
		if (core->phys[NVT_PHY_PCIE0].is_dcc_valid) {
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0814, 0, 4, 1);	// trim mode

			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0814, core->phys[NVT_PHY_PCIE0+idx].dcc_dec, 15, 5);
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0814, core->phys[NVT_PHY_PCIE0+idx].dcc_inc, 10, 5);
		}
	}

#if 1	// 635
	for(idx=0;idx<MAX_NUM_PHYS;idx++){
		//
		// DPMA block
		//
#if 0
		if (core.is_phy_aggressive) {
			// TX DCC/RX DCC/RX QEC @gen2/gen3 will re-cal
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x918, 0x1, 20, 1);	// tx_dcc_hw_rstb_en
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x918, 0x1, 21, 1);	// rx_dcc_hw_rstb_en
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1218, 0x1, 9, 1);	// cfg_8g_txdcc_cal
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1218, 0x1, 10, 1);	// cfg_5g_txdcc_cal
		}
#endif

		// bypass clk align function
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x968, 0x0, 28, 1);	// reg_clkalign_en
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x968, 0x1, 31, 1);	// reg_clkalign_manu_done
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x106C, 0x1, 1, 1);	// cfg_bypass_edgecal

		// tx_cal_done delay for un-deglitch mux clkgen
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x918, 0x1, 26, 1);	// tx_cal_done_delay_sel

		// rx_cal_done delay for un-deglitch mux clkgen @LTSSM - L1
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x918, 0x1, 29, 1);	// rx_cal_done_delay_sel

		// RXPUB PD sel
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x85c, 0x1, 17, 1);	// cfg_rxpd_sel_rxpub

		if (core->is_phy_aggressive) {
			// bypass PLL mode
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x950, 0x1, 23, 1);	// cfg_bypass_PLL_mode_en
		}

		// DPHY QEC approach
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x124, 0x1, 22, 1);	// cfg_set_v1_design

		// RX DCO coarse tune as TXDCC
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x85C, 0x3, 20, 2);	// RX_DCC_DCOCT

		// RX DCO compute approach
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x124, 0x0, 16, 3);	// bypass_dco_phase0

		if (core->is_phy_aggressive) {
			// gen3/2/1 TXDCC/RXDCC/RXQEC calibration flow in POR
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1098, 0x1, 0, 1);	// cfg_cal_gen123_bootup

			// RXDCC output median
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x100, 0x1, 29, 1);	// cfg_rxdcc_median_mode_en

			// RXDCC output sample upper threshold
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x100, 0x9, 25, 4);	// cfg_rxdcc_odc_sample_th

			// TXDCC output median
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x814, 0x1, 26, 1);	// cfg_txdcc_median_mode_en

			// TXDCC output sample upper threshold
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x814, 0x9, 27, 4);	// cfg_txdcc_dca_sample_th

			// RXQEC output median
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x114, 0x1, 31, 1);	// cfg_rxqec_median_mode_en

			// RXQEC output sample upper threshold
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0xf8, 0x9, 18, 4);	// cfg_rxqec_oqec_sample_th

			// system calibration rate cycle lower bound
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1098, 0x15, 1, 15);	// cfg_sys_cnt_rate_change_th

			// rx calibration rate cycle lower bound
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1098, 0x15, 16, 10);	// cfg_rx_cnt_rate_change_th

			// EN_RX_EOM = 0 @normal mode for power saving
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x908, 1, 4, 1);	// EN_RX_EOM
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x928, 1, 4, 1);
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0010, 0, 0, 1);
		}

		// RXDCC max counter
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x948, 0x1FFFF, 0, 17);	// cfg_rxdcc_manual_cnt, cfg_rxdcc_manual_cnt_en

		// TXDCC max counter
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x944, 0xFFFF, 8, 16);	// num_dcc_max_reg, num_dcc_max_sw

		// RXQEC max counter
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x108, 0x7, 0, 3);	// cfg_rxqec_refn

		//
		// PCS block
		//

		// TX_EN_HIZIDLE=0 @ all mode for RA
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0B68, 0x00, 3, 1);	// tx_en_hizidle_sel

		// vref = 3 for TXDETRX fail issue
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x081C, 0x03, 24, 2);	// TX_CTL_VREF_DET

		switch (core->mode) {
		case NVT_PHY_ONE_x4:
			switch (idx) {
			case NVT_PHY_PCIE0:
			default:
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x00D0, 0x78, 0, 9); // CT 8G initial value for lane0 only
				break;
			case NVT_PHY_PCIE1:
			case NVT_PHY_PCIE3:
			case NVT_PHY_PCIE2:
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0844, 0x1, 2, 1); //5G/8G recal AFC for lane1/2/3 only
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0844, 0x1, 4, 1); //5G/8G recal AFC for lane1/2/3 only
				break;
			}
			break;
		case NVT_PHY_TWO_x2:
			switch (idx) {
			case NVT_PHY_PCIE0:
			case NVT_PHY_PCIE2:
			default:
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x00D0, 0x78, 0, 9); // CT 8G initial value for lane0/2 only
				break;
			case NVT_PHY_PCIE1:
			case NVT_PHY_PCIE3:
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0844, 0x1, 2, 1); //5G/8G recal AFC for lane1/3 only
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0844, 0x1, 4, 1); //5G/8G recal AFC for lane1/3 only
				break;
			}
			break;
		case NVT_PHY_ONE_x2_TWO_x1:
			switch (idx) {
			case NVT_PHY_PCIE0:
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x00D0, 0x78, 0, 9); // CT 8G initial value for lane0 only
				break;
			case NVT_PHY_PCIE1:
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0844, 0x1, 2, 1); //5G/8G recal AFC for lane1 only
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0844, 0x1, 4, 1); //5G/8G recal AFC for lane1 only
				break;
			case NVT_PHY_PCIE2:
			case NVT_PHY_PCIE3:
			default:
				break;
			}
			break;
		case NVT_PHY_FOUR_x1:
		default:
			break;
		}

		//Gen3 avoid missing code & reduce KP to prevent dead zone and misssing code error.
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x0854, 0x3, 10, 4);   // reduce PLL mode KP = 0x3 (default 0x8)
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x094C, 0x1, 24, 1);   // Gen3 TC init decrease 100 code(default 0x200)
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x094C, 0x19C, 12, 10);// Gen3 TC init decrease 100 code(default 0x200)

		// TX EIOS align TX_ELECIDLE
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x874, 0x3, 0, 2);
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1088, 0x0, 24, 3);
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x3000, 0x1, 2, 2);

		// lane0/1/2/3 update SKIP at the same time (2lane/4lane mode only)
		switch (core->mode) {
		case NVT_PHY_FOUR_x1:
			break;
		case NVT_PHY_ONE_x2_TWO_x1:
			if ((idx==NVT_PHY_PCIE2) || (idx==NVT_PHY_PCIE3)) {
				// When x2*1 and x1*2 mode, only x2 (lane0/1) need to be set
				break;
			}
			fallthrough;
		default:
			// x2/x4
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x3010, 0x0, 8, 1);	// add/remove skip for L0/L1
			break;
		}

		// calibration at the same time
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1068, 0x0, 30, 1);	// cfg_cal_separate_en

		if (core->is_phy_aggressive) {
			// Gen3 RXELCIDLE 1T pulse inhibit
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x3028, 0x1, 0, 1);	// cfg_rx_eidle_dly_en

			// fix gen3 FIFO decode err
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x3028, 0x1, 1, 1);	// cfg_gen3_dec_err_when_fifo_end_en

			// Gen3 SKP add/rmv flag
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x3028, 0x1, 2, 1);	// cfg_gen3_skpaddrmv_end_en
		}

		// L0s shrink rxalg timer
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1210, 0x1, 17, 1);	// cfg_l0s_tm_sel

		// return to L0 after decode lock
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x3020, 0x1, 18, 1);	// cfg_dec128b_algn_buf_clr_sel

		if (core->is_phy_aggressive) {
			// pclk meet spec in LTSSM-L2
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x3038, 0x1, 0, 1);	// cfg_p2_pclk_sel

			// enable RX_DCO @gen2/gen3 re-cal RXDCC
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x138C, 0x1, 26, 1);	// cfg_recal_en_rx_dco_sel
		}

		// lane0/1/2/3 wait mutual decode (for 2lane/4lane mode only)
		switch (core->mode) {
		case NVT_PHY_FOUR_x1:
			break;
		case NVT_PHY_ONE_x2_TWO_x1:
			if ((idx==NVT_PHY_PCIE2) || (idx==NVT_PHY_PCIE3)) {
				// When x2*1 and x1*2 mode, only x2 (lane0/1) need to be set
				break;
			}
		default:
			// x2/x4
			// if enabled, can't support broken lane
//			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x3020, 0x1, 10, 1);	// reg_rx_valid_2lane
			break;
		}

		// Bypass cal_rx_ofc & cal_rx_ofc2 when ofc/ofc2 are done
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x135c, 0x1, 28, 1);	// cfg_ofcsig_clean

		// phy tx compliance by controller
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x3020, 0x1, 14, 1);	// reg_TxCmpl_sel

		// fix 690 txdetrx fail CDC issue
		rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1360, 0x1, 23, 1);	// cfg_txdetrx_eco

		if (core->is_phy_aggressive) {
			// *LTSSM-L2 : enable EN_TXRX option
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1068, 0x1, 8, 1);	// cfg_p2_pd_sm

			// *LTSSM-L2
			switch (core->mode) {
			case NVT_PHY_ONE_x4:
			case NVT_PHY_TWO_x2:
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x138c, 0x1F7BF, 0, 18);	// cfg_p2_pd_sm
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x3028, 0x1, 4, 1);
				break;
			case NVT_PHY_ONE_x2_TWO_x1:
				switch (idx) {
				case NVT_PHY_PCIE0:
				case NVT_PHY_PCIE1:
//				case NVT_PHY_PCIE3:
					rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x138c, 0x1F7BF, 0, 18);	// cfg_p2_pd_sm
					break;
				case NVT_PHY_PCIE3:
					rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x138c, 0x017BF, 0, 18);	// cfg_p2_pd_sm
					break;
				case NVT_PHY_PCIE2:
				default:
					rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x138c, 0x017BE, 0, 18);	// cfg_p2_pd_sm
					break;
				}
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x3028, 0x1, 4, 1);
				break;
			case NVT_PHY_FOUR_x1:
			default:
				switch (idx) {
				case NVT_PHY_PCIE0:
				case NVT_PHY_PCIE2:
					rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x138c, 0x017BE, 0, 18);	// cfg_p2_pd_sm
					break;
				case NVT_PHY_PCIE1:
				case NVT_PHY_PCIE3:
				default:
					rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x138c, 0x017BF, 0, 18);	// cfg_p2_pd_sm
					break;
				}
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x3028, 0x1, 4, 1);
				break;
			}

			// *LTSSM-L1 : enable EN_TXRX option
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x135c, 0x1, 0, 1);	// cfg_p1_pd_sm

			// *LTSSM-L1 : delay for diable TXRX clock
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1388, 0x20, 0, 8);	// cfg_p1_cnt
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1388, 0x1, 8, 1);	// cfg_p1_txclkoff
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1388, 0x1, 9, 1);	// cfg_p1_rxclkoff

			// *LTSSM-L1 : EN_TX_HIZIDLE = 1
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x135c, 0x30, 1, 12);	// cfg_p1_pd_sw

			// *LTSSM-L1 : TX_RATE_RSTN = 0 + TX_RSTN = 0 RX_RATE_RSTN = 0 + RX_RSTN = 0 + RX_CDR_EOM_DIV45_RSTN = 0
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x1388, 0x1, 31, 1);	// cfg_tx_rate_rstn_p1_pd
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x138c, 0x1, 24, 1);	// cfg_tx_rstn_p1_pd
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x138c, 0x1, 20, 1);	// cfg_rx_rate_rstn_p1_pd
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x138c, 0x1, 21, 1);	// cfg_rx_rstn_p1_pd
			rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x138c, 0x1, 22, 1);	// cfg_rx_rsv1_p1_pd

			// *LTSSM-L1 : EN_TXPUB_8GBUF = 0 for 2lane/4lane mode only
			switch (core->mode) {
			case NVT_PHY_FOUR_x1:
				break;
			case NVT_PHY_ONE_x2_TWO_x1:
				if ((idx==NVT_PHY_PCIE2) || (idx==NVT_PHY_PCIE3)) {
					// When x2*1 and x1*2 mode, only x2 (lane0/1) need to be set
					break;
				}
				fallthrough;
			default:
				// x2/x4
				rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x138c, 0x1, 25, 1);	// cfg_tx_8gbuf_p1_pd
				break;
			}
		}

		//Fix the issue of 1-lane/2-lane mode failing to link up due to the absence of a reference clock.
		switch (core->mode) {
		case NVT_PHY_ONE_x2_TWO_x1:
		case NVT_PHY_TWO_x2:
		case NVT_PHY_FOUR_x1:
			switch (idx) {
				case NVT_PHY_PCIE0:
					rc_apbm_srmw(core->v_base[NVT_PHY_PCIE0+idx]+0x41d4, 0x3, 1, 2);     // The refclk of CMM0 can be routed to CMM1/3 1lane/2lane mode only
					break;
				case NVT_PHY_PCIE1:
				case NVT_PHY_PCIE3:
				case NVT_PHY_PCIE2:
				default:
					break;
				}

			break;
		case NVT_PHY_ONE_x4:
		default:
			break;
		}

	}
#endif	// endif of 635

#else
	printf("%s: FPGA\r\n", __func__);

	// FPGA configuration
	for (idx=0; idx<MAX_NUM_PHYS; idx++) {
		rc_apbm_srmw(core->v_base[idx]+0x10fc, 0x7d0, 0, 11); //dco simulation

		// speed up RX_CAL
		rc_apbm_srmw(core->v_base[idx]+0x107c, 0x50, 0, 14);	// CDR timer
		rc_apbm_srmw(core->v_base[idx]+0x1080, 0x50, 0, 14); // speed up RX_CAL
		rc_apbm_srmw(core->v_base[idx]+0x1080, 0x50, 16, 14); // speed up RX_CAL
		rc_apbm_srmw(core->v_base[idx]+0x920,  0x300, 0, 16); // speed up RX_CAL

		rc_apbm_srmw(core->v_base[idx]+0x1208,  0x100, 0, 16); // bypass rate_rst = 1, avoid AFC reset when rate change
		rc_apbm_srmw(core->v_base[idx]+0x1348,  0x0, 0, 1); // phase calibration org method
		rc_apbm_srmw(core->v_base[idx]+0x1210,  0x1, 8, 1); // test ctle off at phase calibration
		rc_apbm_srmw(core->v_base[idx]+0x3020,  0x1, 11, 1); // for 2lane cfg_lanenum change
		rc_apbm_srmw(core->v_base[idx]+0x1210,  0x1, 4, 1); // bypass phase calibration
		rc_apbm_srmw(core->v_base[idx]+0x1034,  0x1, 8, 1); // enable FOM

		//RX QEC/DCC manual mode
		rc_apbm_srmw(core->v_base[idx]+0x108,  0x0, 16, 1);
		rc_apbm_srmw(core->v_base[idx]+0x104,  0x0, 0, 1);
		rc_apbm_srmw(core->v_base[idx]+0x28,   0x0, 0, 1);
		rc_apbm_srmw(core->v_base[idx]+0x2c,   0x0, 5, 1);
		rc_apbm_srmw(core->v_base[idx]+0x2c,   0x0, 13, 1);
		rc_apbm_srmw(core->v_base[idx]+0x10,   0x0, 0, 1);
		rc_apbm_srmw(core->v_base[idx]+0x18,   0x0, 0, 1);
		rc_apbm_srmw(core->v_base[idx]+0x20,   0x0, 0, 1);
		rc_apbm_srmw(core->v_base[idx]+0x0,    0x0, 0, 1);
		rc_apbm_srmw(core->v_base[idx]+0x8,    0x0, 0, 1);

#if 1
		// TX DCC
		rc_apbm_srmw(core->v_base[idx]+0x814,  0x0, 4, 1);
		rc_apbm_srmw(core->v_base[idx]+0x1218, 0x3, 9, 2);
		// RX reset bypass to determine pt_en
		rc_apbm_srmw(core->v_base[idx]+0x1070, 0x1, 8, 1);
#endif

		// seperate calibration
		rc_apbm_srmw(core->v_base[idx]+0x1068, 0x0, 30, 1); //2lane phy

#if 1
		// clkrstgen tx_cal_done delay for un-deglitch mux
		rc_apbm_srmw(core->v_base[idx]+0x0918, 0x1, 26, 1);
		// NT98690 gen3 FIFO issue
		rc_apbm_srmw(core->v_base[idx]+0x3028, 0x1, 1, 1);
		rc_apbm_srmw(core->v_base[idx]+0x3028, 0x1, 2, 1);
#endif

		rc_apbm_srmw(core->v_base[idx]+0x1210, 0x1, 17, 1); //L0s bug

#if 1
		// reg_pcie_latch sel
		rc_apbm_srmw(core->v_base[idx]+0x3010, 0x1, 14, 1);
		// txampboost
		rc_apbm_srmw(core->v_base[idx]+0x0950, 0x1, 20, 1);
		// Dec128b align buffer clear
		rc_apbm_srmw(core->v_base[idx]+0x3020, 0x1, 18, 1);
		// bypass edge calibration
		rc_apbm_srmw(core->v_base[idx]+0x106C, 0x1, 1, 1);
		// L1 best setting
		rc_apbm_srmw(core->v_base[idx]+0x1388, 0x20, 0, 8);
		rc_apbm_srmw(core->v_base[idx]+0x135C, 0x1, 0, 1);
		// clkrstgen rx_cal_done delay for un-deglitch mux
		rc_apbm_srmw(core->v_base[idx]+0x0918, 0x1, 29, 1);
		// RXPUB PD selection
		rc_apbm_srmw(core->v_base[idx]+0x085C, 0x1, 17, 1);
		// TX EIOS align TX_ELECIDLE
		rc_apbm_srmw(core->v_base[idx]+0x1088, 0x0, 24, 3);
		rc_apbm_srmw(core->v_base[idx]+0x3000, 0x1, 2, 2);
		rc_apbm_srmw(core->v_base[idx]+0x0874, 0x1, 1, 1);
#endif

		// bypass clock align mode
		rc_apbm_srmw(core->v_base[idx]+0x0968, 0x0, 28, 1);
		rc_apbm_srmw(core->v_base[idx]+0x0968, 0x1, 31, 1);

#if 1
		// New TXDCC/RXDCC/RXQEC calibration flow
		rc_apbm_srmw(core->v_base[idx]+0x1098, 0x1, 0, 1);
		rc_apbm_srmw(core->v_base[idx]+0x0100, 0x1, 29, 1);
		rc_apbm_srmw(core->v_base[idx]+0x0100, 0x9, 25, 4);
		rc_apbm_srmw(core->v_base[idx]+0x0814, 0x1, 26, 1);
		rc_apbm_srmw(core->v_base[idx]+0x0814, 0x9, 27, 4);
		rc_apbm_srmw(core->v_base[idx]+0x0114, 0x1, 31, 1);
		rc_apbm_srmw(core->v_base[idx]+0x00F8, 0x9, 18, 4);
		rc_apbm_srmw(core->v_base[idx]+0x1098, 0x15, 1, 15);
		rc_apbm_srmw(core->v_base[idx]+0x1098, 0x15, 16, 10);

		// CMM manual lock
		if (idx != 2) {	// bypass lane2
			rc_apbm_srmw(core->v_base[idx]+0x4140, 0x1, 4, 1);
			rc_apbm_srmw(core->v_base[idx]+0x401C, 0x1, 3, 1);
			rc_apbm_srmw(core->v_base[idx]+0x401C, 0x1, 2, 1);
		}
#endif

		rc_apbm_srmw(core->v_base[idx]+0x1084, 0xb, 16, 6); //timer 0xb

		rc_apbm_srmw(core->v_base[idx]+0x938, 0x1, 6, 1); //L2 Beacon

		rc_apbm_srmw(core->v_base[idx]+0x020C, 0x1, 20, 1); //L2 gen1/2 beacon fail
		rc_apbm_srmw(core->v_base[idx]+0x020C, 0x1, 31, 1); //L2 gen1/2 rx idle fail

		rc_apbm_srmw(core->v_base[idx]+0x020C, 0x1, 24, 1); //Use I/O elecidle

	}
#endif	// endif of !FPGA

	return 0;
}

static int pcie_phy_xlate(struct phy *phy, struct ofnode_phandle_args *args)
{
	debug("%s %d\n", __func__, args->args[0]);
	if (args->args_count && args->args[0] < SR_NR_PCIE_PHYS)
		phy->id = args->args[0];
	else
		return -ENODEV;

	return 0;
}

static int pcie_phy_configure(struct phy *phy, void *params)
{
	if (params == NULL) {
		pr_err("%s: params is NULL\n", __func__);
		return -ENODEV;
	}

	// after NT98635, core->mode is set to bootstrap setting when phy is probe
	// so just do nothing

	return 0;
}

static const struct phy_ops pcie_phy_ops = {
	.of_xlate = pcie_phy_xlate,
	.init = pcie_phy_init,
	.configure = pcie_phy_configure,
};

static int load_efuse_trim(struct nvt_pcie_phy_core *core)
{
	u32 trim = 0;
	u32 addr;
	int code;
	int is_found = 0;
	int idx;

	if (core == NULL) {
		pr_err("core NULL\n");
		return -1;
	}

	// PCIe
	addr = EFUSE_TRIM_PCIe0_ODT_PRI;	// RINT
	code = otp_key_manager(EFUSE_ID_1, addr);
	if (code == -33) {
		//!!!Please apply default value here!!!
		printf("Read PCIe RINT trim error\r\n");
		core->phys[NVT_PHY_PCIE0].is_rint = 0;
	} else {
		is_found = extract_trim_valid(code, (u32 *)&trim);
		if (is_found && (trim & (1<<13))) {
			printf("PCIe RINT Trim data read success addr[0x%02x] = 0x%04x\r\n", (int)addr, (int)trim);
			core->phys[NVT_PHY_PCIE0].is_rint = 1;
			core->phys[NVT_PHY_PCIE0].rint = (trim>>8) & 0x1F;
			// clamp at 0x7~0x19
			if (core->phys[NVT_PHY_PCIE0].rint < 0x7) core->phys[NVT_PHY_PCIE0].rint = 0x7;
			if (core->phys[NVT_PHY_PCIE0].rint > 0x19) core->phys[NVT_PHY_PCIE0].rint = 0x19;
		} else {
			//!!!Please apply default value here!!!
			pr_err("PCIe Trim data raw(0x%x) = NULL\n", addr);
			printf("is found [%d][PCIe RINT] = 0x%08x\r\n", is_found, (int)code);
			core->phys[NVT_PHY_PCIE0].is_rint = 0;
		}
	}

	addr = EFUSE_TRIM_PCIe1_ODT_PRI;	// if use aggressive setting
	code = otp_key_manager(EFUSE_ID_1, addr);
	if (code == -33) {
		//!!!Please apply default value here!!!
		printf("Read PCIe Aggressive trim error\r\n");
		core->is_phy_aggressive = 0;
	} else {
		is_found = extract_trim_valid(code, (u32 *)&trim);
		if (is_found && (trim & (1<<13))) {
			printf("PCIe Aggressive trim read success addr[0x%02x] = 0x%04x\r\n", (int)addr, (int)trim);
			core->is_phy_aggressive = 1;
		} else {
			//!!!Please apply default value here!!!
			pr_err("PCIe Trim data raw(0x%x) = NULL\n", addr);
			printf("is found [%d][Aggresive] = 0x%08x\r\n", is_found, (int)code);
			core->is_phy_aggressive = 0;
		}
	}

	addr = EFUSE_TRIM_PCIE_PHY0_ODC_PRI;
	code = otp_key_manager(EFUSE_ID_1, addr);
	core->phys[0].is_dcc_valid = 0;
	if (code == -33) {
		//!!!Please apply default value here!!!
		printf("Read PCIe DCC trim error\r\n");
		core->phys[NVT_PHY_PCIE0].is_dcc_valid = 0;
	} else {
		is_found = extract_trim_valid(code, (u32 *)&trim);
		if (is_found && (trim & (1<<13))) {
			printf("PCIe DCC Trim data read success addr[0x%02x] = 0x%04x\r\n", (int)addr, (int)trim);
			core->phys[0].is_dcc_valid = 1;
		} else {
			pr_info("   PCIe DCC disabled: addr[%02x] 0x%04x\r\n", (int)addr, (int)trim);
			core->phys[0].is_dcc_valid = 0;
		}
	}

	for (idx=0; idx<MAX_NUM_PHYS; idx++) {
		core->phys[NVT_PHY_PCIE0+idx].id = idx;

		// ODT trim
		switch (idx) {
		case NVT_PHY_PCIE0:
			addr = EFUSE_TRIM_PCIe0_ODT_PRI;
			break;
		case NVT_PHY_PCIE1:
			addr = EFUSE_TRIM_PCIe1_ODT_PRI;
			break;
		case NVT_PHY_PCIE2:
			addr = EFUSE_TRIM_PCIe2_ODT_PRI;
			break;
		case NVT_PHY_PCIE3:
		default:
			addr = EFUSE_TRIM_PCIe3_ODT_PRI;
			break;
		}
		code = otp_key_manager(EFUSE_ID_1, addr);
		if (code == -33) {
			//!!!Please apply default value here!!!
			printf("Read PCIe lane%d ODT trim error\r\n", idx);
			core->phys[NVT_PHY_PCIE0+idx].tx_odt_p = 0x9;
			core->phys[NVT_PHY_PCIE0+idx].tx_odt_n = 0xB;
			core->phys[NVT_PHY_PCIE0+idx].tx_odt_s = 0x3;
		} else {
			is_found = extract_trim_valid(code, (u32 *)&trim);
			if (is_found) {
				printf("PCIe lane%d ODT Trim data read success addr[0x%02x] = 0x%04x\r\n", idx, (int)addr, (int)trim);
				core->phys[NVT_PHY_PCIE0+idx].tx_odt_p = (trim>>0)&0x0F;
				// clamp at 0x4~0xE
				if (core->phys[NVT_PHY_PCIE0+idx].tx_odt_p < 0x4) core->phys[NVT_PHY_PCIE0+idx].tx_odt_p = 0x4;
				if (core->phys[NVT_PHY_PCIE0+idx].tx_odt_p > 0xE) core->phys[NVT_PHY_PCIE0+idx].tx_odt_p = 0xE;

				core->phys[NVT_PHY_PCIE0+idx].tx_odt_n = (trim>>4)&0x0F;
				// clamp at 0x4~0xE
				if (core->phys[NVT_PHY_PCIE0+idx].tx_odt_n < 0x4) core->phys[NVT_PHY_PCIE0+idx].tx_odt_n = 0x4;
				if (core->phys[NVT_PHY_PCIE0+idx].tx_odt_n > 0xE) core->phys[NVT_PHY_PCIE0+idx].tx_odt_n = 0xE;

				core->phys[NVT_PHY_PCIE0+idx].tx_odt_s = (trim>>8)&0x0F;
				// clamp at 0x4~0xE
				if (core->phys[NVT_PHY_PCIE0+idx].tx_odt_s < 0x4) core->phys[NVT_PHY_PCIE0+idx].tx_odt_s = 0x4;
				if (core->phys[NVT_PHY_PCIE0+idx].tx_odt_s > 0xE) core->phys[NVT_PHY_PCIE0+idx].tx_odt_s = 0xE;
			} else {
				pr_err("PCIe Trim data raw(0x%x) = NULL\n", addr);
				printf("is found [%d][PCIe lane%d ODT] = 0x%08x\r\n", is_found, idx, (int)code);
				core->phys[NVT_PHY_PCIE0+idx].tx_odt_p = 0x9;
				core->phys[NVT_PHY_PCIE0+idx].tx_odt_n = 0xB;
				core->phys[NVT_PHY_PCIE0+idx].tx_odt_s = 0x3;
			}
		}

		// DCC ODC trim
		if (core->phys[0].is_dcc_valid) {
			addr = EFUSE_TRIM_PCIE_PHY0_ODC_PRI + idx;
			code = otp_key_manager(EFUSE_ID_1, addr);
			if (code == -33) {
				//!!!Please apply default value here!!!
				printf("Read PCIe lane%d DCC trim error\r\n", idx);
			} else {
				is_found = extract_trim_valid(code, (u32 *)&trim);
				if (is_found) {
					printf("PCIe lane%d DCC Trim data read success addr[0x%02x] = 0x%04x\r\n", idx, (int)addr, (int)trim);
					// DEC [4..0]
					core->phys[idx].dcc_dec = (trim>>0) & 0x1F;
					// INC [9..5]
					core->phys[idx].dcc_inc = (trim>>5) & 0x1F;
				} else {
					pr_err("PCIe Trim data raw(0x%x) = NULL\n", addr);
				}
			}
		}
	}

	return 0;
};

static int nvt_pcie_phy_probe(struct udevice *dev)
{
	struct nvt_pcie_phy_core *core;
	int ret;
	u32 val;

	printf("%s: ver %s\r\n", __func__, DRV_VERSION);
	if (dev == NULL) {
		pr_err("NULL dev\r\n");
		return -EINVAL;
	}

	core = dev_get_priv(dev);
	if (core == NULL) {
		pr_err("NULL priv\r\n");
		return -EINVAL;
	}

	core->dev = dev;

	core->v_base[NVT_PHY_PCIE0] = (void __iomem *)devfdt_get_addr_index(dev, 0);
	if (((fdt_addr_t)core->v_base[NVT_PHY_PCIE0] == FDT_ADDR_T_NONE) ||
		(core->v_base[NVT_PHY_PCIE0] == NULL)) {
		pr_err("PHY0 reg not found in dts\r\n");
		return -EINVAL;
	}
	core->v_base[NVT_PHY_PCIE1] = (void __iomem *)devfdt_get_addr_index(dev, 1);
	if (((fdt_addr_t)core->v_base[NVT_PHY_PCIE1] == FDT_ADDR_T_NONE) ||
		(core->v_base[NVT_PHY_PCIE1] == NULL)) {
		pr_err("PHY1 reg not found in dts\r\n");
		return -EINVAL;
	}
	core->v_base[NVT_PHY_PCIE2] = (void __iomem *)devfdt_get_addr_index(dev, 2);
	if (((fdt_addr_t)core->v_base[NVT_PHY_PCIE2] == FDT_ADDR_T_NONE) ||
		(core->v_base[NVT_PHY_PCIE2] == NULL)) {
		pr_err("PHY2 reg not found in dts\r\n");
		return -EINVAL;
	}
	core->v_base[NVT_PHY_PCIE3] = (void __iomem *)devfdt_get_addr_index(dev, 3);
	if (((fdt_addr_t)core->v_base[NVT_PHY_PCIE3] == FDT_ADDR_T_NONE) ||
		(core->v_base[NVT_PHY_PCIE3] == NULL)) {
		pr_err("PHY3 reg not found in dts\r\n");
		return -EINVAL;
	}
	debug("ip base %p %p %p %p\n", core->v_base[NVT_PHY_PCIE0], core->v_base[NVT_PHY_PCIE1], core->v_base[NVT_PHY_PCIE2], core->v_base[NVT_PHY_PCIE3]);
	printf("ip base %p %p %p %p\n", core->v_base[NVT_PHY_PCIE0], core->v_base[NVT_PHY_PCIE1], core->v_base[NVT_PHY_PCIE2], core->v_base[NVT_PHY_PCIE3]);

	val = readl(IOADDR_TOP_REG_BASE + TOP_BS_REG_OFS);
	switch (val&TOP_BS_PCIE_LANE_MODE_MSK) {
	case TOP_BS_PCIE_LANE_MODE_FOUR_X1:
		core->mode = NVT_PHY_FOUR_x1;
		break;
	case TOP_BS_PCIE_LANE_MODE_ONE_X4:
		core->mode = NVT_PHY_ONE_x4;
		break;
	case TOP_BS_PCIE_LANE_MODE_TWO_X2:
		core->mode = NVT_PHY_TWO_x2;
		break;
	case TOP_BS_PCIE_LANE_MODE_ONE_X2_TWO_X1:
	default:
		core->mode = NVT_PHY_ONE_x2_TWO_x1;
		break;
	};

	core->is_phy_aggressive = 0;
	ret = load_efuse_trim(core);
	if (ret != 0) return ret;

	pr_info("Novatek PCIe PHY driver initialized\n");

	return 0;
}

static const struct udevice_id pcie_phy_match_table[] = {
	{ .compatible = "nvt,pcie-v3-phy" },
	{ }
};

U_BOOT_DRIVER(nvt_pcie_phy) = {
	.name = "nvt-pcie-phy",
	.id = UCLASS_PHY,
	.probe = nvt_pcie_phy_probe,
	.of_match = pcie_phy_match_table,
	.ops = &pcie_phy_ops,
	.plat_auto	= sizeof(struct nvt_pcie_phy_core),
	.priv_auto	= sizeof(struct nvt_pcie_phy_core),
};

