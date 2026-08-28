/*
    HDMI 2.1 module internal defintion header

    HDMI 2.1 module internal defintion header.

    @file       hdmitx2p1_int.h
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2024.  All rights reserved.
*/

#ifndef _HDMITX2P1_INT_PHY_H
#define _HDMITX2P1_INT_PHY_H





//******************************************************************************
// P L A T F O R M  D E F I N E
//******************************************************************************



//******************************************************************************
// D A T A  S T R U C T U R E
//******************************************************************************
enum  FREQ_BAND {
	FREQ_BAND_0_148,
	FREQ_BAND_148_340,
	FREQ_BAND_340_594,
	FREQ_BAND_OTHER_SETTING,
};


//******************************************************************************
// F U N C T I O N    P R O T O T Y P E S (P H Y  P A R T)
//******************************************************************************
void HDMITX_PHY_P2S_ENABLE(int link, BOOL enable);
void HDMITX_PHY_TCLK_Setting(int link, int pll_index);
void HDMITX_PHY_FRLCLK_Setting(int link, int pll_index);
void HDMITX_PHY_PLL1_ENABLE(int ucHDMITxBlk, BOOL enable);
void HDMITX_PHY_PLL2_ENABLE(int ucHDMITxBlk, BOOL enable);
void HDMITX_PHY_LDO_EN(int ucHDMITxBlk, BOOL enable);
void HDMITX_PHY_BIAS_EN(int ucHDMITxBlk, BOOL enable);
void HDMITX_PHY_RES_TRIM(int ucHDMITxBlk, int coarse_value, int L0_fine_value, int L1_fine_value, int L2_fine_value, int L3_fine_value);
void drv_HDMI_Tx_PHY_Load_OTP_Value(int ucHDMITxBlk);
void HDMITX_PHY_TxFFE_Set_byLane_EN(int ucHDMITxBlk, BOOL enable);
void HDMITX_PHY_Termination_byLane(int ucHDMITxBlk, int value);
void HDMITX_PHY_Maincursor(int ucHDMITxBlk, int freq_band,int othervalue);
void HDMITX_PHY_Deemphasis(int ucHDMITxBlk, int value);
void HDMITX_PHY_Regulator_Output_Voltage(int ucHDMITxBlk, int freq_band,int othervalue);
void HDMITX_PHY_Preshoot(int ucHDMITxBlk, int value);
void HDMITX_PHY_MODE_SEL(int ucHDMITxBlk, int ucMode);
void HDMITX_PHY_TOTAL_PLL_CONFIG(int ucHDMITxBlk);
void HDMITX_PHY_TOTAL_PLL_ENABLE(int ucHDMITxBlk);
void HDMITX_PHY_POWER_ON(int ucHDMITxBlk);
void HDMITX_PHY_POWER_DOWN(int ucHDMITxBlk);
void HDMITX_PHY_RESET(int ucHDMITxBlk, BOOL enable);
void HDMITX_PHY_MR_DATA_PERIOD(int ucHDMITxBlk, BOOL b4096_Period);
void HDMITX_PHY_FRLCLK_BYPASS_MODE(int ucHDMITxBlk, int ucInput, BOOL bBYPASS_ENABLE);
void HDMITX_PHY_FRL_CLK_SOURCE(int ucHDMITxBlk, int ucFRL_Rate);
void HDMITX_PHY_FRL_CONFIG(int ucHDMITxBlk, int ucFRL_Rate);
void HDMITX_PHY_PCLK_MPLL_FILTER(int ucHDMITxBlk, int ucInput, BOOL bMpllFilter_en);
void HDMITX_PHY_TMDS_CONFIG(int ucHDMITxBlk, int input_clk);
void HDMITX_PHY_TMDS_Prefered_ChargePump(int ucHDMITxBlk, int color_space, int input_clk);
void HDMITX_PHY_PRE_DRVER(int ucHDMITxBlk, BOOL enable);
void HDMITX_PHY_DCC_BYPASS(int ucHDMITxBlk, BOOL enable);
void HDMITX_PHY_DCC_AUTO_K(int ucHDMITxBlk);

#endif
