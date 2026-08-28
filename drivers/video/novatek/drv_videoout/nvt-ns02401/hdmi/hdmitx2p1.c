/*
    HDMI TX 2.1 module driver

    HDMI TX 2.1 module driver.

    @file       hdmitx_2p1.c

    Copyright   Novatek Microelectronics Corp. 2024.  All rights reserved.
*/
#include "hdmitx_int.h"
#include "hdmitx2p1_int.h"
#include "hdmitx2p1_int_PHY.h"
#include "hdmitx2p1_int_FRL.h"
#include "hdmitx2p1_reg.h"
#include "dispdev_hdmi.h"







/*global resource define*/
/*
VK_TASK_HANDLE hdmi2p1_tsk_hdl = NULL;
VK_TASK_HANDLE hdmi2p1_frl_tsk_hdl = NULL;
static SEM_HANDLE SEMID_HDMI2P1;
static SEM_HANDLE SEMID_HDMI2P1_HPD;
static SEM_HANDLE SEMID_HDMI2P1_EDID;
*/
#define FLGPTN_HDMI2P1			FLGPTN_BIT(0) 
#define DRV_INT_HDMI2P1        INT_ID_HDMI2P1
static ID       FLG_ID_HDMI2P1;


#define loc_cpu(flag) 
#define unl_cpu(flag)


/*global variable define*/
HDMITXDataFormat HDMITxData[_Num_HDMI2P1_TX];		//HTX Parameter Structue
HDMITXDataFormat HDMITxData_option[_Num_HDMI2P1_TX];		//HTX Parameter Structue


//int 			hdmi2p1_id = 0;
static BOOL         b_hdmi2p1_tx_open     = FALSE;
//static UINT32       gui_hdmi_int_sts;
static BOOL 		hdmi2p1_init = FALSE;
BOOL				b_hdmi2p1_edid_msg			= DISABLE;
BOOL                hdmi2p1_i2c_select = 0;
hdmi_state_t 		hdmi2p1_state = STATE_NONE;
int 				hdmi2p1_hpd_status;

UINT32              hdmi2p1_ctrl_option[HDMICFG_OPTION_ALL] = {FALSE, TRUE, TRUE, FALSE, FALSE, 0x0};
static int g_Sink_adapt_4kp60_420_mode=0;


UINT32 PPS_table[26]={0};
static DSC_enc_Data DSCENCData={0};
UINT32 g_HDMI21_TriByteDiff = TX_TriByte_H;

#if !_FPGA_EMULATION_
static int dcc_timeout_cnt = 0;
#endif


#if defined(__FREERTOS) // i2c usage define

int hdmitx2p1_i2c_cur_dev = 0;
struct i2c_adapter *hdmitx2p1_i2c_adapter = NULL;
struct i2c_client *hdmitx2p1_i2c_client = NULL;


static int hdmitx2p1_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	hdmitx_i2c_info[0] = &hdmitx_i2c_info_def[0];

	hdmitx_i2c_info[0]->iic_client  = client;
	hdmitx_i2c_info[0]->iic_adapter = client->adapter;

	i2c_set_clientdata(client, hdmitx_i2c_info[0]);

	return 0;
}



static int hdmitx2p1_i2c_remove(struct i2c_client *client)
{
	hdmitx_i2c_info[0] = NULL;

	return 0;
}


static struct i2c_board_info hdmitx2p1_i2c_device[1] = {
	{
		.type = HDMITX2P1_I2C_NAME,
		.addr = HDMITX_I2C_ADDR
	},
};


static const struct i2c_device_id hdmitx2p1_i2c_device_id[][1] = {
	{
		{ HDMITX2P1_I2C_NAME, 0 },
	},
};

static struct i2c_driver hdmitx2p1_i2c_driver[1] = {
	{
		.driver = {
			.name  = HDMITX2P1_I2C_NAME,
			.owner = THIS_MODULE,
		},
		.probe    = hdmitx2p1_i2c_probe,
		.remove   = hdmitx2p1_i2c_remove,
		.id_table = hdmitx2p1_i2c_device_id[0]
	},
};

int hdmitx2p1_i2c_init(void)
{
	I2C_ID i2c_id;

	i2c_id = 5;// for i2c6
	
	/*
		new device and add driver
	*/
	hdmitx2p1_i2c_adapter = i2c_get_adapter(i2c_id);
	hdmitx2p1_i2c_client = i2c_new_device(hdmitx2p1_i2c_adapter, &hdmitx2p1_i2c_device[hdmitx2p1_i2c_cur_dev]);
	if (hdmitx2p1_i2c_client == NULL) {
		DBG_ERR("[2p1] i2c_new_device fail\r\n");
		return FALSE;
	}
	if (i2c_add_driver(&hdmitx2p1_i2c_driver[hdmitx2p1_i2c_cur_dev]) != 0) {
		DBG_ERR("[2p1] i2c_add_driver fail\r\n");
		return FALSE;
	}
	return TRUE;

}
#endif





void hdmitx2p1_set_tmds_oe(BOOL b_en);
void hdmitx2p1_hotplug_on(void);
void hdmitx2p1_hotplug_off(void);


#if 0
THREAD_RETTYPE hdmi2p1_polling_tsk(void *param)
{
	int status;
	int hpg_change_cnt = 0;
	
	DBG_WRN("[2p1] %s is running\n", __func__);
	
	do {		
		vos_sem_wait(SEMID_HDMI2P1_HPD);

		if (hdmi2p1_state & HDMI_THREAD_STOP){
			hdmi2p1_state &= ~HDMI_THREAD_STOP;
			THREAD_RETURN(0);
		}
	
		/* check hotplug */
		status = hdmitx2p1_check_hotplug();
			
		if (hdmi2p1_state & HDMI_OPEN) {
			if (status != hdmi2p1_hpd_status) {

				if (hdmi_cts_test == 0) {
					hpg_change_cnt ++;
					if (hpg_change_cnt < 20){
						//DBG_WRN("[2p1] hdmi: hotplug chg cnt [%d]!\n",hpg_change_cnt);
						goto loop;
					} else {
						hpg_change_cnt = 0;
						//DBG_WRN("[2p1] hdmi: hotplug chged! cnt = [%d]\n",hpg_change_cnt);
					}
				}	
				
				if (status == TRUE) {
					hdmitx2p1_hotplug_on();
					hdmi2p1_state |= HPG_CONNECT;
					DBG_WRN("[2p1] hdmi: hotplug is ON!\n");
				} else {
					hdmitx2p1_hotplug_off();
					hdmi2p1_state &= ~HPG_CONNECT;
					DBG_WRN("[2p1] hdmi: hotplug is OFF!\n");
				}
				hdmi2p1_state |= HPD_CHANGE;
				hdmi2p1_hpd_status = status;
			} else if(!(hdmi2p1_state & HPG_CONNECT) && !(hdmi2p1_state & HPD_CHANGE)){ // unplug when open
					hdmitx2p1_hotplug_off();
					hdmi2p1_state &= ~HPG_CONNECT;
					hdmi2p1_state |= HPD_CHANGE;
					DBG_WRN("[2p1] hdmi: hotplug is OFF when open state!\n");						
			}
		}
loop:		
		vos_sem_sig(SEMID_HDMI2P1_HPD);
		DELAY_MS(5);
#ifdef __KERNEL__
	} while(!nvt_hdmitx_drv_task_should_stop());
	
	DBG_WRN("[2p1] %s is stopped\n", __func__);
	
	return 0;	
#else
	} while(1);
#endif
}
#endif

void hdmitx2p1_create_resource(void)
{
/*
	vos_flag_create(&FLG_ID_HDMI2P1, NULL, "FLG_ID_HDMI2P1");
	vos_sem_create(&SEMID_HDMI2P1, 1, "SEMID_HDMI2P1");
	vos_sem_create(&SEMID_HDMI2P1_HPD, 1, "SEMID_HDMI2P1_HPD");
	vos_sem_create(&SEMID_HDMI2P1_EDID, 1, "SEMID_HDMI2P1_EDID");
	hdmi2p1_tsk_hdl = vos_task_create(hdmi2p1_polling_tsk, NULL, "hdmi2p1_polling_tsk", 10, 4096);
	vos_task_resume(hdmi2p1_tsk_hdl);

	disphdmi_debug(("%s done\r\n",__func__));
	*/
}

void hdmitx2p1_release_resource(void)
{
/*
	vos_flag_destroy(FLG_ID_HDMI2P1);
	vos_sem_destroy(SEMID_HDMI2P1);
	vos_sem_destroy(SEMID_HDMI2P1_HPD);
	vos_sem_destroy(SEMID_HDMI2P1_EDID);
	vos_task_destroy(hdmi2p1_tsk_hdl);
	*/
}

void hdmitx2p1_create_resource_frl(void)
{
/*
	hdmi2p1_frl_tsk_hdl = vos_task_create(hdmi2p1_polling_tsk_frl, NULL, "hdmi2p1_polling_tsk_frl", 10, 4096);
	vos_task_resume(hdmi2p1_frl_tsk_hdl);
	disphdmi_debug(("%s done\r\n",__func__));
	*/
}

void hdmitx2p1_release_resource_frl(void)
{
/*
	vos_task_destroy(hdmi2p1_frl_tsk_hdl);
	disphdmi_debug(("%s done\r\n",__func__));
	*/
}



/* hdmi dbg dump*/
void _hdmi2p1_dump_reg(UINT32 page, int cnt, char *log)
{
	int i=0;


	DBG_WRN("\r\n");
	DBG_WRN("[%s] dump as below:\r\n",log);
	DBG_WRN("       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");
	DBG_WRN("       ===============================================\r\n");
	for(i=0; i<cnt;i++){
		DBG_WRN("0x%2x : %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \r\n"
			,(unsigned int)i*16
			,(unsigned int)hdmitx2p1_read_reg(0, page,0+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,1+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,2+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,3+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,4+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,5+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,6+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,7+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,8+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,9+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,10+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,11+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,12+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,13+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,14+16*i)
			,(unsigned int)hdmitx2p1_read_reg(0, page,15+16*i));

	}
}



/*
    hdmi isr

    hdmi isr

    @return void
*/
//void (void)
/*
irqreturn_t hdmi2p1_isr(int irq, void *devid)
{
	return IRQ_HANDLED;
}
*/


/*
    Reset hdmi Tx module

    Reset hdmi Tx module's analog & digital logic to default state

    @param[in] b_en     Reset hdmi Tx module
       -@b TRUE: reset
       -@b FALSE: unreset

    @return void
*/
static void hdmitx2p1_reset(BOOL b_en)
{
	disphdmi_debug(("%s\r\n",__func__));
}

/*
    Set HDMI TMDS Output Enable

    Set HDMI TMDS Output Enable. Disable TMDS output can save power during HDMI power down.

    @param[in] b_en
     - @b TRUE:  Enable.
     - @b FALSE: Disable

    @return void
*/
void hdmitx2p1_set_tmds_oe(BOOL b_en)
{
	disphdmi_debug(("%s\r\n",__func__));
}

/*
    Set HDMI controller's Power ON/OFF

    Enable HDMI Power Down would also Disable TMDS Output Enable in this API.

    @param[in] b_pd
     - @b TRUE:   Set HDMI Power OFF
     - @b FALSE:  Set HDMI Power ON

    @return void
*/
static void hdmitx2p1_set_power_down(BOOL b_pd)
{
	disphdmi_debug(("%s\r\n",__func__));
}

/*
    Set HDMI Interrupt Enable/Disable

    Set HDMI Interrupt Enable/Disable

    @param[in] int_type  Interrupt ID to be ENABLE/DISABLE
    @param[in] enable
     - @b TRUE:  ENABLE interrupt
     - @b FALSE: DISABLE interrupt

    @return void
*/
void hdmitx2p1_set_int_en(HDMI_INT_TYPE int_type, BOOL enable)
{
	disphdmi_debug(("%s\r\n",__func__));
}

/*
    HDMI Pad contorl 

    This api is used to pull-none pad HPD gpio to get correct status from pin
*/
static void hdmitx2p1_hpd_pad_control(void)
{
 	//pad_set_pull_updown(PAD_PIN_PGPIO41, PAD_NONE);
	DELAY_MS(5);

	disphdmi_debug(("%s\r\n",__func__));
	
}


/*
    HDMI Driver Attach

    This api would open DDC's pinmux and then enable HDMI's source clock.
*/
static void hdmitx2p1_attach(void)
{
	disphdmi_debug(("%s\r\n",__func__));
	//_hdmitx2p1_enableclk_platform(0);
	CG_SETREG(0x80,CG_GETREG(0x80)|0x10);
}

/*
    HDMI Driver Detach

    This api would close DDC's pinmux and disable HDMI's source clock.
*/
static void hdmitx2p1_detach(void)
{
	disphdmi_debug(("%s\r\n",__func__));
	//_hdmitx2p1_disableclk_platform(0);
	CG_SETREG(0x80,CG_GETREG(0x80)&0xFFFFFFEF);
}

/*
    Enable HDMI InfoFrames Transmission.
*/
void hdmitx2p1_enable_infofrm_tx(HDMIINFO_EN  info_en, HDMIINFO_RPT info_rpt_en)
{
	disphdmi_debug(("%s\r\n",__func__));
}

/*
    Disable HDMI InfoFrames Transmission.
*/
void hdmitx2p1_disable_infofrm_tx(HDMIINFO_EN  info_dis, HDMIINFO_RPT info_rpt_dis)
{
	disphdmi_debug(("%s\r\n",__func__));
}

/*
    Setup HDMI AUDIO's configurations
*/
UINT32 hdmitx2p1_config_audio(int uHDMITxBlk, HDMI_AUDIOFMT ADOFMT)
{
	int N_VAL, FS,MAX_LEN, WORD_LEN;
	
	disphdmi_debug(("%s\r\n",__func__));

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_N_CTS_Ctrl, BIT1, TRUE);
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_N_CTS_Ctrl, BIT0, FALSE);


	switch (ADOFMT) {
	case HDMI_AUDIO48KHZ: {
			//N = 6144 = 0x1800
			N_VAL = HDMI_NVAL6144;
			FS = HDMI_PCM_FS48KHZ;
			DBG_WRN("[2p1] AUD_SR      = [48Khz]\r\n");
			break;
		}
	case HDMI_AUDIO44_1KHZ: {
			//N = 6272 = 0x1880
			N_VAL = HDMI_NVAL6272;
			FS = HDMI_PCM_FS44_1KHZ;
			DBG_WRN("[2p1] AUD_SR      = [44.1Khz]\r\n");
			break;
		}
	case HDMI_AUDIO32KHZ: {
			//N = 4096 = 0x1000
			N_VAL = HDMI_NVAL4096;
			FS = HDMI_PCM_FS32KHZ;
			DBG_WRN("[2p1] AUD_SR      = [32Khz]\r\n");
			break;
		}
	default: {
			DBG_WRN("[2p1] Not HDMI support Audio mode\r\n");
			return 1;
		}

	}

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_N_SVAL_HI, (N_VAL>>16)&0xFF);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_N_SVAL_MI, (N_VAL>>8)&0xFF);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_N_SVAL_LO, (N_VAL)&0xFF);	


	// Set PCM Header: Audio 16bits per sample
	MAX_LEN = 0;  //MaxLen = 20bits
	WORD_LEN = 1; //16bits

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_ChannelStatus, 0x0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_ChannelStatus2, 0x0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_ChannelStatus3, 0x0);
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_ChannelStatus4, 0xF,FS);

	hdmitx2p1_write_reg_bit(uHDMITxBlk , HTX_LINK_CTRL1, _HDMITXLINK_ChannelStatus5, BIT0, MAX_LEN);
	hdmitx2p1_write_reg_mask(uHDMITxBlk , HTX_LINK_CTRL1, _HDMITXLINK_ChannelStatus5, 0xE, WORD_LEN);


	//
	//  Audio infoframe Configuration
	//

	HDMITX_LINK_AUDIO_INFO_PACKET_CONFIG(uHDMITxBlk);


	// config audio TMDS/FRL mode

	if(HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode == TRUE) {
			hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Audio_FRL_Mode, BIT3, TRUE);
	}
		else {
			hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Audio_FRL_Mode, BIT3, FALSE);
	}	
	
	return 0;
}



/*
    Setup HDMI VIDEO's configurations
*/
UINT32 hdmitx2p1_config_video(HDMI_VIDEOID vid_code)
{
	disphdmi_debug(("%s\r\n",__func__));
	return 0;
}


#if 1 //conttoller config


/*=========================================================================================
Function 	: etHDMITx_Current_FRL(int uchdmitxblk, int ucValue)
Description	: Set FRL Rate
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
void SetHDMITx_Current_FRL(int uchdmitxblk, int ucValue)
{
	disphdmi_debug(("%s\r\n",__func__));
    HDMITxData[uchdmitxblk].ucCurrentFRLRate = ucValue;
}

/*=========================================================================================
Function 	: GetHDMITx_Current_FRL(int uchdmitxblk, int ucValue)
Description	: Set FRL Rate
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
int GetHDMITx_Current_FRL(int ucHDMITxBlk)
{
	disphdmi_debug(("%s\r\n",__func__));
    return HDMITxData[ucHDMITxBlk].ucCurrentFRLRate;
}

/*=========================================================================================
Function 	: GetHDMITx_FRL_Mode(int ucHDMITxBlk)
Description	: Set FRL Rate
Note		:
Last Updated: 2020/03/02
==========================================================================================*/
BOOL GetHDMITx_FRL_Mode(int ucHDMITxBlk)
{
	disphdmi_debug(("%s\r\n",__func__));
    return HDMITxData[ucHDMITxBlk].bHTX21_FRL_Mode;
}

/*=========================================================================================
Function 	: ClearHDMITx_FRL_Mode(int uchdmitxblk)
Description	: Set FRL Rate
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
void ClearHDMITx_FRL_Mode(int uchdmitxblk)
{
	disphdmi_debug(("%s\r\n",__func__));
    HDMITxData[uchdmitxblk].bHTX21_FRL_Mode = FALSE;
}


/*=========================================================================================
Function 	: ClearHDMITx_FRL_Update(int uchdmitxblk)
Description	: Set FRL Rate
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
void ClearHDMITx_FRL_Update(int uchdmitxblk)
{
	disphdmi_debug(("%s\r\n",__func__));
    HDMITxData[uchdmitxblk].bFRL_Rate_Update = FALSE;
}

/*=========================================================================================
Function 	: SetHDMITx_FRL_Update(int uchdmitxblk)
Description	: Set FRL Rate
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
void SetHDMITx_FRL_Update(int uchdmitxblk)
{
	disphdmi_debug(("%s\r\n",__func__));
    HDMITxData[uchdmitxblk].bFRL_Rate_Update = TRUE;
}


/*=========================================================================================
Function 	: SetHDMITx_FRL_Failed_Flag(int ucHDMITxBlk), ClearHDMITx_FRL_Failed_Flag(int ucHDMITxBlk)
Description	: Set/Clear HDMI bDataRdy flag (input_source)
Note		:
Last Updated: 2019/12/27
==========================================================================================*/
void SetHDMITx_FRL_Failed_Flag(int ucHDMITxBlk)
{
	disphdmi_debug(("%s\r\n",__func__));
    if (ucHDMITxBlk < _HDMI_TX_All) {
        HDMITxData[ucHDMITxBlk].bHTX21_FRL_Failed = 1;
    }
}


void ClearHDMITx_FRL_Failed_Flag(int ucHDMITxBlk)
{
	disphdmi_debug(("%s\r\n",__func__));
    if (ucHDMITxBlk < _HDMI_TX_All) {
        HDMITxData[ucHDMITxBlk].bHTX21_FRL_Failed = 0;
    }
}

/*=========================================================================================
Function 	: HDMITX_LINK_FRL_RATE_DBG(int ucFRL_Rate)
Description	: HTX Link setting in TMDS mode
Note		: N/A
Last Updated: 2020/04/09, Tim Chien
==========================================================================================*/

void HDMITX_LINK_FRL_RATE_DBG(int ucFRL_Rate)
{

	if(ucFRL_Rate == FRL_RATE_DISABLE) {
		DBG_WRN("[2p1] FRL Rate = DISABLE\n\n");
	}
	else if(ucFRL_Rate ==FRL_RATE_3G_3L) {
		DBG_WRN("[2p1] FRL Rate = 3G/3L\n\n");
	}
	else if(ucFRL_Rate == FRL_RATE_6G_3L) {
		DBG_WRN("[2p1] FRL Rate = 6G/3L\n\n");
	}
	else if(ucFRL_Rate == FRL_RATE_6G_4L) {
		DBG_WRN("[2p1] FRL Rate = 6G/4L\n\n");
	}
	else if(ucFRL_Rate == FRL_RATE_8G_4L) {
		DBG_WRN("[2p1] FRL Rate = 8G/4L\n\n");
	}	
	else if(ucFRL_Rate == FRL_RATE_10G_4L) {
		DBG_WRN("[2p1] FRL Rate = 10G/4L\n\n");
	}
	else if(ucFRL_Rate == FRL_RATE_12G_4L) {
		DBG_WRN("[2p1] FRL Rate = 12G/4L\n\n");
	}		
	else {
		DBG_WRN("[2p1] FRL Rate = NG\n\n");		
	}

}

/*=========================================================================================
Function 	: HDMITX_LINK_FRL_Tx_GAP(int ucHDMITxBlk, BOOL enable)
Description	: HDMI Tx send GAP signal 
Note		:
Last Updated: 2020/04/08, Tim Chien
==========================================================================================*/
void HDMITX_LINK_FRL_Tx_GAP(int ucHDMITxBlk, BOOL enable)
{	
	disphdmi_debug(("%s\r\n",__func__));
	hdmitx2p1_write_reg_bit(ucHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_1Port_DeepColor_ECO, BIT0, enable);	
}

/*=========================================================================================
Function 	: SetHDMITx_FRL_Done_Flag(int ucHDMITxBlk), ClearHDMITx_FRL_Done_Flag(int ucHDMITxBlk)
Description	: Set/Clear HDMI bDataRdy flag (input_source)
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
void SetHDMITx_FRL_Done_Flag(int ucHDMITxBlk)
{
	disphdmi_debug(("%s\r\n",__func__));
    if (ucHDMITxBlk < _HDMI_TX_All) {
        HDMITxData[ucHDMITxBlk].bFRL_Training_done = 1;
    }
}


void ClearHDMITx_FRL_Done_Flag(int ucHDMITxBlk)
{
	disphdmi_debug(("%s\r\n",__func__));
    if (ucHDMITxBlk < _HDMI_TX_All) {
        HDMITxData[ucHDMITxBlk].bFRL_Training_done = 0;
    }
}

BOOL IsHDMITxOutStatus(int ucHDMITxBlk)
{
	disphdmi_debug(("%s\r\n",__func__));

	return TRUE;

	/*
	if (HDMITxData[ucHDMITxBlk].bOutStatus) {
		return TRUE;
	} else {
		return FALSE;
	}
	*/
}



/*=========================================================================================
Function 	: HDMITX_PHY_PRE_DRVER(int uHDMITxBlk, BOOL enable)
Description	: (1) Enable / Disable DeEmphasis pre-driver
			  (2) Enable / Disable PreShoot pre-driver
Note		: N/A
Last Updated: 2022/10/19, Kled Chen
==========================================================================================*/
void HDMITX_PHY_PRE_DRVER(int uHDMITxBlk, BOOL enable)
{
	if(enable) {
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_FRL_CFG7, 0xFF);  //877B[7:4] Enable pre-shoot pre-driver
																					   //877B[3:0] Enable de-emphasis pre-driver
	}
	else {
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_FRL_CFG7, 0x00);
	}
}


/*=========================================================================================
Function 	: HDMITX_LINK_RESET(int uHDMITxBlk)
Description	: Rest Link
Note		:
Last Updated: 2019/12/20, Kled Chen
==========================================================================================*/
void HDMITX_LINK_RESET(int uHDMITxBlk)
{
	char HTX_packet_enable_status[5];

	// Store packet enable setting.
	//HTX_packet_enable_status[0] = hdmitx2p1_read_reg60(uHDMITxBlk, _HDMITXLINK_UDP_PACKET_CTL);
	HTX_packet_enable_status[1] = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_0);
	HTX_packet_enable_status[2] = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_1);
	HTX_packet_enable_status[3] = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS_LOAD);
	HTX_packet_enable_status[4] = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_CTRL_CFG);

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, ADR_HDMI_SRST, BIT0, TRUE);				//Soft Reset
	DELAY_MS(1);
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, ADR_HDMI_SRST, BIT0, FALSE);

	// Restore packet enable setting after HTX link reset.
	//SC_MaskSetByteHDMITX60(uHDMITxBlk, _HDMITXLINK_UDP_PACKET_CTL, 0x0, HTX_packet_enable_status[0]);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_0, HTX_packet_enable_status[1]);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_1, HTX_packet_enable_status[2]);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS_LOAD, HTX_packet_enable_status[3]);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_CTRL_CFG, HTX_packet_enable_status[4]);
}


/*=========================================================================================
Function 	: HDMITX_LINK_PHY_Init(int uHDMITxBlk)
Description	: HDMI TX PHY/LINK Initialization
Note		:
Last Updated: 2019/12/20, Kled Chen
==========================================================================================*/
void HDMITX_LINK_PHY_Init(int uHDMITxBlk)
{
	//Link Initial Setting
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_LineBuf_Ctrl, BIT7, TRUE); 		//new line buffer mode enable, turn on this for non regular input timing
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_TMDS_Ctrl8, 0xD6);				//new line buffer mode delay counter, end region of the data island packet
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMI_XWIN_REKEY_CNT, 0x2E);				//HDCP1.4 rekey counter
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL0, ADR_HDMI_SYS_CTRL1, 0x30);					//VSYNC,HSYNC Enable
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Mode_DeepColor, BIT7, TRUE);	//HDMI Mode
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_EMP_MASK_LB, 0xFF);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_EMP_MASK_HB, 0xFF);
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_ACEN, BIT5, TRUE);		//Bypass Hs Dejitter circuit
}

/*=========================================================================================
Function 	: HDMITX_LINK_HDMI_MODE_CONFIG(int uHDMITxBlk, BOOL enable)
Description	: HDMI Mode or DVI Mode
Note		:
Last Updated: 2020/02/17
==========================================================================================*/
void HDMITX_LINK_HDMI_MODE_CONFIG(int uHDMITxBlk, BOOL enable)
{

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Mode_DeepColor, BIT7|BIT0, enable);
}



/*=========================================================================================
Function 	: SetHDMITxOutStatus(int uHDMITxBlk), ClearHDMITxOutStatus(int uHDMITxBlk)
Description	: Set/Clear HDMI bOutStatus flag (tx_output)
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
void SetHDMITxOutStatus(int uHDMITxBlk)
{
	disphdmi_debug(("%s\r\n",__func__));
    if (uHDMITxBlk < _HDMI_TX_All) {
        HDMITxData[uHDMITxBlk].bOutStatus = 1;
    }
}

void ClearHDMITxOutStatus(int uHDMITxBlk)
{
	disphdmi_debug(("%s\r\n",__func__));
    if (uHDMITxBlk < _HDMI_TX_All) {
        HDMITxData[uHDMITxBlk].bOutStatus = 0;
    }
}
/*=========================================================================================
Function 	: GetHTxTableVPol(int uHDMITxBlk), GetHTxTableHPol(int uHDMITxBlk)
Description	: Get H/V Polarity
Note		:
Last Updated: 2020/03/02
==========================================================================================*/
BOOL GetHTxTableVPol(int uHDMITxBlk)
{
    return HDMITxData[uHDMITxBlk].bV_POL;
}

BOOL GetHTxTableHPol(int uHDMITxBlk)
{
    return HDMITxData[uHDMITxBlk].bH_POL;
}

/*=========================================================================================
Function 	: HDMITX_LINK_Source_Selection(int uHDMITxBlk)
Description	: Select HTX LINK Input Source
Note		:
Last Updated: 2019/12/20, Kled Chen
==========================================================================================*/
void HDMITX_INPUT_POLARITY(int uHDMITxBlk)
{
	char tmp_3F;

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_OUTPUT_HV_POL, (BIT1|BIT0), FALSE); //6ABA[1:0] = 2'b00
	
	if (HDMITxData[uHDMITxBlk].bLinkBypass) {		
		tmp_3F = (hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_INTPUT_DET) & 0x03) << 4; //6A3F[1:0] -> 6A33[5:4]
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_INPUT_HV_POL_, tmp_3F);
	}
	else {
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_INPUT_HV_POL_, (BIT5|BIT4), FALSE); //6A33[5:4] = 2'b00
		
		if (GetHTxTableHPol(uHDMITxBlk) == TRUE) { //H
			disphdmi_debug(("Desired H Polarity + \n\n"));
			hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_SYNC_SCCP_CFG, BIT1, FALSE);               //Count fromrising
		}
		else {
			disphdmi_debug(("Desired H Polarity - \n\n"));
			hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_SYNC_SCCP_CFG, BIT1, TRUE);             //Count from falling
			hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_OUTPUT_HV_POL, BIT1, TRUE);
			hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_INPUT_HV_POL_, BIT4, TRUE);
		}

		if (GetHTxTableVPol(uHDMITxBlk) == TRUE) { //V
			disphdmi_debug(("Desired V Polarity + \n\n"));
			hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_SYNC_SCCP_CFG, BIT0, FALSE);               //Count from rising
		}
		else {
			disphdmi_debug(("Desired V Polarity - \n\n"));
			hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_SYNC_SCCP_CFG, BIT0, TRUE);             //Count from falling
			hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_OUTPUT_HV_POL, BIT0, TRUE);
			hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_INPUT_HV_POL_, BIT5, TRUE);
		}
	}
}

/*=========================================================================================
Function 	: HDMITX_LINK_Source_Selection(int uHDMITxBlk)
Description	: Select HTX LINK Input Source
Note		:
Last Updated: 2024/06/19, Kled Chen
==========================================================================================*/
void HDMITX_LINK_Source_Selection(int uHDMITxBlk)
{
	disphdmi_debug(("%s\r\n",__func__));

	if(HDMITxData[uHDMITxBlk].bDSCEN == 0){
		if (hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] & 0x1) {
			hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, 	_HDMITX_SYS_Ctrl_0, 		 0x89); // FIFO_en
		} else if (hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] & 0x2) {
			hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL,	_HDMITX_SYS_Ctrl_0, 		 0x98); // FIFO_en
		} else if ((hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] & 0x4)){
			hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL,	_HDMITX_SYS_Ctrl_0, 		 0x98); // FIFO_en
		} else {
			hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, 	_HDMITX_SYS_Ctrl_0, 		 0xA4); // FIFO_en
		}
	} else {
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, 	_HDMITX_SYS_Ctrl_0, 		 0xA4); // FIFO_en, DSC en always use default setting
	}


	if(HDMITxData[uHDMITxBlk].bFRLPortMode == 0){
		if (HDMITxData[uHDMITxBlk].bDual_Display_Sel == 1) {
			hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL,	_HDMITX_SYS_Ctrl_1, 		 0x10); // LCD select
			disphdmi_debug(("Dual-Display with LCD_Right\r\n"));
		} else if (HDMITxData[uHDMITxBlk].bDual_Display_Sel == 0) {
			hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL,	_HDMITX_SYS_Ctrl_1, 		 0x0); // LCD select
			disphdmi_debug(("Dual-Display with LCD_Left\r\n"));
		}
	} else {
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL,	_HDMITX_SYS_Ctrl_1, 		 0x0); // LCD select
	}
}


/*=========================================================================================
Function 	: HDMITX_LINK_POWER_DOWN(int uHDMITxBlk)
Description	: Link Power Down Setting
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
void HDMITX_LINK_POWER_DOWN(int uHDMITxBlk)
{
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Mode_DeepColor, BIT6, FALSE);	//Disable GCP
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_0, 0x00);			//Disable all packet enable
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_1, 0x00);
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, ADR_HDMI_SYS_CTRL1, BIT0, FALSE);			//Power Down Mode
}

/*=========================================================================================
Function 	: HDMITX_LINK_TMDS_CONFIG(int uHDMITxBlk)
Description	: HTX Link setting in TMDS mode
Note		: N/A
Last Updated: 2019/12/20, Kled Chen
==========================================================================================*/

void HDMITX_LINK_TMDS_CONFIG(int uHDMITxBlk)
{
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, ADR_HDMI_SYS_CTRL1, BIT4, TRUE);				//Enable HSYNC
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, ADR_HDMI_SYS_CTRL1, BIT5, TRUE);				//Enable VSYNC
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_SSCP_EN, 0x08|BIT1);				//bit1:null pkt during V sync
}




/*=========================================================================================
Function 	: HDMITxUpdateAudio(int uHDMITxBlk, char ucInput)
Description	: Update Audio Packet from Source
Note		:
Last Updated: 2024/06/19, Kled Chen
==========================================================================================*/
BOOL HDMITxUpdateAudio(int uHDMITxBlk, char ucInput)
{
#if 0
    char linkidx = 0;
    char phyidx = 0;
#if NEW_HDMITx_N_CTS == ON
    char i = 0;
    ULONG temp_N = 0;
    ULONG temp_CTS = 0;
    char FS = 0;
    char Rx_FRLRate = 0;
    #define FS_MultiRate 6
#endif
#if (INPUT_INTERFACE&INPUT_DP_MASK)
    char fsidx, clkidx, reg1081;
    ULONG audio_m, audio_n, video_m, video_n;
    int audio_clk, video_clk;
#endif
    typedef enum _CLKTYPE_ {
        CLK_25MHz,
        CLK_27MHz,
        CLK_54MHz,
        CLK_74MHz,
        CLK_148MHz,
        CLK_297MHz,
        NUM_CLKTYPE
    } CLKTYPE;

    typedef enum _FSTYPE_ {
        AUDIOFS_44KHz,
        AUDIOFS_NOTINDICATED,
        AUDIOFS_48KHz,
        NUM_FSTYPE
    } FSTYPE;

#if NEW_HDMITx_N_CTS == ON

    static const char FRL_MCSL[] = {
           1,        // FRL_RATE_DISABLE	= 0x0,
        1666,        // FRL_RATE_3G_3L		= 0x1,
        3333,        // FRL_RATE_6G_3L		= 0x2,
        3333,        // FRL_RATE_6G_4L		= 0x3,
        4444,        // FRL_RATE_8G_4L		= 0x4,
        5555,        // FRL_RATE_10G_4L		= 0x5,
        6666,        // FRL_RATE_12G_4L		= 0x6,
    };

    typedef enum _FRL_FSTYPE_ {
        FRL_AUDIOFS_32KHz,
        FRL_AUDIOFS_44_1KHz,
        FRL_AUDIOFS_48KHz,
        NUM_FRL_FSTYPE
    } FRL_FSTYPE;

    static const char FSVALUE[NUM_FSTYPE*FS_MultiRate] = {
         320,   640,   1280,   2560,   5120,   10240,
         441,   882,   1764,   3528,   7056,   14112,
         480,   960,   1920,   3840,   7680,   15360,
    };
    
    static const ULONG NVALUE_32K[4][FS_MultiRate] = {
    //    32K   64K   128K   256K   512K   1024K
    //3G
        {4224, 8448, 16896, 33792, 67584, 135168},
    //6G. 8G
        {4032, 8064, 16128, 32256, 64512, 129024},
    //10G
        {3456, 6912, 13824, 27648, 55296, 110592},
    //12G
        {3072, 6144, 12288, 24576, 49152, 98304},
    };

    static const ULONG NVALUE_44_1K[2][FS_MultiRate] = {
    //  44.1K 88.24K 176.4K 352.8K 705.6K 1411.2K
    //3G. 6G
        {5292, 10584, 21168, 42336, 84672, 169344 },
    //8G.10G.12G
        {3969,  7938, 15876, 31752, 63504, 127008 },
    };

    static const ULONG NVALUE_48K[4][FS_MultiRate] = {
    //    48K    96K   192K   384K   768K   1536K
    //3G
        {5760, 11520, 23040, 46080, 92160, 184320 },
    //6G.8G    
        {6048, 12096, 24192, 48384, 96768, 193536 },
    //10G
    
        {5184, 10368, 20736, 41472, 82944, 165888 },
    //12G
        {4752,  9504, 19008, 38016, 76032, 152064 },
    };
   
#endif

#if (INPUT_INTERFACE&INPUT_DP_MASK)
    static const ULONG NVALUE[NUM_FSTYPE * NUM_CLKTYPE] = {
        // Audio FS = 44.1KHz
        6272, 6272,  6272, 0x001880, 6272, 4704,
        // Audio FS = Not indicated
        6272,  6272,  6272,  0x001880, 6272, 4704,
        // Audio FS = 48KHz
        6144,  6144, 6144, 6144, 6144, 5120,
    };

    static const ULONG CTSVALUE[NUM_FSTYPE * NUM_CLKTYPE] = {
        // Audio FS = 44.1KHz
        28000, 30030, 60060, 0x014244, 165000, 247500,
        // Audio FS = Not indicated
        28000,  30030, 60060, 0x014244, 165000, 247500,
        // Audio FS = 48KHz
        25200, 27027, 54054, 74250, 148500, 247500,
    };
#endif
    if (uHDMITxBlk >= _HDMI_TX_All) {
        return FALSE;
    }

    switch (ucInput) {
        case _audio_HDMI0:
        case _audio_HDMI1:
        case _audio_HDMI2:
        case _audio_HDMI3://maybe no use 
            if (ucInput == _audio_HDMI0) {
                linkidx = _HDMI21_Link0;
                pSC_GetByteHDMIRX = SC_GetByteHDMI21RX;
            }
            else if (ucInput == _audio_HDMI1) {
                linkidx = _HDMI21_Link1;
                pSC_GetByteHDMIRX = SC_GetByteHDMI21RX;
                }
            else if (ucInput == _audio_HDMI2) {
                linkidx = _HDMI21_Link2;
                pSC_GetByteHDMIRX = SC_GetByteHDMI21RX;
                }
            else if (ucInput == _audio_HDMI3) {
                linkidx = _HDMI21_Link3;
                pSC_GetByteHDMIRX = SC_GetByteHDMI21RX;
            }
            if (((pSC_GetByteHDMIRX(linkidx, _HDMI21RX_AUDIO_CHANNEL_STATUS1) & BIT1) != 0)
              && (pSC_GetByteHDMIRX(linkidx, _CTRL_PACKET_280) == 0x09)) {//Compress
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Audio_I2S_Ctrl2, 0x9B);
                hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Mode_DeepColor, (BIT2|BIT1), FALSE); 
                hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Audio_I2S_Ctrl, BIT7, TRUE);//8ch
            }
            else {//PCM
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Audio_I2S_Ctrl2, 0x2B);//need to check
                hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Mode_DeepColor, BIT2, FALSE);
#if TX_8CH_AUDIO == ON
                if ((pSC_GetByteHDMIRX(linkidx, _CTRL_PACKET_281) & BIT4) != 0) {
                    hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Mode_DeepColor, BIT1, TRUE); //8ch
                }
                else
#endif
                {
                    hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Mode_DeepColor, BIT1, FALSE); //2ch
                }
            }
            UpdateHDMIRxToHDMITxAudio(linkidx, uHDMITxBlk);
            
            break;
#if (INPUT_INTERFACE&INPUT_DP_MASK)
        case _audio_DP0:
        case _audio_DP2:
            if (ucInput == _audio_DP0) {
                linkidx = _DP_Link0;
            }
            else if (ucInput == _audio_DP2) {
                linkidx = _DP_Link1;
            }

            if ((SC_GetByteDPRX(linkidx, _DP_AUDIO_FIFO_CTRL2) & 0x1C) == 0x1C) {
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Audio_I2S_Ctrl2, 0x2B);//need to check
                hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Mode_DeepColor, BIT2, FALSE);
#if TX_8CH_AUDIO == ON
                hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Mode_DeepColor, BIT1, TRUE); //8ch
#else
                hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Mode_DeepColor, BIT1, FALSE); //2ch
#endif
            }
            else {
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Audio_I2S_Ctrl2, 0x2B);//need to check
                hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Mode_DeepColor, (BIT2|BIT1), FALSE);
            }
            UpdateDPRxToHDMITxAudio(linkidx, uHDMITxBlk);
            break;
#endif
        default://Linein
            //disphdmi_debug(("Audio Packet updated\r\n");
            UpdateLineInToHDMITxAudio(uHDMITxBlk);
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Audio_I2S_Ctrl2, 0x2B);//need to check
            hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_Mode_DeepColor, (BIT2|BIT1), FALSE);
        break;
    }


    switch (ucInput) {
        case _audio_HDMI0:
        case _audio_HDMI1:
        case _audio_HDMI2: //HDMI21 Rx
        case _audio_HDMI3://maybe no use
            if (ucInput == _audio_HDMI0) {
                linkidx = _HDMI21_Link0;
                phyidx = 0;
                pSC_GetByteHDMIRX = SC_GetByteHDMI21RX;
            }
            else if (ucInput == _audio_HDMI1) {
                linkidx = _HDMI21_Link1;
                phyidx = 1;
                pSC_GetByteHDMIRX = SC_GetByteHDMI21RX;
            }
            else if (ucInput == _audio_HDMI2) {
                linkidx = _HDMI21_Link0;
                phyidx = 2;
                pSC_GetByteHDMIRX = SC_GetByteHDMI21RX;
            }
            else if (ucInput == _audio_HDMI3) {
                linkidx = _HDMI21_Link1;
                phyidx = 3;
                pSC_GetByteHDMIRX = SC_GetByteHDMI21RX;
            }

#if NEW_HDMITx_N_CTS == ON         
            hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_CTS_Ctrl, BIT1, TRUE);
            if (pSC_GetByteHDMIRX(linkidx, _ACR_CTRL)&BIT3) {
                temp_N = (((ULONG)pSC_GetByteHDMIRX(linkidx, _ACR_N_VALUE_HI))<<16)+(pSC_GetByteHDMIRX(linkidx, _ACR_N_VALUE_MI)<<8)+pSC_GetByteHDMIRX(linkidx, _ACR_N_VALUE_LO);
            }
            else {
                temp_N = (((ULONG)pSC_GetByteHDMIRX(linkidx, _ACR_N_HVAL_HI))<<16)+(pSC_GetByteHDMIRX(linkidx, _ACR_N_HVAL_MI)<<8)+pSC_GetByteHDMIRX(linkidx, _ACR_N_HVAL_LO);
            }

            if (pSC_GetByteHDMIRX(linkidx, _ACR_CTRL)&BIT4) {
                temp_CTS = (((ULONG)pSC_GetByteHDMIRX(linkidx, _ACR_CTS_VALUE_HI ))<<16)+(pSC_GetByteHDMIRX(linkidx, _ACR_CTS_VALUE_MI)<<8)+pSC_GetByteHDMIRX(linkidx, _ACR_CTS_VALUE_LO);
            }
            else {
                temp_CTS = (((ULONG)pSC_GetByteHDMIRX(linkidx, _ACR_CTS_HVAL_HI))<<16)+(pSC_GetByteHDMIRX(linkidx, _ACR_CTS_HVAL_MI)<<8)+pSC_GetByteHDMIRX(linkidx, _ACR_CTS_HVAL_LO);
            }
            if(temp_CTS < 125)
                temp_CTS = 125;
            //follow spec recommand N value by audio FS
			if((ucInput == _audio_HDMI2 || ucInput == _audio_HDMI3) && IsHDMI21_FRLMode(linkidx) == TRUE) {
            #if (NT68580_HDMI_RX_PHY == ENABLE)
                Rx_FRLRate = HDMI21_RX_GET_FRL_RATE(phyidx);
            #else
                Rx_FRLRate = DRV_HDMI21_RX_GET_FRL_RATE(phyidx);
            #endif
                if(Rx_FRLRate > FRL_RATE_12G_4L) {
                    Rx_FRLRate = FRL_RATE_12G_4L;
                }
	            FS = (ULONG)((temp_N/128)*FRL_MCSL[Rx_FRLRate]/(temp_CTS/125))*8;
                for(i=0; i<NUM_FRL_FSTYPE*FS_MultiRate; i++) {
                    if(abs(FS - FSVALUE[i])<=8) {
                        break;
                    }
                }
                switch(i/FS_MultiRate) {
                    case 0:
                    default:
                        switch(GetHDMITx_Current_FRL(uHDMITxBlk)) {
                            case FRL_RATE_3G_3L:
                            default:    
                                temp_N = NVALUE_32K[0][i%FS_MultiRate];
                                break;
                            case FRL_RATE_6G_3L:
                            case FRL_RATE_6G_4L:
                            case FRL_RATE_8G_4L:
                                temp_N = NVALUE_32K[1][i%FS_MultiRate];
                                break;
                            case FRL_RATE_10G_4L:
                                temp_N = NVALUE_32K[2][i%FS_MultiRate];
                                break;
                            case FRL_RATE_12G_4L:
                                temp_N = NVALUE_32K[3][i%FS_MultiRate];
                                break;
                        }                        
                        break;
                    case 1:
                        switch(GetHDMITx_Current_FRL(uHDMITxBlk)) {
                            case FRL_RATE_3G_3L:
                            case FRL_RATE_6G_3L:
                            case FRL_RATE_6G_4L:
                            default:    
                                temp_N = NVALUE_44_1K[0][i%FS_MultiRate];
                                break;
                            case FRL_RATE_8G_4L:
                            case FRL_RATE_10G_4L:
                            case FRL_RATE_12G_4L:
                                temp_N = NVALUE_44_1K[1][i%FS_MultiRate];
                                break;
                        }                        
                        break;
                    case 2:
                        switch(GetHDMITx_Current_FRL(uHDMITxBlk)) {
                            case FRL_RATE_3G_3L:
                            default:    
                                temp_N = NVALUE_48K[0][i%FS_MultiRate];
                                break;
                            case FRL_RATE_6G_3L:
                            case FRL_RATE_6G_4L:
                            case FRL_RATE_8G_4L:
                                temp_N = NVALUE_48K[1][i%FS_MultiRate];
                                break;
                            case FRL_RATE_10G_4L:
                                temp_N = NVALUE_48K[2][i%FS_MultiRate];
                                break;
                            case FRL_RATE_12G_4L:
                                temp_N = NVALUE_48K[3][i%FS_MultiRate];
                                break;
                        }                        
                        break;
                }
			}
			else {	
                if(ucInput == _audio_HDMI0 || ucInput == _audio_HDMI1) 
    	            FS = (ULONG)((temp_N/128)*(HDMI_PORT_STATUS_GET_PHYTCLK(linkidx)/1000)*10/(temp_CTS/125))*8;  
                else
    	            FS = (ULONG)((temp_N/128)*(HDMI_PORT_STATUS_GET_PHYTCLK(linkidx)/1000)/(temp_CTS/125))*8;
	            if(abs(FS - 320) <= 8) {  //32K
	                temp_N = 4096;
	            }
	            else if(abs(FS - 640) <= 8) {  //64K
	                temp_N = 8192;
	            }
	            else if(abs(FS - 1280) <= 8) {  //128K
	                temp_N = 16384;
	            }
	            else if(abs(FS - 2560) <= 8) {  //256K
	                temp_N = 32768;
	            }
	            else if(abs(FS - 441) <= 8) {  //44.1K
	                temp_N = 6272;
	            }
	            else if(abs(FS - 882) <= 8) {  //88.2K
	                temp_N = 12544;
	            }
	            else if(abs(FS - 1764) <= 8) {  //176.4K
	                temp_N = 25088;
	            }
	            else if(abs(FS - 3528) <= 8) {  //352.8K
	                temp_N = 50176;
	            }
	            else if(abs(FS - 480) <= 8) {  //48K
	                temp_N = 6144;
	            }
	            else if(abs(FS - 960) <= 8) {  //96K
	                temp_N = 12288;
	            }
	            else if(abs(FS - 1920) <= 8) {  //192K
	                temp_N = 24576;
	            }
	            else if(abs(FS - 3840) <= 8) {  //384K
	                temp_N = 49152;
	            }
			}
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_HI, (char)(temp_N>>16));
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_MI, (char)(temp_N>>8));
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_LO, (char)(temp_N));

            
#elif Fixed_N == ON
            hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_CTS_Ctrl, BIT1, TRUE);

            if (pSC_GetByteHDMIRX(linkidx, _ACR_CTRL)&BIT3) {
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_LO, pSC_GetByteHDMIRX(linkidx, _ACR_N_VALUE_LO));
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_MI, pSC_GetByteHDMIRX(linkidx, _ACR_N_VALUE_MI));
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_HI, pSC_GetByteHDMIRX(linkidx, _ACR_N_VALUE_HI));
            }
            else {
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_LO, pSC_GetByteHDMIRX(linkidx, _ACR_N_HVAL_LO));
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_MI, pSC_GetByteHDMIRX(linkidx, _ACR_N_HVAL_MI));
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_HI, pSC_GetByteHDMIRX(linkidx, _ACR_N_HVAL_HI));
            }

            if (pSC_GetByteHDMIRX(linkidx, _ACR_CTRL)&BIT4) {
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_CTS_SVAL_LO, pSC_GetByteHDMIRX(linkidx, _ACR_CTS_VALUE_LO));
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_CTS_SVAL_MI, pSC_GetByteHDMIRX(linkidx, _ACR_CTS_VALUE_MI));
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_CTS_SVAL_HI, pSC_GetByteHDMIRX(linkidx, _ACR_CTS_VALUE_HI));
            }
            else {
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_CTS_SVAL_LO, pSC_GetByteHDMIRX(linkidx, _ACR_CTS_HVAL_LO));
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_CTS_SVAL_MI, pSC_GetByteHDMIRX(linkidx, _ACR_CTS_HVAL_MI));
                hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_CTS_SVAL_HI, pSC_GetByteHDMIRX(linkidx, _ACR_CTS_HVAL_HI));
            }
#else
            hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_CTS_Ctrl, BIT1, TRUE);
            hdmitx2p1_write_reg(uHDMITxBlk,HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_LO, 0x00);
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_MI, 0x18);
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_HI, 0x00);
#endif
            return TRUE;
            break;
#if (INPUT_INTERFACE&INPUT_DP_MASK)
        case _audio_DP0:
        case _audio_DP2:
            if (ucInput == _audio_DP0) {
                linkidx = _DP_Link0;
            }
            else if (ucInput == _audio_DP2) {
                linkidx = _DP_Link1;
            }
            #if DpInHDMITxOutWithAudioPkt == ON
            hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_CTS_Ctrl, BIT1, TRUE);
            #else
            hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_CTS_Ctrl, BIT1, FALSE);
            #endif

            reg1081 = SC_GetByteDPRX(linkidx, _DP_PLL_CTRL);
            SC_SetByteDPRX(linkidx, _DP_PLL_CTRL, (reg1081 & (~(BIT5))));

            // Video M/N
            video_m = SC_GetByteDPRX(linkidx, _DP_PLL_M_VID_HI);
            video_m = (video_m << 8) | SC_GetByteDPRX(linkidx, _DP_PLL_M_VID_MI);
            video_m = (video_m << 8) | SC_GetByteDPRX(linkidx, _DP_PLL_M_VID_LO);

            video_n = SC_GetByteDPRX(linkidx, _DP_PLL_N_VID_HI);
            video_n = (video_n << 8) | SC_GetByteDPRX(linkidx, _DP_PLL_N_VID_MI);
            video_n = (video_n << 8) | SC_GetByteDPRX(linkidx, _DP_PLL_N_VID_LO);

            SC_SetByteDPRX(linkidx, _DP_PLL_CTRL, (reg1081 | (BIT5)));
            // Audio M/N
            audio_m = SC_GetByteDPRX(linkidx, _DP_PLL_M_VID_HI);
            audio_m = (audio_m << 8) | SC_GetByteDPRX(linkidx, _DP_PLL_M_VID_MI);
            audio_m = (audio_m << 8) | SC_GetByteDPRX(linkidx, _DP_PLL_M_VID_LO);

            audio_n = SC_GetByteDPRX(linkidx, _DP_PLL_N_VID_HI);
            audio_n = (audio_n << 8) | SC_GetByteDPRX(linkidx, _DP_PLL_N_VID_MI);
            audio_n = (audio_n << 8) | SC_GetByteDPRX(linkidx, _DP_PLL_N_VID_LO);

            if(video_n == 0 || audio_n == 0) {
				return TRUE;
            }

            if ((SC_GetByteDPRX(linkidx, _DP_MISC_CTRL) & (BIT1 | BIT0)) == 0x00) { // 1.62G                       //x01080
                audio_clk = (int)((ULONG)audio_m * 1620000 / 512  / audio_n );
                video_clk = (int)((ULONG)video_m * 1620  / video_n ) * 2;
            }
            else if ((SC_GetByteDPRX(linkidx, _DP_MISC_CTRL) & (BIT1 | BIT0)) == 0x01) { // 2.7G
                audio_clk = (int)((ULONG)audio_m * 2700000 / 512  / audio_n );
                video_clk = (int)((ULONG)video_m * 2700  / video_n ) * 2;
            }
            else {  //5.4G
                audio_clk = (int)((ULONG)audio_m * 5400000 / 512  / audio_n );
                video_clk = (int)((ULONG)video_m * 5400  / video_n ) * 2;
            }
            SC_SetByteDPRX(linkidx, _DP_PLL_CTRL, reg1081);
#if Fixed_N == ON
            if (abs(audio_clk - 441) <= 5) {  // 44.1k
                fsidx = 0;
            }
            else if (abs(audio_clk - 480) <= 5) { // 48k
                fsidx = 2;
            }
            else {
                fsidx = 1;
            }

            if (video_clk  <=  ((250 + 270) / 2)) {
                clkidx = 0;
            }
            else if (video_clk  <=  ((270 + 540) / 2)) {
                clkidx = 1;
            }
            else if (video_clk  <=  ((540 + 740) / 2)) {
                clkidx = 2;
            }
            else if (video_clk  <=  ((740 + 1480) / 2)) {
                clkidx = 3;
            }
            else if (video_clk  <=  ((1480 + 2970) / 2)) {
                clkidx = 4;
            }
            else {
                clkidx = 5;
            }
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_LO, NVALUE[fsidx * NUM_CLKTYPE + clkidx] & 0xFF);
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_MI, (NVALUE[fsidx * NUM_CLKTYPE + clkidx] >> 8) & 0xFF);
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_HI, (NVALUE[fsidx * NUM_CLKTYPE + clkidx] >> 16) & 0xFF);
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_CTS_SVAL_LO, CTSVALUE[fsidx * NUM_CLKTYPE + clkidx] & 0xFF);
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_CTS_SVAL_MI, (CTSVALUE[fsidx * NUM_CLKTYPE + clkidx] >> 8) & 0xFF);
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_CTS_SVAL_HI, (CTSVALUE[fsidx * NUM_CLKTYPE + clkidx] >> 16) & 0xFF);
#else
            hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_CTS_Ctrl, BIT1, TRUE);
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_LO, 0x00);
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_MI, 0x18);
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_HI, 0x00);
#endif
            return TRUE;

            break;
#endif
    }
    hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_CTS_Ctrl, BIT1, TRUE);
    hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_LO, 0x00);
    hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_MI, 0x18);
    hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL, _HDMITXLINK_N_SVAL_HI, 0x00);
#endif
    return TRUE;//FALSE;
}

/*=========================================================================================
Function 	: HDMITX_LINK_Port_Mode(int uHDMITxBlk)
Description	: Select HTX LINK 1 Port or 2 Port Mode
Note		:
Last Updated: 2024/06/19, Kled Chen
==========================================================================================*/
void HDMITX_LINK_Port_Mode(int uHDMITxBlk)
{
	char mode;
	mode = HDMITxData[uHDMITxBlk].bPortMode;
	

	if(mode) {														//Port: 2Pixel Mode
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_Port_CFG, BIT0, TRUE);
		if(HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode == FALSE) {
			hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_Port_CFG2, BIT0, TRUE);
		}
		disphdmi_debug(("2 pxl mode\n\n"));
	}
	else {															//Port: 1Pixel Mode
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_Port_CFG, BIT0, FALSE);
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_Port_CFG2, BIT0, FALSE);
		disphdmi_debug(("1 pxl mode\n\n"));
	}
}


//=======================================================================================
//=======================================================================================
//********************************* GCP Packet*******************************************
//=======================================================================================
//=======================================================================================


/*=========================================================================================
Function 	: HDMITX_LINK_GCP_Deep_Color_ENABLE(int uHDMITxBlk, BOOL enable)
Description	: GCP Packet Deep Color enable/disable
Note		:
Last Updated: 2019/12/25
==========================================================================================*/
void HDMITX_LINK_GCP_Deep_Color_ENABLE(int uHDMITxBlk, BOOL enable)
{
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Mode_DeepColor, BIT6, enable);	//4 	//GCP Deep Color Packet Enable/disable
}


/*=========================================================================================
Function 	: HDMITX_LINK_DEEP_COLOR_PACKET_CONFIG(int uHDMITxBlk, char ucColorDepth)
Description	: Set deep color (GCP) packet
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
void HDMITX_LINK_DEEP_COLOR_PACKET_CONFIG(int uHDMITxBlk, char ucColorDepth)
{
    char reg12F, reg04A;

	HDMITX_LINK_GCP_Deep_Color_ENABLE(uHDMITxBlk,FALSE);

    reg12F = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Mode_DeepColor);		//GCP Packet
    reg12F &= ~(BIT6 | BIT5 | BIT4 | BIT3);
    reg04A = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_MODE);			//Dither Mode Bit
    reg04A &= ~(BIT7 | BIT6 | BIT5);

    if (ucColorDepth == _8bit_Depth) {
        disphdmi_debug(("8bit_Depth\n\n"));
        hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Mode_DeepColor, (reg12F | (0x04 << 3)));
        hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_MODE, reg04A);
        HDMITX_LINK_GCP_Deep_Color_ENABLE(uHDMITxBlk,FALSE);
    }
    else if (ucColorDepth == _10bit_Depth) {
        disphdmi_debug(("10bit_Depth\n\n"));
        hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Mode_DeepColor, (reg12F | (0x05 << 3)));
        hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_MODE, (reg04A | (1 << 6)));
        HDMITX_LINK_GCP_Deep_Color_ENABLE(uHDMITxBlk,TRUE);
    }
    else if (ucColorDepth == _12bit_Depth) {
		disphdmi_debug(("12bit_Depth\n\n"));
        hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Mode_DeepColor, (reg12F | (0x06 << 3)));
        hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_MODE, (reg04A | (2 << 6)));
        HDMITX_LINK_GCP_Deep_Color_ENABLE(uHDMITxBlk,TRUE);
    }
	else{
        disphdmi_debug(("8bit_Depth default\n\n"));
        hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Mode_DeepColor, (reg12F | (0x04 << 3)));
        hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_MODE, reg04A);
        HDMITX_LINK_GCP_Deep_Color_ENABLE(uHDMITxBlk,FALSE);
    }
}


/*=========================================================================================
Function 	: HDMITX_LINK_GCP_Packet_Enable(int uHDMITxBlk, BOOL enable)
Description	: GCP Packet enable/disable
Note		:
Last Updated: 2019/12/25
==========================================================================================*/
void HDMITX_LINK_GCP_PACKET_ENABLE(int uHDMITxBlk, BOOL enable)
{
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_1, BIT3, enable);	//4 	//GCP Packet Enable/disable
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_1, BIT2, enable);		//GCP Packet Repeat
}


/*=========================================================================================
Function 	: HDMITxAvMuteSet(int uHDMITxBlk, char avmute_ctrl)
Description	: Enable/Disable AVMute
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
void HDMITxAvMuteSet(int uHDMITxBlk, char avmute_ctrl)
{

	HDMITX_LINK_GCP_PACKET_ENABLE(uHDMITxBlk, FALSE);

    if (avmute_ctrl == HdmiTx_AvMute_Set) {
        if (hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AvMute_Ctrl) != 0x01) {
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AvMute_Ctrl, 0x01);	//Set AV Mute Flag
            disphdmi_debug(("HdmiTx_AvMute_Set \r\n"));
        }
    }
    else {
        if (hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AvMute_Ctrl) != 0x10) {
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AvMute_Ctrl, 0x10);	//clear AV Mute Flag
            disphdmi_debug(("HdmiTx_AvMute_Clear\r\n"));
        }
    }

	HDMITX_LINK_GCP_PACKET_ENABLE(uHDMITxBlk, TRUE);
}




//=======================================================================================
//=======================================================================================
//********************************* AVI Infoframe Packet ********************************
//=======================================================================================
//=======================================================================================

/*=========================================================================================
Function 	: SetHDMITX_Color(int uHDMITxBlk,char TxColorSpace,char TxColorRange)
Description	: Set Color Space
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
void SetHDMITX_Color(int uHDMITxBlk,char TxColorSpace,char TxColorRange)
{
    switch (TxColorSpace) {
    case _RGB:
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_CSCSEL, BIT4, FALSE);
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_ACEN, BIT2, FALSE);
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_ACEN, BIT0, FALSE);
    break;
    case  _YCbCr709444:
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_CSCSEL, BIT4, TRUE);		// 709
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_ACEN, BIT2, TRUE);		//RGB2YCC
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_ACEN, BIT0, FALSE);		// 444
    break;
    case  _YCbCr601444:
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_CSCSEL, BIT4, FALSE);		// 709
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_ACEN, BIT2, TRUE);		//RGB2YCC
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_ACEN, BIT0, FALSE);		// 422
    break;
    case  _YCbCr709422:
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_CSCSEL, BIT4, TRUE);		// 709
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_ACEN, BIT2, TRUE);		//RGB2YCC
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_ACEN, BIT0, TRUE);		// 422
    break;
    case  _YCbCr601422:
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_CSCSEL, BIT4, FALSE);		// 601
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_ACEN, BIT2, TRUE);		//RGB2YCC
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_ACEN, BIT0, TRUE);		// 422
    break;
    }

    if(TxColorRange == _FULL_RANGE){
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_ACEN, BIT1, FALSE);		//Full2Limited disable
    }
    else{
        hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VID_ACEN, BIT1, TRUE);		//Full2Limited
    }
}


/*=========================================================================================
Function 	: HDMITxSetReaptition(int uHDMITxBlk, char repetition)
Description	: Set Repeatition
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
void HDMITxSetRepetition(int uHDMITxBlk, char repetition)
{
    char ucTmp;
    ucTmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_REPETITION_Ctrl);
	//disphdmi_debug(("_HDMITXLINK_REPETITION_Ctrl = 0x%x",ucTmp);
    switch (repetition & 0x0F) {
        case 1: // pixel send two times
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_REPETITION_Ctrl, ((ucTmp & 0x9F) | (2 << 5)));
            //HDMITX_PHY_Repetition_PLL1_Setting(uHDMITxBlk, Repetition2x); //move to HDMITX_PHY_TOTAL_PLL_CONFIG() run Roger 20221005
            HDMITxData[uHDMITxBlk].ucRepeatition = Repetition2x;
            break;
        case 3: // pixel send four times
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_REPETITION_Ctrl, ((ucTmp & 0x9F) | (3 << 5)));
            //HDMITX_PHY_Repetition_PLL1_Setting(uHDMITxBlk, Repetition4x); //move to HDMITX_PHY_TOTAL_PLL_CONFIG() run Roger 20221005
            HDMITxData[uHDMITxBlk].ucRepeatition = Repetition4x;
            break;
        case 0: // No Repetition
        default:
            hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_REPETITION_Ctrl, ((ucTmp & 0x9F) | (1 << 5)));
            //HDMITX_PHY_Repetition_PLL1_Setting(uHDMITxBlk, Repetition1x); //move to HDMITX_PHY_TOTAL_PLL_CONFIG() run Roger 20221005
            HDMITxData[uHDMITxBlk].ucRepeatition = Repetition1x;
            break;
    }
}


/*=========================================================================================
Function 	: HDMITX_LINK_AVI_INFO_PACKET_CONFIG(int uHDMITxBlk)
Description	: Set AVI_Info Frame
Note		:
Last Updated: 2020/06/02
==========================================================================================*/
void HDMITX_LINK_AVI_INFO_PACKET_CONFIG(int uHDMITxBlk)
{
	char TxColorSpaceRange = 0; 
	int TxColorSpace = 0;//, interface;
	UINT32 ucTmp = 0, ucTmp2 = 0, ucTmp3 = 0, repetition = 0, checksum;//, reg1081;
    char ucVICCode = 0;
	char k;
	int  video_clk;
	char 					*mode_name[] = {
		"RGB888",
		"YCbCr709444",
		"YCbCr709422",
		"YCbCr709420",
		"YCbCr444",
		"YCbCr422",
		"YCbCr420",
	};	

	ucVICCode = HDMITxData[uHDMITxBlk].ucVICCode;
	TxColorSpaceRange = HDMITxData[uHDMITxBlk].bFull2Limit;
	TxColorSpace = HDMITxData[uHDMITxBlk].ucColorSapce;
	video_clk = HDMITxData[uHDMITxBlk].InputClock;


#if 0
	if ((HDMITxData[uHDMITxBlk].ucSourceInterface == INTERNAL_1_INPUT) || (HDMITxData[uHDMITxBlk].ucSourceInterface == INTERNAL_2_INPUT)) {
		interface = HDMITxData[uHDMITxBlk].ucPacketSource;
	}
	else {
		interface = HDMITxData[uHDMITxBlk].ucSourceInterface;
	}	
	switch(interface){
		case DISPLAY_PORT_INPUT0:
			//if(HDMITxData[uHDMITxBlk].ucDSC_ColorSpace == DSC_YUV444){
				if(IsDPGetSimple422(_DP_Link0)){
					TxColorSpace += 2;
					disphdmi_debug(("AVI 422 \n\r"));
			   	}
			//}
			break;
		case DISPLAY_PORT_INPUT1:
		  // if(HDMITxData[uHDMITxBlk].ucDSC_ColorSpace == DSC_YUV444){
			   if(IsDPGetSimple422(_DP_Link1)){
				   	TxColorSpace += 2;  
				    disphdmi_debug(("AVI 422 \n\r"));
	}
	      // }
			break;
		default:		
		    break;
	}
#endif

     /*  
	if(TxColorSpace == CSCInput_RGB_Limit || TxColorSpace == CSCInput_RGB_Full){
		disphdmi_debug(("Color Space = RGB\n\n"));
		TxColorSpace = _RGB;
	} else if ((TxColorSpace == CSCInput_709_Limit_444)||(TxColorSpace == CSCInput_709_Full_444)||
		  (TxColorSpace == CSCInput_xvYCC709_Limit_444)||(TxColorSpace == CSCInput_xvYCC709_Full_444)){
		   disphdmi_debug(("Color Space = YUV 709 444\n\n"));
		   TxColorSpace = _YCbCr709444;
	} else if ( (TxColorSpace == CSCInput_601_Limit_444)||(TxColorSpace == CSCInput_601_Full_444)||
		  (TxColorSpace == CSCInput_xvYCC601_Limit_444)||(TxColorSpace == CSCInput_xvYCC601_Full_444)){
		   disphdmi_debug(("Color Space = YUV 601 444\n\n"));
		   TxColorSpace = _YCbCr601444;
	} else if ((TxColorSpace == CSCInput_709_Limit_422)||(TxColorSpace == CSCInput_709_Full_422)||
		   (TxColorSpace == CSCInput_xvYCC709_Limit_422)||(TxColorSpace == CSCInput_xvYCC709_Full_422)){
		   disphdmi_debug(("Color Space = YUV 709 422\n\n"));
		   TxColorSpace = _YCbCr709422;	 
	} else if ((TxColorSpace == CSCInput_601_Limit_422)||(TxColorSpace == CSCInput_601_Full_422)||
	   (TxColorSpace == CSCInput_xvYCC601_Limit_422)||(TxColorSpace == CSCInput_xvYCC601_Full_422)){
	   		   disphdmi_debug(("Color Space = YUV 601 422\n\n"));
		   TxColorSpace = _YCbCr601422;
	} else {
		disphdmi_debug(("Color Space = NG\n\n"));		
	}
	*/

	// Output Format RGB444 or YUV444 depends on EDID
	if ((hdmitx2p1_get_edid_info()&HDMITX_TV_YCBCR444) && (hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_RGB] == 0)) {
		TxColorSpace =  _YCbCr601444;
	} else if ((hdmitx2p1_get_edid_info()&HDMITX_TV_YCBCR422) && (hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_RGB] == 0)) {
		TxColorSpace =  _YCbCr601422;
	} else {
		TxColorSpace =  _RGB;
	}	



	if (hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] & 0x1) {
			TxColorSpace = _YCbCr601444;
			disphdmi_debug(("Color Space = force to YUV 601 444\n\n"));
	} else if (hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] & 0x2) {
			TxColorSpace = _YCbCr601422;
			disphdmi_debug(("Color Space = force to YUV 601 422\n\n"));
	} else if ((hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] & 0x4)){
			TxColorSpace = _YCbCr601420;
			disphdmi_debug(("Color Space = force to YUV 601 420\n\n"));
	}

	if(g_Sink_adapt_4kp60_420_mode){
		TxColorSpace = _YCbCr601420;
	}	

	DBG_WRN("[2p1] AVI_mode    = [%s]\r\n",mode_name[TxColorSpace]);

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_0, BIT1 | BIT0, FALSE);		//AVI Info Packet Disable

	//setup Tx header
	checksum = 0;
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_Type_Code, HDMIINFO_AVI_TYPE);        // HB0
	checksum += HDMIINFO_AVI_TYPE;
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_Type_Code + 1, HDMIINFO_AVI_VER);     // HB1
	checksum += HDMIINFO_AVI_VER;
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_Type_Code + 2, HDMIINFO_AVI_LEN);     // HB2 
	checksum += HDMIINFO_AVI_LEN;

	//setup Tx color space
	switch (TxColorSpace) {
		case _RGB:
			ucTmp = BIT1;
			ucTmp2 = 0;
			ucTmp3 = 0;
		break;
		/*
		case  _YCbCr709444:
			ucTmp = BIT6 | BIT1;
			ucTmp2 = BIT6|BIT7;
			ucTmp3 = BIT4;
	     break;
		*/
		case  _YCbCr601444:
			ucTmp = BIT6 | BIT1;
			ucTmp2 = BIT6|BIT7;
			ucTmp3 = 0;
		break;
		/*
		 case  _YCbCr709422:
		 	ucTmp = BIT5 | BIT1;
			ucTmp2 = BIT6|BIT7;
			ucTmp3 = BIT4;
		break;
		*/
		case  _YCbCr601422:
			ucTmp = BIT5 | BIT1;
			ucTmp2 = BIT6|BIT7;
			ucTmp3 = 0;
		break;
		/*
		case  _YCbCr709420:
		 	ucTmp = (BIT5|BIT6) | BIT1;
			ucTmp2 = BIT6|BIT7;
			ucTmp3 = BIT4;
		break;		
		*/
		case  _YCbCr601420:
			ucTmp = (BIT5|BIT6) | BIT1;
			ucTmp2 = BIT6|BIT7;
			ucTmp3 = 0;
		break;		

	}

	if(TxColorSpaceRange){
		ucTmp3 |= BIT2;
		repetition = 0;//YQ
	}
	else{
		ucTmp3 |= BIT3;
		repetition = BIT6;//YQ
	}
        // do not turn on HDMTX link CSC
//	SetHDMITX_Color(uHDMITxBlk, TxColorSpace, TxColorSpaceRange);

	disphdmi_debug(("AVI byte 4/5/6 = [%x][%x][%x]\n\n",ucTmp,ucTmp2,ucTmp3));


	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_Type_Code + 4, ucTmp);         	// PB1 
	checksum += ucTmp;
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_Type_Code + 5, ucTmp2);         // PB2 
	checksum += ucTmp2;
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_Type_Code + 6, ucTmp3);			// PB3 
	checksum += ucTmp3;

	//setup Tx VIC code
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_Type_Code + 7, ucVICCode);		// PB4
	checksum += ucVICCode;


	// Repetition
	// pixel clock < 25MHz
	if(HDMITxData[uHDMITxBlk].bDSCEN == FALSE) {
		if ( video_clk < 13) {
			repetition |= BIT0|BIT1;
		}
		else if (video_clk <= 25) { //25MHz
			repetition |= BIT0;
		}
		else {
			repetition |= 0;
		}
	}
	else
	{
		repetition |= 0;	
	}
	
	HDMITxSetRepetition(uHDMITxBlk, repetition);

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_Type_Code + 8, repetition); 		// PB5
	checksum += repetition;

		
	for ( k = 9; k < 31; k++) { //NT68580 : length from 19 to 31
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_Type_Code + k, 0x00);
		checksum += 0;
	}
	checksum = ~(checksum) + 1;
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_Type_Code + 3, checksum);			// PB0 checksum


	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_0, BIT1 | BIT0, TRUE);

}


/*=========================================================================================
Function 	: HDMITX_LINK_AVI_INFO_PACKET_Enable(int uHDMITxBlk, BOOL enable)
Description	: AVI_Info Packet enable/disable
Note		:
Last Updated: 2019/12/25
==========================================================================================*/
void HDMITX_LINK_AVI_INFO_PACKET_Enable(int uHDMITxBlk, BOOL enable)
{
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_0, BIT0|BIT1, enable);			//4
}


/*=========================================================================================
Function 	: HDMITX_LINK_AVI_Info_PACKET_SEND(int uHDMITxBlk, P_HTX_AVI_INFO AVI_Info_packet)
Description	: Write AVI_Info Packet parameter to register
Note		:
Last Updated: 2019/12/26
==========================================================================================*/
//void HDMITX_LINK_AVI_Info_PACKET_SEND(int uHDMITxBlk, P_HTX_AVI_INFO AVI_Info_packet)
void HDMITX_LINK_AVI_Info_PACKET_SEND(int uHDMITxBlk, HDMI_AVI_PKT *AVI_Info_packet)
{
	int i, checksum_tmp;
	checksum_tmp = 0;

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_Type_Code, AVI_Info_packet->Type);		//Type
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_HB1, AVI_Info_packet->Version);		//Version
	if(AVI_Info_packet->Length > 0x1B) {
		AVI_Info_packet->Length = 0x1B;
	}
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_HB2, AVI_Info_packet->Length);			//Length

	for(i=0;i<AVI_Info_packet->Length;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_DB1+i, AVI_Info_packet->Data[i]);	//PB Data
		checksum_tmp += AVI_Info_packet->Data[i];
	}
	checksum_tmp = ~(checksum_tmp) + 1;
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AVI_HB3_SUM, checksum_tmp);				//Checksum

}


/*=========================================================================================
Function 	: SetHTX_AVI_PKT(int link, HDMI_AVI_PKT *AVI)
Description	: Write AVI_Info Packet (used by convertor)
Note		:
Last Updated: 2019/12/30
==========================================================================================*/
void SetHTX_AVI_PKT(int link, HDMI_AVI_PKT *AVI)
{
	HDMITX_LINK_AVI_INFO_PACKET_Enable(link,FALSE);
	HDMITX_LINK_AVI_Info_PACKET_SEND(link,AVI);
	HDMITX_LINK_AVI_INFO_PACKET_Enable(link,TRUE);
}

//=======================================================================================
//=======================================================================================
//********************************** MPEG Packet****************************************
//=======================================================================================
//=======================================================================================


/*=========================================================================================
Function 	: HDMITX_LINK_MPEG_PACKET_Enable(int uHDMITxBlk, BOOL enable)
Description	: MPEG Packet enable/disable
Note		:
Last Updated: 2019/12/25
==========================================================================================*/
void HDMITX_LINK_MPEG_PACKET_Enable(int uHDMITxBlk, BOOL enable)
{
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_0, BIT7|BIT6, enable);			//MPEG Packet Enable/disable
}


/*=========================================================================================
Function 	: HDMITX_LINK_MPEG_PACKET_SEND(int uHDMITxBlk, HDMI_COMINFO_PKT *mpeg_packet)
Description	: Write MPEG Packet parameter to register
Note		:
Last Updated: 2019/12/26
==========================================================================================*/
void HDMITX_LINK_MPEG_PACKET_SEND(int uHDMITxBlk, HDMI_COMINFO_PKT *mpeg_packet)
{
	int i, checksum_tmp;
	checksum_tmp=0;

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_MPEG_HB0, mpeg_packet->Type);			//Type
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_MPEG_HB1, mpeg_packet->Version);		//Version
	if(mpeg_packet->Length > 0x1B) {
		mpeg_packet->Length = 0x1B;
	}
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_MPEG_HB2, mpeg_packet->Length);		//Length

	for(i=0;i<mpeg_packet->Length;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_MPEG_DB1+i, mpeg_packet->Length);	//PB Data
		checksum_tmp += mpeg_packet->Data[i];
	}
	checksum_tmp = ~(checksum_tmp) + 1;
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_MPEG_HB3_SUM, checksum_tmp);			//Checksum

}


/*=========================================================================================
Function 	: HDMITX_LINK_MPEG_PACKET_CONFIG(int uHDMITxBlk, char* PB_buf, char PB_length)
Description	: Configure MPEG packet data from buffer and write to register
Note		:
Last Updated: 2019/12/26
==========================================================================================*/
void HDMITX_LINK_MPEG_PACKET_CONFIG(int uHDMITxBlk, char* PB_buf, char PB_length)
{
	int i, length;
	HDMI_COMINFO_PKT mpeg;

	mpeg.Type=0x85;
	mpeg.Version=0x01;
	mpeg.Length=0x1B;	//Maximum Length

	if(PB_length > 0x1B)
		length = 0x1B;
	else
		length = PB_length;

	for(i=0;i<length;i++){
		mpeg.Data[i]=PB_buf[i];
	}

	HDMITX_LINK_MPEG_PACKET_SEND(uHDMITxBlk, &mpeg);
}


//=======================================================================================
//=======================================================================================
//********************************** SPD Packet****************************************
//=======================================================================================
//=======================================================================================


/*=========================================================================================
Function 	: HDMITX_LINK_SPD_PACKET_Enable(int uHDMITxBlk, BOOL enable)
Description	: SPD Packet enable/disable
Note		:
Last Updated: 2019/12/25
==========================================================================================*/
void HDMITX_LINK_SPD_PACKET_Enable(int uHDMITxBlk, BOOL enable)
{
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_0, BIT3|BIT2, enable);			//SPD Packet Enable/disable
	
}


/*=========================================================================================
Function 	: HDMITX_LINK_SPD_PACKET_SEND(int uHDMITxBlk, P_HTX_SPD spd_packet)
Description	: Write SPD Packet parameter to register
Note		:
Last Updated: 2019/12/26
==========================================================================================*/
void HDMITX_LINK_SPD_PACKET_SEND(int uHDMITxBlk, HDMI_COMINFO_PKT *spd_packet)
{
	int i, checksum_tmp;
	checksum_tmp=0;

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_SPD_HB0, spd_packet->Type);			//Type
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_SPD_HB1, spd_packet->Version);			//Version
	if(spd_packet->Length > 0x1B) {
		spd_packet->Length = 0x1B;
	}
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_SPD_HB2, spd_packet->Length);			//Length

	for(i=0;i<spd_packet->Length;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_SPD_DB1+i, spd_packet->Length);	//Data
		checksum_tmp += spd_packet->Data[i];
	}
	checksum_tmp = ~(checksum_tmp) + 1;
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_SPD_HB3_SUM, checksum_tmp);			//Checksum

}


/*=========================================================================================
Function 	: HDMITX_LINK_SPD_PACKET_CONFIG(int uHDMITxBlk, char* PB_buf, char PB_length)
Description	: Configure SPD packet data from buffer and write to register
Note		:
Last Updated: 2019/12/26
==========================================================================================*/
void HDMITX_LINK_SPD_PACKET_CONFIG(int uHDMITxBlk, char* PB_buf, char PB_length)
{
	int i, length;
	HDMI_COMINFO_PKT spd;

	spd.Type=0x83;
	spd.Version=0x01;
	spd.Length=0x1B;	//Maximum Length

	if(PB_length > 0x1B)
		length = 0x1B;
	else
		length = PB_length;

	for(i=0;i<length;i++){
		spd.Data[i]=PB_buf[i];
	}

	HDMITX_LINK_SPD_PACKET_SEND(uHDMITxBlk, &spd);
}


/*=========================================================================================
Function 	: SetHTX_SPD_PKT(int link, HDMI_COMINFO_PKT *SPD)
Description	: Write SPD Packet (used by convertor)
Note		:
Last Updated: 2019/12/30
==========================================================================================*/
void SetHTX_SPD_PKT(int link, HDMI_COMINFO_PKT *SPD)
{
		HDMITX_LINK_SPD_PACKET_Enable(link,FALSE);
		HDMITX_LINK_SPD_PACKET_SEND(link,SPD);
		HDMITX_LINK_SPD_PACKET_Enable(link,TRUE);
}


//=======================================================================================
//=======================================================================================
//********************************** PPS Packet****************************************
//=======================================================================================
//=======================================================================================


/*=========================================================================================
Function 	: HDMITX_LINK_PPS_PACKET_Enable(int uHDMITxBlk, BOOL enable)
Description	: PPS Packet enable/disable
Note		:
Last Updated: 2019/12/27
==========================================================================================*/
void HDMITX_LINK_PPS_PACKET_Enable(int uHDMITxBlk, BOOL enable)
{
	if(enable){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_START_LB, (HDMITxData[uHDMITxBlk].usVact & 0xFF));
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_START_HB, (HDMITxData[uHDMITxBlk].usVact >> 8));
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS_LOAD, BIT0, TRUE);					//PPS Load
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_CTRL_CFG, BIT0|BIT1, TRUE);
	}
	else{
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_CTRL_CFG, BIT0|BIT1, FALSE);
	}
}

/*=========================================================================================
Function 	: HDMITX_LINK_VTEM_PACKET_Enable(int uHDMITxBlk, BOOL enable)
Description	: VTEM Packet enable/disable
Note		:
Last Updated: 2019/12/27
==========================================================================================*/
void HDMITX_LINK_VTEM_PACKET_Enable(int uHDMITxBlk, BOOL enable) //EMP-VTEM enable
{
	if(enable){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_START_LB, (HDMITxData[uHDMITxBlk].usVact & 0xFF));
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_START_HB, (HDMITxData[uHDMITxBlk].usVact >> 8));

		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_EMP_LOAD, BIT0, TRUE);
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_EMP_MASK_LB, BIT0, FALSE);
		HDMITX_LINK_EMP_PACKET_Enable(uHDMITxBlk, TRUE, FALSE);
	}
	else{
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_EMP_MASK_LB, BIT0, TRUE);
		HDMITX_LINK_EMP_PACKET_Enable(uHDMITxBlk, FALSE, FALSE);
	}
}

/*=========================================================================================
Function 	: HDMITX_LINK_EMP_PACKET_Enable(int uHDMITxBlk, BOOL enable)
Description	: EMP Packet enable/disable
Note		:
Last Updated: 2019/12/27
==========================================================================================*/
void HDMITX_LINK_EMP_PACKET_Enable(int uHDMITxBlk, BOOL enable, BOOL force) //EMP enable
{
	char EMP_MASK_HB, EMP_MASK_LB;
	int EMP_CTRL;
	
	if(enable){
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_CTRL_CFG, BIT0|BIT1, TRUE);
	}
	else if((!enable)&& force){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_EMP_MASK_LB, 0xFF);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_EMP_MASK_HB, 0xFF);
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_CTRL_CFG, BIT0|BIT1, FALSE);
	}
	else{
		EMP_MASK_LB = hdmitx2p1_read_reg(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_EMP_MASK_LB);
		EMP_MASK_HB = hdmitx2p1_read_reg(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_EMP_MASK_HB);
		if((EMP_MASK_LB == 0xFF) && (EMP_MASK_HB == 0xFF)){
			hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_CTRL_CFG, BIT0|BIT1, FALSE);
		}
		else{
			EMP_CTRL = hdmitx2p1_read_reg(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_CTRL_CFG);
			hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_CTRL_CFG, EMP_CTRL);
		}
	}
}

/*=========================================================================================
Function 	: HDMITX_LINK_PPS_PACKET_SEND(int uHDMITxBlk, HDMI_PPS_PKT *pps_packet)
Description	: Write PPS Packet parameter to register
Note		:
Last Updated: 2019/12/27
==========================================================================================*/
void HDMITX_LINK_PPS_PACKET_SEND(int uHDMITxBlk, HDMI_PPS_PKT *pps_packet)
{
	int i;

	//PPS0
	for(i=0;i<3;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS0_PB0+i, pps_packet->PKT0_HB[i]);
	}
	for(i=0;i<7;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS0_PB0+i+3, pps_packet->PKT0_INFO[i]);
	}
	for(i=0;i<21;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS0_PB0+i+10, pps_packet->PKT0_PPS0[i]);
	}

	//PPS1
	for(i=0;i<3;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS1_PB0+i, pps_packet->PKT1_HB[i]);
	}
	for(i=0;i<28;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS1_PB0+i+3, pps_packet->PKT1_PPS21[i]);
	}


	//PPS2
	for(i=0;i<3;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS2_PB0+i, pps_packet->PKT2_HB[i]);
	}
	for(i=0;i<28;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS2_PB0+i+3, pps_packet->PKT2_PPS49[i]);
	}

	//PPS3
	for(i=0;i<3;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS3_PB0+i, pps_packet->PKT3_HB[i]);
	}
	for(i=0;i<28;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS3_PB0+i+3, pps_packet->PKT3_PPS77[i]);
	}

	//PPS4
	for(i=0;i<3;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS4_PB0+i, pps_packet->PKT4_HB[i]);
	}
	for(i=0;i<23;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS4_PB0+i+3, pps_packet->PKT4_PPS105[i]);
	}
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS4_PB0+26, pps_packet->PKT4_Hfront_low);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS4_PB0+27, pps_packet->PKT4_Hfront_high);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS4_PB0+28, pps_packet->PKT4_Hsw_low);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS4_PB0+29, pps_packet->PKT4_Hsw_high);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS4_PB0+30, pps_packet->PKT4_Hback_low);


	//PPS5
	for(i=0;i<3;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS5_PB0+i, pps_packet->PKT5_HB[i]);
	}
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS5_PB0+3, pps_packet->PKT5_Hback_high);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS5_PB0+4, pps_packet->PKT5_HCactive_low);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PPS_CTRL, _HDMITXPPS_PPS5_PB0+5, pps_packet->PKT5_HCactive_high);
	//disphdmi_debug(("HCactive_low = %02X\n\r", pps_packet->PKT5_HCactive_low));
	//disphdmi_debug(("HCactive_high = %02X\n\r", pps_packet->PKT5_HCactive_high));

}

/*=========================================================================================


==========================================================================================*/
void HDMITX_LINK_VTEM_PACKET_SEND(int uHDMITxBlk, HDMI_VTEM_PKT *vtem_packet){
	int i;
	for(i=0;i<3;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_EMP0_HB0+i, vtem_packet->PKT0_HB[i]);
	}
	for(i=0;i<7;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_EMP0_PB0+i, vtem_packet->PKT0_INFO[i]);
	}
	for(i=0;i<4;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_EMP0_PB0+i+7, vtem_packet->PKT0_MD[i]);
	}
}

int PPS_get_val(int i){
	int idx, val;

	idx = i/4;

	if(idx < 24){
		val = (PPS_table[idx] >> (8 * (3-(i%4)))) & 0x000000ff;
	} else {
		val = 0;
	}
	
	return val;
}

/*=========================================================================================
Function 	: HDMITX_LINK_PPS_PACKET_CONFIG(int uHDMITxBlk, char* PB_buf, char PB_length)
Description	: Configure PPS packet data from buffer and write to register
Note		:
Last Updated: 2021/11/23
==========================================================================================*/
void HDMITX_LINK_PPS_PACKET_CONFIG(int uHDMITxBlk)
{

	int i;
	//int DSC_HCACTIVE_L, DSC_HCACTIVE_H;
	int DSC_HCACTIVE;
	int DSC_TIMING_TEMP[3];
	HDMI_PPS_PKT pps;
	int TxColorSpace;


	disphdmi_debug(("\nWrite PPS table into DSC packet Register\n\r"));


	// Output Format RGB444 or YUV444 depends on EDID
	if ((hdmitx2p1_get_edid_info()&HDMITX_TV_YCBCR444) && (hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_RGB] == 0)) {
		TxColorSpace =  _YCbCr601444;
	} else if ((hdmitx2p1_get_edid_info()&HDMITX_TV_YCBCR422) && (hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_RGB] == 0)) {
		TxColorSpace =  _YCbCr601422;
	} else {
		TxColorSpace =  _RGB;
	}	



	if (hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] & 0x1) {
			TxColorSpace = _YCbCr601444;
			disphdmi_debug(("Color Space = force to YUV 601 444\n\n"));
	} else if (hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] & 0x2) {
			TxColorSpace = _YCbCr601422;
			disphdmi_debug(("Color Space = force to YUV 601 422\n\n"));
	} else if ((hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] & 0x4)){
			TxColorSpace = _YCbCr601420;
			disphdmi_debug(("Color Space = force to YUV 601 420\n\n"));
	}

	if(g_Sink_adapt_4kp60_420_mode){
		TxColorSpace = _YCbCr601420;
	}
	/*
	switch(TxColorSpace){
		default:
		case _RGB: dsc_set_config(DSC_ID_1, DSC_CONFIG_SET_PPS_TABLE, DSC_RGB888); disphdmi_debug(("DSC_RGB888\n\n")); break;
		case _YCbCr601444: dsc_set_config(DSC_ID_1, DSC_CONFIG_SET_PPS_TABLE, DSC_YUV444); disphdmi_debug(("DSC_YUV444\n\n"));  break;
		case _YCbCr601422: dsc_set_config(DSC_ID_1, DSC_CONFIG_SET_PPS_TABLE, DSC_YUV422); disphdmi_debug(("DSC_YUV422\n\n"));  break;
		case _YCbCr601420: dsc_set_config(DSC_ID_1, DSC_CONFIG_SET_PPS_TABLE, DSC_YUV420); disphdmi_debug(("DSC_YUV420\n\n"));  break;
	}

	dsc_get_config(DSC_ID_1, DSC_CONFIG_GET_PPS_TABLE, &PPS_table[0]);
	dsc_open(DSC_ID_1);
*/
	DSCENCData.ucDSC_Htotal = 4500;
	DSCENCData.ucDSC_HDE	= 3840;
	DSCENCData.ucDSC_HSW	=   88;
	DSCENCData.ucDSC_HBP    =  296;


	DSC_TIMING_TEMP[0] = DSCENCData.ucDSC_Htotal - DSCENCData.ucDSC_HDE - DSCENCData.ucDSC_HSW - DSCENCData.ucDSC_HBP;
	DSC_TIMING_TEMP[1] = DSCENCData.ucDSC_HSW;
	DSC_TIMING_TEMP[2] = DSCENCData.ucDSC_HBP;

	//HCACTIVE
	/*
	DSC_HCACTIVE_H = DSC_PPS_TEMP[14];
	DSC_HCACTIVE_L = DSC_PPS_TEMP[15];
	disphdmi_debug(("DSC_HCACTIVE_H = %02X\n\r", DSC_HCACTIVE_H));
	disphdmi_debug(("DSC_HCACTIVE_L = %02X\n\r", DSC_HCACTIVE_L));
	DSC_HCACTIVE = (int)(DSC_HCACTIVE_H<<8) + (int)DSC_HCACTIVE_L;
	*/
	DSC_HCACTIVE = 7680;//DSC_HCACTIVE/3;	//Tri-Byte Domain
	disphdmi_debug(("DSC_HCACTIVE = %02X\n\r", DSC_HCACTIVE));

	//============= PPS0 ============
	//-------------------------------
	//PPS Packet0 : PB 0 ~ 9
	pps.PKT0_HB[0]=0x7F;
	pps.PKT0_HB[1]=0x80;
	pps.PKT0_HB[2]=0x00;

	pps.PKT0_INFO[0]=0x06;
	pps.PKT0_INFO[1]=0x00;
	pps.PKT0_INFO[2]=0x01;
	pps.PKT0_INFO[3]=0x00;
	pps.PKT0_INFO[4]=0x02;
	pps.PKT0_INFO[5]=0x00;
	pps.PKT0_INFO[6]=0x88;

	//PPS0 ~ PPS20
	for (i = 0; i<21; i++)						//PPS0 ~ PPS20
		pps.PKT0_PPS0[i]=PPS_get_val(i);
	//------------------------------

	//============= PPS1 ============
	//------------------------------
	//PPS Packet1
	pps.PKT1_HB[0]=0x7F;
	pps.PKT1_HB[1]=0x00;
	pps.PKT1_HB[2]=0x01;

	//PPS21 ~ PPS48
	for (i = 0; i<28; i++)						//PPS21 ~ PPS48
		pps.PKT1_PPS21[i]=PPS_get_val(21+i);
	//------------------------------

	//============= PPS2 ============
	//------------------------------
	//PPS Packet2
	pps.PKT2_HB[0]=0x7F;
	pps.PKT2_HB[1]=0x00;
	pps.PKT2_HB[2]=0x02;

	//PPS49 ~ PPS76
	for (i = 0; i<28; i++)						//PPS49 ~ PPS76
		pps.PKT2_PPS49[i]=PPS_get_val(49+i);
	//------------------------------

	//============= PPS3 ============
	//------------------------------
	//PPS Packet3
	pps.PKT3_HB[0]=0x7F;
	pps.PKT3_HB[1]=0x00;
	pps.PKT3_HB[2]=0x03;

	//PPS77 ~ PPS104
	for (i = 0; i<28; i++)						//PPS77 ~ PPS104
		pps.PKT3_PPS77[i]=PPS_get_val(77+i);
	//------------------------------

	//============= PPS4 ============
	//------------------------------
	//PPS Packet4
	pps.PKT4_HB[0]=0x7F;
	pps.PKT4_HB[1]=0x00;
	pps.PKT4_HB[2]=0x04;

	//PPS105 ~ PPS127
	for (i = 0; i<23; i++)						//PPS105 ~ PPS127
		pps.PKT4_PPS105[i]=PPS_get_val(105+i);

	//HFRONT[7:0]/HFRONT[15:8]

	pps.PKT4_Hfront_low=(int)((DSC_TIMING_TEMP[0]) & 0x00ff);
	pps.PKT4_Hfront_high=(int)((DSC_TIMING_TEMP[0]>>8) & 0x00ff);

	//HSYNC[7:0]/HSYNC[15:8]
	pps.PKT4_Hsw_low=(int)((DSC_TIMING_TEMP[1]) & 0x00ff);
	pps.PKT4_Hsw_high=(int)((DSC_TIMING_TEMP[1]>>8) & 0x00ff);

	//HBACK[7:0]
	pps.PKT4_Hback_low=(int)((DSC_TIMING_TEMP[2]) & 0x00ff);
	//------------------------------

	//============= PPS5 ============
	//------------------------------
	//PPS Packet5
	pps.PKT5_HB[0]=0x7F;
	pps.PKT5_HB[1]=0x40;
	pps.PKT5_HB[2]=0x05;

	//HBACK[15:8]
	pps.PKT5_Hback_high=(int)((DSC_TIMING_TEMP[2]>>8) & 0x00ff);

	//HCACTIVE[7:0]/HCACTIVE[15:8]
	pps.PKT5_HCactive_low=(int)((DSC_HCACTIVE) & 0x00ff);
	pps.PKT5_HCactive_high=(int)((DSC_HCACTIVE>>8) & 0x00ff);
	//------------------------------
	disphdmi_debug(("PKT5_HCactive_low = %02X\n\r", pps.PKT5_HCactive_low));
	disphdmi_debug(("PKT5_HCactive_high = %02X\n\r", pps.PKT5_HCactive_high));

	HDMITX_LINK_PPS_PACKET_SEND(uHDMITxBlk, &pps);
}


/*=========================================================================================
Function 	: void SetHTX_PPS(int link, HDMI_PPS_PKT *PPS)
Description	: Write PPS Packet (used by convertor)
Note		:
Last Updated: 2019/12/30
==========================================================================================*/
void SetHTX_PPS(int link, HDMI_PPS_PKT *PPS)
{

	HDMITX_LINK_PPS_PACKET_Enable(link,FALSE);
	HDMITX_LINK_PPS_PACKET_SEND(link,PPS);
	HDMITX_LINK_PPS_PACKET_Enable(link,TRUE);
}

/*=========================================================================================


==========================================================================================*/
void SetHTX_VTEM(int link, HDMI_VTEM_PKT *VTEM)
{
	HDMITX_LINK_VTEM_PACKET_Enable(link, FALSE);
	HDMITX_LINK_VTEM_PACKET_SEND(link, VTEM);
	HDMITX_LINK_VTEM_PACKET_Enable(link, TRUE);
}

//=======================================================================================
//=======================================================================================
//********************************** Audio Packet****************************************
//=======================================================================================
//=======================================================================================


/*=========================================================================================
Function 	: HDMITX_LINK_AUDIO_INFO_PACKET_Enable(int uHDMITxBlk, BOOL enable)
Description	: Audio_Info Packet enable/disable
Note		:
Last Updated: 2019/12/25
==========================================================================================*/
void HDMITX_LINK_AUDIO_INFO_PACKET_Enable(int uHDMITxBlk, BOOL enable)
{
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_0, BIT5|BIT4, enable);			//Audio_Info Packet Enable/disable
}


/*=========================================================================================
Function 	: HDMITX_LINK_AUDIO_INFO_PACKET_SEND(int uHDMITxBlk, HDMI_AUDINFO_PKT *audio_info_packet)
Description	: Write Audio Info Packet parameter to register
Note		:
Last Updated: 2019/12/26
==========================================================================================*/
void HDMITX_LINK_AUDIO_INFO_PACKET_SEND(int uHDMITxBlk, HDMI_AUDINFO_PKT *audio_info_packet)
{
	int i; 
	UINT8 checksum_tmp;
	checksum_tmp=0;

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AUDInfo_Type_Code, audio_info_packet->Type);	//Header
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AUDInfo_HB1, audio_info_packet->Version);		//Version
	if(audio_info_packet->Length > 0x0A) {
		audio_info_packet->Length = 0x0A;
	}
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AUDInfo_HB2, audio_info_packet->Length);		//Length

	checksum_tmp += (UINT8)audio_info_packet->Type;
	checksum_tmp += (UINT8)audio_info_packet->Version;
	checksum_tmp += (UINT8)audio_info_packet->Length;

	for(i=0;i<audio_info_packet->Length;i++){
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AUDInfo_DB1+i, audio_info_packet->Data[i]);	//PB Data
		checksum_tmp += (UINT8)audio_info_packet->Data[i];
		disphdmi_debug(("checksum_tmp = 0x%x \n\n",checksum_tmp));
	}

	checksum_tmp = 0x100-(UINT8)checksum_tmp;
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_AUDInfo_HB3_SUM, checksum_tmp);				//Checksum

}


/*=========================================================================================
Function 	: HDMITX_LINK_AUDIO_INFO_PACKET_CONFIG(int uHDMITxBlk, char* PB_buf, char PB_length)
Description	: Configure SPD packet data from buffer and write to register
Note		:
Last Updated: 2019/12/26
==========================================================================================*/
void HDMITX_LINK_AUDIO_INFO_PACKET_CONFIG(int uHDMITxBlk)
{
	int i;
	HDMI_AUDINFO_PKT audio_info;

	audio_info.Type=HDMIINFO_AUD_TYPE;
	audio_info.Version=HDMIINFO_AUD_VER;
	audio_info.Length=HDMIINFO_AUD_LEN;	//Maximum Length


	for(i=0;i<audio_info.Length;i++){
		audio_info.Data[i]=0;
	}

	audio_info.Data[0] = HDMIINFO_AUD_D1_CHCNT_2CH; //data byte 1

	

	HDMITX_LINK_AUDIO_INFO_PACKET_SEND(uHDMITxBlk, &audio_info);
	
}



/*=========================================================================================
Function 	: SetHTX_AUDInfo_PKT(int link, HDMI_AUDINFO_PKT *AUDInfo)
Description	: Write Audio Info Packet (used by convertor)
Note		:
Last Updated: 2020/01/14
==========================================================================================*/
void SetHTX_AUDInfo_PKT(int link, HDMI_AUDINFO_PKT *AUDInfo)
{
	HDMITX_LINK_AUDIO_INFO_PACKET_Enable(link,FALSE);
	HDMITX_LINK_AUDIO_INFO_PACKET_SEND(link,AUDInfo);
	HDMITX_LINK_AUDIO_INFO_PACKET_Enable(link,TRUE);
}

/*=========================================================================================
Function 	: HDMITX_LINK_Check_PSTABLE(int uHDMITxBlk)
Description	: Check Pstable
Note		:
Last Updated: 2019/12/25, Kled Chen
==========================================================================================*/
void HDMITX_LINK_Check_PSTABLE(int uHDMITxBlk)
{
	char Pstable_tmp;
	char VT_LO, VT_HI, VDE_LO, VDE_HI, VS_LO, VS_HI, VFP_LO, VFP_HI;
	char HT_LO, HT_HI, HDE_LO, HDE_HI, HS_LO, HS_HI, HFP_LO, HFP_HI;

	VDE_LO = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VDE_LO);
	VDE_HI = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VDE_HI);
	VS_LO = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VSW_LO);
	VS_HI = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VSW_HI);
	VFP_LO = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VFP_LO);
	VFP_HI = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VFP_HI);	
	VT_LO = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_INPUT_VTOTAL_LO);
	VT_HI = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_INPUT_VTOTAL_HI);

	HDE_LO = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_HDE_LO);
	HDE_HI = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_HDE_HI);
	HS_LO = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_HSW_LO);
	HS_HI = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_HSW_HI);
	HFP_LO = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_HFP_LO);
	HFP_HI = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_HFP_HI);
	HT_LO = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_INPUT_HTOTAL_LO);
	HT_HI = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_INPUT_HTOTAL_HI);	

	Pstable_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, ADR_HDMI_SYS_STAT) & BIT0;

	if(Pstable_tmp == 0x01) {
		//disphdmi_debug(("PSTABLE flag is correct \n\n");
		DBG_WRN("[2p1] VTOTAL = 0x%02x%02x, VDE = 0x%02x%02x, VSW = 0x%02x%02x, VFP = 0x%02x%02x \n", VT_HI, VT_LO, VDE_HI, VDE_LO, VS_HI, VS_LO, VFP_HI, VFP_LO);
		DBG_WRN("[2p1] HTOTAL = 0x%02x%02x, HDE = 0x%02x%02x, HSW = 0x%02x%02x, HFP = 0x%02x%02x \n", HT_HI, HT_LO, HDE_HI, HDE_LO, HS_HI, HS_LO, HFP_HI, HFP_LO);
		
	}
	else {
		DBG_WRN("[2p1] PSTABLE flag is failed \n\n");
	}

	

}

#endif

#if 1//phy usage
/*=========================================================================================
Function 	: HDMITX_PHY_P2S_ENABLE(int link, BOOL enable)
Description	: PHY P2S Enable/Disable
Note		: [3:0] : P2S Enable for 4 Lane mode
            : [7:4] : DRV HDMI TX Enable for 4 Lane mode (580 new add)
Last Updated: 2022/09/23
==========================================================================================*/
void HDMITX_PHY_P2S_ENABLE(int link, BOOL enable)
{
	if (enable) {
		hdmitx2p1_write_reg(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG5, 0xFF);
		hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_RESTRIM_CFG1, BIT3, TRUE); //870D[3]=1
	}
	else {
		hdmitx2p1_write_reg(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG5, 0x00);
		hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_RESTRIM_CFG1, BIT3, FALSE); //870D[3]=0
	}
}


/*=========================================================================================
Function 	: HDMITX_PHY_FRLCLK_Setting(int link, int pll_index)
Description	: PHY PLL1 PCLK setting
Note		: use 8707[2], 8707[0], 8705[3:1], 8705[6:4], 8704[7:3], 8708[2:1]
Last Updated: 2022/10/17
==========================================================================================*/
void HDMITX_PHY_FRLCLK_Setting(int link, int pll_index)
{

	hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG3, BIT2, TRUE); //8707[2]=1

	disphdmi_debug(("FRL PLL setting: [%d]\n\r", HDMITxData[link].ucCurrentFRLRate));
	
	switch (HDMITxData[link].ucCurrentFRLRate) {

		case FRL_RATE_12G_4L:
		case FRL_RATE_10G_4L:
		case FRL_RATE_8G_4L:
			/*
			hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG3, BIT0, FALSE); //8707[0]=0			
			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG1, 0x0E, 0x06); //8705[3:1]=011
			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG1, 0x70, 0x30); //8705[6:4]=011
			*/
			hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG3, BIT2, 1);
			hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG3, BIT0, 0);
			hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, 0x00, BIT0, 0);

			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG1, BIT3|BIT2|BIT1, 0x3 << 1);
			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG1, BIT6|BIT5|BIT4, 0x3 << 4);

			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG4, BIT2|BIT1, 0x0 << 0);

			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG0, BIT7|BIT5|BIT6|BIT4|BIT3, 0x13 << 3);

			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG0, BIT2|BIT1, 0x1 << 1);

			hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG0, BIT0, 1);

			hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG4, BIT3, 1);			
			break;

		case FRL_RATE_6G_4L:
		case FRL_RATE_6G_3L:
			/*
			hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG3, BIT0, FALSE); //8707[0]=1
			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG1, 0x0E, 0x06); //8705[3:1]=011
			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG1, 0x70, 0x30); //8705[6:4]=011
			*/

			hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG3, BIT2, 1);
			hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG3, BIT0, 1);
			hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, 0x00, BIT0, 0);

			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG1, BIT3|BIT2|BIT1, 0x3 << 1);
			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG1, BIT6|BIT5|BIT4, 0x3 << 4);

			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG4, BIT2|BIT1, 0x0 << 0);

			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG0, BIT7|BIT5|BIT6|BIT4|BIT3, 0x13 << 3);

			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG0, BIT2|BIT1, 0x1 << 1);

			hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG0, BIT0, 1);

			hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG4, BIT3, 1);
			
			break;

		case FRL_RATE_3G_3L:
			hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG3, BIT0, FALSE); //8707[0]=1
			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG1, 0x0E, 0x04); //8705[3:1]=010
			hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG1, 0x70, 0x20); //8705[6:4]=010
			break;

		default:
			break;
	}
	
}


/*=========================================================================================
Function 	: HDMITX_PHY_TCLK_Setting(int link, int pll_index)
Description	: Output PHY PLL1 / PLL2 TCLK setting
Note		: Use 8784[2], 8784[0], 8782[3:1], 8782[6:4], 8781[7:3], 8785[2:1]
Last Updated: 2022/09/28
==========================================================================================*/
void HDMITX_PHY_TCLK_Setting(int link, int pll_index)
{
	int InputClock;
	char ucDeepColor;
	char ucRepeatition;
	BOOL PLL_VCO_BAND_SEL = 0;	//0x07[2], 0x84[2]
	BOOL PLL_VCO_CAP_EN = 0;	//0x07[0], 0x84[0]
	char PLL_CLKMULT_CTL = 0;	//0x05[3:1], 0x82[3:1]
	char PLL_DIV_PRE = 0;		//0x05[6:4], 0x82[6:4]
	char PLL_DIV_CNT = 0;		//0x04[7:3], 0x81[7:3]
	char PLL_CP = 0;			//0x08[2:1], 0x85[2:1]
	char PLL_TCLKX2_EN = 0;			//0x08[2:1], 0x85[2:1]

	if (HDMITxData[link].bDSCEN == FALSE) {
		HDMITxData[link].InputClock = clock_rate/1000000;
		InputClock = HDMITxData[link].InputClock;
		ucDeepColor = HDMITxData[link].ucDeepColor;
		ucRepeatition = HDMITxData[link].ucRepeatition;
	}
	else {
		InputClock = DSCENCData.ucDSC_TX_Clk;
		ucDeepColor = _8bit_Depth; //DSCENCData.ucDSC_DeepColor;
		if (HDMITxData[link].bHTX21_FRL_Mode == TRUE) {
			ucRepeatition = Repetition1x;
		}
		else {
			ucRepeatition = Repetition2x; //For DSC_ENC + TMDS case
		}
	}
	
	disphdmi_debug(("HTX Input_PCLK = %d\n\r", InputClock));
	//ucDeepColor = HDMITxData[link].ucDeepColor; //_8bit_Depth, _10bit_Depth, _12bit_Depth, _16bit_Depth
	//ucRepeatition = HDMITxData[link].ucRepeatition; //Repetition1x, Repetition2x, Repetition4x

	// Init
	hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_CLK_CFG, BIT4, FALSE); //8700[4]=0
	hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL2_CFG1, BIT0, FALSE); //8782[0]=0
	

	if (ucRepeatition == Repetition4x) {
		//PCLK < 13MHz
		switch (ucDeepColor) {
			case _8bit_Depth:
				PLL_VCO_BAND_SEL = FALSE;
				PLL_VCO_CAP_EN = TRUE;
				PLL_CLKMULT_CTL = 0x00;
				PLL_DIV_PRE = 0x01;
				PLL_DIV_CNT = 0x13;
				PLL_CP = 0x00;
				break;

			case _10bit_Depth:
				PLL_VCO_BAND_SEL = FALSE;
				PLL_VCO_CAP_EN = TRUE;
				PLL_CLKMULT_CTL = 0x00;
				PLL_DIV_PRE = 0x01;
				PLL_DIV_CNT = 0x18;
				PLL_CP = 0x00;
				break;

			case _12bit_Depth:
				PLL_VCO_BAND_SEL = FALSE;
				PLL_VCO_CAP_EN = TRUE;
				PLL_CLKMULT_CTL = 0x00;
				PLL_DIV_PRE = 0x01;
				PLL_DIV_CNT = 0x1D;
				PLL_CP = 0x00;
				break;

			case _16bit_Depth:
				PLL_VCO_BAND_SEL = FALSE;
				PLL_VCO_CAP_EN = TRUE;
				PLL_CLKMULT_CTL = 0x00;
				PLL_DIV_PRE = 0x02;
				PLL_DIV_CNT = 0x13;
				PLL_CP = 0x00;
				break;
		}
		
	}
	else if (ucRepeatition == Repetition2x) {
		//13MHz <= PCLK < 25MHz
		switch (ucDeepColor) {
			case _8bit_Depth:
				PLL_VCO_BAND_SEL = FALSE;
				PLL_VCO_CAP_EN = TRUE;
				PLL_CLKMULT_CTL = 0x01;
				PLL_DIV_PRE = 0x01;
				PLL_DIV_CNT = 0x13;
				PLL_CP = 0x00;
				break;

			case _10bit_Depth:
				PLL_VCO_BAND_SEL = FALSE;
				PLL_VCO_CAP_EN = TRUE;
				PLL_CLKMULT_CTL = 0x01;
				PLL_DIV_PRE = 0x01;
				PLL_DIV_CNT = 0x18;
				PLL_CP = 0x00;
				break;

			case _12bit_Depth:
				PLL_VCO_BAND_SEL = FALSE;
				PLL_VCO_CAP_EN = TRUE;
				PLL_CLKMULT_CTL = 0x01;
				PLL_DIV_PRE = 0x01;
				PLL_DIV_CNT = 0x1D;
				PLL_CP = 0x00;
				break;

			case _16bit_Depth:
				PLL_VCO_BAND_SEL = FALSE;
				PLL_VCO_CAP_EN = TRUE;
				PLL_CLKMULT_CTL = 0x00;
				PLL_DIV_PRE = 0x01;
				PLL_DIV_CNT = 0x13;
				PLL_CP = 0x00;
				break;
		}
	}
	else { //No Repetition
		switch (ucDeepColor) {
			case _8bit_Depth:
				if ((InputClock >= 27) && (InputClock < 54)) { // 27Mhz
					PLL_VCO_BAND_SEL = FALSE;
					PLL_VCO_CAP_EN = TRUE;
					PLL_TCLKX2_EN = TRUE;
					PLL_CLKMULT_CTL = 0x02;
					PLL_DIV_PRE = 0x01;
					PLL_DIV_CNT = 0x13;
					PLL_CP = 0x00;
					
				} else if (InputClock < 100) { // 74.25Mhz
					PLL_VCO_BAND_SEL = FALSE;
					PLL_VCO_CAP_EN = TRUE;
					PLL_TCLKX2_EN = FALSE;
					PLL_CLKMULT_CTL = 0x03;
					PLL_DIV_PRE = 0x01;
					PLL_DIV_CNT = 0x13;
					PLL_CP = 0x00;
				} else if (InputClock < 160) { // 160Mhz
					PLL_VCO_BAND_SEL = FALSE;
					PLL_VCO_CAP_EN = TRUE;
					PLL_TCLKX2_EN = FALSE;
					PLL_CLKMULT_CTL = 0x03;
					PLL_DIV_PRE = 0x01;
					PLL_DIV_CNT = 0x13;
					PLL_CP = 0x00;
				} else if (InputClock < 340) { //297Mhz
					PLL_VCO_BAND_SEL = TRUE;
					PLL_VCO_CAP_EN = TRUE;
					PLL_TCLKX2_EN = FALSE;
					PLL_CLKMULT_CTL = 0x04;
					PLL_DIV_PRE = 0x02;
					PLL_DIV_CNT = 0x13;
					PLL_CP = 0x00;
				} else { // 594Mhz
					PLL_VCO_BAND_SEL = TRUE;
					PLL_VCO_CAP_EN = TRUE;
					PLL_TCLKX2_EN = FALSE;
					PLL_CLKMULT_CTL = 0x04;
					PLL_DIV_PRE = 0x02;
					PLL_DIV_CNT = 0x13;
					PLL_CP = 0x00;					
				}
					disphdmi_debug(("HTX PLL_VCO_BAND_SEL = %d\n\r", PLL_VCO_BAND_SEL));
					disphdmi_debug(("HTX PLL_VCO_CAP_EN = %d\n\r", PLL_VCO_CAP_EN));
					disphdmi_debug(("HTX PLL_TCLKX2_EN = %d\n\r", PLL_TCLKX2_EN));
					disphdmi_debug(("HTX PLL_CLKMULT_CTL = %d\n\r", PLL_CLKMULT_CTL));
					disphdmi_debug(("HTX PLL_DIV_PRE = %d\n\r", PLL_DIV_PRE));
					disphdmi_debug(("HTX PLL_DIV_CNT = %d\n\r", PLL_DIV_CNT));
					disphdmi_debug(("HTX PLL_CP = %d\n\r", PLL_CP));				
				break;

			case _10bit_Depth:
				if ((InputClock >= 27) && (InputClock < 44)) {
					PLL_VCO_BAND_SEL = FALSE;
					PLL_VCO_CAP_EN = TRUE;
					PLL_CLKMULT_CTL = 0x02;
					PLL_DIV_PRE = 0x00;
					PLL_DIV_CNT = 0x18;
					PLL_CP = 0x00;
				}
				else if (InputClock < 145) {
					PLL_VCO_BAND_SEL = FALSE;
					PLL_VCO_CAP_EN = TRUE;
					PLL_CLKMULT_CTL = 0x03;
					PLL_DIV_PRE = 0x01;
					PLL_DIV_CNT = 0x18;
					PLL_CP = 0x00;
				}
				else if (InputClock < 481) {
					PLL_VCO_BAND_SEL = TRUE;
					PLL_VCO_CAP_EN = TRUE;
					PLL_CLKMULT_CTL = 0x04;
					PLL_DIV_PRE = 0x02;
					PLL_DIV_CNT = 0x18;
					PLL_CP = 0x00;
				}
				else if (InputClock < 1081) {
					PLL_VCO_BAND_SEL = TRUE;
					PLL_VCO_CAP_EN = FALSE;
					PLL_CLKMULT_CTL = 0x05;
					PLL_DIV_PRE = 0x03;
					PLL_DIV_CNT = 0x18;
					PLL_CP = 0x00;
				}
				else {
				}
				break;

			case _12bit_Depth:
				if ((InputClock >= 27) && (InputClock < 121)) {
					PLL_VCO_BAND_SEL = FALSE;
					PLL_VCO_CAP_EN = TRUE;
					PLL_CLKMULT_CTL = 0x03;
					PLL_DIV_PRE = 0x01;
					PLL_DIV_CNT = 0x1D;
					PLL_CP = 0x00;
				}
				else if (InputClock < 401) {
					PLL_VCO_BAND_SEL = TRUE;
					PLL_VCO_CAP_EN = TRUE;
					PLL_CLKMULT_CTL = 0x04;
					PLL_DIV_PRE = 0x02;
					PLL_DIV_CNT = 0x1D;
					PLL_CP = 0x00;
				}
				else if (InputClock < 901) {
					PLL_VCO_BAND_SEL = TRUE;
					PLL_VCO_CAP_EN = FALSE;
					PLL_CLKMULT_CTL = 0x04;
					PLL_DIV_PRE = 0x02;
					PLL_DIV_CNT = 0x1D;
					PLL_CP = 0x00;
				}
				else {
				}
				break;

			case _16bit_Depth:
				if ((InputClock >= 27) && (InputClock < 91)) {
					PLL_VCO_BAND_SEL = FALSE;
					PLL_VCO_CAP_EN = TRUE;
					PLL_CLKMULT_CTL = 0x02;
					PLL_DIV_PRE = 0x01;
					PLL_DIV_CNT = 0x13;
					PLL_CP = 0x00;
				}
				else if (InputClock < 301) {
					PLL_VCO_BAND_SEL = TRUE;
					PLL_VCO_CAP_EN = TRUE;
					PLL_CLKMULT_CTL = 0x04;
					PLL_DIV_PRE = 0x03;
					PLL_DIV_CNT = 0x13;
					PLL_CP = 0x00;
				}
				else if (InputClock < 676) {
					PLL_VCO_BAND_SEL = TRUE;
					PLL_VCO_CAP_EN = FALSE;
					PLL_CLKMULT_CTL = 0x04;
					PLL_DIV_PRE = 0x03;
					PLL_DIV_CNT = 0x13;
					PLL_CP = 0x00;
				}
				else {
				}
				break;
		}
	}

	//Write into PLL1 related register
	if (pll_index == PHY_PLL1) {
		hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG3, BIT2, PLL_VCO_BAND_SEL); //8707[2]
		hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG3, BIT0, PLL_VCO_CAP_EN); //8707[0]
		hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG1, 0x0E, PLL_CLKMULT_CTL << 1); //8705[3:1]
		hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG1, 0x70, PLL_DIV_PRE << 4); //8705[6:4]
		hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG0, 0xF8, PLL_DIV_CNT << 3); //8704[7:3]
		hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG4, 0x06, PLL_CP << 1); //8708[2:1]
		hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_CLK_CFG, BIT4, PLL_TCLKX2_EN); //8700[4]=1

	}

	//Write into PLL2 related register
	else if (pll_index == PHY_PLL2) {
		hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL2_CFG3, BIT2, PLL_VCO_BAND_SEL); //8784[2]
		hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL2_CFG3, BIT0, PLL_VCO_CAP_EN); //8784[0]
		hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL2_CFG1, 0x0E, PLL_CLKMULT_CTL << 1); //8782[3:1]
		hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL2_CFG1, 0x70, PLL_DIV_PRE << 4); //8782[6:4]
		hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL2_CFG0, 0xF8, PLL_DIV_CNT << 3); //8781[7:3]
		hdmitx2p1_write_reg_mask(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL2_CFG4, 0x06, PLL_CP << 1); //8785[2:1]
		hdmitx2p1_write_reg_bit(link, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL2_CFG1, BIT0, PLL_TCLKX2_EN); //8782[0]=1
	}

}

/*=========================================================================================
Function 	: HDMITX_PHY_PLL1_ENABLE(int uHDMITxBlk, BOOL enable)
Description	: Enable/Disable PLL1
Note		:
Last Updated: 2022/10/14
==========================================================================================*/
void HDMITX_PHY_PLL1_ENABLE(int uHDMITxBlk, BOOL enable)
{
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG0, BIT0, enable); //8704[0] (1: enable, 0: disable) //PLL1 LDO
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG4, BIT3, enable); //8708[3] (1: enable, 0: disable) //PLL1 Enable/Disable
}

/*=========================================================================================
Function 	: HDMITX_PHY_PLL2_ENABLE(int uHDMITxBlk, BOOL enable)
Description	: Enable/Disable PLL2
Note		:
Last Updated: 2022/10/14
==========================================================================================*/
void HDMITX_PHY_PLL2_ENABLE(int uHDMITxBlk, BOOL enable)
{
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL2_CFG0, BIT0, enable); //8781[0] (1: enable, 0: disable) //PLL2 LDO
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL2_CFG4, BIT3, enable); //8785[3] (1: enable, 0: disable) //PLL2 Enable/Disable
}

/*=========================================================================================
Function 	: HDMITX_PHY_LDO_EN(int uHDMITxBlk, BOOL enable)
Description	: Enable/Disable TX PHY LDO
Note		:
Last Updated: 2022/10/05
==========================================================================================*/
void HDMITX_PHY_LDO_EN(int uHDMITxBlk, BOOL enable)
{
	char value_tmp;
	
	//LDO, ON/OFF
	if(enable) {
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_LDO_CFG0, (BIT1|BIT0), TRUE); //8709[1:0]=11 //TX Termination LDO
		value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_SWAP_CFG1); //8703[6:4]=111 //Power on LDO
		value_tmp |= (BIT6|BIT5|BIT4);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_SWAP_CFG1, value_tmp);
	}
	else {
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_LDO_CFG0, BIT1, FALSE);
		value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_SWAP_CFG1); //8703[6:4]=000 (add by Roger) need to check
		value_tmp &= ~(BIT6|BIT5|BIT4);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_SWAP_CFG1, value_tmp);
	}
}

/*=========================================================================================
Function 	: HDMITX_PHY_BIAS_EN(int uHDMITxBlk, BOOL enable)
Description	: Enable/Disable TX PHY BIAS
Note		:
Last Updated: 2022/10/05
==========================================================================================*/
void HDMITX_PHY_BIAS_EN(int uHDMITxBlk, BOOL enable)
{
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG1, BIT0, enable); //8705[0] (1: enable, 0: disable) //Power on Bias
}

/*=========================================================================================
Function 	: HDMITX_PHY_RES_TRIM(int uHDMITxBlk, char value)
Description	: Set PHY embedded RES Value ==> (VL-trim)
Note		:
			VL-Trim (fine-tune 4 lane swing level)
			register setting by Efuse
			0x870D[3]=1 (Enable VL trim)
			0x870C[4:0] coarse tune (original, same as 500)
			0x870D[2:0] lan0 fine tune (580 new add)
			0x870D[6:4] lan1 fine tune (580 new add)
			0x870E[2:0] lan2 fine tune (580 new add)
			0x870E[6:4] lan3 fine tune (580 new add)
Last Updated: 2022/10/05
==========================================================================================*/
void HDMITX_PHY_RES_TRIM(int uHDMITxBlk, int coarse_value, int L0_fine_value, int L1_fine_value, int L2_fine_value, int L3_fine_value)
{
	int value_tmp = 0x00;

	disphdmi_debug(("%s : coarse_value %d, L0_fine_value %d,  L1_fine_value %d,  L2_fine_value %d,  L3_fine_value %d\r\n",__func__,coarse_value,  L0_fine_value,  L1_fine_value,  L2_fine_value,  L3_fine_value));

	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x0C); //870C[4:0] : coarse tune
	value_tmp &= ~(BIT4|BIT3|BIT2|BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x0C, value_tmp | (coarse_value & 0x1F));

	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x0D); //870D[2:0] : Lane0 fine tune
	value_tmp &= ~(BIT2|BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x0D, (L0_fine_value & 0x07)|value_tmp);

	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x0D); //870D[6:4] : Lane1 fine tune
	value_tmp &= ~(BIT6|BIT5|BIT4);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x0D, ((L1_fine_value & 0x07)<<4)|value_tmp);

	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x0E); //870E[2:0] : Lane2 fine tune
	value_tmp &= ~(BIT2|BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x0E, (L2_fine_value & 0x07)|value_tmp);

	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x0E); //870E[6:4] : Lane1 fine tune
	value_tmp &= ~(BIT6|BIT5|BIT4);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x0E, ((L3_fine_value & 0x07)<<4)|value_tmp);

}

/*=========================================================================================
Function 	: drv_HDMI_Tx_PHY_Load_OTP_Value(void)
Description	: Load OTP value
Note		: N/A
Last Updated: 2020/06/11, P T
==========================================================================================*/
void drv_HDMI_Tx_PHY_Load_OTP_Value(int uHDMITxBlk)
{
}

/*=========================================================================================
Function 	: HDMITX_PHY_TxFFE_Set_byLane_EN(int uHDMITxBlk, BOOL enable)
Description	: TxFFE Lane option
Note		:
Last Updated: 2022/10/15
==========================================================================================*/
void HDMITX_PHY_TxFFE_Set_byLane_EN(int uHDMITxBlk, BOOL enable)
{
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_RST_CFG, BIT2, enable); //8701[2] (0: Set by 4 lane, 1: As Lane0/CK)
}

/*=========================================================================================
Function 	: HDMITX_PHY_Termination_byLane(int uHDMITxBlk, char value)
Description	: TX PHY Termination Value
Note		:
Last Updated: 2022/10/05
==========================================================================================*/
void HDMITX_PHY_Termination_byLane(int uHDMITxBlk, int value)
{
	int value_tmp = 0x00;

 	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG1);
	value_tmp &= ~(BIT4|BIT3|BIT2|BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG1, (value & 0x1F)|value_tmp);  //877C[4:0]: lane 0

 	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG2);
	value_tmp &= ~(BIT4|BIT3|BIT2|BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG2, (value & 0x1F)|value_tmp);	//877D[4:0]: lane 1

 	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG3);
	value_tmp &= ~(BIT4|BIT3|BIT2|BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG3, (value & 0x1F)|value_tmp);	//877E[4:0]: lane 2

 	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG4);
	value_tmp &= ~(BIT4|BIT3|BIT2|BIT1|BIT0);	
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG4, (value & 0x1F)|value_tmp);	//877F[4:0]: lane 3

}

void HDMITX_PHY_Termination_by_each_Lane(int uHDMITxBlk, int lane0, int lane1, int lane2, int lane3)
{
	int value_tmp = 0x00;

 	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG1);
	value_tmp &= ~(BIT4|BIT3|BIT2|BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG1, (lane0 & 0x1F)|value_tmp);  //877C[4:0]: lane 0

 	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG2);
	value_tmp &= ~(BIT4|BIT3|BIT2|BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG2, (lane1 & 0x1F)|value_tmp);	//877D[4:0]: lane 1

 	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG3);
	value_tmp &= ~(BIT4|BIT3|BIT2|BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG3, (lane2 & 0x1F)|value_tmp);	//877E[4:0]: lane 2

 	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG4);
	value_tmp &= ~(BIT4|BIT3|BIT2|BIT1|BIT0);	
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG4, (lane3 & 0x1F)|value_tmp);	//877F[4:0]: lane 3

}



/*=========================================================================================
Function 	: HDMITX_PHY_Maincursor(int uHDMITxBlk, char freq_band, char othervalue)
Description	: Main cursor current control
Note		:
Last Updated: 2022/10/05
==========================================================================================*/
void HDMITX_PHY_Maincursor(int uHDMITxBlk, int freq_band, int othervalue)
{
	int value = 0x00;
	int value_tmp = 0x00;
	
	switch(freq_band)
	{
		case FREQ_BAND_0_148:
			value = 0x00;
			break;

		case FREQ_BAND_148_340:
			value = 0x04;
			break;

		default:
		case FREQ_BAND_340_594:
			value = 0x0F;
			break;

		case FREQ_BAND_OTHER_SETTING:
			value = othervalue;
			break;
	}

 	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG6);
	value_tmp &= ~(BIT3|BIT2|BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG6, value|value_tmp);		//lane 0

 	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG6);
	value_tmp &= ~(BIT7|BIT6|BIT5|BIT4);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG6, (value<<4)|value_tmp);		//lane 1

 	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG7);
	value_tmp &= ~(BIT3|BIT2|BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG7, value|value_tmp);	//lane 2


 	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG7);
	value_tmp &= ~(BIT7|BIT6|BIT5|BIT4);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG7, (value<<4)|value_tmp);		//lane 3

}


/*=========================================================================================
Function 	: HDMITX_PHY_Deemphasis(int uHDMITxBlk, char value)
Description	: TX PHY Deemphasis Value
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
void HDMITX_PHY_Deemphasis(int uHDMITxBlk, int value)
{

	int value_tmp = 0x0;
	
	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG9);
	value_tmp &= ~(BIT2|BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG9, value|value_tmp);		//lane 0
	
	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG10);
	value_tmp &= ~(BIT6|BIT5|BIT4);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG10, (value<<4)|value_tmp);	//lane 3
		
	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG9);
	value_tmp &= ~(BIT6|BIT5|BIT4);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG9, (value<<4)|value_tmp);		//lane 1
	
	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG10);
	value_tmp &= ~(BIT2|BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG10, value|value_tmp);	//lane 2
}

/*=========================================================================================
Function 	: HDMITX_PHY_TxFFE(int uHDMITxBlk, char value)
Description	: TX PHY TxFFE Value
Note		:
Last Updated: 2019/12/23
==========================================================================================*/
void HDMITX_PHY_TxFFE(int uHDMITxBlk)
{
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x96, (BIT3|BIT2|BIT1|BIT0),0xC);
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x96, (BIT7|BIT6|BIT5|BIT4),(0xB << 4));
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x97, (BIT3|BIT2|BIT1|BIT0),0xA);
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x97, (BIT7|BIT6|BIT5|BIT4),(0x9 << 4));
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x92, (BIT7|BIT6|BIT5),(0x2 << 5));
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x93, (BIT7|BIT6|BIT5),(0x4 << 5));
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x94, (BIT7|BIT6|BIT5),(0x5 << 5));
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x95, (BIT7|BIT6|BIT5),(0x6 << 5));
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x92, (BIT1|BIT0),0x3);
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x93, (BIT1|BIT0),0x3);
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x94, (BIT1|BIT0),0x3);
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x95, (BIT1|BIT0),0x3);
}


/*=========================================================================================
Function 	: HDMITX_PHY_Regulator_Output_Voltage(int uHDMITxBlk, char freq_band,char othervalue)
Description	: TX PHY Output Voltage Value
Note		:
Last Updated: 2022/10/05
==========================================================================================*/
void HDMITX_PHY_Regulator_Output_Voltage(int uHDMITxBlk, int freq_band, int othervalue)
{
	char value = 0x00;
	char value_tmp = 0x00;
	char VREF_voltage = 0x00;

	switch(freq_band)
	{
		case FREQ_BAND_0_148:
			value = 0x01;
			VREF_voltage = 0x01;
			break;

		case FREQ_BAND_148_340:
			value = 0x01;
			VREF_voltage = 0x01;
			break;

		case FREQ_BAND_340_594:
			value = 0x01;
			VREF_voltage = 0x01;
			break;

		case FREQ_BAND_OTHER_SETTING: //for FRL
			value = othervalue;
			VREF_voltage = 0x01;
			break;

		default:
			value = 0x01;
			VREF_voltage = 0x01;
			break;

	}

	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_LDO_CFG0);
	value_tmp &= ~(BIT7|BIT6);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_LDO_CFG0, (value<<6)|value_tmp);	//lane 3 & lane 0 (DATA1 ldo vout selection)
	
	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_LDO_CFG0);
	value_tmp &= ~(BIT5|BIT4);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_LDO_CFG0, (value<<4)|value_tmp);	//lane 2 & lane 1 (DATA0 ldo vout selection)
	
	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_LDO_CFG0);
	value_tmp &= ~(BIT3|BIT2);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_LDO_CFG0, (value<<2)|value_tmp);	//(CLK ldo vout selection)
	
	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG0);
	value_tmp &= ~(BIT2|BIT1);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG0, (value<<1)|value_tmp);		//Ring PLL1 (TTX_AVCC_PLL)
	
	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL2_CFG0);
	value_tmp &= ~(BIT2|BIT1);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL2_CFG0, (value<<1)|value_tmp);	//Ring PLL2 (TTX_AVCC_PLL)
	
	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG4); 				//8708[7:6] VREF setting
	value_tmp &= ~(BIT7|BIT6);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_PLL_CFG4, (VREF_voltage<<6)|value_tmp);
}

/*=========================================================================================
Function 	: HDMITX_PHY_Preshoot(int uHDMITxBlk, char value)
Description	: TX PHY Preshoot Value
Note		:
Last Updated: 2022/10/05
==========================================================================================*/
void HDMITX_PHY_Preshoot(int uHDMITxBlk, int value)
{

	int value_tmp = 0x0;

	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG8);
	value_tmp &= ~(BIT1|BIT0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG8, value|value_tmp);	//lane 0 DRV pre-shoot enable
	
	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG8);
	value_tmp &= ~(BIT3|BIT2);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG8, (value<<2)|value_tmp);		//lane 1 DRV pre-shoot enable
	
	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG8);
	value_tmp &= ~(BIT5|BIT4);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG8, (value<<4)|value_tmp);	//lane 2 DRV pre-shoot enable
	
	value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG8);
	value_tmp &= ~(BIT7|BIT6);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG8, (value<<6)|value_tmp);	//lane 3 DRV pre-shoot enable
}

/*=========================================================================================
Function 	: HDMITX_PHY_MODE_SEL(int uHDMITxBlk, char ucMode)
Description	: TX PHY Mode Selection
Note		:
Last Updated: 2022/10/05
==========================================================================================*/
void HDMITX_PHY_MODE_SEL(int uHDMITxBlk, int ucMode)
{
	BOOL FRL_Mode;
	BOOL FRL_SCRAMEBLE;
	char H20_SCRAMBLE;

	if(ucMode == 0) {						//HDMI 1.4
		//disphdmi_debug(("PHY 1.4 Setting \n\n"));
		FRL_Mode = FALSE;
		FRL_SCRAMEBLE = FALSE;
		H20_SCRAMBLE = 0x00;
	}
	else if(ucMode == 1) {				//HDMI 2.0 (SCRAMBLE)
		//disphdmi_debug(("PHY 2.0 SCRAMBLE \n\n"));
		FRL_Mode = FALSE;
		FRL_SCRAMEBLE = FALSE;
		H20_SCRAMBLE = 0x06;
	}
	else if(ucMode == 2) {				//HDMI 2.1 (FRL/SCRAMBLE)
		//disphdmi_debug(("PHY FRL Mode  \n\n"));
		FRL_Mode = TRUE;
		FRL_SCRAMEBLE = TRUE;
		H20_SCRAMBLE = 0x00;
	}
	else {
		FRL_Mode = FALSE;
		FRL_SCRAMEBLE = FALSE;
		H20_SCRAMBLE = 0x00;
	}

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_FRL_CFG0, BIT4, FALSE); //8739[4]=0 (LFSR)
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_FRL_CFG0, BIT5, FALSE); //Fixed at using Ring PLL
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_FRL_CFG0, BIT0, FRL_Mode);
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_FRL_CFG0, BIT1, FRL_SCRAMEBLE);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TG_CFG, H20_SCRAMBLE);	
}

/*=========================================================================================
Function 	: HDMITX_PHY_TOTAL_PLL_CONFIG(int uHDMITxBlk)
Description	: TX PHY PLL Config
Note		:
Last Updated: 2022/09/28
==========================================================================================*/
void HDMITX_PHY_TOTAL_PLL_CONFIG(int uHDMITxBlk)
{
	
	disphdmi_debug(("bLinkBypass = %02X, H21_MODE = %02X, ucColorDepth = %02X\n\r", HDMITxData[uHDMITxBlk].bLinkBypass, HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode, HDMITxData[uHDMITxBlk].ucDeepColor));

	if(HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode == FALSE) {

		HDMITX_PHY_TCLK_Setting(uHDMITxBlk, PHY_PLL1); //TMDS mode : PLL1 is PCLK/TCLK
		HDMITX_PHY_PLL1_ENABLE(uHDMITxBlk, TRUE);			//Enable PLL1
		HDMITX_PHY_PLL2_ENABLE(uHDMITxBlk, FALSE);			//Disable PLL2
	}
	else {

		HDMITX_PHY_FRLCLK_Setting(uHDMITxBlk, PHY_PLL1); //FRL mode : PLL1 is FRLREF_CLK
		HDMITX_PHY_TCLK_Setting(uHDMITxBlk, PHY_PLL2); //FRL mode : PLL2 is PCLK/TCLK
		HDMITX_PHY_PLL1_ENABLE(uHDMITxBlk, TRUE);			//Enable PLL1
		HDMITX_PHY_PLL2_ENABLE(uHDMITxBlk, TRUE);			//Enable PLL2
	}
}

/*=========================================================================================
Function 	: HDMITX_PHY_TOTAL_PLL_ENABLE(int uHDMITxBlk)
Description	: TX PHY PLL Configuration
Note		:
Last Updated: 2022/09/29
==========================================================================================*/
void HDMITX_PHY_TOTAL_PLL_ENABLE(int uHDMITxBlk)
{
	if(HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode == FALSE) {		//HDMI 1.4/2.0 TMDS
		//Only use PLL1
		HDMITX_PHY_PLL1_ENABLE(uHDMITxBlk, TRUE);			//Enable PLL1
		HDMITX_PHY_PLL2_ENABLE(uHDMITxBlk, FALSE);			//Disable PLL2
	}
	else {													//HDMI 2.1 FRL
		//Use PLL1 & PLL2
		HDMITX_PHY_PLL1_ENABLE(uHDMITxBlk, TRUE);			//Enable PLL1
		HDMITX_PHY_PLL2_ENABLE(uHDMITxBlk, TRUE);			//Enable PLL2
	}
	
	DELAY_MS(10);													//Delay 10ms for PLL1/PLL2 stable
}

/*=========================================================================================
Function 	: HDMITX_PHY_POWER_ON(int uHDMITxBlk)
Description	: TX PHY Power ON
Note		:
Last Updated: 2022/09/28
==========================================================================================*/
void HDMITX_PHY_POWER_ON(int uHDMITxBlk)
{
	//--- Wait until MPLL is ready ---//
	
	HDMITX_PHY_TxFFE_Set_byLane_EN(uHDMITxBlk, FALSE);	//Lane Setting

	HDMITX_PHY_BIAS_EN(uHDMITxBlk, TRUE);				//Enable Bias ?? Roger 20220918
	DELAY_MS(1);
	HDMITX_PHY_LDO_EN(uHDMITxBlk, TRUE);					//Enable LDO

	HDMITX_PHY_PRE_DRVER(uHDMITxBlk, TRUE);				//Enable De-Emphasis / Pre-shoot Pre_driver

	drv_HDMI_Tx_PHY_Load_OTP_Value(uHDMITxBlk);				//Load from Efuse
}

/*=========================================================================================
Function 	: HDMITX_PHY_POWER_ON(int uHDMITxBlk)
Description	: TX PHY Power down
Note		:
Last Updated: 2022/10/05
==========================================================================================*/
void HDMITX_PHY_POWER_DOWN(int uHDMITxBlk)
{
	disphdmi_debug((" PHY Power Down Setting  \n\n"));
	
	HDMITX_PHY_P2S_ENABLE(uHDMITxBlk, FALSE);				//Disable P2S
	HDMITX_PHY_Preshoot(uHDMITxBlk, 0x00);					//PRESHOOT Disable (0mA)
	HDMITX_PHY_Deemphasis(uHDMITxBlk, 0x00);				//Disable De-emphasis
	HDMITX_PHY_PRE_DRVER(uHDMITxBlk, FALSE);				//Enable De-Emphasis / Pre-shoot Pre_driver
	HDMITX_PHY_Maincursor(uHDMITxBlk, 0x00, 0x00); 		//Pre-shoot pre-driver (main cursor current) //98 ~ 99
	HDMITX_PHY_Termination_byLane(uHDMITxBlk, 0x00);		//Disable Lane Termination //7C ~ 7F
	HDMITX_PHY_PLL2_ENABLE(uHDMITxBlk, FALSE);				//Disable PPL2
	HDMITX_PHY_PLL1_ENABLE(uHDMITxBlk, FALSE);				//Disable PPL1
	HDMITX_PHY_BIAS_EN(uHDMITxBlk, FALSE);					//Disable Bias
	HDMITX_PHY_LDO_EN(uHDMITxBlk, FALSE);					//Disable LDO
	
}

/*=========================================================================================
Function 	: HDMITX_PHY_RESET(int uHDMITxBlk)
Description	: Rest PHY
Note		:
Last Updated: 2022/10/14, Kled Chen
==========================================================================================*/
void HDMITX_PHY_RESET(int uHDMITxBlk, BOOL enable)
{
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_RST_CFG, BIT0, enable); //8701[0] (1: PHY RESET, 0: PHY Normal Run)
}


/*=========================================================================================
Function 	: HDMITX_PHY_MR_DATA_PERIOD(u8 u8Period)
Description	: 	328 MR, fix 4097 FRL characters issue
				0x3B	
				ltp_data_pat_done=(data_pat_cnt==data_period_init[15:0])
				0: data_period_init[11:0]=12'h000;	
				1: data_period_init[11:0]=12'hFFF;
				[6] reg_ltp_pat_done_option
				[3-0] tpgen_pat_period	
				tpgen counter Number is set as data_period_init[15:12]:	
				0: data_period_init[15:12]=4'h0;	
				1: data_period_init[15:12]=4'h1;	
				0x3B[6]=0&0x3B[3:0]=0x1:12'h000=>16'h1000=4096;(default)	
				0x3B[6]=1&0x3B[3:0]=0x0:12'hFFF=>16'h0FFF=4095; R/W
Note		: N/A
Last Updated: 2020/04/13, Kled Chen
==========================================================================================*/
void HDMITX_PHY_MR_DATA_PERIOD(int uHDMITxBlk, BOOL b4096_Period)
{
	char value_tmp = 0x0;


	if (b4096_Period == TRUE) {		
		// set 4095, hence count from 0 to 4095, total 4096 data period		
		// set reg_ltp_pat_done_option 1				
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_FRL_CFG2, BIT6, TRUE);
		
		// set tpgen_pat_period 0		
 		value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_FRL_CFG2);
		value_tmp &= ~(BIT3|BIT2|BIT1|BIT0);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_FRL_CFG2, value_tmp);			
	}
	else {		
		// set 4096, hence count from 0 to 4096, total 4097 data period		
		// set reg_ltp_pat_done_option 0		
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_FRL_CFG2, BIT6, FALSE);
		
		// set tpgen_pat_period 1	
		value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_FRL_CFG2);
		value_tmp &= ~(BIT3|BIT2|BIT1|BIT0);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_FRL_CFG2, value_tmp|BIT0);			
	}
}


/*=========================================================================================
Function 	: HDMITX_PHY_FRLCLK_BYPASS_MODE(int uHDMITxBlk, char ucInput, BOOL bBYPASS_ENABLE)
Description	: (1) If user enable this function, user sets HDMITX PHY FRLCLK speed in Bypass(Convertor) Mode from (RX2/RX3)
			  (2) If user disable this function, user sets HDMITX PHY FRLCLK comes from MPLL
Note		: User should enable this function in bypass mode
Last Updated: 2019/12/20, Kled Chen
==========================================================================================*/
void HDMITX_PHY_FRLCLK_BYPASS_MODE(int uHDMITxBlk, int ucInput, BOOL bBYPASS_ENABLE)
{
}

/*=========================================================================================
Function 	: HDMITX_PHY_FRL_CLK_SOURCE(int uHDMITxBlk, char uFRL_Rate)
Description	: Set HDMITX PHY FRLCLK speed (3G/6G/8G/10G/12GHz) from MPLL (75M/150M/200M/300MHz/600MHz)
Note		: N/A
Last Updated: 2022/10/05, Kled Chen
==========================================================================================*/
void HDMITX_PHY_FRL_CLK_SOURCE(int uHDMITxBlk, int uFRL_Rate)
{	


	switch (uFRL_Rate) {
		case FRL_RATE_3G_3L:
			disphdmi_debug(("FRL CLK 3GHz\n\n"));
			break;

		case FRL_RATE_6G_3L:
		case FRL_RATE_6G_4L:
			disphdmi_debug(("FRL CLK 6GHz\n\n"));
			break;

		case FRL_RATE_8G_4L:
			disphdmi_debug(("FRL CLK 8GHz\n\n"));
			break;

		case FRL_RATE_10G_4L:
			disphdmi_debug(("FRL CLK 10GHz\n\n"));
			break;

		case FRL_RATE_12G_4L:
			disphdmi_debug(("FRL CLK 12GHz\n\n"));
			break;

		default:
			break;
	}
}

/*=========================================================================================
Function 	: HDMITX_PHY_FRL_CONFIG(int uHDMITxBlk, char uFRL_Rate)
Description	: HTX PHY setting in FRL mode.
			  User sets termination/deemphasis/reg_option/preshoot values.
Note		: N/A
Last Updated: 2022/10/05, Kled Chen
==========================================================================================*/
void HDMITX_PHY_FRL_CONFIG(int uHDMITxBlk, int uFRL_Rate)
{
	int Preshoot_tmp = 0x00;
	
	HDMITX_PHY_MODE_SEL(uHDMITxBlk, 0x02);
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG2, BIT7, TRUE);		//TCLK Output: FRL
	HDMITX_PHY_Termination_byLane(uHDMITxBlk, 0x19);											//Lane Termination
	HDMITX_PHY_Maincursor(uHDMITxBlk, FREQ_BAND_OTHER_SETTING, 0x0B);							//Set Main Cursor Current
	HDMITX_PHY_Deemphasis(uHDMITxBlk, 0x04);												//De-emphasis
	HDMITX_PHY_TxFFE(uHDMITxBlk);

	if ( ( uFRL_Rate == FRL_RATE_6G_3L ) || ( uFRL_Rate == FRL_RATE_3G_3L ) )
		Preshoot_tmp = 0x00;																	//Preshoot 0mA
	else
		Preshoot_tmp = 0x03;																	//Preshoot 2mA

	HDMITX_PHY_Preshoot(uHDMITxBlk, Preshoot_tmp);
	HDMITX_PHY_MR_DATA_PERIOD(uHDMITxBlk, TRUE);												//MR: Data Period needs 4096, 2020/04/13
	//hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_BIAS_CFG0, 0x1D);				//Swing
	HDMITX_PHY_Regulator_Output_Voltage(uHDMITxBlk, FREQ_BAND_OTHER_SETTING, 0x01);			//0.95V	
}

/*=========================================================================================
Function 	: HDMITX_PHY_PCLK_MPLL_FILTER(int uHDMITxBlk, char ucInput, BOOL bMpllFilter_en)
Description	: User set PCLK comes from MPLL filter or original ICP/RX
Note		: N/A
Last Updated: 2019/12/20, Kled Chen
==========================================================================================*/
void HDMITX_PHY_PCLK_MPLL_FILTER(int uHDMITxBlk, int ucInput, BOOL bMpllFilter_en)
{

}

/*=========================================================================================
Function 	: HDMITX_LINK_FRL_TriByteConfig(UINT32 TriByteDiff)
Description	: Fix CTS HFR1-23 Data Metering Issue
Note		:
Last Updated: 2020/04/14, Tim Chien
==========================================================================================*/
void HDMITX_LINK_FRL_TriByteConfig(int uHDMITxBlk, UINT32 TriByteDiff)
{	
	UINT32 s_thld_st = 0, s_rc_lower = 0, s_rc_upper = 0;	
	UINT32 s_rc_upper_in_adv02 = 0, s_rc_upper_in_adv1 = 0, s_rc_upper_in_adv3 = 0;
	
	if (TriByteDiff > 10 && TriByteDiff < 493) {	
		
		g_HDMI21_TriByteDiff = TriByteDiff;		
		
		s_thld_st = 1024;		
		s_rc_upper = s_thld_st + TriByteDiff;		
		s_rc_upper_in_adv02 = s_rc_upper - 8;		
		s_rc_upper_in_adv1 = s_rc_upper - 8;		
		s_rc_upper_in_adv3 = s_rc_upper - 12;		
		s_rc_lower = s_thld_st - TriByteDiff;		
				
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_UPPDER_LO, s_rc_upper);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_UPPDER_HI, s_rc_upper>>8);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_LOWER_LO, s_rc_lower);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_LOWER_HI, s_rc_lower>>8);		
		
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_UPPDER_IN_ADV02_LO, s_rc_upper_in_adv02);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_UPPDER_IN_ADV02_HI, s_rc_upper_in_adv02>>8);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_UPPDER_IN_ADV1_LO, s_rc_upper_in_adv1);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_UPPDER_IN_ADV1_HI, s_rc_upper_in_adv1>>8);	

		hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_UPPDER_IN_ADV3_LO, s_rc_upper_in_adv3);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_UPPDER_IN_ADV3_HI, s_rc_upper_in_adv3>>8);

		//HDMI_MSG_HTX2("Tri-Byte=%d\n", (int)TriByteDiff);
	}	
}


/*=========================================================================================
Function 	: HDMITX_FRL_LINK_RST(int uHDMITxBlk)
Description	: HTX FRL reset
Note		:
Last Updated: 2020/04/07, P T
==========================================================================================*/
void HDMITX_FRL_LINK_RST(int uHDMITxBlk)
{
    hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_Port_CFG, BIT7, TRUE);
    DELAY_MS(1);
    hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_Port_CFG, BIT7, FALSE);
}


/*=========================================================================================
Function 	: HDMITX_LINK_FRL_CONFIG(int uHDMITxBlk, int hdmitx_FRL_rate, BOOL DSC_MODE)
Description	: HTX Link setting in FRL mode, select lane/port and disable HSYNC
Note		: N/A
Last Updated: 2020/03/02, Tim Chien
==========================================================================================*/
void HDMITX_LINK_FRL_CONFIG(int uHDMITxBlk, int hdmitx_FRL_rate, BOOL DSC_MODE)
{
	
	if ((hdmitx_FRL_rate == FRL_RATE_6G_3L) || (hdmitx_FRL_rate == FRL_RATE_3G_3L) || (hdmitx_FRL_rate == FRL_RATE_0p5G_3L)) {
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_LANE_PORT_CFG, BIT1, FALSE);		//Lane : 3
		disphdmi_debug(("3 lane mode \r\n"));
	}
	else {
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_LANE_PORT_CFG, BIT1, TRUE);		//Lane : 4
		disphdmi_debug(("4 lane mode \r\n"));
	}

	if((HDMITxData[uHDMITxBlk].bFRLPortMode) == FALSE) {
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_LANE_PORT_CFG, BIT0, FALSE);		//Port Mode: 1
		disphdmi_debug(("1 port mode \r\n"));
	}
	else {
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_LANE_PORT_CFG, BIT0, TRUE);		//Port Mode: 2
		disphdmi_debug(("2 port mode \r\n"));
	}

	if(DSC_MODE == TRUE) {
		//hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, ADR_HDMI_SYS_CTRL1, BIT4, FALSE);			//Disable HSYNC kled test
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, ADR_HDMI_SYS_CTRL1, BIT4, TRUE);			//Enable HSYNC
		//hdmitx2p1_write_reg_bit60(uHDMITxBlk, HTX_Option_Ctrl_Reg, BIT4, TRUE);							//Gen Fake Hs for HTX link
		disphdmi_debug(("force hsync output \r\n"));
	}
	else {
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, ADR_HDMI_SYS_CTRL1, BIT4, TRUE);			//Enable HSYNC
	}
	
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, ADR_HDMI_SYS_CTRL1, BIT5, TRUE);				//Enable VSYNC

}


/*=========================================================================================
Function 	: HDMITX_PHY_TMDS_CONFIG(int uHDMITxBlk, int input_clk)
Description	: (1) Configure HTX PHY Termination/Deemphasis/Regular Voltage/Current
			  (2) Select PHY HDMI 1.4(Non-Scramble)/2.0(Scramble)
Note		: N/A
Last Updated: 2020/02/20, Kled Chen
==========================================================================================*/
void HDMITX_PHY_TMDS_CONFIG(int uHDMITxBlk, int input_clk)
{

	char hdmitxbw;
	char Termination_tmp = 0;
	char FREQ_BAND_tmp = 0;
	char Deemphasis_tmp = 0;
	char Regular_Voltage_tmp = 0;
	char Preshoot_tmp = 0;
	char Current_tmp = 0;
	char SCDC_DATA_tmp = 0;
	char HDMI_Mode_tmp = 0;

	disphdmi_debug(("Input Clock = %d \r\n", input_clk));

	if(input_clk <= 148) {
		hdmitxbw = CLK_148MHz;
	}
	else if((input_clk > 148) && (input_clk <= 340)) {
		hdmitxbw = CLK_340MHz;
	}
	else {
		hdmitxbw = CLK_594MHz;
	}
	
    switch (hdmitxbw) {
        case CLK_148MHz://HDMI1.4, <148MHz
			disphdmi_debug(("Pixel Clock <= 148Mhz \n\n"));
			Termination_tmp = 0x00;
			FREQ_BAND_tmp = FREQ_BAND_0_148;
			Deemphasis_tmp = 0x00;
			Regular_Voltage_tmp = 0x00;
			Preshoot_tmp = 0x00;
			Current_tmp = 0x1D;
			SCDC_DATA_tmp = 0x00;
			HDMI_Mode_tmp = 0x00;
			break;

        case CLK_340MHz://HDMI1.4, >148MHz, <=340MHz
			disphdmi_debug(("148MHz < Pixel Clock <= 340Mhz \n\n"));
			Termination_tmp = 0x06;
			FREQ_BAND_tmp = FREQ_BAND_148_340;
			Deemphasis_tmp = 0x00;
			Regular_Voltage_tmp = 0x00;
			Preshoot_tmp = 0x00;
			Current_tmp = 0x1D;
			SCDC_DATA_tmp = 0x00;
			HDMI_Mode_tmp = 0x00;
            break;

        case CLK_594MHz://HDMI2.0, >340MHz, <=594MHz
			disphdmi_debug(("340MHz <Pixel Clock <= 594Mhz \n\n"));
			Termination_tmp = 0x19;
			FREQ_BAND_tmp = FREQ_BAND_340_594;
			Deemphasis_tmp = 0x01;
			Regular_Voltage_tmp = 0x00;
			Preshoot_tmp = 0x00;
			Current_tmp = 0xD;
			SCDC_DATA_tmp = 0x03;
			HDMI_Mode_tmp = 0x01;
            break;
    }

	HDMITX_PHY_MODE_SEL(uHDMITxBlk, HDMI_Mode_tmp);
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG2, BIT7, FALSE);		//TCLK Output : TMDS
	HDMITX_PHY_Termination_byLane(uHDMITxBlk, Termination_tmp);								//Lane Termination
	if(hdmitxbw == CLK_594MHz){
		hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_P2S_DRV_CFG4, (BIT4|BIT3|BIT2|BIT1|BIT0),0x04);
		disphdmi_debug(("594Mhz  fine tune  termination lane 3\n\n"));
	}
	
	HDMITX_PHY_Maincursor(uHDMITxBlk, FREQ_BAND_tmp, 0);										//Set Main Cursor Current

	if(hdmitxbw == CLK_594MHz){
		hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG7, (BIT7|BIT6|BIT5|BIT4),(0x02 << 4));
		disphdmi_debug(("594Mhz  fine tune cursor lane 3\n\n"));
	}

	
	HDMITX_PHY_Deemphasis(uHDMITxBlk, Deemphasis_tmp);											//De-emphasis
	
	if(FREQ_BAND_tmp == FREQ_BAND_340_594) {
		hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TXFFE_CFG9, 0x70, 0x10);
		//HDMITX_LINK_SCDC_SCRAMBLE_CONFIG(uHDMITxBlk, TRUE);
	}
	
	HDMITX_PHY_Regulator_Output_Voltage(uHDMITxBlk, FREQ_BAND_tmp, Regular_Voltage_tmp);		//Switch regulator output voltage
	HDMITX_PHY_Preshoot(uHDMITxBlk, Preshoot_tmp); 											//Preshoot
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_BIAS_CFG0, Current_tmp);			//Switch current(TX output swing fine tune)

	//HDMITx_WriteByte_SCDC(uHDMITxBlk, 0x20, SCDC_DATA_tmp);

	if(SCDC_DATA_tmp == 0x03)
	{
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_SCCP_HSYNC_L_DLY, 0x00);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_SCCP_HSYNC_H_DLY, 0x00);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_SCCP_PIXEL_L_DLY, 0x32); 	
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_SCCP_PIXEL_H_DLY, 0x00);
	}
}



/*=========================================================================================
Function 	: HDMITX_PHY_DCC_BYPASS(int uHDMITxBlk, BOOL enable)
Description	: Enable / Disable DCC Bypass
Note		: N/A
Last Updated: 2022/10/27, Kled Chen
==========================================================================================*/
void HDMITX_PHY_DCC_BYPASS(int uHDMITxBlk, BOOL enable)
{
	char value_tmp = 0x00;

	if(enable) { //bypass
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_C_CFG04, BIT0, TRUE);  //87A8[0] REG_BYP_EN[0] for lane0
		value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_C_CFG05);
		value_tmp |= (BIT7|BIT6|BIT5);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_C_CFG05, value_tmp); //87A9[7] REG_BYP_EN[3] for lane3, 87A9[6] REG_BYP_EN[2] for lane2, 87A9[5] REG_BYP_EN[1] for lane1
	}
	else { //non-bypass
		hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_C_CFG04, BIT0, FALSE);  //87A8[0] REG_BYP_EN[0] for lane0
		value_tmp = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_C_CFG05);
		value_tmp &= ~(BIT7|BIT6|BIT5);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_C_CFG05, value_tmp); //87A9[7] REG_BYP_EN[3] for lane3, 87A9[6] REG_BYP_EN[2] for lane2, 87A9[5] REG_BYP_EN[1] for lane1
	}
}

#if 1//!_FPGA_EMULATION_

static void hdmitx2p1_validate_rtrim_VL_TMDS(int uHDMITxBlk)
{
	disphdmi_debug(("%s\r\n",__func__));

	INT32		trim;
	u32		    value;
	BOOL	 is_found;
	UINT32		HDMI_2_1_RESISTOR_TMDS;    //0x3A
	UINT32		HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane0;
	UINT32		HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane1;
	UINT32		HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane2;
	UINT32		HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane3;


	HDMI_2_1_RESISTOR_TMDS = 0x0D;
	HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane0 = 0x04;
	HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane1 = 0x04;
	HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane2 = 0x04;
	HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane3 = 0x04;
	
	// 0x13A (Rough tuning) (where 0x135 = fine tuning)
	// HDMI2.1's TMDS 1.4 / 2.0 VL Trim (rough tune) & HDMI2.1's FRL 2.1 VL Trim  (rough tune)
	// bit[ 4..0]: TMDS 1.4/2.0 VL Trim
	// bit[ 9..5]:		FRL 2.1 VL Trim
	// rough tuning 	
	//===========Caution HDMI2.1's FRL 2.1 VL Trim's fine tune @0x35 & 0x36
	// HDMI2.1's TMDS 1.4 / 2.0 VL Trim (fine tune) @0x35
	// HDMI2.1's		FRL 2.1 VL Trim (fine tune) @0x36
	
	trim = otp_key_manager(EFUSE_ID_1, EFUSE_TRIM_DATA_HDMI_RESISTOR_2_1_PRI);
	if (trim != -33) {
		is_found = extract_trim_valid(trim, (u32 *)&value);

		if(is_found){
			disphdmi_debug(("[2p1] HDMI2.0 & 1.4 resistor Trim data read success addr = 0x%04x\r\n",(int)value));
			//0x13A (Rough tuning) (where 0x135 = fine tuning)
			HDMI_2_1_RESISTOR_TMDS = value & 0x1F;		 // TMDS 1.4/2.0 VL Trim => bit[4..0]										
			DBG_WRN("[2p1]    =>HDMI_2_1_RESISTOR_TMDS = 0x%04x\r\n", (int)HDMI_2_1_RESISTOR_TMDS);					
			//0x8 <= xx <= 0x14
			if (HDMI_2_1_RESISTOR_TMDS < 0x8 || HDMI_2_1_RESISTOR_TMDS > 0x14) {
				DBG_WRN("[2p1]    =>2nd HDMI1.4 resistor 0x8 <= HDMI_2_1_RESISTOR_TMDS[0x%04x] <= 0x14 error\r\n", (int)HDMI_2_1_RESISTOR_TMDS);
				HDMI_2_1_RESISTOR_TMDS = 0x0D; // use default if trim val error
			} else {
				disphdmi_debug(("[2p1]    =>2nd HDMI1.4 resistor Trim data range success 0x8 <= HDMI_2_1_RESISTOR_TMDS[0x%04x] <= 0x14\r\n", (int)HDMI_2_1_RESISTOR_TMDS));
			}
		} else {
			HDMI_2_1_RESISTOR_TMDS = 0x0D; // use default if trim val error
		}
	} else {
		DBG_WRN("[2p1] addr EFUSE_TRIM_DATA_HDMI_RESISTOR_2_1_PRI error %d\r\n", (int)trim);
		HDMI_2_1_RESISTOR_TMDS = 0x0D; // use default if trim val error
	}


	trim = otp_key_manager(EFUSE_ID_1, EFUSE_TRIM_HDMI_2_1_EACH_LANE_FINE_TUNE_VL_1_4_PRI);
	if (trim != -33) {
		is_found = extract_trim_valid(trim, (u32 *)&value);

		if(is_found){
		    disphdmi_debug(("[2p1] HDMI2.1(1.4) fine tune VL Trim data read success addr = 0x%04x\r\n", (int)value));
		    HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane0 = value & 0x7;
		    HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane1 = (value >> 3) & 0x7;
		    HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane2 = (value >> 6) & 0x7;
		    HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane3 = (value >> 9) & 0x7;
		    DBG_WRN("[2p1]    =>HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune: Lane0/1/2/3 = 0x%02x/0x%02x/0x%02x/0x%02x\r\n"
				, (int)HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane0
				, (int)HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane1
				, (int)HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane2
				, (int)HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane3);	
		} else {
			HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane0 = 0x04;
			HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane1 = 0x04;
			HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane2 = 0x04;
			HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane3 = 0x04;		
		}
	} else {
	    DBG_WRN("[2p1]addr EFUSE_TRIM_HDMI_2_1_EACH_LANE_FINE_TUNE_VL_1_4_PRI error %d\r\n",(int)trim);
		HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane0 = 0x04;
		HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane1 = 0x04;
		HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane2 = 0x04;
		HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane3 = 0x04;// use default if trim val error
	}	


	HDMITX_PHY_RES_TRIM(uHDMITxBlk, 
		HDMI_2_1_RESISTOR_TMDS, 
		HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane0, 
		HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane1, 
		HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane2, 
		HDMI2_1_LANE_VL_1_4_and_2_0_fine_tune_lane3); // VL trim

}

static void hdmitx2p1_validate_rtrim_VL_FRL(int uHDMITxBlk)
{

	disphdmi_debug(("%s\r\n",__func__));

	INT32		trim;
	u32			value;
	BOOL	 is_found;
	UINT32		HDMI_2_1_RESISTOR_FRL;	   //0x3A
	UINT32		HDMI2_1_FRL_2_1_VL_fine_tune_lane0;
	UINT32		HDMI2_1_FRL_2_1_VL_fine_tune_lane1;
	UINT32		HDMI2_1_FRL_2_1_VL_fine_tune_lane2;
	UINT32		HDMI2_1_FRL_2_1_VL_fine_tune_lane3;


	HDMI_2_1_RESISTOR_FRL = 0x0D;
	HDMI2_1_FRL_2_1_VL_fine_tune_lane0 = 0x04;
	HDMI2_1_FRL_2_1_VL_fine_tune_lane1 = 0x04;
	HDMI2_1_FRL_2_1_VL_fine_tune_lane2 = 0x04;
	HDMI2_1_FRL_2_1_VL_fine_tune_lane3 = 0x04;
	
	// 0x13A (Rough tuning) (where 0x135 = fine tuning)
	// HDMI2.1's TMDS 1.4 / 2.0 VL Trim (rough tune) & HDMI2.1's FRL 2.1 VL Trim  (rough tune)
	// bit[ 4..0]: TMDS 1.4/2.0 VL Trim
	// bit[ 9..5]:		FRL 2.1 VL Trim
	// rough tuning 	
	//===========Caution HDMI2.1's FRL 2.1 VL Trim's fine tune @0x35 & 0x36
	// HDMI2.1's TMDS 1.4 / 2.0 VL Trim (fine tune) @0x35
	// HDMI2.1's		FRL 2.1 VL Trim (fine tune) @0x36
	
	trim = otp_key_manager(EFUSE_ID_1, EFUSE_TRIM_DATA_HDMI_RESISTOR_2_1_PRI);
	if (trim != -33) {
		is_found = extract_trim_valid(trim, (u32 *)&value);

		if(is_found){
			disphdmi_debug(("[2p1] HDMI2.0 & 1.4 resistor Trim data read success addr = 0x%04x\r\n", (int)value));
			//0x13A (Rough tuning) (where 0x135 = fine tuning)				
			HDMI_2_1_RESISTOR_FRL = (value >> 5)& 0x1F;  // 	 FRL 2.1 VL Trim => bit[9..5]										
			DBG_WRN("[2p1]    => HDMI_2_1_RESISTOR_FRL = 0x%04x\r\n", (int)HDMI_2_1_RESISTOR_FRL);					
			//0x8 <= xx <= 0x14
			if (HDMI_2_1_RESISTOR_FRL < 0x8 || HDMI_2_1_RESISTOR_FRL > 0x14) {
				DBG_WRN("[2p1]    =>2nd HDMI1.4 resistor 0x8 <=  HDMI_2_1_RESISTOR_FRL[0x%04x] <= 0x14 error\r\n", (int)HDMI_2_1_RESISTOR_FRL);
				HDMI_2_1_RESISTOR_FRL = 0x0D;// use default if trim val error
			} else {
				disphdmi_debug(("[2p1]    =>2nd HDMI1.4 resistor Trim data range success 0x8 <=  HDMI_2_1_RESISTOR_FRL[0x%04x] <= 0x14\r\n", (int)HDMI_2_1_RESISTOR_FRL));
			}
		} else {
			HDMI_2_1_RESISTOR_FRL = 0x0D;// use default if trim val error
		}
	} else {
		DBG_WRN("[2p1] addr EFUSE_TRIM_DATA_HDMI_RESISTOR_2_1_PRI error %d\r\n", (int)trim);
		HDMI_2_1_RESISTOR_FRL = 0x0D;// use default if trim val error
	}


	trim = otp_key_manager(EFUSE_ID_1, EFUSE_TRIM_HDMI_2_1_EACH_LANE_FINE_TUNE_VL_2_0_PRI);
	if (trim != -33) {	
		is_found = extract_trim_valid(trim, (u32 *)&value);

		if(is_found){
		    disphdmi_debug(("[2p1]  HDMI2.1(2.0) fine tune VL Trim data read success addr = 0x%04x\r\n", (int)value));
		    HDMI2_1_FRL_2_1_VL_fine_tune_lane0 = value & 0x7;
		    HDMI2_1_FRL_2_1_VL_fine_tune_lane1 = (value >> 3) & 0x7;
		    HDMI2_1_FRL_2_1_VL_fine_tune_lane2 = (value >> 6) & 0x7;
		    HDMI2_1_FRL_2_1_VL_fine_tune_lane3 = (value >> 9) & 0x7;			
		    DBG_WRN("[2p1] HDMI2_1_LANE_VL_2_1_fine_tune: Lane0/1/2/3 = 0x%02x/0x%02x/0x%02x/0x%02x\r\n"
				, (int)HDMI2_1_FRL_2_1_VL_fine_tune_lane0
				, (int)HDMI2_1_FRL_2_1_VL_fine_tune_lane1
				, (int)HDMI2_1_FRL_2_1_VL_fine_tune_lane2
				, (int)HDMI2_1_FRL_2_1_VL_fine_tune_lane3);	
		} else {
			HDMI2_1_FRL_2_1_VL_fine_tune_lane0 = 0x04;
			HDMI2_1_FRL_2_1_VL_fine_tune_lane1 = 0x04;
			HDMI2_1_FRL_2_1_VL_fine_tune_lane2 = 0x04;
			HDMI2_1_FRL_2_1_VL_fine_tune_lane3 = 0x04;		
		}
	} else {
	    DBG_WRN("[2p1]addr EFUSE_TRIM_HDMI_2_1_EACH_LANE_FINE_TUNE_VL_2_0_PRI error %d\r\n", (int)trim);
		HDMI2_1_FRL_2_1_VL_fine_tune_lane0 = 0x04;
		HDMI2_1_FRL_2_1_VL_fine_tune_lane1 = 0x04;
		HDMI2_1_FRL_2_1_VL_fine_tune_lane2 = 0x04;
		HDMI2_1_FRL_2_1_VL_fine_tune_lane3 = 0x04;
	}	


	HDMITX_PHY_RES_TRIM(uHDMITxBlk, 
		HDMI_2_1_RESISTOR_FRL, 
		HDMI2_1_FRL_2_1_VL_fine_tune_lane0, 
		HDMI2_1_FRL_2_1_VL_fine_tune_lane1, 
		HDMI2_1_FRL_2_1_VL_fine_tune_lane2, 
		HDMI2_1_FRL_2_1_VL_fine_tune_lane3); // VL trim

}

static void hdmitx2p1_validate_rtrim_Termination(int uHDMITxBlk)
{

	//HDMI2.1  Rterm Trim Lane 0
	//bit[ 4..0]: Rterm Trim Lane 0
	INT32     trim;
	u32	     value;
	BOOL	 is_found;
	UINT32    HDMI2_1_Rterm_trim_lane0;
	UINT32    HDMI2_1_Rterm_trim_lane1;
	UINT32    HDMI2_1_Rterm_trim_lane2;
	UINT32    HDMI2_1_Rterm_trim_lane3;

	disphdmi_debug(("%s\r\n",__func__));

	HDMI2_1_Rterm_trim_lane0 = 0x19;
	HDMI2_1_Rterm_trim_lane1 = 0x19;
	HDMI2_1_Rterm_trim_lane2 = 0x19;
	HDMI2_1_Rterm_trim_lane3 = 0x19;
	
	
	

	trim = otp_key_manager(EFUSE_ID_1, EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE0_8G_PRI);
	if (trim != -33) {
		is_found = extract_trim_valid(trim, (u32 *)&value);

		if(is_found){
		    disphdmi_debug(("[2p1] HDMI2.1 Lane0 Term 8b (8G) Trim data read success = 0x%04x\r\n", (int)value));
		    // bit[ 4..0]: Rterm Trim Lane 0
		    HDMI2_1_Rterm_trim_lane0 = value & 0x1F;
		} else {
			HDMI2_1_Rterm_trim_lane0 = 0x19;
		}
	} else {
	    DBG_WRN("[2p1] EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE0_8G_PRI error %d\r\n",(int)trim);
	    HDMI2_1_Rterm_trim_lane0 = 0x19;
	}

	trim = otp_key_manager(EFUSE_ID_1, EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE1_8G_PRI);
	if (trim != -33) {	
		is_found = extract_trim_valid(trim, (u32 *)&value);

		if(is_found){		
		    disphdmi_debug(("[2p1] HDMI2.1 Lane1 Term 8b (8G) Trim data read success = 0x%04x\r\n", (int)value));
		    // bit[ 4..0]: Rterm Trim Lane 1
		    HDMI2_1_Rterm_trim_lane1 = value & 0x1F;	
		} else {
			 HDMI2_1_Rterm_trim_lane1 = 0x19;
		}
	} else {
	    DBG_WRN("[2p1] EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE1_8G_PRI error %d\r\n",(int)trim);
	    HDMI2_1_Rterm_trim_lane1 = 0x19;
	}

	trim = otp_key_manager(EFUSE_ID_1, EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE2_8G_PRI);
	if (trim != -33) {
		is_found = extract_trim_valid(trim, (u32 *)&value);

		if(is_found){			
		    disphdmi_debug(("[2p1] HDMI2.1 Lane2 Term 8b (8G) Trim data read success = 0x%04x\r\n", (int)value));
		    // bit[ 4..0]: Rterm Trim Lane 2
		    HDMI2_1_Rterm_trim_lane2 = value & 0x1F;
		} else {
			HDMI2_1_Rterm_trim_lane2 = 0x19;
		}
	} else {
	    DBG_WRN("[2p1] EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE2_8G_PRI error %d\r\n",(int)trim);
	    HDMI2_1_Rterm_trim_lane2 = 0x19;
	}

	trim = otp_key_manager(EFUSE_ID_1, EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE3_8G_PRI);
	if (trim != -33) {
		is_found = extract_trim_valid(trim, (u32 *)&value);

		if(is_found){		
		    disphdmi_debug(("[2p1] HDMI2.1 Lane3 Term 8b (8G) Trim data read success = 0x%04x\r\n", (int)value));
		    // bit[ 4..0]: Rterm Trim Lane 3
		    HDMI2_1_Rterm_trim_lane3 = value & 0x1F;
		} else {
			HDMI2_1_Rterm_trim_lane3 = 0x19;
		}
	} else {
	    DBG_WRN("[2p1] EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE3_8G_PRI error %d\r\n",(int)trim);
	    HDMI2_1_Rterm_trim_lane3 = 0x19;
	}

	DBG_WRN("[2p1] Rterm_trim_lane0/1/2/3 = 0x%04x/0x%04x/0x%04x/0x%04x\r\n"
		, (int)HDMI2_1_Rterm_trim_lane0
		, (int)HDMI2_1_Rterm_trim_lane1
		, (int)HDMI2_1_Rterm_trim_lane2
		, (int)HDMI2_1_Rterm_trim_lane3);

	HDMITX_PHY_Termination_by_each_Lane(uHDMITxBlk,
		HDMI2_1_Rterm_trim_lane0,
		HDMI2_1_Rterm_trim_lane1,
		HDMI2_1_Rterm_trim_lane2,
		HDMI2_1_Rterm_trim_lane3);


}

static void hdmitx2p1_validate_rtrim_DCC(int uHDMITxBlk)
{

	//HDMI2.1 8G DCC Trim Lane 0
	//bit[ 8..5]: 8G DCC Trim Lane 0 INC
	//bit[12..9]: 8G DCC Trim Lane 0 DEC
	INT32     trim;
	u32	     value;
	BOOL  is_found;
	
	UINT32    HDMI2_1_8G_DCC_trim_INC_lane0 = 0;
	UINT32    HDMI2_1_8G_DCC_trim_DEC_lane0 = 0;
	UINT32    HDMI2_1_8G_DCC_trim_INC_lane1 = 0;
	UINT32    HDMI2_1_8G_DCC_trim_DEC_lane1 = 0;
	UINT32    HDMI2_1_8G_DCC_trim_INC_lane2 = 0;
	UINT32    HDMI2_1_8G_DCC_trim_DEC_lane2 = 0;
	UINT32    HDMI2_1_8G_DCC_trim_INC_lane3 = 0;
	UINT32    HDMI2_1_8G_DCC_trim_DEC_lane3 = 0;

	UINT32 ret_1, ret_2;
	UINT32 val_1, val_2;

	disphdmi_debug(("%s\r\n",__func__));
	
	trim = otp_key_manager(EFUSE_ID_1, EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE0_8G_PRI);
	if (trim != -33) {
		is_found = extract_trim_valid(trim, (u32 *)&value);

		if(is_found){
		    disphdmi_debug(("[2p1] HDMI2.1 Lane0 Term 8b (8G) Trim data read success addr EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE0_8G_PRI = 0x%04x\r\n", (int)value));
		    // bit[ 8..5]: 8G DCC Trim Lane 0 INC
		    HDMI2_1_8G_DCC_trim_INC_lane0 = ((value >> 5) & 0xF);
		    // bit[12..9]: 8G DCC Trim Lane 0 DEC
		    HDMI2_1_8G_DCC_trim_DEC_lane0 = ((value >> 9) & 0xF);	
		}
	} else {
	    DBG_WRN("[2p1]addr EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE0_8G_PRI error %d\r\n",(int)trim);
	}

	trim = otp_key_manager(EFUSE_ID_1, EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE1_8G_PRI);
	if (trim != -33) {
		is_found = extract_trim_valid(trim, (u32 *)&value);

		if(is_found){		
		    disphdmi_debug(("[2p1] HDMI2.1 Lane1 Term 8b (8G) Trim data read success addr EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE1_8G_PRI = 0x%04x\r\n", (int)value));
		    // bit[ 8..5]: 8G DCC Trim Lane 1 INC
		    HDMI2_1_8G_DCC_trim_INC_lane1 = ((value >> 5) & 0xF);
		    // bit[12..9]: 8G DCC Trim Lane 1 DEC
		    HDMI2_1_8G_DCC_trim_DEC_lane1 = ((value >> 9) & 0xF);
		}
	} else {
	    DBG_WRN("[2p1]addr EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE1_8G_PRI error %d\r\n",(int)trim);
	}
	
	trim = otp_key_manager(EFUSE_ID_1, EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE2_8G_PRI);
	if (trim != -33) {
		is_found = extract_trim_valid(trim, (u32 *)&value);

		if(is_found){		
		    disphdmi_debug(("[2p1] HDMI2.1 Lane2 Term 8b (8G) Trim data read success addr EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE2_8G_PRI = 0x%04x\r\n", (int)value));
		    // bit[ 8..5]: 8G DCC Trim Lane 2 INC
		    HDMI2_1_8G_DCC_trim_INC_lane2 = ((value >> 5) & 0xF);
		    // bit[12..9]: 8G DCC Trim Lane 2 DEC
		    HDMI2_1_8G_DCC_trim_DEC_lane2 = ((value >> 9) & 0xF);
		}
	} else {
	    DBG_WRN("[2p1]addr EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE2_8G_PRI error %d\r\n",(int)trim);
	}

	trim = otp_key_manager(EFUSE_ID_1, EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE3_8G_PRI);
	if (trim != -33) {	
		is_found = extract_trim_valid(trim, (u32 *)&value);

		if(is_found){		
		    disphdmi_debug(("[2p1] HDMI2.1 Lane3 Term 8b (8G) Trim data read success addr EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE3_8G_PRI = 0x%04x\r\n", (int)value));
		    // bit[ 8..5]: 8G DCC Trim Lane 3 INC
		    HDMI2_1_8G_DCC_trim_INC_lane3 = ((value >> 5) & 0xF);
		    // bit[12..9]: 8G DCC Trim Lane 3 DEC
		    HDMI2_1_8G_DCC_trim_DEC_lane3 = ((value >> 9) & 0xF);	
		}
	} else {
	    DBG_WRN("[2p1]addr EFUSE_TRIM_HDMI_2_1_TERM_DCC_LANE1_8G_PRI error %d\r\n",(int)trim);
	}

	DBG_WRN("[2p1] DCC INC lane0/1/2/3 = 0x%04x/0x%04x/0x%04x/0x%04x\r\n"
		,(int)HDMI2_1_8G_DCC_trim_INC_lane0
		,(int)HDMI2_1_8G_DCC_trim_INC_lane1
		,(int)HDMI2_1_8G_DCC_trim_INC_lane2
		,(int)HDMI2_1_8G_DCC_trim_INC_lane3);

	DBG_WRN("[2p1] DCC DEC lane0/1/2/3 = 0x%04x/0x%04x/0x%04x/0x%04x\r\n"
		,(int)HDMI2_1_8G_DCC_trim_DEC_lane0
		,(int)HDMI2_1_8G_DCC_trim_DEC_lane1
		,(int)HDMI2_1_8G_DCC_trim_DEC_lane2
		,(int)HDMI2_1_8G_DCC_trim_DEC_lane3);	

	/* Apply Lane 0 DCC trim value flow start*/
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA9, 0x0);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA8, 0x2C);

	/* Lane 0 value*/

	val_2 = HDMI2_1_8G_DCC_trim_DEC_lane0;
	val_1 = HDMI2_1_8G_DCC_trim_INC_lane0;

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA1, val_2);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA2, val_1);

	/* Update Value*/
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA0, 0x11);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA0, 0x31);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA0, 0x11);

	/* Wait update done*/

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0xA7, BIT7, 0x1);

	while(!(hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL ,0xA9) & BIT0)){
		DELAY_MS(1);
		disphdmi_debug(("[2p1] DCC lane 0 wait done [%d]\r\n",hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL ,0xA9)));
		dcc_timeout_cnt++;
		if(dcc_timeout_cnt > 1000) break;		
	}
	dcc_timeout_cnt = 0;
	
	/* Read DCC lane 0 value*/
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x39, 0x23);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xAA, 0x1);

	ret_1 = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x3E) & (BIT0|BIT1|BIT2|BIT3);

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xAA, 0x2);

	ret_2 = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x3E) & (BIT0|BIT1|BIT2|BIT3);

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA8, 0x28);

	if((ret_1 != val_1) || (ret_2 != val_2)){
		DBG_WRN("[2p1] DCC lane 0 auto-K fail [%d][%d] != [%d][%d]\r\n",ret_1,ret_2,val_1,val_2);
	} else {
		DBG_WRN("[2p1] DCC lane 0 auto-K pass [%d][%d] == [%d][%d]\r\n",ret_1,ret_2,val_1,val_2);
	}


	/* Apply Lane 1 DCC trim value flow start*/
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA9, 0x4);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x01, 0x28);

	/* Lane 1 value*/
	
	val_2 = HDMI2_1_8G_DCC_trim_DEC_lane1;
	val_1 = HDMI2_1_8G_DCC_trim_INC_lane1;

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA1, val_2);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA2, val_1);

	/* Update Value*/
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA0, 0x11);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA0, 0x31);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA0, 0x11);

	/* Wait update done*/

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0xA7, BIT7, 0x1);

	while(!(hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL ,0xA9) & BIT1)){
		DELAY_MS(1);
		disphdmi_debug(("[2p1] DCC lane 1 wait done [%d]\r\n",hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL ,0xA9)));
		dcc_timeout_cnt++;
		if(dcc_timeout_cnt > 1000) break;
	}
	dcc_timeout_cnt = 0;

	/* Read DCC lane 1 value*/
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x39, 0x23);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xAA, 0x1);

	ret_1 = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x3E) & (BIT0|BIT1|BIT2|BIT3);

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xAA, 0x2);

	ret_2 = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x3E) & (BIT0|BIT1|BIT2|BIT3);

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x01, 0x8);

	if((ret_1 != val_1) || (ret_2 != val_2)){
		DBG_WRN("[2p1] DCC lane 1 auto-K fail [%d][%d] != [%d][%d]\r\n",ret_1,ret_2,val_1,val_2);
	} else {
		DBG_WRN("[2p1] DCC lane 1 auto-K pass [%d][%d] == [%d][%d]\r\n",ret_1,ret_2,val_1,val_2);
	} 


	/* Apply Lane 2 DCC trim value flow start*/
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA9, 0x8);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x01, 0x48);

	/* Lane 2 value*/

	val_2 = HDMI2_1_8G_DCC_trim_DEC_lane2;
	val_1 = HDMI2_1_8G_DCC_trim_INC_lane2;

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA1, val_2);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA2, val_1);

	/* Update Value*/
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA0, 0x11);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA0, 0x31);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA0, 0x11);

	/* Wait update done*/

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0xA7, BIT7, 0x1);

	while(!(hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL ,0xA9) & BIT1)){
		DELAY_MS(1);
		disphdmi_debug(("[2p1] DCC lane 2 wait done [%d]\r\n",hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL ,0xA9)));
		dcc_timeout_cnt++;
		if(dcc_timeout_cnt > 1000) break;		
	}
	dcc_timeout_cnt = 0;

	/* Read DCC lane 1 value*/
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x39, 0x23);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xAA, 0x1);

	ret_1 = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x3E) & (BIT0|BIT1|BIT2|BIT3);

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xAA, 0x2);

	ret_2 = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x3E) & (BIT0|BIT1|BIT2|BIT3);

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x01, 0x8);

	if((ret_1 != val_1) || (ret_2 != val_2)){
		DBG_WRN("[2p1] DCC lane 2 auto-K fail [%d][%d] != [%d][%d]\r\n",ret_1,ret_2,val_1,val_2);
	} else {
		DBG_WRN("[2p1] DCC lane 2 auto-K pass [%d][%d] == [%d][%d]\r\n",ret_1,ret_2,val_1,val_2);
	}


	/* Apply Lane 3 DCC trim value flow start*/
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA9, 0xC);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x01, 0x88);

	/* Lane 3 value*/

	val_2 = HDMI2_1_8G_DCC_trim_DEC_lane3;
	val_1 = HDMI2_1_8G_DCC_trim_INC_lane3;

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA1, val_2);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA2, val_1);

	/* Update Value*/
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA0, 0x11);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA0, 0x31);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xA0, 0x11);

	/* Wait update done*/

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0xA7, BIT7, 0x1);

	while(!(hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL ,0xA9) & BIT1)){
		DELAY_MS(1);
		disphdmi_debug(("[2p1] DCC lane 3 wait done [%d]\r\n",hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL ,0xA9)));
		dcc_timeout_cnt++;
		if(dcc_timeout_cnt > 1000) break;		
	}
	dcc_timeout_cnt = 0;

	/* Read DCC lane 3 value*/
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x39, 0x23);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xAA, 0x1);

	ret_1 = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x3E) & (BIT0|BIT1|BIT2|BIT3);

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0xAA, 0x2);

	ret_2 = hdmitx2p1_read_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x3E) & (BIT0|BIT1|BIT2|BIT3);

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x01, 0x8);

	if((ret_1 != val_1) || (ret_2 != val_2)){
		DBG_WRN("[2p1] DCC lane 3 auto-K fail [%d][%d] != [%d][%d]\r\n",ret_1,ret_2,val_1,val_2);
	} else {
		DBG_WRN("[2p1] DCC lane 3 auto-K pass [%d][%d] == [%d][%d]\r\n",ret_1,ret_2,val_1,val_2);
	}



}
#endif


static void hdmitx2p1_validate_rtrim(int uHDMITxBlk)
{

	disphdmi_debug(("%s\r\n",__func__));
#if !_FPGA_EMULATION_	
#if HDMI_EFUSE_USED	

	/* VL part of trim*/
	if(HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode){
		hdmitx2p1_validate_rtrim_VL_FRL(uHDMITxBlk);
	} else {
		hdmitx2p1_validate_rtrim_VL_TMDS(uHDMITxBlk);
	}

	/* Termination Resistor part of trim (FRL mode only)*/
	if(HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode){
		hdmitx2p1_validate_rtrim_Termination(uHDMITxBlk);
	}

	/* DCC part of trim (FRL mode only)*/
	if(HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode){
		hdmitx2p1_validate_rtrim_DCC(uHDMITxBlk);
	}	
#endif
#endif

}


static void hdmitx2p1_hotfix(void)
{
	disphdmi_debug(("%s\r\n",__func__));
}

void hdmitx2p1_init_phy_TMDS(int uHDMITxBlk)
{

	disphdmi_debug(("%s\r\n",__func__));

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0x39, BIT0, 0x0); //HDMI_2P1
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0x39, BIT1, 0x0); //FRL_SCRAMBLE_EN
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0x30, BIT1, 0x0); //HDMI_2P0
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0x30, BIT2, 0x0); //SCRAMBLE_EN
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0x7D, BIT7, 0x0); //REG_FRL_TCLK_EN

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0xA8, BIT0, 0x1); //REG_BYP_EN_A[0]=1
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0xA9, BIT5, 0x1); //REG_BYP_EN_A[1]=1
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0xA9, BIT6, 0x1); //REG_BYP_EN_A[2]=1
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0xA9, BIT7, 0x1); //REG_BYP_EN_A[3]=1
	
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0x01, BIT2, 0x0); //BY_LANE_OPTION=0(set by 4 lane)
	
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x08, 0xC0, 0x40); //reg_rgbg[1:0] = 1
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x09, 0x0C, 0x04); //reg_rgldo_0[1:0] = 1
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x09, 0x30, 0x10); //reg_rgldo_1[1:0] = 1
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x09, 0xC0, 0x40); //reg_rgldo_2[1:0] = 1

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0x09, BIT0, 0x1); //REG_VBGSEL=1
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0x09, BIT1, 0x1); //REG_BGR_EN=1
	hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_PHY_CTRL, 0x03, 0x70, 0x70); //REG_PDN_LDO[2:0]=3'b111

	DELAY_MS(1);

	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0x05, BIT0, 0x1); //REG_LVCKT_EN=1
	hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, 0x01, BIT0, 0x1); //TTX_RST=1

	HDMITX_PHY_TOTAL_PLL_CONFIG(uHDMITxBlk);

	DELAY_MS(10);

	HDMITX_PHY_RESET(uHDMITxBlk, 0x0);

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, 0x0B, 0x1D); //reg_sw_ctl[5:0] 

	hdmitx2p1_validate_rtrim(uHDMITxBlk); // trim value apply

	HDMITX_PHY_TMDS_CONFIG(uHDMITxBlk,HDMITxData[uHDMITxBlk].InputClock);

	HDMITX_PHY_PRE_DRVER(uHDMITxBlk, TRUE);

	HDMITX_PHY_P2S_ENABLE(uHDMITxBlk, TRUE);
	disphdmi_debug(("%s init done\r\n",__func__));

	DBG_WRN("[2p1] TMDS phy init with clk rate [%d]Mhz\r\n",HDMITxData[uHDMITxBlk].InputClock);
}

void hdmitx2p1_init_phy_FRL(int uHDMITxBlk)
{
	disphdmi_debug(("%s\r\n",__func__));

	HDMITX_PHY_FRL_CLK_SOURCE(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucCurrentFRLRate);

	HDMITX_PHY_FRL_CONFIG(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucCurrentFRLRate);

	if(HDMITxData[uHDMITxBlk].InputClock > 100) {
		g_HDMI21_TriByteDiff = TX_TriByte_H;
		}else {
		g_HDMI21_TriByteDiff = TX_TriByte_L;
	}

	HDMITX_LINK_FRL_TriByteConfig(uHDMITxBlk, g_HDMI21_TriByteDiff);	

	HDMITX_PHY_POWER_ON(uHDMITxBlk);


	HDMITX_PHY_RESET(uHDMITxBlk, TRUE);

	DELAY_MS(1);
	HDMITX_PHY_TOTAL_PLL_CONFIG(uHDMITxBlk);
	DELAY_MS(10);

	HDMITX_PHY_RESET(uHDMITxBlk, FALSE);

	HDMITX_PHY_DCC_BYPASS(uHDMITxBlk, FALSE);

	hdmitx2p1_validate_rtrim(uHDMITxBlk); // trim value apply

	HDMITX_PHY_P2S_ENABLE(uHDMITxBlk, TRUE);	
	
	
	disphdmi_debug(("%s init done\r\n",__func__));
	DBG_WRN("[2p1] FRL phy init with clk rate [%d]Mhz\r\n",HDMITxData[uHDMITxBlk].InputClock);
}




/*
    hdmitx2p1 2p1 phy initialization
*/
static void hdmitx2p1_init_phy(int uHDMITxBlk)
{
	disphdmi_debug(("%s\r\n",__func__));


	if(HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode){
		hdmitx2p1_init_phy_FRL(uHDMITxBlk);
	} else {
		hdmitx2p1_init_phy_TMDS(uHDMITxBlk);
	}	

	hdmitx2p1_hotfix();
}

#endif


#if 1//main config flow

/*=========================================================================================
Function 	: HDMITX_CONFIG_STATE(int uHDMITxBlk, char uHDMITx_Step)
Description	: HTX link major setting table
Note		: N/A
Last Updated: 2024/06/19, Kled Chen
==========================================================================================*/
void HDMITX_CONFIG_STATE(int uHDMITxBlk, int uHDMITx_Step)
{
	switch (uHDMITx_Step) {
		case Input_Source_Selection:
			HDMITX_LINK_Source_Selection(uHDMITxBlk); //Select HTX Link Input Source
		break;

		case Output_Packet_Bypass_Configuration: //Bypass
			//Convertor auto control Packet
			//CONV2HDMITX_UpdateInterface(uHDMITxBlk, &HDMITxData[uHDMITxBlk]);
			//HDMITX_PacketSwitch_Bypass(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucPacketSource);
			
			if ((HDMITxData[uHDMITxBlk].bLinkBypass) || (HDMITxData[uHDMITxBlk].bDSCEN == TRUE)) {
				HDMITX_LINK_DEEP_COLOR_PACKET_CONFIG(uHDMITxBlk, _8bit_Depth); // Jack : Color depth should be set as 8-bit mode for link bypass
			}

			if(HDMITxData[uHDMITxBlk].bAudio_Info_Packet_EN == TRUE) { //Audio Info Packet
				HDMITxUpdateAudio(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucAudioSource);
			}
			
			HDMITX_INPUT_POLARITY(uHDMITxBlk); //Link Input Polarity Setting
			HDMITX_LINK_Port_Mode(uHDMITxBlk);
		break;

		case Output_Packet_Configuration: //non-bypass
		
			if (HDMITxData[uHDMITxBlk].bDSCEN == FALSE) {
				HDMITX_LINK_DEEP_COLOR_PACKET_CONFIG(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucDeepColor); //Deep Color Packet
			}
			else {
				HDMITX_LINK_DEEP_COLOR_PACKET_CONFIG(uHDMITxBlk, _8bit_Depth); //DSC keeps 8 Deep Color
			}
			/*
			if ((HDMITxData[uHDMITxBlk].ucSourceInterface == INTERNAL_1_INPUT) || (HDMITxData[uHDMITxBlk].ucSourceInterface == INTERNAL_2_INPUT)) {
				PacketSwitch2HDMITX(HDMIRX_HDR_SW, HDMITxData[uHDMITxBlk].ucPacketSource, uHDMITxBlk, 0x00, TRUE, FALSE);
				PacketSwitch2HDMITX(HDMIRX_BYP_SW, HDMITxData[uHDMITxBlk].ucPacketSource, uHDMITxBlk, HTX_BYPSW_FIFO_DELAY, FALSE, FALSE);
				PacketSwitch2HDMITX(HDMIRX_FS_SW, HDMITxData[uHDMITxBlk].ucPacketSource, uHDMITxBlk, 0x00, FALSE, FALSE);
			}
			else {
				PacketSwitch2HDMITX(HDMIRX_HDR_SW, HDMITxData[uHDMITxBlk].ucSourceInterface, uHDMITxBlk, 0x00, TRUE, FALSE);
				PacketSwitch2HDMITX(HDMIRX_BYP_SW, HDMITxData[uHDMITxBlk].ucSourceInterface, uHDMITxBlk, HTX_BYPSW_FIFO_DELAY, FALSE, FALSE);
				PacketSwitch2HDMITX(HDMIRX_FS_SW, HDMITxData[uHDMITxBlk].ucSourceInterface, uHDMITxBlk, 0x00, FALSE, FALSE);
			}

			if (((HDMITxData[uHDMITxBlk].ucSourceInterface == INTERNAL_1_INPUT) || (HDMITxData[uHDMITxBlk].ucSourceInterface == INTERNAL_2_INPUT)) && 
				(HDMITxData[uHDMITxBlk].ucPacketSource == DISPLAY_PORT_INPUT0 || HDMITxData[uHDMITxBlk].ucPacketSource == DISPLAY_PORT_INPUT1)) {
				PacketSwitch2HDMITX(HDMIRX_HDR_SW, HDMITxData[uHDMITxBlk].ucPacketSource, uHDMITxBlk, 0x00, FALSE, TRUE);
			} */

			//if(HDMITxData[uHDMITxBlk].ucAudioSource==TRUE) //AVI Info Packet
				HDMITX_LINK_AVI_INFO_PACKET_CONFIG(uHDMITxBlk);

			if(HDMITxData[uHDMITxBlk].bAudio_Info_Packet_EN == TRUE) { //Audio Info Packet
				HDMITxUpdateAudio(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucAudioSource);
			}

//			if(HDMITxData[uHDMITxBlk].bPPS_Packet_EN == TRUE) { //PPS Packet //580 move to FRL_DSC_Configuration state
//				HDMITX_LINK_PPS_PACKET_CONFIG(uHDMITxBlk);
//			}
			
			HDMITX_INPUT_POLARITY(uHDMITxBlk); //Link Input Polarity Setting
			HDMITX_LINK_Port_Mode(uHDMITxBlk);
		break;

		case Output_Packet_Disable:
			HDMITX_LINK_AVI_INFO_PACKET_Enable(uHDMITxBlk, FALSE); //AVI Info Packet
			HDMITX_LINK_SPD_PACKET_Enable(uHDMITxBlk, FALSE); //SPD Packet
			HDMITX_LINK_AUDIO_INFO_PACKET_Enable(uHDMITxBlk, FALSE); //Audio Info Packet
			HDMITX_LINK_MPEG_PACKET_Enable(uHDMITxBlk, FALSE); //MPEG Packet
			HDMITX_LINK_PPS_PACKET_Enable(uHDMITxBlk, FALSE); //PPS Packet
		break;

		case Output_Packet_Enable:
			HDMITX_LINK_AVI_INFO_PACKET_Enable(uHDMITxBlk, TRUE); //AVI Info Packet
			disphdmi_debug(("AVI Packet Enable \n\n"));

			if(HDMITxData[uHDMITxBlk].bSPD_Packet_EN == TRUE) { //SPD Packet
				HDMITX_LINK_SPD_PACKET_Enable(uHDMITxBlk, TRUE);
				disphdmi_debug(("SPD Packet Enable \n\n"));
			}

			if(HDMITxData[uHDMITxBlk].bAudio_Info_Packet_EN == TRUE) { //Audio Info Packet
				HDMITX_LINK_AUDIO_INFO_PACKET_Enable(uHDMITxBlk, TRUE);
				disphdmi_debug(("Audio INFO Packet Enable \n\n"));
			}
			
			if(HDMITxData[uHDMITxBlk].bMPEG_Packet_EN == TRUE) { //MPEG Packet
				HDMITX_LINK_MPEG_PACKET_Enable(uHDMITxBlk, TRUE);
				disphdmi_debug(("MPEG Packet Enable \n\n"));
			}

			if(HDMITxData[uHDMITxBlk].bPPS_Packet_EN == TRUE) { //PPS Packet
				/*
				SC_DSC_RST_REG |= BIT3;  //dsc_enc_rx_rst, to avoid DSC Encoder overflow
				drv_Sys_UsNOPDelay(100);
				SC_DSC_RST_REG &= ~BIT3;
				*/
				HDMITX_LINK_PPS_PACKET_Enable(uHDMITxBlk, TRUE);
				disphdmi_debug(("PPS Packet Enable \n\n"));
			}
		break;

		case Phy_PCLK_Source_Configuration:
			HDMITX_PHY_PCLK_MPLL_FILTER(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucSourceInterface, HDMITxData[uHDMITxBlk].bPCLK_ExternalMPLL); //PCLK from MPLL Filter or ICP/RX
		break;

		case Phy_FRLCLK_Source_Configuration:
			HDMITX_PHY_PCLK_MPLL_FILTER(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucSourceInterface, FALSE);
			HDMITX_PHY_FRLCLK_BYPASS_MODE(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucSourceInterface, FALSE);//always false
			HDMITX_PHY_FRL_CLK_SOURCE(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucCurrentFRLRate);
		break;

		case Phy_CLK_Configuration:
			if(HDMITxData[uHDMITxBlk].bLinkBypass == FALSE) {
				hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_Port_CFG2, BIT2, FALSE);//Jack - cut3 ECO for bypass
			}
			else {
				hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_EMP_CTRL, _HDMITXEMP_Port_CFG2, BIT2, TRUE); //Jack - cut3 ECO for bypass
				HDMITxData[uHDMITxBlk].ucDeepColor = _8bit_Depth;
			}
			
			HDMITX_PHY_RESET(uHDMITxBlk, TRUE);
			HDMITX_PHY_TOTAL_PLL_CONFIG(uHDMITxBlk);
			HDMITX_PHY_TOTAL_PLL_ENABLE(uHDMITxBlk);
			HDMITX_PHY_RESET(uHDMITxBlk, FALSE);
		break;

		case Phy_Power_On:
			HDMITX_PHY_POWER_ON(uHDMITxBlk);
		break;

		case Phy_Power_Down:
			HDMITX_PHY_POWER_DOWN(uHDMITxBlk);
			HDMITX_LINK_POWER_DOWN(uHDMITxBlk);
			HDMITX_PHY_P2S_ENABLE(uHDMITxBlk, FALSE);
			ClearHDMITxOutStatus(uHDMITxBlk);
			HDMITxData[uHDMITxBlk].bHTX_Training_First = TRUE;
			HDMITxData[uHDMITxBlk].bHTX_Enable = FALSE;
		break;

		case TMDS_Configuration:
			HDMITX_LINK_TMDS_CONFIG(uHDMITxBlk); //TMDS LINK
			HDMITX_PHY_TMDS_CONFIG(uHDMITxBlk, HDMITxData[uHDMITxBlk].InputClock*HDMITxData[uHDMITxBlk].ucDeepColor/8);												//TMDS PHY
			//HDMITX_PHY_TMDS_Prefered_ChargePump(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucDeepColor, HDMITxData[uHDMITxBlk].InputClock); //TC68580/NT68580 no this function
		break;

		case FRL_Configuration:
			
			HDMITX_LINK_FRL_CONFIG(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucCurrentFRLRate, HDMITxData[uHDMITxBlk].bDSCEN); //FRL LINK
			HDMITX_PHY_FRL_CONFIG(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucCurrentFRLRate);
			//HDMITX_LINK_FRL_RX_BYPASS_SETTING(uHDMITxBlk, HDMITxData[uHDMITxBlk].bLinkBypass); //need to confirm by Roger 20220916
			//HDMITX_LINK_FRL_RX_1Port_DeepColor_ECO(uHDMITxBlk,TRUE);
			//HDMITX_LINK_FRL_3LANE_HFR1_19_ECO(uHDMITxBlk,TRUE);

			if(HDMITxData[uHDMITxBlk].InputClock > 100) {
				g_HDMI21_TriByteDiff = TX_TriByte_H;
			}
			else {
				g_HDMI21_TriByteDiff = TX_TriByte_L;
			}

			HDMITX_LINK_FRL_TriByteConfig(uHDMITxBlk, g_HDMI21_TriByteDiff);
			HDMITX_FRL_LINK_RST(uHDMITxBlk);
			
			//FRL PHY
		break;

		case FRL_DSC_Configuration:
			//DSC_ENC_SRC_CONFIG();
			//DSC_ENC_PPS_GEN();
			HDMITX_LINK_PPS_PACKET_CONFIG(uHDMITxBlk); //HDMITxData[uHDMITxBlk].bPPS_Packet_EN == TRUE  //MUST BE
			//DSC_ENC_TIMER_CONFIG(uHDMITxBlk);
			HDMITX_LINK_DEEP_COLOR_PACKET_CONFIG(uHDMITxBlk, _8bit_Depth);
		break;
		
		case Driving_Enable:
			HDMITX_PHY_DCC_BYPASS(uHDMITxBlk, TRUE);
			HDMITX_PHY_P2S_ENABLE(uHDMITxBlk, TRUE);
		break;

		case Driving_Disable:
			HDMITX_PHY_P2S_ENABLE(uHDMITxBlk, FALSE);
			HDMITX_PHY_RES_TRIM(uHDMITxBlk, 0x00, 0x00, 0x00, 0x00, 0x00); //Alex sugguestion
		break;

		case Reset_Link_PHY:
			HDMITX_LINK_RESET(uHDMITxBlk);
		break;

		case Link_PHY_initialization:
			HDMITX_LINK_PHY_Init(uHDMITxBlk);
		break;

		case Link_Input_Timing:
			if(HDMITxData[uHDMITxBlk].bDSCEN) {
				HDMITxData[uHDMITxBlk].usVact = DSCENCData.ucDSC_VDE;
			}
			else {
				HDMITxData[uHDMITxBlk].usVact = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VDE_LO) + (hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_VDE_HI)<<8);
			}
		break;

		default:
		break;

	}
}

/*=========================================================================================
Function 	: HDMITX_OUTPUT_CONFIG_TOP(int uHDMITxBlk)
Description	: HDMITX data output setting flow
Note		: N/A
Last Updated: 2024/06/19, Kled Chen
==========================================================================================*/
void HDMITX_OUTPUT_CONFIG_TOP(int uHDMITxBlk)
{
	if (1) {	//Check source SYNC stable

		if (1) { 											//TX doesn't set output, start to config HTX output

			disphdmi_debug(("(1) Port%d: Input Source Selection \n", uHDMITxBlk));
			HDMITX_CONFIG_STATE(uHDMITxBlk, Input_Source_Selection);					//Set TX Link source
			HDMITX_CONFIG_STATE(uHDMITxBlk, Output_Packet_Disable);					//Packet Disable

			disphdmi_debug(("(2) Port%d: Packet Configuration \n", uHDMITxBlk));
			if(HDMITxData[uHDMITxBlk].bConverter_Mode == FALSE) {						//Set Link Packet
				HDMITX_CONFIG_STATE(uHDMITxBlk, Output_Packet_Configuration);
			}
			else {
				HDMITX_CONFIG_STATE(uHDMITxBlk, Output_Packet_Bypass_Configuration);
			}

			if(HDMITxData[uHDMITxBlk].bFRL_Rate_Update == TRUE) {
				disphdmi_debug(("-- Port%d: Update FRL Rate !!! --\n", uHDMITxBlk));
				//HDMITX_LINK_FRL_RATE_DBG(HDMITxData[uHDMITxBlk].ucCurrentFRLRate);
				//ClearHDMITx_FRL_Update(uHDMITxBlk);
			}
				
			disphdmi_debug(("(3) Port%d: Clock Source Configuration \n", uHDMITxBlk));
			if(HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode == FALSE) {
				HDMITX_CONFIG_STATE(uHDMITxBlk, Phy_PCLK_Source_Configuration);		//Set PCLK Source (w/ HTX PHY)
			}
			else {
				HDMITX_CONFIG_STATE(uHDMITxBlk, Phy_FRLCLK_Source_Configuration);		//Set FRL/PCLK Source/Value
			}

			disphdmi_debug(("(4) Port%d: HTX Link_Input_Timing \n", uHDMITxBlk));
			HDMITX_CONFIG_STATE(uHDMITxBlk, Link_Input_Timing);

			disphdmi_debug(("(5) Port%d: HTX PHY+Link Configuration \n", uHDMITxBlk));
			if(HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode == FALSE) {
				//disphdmi_debug(2("TMDS Mode \n\n");
				HDMITX_CONFIG_STATE(uHDMITxBlk, TMDS_Configuration);					//TMDS PHY + Link Configuration
#if 0 //add by Roger 20220526 for 580 FPGA verify 
				if(HDMITxData[uHDMITxBlk].bDSCEN == TRUE) {
					disphdmi_debug(2("TMDS + DSC Enable \n\n");
					HDMITX_CONFIG_STATE(uHDMITxBlk, FRL_DSC_Configuration);			//TMDS PHY + Link + DSC
				}
				else {
					disphdmi_debug(2("TMDS + DSC Disable \n\n");
				}
#endif
			}
			else {
				if(HDMITxData[uHDMITxBlk].bDSCEN == TRUE) {
					disphdmi_debug(("FRL + DSC Enable \n\n"));
					HDMITX_CONFIG_STATE(uHDMITxBlk, FRL_DSC_Configuration);			//FRL PHY + Link + DSC
				}
				else{
					disphdmi_debug(("FRL + DSC Disable \n\n"));
				}
				
				//disphdmi_debug(2("FRL Mode \n\n");
				HDMITX_CONFIG_STATE(uHDMITxBlk, FRL_Configuration);					//FRL PHY + Link Configuration	
			}

			disphdmi_debug(("(6) Port%d: HTX PHY Power ON \n", uHDMITxBlk));
			HDMITX_CONFIG_STATE(uHDMITxBlk, Phy_Power_On);							//PHY POWER ON (Wait MPLL ready)

			disphdmi_debug(("(7) Port%d: HTX PHY CLK Configuration \n", uHDMITxBlk));
			HDMITX_CONFIG_STATE(uHDMITxBlk, Phy_CLK_Configuration);					//PHY CLOCK (PLL1/PLL2) Setting
#if 0 //DDC debug used
			HDMITX_PHY_DCC_AUTO_K(uHDMITxBlk);
#endif	
			HDMITX_LINK_HDMI_MODE_CONFIG(uHDMITxBlk, HDMITxData[uHDMITxBlk].bHDMITXHdmiMode);	
			if((HDMITxData[uHDMITxBlk].bLinkBypass == FALSE)) {
				disphdmi_debug(("(8) Port%d: Packet Enable/Disable \n", uHDMITxBlk));
				HDMITX_CONFIG_STATE(uHDMITxBlk, Output_Packet_Enable);				//Packet Enable
			}

			//Austin add audio after output stable
			if(HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode == TRUE) {
				hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Audio_FRL_Mode, BIT3, TRUE);
			}
			else {
				hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Audio_FRL_Mode, BIT3, FALSE);
			}

			disphdmi_debug(("(9) Port%d: Check PSTABLE/Timing \n", uHDMITxBlk));
			HDMITX_LINK_Check_PSTABLE(uHDMITxBlk);

			// Link Reset after PHY/link configured.
			HDMITX_CONFIG_STATE(uHDMITxBlk, Reset_Link_PHY);						//Reset Link & PHY

			HDMITX_CONFIG_STATE(uHDMITxBlk, Driving_Enable);							//P2S + Driving Enable			
			SetHDMITxOutStatus(uHDMITxBlk);											//Set TX Output ready status

			/*

			if(HDMITxData[uHDMITxBlk].bConverter_Mode == TRUE) {
				CONV2HDMITX_UpdatePacket(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucPacketSource);
			}
			*/

		}
		else {
			// TX already output data, only needs to update packet information
			disphdmi_debug(("TX already output data, only needs to update packet information\n"));
			//HDMITxUpdatePkt(uHDMITxBlk, _All_PKT, FALSE);
		}
	}
}


void HDMITX_OUTPUT_CONFIG_TEST(int uHDMITxBlk)
{

	disphdmi_debug(("\r\n"));
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Mode_DeepColor,  0x81);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_0,   0x03);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Audio_SD_Ctrl,   0x09);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Audio_I2S_Ctrl,  0x41);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_SSCP_EN, 		 0x00); //bit 0 = 1 for null pack enable on vsync
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Audio_I2S_Ctrl2, 0x22);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_N_SVAL_LO, 		 0x00);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_N_SVAL_MI, 		 0x18);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_N_SVAL_HI, 		 0x00);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_REPETITION_Ctrl, 0x2D);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, 	_HDMITXEMP_Port_CFG, 		 0x70);
	HDMITX_LINK_Source_Selection(uHDMITxBlk);

	HDMITX_CONFIG_STATE(uHDMITxBlk, Output_Packet_Enable);				//Packet Enable
	HDMITxAvMuteSet(uHDMITxBlk,HdmiTx_AvMute_Clear);
	
}

void HDMITX_OUTPUT_CONFIG_TEST_FRL(int uHDMITxBlk)
{

	disphdmi_debug(("\r\n"));
	HDMITX_LINK_FRL_CONFIG(uHDMITxBlk, HDMITxData[uHDMITxBlk].ucCurrentFRLRate, HDMITxData[uHDMITxBlk].bDSCEN); //FRL LINK
	
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Mode_DeepColor,  0x81);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Packet_Ctrl_0,   0x03);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Audio_SD_Ctrl,   0x09);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Audio_I2S_Ctrl,  0x41);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_SSCP_EN, 		 0x01); //bit 0 = 1 for null pack enable on vsync
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_Audio_I2S_Ctrl2, 0x22);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_N_SVAL_LO, 		 0x00);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_N_SVAL_MI, 		 0x18);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL1, _HDMITXLINK_N_SVAL_HI, 		 0x00);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_REPETITION_Ctrl, 0x2D);
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL, 	_HDMITXEMP_Port_CFG, 		 0x71);
	HDMITX_LINK_Source_Selection(uHDMITxBlk);

	HDMITX_LINK_Port_Mode(uHDMITxBlk);

	hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_SSB_SR_THLD,  0x2);
	/*
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_UPPDER_LO, 0x80);	
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_UPPDER_LO, 0x04);	
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_LOWER_LO,  0x80);	
	hdmitx2p1_write_reg(uHDMITxBlk, HTX_FRL_CTRL, _HDMITXFRL_RC_LOWER_HI,  0x03);
	*/

	if(HDMITxData[uHDMITxBlk].bDSCEN){
		HDMITX_LINK_PPS_PACKET_CONFIG(uHDMITxBlk);
		HDMITX_LINK_DEEP_COLOR_PACKET_CONFIG(uHDMITxBlk, _8bit_Depth);
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL,	_HDMITX_SYS_Ctrl_1, 		 0x1); // LCD select DSC
		disphdmi_debug(("DSC Enalbe sel source to DSC\r\n"));
	} else {
		hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL,	_HDMITX_SYS_Ctrl_1, 		 0x0); // LCD select
	}

	HDMITX_CONFIG_STATE(uHDMITxBlk, Output_Packet_Enable);				//Packet Enable	

	HDMITX_FRL_LINK_RST(uHDMITxBlk);
}



#endif



#if 1

/*
    Lock HDMI EDID module

    Use semaphore lock for the HDMI EDID module

    @return
	@b E_OK: success
	@b Else: fail
*/
ER hdmitx2p1_edid_lock(void)
{
	ER er_ret;
/*
	er_ret        = SEM_WAIT(SEMID_HDMI2P1_EDID);
	if (er_ret != E_OK) {
		DBG_ERR("[2p1] wait semaphore fail\r\n");
		return er_ret;
	}
*/
	return E_OK;
}

/*
    Unlock HDMI EDID module

    Release semaphore lock for the HDMI EDID module

    @return
	@b E_OK: success
	@b Else: fail
*/
ER hdmitx2p1_edid_unlock(void)
{
	//SEM_SIGNAL(SEMID_HDMI2P1_EDID);
	return E_OK;
}



/**
    Open HDMI Transmitter Driver

   Open HDMI Transmitter and then intialize the source clock / DDC / interrupt for usage.
   Besides, this would also read the TV/Display's EDID information from DDC channel and parsing the EDID.
   The user can get the EDID information after hdmitx2p1_open() by the api hdmitx2p1_get_edid_info().
   The driver is opened or not can be checked by the hdmitx2p1_is_opened.
   The behavior of the re-open of the HDMI driver in under save protection in the driver design.\n
   In the system design, the DDC channel is using the I2C controller in normal operation.
   The user must remember opening the I2C2 pinmux in the systemInit phase.

    @return
     - @b E_OK: Done and success
*/
ER hdmitx2p1_open(void)
{
	int uHDMITxBlk = 0;
	disphdmi_debug(("%s\r\n",__func__));
/*
	if(NVT_API_CHK_CALL(hdmitx2p1) != TRUE){
		DBG_ERR("[2p1] objver mismatch %s\r\n", "HDMITX2P1");
		return E_SYS;
	}
*/	

	if (b_hdmi2p1_tx_open == TRUE) {
		return E_OK;
	}


	
	HDMITxData[uHDMITxBlk].bDataRdy 			= 0; //not used now
	HDMITxData[uHDMITxBlk].bLinkBypass 			= 0; //not used now
	HDMITxData[uHDMITxBlk].bConverter_Mode 		= 0; //not used now
	HDMITxData[uHDMITxBlk].ucSourceInterface 	= 0; //not used now
	HDMITxData[uHDMITxBlk].ucAudioSource		= 0; //not used now
	HDMITxData[uHDMITxBlk].bH_POL				= 0;
	HDMITxData[uHDMITxBlk].bV_POL				= 0;
	HDMITxData[uHDMITxBlk].bHDMITXHdmiMode		= 1;
	HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode		= HDMITxData_option[uHDMITxBlk].bHTX21_FRL_Mode;
	HDMITxData[uHDMITxBlk].bDSCEN				= HDMITxData_option[uHDMITxBlk].bDSCEN;
	HDMITxData[uHDMITxBlk].bPortMode			= HDMITxData_option[uHDMITxBlk].bPortMode; //0: 1 pixel mode 1: 2 pixel mode
	HDMITxData[uHDMITxBlk].bHTX_SC_en			= 0; //not used now
	HDMITxData[uHDMITxBlk].bPCLK_ExternalMPLL	= 0; //not used now
	HDMITxData[uHDMITxBlk].bHTX21_FRL_Failed	= 0; //not used now
	HDMITxData[uHDMITxBlk].InputClock			= clock_rate/1000000;			
	HDMITxData[uHDMITxBlk].bFull2Limit			= 0;			
	HDMITxData[uHDMITxBlk].ucColorSapce			= CSCInput_RGB_Limit;
	HDMITxData[uHDMITxBlk].ucVICCode			= 34;
	HDMITxData[uHDMITxBlk].ucRepeatition		= Repetition1x;
	HDMITxData[uHDMITxBlk].bSCDCPresent			= 0; //not used now
	HDMITxData[uHDMITxBlk].ucDeepColor			= _8bit_Depth;
	HDMITxData[uHDMITxBlk].ucMaxFRLRate			= 0; //not used now
	HDMITxData[uHDMITxBlk].bAVI_Info_Packet_EN	= 1; 
	HDMITxData[uHDMITxBlk].bSPD_Packet_EN		= 0; 
	HDMITxData[uHDMITxBlk].bAudio_Info_Packet_EN= 1; 
	HDMITxData[uHDMITxBlk].bMPEG_Packet_EN		= 0; 
	HDMITxData[uHDMITxBlk].bPPS_Packet_EN		= HDMITxData_option[uHDMITxBlk].bPPS_Packet_EN;
	HDMITxData[uHDMITxBlk].bDataUpdated			= 0; //not used now
	HDMITxData[uHDMITxBlk].bOutStatus			= 1; 
	HDMITxData[uHDMITxBlk].bCtsNUpdate			= 0; //not used now
	HDMITxData[uHDMITxBlk].bAudioSample			= 0; //not used now
	HDMITxData[uHDMITxBlk].bAviInfoUpdate		= 0; //not used now
	HDMITxData[uHDMITxBlk].ucCurrentFRLRate		= FRL_RATE_8G_4L;
	HDMITxData[uHDMITxBlk].bFRLPortMode			= HDMITxData_option[uHDMITxBlk].bFRLPortMode; //0: 1 port mode 1: 2port mode
	HDMITxData[uHDMITxBlk].bFRL_Training_done	= 0; //not used now
	HDMITxData[uHDMITxBlk].bFRL_Rate_Update		= 0;
	HDMITxData[uHDMITxBlk].bHTX_Enable			= 0; //not used now		
	HDMITxData[uHDMITxBlk].bHTX_Training_First	= 0; //not used now	
	HDMITxData[uHDMITxBlk].ucPacketSource		= 0; //not used now
	HDMITxData[uHDMITxBlk].usVact				= 0;
	HDMITxData[uHDMITxBlk].bDual_Display_Sel	= HDMITxData_option[uHDMITxBlk].bDual_Display_Sel;
	HDMITxData[uHDMITxBlk].ucAudio_fmt			= HDMI_AUDIO48KHZ;

	if (!hdmi2p1_init){
		hdmi2p1_init = TRUE;
#if defined(__FREERTOS)
		hdmitx2p1_create_resource();
		if(hdmitx2p1_i2c_init() != 1){
			DBG_ERR("[2p1] i2c driver probe fail\r\n");
		} else {
			DBG_ERR("[2p1] i2c driver probe OK\r\n");
		}
#endif	
		hdmitx2p1_hpd_pad_control();
		hdmi2p1_hpd_status = hdmitx2p1_check_hotplug();
	}

	hdmitx2p1_init_ddc(1);
	
	hdmitx2p1_attach();
	hdmitx2p1_set_power_down(DISABLE);
	hdmitx2p1_reset(DISABLE);

	if(HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode){
		HDMITX_OUTPUT_CONFIG_TEST_FRL(uHDMITxBlk);
	} else {
		HDMITX_OUTPUT_CONFIG_TEST(uHDMITxBlk);
	}

	
	b_hdmi2p1_tx_open = 1;
	hdmi2p1_state |= (HDMI_OPEN | HPG_CONNECT); // assume init with plug on, will check status later when device open done.
	return E_OK; 
}

/**
    Check the HDMI-Tx driver is opened or not

    Check the HDMI-Tx driver is opened or not

    @return
     - @b TRUE:  Already opened.
     - @b FALSE: Have not opened yet.
*/
BOOL hdmitx2p1_is_opened(void)
{
	return b_hdmi2p1_tx_open;
}

/**
    Close the HDMI-Tx module

    Close the HDMI-Tx module, this api would power down the hdmi-tx module and also close the I2C2(DDC) usage.

    @return void
     - @b E_OK: Done and success
*/
ER  hdmitx2p1_close(void)
{
	disphdmi_debug(("%s\r\n",__func__));
	
	if (b_hdmi2p1_tx_open == FALSE) {
		return E_OK;
	}

#if defined(__FREERTOS)
	hdmitx2p1_release_resource();
#endif	

	hdmitx2p1_detach();
	hdmitx2p1_set_power_down(ENABLE);
	hdmi2p1_init = 0;
	b_hdmi2p1_tx_open = 0;
	hdmi2p1_state = 0;
	return E_OK;
}


/**
    Check hotplug or not

    This is used to check if the HDMI cable is plugged or not.

    @return
     - @b TRUE:  Hotplug attached.
     - @b FALSE: Hotplug detached.
*/
BOOL hdmitx2p1_check_hotplug(void)
{
	int uHDMITxBlk = 0;
	BOOL ret;
	int val;
	
	val = hdmitx2p1_read_reg(uHDMITxBlk, HTX_LINK_CTRL0, ADR_HDMI_SYS_STAT);

	val &= BIT1; // HPD bit

	if(val){
		ret = 1;
	} else {
		ret = 0;
	}
	
	return ret;
}

/**
    Check if HDMI's video Clock is stable or not

    This api is used in the HDMI display device object to check if the hdmi input video clock is stable or not after
    opening the IDE Pixel clock. If the none of the video clock or the clock is not stable, this api would output the
    warning message "HDMI Video Clock Not Stable! Timeout!!" on console.

    @return void
*/
void hdmitx2p1_check_video_stable(void)
{
	disphdmi_debug(("%s\r\n",__func__));
}
#endif

/**
    Set HDMI functionality configuration

    This is used to configure the main functions of the HDMI controller, including the Video/Audio formats, output-enable,
    video-source, 3D-format, ...etc. Please reference to the "HDMI_CONFIG_ID" and the application note for the control function details.

    @param[in] config_id     Control function selection ID.
    @param[in] ui_config     Please reference to the "HDMI_CONFIG_ID" and the application note for the control function parameter details.

    @return void
*/
void hdmitx2p1_set_config(HDMI_CONFIG_ID config_id, UINT32 ui_config)
{
#if HDMITX_SCDC_SUPPORT	
	int timeout = 20;
#endif

	int uHDMITxBlk = 0;
	unsigned long flag;
	disphdmi_debug(("%s\r\n",__func__));
	
	loc_cpu(flag);
	unl_cpu(flag);

	switch (config_id) {
		/* Control Options */
		case HDMI_CONFIG_ID_FORCE_RGB: {
				hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_RGB]        = (ui_config > 0);
			}
			return;

		case HDMI_CONFIG_ID_FORCE_UNDERSCAN: {
				hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_UNDERSCAN]  = (ui_config > 0);
			}
			return;

		case HDMI_CONFIG_ID_VERIFY_VIDEOID: {
				hdmi2p1_ctrl_option[HDMICFG_OPTION_VERIFY_VID]       = (ui_config > 0);
			}
			return;
			
		case HDMI_CONFIG_ID_FORCE_YUV444: {
				if(ui_config > 0)
					hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] |= 0x1;
				else
					hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] &= ~0x1;
			}
			return;

		case HDMI_CONFIG_ID_FORCE_YUV422: {
				if(ui_config > 0)
					hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] |= 0x2;
				else
					hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] &= ~0x2;
			}
			return;

		case HDMI_CONFIG_ID_FORCE_YUV420: {
				if(ui_config > 0)
					hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] |= 0x4;
				else
					hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] &= ~0x4;
			}
			return;	

		case HDMI_CONFIG_ID_2P1_PXL_MODE:{
				if(ui_config > 0){
					HDMITxData_option[uHDMITxBlk].bPortMode= 1;
					HDMITxData_option[uHDMITxBlk].bFRLPortMode = 1;
				} else {
					HDMITxData_option[uHDMITxBlk].bPortMode = 0;
					HDMITxData_option[uHDMITxBlk].bFRLPortMode = 0;
				}
			}
			return;

		case HDMI_CONFIG_ID_2P1_DUAL_DISP_SEL:{
				if(ui_config > 0){
					HDMITxData_option[uHDMITxBlk].bDual_Display_Sel = 1;
					hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL,	_HDMITX_SYS_Ctrl_1, 		 0x10); // LCD select
				} else {
					HDMITxData_option[uHDMITxBlk].bDual_Display_Sel = 0;
					hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL,	_HDMITX_SYS_Ctrl_1, 		 0x00); // LCD select
				}
			}
			return;

		case HDMI_CONFIG_ID_2P1_DSC_EN:{
				if(ui_config > 0){
					HDMITxData_option[uHDMITxBlk].bDSCEN= 1;
					HDMITxData_option[uHDMITxBlk].bPPS_Packet_EN= 1;
					//dsc_set_config(DSC_ID_1, DSC_CONFIG_SET_PPS_TABLE, DSC_RGB888);
					//dsc_open(DSC_ID_1);				
				} else {
					HDMITxData_option[uHDMITxBlk].bDSCEN = 0;
					HDMITxData_option[uHDMITxBlk].bPPS_Packet_EN= 0;
					//dsc_close(DSC_ID_1);
				}
			}
			return;

		case HDMI_CONFIG_ID_2P1_PPS_TABLE:{
			}
			return;

		case HDMI_CONFIG_ID_2P1_FORCE_FRL_EN:{
				if(ui_config > 0){
					HDMITxData_option[uHDMITxBlk].bHTX21_FRL_Mode = 1;
				} else {
					HDMITxData_option[uHDMITxBlk].bHTX21_FRL_Mode = 0;
				}
			}
			return;

		case HDMI_CONFIG_ID_VIDEO: {
				HDMITxData[uHDMITxBlk].ucVICCode = ui_config;
				HDMITxData_option[uHDMITxBlk].ucVICCode = ui_config;
				HDMITX_LINK_AVI_INFO_PACKET_CONFIG(uHDMITxBlk);
			}
			return;

		case HDMI_CONFIG_ID_AUDIO: {
				HDMITxData[uHDMITxBlk].ucAudio_fmt= ui_config;
				HDMITxData_option[uHDMITxBlk].ucAudio_fmt = ui_config;
				hdmitx2p1_config_audio(uHDMITxBlk, ui_config);
			}
			return;			

		case HDMI_CONFIG_ID_OE: {
				if(ui_config > 0){
					HDMITX_CONFIG_STATE(uHDMITxBlk, Output_Packet_Enable);
				}
			}
			return;

		case HDMI_CONFIG_ID_SOURCE :{
				switch(ui_config){
					case HDMI2P1_SOURCE_DSC: 
							hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL,	_HDMITX_SYS_Ctrl_1, 		 0x01); // LCD select
						break;
					case HDMI2P1_SOURCE_LCD_R: 
							hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL,	_HDMITX_SYS_Ctrl_1, 		 0x10); // LCD select
						break;
					case HDMI2P1_SOURCE_LCD: 
					default:
							hdmitx2p1_write_reg(uHDMITxBlk, HTX_EMP_CTRL,	_HDMITX_SYS_Ctrl_1, 		 0x00); // LCD select
						break;
				}
			}
			return;

		case HDMI_CONFIG_ID_VSYNC_INVERT: {
				hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_OUTPUT_HV_POL, BIT0, ui_config);
				hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_SYNC_SCCP_CFG, BIT0, ui_config);
				hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_INPUT_HV_POL_ , BIT5, ui_config);
				disphdmi_debug(("%s HDMI_CONFIG_ID_VSYNC_INVERT %d\r\n",__func__,ui_config));

			}
			return;
		case HDMI_CONFIG_ID_HSYNC_INVERT: {
				hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_OUTPUT_HV_POL, BIT1, ui_config);
				hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_SYNC_SCCP_CFG, BIT1, ui_config);
				hdmitx2p1_write_reg_bit(uHDMITxBlk, HTX_LINK_CTRL0, _HDMITXLINK_INPUT_HV_POL_ , BIT4, ui_config);
				disphdmi_debug(("%s HDMI_CONFIG_ID_HSYNC_INVERT %d\r\n",__func__,ui_config));
			}
			return;	


	case HDMI_CONFIG_ID_SCDC_ENABLE: {
		
#if HDMITX_SCDC_SUPPORT		
			if (ui_config > 0) {
				// SCDC Cap
				if (hdmitx2p1_get_hdmi_2_vsdb_cap() & HDMI2VSDB_SCDC_PRESENT) {
					UINT32 data;

					g_Sink_adapt_4kp60_420_mode = 0;

					if (hdmitx2p1_scdc_read(0x01, &data) != E_OK) {
						DBG_ERR("[2p1] Read SCDC Sink Version failed\r\n");
						return;
					}

					if (data != HDMI2_SCDC_VERSION) {
						DBG_ERR("[2p1] Sink's SCDC version not matched.(%d)\r\n", data);
					} else {
						DBG_WRN("[2p1] SCDC Enable!\r\n");
					}

					if (hdmitx2p1_scdc_write(0x02, HDMI2_SCDC_VERSION) != E_OK) {
						DBG_ERR("[2p1] Write SCDC Src Version failed\r\n");
						return;
					}

					if((clock_rate > 340000000)&& (!(hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] & 0x4))){ //when TMDS > 340Mhz and not use 420 mode, need 1:40 rate and scramble on
						if(hdmitx2p1_scdc_write(0x20, 0x03) != E_OK){
							DBG_ERR("[2p1] Write SCDC scramble, tmds 1:40 enable fail\r\n");
							return;
						}
						hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_SCCP_PIXEL_H_DLY, 0x02);
						hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TG_CFG, 0x06);/*1:40 tmds rate & scramble on*/
						DBG_WRN("[2p1] Write SCDC scramble, tmds 1:40 enable success\r\n");	

						do {
							DELAY_MS(5);
							if (hdmitx2p1_scdc_read(0x21, &data) != E_OK) {
	                            DBG_ERR("[2p1] Read SCDC scramble sts , no support SCDC deviced\r\n");
	                            return;
	                        }
	                        if (data & 0x1) {
		                        break;
	                        }

						} while(timeout--);

						if(timeout == 0){
							DBG_ERR("[2p1] Read SCDC scramble sts timeout\r\n");
							return;
						}
					
					} else {
						if(hdmitx2p1_scdc_write(0x20, 0x00) != E_OK){
							DBG_WRN("[2p1] Write SCDC scramble, tmds 1:40 disable fail\r\n");
							return;
						}
						hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_SCCP_PIXEL_H_DLY, 0x00);
						hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TG_CFG, 0x00); /*1:40 tmds rate & scramble dis*/
						DBG_WRN("[2p1] Write SCDC scramble, tmds 1:40 disable success\r\n");
					}


					do {
						DELAY_MS(5);
						if (hdmitx2p1_scdc_read(0x40, &data) != E_OK) {
							DBG_ERR("[2p1] Read SCDC clock detect sts , no support SCDC deviced\r\n");
							return;
						}
						if (data & 0x1) {
							break;
						}

					} while(timeout--);

					if(timeout == 0){
						DBG_ERR("[2p1] Read SCDC clock detect sts timeout\r\n");
						return;
					}
					

				} else {
					g_Sink_adapt_4kp60_420_mode = 0;
					DBG_WRN("[2p1] HDMI2.0 VSDB SCDC not present!\r\n");
				}

			} else {

				hdmitx2p1_write_reg(uHDMITxBlk, HTX_PHY_CTRL, _HDMITXPHY_TTX_TG_CFG, 0x00); /*1:40 tmds rate & scramble dis*/
				g_Sink_adapt_4kp60_420_mode = 0;
							}
#endif

		}
		break;

	case HDMI_CONFIG_ID_PHY_INIT: {
			hdmitx2p1_init_phy(uHDMITxBlk);
		}
		break;

	case HDMI_CONFIG_ID_2P1_FRL_LTP_EN:{
			if(ui_config){
				hdmitx2p1_create_resource_frl();
			} else {
				hdmitx2p1_release_resource_frl();
			}
		}
		break;

	case HDMI_CONFIG_ID_COMPONENT0: {
			if(HDMITxData_option[uHDMITxBlk].bDSCEN == 0){
				hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_EMP_CTRL, 	_HDMITX_SYS_Ctrl_0, BIT0|BIT1, ui_config);
			}
		}
		break;
	case HDMI_CONFIG_ID_COMPONENT1: {
			if(HDMITxData_option[uHDMITxBlk].bDSCEN == 0){
				hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_EMP_CTRL, 	_HDMITX_SYS_Ctrl_0, BIT2|BIT3, ui_config<<2);
			}
		}
		break;
	case HDMI_CONFIG_ID_COMPONENT2: {
			if(HDMITxData_option[uHDMITxBlk].bDSCEN == 0){
				hdmitx2p1_write_reg_mask(uHDMITxBlk, HTX_EMP_CTRL, 	_HDMITX_SYS_Ctrl_0, BIT4|BIT5, ui_config<<4);
			}
		}
		break;

	case HDMI_CONFIG_ID_ADAPT_4KP60_420:{
			if(ui_config > 0){
				if (!(hdmitx2p1_get_hdmi_2_vsdb_cap() & HDMI2VSDB_SCDC_PRESENT)) {
					g_Sink_adapt_4kp60_420_mode = 1;
					DBG_WRN("[2p1] get Sink info to use 4KP60_YCC420_mode!\r\n");
				} else {
					g_Sink_adapt_4kp60_420_mode = 0;
				}
			} else {
				g_Sink_adapt_4kp60_420_mode = 0;
			}
		}
		break;

	case HDMI_CONFIG_ID_THREAD_STOP:{
			if(ui_config > 0){
				//vos_task_destroy(hdmi2p1_frl_tsk_hdl);
			} else {
				//hdmi2p1_frl_tsk_hdl = vos_task_create(hdmi2p1_polling_tsk_frl, NULL, "hdmi2p1_polling_tsk_frl", 10, 4096);
				//vos_task_resume(hdmi2p1_frl_tsk_hdl);
			}
		}
		break;

		default:
			return;
	}
}

/**
    Get HDMI functionality configuration

    Use this api to get the current HDMI function configuration.

    @param[in] config_id     Get which Control function configuration selection.

    @return Please reference to the "HDMI_CONFIG_ID" for the control function parameter details.
*/
UINT32 hdmitx2p1_get_config(HDMI_CONFIG_ID config_id)
{
	int uHDMITxBlk = 0;
	UINT32 ret = 0;
	disphdmi_debug(("%s\r\n",__func__));

	switch(config_id){

		case HDMI_CONFIG_ID_FORCE_RGB: {
			ret = hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_RGB];
		}
		break;

		case HDMI_CONFIG_ID_FORCE_YUV444: {
				ret = (hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] & 0x1) > 0;
			}
			break;
		case HDMI_CONFIG_ID_FORCE_YUV422: {
				ret = (hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] & 0x2) > 0;
			}
			break;
		case HDMI_CONFIG_ID_FORCE_YUV420: {
				ret = (hdmi2p1_ctrl_option[HDMICFG_OPTION_FORCE_YUV] & 0x4) > 0;
			}
			break;
		case HDMI_CONFIG_ID_2P1_DSC_EN: {
				ret = HDMITxData_option[uHDMITxBlk].bDSCEN;
				disphdmi_debug(("%s HDMI_CONFIG_ID_2P1_DSC_EN ret %d\r\n",__func__,HDMITxData_option[uHDMITxBlk].bDSCEN));
			}
			break;
		case HDMI_CONFIG_ID_2P1_FORCE_FRL_EN: {
				ret = HDMITxData_option[uHDMITxBlk].bHTX21_FRL_Mode;
				disphdmi_debug(("%s HDMI_CONFIG_ID_2P1_FORCE_FRL_EN ret %d\r\n",__func__,HDMITxData_option[uHDMITxBlk].bHTX21_FRL_Mode));
			}
			break;
		case HDMI_CONFIG_ID_REG_DUMP:{
				_hdmi2p1_dump_reg(HTX_LINK_CTRL0, 16 ,"HDMI page 0(0x6A)");
				_hdmi2p1_dump_reg(HTX_LINK_CTRL1, 16 ,"HDMI page 1(0x6B)");
				_hdmi2p1_dump_reg(HTX_LINK_CTRL2, 16 ,"HDMI page 2(0x60)");
				_hdmi2p1_dump_reg(HTX_PPS_CTRL, 16 ,"HDMI page 3(0x32)");
				_hdmi2p1_dump_reg(HTX_EMP_CTRL, 16 ,"HDMI page 4(0x33)");
				_hdmi2p1_dump_reg(HTX_PHY_CTRL, 16 ,"HDMI page 7");
				_hdmi2p1_dump_reg(HTX_FRL_CTRL, 16 ,"HDMI page F(0x34)");				
			}
			break;
			
		default:
			break;		
	}
	return ret;
}

#if 1
void hdmitx2p1_hotplug_on_config_mode(void)
{
#if 0
	disphdmi_debug(("%s\r\n",__func__));

	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	DISPDEV_IOCTRL          p_dispdev_control = p_hdmi_io_control[hdmi2p1_id];

	disphdmi_debug(("hdmitx_hotplug_on_config_mode\r\n"));

	dev_io_ctrl.SEL.SET_DITHER_EN.b_en = FALSE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_DITHER_EN, &dev_io_ctrl);

	if (g_dispdev_hdmi_mode[hdmi2p1_id] == HDMI_MODE_YCBCR422) {
		/* 16bits */
		dev_io_ctrl.SEL.SET_HDMI_FMT.hdmi_fmt = HDMI_FMT_HDMI422;
	} else if (g_dispdev_hdmi_mode[hdmi2p1_id] == HDMI_MODE_YCBCR420){
		/* 16bits */
		dev_io_ctrl.SEL.SET_HDMI_FMT.hdmi_fmt = HDMI_FMT_HDMI420;
	} else {
		/* 24bits */
		dev_io_ctrl.SEL.SET_HDMI_FMT.hdmi_fmt = HDMI_FMT_HDMI444;
	}
	p_dispdev_control(DISPDEV_IOCTRL_SET_HDMI_FMT, &dev_io_ctrl);
	disphdmi_debug(("DISPDEV_IOCTRL_SET_HDMI_FMT\r\n"));



	if (g_dispdev_hdmi_mode[hdmi2p1_id] == HDMI_MODE_RGB444) { /* output=RGB for 444 mode */
		/* Output=RGB for 444 mode */
		dev_io_ctrl.SEL.SET_ICST_EN.b_en    = TRUE;
		dev_io_ctrl.SEL.SET_ICST_EN.select = CST_YCBCR2RGB;
		p_dispdev_control(DISPDEV_IOCTRL_SET_ICST_EN, &dev_io_ctrl);
		hdmitx2p1_set_config(HDMI_CONFIG_ID_COMPONENT0, 0);
		hdmitx2p1_set_config(HDMI_CONFIG_ID_COMPONENT1, 1);
		hdmitx2p1_set_config(HDMI_CONFIG_ID_COMPONENT2, 2);
	} else if (g_dispdev_hdmi_mode[hdmi2p1_id] == HDMI_MODE_YCBCR444) {
		/* output=YCbCr for 444 mode */
		dev_io_ctrl.SEL.SET_ICST_EN.b_en    = FALSE;
		dev_io_ctrl.SEL.SET_ICST_EN.select = CST_RGB2YCBCR;
		p_dispdev_control(DISPDEV_IOCTRL_SET_ICST_EN, &dev_io_ctrl);
		hdmitx2p1_set_config(HDMI_CONFIG_ID_COMPONENT0, 1);
		hdmitx2p1_set_config(HDMI_CONFIG_ID_COMPONENT1, 2);
		hdmitx2p1_set_config(HDMI_CONFIG_ID_COMPONENT2, 0);
	} else if (g_dispdev_hdmi_mode[hdmi2p1_id] == HDMI_MODE_YCBCR422){
		/* output=YCbCr for 422 mode */
		dev_io_ctrl.SEL.SET_ICST_EN.b_en = FALSE;
		dev_io_ctrl.SEL.SET_ICST_EN.select = CST_RGB2YCBCR;
		p_dispdev_control(DISPDEV_IOCTRL_SET_ICST_EN, &dev_io_ctrl);

		hdmitx2p1_set_config(HDMI_CONFIG_ID_COMPONENT0, 0);
		hdmitx2p1_set_config(HDMI_CONFIG_ID_COMPONENT1, 2);
		hdmitx2p1_set_config(HDMI_CONFIG_ID_COMPONENT2, 1);
	} else {
		/* output=YCbCr for 420 mode */
		dev_io_ctrl.SEL.SET_ICST_EN.b_en = FALSE;
		dev_io_ctrl.SEL.SET_ICST_EN.select = CST_RGB2YCBCR;
		p_dispdev_control(DISPDEV_IOCTRL_SET_ICST_EN, &dev_io_ctrl);

		hdmitx2p1_set_config(HDMI_CONFIG_ID_COMPONENT0, 0);
		hdmitx2p1_set_config(HDMI_CONFIG_ID_COMPONENT1, 2);
		hdmitx2p1_set_config(HDMI_CONFIG_ID_COMPONENT2, 1);
	}

	switch(g_dispdev_hdmi_mode[hdmi2p1_id]){
		case HDMI_MODE_RGB444:
			DBG_WRN("[2p1] output_mode = [RGB888]\r\n");
			break;
		case HDMI_MODE_YCBCR444:
			DBG_WRN("[2p1] output_mode = [YCbCr444]\r\n");
			break;
		case HDMI_MODE_YCBCR422:
			DBG_WRN("[2p1] output_mode = [YCbCr422]\r\n");
			break;
		case HDMI_MODE_YCBCR420:
			DBG_WRN("[2p1] output_mode = [YCbCr420]\r\n");
			break;			
		default:
			DBG_WRN("[2p1] output_mode not support [%d]!\r\n",g_dispdev_hdmi_mode[hdmi2p1_id]);
			break;		
	}	
#endif
}
void hdmitx2p1_hotplug_on(void)
{
	int uHDMITxBlk = 0;
	disphdmi_debug(("%s\r\n",__func__));
	
	hdmitx2p1_init_ddc(1);

	disphdmi_debug(("Plug on: 1\r\n"));

	if(HDMITxData[uHDMITxBlk].bHTX21_FRL_Mode){
		HDMITX_OUTPUT_CONFIG_TEST_FRL(uHDMITxBlk);
	} else {
		HDMITX_OUTPUT_CONFIG_TEST(uHDMITxBlk);
	}

	disphdmi_debug(("Plug on: 2\r\n"));

	hdmitx2p1_set_config(HDMI_CONFIG_ID_ADAPT_4KP60_420,	ENABLE);

	disphdmi_debug(("Plug on: 3\r\n"));
		
	// init HDMI2p1 phy after clock ready
	hdmitx2p1_set_config(HDMI_CONFIG_ID_PHY_INIT, TRUE);
	
	disphdmi_debug(("Plug on: 4\r\n"));
	
/*	
	hdmitx2p1_set_config(HDMI_CONFIG_ID_VIDEO, g_dispdev_hdmi_vid[hdmi2p1_id]);
	hdmitx2p1_set_config(HDMI_CONFIG_ID_AUDIO, g_dispdev_aud_fmt[hdmi2p1_id]);

	// Configure HDMI mode by EDID content when plug on
	if ((!hdmitx2p1_get_config(HDMI_CONFIG_ID_FORCE_RGB)) && (hdmitx2p1_get_edid_info()&HDMITX_TV_YCBCR444)) {
		g_dispdev_hdmi_mode[hdmi2p1_id] = HDMI_MODE_YCBCR444;
	} else if ((!hdmitx2p1_get_config(HDMI_CONFIG_ID_FORCE_RGB)) && (hdmitx2p1_get_edid_info()&HDMITX_TV_YCBCR422)) {
		g_dispdev_hdmi_mode[hdmi2p1_id] = HDMI_MODE_YCBCR422;
	} else {
		g_dispdev_hdmi_mode[hdmi2p1_id] = HDMI_MODE_RGB444;
	}

	if (hdmitx2p1_get_config(HDMI_CONFIG_ID_FORCE_YUV444)) {
		g_dispdev_hdmi_mode[hdmi2p1_id] = HDMI_MODE_YCBCR444;
	} else if (hdmitx2p1_get_config(HDMI_CONFIG_ID_FORCE_YUV422)) {
		g_dispdev_hdmi_mode[hdmi2p1_id] = HDMI_MODE_YCBCR422;
	} else if (hdmitx2p1_get_config(HDMI_CONFIG_ID_FORCE_YUV420) && (g_dispdev_hdmi_vid[hdmi2p1_id] == HDMI_3840X2160P60 || g_dispdev_hdmi_vid[hdmi2p1_id] == HDMI_4096X2160P60)) {
		g_dispdev_hdmi_mode[hdmi2p1_id] = HDMI_MODE_YCBCR420;
	}

	if(g_Sink_adapt_4kp60_420_mode){
		g_dispdev_hdmi_mode[hdmi2p1_id] = HDMI_MODE_YCBCR420;
	}	
*/
	hdmitx2p1_hotplug_on_config_mode();


	
	
	hdmitx2p1_set_config(HDMI_CONFIG_ID_OE, 			TRUE);

	disphdmi_debug(("Plug on: 5\r\n"));


	if(hdmitx2p1_get_config(HDMI_CONFIG_ID_2P1_FORCE_FRL_EN) == 0){
		hdmitx2p1_set_config(HDMI_CONFIG_ID_SCDC_ENABLE,	TRUE);
		disphdmi_debug(("Plug on: 6\r\n"));
	} else {
		hdmitx2p1_set_config(HDMI_CONFIG_ID_2P1_FRL_LTP_EN,	TRUE);
		disphdmi_debug(("Plug on: 6.1 FRL task back\r\n"));
	}
}
void hdmitx2p1_hotplug_off(void)
{
	int uHDMITxBlk = 0;
	disphdmi_debug(("%s\r\n",__func__));
	if(hdmitx2p1_get_config(HDMI_CONFIG_ID_2P1_FORCE_FRL_EN) == 0){
		hdmitx2p1_set_config(HDMI_CONFIG_ID_SCDC_ENABLE,	FALSE);
		disphdmi_debug(("Plug off: 1.1\r\n"));
	} else {
		hdmitx2p1_set_config(HDMI_CONFIG_ID_2P1_FRL_LTP_EN,	FALSE);
		disphdmi_debug(("Plug off: 1.1 FRL task off\r\n"));
	}

	HDMITX_PHY_POWER_DOWN(uHDMITxBlk);
}
#endif


