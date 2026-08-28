/*
    HDMI 2.1 module internal defintion header

    HDMI 2.1 module internal defintion header.

    @file       hdmitx2p1_int.h
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2024.  All rights reserved.
*/

#ifndef _HDMITX2P1_INT_H
#define _HDMITX2P1_INT_H

#include "hdmi_port.h"
/*
#define FALSE 0
#define TRUE 1
#define E_OK  			0
#define E_SYS			-5			///< System error
#define E_FAIL 			E_SYS
#define E_NOSPT         (-17)   	///< Feature not supported
#define E_PAR           (-33)		///< Parameter error
*/








//******************************************************************************
// P L A T F O R M  D E F I N E
//******************************************************************************


#define _Num_HDMI2P1_TX 1
#define _HDMI_TX_All _Num_HDMI2P1_TX
#define TX_TriByte_H 350
#define TX_TriByte_L 180



//******************************************************************************
// D A T A  S T R U C T U R E
//******************************************************************************

// **** DSC Encode Data ****
typedef struct DSC_enc_Data{
	int ucDSC_Htotal;
	int ucDSC_HDE;
	int ucDSC_HSW;
	int ucDSC_HBP;
	int ucDSC_TX_Clk;
	int ucDSC_VDE;
}DSC_enc_Data, *P_DSC_enc_Data;


enum  CSCInput {
	CSCInput_RGB_Limit,
	CSCInput_RGB_Full,
	CSCInput_709_Limit_444,
	CSCInput_709_Full_444,
	CSCInput_xvYCC709_Limit_444,
	CSCInput_xvYCC709_Full_444,
	CSCInput_601_Limit_444,
	CSCInput_601_Full_444,
	CSCInput_xvYCC601_Limit_444,
	CSCInput_xvYCC601_Full_444,
	CSCInput_709_Limit_422,
	CSCInput_709_Full_422,
	CSCInput_xvYCC709_Limit_422,
	CSCInput_xvYCC709_Full_422,
	CSCInput_601_Limit_422,
	CSCInput_601_Full_422,
	CSCInput_xvYCC601_Limit_422,
	CSCInput_xvYCC601_Full_422,
};


enum  HDMITx_DeepColor {
    DeepColor24Bit = 8,
    DeepColor30Bit = 10,
    DeepColor36Bit = 12,
};


typedef enum _CLKTYPE_ {
	CLK_25MHz,
	CLK_27MHz,
	CLK_54MHz,
	CLK_74MHz,
	CLK_148MHz,
	CLK_297MHz,
	CLK_340MHz,
	CLK_594MHz,
	NUM_CLKTYPE
} CLKTYPE;

typedef enum _FSTYPE_ {
	AUDIOFS_44KHz,
	AUDIOFS_NOTINDICATED,
	AUDIOFS_48KHz,
	NUM_FSTYPE
} FSTYPE;


enum  HDMITx_Reaptition {
    Repetition1x,
    Repetition2x,
    Repetition4x,
};


enum  HDMITx_Step {
    Input_Source_Selection,
    Output_Packet_Bypass_Configuration,
    Output_Packet_Configuration,
    Output_Packet_Disable,
    Output_Packet_Enable,
    Phy_PCLK_Source_Configuration,
    Phy_FRLCLK_Source_Configuration,
    Phy_CLK_Configuration,
    Phy_Power_On,
    Phy_Power_Down,
    TMDS_Configuration,
    FRL_Configuration,
    FRL_DSC_Configuration,
    Driving_Enable,
    Driving_Disable,
    Reset_Link_PHY,
    Link_PHY_initialization,
    Link_Input_Timing,
};


enum PacketTypeSelect {
    _NULL_PKT = 0x00,
    _CTS_N_PKT=0x01,
    _AudioSample_PKT=0x02,
    _Gamut_PKT = 0x0A,
    _ChannelStatus = 0xF2, // Jack, in Audio sample
    _VendorSpecificInfo_PKT = 0x81,//Jack
    _AviInfoFrame_PKT=0x82,
    _AudioInfo_PKT = 0x84, //Jack
    _All_PKT = 0xFF,
};

enum HDMITxModeChangeInvokerSelect {
    _TxSelfInvoker,
    _RxInvoker,
};

enum DeepColorDepthSelect {
    _8bit_Depth = 8,
    _10bit_Depth = 10,
    _12bit_Depth = 12,
    _16bit_Depth = 16,
};

enum YCbCrFormat {
    _RGB,
    _YCbCr709444,
    _YCbCr709422,
    _YCbCr709420,
    _YCbCr601444,
    _YCbCr601422,
    _YCbCr601420,
};

enum ColorRange {
    _FULL_RANGE,
    _LIMITED_RANGE,
};

enum {
    HdmiTx_AvMute_Set = 0,
    HdmiTx_AvMute_Clear,
};

enum _HTX_PATH_SELECT_{
    _ICP_ht_2_HTx,
    _ICP_hx_2_HTx,
    _ICP_ht_VGA_2_HTx,
    _ICP_hx_VGA_2_HTx,
    _HDMI_RX0_2_HTx,
    _HDMI_RX1_2_HTx,
    _HDMI_RX2_2_HTx,
    _HDMI_RX3_2_HTx,
    _DP0_2_HTx,
    _DP1_2_HTx,
    _Vx1_2_HTx,
    _VD656_2_HTx,
    _DSC_ENC_2_HTx,
    _Vx1_Dual_2_HTx,
    _ICP_HTXSC_HTx,
};

enum _HTX_INPUT_CLOCK_SOURCE_{
    INPUT_IS_PCLK,
    INPUT_IS_TCLK,
    INPUT_IS_FCLK,
};


typedef struct _HDMI_TX_DATA_FORMAT_ {
	// info from Main
    char bDataRdy  : 1;				//Leon2 set input source is ready or not
    char bLinkBypass  : 1;				//Link Bypass selection
    char bConverter_Mode : 1;			
    char ucSourceInterface;			//Link Input source slection
    char ucAudioSource;				//Audio Input Source
    char bH_POL  : 1;					//H Polarity
    char bV_POL  : 1;					//V Polarity
    char bHDMITXHdmiMode  : 1;			//HDMI Mode : 0: DVI; 1: HDMI
    char bHTX21_FRL_Mode:1;			//H21 FRL Mode Enable
    char bDSCEN:1;						//H21 DSC Enable
    char bPortMode:1;					//TX Link 1port/2port selection
    char bHTX_SC_en:1;					//ICP HT/HX SCALER Enable
    char bPCLK_ExternalMPLL:1;			//PCLK uses external MPLL filiter or not
    char bHTX21_FRL_Failed:1;		    //FRL Training Failed for LEON2M get
    int   InputClock;					//PCLK
    
    char bFull2Limit  : 1;				//AVI INFO
    int ucColorSapce;					//Color : 0:RGB; 1:YUV444; 2:YUV422; 3: YUV420
    char ucVICCode;					//
    char ucRepeatition;				//
    char bSCDCPresent  : 1;

    char ucDeepColor;					//Deep Color Packet
    char ucMaxFRLRate;					//Maximumx FRL Rate

    char bAVI_Info_Packet_EN:1;		//AVI Infofram Packet Enable (0x82)
    char bSPD_Packet_EN:1;				//SPD Packet Enable (0x83)
    char bAudio_Info_Packet_EN:1;		//Audio Infofram Packet Enable (0x84)
    char bMPEG_Packet_EN:1;			//MPEG Packet Enable (0x85)
    char bPPS_Packet_EN:1;				//PPS Packet Enable (0x7F, 0x01)


	// Tx use only
    char bDataUpdated:1;
    char bOutStatus:1;    				//TX set output is ready or not
    char bCtsNUpdate:1;
    char bAudioSample:1;
    char bAviInfoUpdate:1;
    char ucCurrentFRLRate;				//FRL rate selection
    char bFRLPortMode:1;				//FRL 1Port/2Port selection
    char bFRL_Training_done:1;			//FRL training done
    char bFRL_Rate_Update:1;			//Update FRL Rate 
    char bHTX_Enable:1;				
    char bHTX_Training_First:1;		
    char ucPacketSource;
    int  usVact;
	char bDual_Display_Sel:1;
	char ucAudio_fmt;

} HDMITXDataFormat;

extern HDMITXDataFormat HDMITxData[1];

// **** AVI_Info Packet ****
typedef struct _HTX_AVI_INFO_{
	int Type;
	int Version;
	int Length;
	int Checksum;
	int Data[27];
}HTX_AVI_INFO, *P_HTX_AVI_INFO;
#define HDMI_AVI_PKT HTX_AVI_INFO

// **** SPD Packet ****
typedef struct _HTX_SPD_{
	int Type;
	int Version;
	int Length;
	int Checksum;
	int Data[27];
}HTX_SPD, *P_HTX_SPD;
#define HDMI_COMINFO_PKT HTX_SPD

// **** Audio_Info Packet ****
typedef struct _HTX_Audio_INFO_{
	int Type;
	int Version;
	int Length;
	int Checksum;
	int Data[10];
}HTX_Audio_INFO, *P_HTX_Audio_INFO;
#define HDMI_AUDINFO_PKT HTX_Audio_INFO

// **** PPS Packet ****
typedef struct _HTX_PPS_{
	int PKT0_HB[4];
	int PKT0_INFO[8];
	int PKT0_PPS0[22];
	int PKT1_HB[4];
	int PKT1_PPS21[29];
	int PKT2_HB[4];
	int PKT2_PPS49[29];
	int PKT3_HB[4];
	int PKT3_PPS77[29];
	int PKT4_HB[4];
	int PKT4_PPS105[24];
	int PKT4_Hfront_low;
	int PKT4_Hfront_high;
	int PKT4_Hsw_low;
	int PKT4_Hsw_high;
	int PKT4_Hback_low;
	int PKT5_HB[31];
	int PKT5_Hback_high;
	int PKT5_HCactive_low;
	int PKT5_HCactive_high;
}HTX_PPS, *P_HTX_PPS;
#define HDMI_PPS_PKT HTX_PPS

// **** MPEG Packet ****
typedef struct _HTX_MPEG_{
	int Header;
	int Version;
	int Length;
	int Checksum;
	int PB[27];
}HTX_MPEG, *P_HTX_MPEG;

// **** Generic Packet ****
typedef struct _HTX_Generic_{
	int PB[31];
}HTX_Generic, *P_HTX_Generic;

// **** Generic2 Packet ****
typedef struct _HTX_Generic2_{
	int PB[31];
}HTX_Generic2, *P_HTX_Generic2;

// **** User Defined Packet ****
typedef struct _HTX_UserDefined_{
	int PB[31];
}HTX_UserDefined, *P_HTX_UserDefined;

// **** User Defined Packet 2 ****
typedef struct _HTX_UserDefined2_{
	int PKT0_HB[4];
	int PKT0_INFO[8];
	int PKT0_MD[5];
}HTX_UserDefined2, *P_HTX_UserDefined2;
#define HDMI_VTEM_PKT HTX_UserDefined2


//******************************************************************************
// F U N C T I O N    P R O T O T Y P E S
//******************************************************************************

void InitHDMITxStatus(int uHDMITxBlk);
void HDMITx_OutputData_Updated(int uHDMITxBlk);
void Set_HDMITx_DataUpdated(int uHDMITxBlk);
void SetHDMITxInputSrc(int uchdmitxblk, int interface);
void SetHDMITxAudioSrc(int uchdmitxblk, int interface);
void SetHDMITx_Current_FRL(int uchdmitxblk, int ucValue);
int GetHDMITx_Current_FRL(int uHDMITxBlk);
BOOL GetHDMITx_FRL_Mode(int uHDMITxBlk);
void ClearHDMITx_FRL_Mode(int uchdmitxblk);
void ClearHDMITx_FRL_Update(int uchdmitxblk);
void SetHDMITx_FRL_Update(int uchdmitxblk);

void SetHDMITxDataRdyFlag(int uHDMITxBlk);
void ClearHDMITxDataRdyFlag(int uHDMITxBlk);
BOOL IsHDMITxDataRdy(int uHDMITxBlk);
void SetHDMITx_FRL_Done_Flag(int uHDMITxBlk);
void ClearHDMITx_FRL_Done_Flag(int uHDMITxBlk);
void SetHDMITx_FRL_Failed_Flag(int uHDMITxBlk);
void ClearHDMITx_FRL_Failed_Flag(int uHDMITxBlk);
void SetHDMITxOutStatus(int uHDMITxBlk);
void ClearHDMITxOutStatus(int uHDMITxBlk);
BOOL IsHDMITxOutStatus(int uHDMITxBlk);
BOOL GetHTxTableVPol(int uHDMITxBlk);
BOOL GetHTxTableHPol(int uHDMITxBlk);
int GetHDMITXPort_Source(int link);
int GetHDMITXPort_LinkBypass(int link);

// **** DDC/SCDC/EDID ****
void SetHDMITXDDCSpeed(int linkidx, int speed);
void HDMITx_WriteByte_SCDC(int linkidx, int offset, int Data);
int HDMITx_ReadByte_SCDC(int linkidx, UINT32 offset);
void HDMITX_Read_EDID(int linkidx, int* buf,int length, int offset);

// **** GCP Packet ****
void HDMITX_LINK_GCP_Deep_Color_ENABLE(int uHDMITxBlk, BOOL enable);
void HDMITX_LINK_DEEP_COLOR_PACKET_CONFIG(int uHDMITxBlk, char ucColorDepth);
void HDMITX_LINK_GCP_PACKET_ENABLE(int uHDMITxBlk, BOOL enable);
void HDMITxAvMuteSet(int uHDMITxBlk, char avmute_ctrl);

// **** AVI_Info Packet ****
void SetHDMITX_Color(int uHDMITxBlk,char TxColorSpace,char TxColorRange);
void HDMITxSetRepetition(int uHDMITxBlk, char repetition);
void HDMITX_LINK_AVI_INFO_PACKET_CONFIG(int uHDMITxBlk);
void HDMITX_LINK_AVI_INFO_PACKET_Enable(int uHDMITxBlk, BOOL enable);
void HDMITX_LINK_AVI_Info_PACKET_SEND(int uHDMITxBlk, HDMI_AVI_PKT *AVI_Info_packet);
void SetHTX_AVI_PKT(int link, HDMI_AVI_PKT *AVI);

// **** MPEG Packet ****
void HDMITX_LINK_MPEG_PACKET_Enable(int uHDMITxBlk, BOOL enable);
void HDMITX_LINK_MPEG_PACKET_SEND(int uHDMITxBlk, HDMI_COMINFO_PKT *mpeg_packet);
void HDMITX_LINK_MPEG_PACKET_CONFIG(int uHDMITxBlk, char* PB_buf, char PB_length);

// **** SPD Packet ****
void HDMITX_LINK_SPD_PACKET_Enable(int uHDMITxBlk, BOOL enable);
void HDMITX_LINK_SPD_PACKET_SEND(int uHDMITxBlk, HDMI_COMINFO_PKT *spd_packet);
void HDMITX_LINK_SPD_PACKET_CONFIG(int uHDMITxBlk, char* PB_buf, char PB_length);
void SetHTX_SPD_PKT(int link, HDMI_COMINFO_PKT *SPD);

// **** PPS Packet ****
void HDMITX_LINK_PPS_PACKET_Enable(int uHDMITxBlk, BOOL enable);
void HDMITX_LINK_PPS_PACKET_SEND(int uHDMITxBlk, HDMI_PPS_PKT *pps_packet);
void SetHTX_PPS(int link, HDMI_PPS_PKT *PPS);

void HDMITX_LINK_EMP_PACKET_Enable(int uHDMITxBlk, BOOL enable, BOOL force);
void HDMITX_LINK_VTEM_PACKET_Enable(int uHDMITxBlk, BOOL enable);
void HDMITX_LINK_VTEM_PACKET_SEND(int uHDMITxBlk, HDMI_VTEM_PKT *vtem_packet);
void SetHTX_VTEM(int link, HDMI_VTEM_PKT *VTEM);


// **** Audio Packet ****
void HDMITX_LINK_AUDIO_INFO_PACKET_Enable(int uHDMITxBlk, BOOL enable);
void HDMITX_LINK_AUDIO_INFO_PACKET_SEND(int uHDMITxBlk, HDMI_AUDINFO_PKT *audio_info_packet);
void HDMITX_LINK_AUDIO_INFO_PACKET_CONFIG(int uHDMITxBlk);
void SetHTX_AUDInfo_PKT(int link, HDMI_AUDINFO_PKT *AUDInfo);
void HDMITxSetAudioPath(int uHDMITxBlk, int audioin);
void UpdateLineInToHDMITxAudio(int txlink);
void UpdateHDMIRxToHDMITxAudio(int rxlink, int txlink);
void UpdateDPRxToHDMITxAudio(int rxlink, int txlink);
BOOL HDMITxUpdateAudio(int uHDMITxBlk, char ucInput);

// **** VID Pattern (Mute) ****
BOOL IS_HDMITx_MuteData(int linkidx);
void HDMITx_UpdateMutePattern(int linkidx);
void HDMITx_MuteData(int linkidx, BOOL enable);

// **** CONFIGURATION ****//
void HDMITX_LINK_HDMI_MODE_CONFIG(int uHDMITxBlk, BOOL enable);
void HDMITX_LINK_SCDC_SCRAMBLE_CONFIG(int uHDMITxBlk, BOOL enable);

void HDMITX_LINK_POWER_DOWN(int uHDMITxBlk);
void HDMITX_LINK_RESET(int uHDMITxBlk);
void HDMITX_LINK_PHY_Init(int uHDMITxBlk);
void HDMITTX_LINK_PHY_LANE_PN_SWAP(int uHDMITxBlk);
void HDMITX_INPUT_POLARITY(int uHDMITxBlk);
void HDMITX_LINK_Source_Selection(int uHDMITxBlk);
void HDMITX_LINK_Port_Mode(int uHDMITxBlk);
void HDMITX_LINK_Check_PSTABLE(int uHDMITxBlk);
void HDMITX_LINK_FRL_CONFIG(int uHDMITxBlk, int hdmitx_FRL_rate, BOOL DSC_MODE);
void HDMITX_LINK_TMDS_CONFIG(int uHDMITxBlk);
void HDMITX_LINK_FRL_RATE_DBG(int ucFRL_Rate);
void HDMITX_LINK_FRL_TriByteConfig(int uHDMITxBlk,UINT32 TriByteDiff);

void HDMITX_CONFIG_STATE(int uHDMITxBlk, int ucHDMITx_Step);
BOOL IsHDMITxNoOutputVaild(void);
void IsHDMITxOutputVaild(int uHDMITxBlk);
void HDMITX_OUTPUT_CONFIG_TOP(int uHDMITxBlk);
void HDMITxHandler_Test_Debug(int uHDMITxBlk);
void HDMITxHandler(void);
void HDMITxInit(void);
void HDMITxShutDown(int uHDMITxBlk);
void HDMITxCheckHPDReadEDID(int uHDMITxBlk);
void HDMITxReadEDID(int uHDMITxBlk);
BOOL IsHDMITXHPDOn(int uHDMITxBlk);
BOOL IsHDMITXFRLFailed(int uHDMITxBlk);
void HDMITX_FRL_LINK_RST(int uHDMITxBlk);
void HDMITX_LINK_FRL_Tx_GAP(int uHDMITxBlk, BOOL enable);
void HDMITX_LINK_GCP_AVMUTE_ECO(int uHDMITxBlk, BOOL enable);
void HDMITX_ClkDetect_Input_Sel(int uHDMITxBlk, int Clock_select);
void HDMITX_ClkDetect_Enable(int uHDMITxBlk, BOOL Enable);
void HDMITX_ClkDetect_Get_Value(int uHDMITxBlk);
void HDMITX_VRR_FAPA_Start_Location(int uHDMITxBlk, BOOL type);
void HDMITX_Get_PCLK_CRC_Value(int uHDMITxBlk);
void HDMITX_Get_TCLK_CRC_Value(int uHDMITxBlk);
void HDMITX_Get_DSC_CRC_Value(int uHDMITxBlk, int frame_stop_cnt, int DSC_ChunkSize, int DSC_VDE);


//
//  hdmitx2p1.c
//
extern void			 hdmitx2p1_hotplug_on(void);
extern void			 hdmitx2p1_hotplug_off(void);

extern ER hdmitx2p1_i2c_open(PI2C_SESSION p_session);
extern ER hdmitx2p1_i2c_close(I2C_SESSION p_session);
extern void hdmitx2p1_i2c_set_config(I2C_SESSION session, I2C_CONFIG_ID config_id, UINT32 ui_config);


//extern THREAD_RETTYPE hdmi2p1_polling_tsk_frl(void *param);











#endif
