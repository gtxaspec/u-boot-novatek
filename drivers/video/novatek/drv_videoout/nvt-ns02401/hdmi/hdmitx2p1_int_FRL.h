/*
    HDMI 2.1 module internal defintion header

    HDMI 2.1 module internal defintion header.

    @file       hdmitx2p1_int.h
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2024.  All rights reserved.
*/

#ifndef _HDMITX2P1_INT_FRL_H
#define _HDMITX2P1_INT_FRL_H



//******************************************************************************
// P L A T F O R M  D E F I N E
//******************************************************************************
#define	HDMI21_TX_GET_FSM(_PortID_)							( gstHDMI21_TX_Detect[_PortID_].enHDMI21TXDetect_FSM )
#define	HDMI21_TX_SET_FSM(_PortID_,_FSM_)					( gstHDMI21_TX_Detect[_PortID_].enHDMI21TXDetect_FSM = (_FSM_) )
#define FIX_RATE_TEST	TRUE


//******************************************************************************
// D A T A  S T R U C T U R E
//******************************************************************************

enum  PHY_PLL {
	PHY_PLL1,
	PHY_PLL2,
};



typedef enum _EN_DRV_HDMI_TX_TIMER_TYPE
{
	EN_DRV_HDMI_TX_TIMER_ALL
}EN_DRV_HDMI_TX_TIMER_TYPE;

typedef enum _FSM_HDMI21_TX_DETECT
{
	FSM_HDMI21_TX_DETECT_INIT,
	FSM_HDMI21_TX_DETECT_LTS_1,
	FSM_HDMI21_TX_DETECT_LTS_2,
	FSM_HDMI21_TX_DETECT_LTS_3,
	FSM_HDMI21_TX_DETECT_LTS_4,
	FSM_HDMI21_TX_DETECT_LTS_P,
	FSM_HDMI21_TX_DETECT_LTS_L,
	
	FSM_HDMI21_TX_DETECT_MAX,
}FSM_HDMI21_TX_DETECT;

typedef struct _ST_DRV_HDMI21_TX_DETECT
{
	FSM_HDMI21_TX_DETECT enHDMI21TXDetect_FSM;	// Mode Detect FSM

}ST_DRV_HDMI21_TX_DETECT;

typedef enum _EN_HDMI21_TX_FRL_RATE
{
	FRL_RATE_DISABLE	= 0x0,
	FRL_RATE_3G_3L		= 0x1,
	FRL_RATE_6G_3L		= 0x2,
	FRL_RATE_6G_4L		= 0x3,
	FRL_RATE_8G_4L		= 0x4,
	FRL_RATE_10G_4L		= 0x5,
	FRL_RATE_12G_4L		= 0x6,
	FRL_RATE_0p5G_3L	= 0xA,
	FRL_RATE_0p5G_4L	= 0xB,
}EN_HDMI21_TX_FRL_RATE;

typedef enum _EN_HDMI21_TX_FFE
{
	TxFFE_DISABLE		= 0x0,
	TxFFE_Level1		= 0x1,
	TxFFE_Level2		= 0x2,
	TxFFE_Level3		= 0x3,
}EN_HDMI21_TX_FFE;

typedef enum{		
	TxFFE_NORMAL=0, //Rsvd	
	TxFFE_ST_PRE=1,			
	TxFFE_ST_DE,	
	TxFFE_ST_NO,	
	TxFFE_ST_RSVD,	
	TxFFE_ST_FLT_NOTIMEOUT,	
	TxFFE_ST_FRLMAX,
} EN_HDMI_TxFFE_ST;


//******************************************************************************
// F U N C T I O N    P R O T O T Y P E S (F R L  P A R T)
//******************************************************************************
void DRV_HDMI_TX_Del_Timer(EN_DRV_HDMI_TX_TIMER_TYPE eTimerType);
void DRV_HDMI_TX_Star_Timer(EN_DRV_HDMI_TX_TIMER_TYPE eTimerType);
void DRV_HDMI_TX_Stop_Timer(EN_DRV_HDMI_TX_TIMER_TYPE eTimerType);
void DRV_HDMI_TX_Init_Timer(EN_DRV_HDMI_TX_TIMER_TYPE eTimerType);
void DRV_HDMI21_CHECK_EDID( int u8PortId, char* edid_buf );


#endif
