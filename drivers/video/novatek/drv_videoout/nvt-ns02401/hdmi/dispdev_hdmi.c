/*
    Display HDMI device object

    @file       dispdev_hdmi.c
    @ingroup    mIDISPHDMITx
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "hdmi_port.h"
#include "hdmitx_int.h"
#include "hdmitx_reg.h"
#include <asm/arch/dispdevctrl.h>
#include "dispdev_hdmi.h"



/* module parameter define*/
int hdmi_cts_test = 1;
int hdmi_ddc_init_delay = 0;
int hdmi_hotfix_en = 1;
int g_hdmi_hotfix_id = -1;
int g_hdmi2_hotfix_id = -1;
//HDMITX_I2C_INFO *hdmitx_i2c_info[PLATFORM_MAX_ENGINE_CNT];
//HDMITX_I2C_INFO hdmitx_i2c_info_def[PLATFORM_MAX_ENGINE_CNT] = {0};

#define REALCHIP 1

#if 1//defined(__FREERTOS)
int skip_edid = 0;
int print_edid =0;
#endif


#if 0//def __KERNEL__
module_param(hdmi_cts_test, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hdmi_cts_test, "1 for hdmi cts test");

module_param(hdmi_ddc_init_delay, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hdmi_ddc_init_delay, " hdmi_ddc_init_delay ");

module_param(hdmi_hotfix_en, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(hdmi_hotfix_en, "1 for hdmi_hotfix_en enable");
#endif

/* macros */
#define DEFAULT_VID		HDMI_1920X1080P60


#if HDMI_DISOBJUSED
	#define DISPDEVIOUSED   1
#else
	#define DISPDEVIOUSED   0
#endif


#if DISPDEVIOUSED
DISPDEV_IOCTRL   p_hdmi_io_control[HDMI_ID_MAX];
HDMI_OBJ		 hdmi_obj[HDMI_ID_MAX]={
										{hdmitx2p1_set_config, hdmitx2p1_get_config, hdmitx2p1_get_edid_info},
										{hdmitx_set_config, hdmitx_get_config, hdmitx_get_edid_info},
										{hdmitx2_set_config, hdmitx2_get_config, hdmitx2_get_edid_info},
										};
#endif

/* dispdev_hdmi_obj api for HDMI2P1*/
static ER               dispdev_open_hdmi2p1(void);
static ER               dispdev_close_hdmi2p1(void);
static ER 				dispdev_preconfig_hdmi2p1(PDISP_HDMI_CFG config);
static void             dispdev_set_hdmi2p1_ioctrl(FP p_io_ctrl_func);
static ER               dispdev_get_hdmi2p1_size(DISPDEV_GET_PRESIZE *t_size);
static DISPDEV_OBJ		dispdev_hdmi2p1_obj = { dispdev_open_hdmi2p1, dispdev_close_hdmi2p1, NULL, NULL, NULL, dispdev_get_hdmi2p1_size, dispdev_set_hdmi2p1_ioctrl, {NULL, NULL }, dispdev_preconfig_hdmi2p1 };

#if DISPDEVIOUSED
static DISPDEVHDMI_CTRL dispdev_hdmi2p1_control = {dispdev_get_hdmi2p1_dev_obj, hdmitx2p1_set_config, hdmitx2p1_get_config, hdmitx2p1_get_edid_info, hdmitx2p1_get_video_ability, hdmitx2p1_get_edid_raw_data, hdmitx2p1_check_hotplug, hdmitx2p1_get_video_id};
#endif


/* dispdev_hdmi_obj api for HDMI1*/
static ER               dispdev_open_hdmi(void);
static ER               dispdev_close_hdmi(void);
static ER 				dispdev_preconfig_hdmi(PDISP_HDMI_CFG config);
static void             dispdev_set_hdmi_ioctrl(FP p_io_ctrl_func);
static ER               dispdev_get_hdmi_size(DISPDEV_GET_PRESIZE *t_size);
static DISPDEV_OBJ		dispdev_hdmi_obj = { dispdev_open_hdmi, dispdev_close_hdmi, NULL, NULL, NULL, dispdev_get_hdmi_size, dispdev_set_hdmi_ioctrl, {NULL, NULL }, dispdev_preconfig_hdmi };

#if DISPDEVIOUSED
static DISPDEVHDMI_CTRL dispdev_hdmi_control = {dispdev_get_hdmi_dev_obj, hdmitx_set_config, hdmitx_get_config, hdmitx_get_edid_info, hdmitx_get_video_ability, hdmitx_get_edid_raw_data, hdmitx_check_hotplug, hdmitx_get_video_id};
#endif

/* dispdev_hdmi_obj api for HDMI2*/
static ER               dispdev_open_hdmi2(void);
static ER               dispdev_close_hdmi2(void);
static ER 				dispdev_preconfig_hdmi2(PDISP_HDMI_CFG config);
static void             dispdev_set_hdmi2_ioctrl(FP p_io_ctrl_func);
static ER               dispdev_get_hdmi2_size(DISPDEV_GET_PRESIZE *t_size);
static DISPDEV_OBJ		dispdev_hdmi2_obj = { dispdev_open_hdmi2, dispdev_close_hdmi2, NULL, NULL, NULL, dispdev_get_hdmi2_size, dispdev_set_hdmi2_ioctrl, {NULL, NULL }, dispdev_preconfig_hdmi2 };

#if DISPDEVIOUSED
static DISPDEVHDMI_CTRL dispdev_hdmi2_control = {dispdev_get_hdmi2_dev_obj, hdmitx2_set_config, hdmitx2_get_config, hdmitx2_get_edid_info, hdmitx2_get_video_ability, hdmitx2_get_edid_raw_data, hdmitx2_check_hotplug, hdmitx2_get_video_id};
#endif


#define     VIDMAX      (100)
const static DISPDEV_HDMI_TIMING  disp_video_timing[VIDMAX] = {
	/* Video Format timing structure defined in CEA-861-D */
	/*           VID                 HS       VS       HVLD      VVLD      CLK       DE  PROGRESSIVE HSYNC  HTOT      HSTART       HEND            VSYNC    VTOT       VOSTART     VOEND       VESTART       VEEND      WINX,   WINY */
	/* 1*/{HDMI_640X480P60,       ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     96, (160 + 640), (96 + 48), (96 + 48 + 640),        2, (480 + 45), (2 + 33), (2 + 33 + 480), (2 + 33), (2 + 33 + 480),   16,     12   },
	/* 2*/{HDMI_720X480P60,       ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     62, (138 + 720), (62 + 60), (62 + 60 + 720),        6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30), (6 + 30 + 480),   20,     12   },
	/* 3*/{HDMI_720X480P60_16X9,  ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     62, (138 + 720), (62 + 60), (62 + 60 + 720),        6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30), (6 + 30 + 480),   20,     12   },
	/* 4*/{HDMI_1280X720P60,      ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     40, (370 + 1280), (40 + 220), (40 + 220 + 1280),      5, (720 + 30), (5 + 20), (5 + 20 + 720), (5 + 20), (5 + 20 + 720),   30,     20   },
	/* 5*/{HDMI_1920X1080I60,     ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,    44, (280 + 1920), (44 + 148), (44 + 148 + 1920),     10, (1080 + 45), (10 + 15 * 2), (10 + 30 + 1080), (10 + 30 + 1), (10 + 30 + 1080 + 1),   45,     30   },
	/* 6*/{HDMI_720X480I60,       ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   124, (276 + 1440),(124 + 114), (124 + 114 + 1440), 6, (480 + 45), (6 + 15 * 2), (6 + 30 + 480), (6 + 15 * 2 + 1), (6 + 30 + 480 + 1),   16,     12   },
	/* 7*/{HDMI_720X480I60_16X9,  ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   124, (276 + 1440),(124 + 114), (124 + 114 + 1440), 6, (480 + 45), (6 + 15 * 2), (6 + 30 + 480), (6 + 15 * 2 + 1), (6 + 30 + 480 + 1),   16,     12   },
	/* 8*/{HDMI_720X240P60,       ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    124, (276 + 1440),(124 + 114), (124 + 114 + 1440), 3, (240 + 22), (3 + 15), (3 + 15 + 240), (3 + 15), (3 + 15 + 240),   16,      6   },
	/* 9*/{HDMI_720X240P60_16X9,  ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    124, (276 + 1440),(124 + 114), (124 + 114 + 1440), 3, (240 + 22), (3 + 15), (3 + 15 + 240), (3 + 15), (3 + 15 + 240),   16,      6   },
	/*10*/{HDMI_1440X480I60,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   248, (552 + 2880),(248 + 228), (248 + 228 + 2880), 6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30 + 1), (6 + 30 + 480 + 1),   32,     12   },
	/*11*/{HDMI_1440X480I60_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   248, (552 + 2880),(248 + 228), (248 + 228 + 2880), 6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30 + 1), (6 + 30 + 480 + 1),   32,     12   },
	/*12*/{HDMI_1440X240P60,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    248, (552 + 2880),(248 + 228), (248 + 228 + 2880), 3, (240 + 22), (3 + 15), (3 + 15 + 240), (3 + 15), (3 + 15 + 240),   32,      6   },
	/*13*/{HDMI_1440X240P60_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    248, (552 + 2880),(248 + 228), (248 + 228 + 2880), 3, (240 + 22), (3 + 15), (3 + 15 + 240), (3 + 15), (3 + 15 + 240),   32,      6   },
	/*14*/{HDMI_1440X480P60,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    124, (276 + 1440), (124 + 120), (124 + 120 + 1440),     6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30), (6 + 30 + 480),   32,     12   },
	/*15*/{HDMI_1440X480P60_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    124, (276 + 1440), (124 + 120), (124 + 120 + 1440),     6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30), (6 + 30 + 480),   32,     12   },
	/*16*/{HDMI_1920X1080P60,     ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     44, (280 + 1920), (44 + 148), (44 + 148 + 1920),      5, (1080 + 45), (5 + 36), (5 + 36 + 1080), (5 + 36), (5 + 36 + 1080),   45,     30   },
	/*17*/{HDMI_720X576P50,       ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     64, (144 + 720), (64 + 68), (64 + 68 + 720),        5, (576 + 49), (5 + 39), (5 + 39 + 576), (5 + 39), (5 + 39 + 576),   20,     16   },
	/*18*/{HDMI_720X576P50_16X9,  ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     64, (144 + 720), (64 + 68), (64 + 68 + 720),        5, (576 + 49), (5 + 39), (5 + 39 + 576), (5 + 39), (5 + 39 + 576),   20,     16   },
	/*19*/{HDMI_1280X720P50,      ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     40, (700 + 1280), (40 + 220), (40 + 220 + 1280),      5, (720 + 30), (5 + 20), (5 + 20 + 720), (5 + 20), (5 + 20 + 720),   30,     20   },
	/*20*/{HDMI_1920X1080I50,     ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,    44, (720 + 1920), (44 + 148), (44 + 148 + 1920),     10, (1080 + 45), (10 + 15 * 2), (10 + 30 + 1080), (10 + 30 + 1), (10 + 30 + 1080 + 1),   45,     30   },
	/*21*/{HDMI_720X576I50,       ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   126, (288 + 1440), (126 + 138), (126 + 138 + 1440), 6, (576 + 49), (6 + 19 * 2), (6 + 38 + 576), (6 + 38 + 1), (6 + 38 + 576 + 1),   16,     16   },
	/*22*/{HDMI_720X576I50_16X9,  ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   126, (288 + 1440), (126 + 138), (126 + 138 + 1440), 6, (576 + 49), (6 + 19 * 2), (6 + 38 + 576), (6 + 38 + 1), (6 + 38 + 576 + 1),   16,     16   },
	/* Warning: VID 23/24 's V-BP in CEA-861D is 18 lines but in Astro VA-1809 is 19 lines. */
	/*23*/{HDMI_720X288P50,       ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    126, (288 + 1440), (126 + 138), (126 + 138 + 1440), 3, (288 + 24), (3 + 19), (3 + 19 + 288), (3 + 19), (3 + 19 + 288),   16,      8   },
	/*24*/{HDMI_720X288P50_16X9,  ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    126, (288 + 1440), (126 + 138), (126 + 138 + 1440), 3, (288 + 24), (3 + 19), (3 + 19 + 288), (3 + 19), (3 + 19 + 288),   16,      8   },
	/*25*/{HDMI_1440X576I50,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   252, (576 + 2880), (252 + 276), (252 + 276 + 2880), 6, (576 + 49), (6 + 19 * 2), (6 + 38 + 576), (6 + 38 + 1), (6 + 38 + 576 + 1),   32,     16   },
	/*26*/{HDMI_1440X576I50_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   252, (576 + 2880), (252 + 276), (252 + 276 + 2880), 6, (576 + 49), (6 + 19 * 2), (6 + 38 + 576), (6 + 38 + 1), (6 + 38 + 576 + 1),   32,     16   },
	/*27*/{HDMI_1440X288P50,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    252, (576 + 2880), (252 + 276), (252 + 276 + 2880), 3, (288 + 24), (3 + 19), (3 + 19 + 288), (3 + 19), (3 + 19 + 288),   32,      8   },
	/*28*/{HDMI_1440X288P50_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    252, (576 + 2880), (252 + 276), (252 + 276 + 2880), 3, (288 + 24), (3 + 19), (3 + 19 + 288), (3 + 19), (3 + 19 + 288),   32,      8   },
	/*29*/{HDMI_1440X576P50,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    128, (288 + 1440), (128 + 136), (128 + 136 + 1440),     5, (576 + 49), (5 + 39), (5 + 39 + 576), (5 + 39), (5 + 39 + 576),   32,     16   },
	/*30*/{HDMI_1440X576P50_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    128, (288 + 1440), (128 + 136), (128 + 136 + 1440),     5, (576 + 49), (5 + 39), (5 + 39 + 576), (5 + 39), (5 + 39 + 576),   32,     16   },
	/*31*/{HDMI_1920X1080P50,     ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     44, (720 + 1920), (44 + 148), (44 + 148 + 1920),      5, (1080 + 45), (5 + 36), (5 + 36 + 1080), (5 + 36), (5 + 36 + 1080),   45,     30   },
	/*32*/{HDMI_1920X1080P24,     ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     44, (830 + 1920), (44 + 148), (44 + 148 + 1920),      5, (1080 + 45), (5 + 36), (5 + 36 + 1080), (5 + 36), (5 + 36 + 1080),   45,     30   },
	/*33*/{HDMI_1920X1080P25,     ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     44, (720 + 1920), (44 + 148), (44 + 148 + 1920),      5, (1080 + 45), (5 + 36), (5 + 36 + 1080), (5 + 36), (5 + 36 + 1080),   45,     30   },
#if 0	// 2p mode
	///*34*/{HDMI_1920X1080P30,     ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     44/2, (280 + 1920)/2, (44 + 148)/2, (44 + 148 + 1920)/2,      5, (1080 + 45), (5 + 36), (5 + 36 + 1080), (5 + 36), (5 + 36 + 1080),   45,     30   },
	/*34*/{HDMI_1920X1080P30,     ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     44/2, (280 + 1920)/2, (44 + 148)/2, (44 + 148 + 1920)/2,      5, (1080 + 45), (5 + 26), (5 + 26 + 1080), (5 + 26), (5 + 26 + 1080),   45,     30   },
#else
	/*34*/{HDMI_1920X1080P30,     ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     44, (280 + 1920), (44 + 148), (44 + 148 + 1920),      5, (1080 + 45), (5 + 36), (5 + 36 + 1080), (5 + 36), (5 + 36 + 1080),   45,     30   },
#endif
	//35~38 Width is 2880  IDE Un-Supported
	/*35*/{35,                    ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,  248 / 2, (552 + 2880) / 2, (248 + 240) / 2, (248 + 240 + 2880) / 2, 6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30), (6 + 30 + 480),   64,     12   },
	/*36*/{36,                    ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,  248 / 2, (552 + 2880) / 2, (248 + 240) / 2, (248 + 240 + 2880) / 2, 6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30), (6 + 30 + 480),   64,     12   },
	/*37*/{37,                    ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,  256 / 2, (576 + 2880) / 2, (256 + 272) / 2, (256 + 272 + 2880) / 2, 5, (576 + 49), (5 + 39), (5 + 39 + 576), (5 + 39), (5 + 39 + 576),   64,     16   },
	/*38*/{38,                    ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,  256 / 2, (576 + 2880) / 2, (256 + 272) / 2, (256 + 272 + 2880) / 2, 5, (576 + 49), (5 + 39), (5 + 39 + 576), (5 + 39), (5 + 39 + 576),   64,     16   },
	/*39*/{HDMI_1920X1080I50_VT1250, ACT_HIGH, ACT_LOW, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   168, (384 + 1920), (168 + 184), (168 + 184 + 1920),    10, (1080 + 170), (10 + 57 * 2), (10 + 114 + 1080), (10 + 57 * 2 + 1), (10 + 114 + 1080 + 1), 45,     30   },
	/*40*/{HDMI_1920X1080I100,    ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,    44, (720 + 1920), (44 + 148), (44 + 148 + 1920),     10, (1080 + 45), (10 + 15 * 2), (10 + 30 + 1080), (10 + 30 + 1), (10 + 30 + 1080 + 1),  45,     30   },
	/*41*/{HDMI_1280X720P100,     ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     40, (700 + 1280), (40 + 220), (40 + 220 + 1280),      5, (720 + 30), (5 + 20), (5 + 20 + 720), (5 + 20), (5 + 20 + 720),   32,     20   },
	/*42*/{HDMI_720X576P100,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     64, (144 + 720), (64 + 68), (64 + 68 + 720),        5, (576 + 49), (5 + 39), (5 + 39 + 576), (5 + 39), (5 + 39 + 576),   20,     16   },
	/*43*/{HDMI_720X576P100_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     64, (144 + 720), (64 + 68), (64 + 68 + 720),        5, (576 + 49), (5 + 39), (5 + 39 + 576), (5 + 39), (5 + 39 + 576),   20,     16   },
	/*44*/{HDMI_720X576I100,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   126, (288 + 1440), (126 + 138), (126 + 138 + 1440), 6, (576 + 49), (6 + 19 * 2), (6 + 38 + 576), (6 + 38 + 1), (6 + 38 + 576 + 1), 16,     16   },
	/*45*/{HDMI_720X576I100_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   126, (288 + 1440), (126 + 138), (126 + 138 + 1440), 6, (576 + 49), (6 + 19 * 2), (6 + 38 + 576), (6 + 38 + 1), (6 + 38 + 576 + 1), 16,     16   },
	/*46*/{HDMI_1920X1080I120,    ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,    44, (280 + 1920), (44 + 148), (44 + 148 + 1920),     10, (1080 + 45), (10 + 15 * 2), (10 + 30 + 1080), (10 + 30 + 1), (10 + 30 + 1080 + 1),  45,     30   },
	/*47*/{HDMI_1280X720P120,     ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     40, (370 + 1280), (40 + 220), (40 + 220 + 1280),      5, (720 + 30), (5 + 20), (5 + 20 + 720), (5 + 20), (5 + 20 + 720),   30,     20   },
	/*48*/{HDMI_720X480P120,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     62, (138 + 720), (62 + 60), (62 + 60 + 720),        6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30), (6 + 30 + 480),   20,     12   },
	/*49*/{HDMI_720X480P120_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     62, (138 + 720), (62 + 60), (62 + 60 + 720),        6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30), (6 + 30 + 480),   20,     12   },
	/*50*/{HDMI_720X480I120,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   124, (276 + 1440), (124 + 114), (124 + 114 + 1440), 6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30 + 1), (6 + 30 + 480 + 1),   16,     12   },
	/*51*/{HDMI_720X480I120_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   124, (276 + 1440), (124 + 114), (124 + 114 + 1440), 6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30 + 1), (6 + 30 + 480 + 1),   16,     12   },
	/*52*/{HDMI_720X576P200,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     64, (144 + 720), (64 + 68), (64 + 68 + 720),        5, (576 + 49), (5 + 39), (5 + 39 + 576), (5 + 39), (5 + 39 + 576),   20,     16   },
	/*53*/{HDMI_720X576P200_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     64, (144 + 720), (64 + 68), (64 + 68 + 720),        5, (576 + 49), (5 + 39), (5 + 39 + 576), (5 + 39), (5 + 39 + 576),   20,     16   },
	/*54*/{HDMI_720X576I200,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   126, (288 + 1440), (126 + 138), (126 + 138 + 1440), 6, (576 + 49), (6 + 19 * 2), (6 + 38 + 576), (6 + 38 + 1), (6 + 38 + 576 + 1), 16,     16   },
	/*55*/{HDMI_720X576I200_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   126, (288 + 1440), (126 + 138), (126 + 138 + 1440), 6, (576 + 49), (6 + 19 * 2), (6 + 38 + 576), (6 + 38 + 1), (6 + 38 + 576 + 1), 16,     16   },
	/*56*/{HDMI_720X480P240,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     62, (138 + 720), (62 + 60), (62 + 60 + 720),        6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30), (6 + 30 + 480),   20,     12   },
	/*57*/{HDMI_720X480P240_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     62, (138 + 720), (62 + 60), (62 + 60 + 720),        6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30), (6 + 30 + 480),   20,     12   },
	/*58*/{HDMI_720X480I240,      ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   124, (276 + 1440), (124 + 114), (124 + 114 + 1440), 6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30 + 1), (6 + 30 + 480 + 1),   16,     12   },
	/*59*/{HDMI_720X480I240_16X9, ACT_LOW,  ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, FALSE,   124, (276 + 1440), (124 + 114), (124 + 114 + 1440), 6, (480 + 45), (6 + 30), (6 + 30 + 480), (6 + 30 + 1), (6 + 30 + 480 + 1),   16,     12   },

	/*60*/{HDMI_1280X720P24,      ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     40, (2020 + 1280), (40 + 220), (40 + 220 + 1280),      5, (720 + 30), (5 + 20), (5 + 20 + 720), (5 + 20), (5 + 20 + 720),   30,     20   },
	/*61*/{HDMI_1280X720P25,      ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     40, (2680 + 1280), (40 + 220), (40 + 220 + 1280),      5, (720 + 30), (5 + 20), (5 + 20 + 720), (5 + 20), (5 + 20 + 720),   30,     20   },
	/*62*/{HDMI_1280X720P30,      ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     40, (2020 + 1280), (40 + 220), (40 + 220 + 1280),      5, (720 + 30), (5 + 20), (5 + 20 + 720), (5 + 20), (5 + 20 + 720),   30,     20   },

	/*83*/ {HDMI_1680X720P60,     ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     40, (520 + 1680), (40 + 220), (40 + 220 + 1680),       5, (720 + 30), (5 + 20), (5 + 20 + 720), (5 + 20), (5 + 20 + 720), 90,     60   },

	/*93*/ {HDMI_3840X2160P24,    ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     88, (1660 + 3840), (88 + 296), (88 + 296 + 3840),      10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90,     60   },
	/*94*/ {HDMI_3840X2160P25,    ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     88, (1440 + 3840), (88 + 296), (88 + 296 + 3840),      10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90,     60   },
#if 0 //2p mode
	/*95*/ {HDMI_3840X2160P30,    ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     88/2, (560 + 3840)/2, (88 + 296)/2, (88 + 296 + 3840)/2,      10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90,     60   },	
#else
	/*95*/ {HDMI_3840X2160P30,	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	   88, (560 + 3840), (88 + 296), (88 + 296 + 3840), 	 10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90, 	60	 }, 
#endif
	/*96*/ {HDMI_3840X2160P50,    ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     88, (1440 + 3840), (88 + 296), (88 + 296 + 3840),      10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90,     60   },
	/*97*/ {HDMI_3840X2160P60,    ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     88, (560 + 3840), (88 + 296), (88 + 296 + 3840),      10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90,     60   },
	/*98*/ {HDMI_4096X2160P24,	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	   88, (1404 + 4096), (88 + 296), (88 + 296 + 4096),	  10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90,	 60   },
	/*99*/ {HDMI_4096X2160P25,	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	   88, (1184 + 4096), (88 + 128), (88 + 128 + 4096),	  10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90,	 60   },
	/*100*/{HDMI_4096X2160P30,	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	   88, (304 + 4096), (88 + 128), (88 + 128 + 4096), 	 10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90, 	60	 },
	/*101*/{HDMI_4096X2160P50,    ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE, 	88, (1184 + 4096), (88 + 128), (88 + 128+ 4096),	   10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90,	  60   },
 	/*102*/{HDMI_4096X2160P60,    ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE, 	88, (304 + 4096), (88 + 128), (88 + 128 + 4096),	  10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90,	 60   },


 /*128+95*/{HDMI14B_3840X2160P30, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     88, (560 + 3840), (88 + 296), (88 + 296 + 3840),      10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90,     60   },
 /*128+94*/{HDMI14B_3840X2160P25, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     88, (1440 + 3840), (88 + 296), (88 + 296 + 3840),      10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90,     60   },
 /*128+93*/{HDMI14B_3840X2160P24, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     88, (1660 + 3840), (88 + 296), (88 + 296 + 3840),      10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90,     60   },
 /*128+98*/{HDMI14B_4096X2160P24, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,     88, (1404 + 4096), (88 + 296), (88 + 296 + 4096),      10, (2160 + 90), (10 + 72), (10 + 72 + 2160), (10 + 72), (10 + 72 + 2160), 90,     60   },

#if 1//2pmode
	/*194*/{HDMI_7680X4320P24,	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 176/2, (3320 + 7680)/2, (176 + 592)/2, (176 + 592 + 7680)/2,	20, (4320 + 180), (20 + 144), (20 + 144 + 4320), (20 + 144), (20 + 144 + 4320), 90, 	60	 },
	/*195*/{HDMI_7680X4320P25,	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 176/2, (3120 + 7680)/2, (176 + 592)/2, (176 + 592 + 7680)/2,	20, (4320 + 80),  (20 + 44),  (20 +  44 + 4320), (20 + 44),  (20 + 44 + 4320), 90, 	60	 },
	/*196*/{HDMI_7680X4320P30,	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 176/2, (1320 + 7680)/2, (176 + 592)/2, (176 + 592 + 7680)/2,	20, (4320 + 80),  (20 + 44),  (20 +  44 + 4320), (20 + 44),  (20 + 44 + 4320), 90, 	60	 },
#else
	/*194*/{HDMI_7680X4320P24,	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 176, (3320 + 7680), (176 + 592), (176 + 592 + 7680),	20, (4320 + 180), (20 + 144), (20 + 144 + 4320), (20 + 144), (20 + 144 + 4320), 90, 	60	 },
	/*195*/{HDMI_7680X4320P25,	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 176, (3120 + 7680), (176 + 592), (176 + 592 + 7680),	20, (4320 + 80),  (20 + 44),  (20 +  44 + 4320), (20 + 44),  (20 + 44 + 4320), 90,	60	 },
	/*196*/{HDMI_7680X4320P30,	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 176, (1320 + 7680), (176 + 592), (176 + 592 + 7680),	20, (4320 + 80),  (20 + 44),  (20 +  44 + 4320), (20 + 44),  (20 + 44 + 4320), 90,	60	 },
#endif

	   /*			 VID				 HS 	  VS	   HVLD 	 VVLD	   CLK		 DE  PROGRESSIVE HSYNC	HTOT	  HSTART	   HEND 				  VSYNC    VTOT 	  VOSTART	  VOEND 	  VESTART		VEEND	   WINX,   WINY */
	/*128+35*/{HDMI_1024X768P60,	 ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 136, (1024 + 320), (136 + 160), (136+ 160 + 1024), 	  6, (768 + 38),  (6 + 29), (6 + 29 +  768), (6 + 29), (6 + 29 +  768), 45, 	30	 },
	/*128+36*/{HDMI_1280X1024P60,	 ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 112, (1280 + 408), (112 + 248), (112+ 248 + 1280), 	  3, (1024 + 42), (3 + 38), (3 + 38 + 1024), (3 + 38), (3 + 38 + 1024), 45, 	30	 },
	/*128+37*/{HDMI_1600X1200P60,	 ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 192, (1600 + 560), (192 + 304), (192+ 304 + 1600), 	  3, (1200 + 50), (3 + 46), (3 + 46 + 1200), (3 + 46), (3 + 46 + 1200), 45, 	30	 },
	/*128+38*/{HDMI_2560X1440P30,	 ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 32,  (2560 + 160), (32  + 80),  (32 +	80 + 2560), 	  5, (1440 + 21), (5 + 13), (5 + 13 + 1440), (5 + 13), (5 + 13 + 1440), 45, 	30	 },
	/*128+39*/{HDMI_2560X1440P60,	 ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 32,  (2560 + 160), (32  + 80),  (32 +	80 + 2560), 	  5, (1440 + 41), (5 + 33), (5 + 33 + 1440), (5 + 33), (5 + 33 + 1440), 45, 	30	},
	
	/*128+41*/{HDMI_1440X900P60,		ACT_LOW, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    152, (1440 + 464), (152 + 232), (152+ 232 + 1440),		6, (900 + 34),	(6 + 25), (6 + 25 +  900), (6 + 25), (6 + 25 +	900), 36,	  22   },
	/*128+42*/{HDMI_1680X1050P60,		ACT_LOW, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    176, (1680 + 560), (176 + 280), (176+ 280 + 1680),		6, (1050 + 39), (6 + 30), (6 + 30 + 1050), (6 + 30), (6 + 30 + 1050), 42,	  26   },
	/*128+43*/{HDMI_1920X1200P60,	   ACT_LOW, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	  200, (1920 + 672), (200 + 336), (200+ 336 + 1920),	   6, (1200 + 45), (6 + 36), (6 + 36 + 1200), (6 + 36), (6 + 36 + 1200), 48,	 30   },
	/*128+44*/{HDMI_640X480P72, 	   ACT_LOW,  ACT_LOW, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	  40,  (640 + 176 + 16), (40  + 120 + 8),  (40 + 120 +8 + 640),3, (480 + 24 + 16), (3 + 20 + 8), (3 + 20 + 8 + 480), (3 + 20 + 8), (3 + 20 + 8 + 480), 16,	  12   },// this timing needs to take care of border
	/*128+45*/{HDMI_640X480P75, 	  ACT_LOW,	ACT_LOW, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,  64,	(640 + 200), (64  + 120),  (64 + 120 + 640),		3, (480 + 20), (3 + 16), (3 + 16 + 480), (3 + 16), (3 + 16 + 480), 16,		12	},
	/*128+46*/{HDMI_800X600P56, 	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	  72,  (800 + 224),  (72 + 128),   (72+ 128 + 800), 	   2, (600 + 25), (2 + 22), (2 + 22 + 600), (2 + 22), (2 + 22 + 600), 20,	  15   },
	/*128+47*/{HDMI_800X600P72, 	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 120,  (800 + 240), (120 + 64), (120+ 64 + 800),		   6, (600 + 66), (6 + 23), (6 + 23 + 600), (6 + 23), (6 + 23 + 600), 20,	  15   },
	/*128+48*/{HDMI_800X600P75, 	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	  80,  (800 + 256), (80 + 160), (80+ 160 + 800),		   3, (600 + 25), (3 + 21), (3 + 21 + 600), (3 + 21), (3 + 21 + 600), 20,	  15   },
	/*128+49*/{HDMI_1024X768P70,	   ACT_LOW,  ACT_LOW, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 136,  (1024 + 304), (136  + 144),(136 + 144 + 1024),	   6, (768 + 38), (6 + 29), (6 + 29 + 768), (6 + 29), (6 + 29 + 768), 25,	  19   }, 
	/*128+50*/{HDMI_1024X768P75,	   ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    96,	(1024 + 288), (96  + 176),	(96 + 176 + 1024),		3, (768 + 32), (3 + 28), (3 + 28 + 768), (3 + 28), (3 + 28 + 768), 25,	   19	}, 
	/*128+51*/{HDMI_1280X1024P75,	   ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,   144,	(1280 + 408), (144 + 248), (144+ 248 + 1280),		3, (1024 + 42),(3 + 38), (3 + 38 + 1024), (3 + 38), (3 + 38 +1024), 32, 	25	 }, 
	/*128+52*/{HDMI_1400X1050P60,	  ACT_LOW, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 144,  (1400 + 464), (144 + 232), (144+ 232 + 1400),	   4, (1050 + 39),(4 + 32), (4 + 32 + 1050), (4 + 32), (4 + 32 + 1050), 35, 	26	 }, 
	/*128+53*/{HDMI_1280X800P60,	  ACT_LOW, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	 128,  (1280 + 400), (128 + 200), (128+ 200 + 1280),	   6, (800 + 31), (6 + 22), (6 + 22 + 800), (6 + 22), (6 + 22 + 800), 32,	  20   }, 
	/*128+54*/{HDMI_1920X1200P75,   ACT_LOW,  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,    208,   (1920 + 688), (208  + 344),  (208 + 344 + 1920),	6, (1200 + 55),(6 + 46), (6 + 46 + 1200),(6 + 46), (6 + 46 + 1200), 45,	 30  },
	/*128+55*/{HDMI_800X600P60, 	  ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_HIGH, ACT_LOW, ACT_HIGH, TRUE,	  128, (800 + 256), (128 + 88), (128+ 88 + 800),		   4, (600 + 28), (4 + 23), (4 + 23 + 600), (4 + 23), (4 + 23 + 600), 20,	  15   },
		

};


const HDMI_VDOIDCAP disp_video_id[MAX_ID_CAP] = {
	{HDMI_640X480P60, 		"640X480P60"		,  640,  480, 60, 0},
	{HDMI_720X480P60, 		"720X480P60"		,  720,  480, 60, 0},
	{HDMI_720X480P60_16X9, 	"720X480P60_16X9"	,  720,  480, 60, 0},
	{HDMI_1280X720P60, 		"1280X720P60"		, 1280,  720, 60, 1},
	{HDMI_1920X1080I60, 	"1920X1080I60"		, 1920, 1080, 60, 1},
	{HDMI_720X480I60, 		"720X480I60"		,  720,  480, 60, 1},
	{HDMI_720X480I60_16X9, 	"720X480I60_16X9"   ,  720,  480, 60, 1},
	{HDMI_720X240P60, 		"720X240P60"		,  720,  240, 60, 0},
	{HDMI_720X240P60_16X9, 	"720X240P60_16X9"	,  720,  240, 60, 0},
	{HDMI_1440X480I60, 		"1440X480I60"		, 1440,  480, 60, 1},
	{HDMI_1440X480I60_16X9, "1440X480I60_16X9"  , 1440,  480, 60, 1},
	{HDMI_1440X240P60,		"1440X240P60"		, 1440,  240, 60, 0},
	{HDMI_1440X240P60_16X9, "1440X240P60_16X9"	, 1440,  240, 60, 0},
	{HDMI_1440X480P60, 		"1440X480P60"		, 1440,  480, 60, 0},
	{HDMI_1440X480P60_16X9, "1440X480P60_16X9"	, 1440,  480, 60, 0},
	{HDMI_1920X1080P60, 	"1920X1080P60"		, 1920, 1080, 60, 0},
	{HDMI_720X576P50, 		"720X576P50"		,  720,  576, 50, 0},
	{HDMI_720X576P50_16X9,  "720X576P50_16X9"	,  720,  576, 50, 0},
	{HDMI_1280X720P50, 		"1280X720P50"		, 1280,  720, 50, 0},
	{HDMI_1920X1080I50, 	"1920X1080I50"		, 1920, 1080, 50, 1},
	{HDMI_720X576I50, 		"720X576I50"		,  720,  576, 50, 1},
	{HDMI_720X576I50_16X9,  "720X576I50_16X9"	,  720,  576, 50, 1},
	{HDMI_720X288P50, 		"720X288P50"		,  720,  288, 50, 0},
	{HDMI_720X288P50_16X9, 	"720X288P50_16X9"	,  720,  288, 50, 0},
	{HDMI_1440X576I50, 		"1440X576I50"		, 1440,  576, 50, 1},
	{HDMI_1440X576I50_16X9, "1440X576I50_16X9"	, 1440,  576, 50, 1},
	{HDMI_1440X288P50, 		"1440X288P50"		, 1440,  288, 50, 0},
	{HDMI_1440X288P50_16X9, "1440X288P50_16X9"	, 1440,  288, 50, 0},
	{HDMI_1440X576P50, 		"1440X576P50"		, 1440,  576, 50, 0},
	{HDMI_1440X576P50_16X9, "1440X576P50_16X9"	, 1440,  576, 50, 0},
	{HDMI_1920X1080P50, 	"1920X1080P50"		, 1920, 1080, 50, 0},
	{HDMI_1920X1080P24, 	"1920X1080P24"		, 1920, 1080, 24, 0},
	{HDMI_1920X1080P25, 	"1920X1080P25"		, 1920, 1080, 25, 0},
	{HDMI_1920X1080P30, 	"1920X1080P30"		, 1920, 1080, 30, 0},
	{HDMI_1920X1080I100, 	"1920X1080I100"		, 1920, 1080,100, 1},
	{HDMI_1280X720P100, 	"1280X720P100"		, 1280,  720,100, 0},
	{HDMI_720X576P100, 		"720X576P100"		,  720,  576,100, 0},
	{HDMI_720X576P100_16X9, "720X576P100_16X9"	,  720,  576,100, 0},
	{HDMI_720X576I100, 		"720X576I100"		,  720,  576,100, 1},
	{HDMI_720X576I100_16X9, "720X576I100_16X9"	,  720,  576,100, 1},
	{HDMI_1920X1080I120, 	"1920X1080I120"		, 1920, 1080,100, 1},
	{HDMI_1280X720P120, 	"1280X720P120"		, 1280,  720,120, 0},
	{HDMI_720X480P120, 		"720X480P120"		,  720,  480,120, 0},
	{HDMI_720X480P120_16X9, "720X480P120_16X9"	,  720,  480,120, 0},
	{HDMI_720X480I120, 		"720X480I120"		,  720,  480,120, 1},
	{HDMI_720X480I120_16X9, "720X480I120_16X9"	,  720,  480,120, 1},
	{HDMI_720X576P200,		"720X576P200"		,  720,  576,200, 0},
	{HDMI_720X576P200_16X9, "720X576P200_16X9"	,  720,  576,200, 0},
	{HDMI_720X576I200, 		"720X576I200"		,  720,  576,200, 1},
	{HDMI_720X576I200_16X9, "720X576I200_16X9"	,  720,  576,200, 1},
	{HDMI_720X480P240, 		"720X480P240"		,  720,  480,240, 0},
	{HDMI_720X480P240_16X9, "720X480P240_16X9"	,  720,  480,240, 0},
	{HDMI_720X480I240, 		"720X480I240"		,  720,  480,240, 1},
	{HDMI_720X480I240_16X9, "720X480I240_16X9"	,  720,  480,240, 1},
	{HDMI_1680X720P60, 		"1680X720P60"		, 1680,  720, 60, 0},
	{HDMI_3840X2160P24, 	"3840X2160P24"		, 3840, 2160, 24, 0},
	{HDMI_3840X2160P25, 	"3840X2160P25"		, 3840, 2160, 25, 0},
	{HDMI_3840X2160P30, 	"3840X2160P30"		, 3840, 2160, 30, 0},
	{HDMI_3840X2160P50, 	"3840X2160P50"		, 3840, 2160, 50, 0},
	{HDMI_3840X2160P60, 	"3840X2160P60"		, 3840, 2160, 60, 0},
	{HDMI_4096X2160P24,		"4096X2160P24"		, 4096, 2160, 24, 0},
	{HDMI_4096X2160P25, 	"4096X2160P25"		, 4096, 2160, 25, 0},
	{HDMI_4096X2160P30, 	"4096X2160P30"		, 4096, 2160, 30, 0},
	{HDMI_4096X2160P50, 	"4096X2160P50"		, 4096, 2160, 50, 0},
	{HDMI_4096X2160P60, 	"4096X2160P60"		, 4096, 2160, 60, 0},
	{HDMI_7680X4320P24,		"7680X4320P24"		, 7680, 4320, 24, 0},
	{HDMI_7680X4320P25, 	"7680X4320P25"		, 7680, 4320, 25, 0},
	{HDMI_7680X4320P30, 	"7680X4320P30"		, 7680, 4320, 30, 0},	
	{HDMI14B_3840X2160P30, 	"HDMI14B_3840X2160P30",0,0,0,0},
	{HDMI14B_3840X2160P25, 	"HDMI14B_3840X2160P25",0,0,0,0},
	{HDMI14B_3840X2160P24, 	"HDMI14B_3840X2160P24",0,0,0,0},
	{HDMI14B_4096X2160P24, 	"HDMI14B_4096X2160P24",0,0,0,0},
	{HDMI_1024X768P60,		"1024X768P60"		, 1024,  768, 60, 0},
	{HDMI_1280X1024P60, 	"1280X1024P60"		, 1280, 1024, 60, 0},
	{HDMI_1600X1200P60, 	"1600X1200P60"		, 1600, 1200, 60, 0},
	{HDMI_2560X1440P30, 	"2560X1440P30"		, 2560, 1440, 30, 0},
	{HDMI_2560X1440P60, 	"2560X1440P60"		, 2560, 1440, 60, 0},
	{HDMI_1440X900P60,		"1440X900P60"		, 1440,  900, 60, 0},
	{HDMI_1680X1050P60, 	"1680X1050P60"		, 1680, 1050, 60, 0},
	{HDMI_1920X1200P60, 	"1920X1200P60"		, 1920, 1200, 60, 0},
	{HDMI_640X480P72,		"640X480P72"		,  640,  480, 72, 0},
	{HDMI_640X480P75,		"640X480P75"		,  640,  480, 75, 0},
	{HDMI_800X600P56,		"800X600P56"		,  800,  600, 56, 0},
	{HDMI_800X600P72,		"800X600P72"		,  800,  600, 72, 0},
	{HDMI_800X600P75,		"800X600P75"		,  800,  600, 75, 0},
	{HDMI_1024X768P70,		"1024X768P70"		, 1024,  768, 70, 0},
	{HDMI_1024X768P75,		"1024X768P75"		, 1024,  768, 75, 0},
	{HDMI_1280X1024P75, 	"1280X1024P75"		, 1280, 1024, 75, 0},
	{HDMI_1400X1050P60, 	"1400X1050P60"		, 1400, 1050, 60, 0},
	{HDMI_1280X800P60,		"1280X800P60"		, 1280,  800, 60, 0},
	{HDMI_1920X1200P75, 	"1920X1200P75"		, 1920, 1200, 75, 0},
	{HDMI_800X600P60, 		"800X600P60"		,  800,  600, 60, 0},
};


#if DISPDEVIOUSED
#if !_FPGA_EMULATION_
static UINT32           g_dispdev_src_freq[HDMI_ID_MAX];
#endif
#endif

HDMI_MODE        g_dispdev_hdmi_mode[HDMI_ID_MAX] = {HDMI_MODE_YCBCR444, HDMI_MODE_YCBCR444, HDMI_MODE_YCBCR444};
HDMI_VIDEOID     g_dispdev_hdmi_vid[HDMI_ID_MAX] = {HDMI_720X480P60, HDMI_720X480P60, HDMI_720X480P60};
HDMI_AUDIOFMT    g_dispdev_aud_fmt[HDMI_ID_MAX]  = {HDMI_AUDIO48KHZ,HDMI_AUDIO48KHZ, HDMI_AUDIO48KHZ};
HDMI_SRCFMT		g_dispdev_src_fmt[HDMI_ID_MAX]  = {HDMI_IDE_0, HDMI_IDE_0,HDMI_IDE_0};
UINT32 		 		g_clock_rate[HDMI_ID_MAX] = {148500000,148500000,148500000};
UINT32 		 	g_Sink_adapt_4kp60_420_mode[HDMI_ID_MAX] = {0,0,0};



static UINT32           g_dispdev_win_hozi_ofs[HDMI_ID_MAX] = {0};
static UINT32           g_dispdev_win_vert_ofs[HDMI_ID_MAX] = {0};
static UINT32           g_dispdev_hdmi_index[HDMI_ID_MAX] = {1};

static BOOL dispdev_init_hdmi_vid_format(HDMI_ID id, HDMI_VIDEOID vid_code)
{
	UINT8                   i, valid = 0;
#if DISPDEVIOUSED
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	DISPDEV_IOCTRL          p_dispdev_control = p_hdmi_io_control[id];
#endif
	disphdmi_debug(("DEVHDMI: initHdmiVidFormat (%d)\r\n", vid_code));

	//
	//Search the mapping Video ID index
	//
	for (i = 0; i < VIDMAX; i++) {
		if (disp_video_timing[i].VID == vid_code) {
			g_dispdev_hdmi_index[id]   = i;
			valid           = 1;
			break;
		}
	}

	if (valid == 0) {
		DBG_ERR("No support Video Format! %d\r\n", vid_code);
		return FALSE;
	}


	//
	//  Handle the TV overscan/underscan capability.
	//
	//if ((!hdmi_obj[id].hdmitx_get_config(HDMI_CONFIG_ID_FORCE_UNDERSCAN)) && ((hdmi_obj[id].hdmitx_get_edid_info()&HDMITX_TV_UNDERSCAN) == 0x0) && (hdmi_obj[id].hdmitx_get_edid_info() != 0x0)) {
	if (0) {
		// Overscan
#if DISPDEVIOUSED
		dev_io_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_x = disp_video_timing[g_dispdev_hdmi_index[id]].WINX_OFS;
		dev_io_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_y = disp_video_timing[g_dispdev_hdmi_index[id]].WINY_OFS;
#endif
		g_dispdev_win_hozi_ofs[id] = disp_video_timing[g_dispdev_hdmi_index[id]].WINX_OFS << 1;
		g_dispdev_win_vert_ofs[id] = disp_video_timing[g_dispdev_hdmi_index[id]].WINY_OFS << 1;
	} else {
		// underscan or EDID Parsing fail
#if DISPDEVIOUSED
		dev_io_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_x = 0;
		dev_io_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_y = 0;
#endif
		g_dispdev_win_hozi_ofs[id] = 0x0;
		g_dispdev_win_vert_ofs[id] = 0x0;
	}
#if DISPDEVIOUSED
	p_dispdev_control(DISPDEV_IOCTRL_SET_CONST_OFS, &dev_io_ctrl);
#endif

	if ((!hdmi_obj[id].hdmitx_get_config(HDMI_CONFIG_ID_FORCE_RGB)) && (hdmi_obj[id].hdmitx_get_edid_info()&HDMITX_TV_YCBCR444)) {
		g_dispdev_hdmi_mode[id] = HDMI_MODE_YCBCR444;
	} else if ((!hdmi_obj[id].hdmitx_get_config(HDMI_CONFIG_ID_FORCE_RGB)) && (hdmi_obj[id].hdmitx_get_edid_info()&HDMITX_TV_YCBCR422)) {
		g_dispdev_hdmi_mode[id] = HDMI_MODE_YCBCR422;
	} else {
		g_dispdev_hdmi_mode[id] = HDMI_MODE_RGB444;
	}
#if DISPDEVIOUSED
	if (hdmi_obj[id].hdmitx_get_config(HDMI_CONFIG_ID_FORCE_YUV444)) {
		g_dispdev_hdmi_mode[id] = HDMI_MODE_YCBCR444;
	} else if (hdmi_obj[id].hdmitx_get_config(HDMI_CONFIG_ID_FORCE_YUV422)) {
		g_dispdev_hdmi_mode[id] = HDMI_MODE_YCBCR422;
#if _FPGA_EMULATION_		
	} else if (hdmi_obj[id].hdmitx_get_config(HDMI_CONFIG_ID_FORCE_YUV420)) {
#else	
	} else if (hdmi_obj[id].hdmitx_get_config(HDMI_CONFIG_ID_FORCE_YUV420) && 
		(vid_code == HDMI_3840X2160P60 
		|| vid_code == HDMI_4096X2160P60
		|| vid_code == HDMI_7680X4320P24
		|| vid_code == HDMI_7680X4320P25
		|| vid_code == HDMI_7680X4320P30)) {
#endif
		g_dispdev_hdmi_mode[id] = HDMI_MODE_YCBCR420;
	}

	if(g_Sink_adapt_4kp60_420_mode[id]){
		g_dispdev_hdmi_mode[id] = HDMI_MODE_YCBCR420;
	}
#endif
	return TRUE;
}


static void dispdev_init_hdmi_display(HDMI_ID id)
{
#if !DISPDEVIOUSED
	hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT0, 0);
	hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT1, 1);
	hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT2, 2);
#else
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	DISPDEV_IOCTRL          p_dispdev_control = p_hdmi_io_control[id];

	disphdmi_debug(("DEVHDMI: initHdmiDisplay\r\n"));

	dev_io_ctrl.SEL.SET_DITHER_EN.b_en = FALSE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_DITHER_EN, &dev_io_ctrl);

	if (g_dispdev_hdmi_mode[id] == HDMI_MODE_YCBCR422) {
		/* 16bits */
		dev_io_ctrl.SEL.SET_HDMI_FMT.hdmi_fmt = HDMI_FMT_HDMI422;
	} else if (g_dispdev_hdmi_mode[id] == HDMI_MODE_YCBCR420){
		/* 16bits */
		dev_io_ctrl.SEL.SET_HDMI_FMT.hdmi_fmt = HDMI_FMT_HDMI420;
	} else {
		/* 24bits */
		dev_io_ctrl.SEL.SET_HDMI_FMT.hdmi_fmt = HDMI_FMT_HDMI444;
	}
	if(id == HDMI_ID_1){
		p_dispdev_control(DISPDEV_IOCTRL_SET_HDMI_FMT, &dev_io_ctrl);
		disphdmi_debug(("DISPDEV_IOCTRL_SET_HDMI_FMT\r\n"));
	} else if(id == HDMI_ID_2){
		p_dispdev_control(DISPDEV_IOCTRL_SET_HDMI2_FMT, &dev_io_ctrl);
		disphdmi_debug(("DISPDEV_IOCTRL_SET_HDMI2_FMT\r\n"));
	} else if(id == HDMI_ID_3){
		p_dispdev_control(DISPDEV_IOCTRL_SET_HDMI3_FMT, &dev_io_ctrl);
		disphdmi_debug(("DISPDEV_IOCTRL_SET_HDMI3_FMT\r\n"));	
	}

#if 0//_FPGA_EMULATION_
	hdmitx_set_pg_1(0, disp_video_timing[g_dispdev_hdmi_index[id]].H_TOTAL);
	hdmitx_set_pg_2(disp_video_timing[g_dispdev_hdmi_index[id]].H_SYNCTIME, disp_video_timing[g_dispdev_hdmi_index[id]].H_STARTTIME);
	hdmitx_set_pg_3((disp_video_timing[g_dispdev_hdmi_index[id]].H_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].H_STARTTIME), (disp_video_timing[g_dispdev_hdmi_index[id]].V_TOTAL));
	hdmitx_set_pg_4((disp_video_timing[g_dispdev_hdmi_index[id]].V_SYNCTIME), disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_STARTTIME);
	hdmitx_set_pg_5((disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_STARTTIME));
	hdmitx_set_pg_pattern(0x0A);
#endif

	hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_VSYNC_INVERT, disp_video_timing[g_dispdev_hdmi_index[id]].VS);
	hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_HSYNC_INVERT, disp_video_timing[g_dispdev_hdmi_index[id]].HS);

	dev_io_ctrl.SEL.SET_VLD_INVERT.b_hvld_inv    = disp_video_timing[g_dispdev_hdmi_index[id]].HVLD;
	dev_io_ctrl.SEL.SET_VLD_INVERT.b_vvld_inv    = disp_video_timing[g_dispdev_hdmi_index[id]].VVLD;
	dev_io_ctrl.SEL.SET_VLD_INVERT.b_field_inv   = FALSE;
	dev_io_ctrl.SEL.SET_VLD_INVERT.b_de_inv      = disp_video_timing[g_dispdev_hdmi_index[id]].DE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_VLD_INVERT, &dev_io_ctrl);

	dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hsync        = disp_video_timing[g_dispdev_hdmi_index[id]].H_SYNCTIME - 1;
	dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_htotal       = disp_video_timing[g_dispdev_hdmi_index[id]].H_TOTAL - 1;
	dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hvld_start    = disp_video_timing[g_dispdev_hdmi_index[id]].H_STARTTIME;
	dev_io_ctrl.SEL.SET_WINDOW_H_TIMING.ui_hvld_end      = disp_video_timing[g_dispdev_hdmi_index[id]].H_ENDTIME - 1;
	p_dispdev_control(DISPDEV_IOCTRL_SET_WINDOW_H_TIMING, &dev_io_ctrl);


	dev_io_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vsync        = disp_video_timing[g_dispdev_hdmi_index[id]].V_SYNCTIME - 1;
	dev_io_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vtotal       = disp_video_timing[g_dispdev_hdmi_index[id]].V_TOTAL - 1;
	dev_io_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vvld_odd_start = disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_STARTTIME;
	dev_io_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vvld_odd_end   = disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_ENDTIME - 1;
	dev_io_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vvld_even_start = disp_video_timing[g_dispdev_hdmi_index[id]].V_EVEN_STARTTIME;
	dev_io_ctrl.SEL.SET_WINDOW_V_TIMING.ui_vvld_even_end  = disp_video_timing[g_dispdev_hdmi_index[id]].V_EVEN_ENDTIME - 1;
	p_dispdev_control(DISPDEV_IOCTRL_SET_WINDOW_V_TIMING, &dev_io_ctrl);

	dev_io_ctrl.SEL.SET_CSB_EN.b_en = FALSE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_CSB_EN, &dev_io_ctrl);

	dev_io_ctrl.SEL.SET_YC_EXCHG.b_cbcr_exchg = FALSE;
	dev_io_ctrl.SEL.SET_YC_EXCHG.b_yc_exchg = FALSE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_YC_EXCHG, &dev_io_ctrl);

	dev_io_ctrl.SEL.SET_SUBPIXEL.b_odd_r     = FALSE;
	dev_io_ctrl.SEL.SET_SUBPIXEL.b_odd_g     = FALSE;
	dev_io_ctrl.SEL.SET_SUBPIXEL.b_odd_b     = FALSE;
	dev_io_ctrl.SEL.SET_SUBPIXEL.b_even_r    = FALSE;
	dev_io_ctrl.SEL.SET_SUBPIXEL.b_even_g    = FALSE;
	dev_io_ctrl.SEL.SET_SUBPIXEL.b_even_b    = FALSE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_SUBPIXEL, &dev_io_ctrl);

	dev_io_ctrl.SEL.SET_CLK1_2.b_clk1_2     = FALSE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_CLK1_2, &dev_io_ctrl);

	#if (defined __UITRON || defined __ECOS)
	ide_set_tv_power_down(TRUE);
	#endif

	if (g_dispdev_hdmi_mode[id] == HDMI_MODE_RGB444) { /* output=RGB for 444 mode */
		/* Output=RGB for 444 mode */
		dev_io_ctrl.SEL.SET_ICST_EN.b_en    = TRUE;
		dev_io_ctrl.SEL.SET_ICST_EN.select = CST_YCBCR2RGB;


		p_dispdev_control(DISPDEV_IOCTRL_SET_ICST_EN, &dev_io_ctrl);
		hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT0, 0);
		hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT1, 1);
		hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT2, 2);
	} else if (g_dispdev_hdmi_mode[id] == HDMI_MODE_YCBCR444) {
		/* output=YCbCr for 444 mode */
		dev_io_ctrl.SEL.SET_ICST_EN.b_en    = FALSE;
		dev_io_ctrl.SEL.SET_ICST_EN.select = CST_RGB2YCBCR;
		p_dispdev_control(DISPDEV_IOCTRL_SET_ICST_EN, &dev_io_ctrl);
		hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT0, 1);
		hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT1, 2);
		hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT2, 0);
	} else if (g_dispdev_hdmi_mode[id] == HDMI_MODE_YCBCR422){
		/* output=YCbCr for 422 mode */
		dev_io_ctrl.SEL.SET_ICST_EN.b_en = FALSE;
		dev_io_ctrl.SEL.SET_ICST_EN.select = CST_RGB2YCBCR;
		p_dispdev_control(DISPDEV_IOCTRL_SET_ICST_EN, &dev_io_ctrl);

		hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT0, 0);
		hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT1, 2);
		hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT2, 1);
	} else {
		/* output=YCbCr for 420 mode */
		dev_io_ctrl.SEL.SET_ICST_EN.b_en = FALSE;
		dev_io_ctrl.SEL.SET_ICST_EN.select = CST_RGB2YCBCR;
		p_dispdev_control(DISPDEV_IOCTRL_SET_ICST_EN, &dev_io_ctrl);

		hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT0, 0);
		hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT1, 2);
		hdmi_obj[id].hdmitx_set_config(HDMI_CONFIG_ID_COMPONENT2, 1);
	}

	switch(g_dispdev_hdmi_mode[id]){
		case HDMI_MODE_RGB444:
			DBG_WRN("output_mode = [RGB888]\r\n");
			break;
		case HDMI_MODE_YCBCR444:
			DBG_WRN("output_mode = [YCbCr444]\r\n");
			break;
		case HDMI_MODE_YCBCR422:
			DBG_WRN("output_mode = [YCbCr422]\r\n");
			break;
		case HDMI_MODE_YCBCR420:
			DBG_WRN("output_mode = [YCbCr420]\r\n");
			break;			
		default:
			DBG_WRN("output_mode not support [%d]!\r\n",g_dispdev_hdmi_mode[id]);
			break;		
	}	

	dev_io_ctrl.SEL.SET_WINDOW_OUT_TYPE.b_interlaced = !(disp_video_timing[g_dispdev_hdmi_index[id]].PROGRESSIVE);
	dev_io_ctrl.SEL.SET_WINDOW_OUT_TYPE.b_field_odd_st = TRUE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_WINDOW_OUT_TYPE, &dev_io_ctrl);

	
	if(g_dispdev_hdmi_mode[id] == HDMI_MODE_RGB444){ // RGB case
/*
		if(((hdmi2p1_ctrl_option[HDMICFG_OPTION_RGB_RANGE] == 0x1) && id == HDMI_ID_1)||
			((hdmi_ctrl_option[HDMICFG_OPTION_RGB_RANGE] == 0x1) && id == HDMI_ID_2)||
			((hdmi2_ctrl_option[HDMICFG_OPTION_RGB_RANGE] == 0x1) && id == HDMI_ID_3)){ // limited range
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_y_low   = 0x10;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_y_up    = 0xEB;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cb_low  = 0x10;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cb_up   = 0xEB;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cr_low  = 0x10;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cr_up   = 0xEB;
			//DBG_WRN("Set HDMI[%d] limited range RGB out\r\n",(int)id+1);			
		} else {*/
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_y_low   = 0x00;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_y_up    = 0xFF;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cb_low  = 0x00;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cb_up   = 0xFF;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cr_low  = 0x00;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cr_up   = 0xFF;
		//}
		
	} else { // YCbCr case
/*
		if(((hdmi2p1_ctrl_option[HDMICFG_OPTION_YUV_RANGE] == 0x0) && id == HDMI_ID_1)||
			((hdmi_ctrl_option[HDMICFG_OPTION_YUV_RANGE] == 0x0) && id == HDMI_ID_2)||
			((hdmi2_ctrl_option[HDMICFG_OPTION_YUV_RANGE] == 0x0) && id == HDMI_ID_3)){ // limited range*/
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_y_low   = 0x10;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_y_up    = 0xEB;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cb_low  = 0x10;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cb_up   = 0xEB;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cr_low  = 0x10;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cr_up   = 0xEB;
			/*
		} else {
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_y_low   = 0x00;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_y_up    = 0xFF;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cb_low  = 0x00;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cb_up   = 0xFF;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cr_low  = 0x00;
			dev_io_ctrl.SEL.SET_OUT_LIMIT.ui_cr_up   = 0xFF;
			DBG_WRN("Set HDMI[%d] full range YCC out\r\n",(int)id+1);	
		}	
		*/
	}

	p_dispdev_control(DISPDEV_IOCTRL_SET_OUT_LIMIT, &dev_io_ctrl);
#endif
}


static void dispdev_disable_ide(HDMI_ID id)
{
/*
	DISPDEV_IOCTRL          p_dispdev_control = p_hdmi_io_control[id];
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;

	p_dispdev_control(DISPDEV_IOCTRL_GET_ENABLE, &dev_io_ctrl);
	if (dev_io_ctrl.SEL.GET_ENABLE.b_en == TRUE) {
		disphdmi_debug(("DEVHDMI: dispdev_disable_ide\r\n"));

		dev_io_ctrl.SEL.SET_ENABLE.b_en = FALSE;
		p_dispdev_control(DISPDEV_IOCTRL_SET_ENABLE,    &dev_io_ctrl);
		p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);

		// Close IDE clock
		dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = FALSE;
		p_dispdev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);
	}
*/
}

static BOOL dispdev_chg_hdmi_clk_freq(HDMI_ID id, BOOL b_native)
{
#if DISPDEVIOUSED
#if !_FPGA_EMULATION_
#if 0//def __KERNEL__
		UINT32					ui_target_freq = 27000000;
		DISPDEV_IOCTRL			p_dispdev_control = p_hdmi_io_control[id];
		DISPDEV_IOCTRL_PARAM	dev_io_ctrl;
		UINT32					cur_freq;
		struct clk *pll_clk = NULL;
		int hdmi_timing_factor = 1000;
	
		disphdmi_debug(("DEVHDMI: chgHdmiClkFreq (%d)\r\n", b_native));
	
		p_dispdev_control(DISPDEV_IOCTRL_GET_SRCCLK, &dev_io_ctrl);
	
		switch (dev_io_ctrl.SEL.GET_SRCCLK.src_clk) {
			
		case DISPCTRL_SRCCLK_PLL9:{
				pll_clk = clk_get(NULL, "pll9");
				disphdmi_debug(("DEVHDMI: Original src_clk = pll[9]\r\n"));
			}break;
		
		case DISPCTRL_SRCCLK_PLL8:{
				pll_clk = clk_get(NULL, "pll8");
				disphdmi_debug(("DEVHDMI: Original src_clk = pll[8]\r\n"));
			}break;
		case DISPCTRL_SRCCLK_PLL10:{
				pll_clk = clk_get(NULL, "pll10");
				disphdmi_debug(("DEVHDMI: Original src_clk = pll[10]\r\n"));
			}break;		
		
		default: {
				pll_clk = clk_get(NULL, "pll8");
				disphdmi_debug(("DEVHDMI: Original default src_clk = pll[8]\r\n"));
			}
	
		}
	
	
		if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
			hdmi_timing_factor = 1001;
		}
	
		if (pll_clk != NULL) {
			if (IS_ERR(pll_clk)) {
				DBG_ERR("get pll failed\r\n");
				return TRUE;
			}
	
			cur_freq = clk_get_rate(pll_clk);
			disphdmi_debug(("DEVHDMI: Original src_clk(%d)Hertz\r\n", cur_freq));
	
			if (b_native) {
				g_dispdev_src_freq[id] = cur_freq;
			}
	
			switch (g_dispdev_hdmi_vid[id]) {
	
			/*
				Change PLL2 to 216Mhz
			*/
			// Target 54MHz
			case HDMI_1440X240P60:			/* VID12 */
			case HDMI_1440X240P60_16X9: 	/* VID13 */			
			case HDMI_1440X480P60:			/* VID14 */
			case HDMI_1440X480P60_16X9: 	/* VID15 */
			case HDMI_1440X576P50:			/* VID29 */
			case HDMI_1440X576P50_16X9: 	/* VID30 */
			case HDMI_720X576P100:			/* VID42 */
			case HDMI_720X576P100_16X9: 	/* VID43 */
			case HDMI_720X480P120:			/* VID48 */
			case HDMI_720X480P120_16X9: 	/* VID49 */
			case HDMI_720X576I200:			/* VID54 */
			case HDMI_720X576I200_16X9: 	/* VID55 */
			case HDMI_720X480I240:			/* VID58 */
			case HDMI_720X480I240_16X9: 	/* VID59 */
			case HDMI_1440X480I60:			/* VID10 */
			case HDMI_1440X480I60_16X9: 	/* VID11 */
			case HDMI_1440X576I50:			/* VID25 */
			case HDMI_1440X576I50_16X9: 	/* VID26 */ 
			case HDMI_720X576I100:			/* VID44 */
			case HDMI_720X576I100_16X9: 	/* VID45 */ 
			case HDMI_720X480I120:			/* VID50 */
			case HDMI_720X480I120_16X9: 	/* VID51 */ 
			case HDMI_1440X288P50:			/* VID27 */
			case HDMI_1440X288P50_16X9: 	/* VID28 */				
			// Target 108MHz
			case HDMI_1280X1024P60:
			case HDMI_720X576P200:			/* VID52 */
			case HDMI_720X576P200_16X9: 	/* VID53 */
			case HDMI_720X480P240:			/* VID56 */
			case HDMI_720X480P240_16X9: {	/* VID57 */
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(216000000,hdmi_timing_factor);
				}
				break;
	
			/*
				Change PLL2 to 252Mhz
			*/
			// Target 25.2MHz
			case HDMI_640X480P60: { 		/* VID1 */
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(252000000,hdmi_timing_factor);
				}
				break;
	
			/*
				Change PLL2 to 297Mhz
			*/
			// Target 27MHz
			case HDMI_720X480P60:			/* VID2 */
			case HDMI_720X480P60_16X9:		/* VID3 */
			case HDMI_720X480I60:			/* VID6 */
			case HDMI_720X480I60_16X9:		/* VID7 */
			case HDMI_720X240P60:			/* VID8 */
			case HDMI_720X240P60_16X9:		/* VID9 */			
			case HDMI_720X576P50:			/* VID17 */
			case HDMI_720X576P50_16X9:		/* VID18 */
			case HDMI_720X576I50:			/* VID21 */
			case HDMI_720X576I50_16X9:		/* VID22 */
			case HDMI_720X288P50:			/* VID23 */
			case HDMI_720X288P50_16X9:		/* VID24 */ 		
			// Target 37.125MHz
			case HDMI_1280X720P30:			/* VID132 */
			case HDMI_1280X720P25:
			// Target 74.25MHz
			case HDMI_1280X720P60:			/* VID4 */
			case HDMI_1920X1080I60: 		/* VID5 */
			case HDMI_1280X720P50:			/* VID19 */
			case HDMI_1920X1080I50: 		/* VID20 */
			case HDMI_1920X1080P24: 		/* VID32 */
			case HDMI_1920X1080P25: 		/* VID33 */
			case HDMI_1920X1080P30: 		/* VID34 */
			// Target 148.5MHz
			case HDMI_1920X1080P60: 		/* VID16 */
			case HDMI_1920X1080P50: 		/* VID31 */
			case HDMI_1920X1080I100:		/* VID40 */
			case HDMI_1280X720P100: 		/* VID41 */
			case HDMI_1920X1080I120:		/* VID46 */
			case HDMI_1280X720P120: 		/* VID47 */
			// Target 297MHz
			case HDMI_3840X2160P24: 		/* VID93 */
			case HDMI_3840X2160P25: 		/* VID94 */
			case HDMI_3840X2160P30: 		/* VID95 */		
			case HDMI_4096X2160P24: 		/* VID98 */
			case HDMI14B_3840X2160P24:
			case HDMI14B_3840X2160P25:
			case HDMI14B_3840X2160P30:
			case HDMI14B_4096X2160P24:
			case HDMI_4096X2160P25: 		/* VID99 */
			case HDMI_4096X2160P30:{ 		/* VID100*/
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(297000000,hdmi_timing_factor);
				}
				break;

			case HDMI_3840X2160P50: 		/* VID96*/			
			case HDMI_3840X2160P60: 		/* VID97*/	
			case HDMI_4096X2160P50: 		/* VID101 */
			case HDMI_4096X2160P60:			/* VID102 */
			case HDMI_7680X4320P24:			/* VID194 */
			case HDMI_7680X4320P25:			/* VID195 */
			case HDMI_7680X4320P30:{		/* VID196 */
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(594000000,hdmi_timing_factor);
				}
				break;

			
	
			case HDMI_1024X768P60: {
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(260000000,hdmi_timing_factor);
				}
				break;
	
			case HDMI_1600X1200P60: {
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(324000000,hdmi_timing_factor);
				}
				break;
			case HDMI_2560X1440P30: {
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(119000000,hdmi_timing_factor);
				}
				break;			
			case HDMI_2560X1440P60: {
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(241500000,hdmi_timing_factor);
				}
				break;
			case HDMI_1440X900P60: {
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(213000000,hdmi_timing_factor);
				}
				break;
			case HDMI_1680X1050P60: {
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(585000000,hdmi_timing_factor);
				}
				break;
			case HDMI_1920X1200P60: {
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(386500000,hdmi_timing_factor);
				}
				break;
			case HDMI_1920X1200P75: {
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(490500000,hdmi_timing_factor);
				}
				break;				
			case HDMI_800X600P72:
			case HDMI_1024X768P70:{
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(150000000,hdmi_timing_factor);
				}
				break;
			case HDMI_800X600P75:{
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(198000000,hdmi_timing_factor);
				}
				break;
			case HDMI_1024X768P75:{
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(315000000,hdmi_timing_factor);
				}
				break;	
			case HDMI_1280X1024P75:{
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(270000000,hdmi_timing_factor);
				}
				break;
			case HDMI_1400X1050P60:{
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(487000000,hdmi_timing_factor);
				}
				break;
			case HDMI_1280X800P60:{
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(334000000,hdmi_timing_factor);
				}
				break;
			case HDMI_800X600P60:{
					g_dispdev_src_freq[id] = HDMI_PLL_DIV(200000000,hdmi_timing_factor);
			}
			break;					
	
			case HDMI_1920X1080I50_VT1250:	/* VID39 */
			default:
				clk_put(pll_clk);
				DBG_ERR("No support HDMI mode(%d)\r\n", g_dispdev_hdmi_vid[id]);
				return TRUE;
			}
			clk_set_rate(pll_clk, g_dispdev_src_freq[id]); 
			clk_put(pll_clk);
	
			if(g_dispdev_src_freq[id] != cur_freq){
				DBG_WRN("Change pll source freq from [%d] to [%d]\r\n",cur_freq ,g_dispdev_src_freq[id]);
			}
		}
	
	
	
		//
		//	Configure IDE Clock frequency according to HDMI Video ID
		//
		switch (g_dispdev_hdmi_vid[id]) {
	
		/* Target frequency 25.2Mhz */
		case HDMI_640X480P60: { 		/* VID1 */
				ui_target_freq = HDMI_PLL_DIV(25200000,hdmi_timing_factor);
			}
			break;
	
		/* Target frequency 27Mhz */
		case HDMI_720X480P60:			/* VID2 */
		case HDMI_720X480P60_16X9:		/* VID3 */
		case HDMI_720X480I60:			/* VID6 */
		case HDMI_720X480I60_16X9:		/* VID7 */	
		case HDMI_720X240P60:			/* VID8 */
		case HDMI_720X240P60_16X9:		/* VID9 */		
		case HDMI_720X576P50:			/* VID17 */
		case HDMI_720X576P50_16X9:		/* VID18 */
		case HDMI_720X576I50:			/* VID21 */
		case HDMI_720X576I50_16X9:		/* VID22 */
		case HDMI_720X288P50:			/* VID23 */
		case HDMI_720X288P50_16X9:{ 	 /* VID24 */		
				ui_target_freq = HDMI_PLL_DIV(27000000,hdmi_timing_factor);
			}
			break;
	
		/* Target frequency 54Mhz */
		case HDMI_1440X480I60:			/* VID10 */
		case HDMI_1440X480I60_16X9: 	/* VID11 */
		case HDMI_1440X240P60:			/* VID12 */
		case HDMI_1440X240P60_16X9: 	/* VID13 */ 
		case HDMI_1440X480P60:			/* VID14 */
		case HDMI_1440X480P60_16X9: 	/* VID15 */
		case HDMI_1440X576I50:			/* VID25 */
		case HDMI_1440X576I50_16X9: 	/* VID26 */
		case HDMI_1440X288P50:			/* VID27 */
		case HDMI_1440X288P50_16X9: 	/* VID28 */ 	
		case HDMI_1440X576P50:			/* VID29 */
		case HDMI_1440X576P50_16X9: 	/* VID30 */
		case HDMI_720X576P100:			/* VID42 */
		case HDMI_720X576P100_16X9: 	/* VID43 */
		case HDMI_720X576I100:			/* VID44 */
		case HDMI_720X576I100_16X9: 	/* VID45 */ 	
		case HDMI_720X480P120:			/* VID48 */
		case HDMI_720X480P120_16X9: 	/* VID49 */
		case HDMI_720X480I120:			/* VID50 */
		case HDMI_720X480I120_16X9: {	/* VID51 */ 	
				ui_target_freq = HDMI_PLL_DIV(54000000,hdmi_timing_factor);
			}
			break;
	
		/* Target frequency 37.125Mhz */
		case HDMI_1280X720P25:
		case HDMI_1280X720P30: {	   /* VID132 */
				ui_target_freq = HDMI_PLL_DIV(37125000,hdmi_timing_factor);
			}
			break;
	
		/* Target frequency 74.25Mhz */
		case HDMI_1280X720P60:			/* VID4 */
		case HDMI_1920X1080I60: 		/* VID5 */
		case HDMI_1280X720P50:			/* VID19 */
		case HDMI_1920X1080I50: 		/* VID20 */
		case HDMI_1920X1080P24: 		/* VID32 */
		case HDMI_1920X1080P25: 		/* VID33 */
		case HDMI_1920X1080P30: {		/* VID34 */
				ui_target_freq = HDMI_PLL_DIV(74250000,hdmi_timing_factor);
			}
			break;
	
		/* Target frequency 108Mhz */
		case HDMI_1280X1024P60:
		case HDMI_720X576P200:			/* VID52 */
		case HDMI_720X576P200_16X9: 	/* VID53 */
		case HDMI_720X576I200:			/* VID54 */
		case HDMI_720X576I200_16X9: 	/* VID55 */ 	
		case HDMI_720X480P240:			/* VID56 */
		case HDMI_720X480P240_16X9: 	/* VID57 */
		case HDMI_720X480I240:			/* VID58 */
		case HDMI_720X480I240_16X9: {	/* VID59 */ 	
				ui_target_freq = HDMI_PLL_DIV(108000000,hdmi_timing_factor);
			}
			break;
	
		/* Target frequency 148.5Mhz */
		case HDMI_1920X1080P60: 		/* VID16 */
		case HDMI_1920X1080P50: 		/* VID31 */
		case HDMI_1920X1080I100:		/* VID40 */
		case HDMI_1280X720P100: 		/* VID41 */
		case HDMI_1920X1080I120:		/* VID46 */
		case HDMI_1280X720P120: {		/* VID47 */
				ui_target_freq = HDMI_PLL_DIV(148500000,hdmi_timing_factor);
			}
			break;
	
		case HDMI_3840X2160P24: 		/* VID93 */
		case HDMI_3840X2160P25: 		/* VID94 */
		case HDMI_3840X2160P30: 		/* VID95 */
		case HDMI_4096X2160P24: 		/* VID98 */
		case HDMI14B_3840X2160P24:
		case HDMI14B_3840X2160P25:
		case HDMI14B_3840X2160P30:
		case HDMI14B_4096X2160P24:
		case HDMI_4096X2160P25: 		/* VID99 */
		case HDMI_4096X2160P30: {		/* VID100*/
				ui_target_freq = HDMI_PLL_DIV(297000000,hdmi_timing_factor);
			}
			break;
	
		case HDMI_1024X768P60: {
				ui_target_freq = HDMI_PLL_DIV(65000000,hdmi_timing_factor);
			}
			break;
		case HDMI_1600X1200P60: {
				ui_target_freq = HDMI_PLL_DIV(162000000,hdmi_timing_factor);
			}
			break;
		case HDMI_2560X1440P30: {
				ui_target_freq = HDMI_PLL_DIV(119000000,hdmi_timing_factor);
			}
			break;		
		case HDMI_2560X1440P60: {
				ui_target_freq = HDMI_PLL_DIV(241500000,hdmi_timing_factor);
			}
			break;
		case HDMI_1440X900P60: {
				ui_target_freq = HDMI_PLL_DIV(106500000,hdmi_timing_factor);
			}
			break;
		case HDMI_1680X1050P60: {
				ui_target_freq = HDMI_PLL_DIV(146250000,hdmi_timing_factor);
			}
			break;
		case HDMI_1920X1200P60: {
				ui_target_freq = HDMI_PLL_DIV(193250000,hdmi_timing_factor);
			}
			break;
		case HDMI_1920X1200P75: {
				ui_target_freq = HDMI_PLL_DIV(245250000,hdmi_timing_factor);
			}
			break;			

		case HDMI_640X480P72:
		case HDMI_640X480P75:{
				ui_target_freq = HDMI_PLL_DIV(31500000,hdmi_timing_factor);
			}
			break;	
		case HDMI_800X600P56:{
				ui_target_freq = HDMI_PLL_DIV(36000000,hdmi_timing_factor);
			}
			break;
		case HDMI_800X600P60:{
				ui_target_freq = HDMI_PLL_DIV(40000000,hdmi_timing_factor);
			}
		break;			
		case HDMI_800X600P72:{
				ui_target_freq = HDMI_PLL_DIV(50000000,hdmi_timing_factor);
			}
			break;
		case HDMI_800X600P75:{
				ui_target_freq = HDMI_PLL_DIV(49500000,hdmi_timing_factor);
			}
			break;		
		case HDMI_1024X768P70:{
				ui_target_freq = HDMI_PLL_DIV(75000000,hdmi_timing_factor);
			}
			break;
		case HDMI_1024X768P75:{
				ui_target_freq = HDMI_PLL_DIV(78750000,hdmi_timing_factor);
			}
			break;
		case HDMI_1280X1024P75:{
				ui_target_freq = HDMI_PLL_DIV(135000000,hdmi_timing_factor);
			}
			break;	
		case HDMI_1400X1050P60:{
				ui_target_freq = HDMI_PLL_DIV(121750000,hdmi_timing_factor);
			}
			break;
		case HDMI_1280X800P60:{
				ui_target_freq = HDMI_PLL_DIV(83500000,hdmi_timing_factor);
			}
			break;
		
		case HDMI_3840X2160P50: 		/* VID96*/
		case HDMI_3840X2160P60: 		/* VID97*/
		case HDMI_4096X2160P50: 		/* VID101*/
		case HDMI_4096X2160P60:			/* VID102*/
		case HDMI_7680X4320P24:			/* VID194 */
		case HDMI_7680X4320P25:			/* VID195 */
		case HDMI_7680X4320P30:{		/* VID196 */			
				ui_target_freq = HDMI_PLL_DIV(594000000,hdmi_timing_factor);
			}
			break;		
	
		default: {
				return TRUE;
			}
	
		}
	
		g_clock_rate[id] = ui_target_freq;
	
		//
		//	Configure IDE Clock to 2x ui_target_freq for Interlace mode Vid
		//
		switch (g_dispdev_hdmi_vid[id]) {
		case HDMI_1920X1080I60: 		/* VID5 */
		case HDMI_720X480I60:			/* VID6 */
		case HDMI_720X480I60_16X9:		/* VID7 */	
		case HDMI_1440X480I60:			/* VID10 */
		case HDMI_1440X480I60_16X9: 	/* VID11 */ 
		case HDMI_1920X1080I50: 		/* VID20 */
		case HDMI_720X576I50:			/* VID21 */
		case HDMI_720X576I50_16X9:		/* VID22 */
		case HDMI_1440X576I50:			/* VID25 */
		case HDMI_1440X576I50_16X9: 	/* VID26 */
		case HDMI_1920X1080I100:		/* VID40 */
		case HDMI_720X576I100:			/* VID44 */
		case HDMI_720X576I100_16X9: 	/* VID45 */
		case HDMI_1920X1080I120:		/* VID46 */
		case HDMI_720X480I120:			/* VID50 */
		case HDMI_720X480I120_16X9: 	/* VID51 */
		case HDMI_720X576I200:			/* VID54 */
		case HDMI_720X576I200_16X9: 	/* VID55 */
		case HDMI_720X480I240:			/* VID58 */
		case HDMI_720X480I240_16X9: {	/* VID59 */
	
				// set clock rate
				ui_target_freq	 *=1;
				disphdmi_debug(("Set interlace mode HDMI Target Frequency = %d Hertz\r\n", ui_target_freq));
	
			}
			break;
		default: {
				ui_target_freq	 *=1;
			}
		
		}
		
		if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){	
			ui_target_freq += 500000; // +0.5Mhz to make sure have currect divider
			dev_io_ctrl.SEL.SET_CLK_FREQ.ui_freq   = ui_target_freq;
		} else {
			dev_io_ctrl.SEL.SET_CLK_FREQ.ui_freq   = ui_target_freq;
		}
		
		dev_io_ctrl.SEL.SET_CLK_FREQ.b_ycc8bit = FALSE;
		p_dispdev_control(DISPDEV_IOCTRL_SET_CLK_FREQ, &dev_io_ctrl);
	
		disphdmi_debug(("HDMI Target Frequency = %d Hertz\r\n", ui_target_freq));
#else // RTOS pll setting
	
		UINT32					ui_target_freq = 27000000;
		DISPDEV_IOCTRL			p_dispdev_control = p_hdmi_io_control[id];
		DISPDEV_IOCTRL_PARAM	dev_io_ctrl;
#if !_FPGA_EMULATION_
		UINT32					cur_freq;
		PLL_ID					src_pll = PLL_ID_8;
		int hdmi_timing_factor = 1000;

		pll_set_driver_pll_hdmi(PLL_ID_11, HDMI_PLL650);// LCD0 MCLK PLL11 to 650Mhz
	
		disphdmi_debug(("DEVHDMI: chgHdmiClkFreq (%d)\r\n", b_native));
	
		p_dispdev_control(DISPDEV_IOCTRL_GET_SRCCLK, &dev_io_ctrl);
	
		switch (dev_io_ctrl.SEL.GET_SRCCLK.src_clk) {
		case DISPCTRL_SRCCLK_PLL8: {
				src_pll =									 PLL_ID_8;
			}
			break;			
		case DISPCTRL_SRCCLK_PLL9: {
				src_pll =									 PLL_ID_9;
			}
			break;
	
		case DISPCTRL_SRCCLK_PLL10: {
				src_pll =									 PLL_ID_10;
			}
			break;
	
	
		default: {
				return TRUE;
			}
	
		}
	
		if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
			hdmi_timing_factor = 1001;
		}
	
		cur_freq			  = pll_get_pll_freq_hdmi(src_pll) / 1000000;
		disphdmi_debug(("DEVHDMI: Original pll[%d] src_clk(%d)Mhz\r\n",src_pll, cur_freq));
	
		if (b_native) {
			g_dispdev_src_freq[id] = cur_freq;
		}
	
		// Calculate PLL Ratio
		cur_freq = cur_freq * 131072 / 12;
	
		switch (g_dispdev_hdmi_vid[id]) {
	
		/*
			Change PLL9 to 216Mhz
		*/
		// Target 54MHz
		case HDMI_1440X240P60:			/* VID12 */
		case HDMI_1440X240P60_16X9: 	/* VID13 */		
		case HDMI_1440X480P60:			/* VID14 */
		case HDMI_1440X480P60_16X9: 	/* VID15 */
		case HDMI_1440X288P50:			/* VID27 */
		case HDMI_1440X288P50_16X9: 	/* VID28 */			
		case HDMI_1440X576P50:			/* VID29 */
		case HDMI_1440X576P50_16X9: 	/* VID30 */
		case HDMI_720X576P100:			/* VID42 */
		case HDMI_720X576P100_16X9: 	/* VID43 */
		case HDMI_720X480P120:			/* VID48 */
		case HDMI_720X480P120_16X9: 	/* VID49 */
		case HDMI_720X576I200:			/* VID54 */
		case HDMI_720X576I200_16X9: 	/* VID55 */
		case HDMI_720X480I240:			/* VID58 */
		case HDMI_720X480I240_16X9: 	/* VID59 */
		// Target 108MHz
		case HDMI_1280X1024P60:
		case HDMI_720X576P200:			/* VID52 */
		case HDMI_720X576P200_16X9: 	/* VID53 */
		case HDMI_720X480P240:			/* VID56 */
		case HDMI_720X480P240_16X9: 	/* VID57 */
			if ((cur_freq != HDMI_PLL216) && (src_pll != PLL_ID_1)) {
				dispdev_disable_ide(id);
				if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
					pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL215P78);
				} else {
					pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL216);
				}	
				disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll + 1), pll_get_pll_freq_hdmi(src_pll) / 1000000));
			}
			break;
		/*
			Change PLL2 to 252Mhz
		*/
		// Target 25.2MHz
		case HDMI_640X480P60:		   /* VID1 */
			if ((cur_freq != HDMI_PLL252) && (src_pll != PLL_ID_1)) {
				dispdev_disable_ide(id);
				if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
					pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL251P17);
				} else {
					pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL252);
				}
				disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll + 1), pll_get_pll_freq_hdmi(src_pll) / 1000000));
			}
			break;
	
		/*
			Change PLL9 to 594Mhz
		*/
		// Target 27MHz
		case HDMI_720X480P60:			/* VID2 */
		case HDMI_720X480P60_16X9:		/* VID3 */
		case HDMI_720X480I60:			/* VID6 */
		case HDMI_720X480I60_16X9:		/* VID7 */	
		case HDMI_720X240P60:			/* VID8 */
		case HDMI_720X240P60_16X9:		/* VID9 */		
		case HDMI_1440X480I60:			/* VID10 */
		case HDMI_1440X480I60_16X9: 	/* VID11 */
		case HDMI_720X576P50:			/* VID17 */
		case HDMI_720X576P50_16X9:		/* VID18 */
		case HDMI_720X576I50:			/* VID21 */
		case HDMI_720X576I50_16X9:		/* VID22 */
		case HDMI_720X288P50:			/* VID23 */
		case HDMI_720X288P50_16X9:		/* VID24 */ 	
		case HDMI_1440X576I50:			/* VID25 */
		case HDMI_1440X576I50_16X9: 	/* VID26 */
		case HDMI_720X576I100:			/* VID44 */
		case HDMI_720X576I100_16X9: 	/* VID45 */
		case HDMI_720X480I120:			/* VID50 */
		case HDMI_720X480I120_16X9: 	/* VID51 */
		// Target 37.125MHz
		case HDMI_1280X720P30:			/* VID132 */
		case HDMI_1280X720P25:
		// Target 74.25MHz
		case HDMI_1280X720P60:			/* VID4 */
		case HDMI_1920X1080I60: 		/* VID5 */
		case HDMI_1280X720P50:			/* VID19 */
		case HDMI_1920X1080I50: 		/* VID20 */
		case HDMI_1920X1080P24: 		/* VID32 */
		case HDMI_1920X1080P25: 		/* VID33 */
		case HDMI_1920X1080P30: 		/* VID34 */
		// Target 148.5MHz
		case HDMI_1920X1080P60: 		/* VID16 */
		case HDMI_1920X1080P50: 		/* VID31 */
		case HDMI_1920X1080I100:		/* VID40 */
		case HDMI_1280X720P100: 		/* VID41 */
		case HDMI_1920X1080I120:		/* VID46 */
		case HDMI_1280X720P120: 		/* VID47 */
		// Target 594MHz
		case HDMI_3840X2160P24: 		/* VID93 */
		case HDMI_3840X2160P25: 		/* VID94 */
		case HDMI_3840X2160P30: 		/* VID95 */	
		case HDMI_4096X2160P24: 		/* VID98 */
		case HDMI14B_3840X2160P24:
		case HDMI14B_3840X2160P25:
		case HDMI14B_3840X2160P30:
		case HDMI14B_4096X2160P24:
		case HDMI_4096X2160P25: 		/* VID99 */
		case HDMI_4096X2160P30:{ 		/* VID100*/
				if ((cur_freq != HDMI_PLL297) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
						pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL296P70);
					} else {
						pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL297);
					}
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;

		case HDMI_3840X2160P50: 		/* VID96*/			
		case HDMI_3840X2160P60: 		/* VID97*/	
		case HDMI_4096X2160P50: 		/* VID101 */
		case HDMI_4096X2160P60:			/* VID102 */
		case HDMI_7680X4320P24:			/* VID194 */
		case HDMI_7680X4320P25:			/* VID195 */
		case HDMI_7680X4320P30:{		/* VID196 */			
				if ((cur_freq != HDMI_PLL594) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
						pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL593P4);
					} else {
						pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL594);
					}
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;			
	
		case HDMI_1024X768P60: {
				if ((cur_freq != HDMI_PLL260) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
						pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL259P74);
					} else {
						pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL260);
					}
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;
	
		case HDMI_1600X1200P60: {
				if ((cur_freq != HDMI_PLL324) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
						pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL323P67);
					} else {
						pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL324);
					}
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;
		case HDMI_2560X1440P30: {
				if ((cur_freq != HDMI_PLL119) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
						pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL118P88);
					} else {
						pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL119);
					}
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;		
		case HDMI_2560X1440P60: {
				if ((cur_freq != HDMI_PLL241) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
						pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL241P25);
					} else {
						pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL241);
					}
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;
		case HDMI_1440X900P60: {
				if ((cur_freq != HDMI_PLL213) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL213);
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;
		case HDMI_1680X1050P60: {
				if ((cur_freq != HDMI_PLL585) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL585);
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;
		case HDMI_1920X1200P60: {
				if ((cur_freq != HDMI_PLL386P5) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL386P5);
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;
		case HDMI_1920X1200P75: {
				if ((cur_freq != HDMI_PLL490P5) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					pll_set_driver_pll_hdmi(src_pll,	  HDMI_PLL490P5);
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;			

		case HDMI_800X600P72: 
		case HDMI_1024X768P70:{
				if ((cur_freq != HDMI_PLL150) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL149P8);
					} else {
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL150);
					}
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;

		case HDMI_800X600P60:{
				if ((cur_freq != HDMI_PLL200) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL199P8);
					} else {
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL200);
					}
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;			

		case HDMI_800X600P75:{
				if ((cur_freq != HDMI_PLL198) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL197P8);
					} else {
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL198);
					}
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;

		case HDMI_1024X768P75:{
				if ((cur_freq != HDMI_PLL315) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL314P6);
					} else {
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL315);
					}
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;

		case HDMI_1280X1024P75:{
				if ((cur_freq != HDMI_PLL270) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL269P7);
					} else {
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL270);
					}
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;

		case HDMI_1400X1050P60:{
				if ((cur_freq != HDMI_PLL487) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL486P5);
					} else {
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL487);
					}
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;

		case HDMI_1280X800P60:{
				if ((cur_freq != HDMI_PLL334) && (src_pll != PLL_ID_1)) {
					dispdev_disable_ide(id);
					if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL333P6);
					} else {
						pll_set_driver_pll_hdmi(src_pll,      HDMI_PLL334);
					}
					disphdmi_debug(("DEVHDMI: Chg PLL%d to(%d)Mhz\r\n", (src_pll), pll_get_pll_freq_hdmi(src_pll) / 1000000));
				}
			}
			break;	
	
		case HDMI_1920X1080I50_VT1250:	/* VID39 */
		default:
			DBG_ERR("No support HDMI mode(%d)\r\n", g_dispdev_hdmi_vid[id]);
			return TRUE;
		}
	
	
#endif
	
		//
		//	Configure IDE Clock frequency according to HDMI Video ID
		//
		switch (g_dispdev_hdmi_vid[id]) {
	
		/* Target frequency 25.2Mhz */
		case HDMI_640X480P60: { 		/* VID1 */
#if _FPGA_EMULATION_
				ui_target_freq = HDMI_PLL_DIV(27000000,hdmi_timing_factor);
#else
				ui_target_freq = HDMI_PLL_DIV(25200000,hdmi_timing_factor);
#endif
			}
			break;
	
		/* Target frequency 27Mhz */
		case HDMI_720X480P60:			/* VID2 */
		case HDMI_720X480P60_16X9:		/* VID3 */
		case HDMI_720X480I60:			/* VID6 */
		case HDMI_720X480I60_16X9:		/* VID7 */
		case HDMI_720X240P60:			/* VID8 */
		case HDMI_720X240P60_16X9:		/* VID9 */		
		case HDMI_720X576P50:			/* VID17 */
		case HDMI_720X576P50_16X9:		/* VID18 */
		case HDMI_720X576I50:			/* VID21 */
		case HDMI_720X576I50_16X9:		/* VID22 */
		case HDMI_720X288P50:			/* VID23 */
		case HDMI_720X288P50_16X9:{ 	 /* VID24 */		
				ui_target_freq = HDMI_PLL_DIV(27000000,hdmi_timing_factor);
			}
			break;
	
		/* Target frequency 54Mhz */
		case HDMI_1440X480I60:			/* VID10 */
		case HDMI_1440X480I60_16X9: 	/* VID11 */
		case HDMI_1440X240P60:			/* VID12 */
		case HDMI_1440X240P60_16X9: 	/* VID13 */ 
		case HDMI_1440X480P60:			/* VID14 */
		case HDMI_1440X480P60_16X9: 	/* VID15 */
		case HDMI_1440X576I50:			/* VID25 */
		case HDMI_1440X576I50_16X9: 	/* VID26 */
		case HDMI_1440X288P50:			/* VID27 */
		case HDMI_1440X288P50_16X9: 	/* VID28 */ 	
		case HDMI_1440X576P50:			/* VID29 */
		case HDMI_1440X576P50_16X9: 	/* VID30 */
		case HDMI_720X576P100:			/* VID42 */
		case HDMI_720X576P100_16X9: 	/* VID43 */
		case HDMI_720X576I100:			/* VID44 */
		case HDMI_720X576I100_16X9: 	/* VID45 */ 	
		case HDMI_720X480P120:			/* VID48 */
		case HDMI_720X480P120_16X9: 	/* VID49 */
		case HDMI_720X480I120:			/* VID50 */
		case HDMI_720X480I120_16X9: {	/* VID51 */ 	
				ui_target_freq = HDMI_PLL_DIV(54000000,hdmi_timing_factor);
			}
			break;
	
		/* Target frequency 37.125Mhz */
		case HDMI_1280X720P25:
		case HDMI_1280X720P30: {	   /* VID132 */
				ui_target_freq = HDMI_PLL_DIV(37125000,hdmi_timing_factor);
			}
			break;
	
		/* Target frequency 74.25Mhz */
		case HDMI_1280X720P60:			/* VID4 */
		case HDMI_1920X1080I60: 		/* VID5 */
		case HDMI_1280X720P50:			/* VID19 */
		case HDMI_1920X1080I50: 		/* VID20 */
		case HDMI_1920X1080P24: 		/* VID32 */
		case HDMI_1920X1080P25: 		/* VID33 */
		case HDMI_1920X1080P30: {		/* VID34 */
				ui_target_freq = HDMI_PLL_DIV(74250000,hdmi_timing_factor);
			}
			break;
	
		/* Target frequency 108Mhz */
		case HDMI_1280X1024P60:
		case HDMI_720X576P200:			/* VID52 */
		case HDMI_720X576P200_16X9: 	/* VID53 */
		case HDMI_720X576I200:			/* VID54 */
		case HDMI_720X576I200_16X9: 	/* VID55 */ 	
		case HDMI_720X480P240:			/* VID56 */
		case HDMI_720X480P240_16X9: 	/* VID57 */
		case HDMI_720X480I240:			/* VID58 */
		case HDMI_720X480I240_16X9: {	/* VID59 */ 	
				ui_target_freq = HDMI_PLL_DIV(108000000,hdmi_timing_factor);
			}
			break;
	
		/* Target frequency 148.5Mhz */
		case HDMI_1920X1080P60: 		/* VID16 */
		case HDMI_1920X1080P50: 		/* VID31 */
		case HDMI_1920X1080I100:		/* VID40 */
		case HDMI_1280X720P100: 		/* VID41 */
		case HDMI_1920X1080I120:		/* VID46 */
		case HDMI_1280X720P120: {		/* VID47 */
				ui_target_freq = HDMI_PLL_DIV(148500000,hdmi_timing_factor);
			}
			break;
	
		case HDMI_3840X2160P24: 		/* VID93 */
		case HDMI_3840X2160P25: 		/* VID94 */
		case HDMI_3840X2160P30: 		/* VID95 */
		case HDMI_4096X2160P24: 		/* VID98 */
		case HDMI14B_3840X2160P24:
		case HDMI14B_3840X2160P25:
		case HDMI14B_3840X2160P30:
		case HDMI14B_4096X2160P24:
		case HDMI_4096X2160P25: 		/* VID99 */
		case HDMI_4096X2160P30: {		/* VID100*/
				ui_target_freq = HDMI_PLL_DIV(297000000,hdmi_timing_factor);
			}
			break;
	
		case HDMI_1024X768P60: {
				ui_target_freq = HDMI_PLL_DIV(65000000,hdmi_timing_factor);
			}
			break;
		case HDMI_1600X1200P60: {
				ui_target_freq = HDMI_PLL_DIV(162000000,hdmi_timing_factor);
			}
			break;
		case HDMI_2560X1440P30: {
				ui_target_freq = HDMI_PLL_DIV(119000000,hdmi_timing_factor);
			}		
		case HDMI_2560X1440P60: {
				ui_target_freq = HDMI_PLL_DIV(241500000,hdmi_timing_factor);
			}
			break;
		case HDMI_1440X900P60: {
				ui_target_freq = HDMI_PLL_DIV(106500000,hdmi_timing_factor);
			}
			break;
		case HDMI_1680X1050P60: {
				ui_target_freq = HDMI_PLL_DIV(146250000,hdmi_timing_factor);
			}
			break;
		case HDMI_1920X1200P60: {
				ui_target_freq = HDMI_PLL_DIV(193250000,hdmi_timing_factor);
			}
			break;
		case HDMI_1920X1200P75: {
				ui_target_freq = HDMI_PLL_DIV(245250000,hdmi_timing_factor);
			}
			break;

		case HDMI_640X480P72:
		case HDMI_640X480P75:{
				ui_target_freq = HDMI_PLL_DIV(31500000,hdmi_timing_factor);
			}
			break;
		case HDMI_800X600P56:{
				ui_target_freq = HDMI_PLL_DIV(36000000,hdmi_timing_factor);
			}
			break;
		case HDMI_800X600P60:{
				ui_target_freq = HDMI_PLL_DIV(40000000,hdmi_timing_factor);
			}
			break;				
		case HDMI_800X600P72:{
				ui_target_freq = HDMI_PLL_DIV(50000000,hdmi_timing_factor);
			}
			break;
		case HDMI_800X600P75:{
				ui_target_freq = HDMI_PLL_DIV(49500000,hdmi_timing_factor);
			}
			break;		
		case HDMI_1024X768P70:{
				ui_target_freq = HDMI_PLL_DIV(75000000,hdmi_timing_factor);
			}
			break;
		case HDMI_1024X768P75:{
				ui_target_freq = HDMI_PLL_DIV(78750000,hdmi_timing_factor);
			}
			break;
		case HDMI_1280X1024P75:{
				ui_target_freq = HDMI_PLL_DIV(135000000,hdmi_timing_factor);
			}
			break;
		case HDMI_1400X1050P60:{
				ui_target_freq = HDMI_PLL_DIV(121750000,hdmi_timing_factor);
			}
			break;
		case HDMI_1280X800P60:{
				ui_target_freq = HDMI_PLL_DIV(83500000,hdmi_timing_factor);
			}
			break;	
		
		case HDMI_3840X2160P50: 		 /* VID96 */
		case HDMI_3840X2160P60: 		 /* VID97 */
		case HDMI_4096X2160P50: 		 /* VID101*/
		case HDMI_4096X2160P60:			 /* VID102*/
		case HDMI_7680X4320P24:			/* VID194 */
		case HDMI_7680X4320P25:			/* VID195 */
		case HDMI_7680X4320P30:{		/* VID196 */			
				ui_target_freq = HDMI_PLL_DIV(594000000,hdmi_timing_factor);
			}
			break;
		default: {
				return TRUE;
			}
	
		}
	
		g_clock_rate[id] = ui_target_freq;
		
		//
		//	Configure IDE Clock to 2x ui_target_freq for Interlace mode Vid
		//
		switch (g_dispdev_hdmi_vid[id]) {
		case HDMI_1920X1080I60: 		/* VID5 */
		case HDMI_720X480I60:			/* VID6 */
		case HDMI_720X480I60_16X9:		/* VID7 */	
		case HDMI_1440X480I60:			/* VID10 */
		case HDMI_1440X480I60_16X9: 	/* VID11 */ 
		case HDMI_1920X1080I50: 		/* VID20 */
		case HDMI_720X576I50:			/* VID21 */
		case HDMI_720X576I50_16X9:		/* VID22 */
		case HDMI_1440X576I50:			/* VID25 */
		case HDMI_1440X576I50_16X9: 	/* VID26 */
		case HDMI_1920X1080I100:		/* VID40 */
		case HDMI_720X576I100:			/* VID44 */
		case HDMI_720X576I100_16X9: 	/* VID45 */
		case HDMI_1920X1080I120:		/* VID46 */
		case HDMI_720X480I120:			/* VID50 */
		case HDMI_720X480I120_16X9: 	/* VID51 */
		case HDMI_720X576I200:			/* VID54 */
		case HDMI_720X576I200_16X9: 	/* VID55 */
		case HDMI_720X480I240:			/* VID58 */
		case HDMI_720X480I240_16X9: {	/* VID59 */
	
				// set clock rate
				ui_target_freq	 *=1;
				disphdmi_debug(("Set interlace mode HDMI Target Frequency = %d Hertz\r\n", ui_target_freq*2));
	
			}
			break;
		default: {
				ui_target_freq	 *=1;
			}
		
		}

		if(id == HDMI_ID_1 || id == HDMI_ID_2){
			clock_rate = ui_target_freq;
		}else{
			hdmi2_clock_rate = ui_target_freq;
		}
	
		if(hdmi_ctrl_option[HDMICFG_OPTION_TIMING_DIV_1P001] == 0x1){	
			ui_target_freq += 500000; // +0.5Mhz to make sure have currect divider
			dev_io_ctrl.SEL.SET_CLK_FREQ.ui_freq   = ui_target_freq;
		} else {
			dev_io_ctrl.SEL.SET_CLK_FREQ.ui_freq   = ui_target_freq;
		}	
#if _FPGA_EMULATION_
		dev_io_ctrl.SEL.SET_CLK_FREQ.b_ycc8bit = TRUE;
#else
		dev_io_ctrl.SEL.SET_CLK_FREQ.b_ycc8bit = FALSE;
#endif
		p_dispdev_control(DISPDEV_IOCTRL_SET_CLK_FREQ, &dev_io_ctrl);
	
		disphdmi_debug(("HDMI Target Frequency = %d Hertz\r\n", ui_target_freq));

#endif

#endif

#endif
	return FALSE;

}

static void dispdev_check_sample_rate(BOOL b_enable)
{
#if 0
#ifndef __KERNEL__
	BOOL                    b_aud_opened;
	AUDIO_SR                aud_sr;

	// Audio must be opened first and can apply set Parameter/Feature.
	b_aud_opened = aud_get_lock_status();
	if (b_aud_opened == FALSE) {
		if (aud_open() != E_OK) {
			disphdmi_debug(("aud op err\r\n"));
		}

	}

	aud_set_feature(AUDIO_FEATURE_CHECK_PLAY_SAMPLING_RATE, b_enable);

	if (b_enable) {
		// Config Audio Check Sampling Rate
		if (g_dispdev_aud_fmt[id] == HDMI_AUDIO32KHZ) {
			aud_sr =  AUDIO_SR_32000;
		} else if (g_dispdev_aud_fmt[id] == HDMI_AUDIO44_1KHZ) {
			aud_sr =  AUDIO_SR_44100;
		} else {
			aud_sr =  AUDIO_SR_48000;
		}
		aud_set_parameter(AUDIO_PARAMETER_CHECKED_PLAY_SAMPLING_RATE, aud_sr);
	}

	// Restore the original aud open status
	if (b_aud_opened == FALSE) {
		aud_close();
	}
#endif
#endif
}

#if 1 // For HDMI2P1 dispdev_hdmi_obj used API
/*
    Display Device API of open HDMI2p1 Transmitter

    Display Device API of open HDMI2p1 Transmitter

    @param[in] pDevIF   Please refer to dispdevctrl.h for details.

    @return
     - @b E_PAR:    Error pDevIF content.
     - @b E_NOSPT:  Desired HDMI format no support.
     - @b E_OK:     HDMI Open done and success.
*/
static ER dispdev_open_hdmi2p1(void)
{
	HDMI_ID id = HDMI_ID_1;
#if DISPDEVIOUSED
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	DISPDEV_IOCTRL          p_dispdev_control = p_hdmi_io_control[id];
	PDISP_OBJ 				p_disp_obj;
	DISPLAYER_PARAM 		disp_lyr;

	p_disp_obj = disp_get_display_object(DISP_1);	

	if (p_dispdev_control == NULL) {
		return E_PAR;
	}

	p_dispdev_control(DISPDEV_IOCTRL_GET_HDMIMODE, &dev_io_ctrl);
	g_dispdev_hdmi_vid[id] = dev_io_ctrl.SEL.GET_HDMIMODE.video_id;
	g_dispdev_aud_fmt[id]  = dev_io_ctrl.SEL.GET_HDMIMODE.audio_id;

	disphdmi_debug(("DEVHDMI: open 2p1 (%d)\r\n", g_dispdev_hdmi_vid[id]));
#endif

	if (hdmitx2p1_is_opened() == FALSE) {
		
#if DISPDEVIOUSED
		p_dispdev_control(DISPDEV_IOCTRL_GET_ENABLE, &dev_io_ctrl);
		if (dev_io_ctrl.SEL.GET_ENABLE.b_en == TRUE) {
			dev_io_ctrl.SEL.SET_ENABLE.b_en = FALSE;
			p_dispdev_control(DISPDEV_IOCTRL_SET_ENABLE,    &dev_io_ctrl);
			p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
		}
#endif		

		disphdmi_debug(("DEVHDMI: open 2p1 1\r\n"));

		if(hdmitx2p1_open()!= E_OK){
			disphdmi_debug(("DEVHDMI: open fail (%d)\r\n", g_dispdev_hdmi_vid[id]));
			return E_NOSPT;			
		}
		
		disphdmi_debug(("DEVHDMI: open 2p1 2\r\n"));

		// Check & Change IDE Source frequency
		if (dispdev_chg_hdmi_clk_freq(id, TRUE)) {
			hdmitx2p1_close();
			//hdmitx2p1_edid_unlock();
			return E_NOSPT;
		}

#if DISPDEVIOUSED
		// Enable IDE clock
		dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = TRUE;
		p_dispdev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);
#endif
		disphdmi_debug(("DEVHDMI: open 2p1 3\r\n"));

	//hdmitx2p1_set_config(HDMI_CONFIG_ID_ADAPT_4KP60_420,    ENABLE);


	disphdmi_debug(("DEVHDMI: open 2p1 4\r\n"));
	


	// init HDMI2p1 phy after clock ready
	hdmitx2p1_set_config(HDMI_CONFIG_ID_PHY_INIT, TRUE);

	disphdmi_debug(("DEVHDMI: open 2p1 5\r\n"));


	hdmitx2p1_set_config(HDMI_CONFIG_ID_VIDEO, g_dispdev_hdmi_vid[id]);
	hdmitx2p1_set_config(HDMI_CONFIG_ID_AUDIO, g_dispdev_aud_fmt[id]);
	//hdmitx2p1_set_config(HDMI_CONFIG_ID_SOURCE, g_dispdev_src_fmt[id]);

	// Configure VID content
	if (dispdev_init_hdmi_vid_format(id, g_dispdev_hdmi_vid[id]) == FALSE) {
		hdmitx2p1_close();
		return E_NOSPT;
	}
	

	// Configure IDE
	dispdev_init_hdmi_display(id);	

	disphdmi_debug(("DEVHDMI: open 2p1 6\r\n"));

#if DISPDEVIOUSED
	dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_width	  = (disp_video_timing[g_dispdev_hdmi_index[id]].H_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].H_STARTTIME);
	dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_height	  = (disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_STARTTIME);
	dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_width	  = (disp_video_timing[g_dispdev_hdmi_index[id]].H_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].H_STARTTIME) - g_dispdev_win_hozi_ofs[id];
	dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_height	  = (disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_STARTTIME) - g_dispdev_win_vert_ofs[id];

	if(hdmitx2p1_get_config(HDMI_CONFIG_ID_2P1_FORCE_FRL_EN) == 1){// FRL always 2 pmode
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_width *= 2;
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_width *= 2;
		disphdmi_debug(("DEVHDMI: open 2p1 FRL BG (%d)*(%d)\r\n",dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_width,dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_height));
	}

	
	p_dispdev_control(DISPDEV_IOCTRL_SET_DISPSIZE, &dev_io_ctrl);

	if(hdmitx2p1_get_config(HDMI_CONFIG_ID_2P1_FORCE_FRL_EN) == 1){// FRL always 2 pmode
		disp_lyr.SEL.SET_BUFWINSIZE.ui_buf_width = dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_width;
		disp_lyr.SEL.SET_BUFWINSIZE.ui_buf_height =dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_height;
		disp_lyr.SEL.SET_BUFWINSIZE.ui_buf_line_ofs = dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_width/4;
		disp_lyr.SEL.SET_BUFWINSIZE.ui_win_width = dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_width;
		disp_lyr.SEL.SET_BUFWINSIZE.ui_win_height = dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_height;
		disp_lyr.SEL.SET_BUFWINSIZE.i_win_ofs_x = 0;
		disp_lyr.SEL.SET_BUFWINSIZE.i_win_ofs_y = 0;
		p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_SET_BUFWINSIZE, &disp_lyr);
		p_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO2, DISPLAYER_OP_SET_BUFWINSIZE, &disp_lyr);
			
		disphdmi_debug(("DEVHDMI: open 2p1 FRL input scaler (%d)*(%d)\r\n",dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_width,dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_height));
	}
			

	dev_io_ctrl.SEL.SET_ENABLE.b_en = TRUE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl);
	p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
#endif

	disphdmi_debug(("DEVHDMI: open 2p1 7\r\n"));

	hdmitx2p1_set_config(HDMI_CONFIG_ID_OE,             TRUE);

	disphdmi_debug(("DEVHDMI: open 2p1 8\r\n"));

	if(hdmitx2p1_get_config(HDMI_CONFIG_ID_2P1_FORCE_FRL_EN) == 0){
		hdmitx2p1_set_config(HDMI_CONFIG_ID_SCDC_ENABLE,    TRUE);
		disphdmi_debug(("DEVHDMI: open 2p1 10.1\r\n"));
	} else {
		hdmitx2p1_set_config(HDMI_CONFIG_ID_2P1_FRL_LTP_EN, TRUE);
		disphdmi_debug(("DEVHDMI: open 2p1 10.2\r\n"));
	}

	
	}

	/* check hot plug when open done*/
	/**
	if(hdmi2p1_hpd_status){
		hdmi2p1_state |= HPG_CONNECT;
	} else {
		hdmi2p1_state &= ~HPG_CONNECT;
	}
	*/

		
	return E_OK;
}

/*
    Display Device API of Close HDMIHDMI2p1 Transmitter

    Display Device API of Close HDMIHDMI2p1 Transmitter

    @param[in] pDevIF   Please refer to dispdevctrl.h for details.

    @return
     - @b E_OK:     HDMI Close done and success.
*/
static ER dispdev_close_hdmi2p1(void)
{
	if(hdmitx2p1_get_config(HDMI_CONFIG_ID_2P1_FORCE_FRL_EN) == 1){
		hdmitx2p1_set_config(HDMI_CONFIG_ID_2P1_FRL_LTP_EN, FALSE);
		disphdmi_debug(("DEVHDMI: close 2p1 task stop\r\n"));
	}
	
	hdmitx2p1_close();
	return E_OK;
}

/*
    Set HDMI2p1 Display Device IOCTRL function pointer
*/
static void dispdev_set_hdmi2p1_ioctrl(FP p_io_ctrl_func)
{
	HDMI_ID id = HDMI_ID_1;
	
#if DISPDEVIOUSED
	p_hdmi_io_control[id] = (DISPDEV_IOCTRL)p_io_ctrl_func;
#endif
}

/*
    PreGet HDMI2p1 size

    PreGet HDMI2p1 size

    @return void
*/
static ER dispdev_get_hdmi2p1_size(DISPDEV_GET_PRESIZE *t_size)
{
	HDMI_ID id = HDMI_ID_1;
	UINT32                  idx = 0;
#if DISPDEVIOUSED
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	DISPDEV_IOCTRL          p_dispdev_control = p_hdmi_io_control[id];


	disphdmi_debug(("getHDMISize START\r\n"));

	if (p_dispdev_control == NULL) {
		return E_PAR;
	}

	//
	//Search the mapping Video ID index
	//
	p_dispdev_control(DISPDEV_IOCTRL_GET_HDMIMODE, &dev_io_ctrl);
	for (idx = 0; idx < VIDMAX; idx++) {
		if (disp_video_timing[idx].VID == dev_io_ctrl.SEL.GET_HDMIMODE.video_id) {
			break;
		}
	}
#endif

	if (idx < VIDMAX) {
		t_size->ui_buf_width   = (disp_video_timing[idx].H_ENDTIME - disp_video_timing[idx].H_STARTTIME);
		t_size->ui_buf_height  = (disp_video_timing[idx].V_ODD_ENDTIME - disp_video_timing[idx].V_ODD_STARTTIME);
	} else {
		t_size->ui_buf_width   = 0;
		t_size->ui_buf_height  = 0;
	}


	return E_OK;
}

/*
    Set HDMI2p1 Display config before open
*/
static ER dispdev_preconfig_hdmi2p1(PDISP_HDMI_CFG config)
{
	HDMI_ID id = HDMI_ID_1;
	
	disphdmi_debug(("dispdev_preconfig_hdmi \r\n"));
	g_dispdev_aud_fmt[id] = (HDMI_AUDIOFMT)config->audio_fmt;
	g_dispdev_hdmi_vid[id] = (HDMI_VIDEOID)config->vid;
	g_dispdev_hdmi_mode[id] = (HDMI_MODE)config->output_mode;
	g_dispdev_src_fmt[id] = (HDMI_SRCFMT)config->source;

	return E_OK;
}

void hdmitx2p1_get_video_id(UINT32 *table)
{
	UINT32 i;
	UINT32 p_len = MAX_ID_CAP;
	UINT32 j;
	UINT32 ofs;

	for (i = 0; i < p_len; i++) {
		j = disp_video_id[i].video_id;
		ofs = j >> 5;
		//printk("j = %d, ofs = %d, shift = %d\r\n", j, ofs, (j % 32));
		table[ofs] |= 1 << (j % 32);
	}
}

void hdmitx2p1_get_video_id_cap(UINT32 *p_len, PHDMI_VDOIDCAP vdo_id_cap, BOOL copystr)
{
	UINT32 i;
	*p_len = MAX_ID_CAP;

	for (i = 0; i < *p_len; i++) {
		vdo_id_cap[i].video_id = disp_video_id[i].video_id;
		if (copystr == 1) {
			vdo_id_cap[i].info = disp_video_id[i].info;
		}
	}
}


#endif

#if 1 // For HDMI1 dispdev_hdmi_obj used API

/*
    Display Device API of open HDMI Transmitter

    Display Device API of open HDMI Transmitter

    @param[in] pDevIF   Please refer to dispdevctrl.h for details.

    @return
     - @b E_PAR:    Error pDevIF content.
     - @b E_NOSPT:  Desired HDMI format no support.
     - @b E_OK:     HDMI Open done and success.
*/
static ER dispdev_open_hdmi(void)
{
	HDMI_ID id = HDMI_ID_2;
#if DISPDEVIOUSED
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	DISPDEV_IOCTRL          p_dispdev_control = p_hdmi_io_control[id];

	if (p_dispdev_control == NULL) {
		return E_PAR;
	}

	p_dispdev_control(DISPDEV_IOCTRL_GET_HDMIMODE, &dev_io_ctrl);
	g_dispdev_hdmi_vid[id] = dev_io_ctrl.SEL.GET_HDMIMODE.video_id;
	g_dispdev_aud_fmt[id]  = dev_io_ctrl.SEL.GET_HDMIMODE.audio_id;
#endif

	

	disphdmi_debug(("DEVHDMI: open (%d)\r\n", g_dispdev_hdmi_vid[id]));

	#ifndef __KERNEL__
	//lcd_init             = pinmux_get_disp_mode(PINMUX_FUNC_ID_HDMI);
	#endif

	if (hdmitx_is_opened() == FALSE) {
	#if DISPDEVIOUSED
		p_dispdev_control(DISPDEV_IOCTRL_GET_ENABLE, &dev_io_ctrl);
		if (dev_io_ctrl.SEL.GET_ENABLE.b_en == TRUE) {
			dev_io_ctrl.SEL.SET_ENABLE.b_en = FALSE;
			p_dispdev_control(DISPDEV_IOCTRL_SET_ENABLE,    &dev_io_ctrl);
			p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
		}
	#endif
		// Open HDMI-Tx & Read EDID
		if (hdmitx_open() != E_OK) {
			//disphdmi_debug(("DEVHDMI: open fail(%d)\r\n", hdmi_cfg.vid));
			disphdmi_debug(("DEVHDMI: open fail (%d)\r\n", g_dispdev_hdmi_vid[id]));
			
			return E_NOSPT;
		}

		if (hdmitx_get_config(HDMI_CONFIG_ID_VERIFY_VIDEOID)) {
			/*
			if(!hdmitx_verify_video_id(&g_dispdev_hdmi_vid[id])){
				DBG_WRN("Desired Video Format (%d) Sink not support!!\r\n",g_dispdev_hdmi_vid[id]);
			}
			*/
		}

		// Check & Change IDE Source frequency
		if (dispdev_chg_hdmi_clk_freq(id, TRUE)) {
			hdmitx_close();
			
			return E_NOSPT;
		}

#if DISPDEVIOUSED
		// Enable IDE clock
		dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = TRUE;
		p_dispdev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);
#endif
		hdmitx_set_config(HDMI_CONFIG_ID_ADAPT_4KP60_420,    ENABLE);

		// init HDMI phy after clock ready
		hdmitx_set_config(HDMI_CONFIG_ID_PHY_INIT, TRUE);


		hdmitx_set_config(HDMI_CONFIG_ID_PHYREG_PD, TRUE);



		hdmitx_set_config(HDMI_CONFIG_ID_VIDEO, g_dispdev_hdmi_vid[id]);
		hdmitx_set_config(HDMI_CONFIG_ID_AUDIO, g_dispdev_aud_fmt[id]);
		hdmitx_set_config(HDMI_CONFIG_ID_SOURCE, g_dispdev_src_fmt[id]);


		// Configure VID content
		if (dispdev_init_hdmi_vid_format(id, g_dispdev_hdmi_vid[id]) == FALSE) {
			hdmitx_close();
			
			return E_NOSPT;
		}


		// Configure IDE
		dispdev_init_hdmi_display(id);
		
#if DISPDEVIOUSED
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_width     = (disp_video_timing[g_dispdev_hdmi_index[id]].H_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].H_STARTTIME);
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_height    = (disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_STARTTIME);
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_width     = (disp_video_timing[g_dispdev_hdmi_index[id]].H_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].H_STARTTIME) - g_dispdev_win_hozi_ofs[id];
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_height    = (disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_STARTTIME) - g_dispdev_win_vert_ofs[id];
		p_dispdev_control(DISPDEV_IOCTRL_SET_DISPSIZE, &dev_io_ctrl);
		
		
		dev_io_ctrl.SEL.SET_ENABLE.b_en = TRUE;
		p_dispdev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl);
		p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
#endif
		
		hdmitx_check_video_stable();
		hdmitx_set_config(HDMI_CONFIG_ID_OE,             TRUE);
		hdmitx_set_config(HDMI_CONFIG_ID_AUDIO_STREAM_EN, TRUE);
		
		DBG_WRN("=================================\n");
		DBG_WRN("audio stream enable. \n");

		hdmitx_set_config(HDMI_CONFIG_ID_SCDC_ENABLE,    ENABLE);

		/* check hot plug when open done*/
		/*
		if(hdmi_hpd_status){
			hdmi_state |= HPG_CONNECT;
		} else {
			hdmi_state &= ~HPG_CONNECT;
		}		
		*/

	} else {
		//
		// HDMI is already opened, Just changing HDMI format
		//
		hdmitx_set_config(HDMI_CONFIG_ID_AV_MUTE, TRUE);
		//p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);

		hdmitx_set_config(HDMI_CONFIG_ID_AUDIO_STREAM_EN, FALSE);
		//p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);

		if (hdmitx_get_config(HDMI_CONFIG_ID_VERIFY_VIDEOID)) {
			/*
			if(!hdmitx_verify_video_id(&g_dispdev_hdmi_vid[id])){
				DBG_WRN("Desired Video Format (%d) Sink not support!!\r\n",g_dispdev_hdmi_vid[id]);
			}
			*/
		}

		// Check & Change IDE Source frequency
		if (dispdev_chg_hdmi_clk_freq(id, FALSE)) {
			hdmitx_close();
			
			return E_NOSPT;
		}

		// Enable IDE clock
		#if DISPDEVIOUSED
		dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = TRUE;
		p_dispdev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);
		#endif

		hdmitx_set_config(HDMI_CONFIG_ID_ADAPT_4KP60_420,    ENABLE);

		// Configure VID content
		if (dispdev_init_hdmi_vid_format(id, g_dispdev_hdmi_vid[id]) == FALSE) {
			hdmitx_close();
			
			return E_NOSPT;
		}

		// Configure IDE
		dispdev_init_hdmi_display(id);
		#if DISPDEVIOUSED
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_width     = (disp_video_timing[g_dispdev_hdmi_index[id]].H_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].H_STARTTIME);
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_height    = (disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_STARTTIME);
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_width     = (disp_video_timing[g_dispdev_hdmi_index[id]].H_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].H_STARTTIME) - g_dispdev_win_hozi_ofs[id];
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_height    = (disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_STARTTIME) - g_dispdev_win_vert_ofs[id];
		p_dispdev_control(DISPDEV_IOCTRL_SET_DISPSIZE, &dev_io_ctrl);
		#endif
		// Open HDMI-Tx
		hdmitx_set_config(HDMI_CONFIG_ID_VIDEO, g_dispdev_hdmi_vid[id]);
		hdmitx_set_config(HDMI_CONFIG_ID_AUDIO, g_dispdev_aud_fmt[id]);
		hdmitx_set_config(HDMI_CONFIG_ID_SOURCE, g_dispdev_src_fmt[id]);
		#if DISPDEVIOUSED
		p_dispdev_control(DISPDEV_IOCTRL_SET_LOAD,      NULL);

		dev_io_ctrl.SEL.SET_ENABLE.b_en = TRUE;
		p_dispdev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl);
		p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
		#endif
		
		hdmitx_check_video_stable();
		hdmitx_set_config(HDMI_CONFIG_ID_OE,             TRUE);
		hdmitx_set_config(HDMI_CONFIG_ID_AUDIO_STREAM_EN, TRUE);

		hdmitx_set_config(HDMI_CONFIG_ID_SCDC_ENABLE,    ENABLE);

		// Wait TV Video Stable and then clear AV-Mute
		hdmitx_set_config(HDMI_CONFIG_ID_AV_MUTE, FALSE);
	}

	dispdev_check_sample_rate(ENABLE);

	

	//hdmi_state |= HDMI_DEV_OPEN;	
	
	return E_OK;
}

/*
    Display Device API of Close HDMI Transmitter

    Display Device API of Close HDMI Transmitter

    @param[in] pDevIF   Please refer to dispdevctrl.h for details.

    @return
     - @b E_OK:     HDMI Close done and success.
*/
static ER dispdev_close_hdmi(void)
{
	HDMI_ID id = HDMI_ID_2;
	
#if DISPDEVIOUSED
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	DISPDEV_IOCTRL          p_dispdev_control = p_hdmi_io_control[id];
#endif
	if (hdmitx_is_opened() == FALSE) {
		return E_OK;
	}

	disphdmi_debug(("DEVHDMI: close\r\n"));

	hdmitx_set_config(HDMI_CONFIG_ID_OE, FALSE);
	// Close HDMI
	hdmitx_close();
#if DISPDEVIOUSED
	// Disable IDE
	dev_io_ctrl.SEL.SET_ENABLE.b_en = FALSE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl);
	p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);

	dev_io_ctrl.SEL.SET_OUT_COMPONENT.comp0    = IDE_COMPONENT_R;
	dev_io_ctrl.SEL.SET_OUT_COMPONENT.comp1    = IDE_COMPONENT_G;
	dev_io_ctrl.SEL.SET_OUT_COMPONENT.comp2    = IDE_COMPONENT_B;
	dev_io_ctrl.SEL.SET_OUT_COMPONENT.b_bit_swap = FALSE;
	dev_io_ctrl.SEL.SET_OUT_COMPONENT.b_length  = FALSE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_OUT_COMPONENT, &dev_io_ctrl);

	dev_io_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_x = 0;
	dev_io_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_y = 0;
	p_dispdev_control(DISPDEV_IOCTRL_SET_CONST_OFS, &dev_io_ctrl);

	// Close IDE clock
	dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = FALSE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);
#endif
	dispdev_check_sample_rate(DISABLE);

	//hdmi_state &= ~HDMI_DEV_OPEN;


	return E_OK;
}

/*
    Set HDMI Display Device IOCTRL function pointer
*/
static void dispdev_set_hdmi_ioctrl(FP p_io_ctrl_func)
{
	HDMI_ID id = HDMI_ID_2;
	
#if DISPDEVIOUSED
	p_hdmi_io_control[id] = (DISPDEV_IOCTRL)p_io_ctrl_func;
#endif
}

/*
    PreGet HDMI size

    PreGet HDMI size

    @return void
*/
static ER dispdev_get_hdmi_size(DISPDEV_GET_PRESIZE *t_size)
{
	HDMI_ID id = HDMI_ID_2;
	UINT32                  idx = 0;
#if DISPDEVIOUSED
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	DISPDEV_IOCTRL          p_dispdev_control = p_hdmi_io_control[id];


	disphdmi_debug(("getHDMISize START\r\n"));

	if (p_dispdev_control == NULL) {
		return E_PAR;
	}

	//
	//Search the mapping Video ID index
	//
	p_dispdev_control(DISPDEV_IOCTRL_GET_HDMIMODE, &dev_io_ctrl);
	for (idx = 0; idx < VIDMAX; idx++) {
		if (disp_video_timing[idx].VID == dev_io_ctrl.SEL.GET_HDMIMODE.video_id) {
			break;
		}
	}
#endif

	if (idx < VIDMAX) {
		t_size->ui_buf_width   = (disp_video_timing[idx].H_ENDTIME - disp_video_timing[idx].H_STARTTIME);
		t_size->ui_buf_height  = (disp_video_timing[idx].V_ODD_ENDTIME - disp_video_timing[idx].V_ODD_STARTTIME);
	} else {
		t_size->ui_buf_width   = 0;
		t_size->ui_buf_height  = 0;
	}


	return E_OK;
}

/*
    Set HDMI Display config before open
*/
static ER dispdev_preconfig_hdmi(PDISP_HDMI_CFG config)
{
	HDMI_ID id = HDMI_ID_2;
	
	disphdmi_debug(("dispdev_preconfig_hdmi \r\n"));
	g_dispdev_aud_fmt[id] = (HDMI_AUDIOFMT)config->audio_fmt;
	g_dispdev_hdmi_vid[id] = (HDMI_VIDEOID)config->vid;
	g_dispdev_hdmi_mode[id] = (HDMI_MODE)config->output_mode;
	g_dispdev_src_fmt[id] = (HDMI_SRCFMT)config->source;

	return E_OK;
}

void hdmitx_get_video_id(UINT32 *table)
{
	UINT32 i;
	UINT32 p_len = MAX_ID_CAP;
	UINT32 j;
	UINT32 ofs;

	for (i = 0; i < p_len; i++) {
		j = disp_video_id[i].video_id;
		ofs = j >> 5;
		//printk("j = %d, ofs = %d, shift = %d\r\n", j, ofs, (j % 32));
		table[ofs] |= 1 << (j % 32);
	}
}

void hdmitx_get_video_id_cap(UINT32 *p_len, PHDMI_VDOIDCAP vdo_id_cap, BOOL copystr)
{
	UINT32 i;
	*p_len = MAX_ID_CAP;

	for (i = 0; i < *p_len; i++) {
		vdo_id_cap[i].video_id = disp_video_id[i].video_id;
		if (copystr == 1) {
			vdo_id_cap[i].info = disp_video_id[i].info;
		}
	}
}


#endif

#if 1 // For HDMI2 dispdev_hdmi_obj used API

/*
    Display Device API of open HDMI Transmitter

    Display Device API of open HDMI Transmitter

    @param[in] pDevIF   Please refer to dispdevctrl.h for details.

    @return
     - @b E_PAR:    Error pDevIF content.
     - @b E_NOSPT:  Desired HDMI format no support.
     - @b E_OK:     HDMI Open done and success.
*/
static ER dispdev_open_hdmi2(void)
{
	HDMI_ID id = HDMI_ID_3;
#if DISPDEVIOUSED
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	DISPDEV_IOCTRL          p_dispdev_control = p_hdmi_io_control[id];

	if (p_dispdev_control == NULL) {
		return E_PAR;
	}

	p_dispdev_control(DISPDEV_IOCTRL_GET_HDMIMODE, &dev_io_ctrl);
	g_dispdev_hdmi_vid[id] = dev_io_ctrl.SEL.GET_HDMIMODE.video_id;
	g_dispdev_aud_fmt[id]  = dev_io_ctrl.SEL.GET_HDMIMODE.audio_id;
#endif

	

	disphdmi_debug(("DEVHDMI: open (%d)\r\n", g_dispdev_hdmi_vid[id]));

	#ifndef __KERNEL__
	//lcd_init             = pinmux_get_disp_mode(PINMUX_FUNC_ID_HDMI);
	#endif

	if (hdmitx2_is_opened() == FALSE) {
	#if DISPDEVIOUSED
		p_dispdev_control(DISPDEV_IOCTRL_GET_ENABLE, &dev_io_ctrl);
		if (dev_io_ctrl.SEL.GET_ENABLE.b_en == TRUE) {
			dev_io_ctrl.SEL.SET_ENABLE.b_en = FALSE;
			p_dispdev_control(DISPDEV_IOCTRL_SET_ENABLE,    &dev_io_ctrl);
			p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
		}
	#endif
		// Open HDMI-Tx & Read EDID
		if (hdmitx2_open() != E_OK) {
			//disphdmi_debug(("DEVHDMI: open fail(%d)\r\n", hdmi_cfg.vid));
			disphdmi_debug(("DEVHDMI: open fail (%d)\r\n", g_dispdev_hdmi_vid[id]));
			
			return E_NOSPT;
		}

		if (hdmitx2_get_config(HDMI_CONFIG_ID_VERIFY_VIDEOID)) {
			/*
			if(!hdmitx2_verify_video_id(&g_dispdev_hdmi_vid[id])){
				DBG_WRN("Desired Video Format (%d) Sink not support!!\r\n",g_dispdev_hdmi_vid[id]);
			}
			*/
		}

		// Check & Change IDE Source frequency
		if (dispdev_chg_hdmi_clk_freq(id, TRUE)) {
			hdmitx2_close();
			
			return E_NOSPT;
		}
#if DISPDEVIOUSED
		// Enable IDE clock
		dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = TRUE;
		p_dispdev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);
#endif

		hdmitx2_set_config(HDMI_CONFIG_ID_ADAPT_4KP60_420,    ENABLE);

		// init HDMI phy after clock ready
		hdmitx2_set_config(HDMI_CONFIG_ID_PHY_INIT, TRUE);


		hdmitx2_set_config(HDMI_CONFIG_ID_PHYREG_PD, TRUE);
		
		hdmitx2_set_config(HDMI_CONFIG_ID_VIDEO, g_dispdev_hdmi_vid[id]);
		hdmitx2_set_config(HDMI_CONFIG_ID_AUDIO, g_dispdev_aud_fmt[id]);
		hdmitx2_set_config(HDMI_CONFIG_ID_SOURCE, 0x0);//g_dispdev_src_fmt[id]

		// Configure VID content
		if (dispdev_init_hdmi_vid_format(id, g_dispdev_hdmi_vid[id]) == FALSE) {
			hdmitx2_close();
			
			return E_NOSPT;
		}

		// Configure IDE
		dispdev_init_hdmi_display(id);
		
#if DISPDEVIOUSED
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_width     = (disp_video_timing[g_dispdev_hdmi_index[id]].H_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].H_STARTTIME);
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_height    = (disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_STARTTIME);
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_width     = (disp_video_timing[g_dispdev_hdmi_index[id]].H_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].H_STARTTIME) - g_dispdev_win_hozi_ofs[id];
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_height    = (disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_STARTTIME) - g_dispdev_win_vert_ofs[id];
		p_dispdev_control(DISPDEV_IOCTRL_SET_DISPSIZE, &dev_io_ctrl);
		
		
		dev_io_ctrl.SEL.SET_ENABLE.b_en = TRUE;
		p_dispdev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl);
		p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
#endif

		hdmitx2_check_video_stable();
		hdmitx2_set_config(HDMI_CONFIG_ID_OE,             TRUE);
		hdmitx2_set_config(HDMI_CONFIG_ID_AUDIO_STREAM_EN, TRUE);
		
		DBG_WRN("=================================\n");
		DBG_WRN("audio stream enable. \n");

		hdmitx2_set_config(HDMI_CONFIG_ID_SCDC_ENABLE,    ENABLE);


		/* check hot plug when open done*/
		/*
		if(hdmi2_hpd_status){
			hdmi2_state |= HPG_CONNECT;
		} else {
			hdmi2_state &= ~HPG_CONNECT;
		}		
		*/

	} else {
		//
		// HDMI is already opened, Just changing HDMI format
		//
		hdmitx2_set_config(HDMI_CONFIG_ID_AV_MUTE, TRUE);
		//p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);

		hdmitx2_set_config(HDMI_CONFIG_ID_AUDIO_STREAM_EN, FALSE);
		//p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);

		if (hdmitx2_get_config(HDMI_CONFIG_ID_VERIFY_VIDEOID)) {
			/*
			if(!hdmitx2_verify_video_id(&g_dispdev_hdmi_vid[id])){
				DBG_WRN("Desired Video Format (%d) Sink not support!!\r\n",g_dispdev_hdmi_vid[id]);
			}
			*/
		}

		// Check & Change IDE Source frequency
		if (dispdev_chg_hdmi_clk_freq(id, FALSE)) {
			hdmitx2_close();
			
			return E_NOSPT;
		}

		// Enable IDE clock
		#if DISPDEVIOUSED
		dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = TRUE;
		p_dispdev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);
		#endif

		hdmitx2_set_config(HDMI_CONFIG_ID_ADAPT_4KP60_420,    ENABLE);

		
		// Configure VID content
		if (dispdev_init_hdmi_vid_format(id, g_dispdev_hdmi_vid[id]) == FALSE) {
			hdmitx2_close();
			
			return E_NOSPT;
		}

		// Configure IDE
		dispdev_init_hdmi_display(id);
		#if DISPDEVIOUSED
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_width     = (disp_video_timing[g_dispdev_hdmi_index[id]].H_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].H_STARTTIME);
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_buf_height    = (disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_STARTTIME);
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_width     = (disp_video_timing[g_dispdev_hdmi_index[id]].H_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].H_STARTTIME) - g_dispdev_win_hozi_ofs[id];
		dev_io_ctrl.SEL.SET_DISPSIZE.ui_win_height    = (disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_ENDTIME - disp_video_timing[g_dispdev_hdmi_index[id]].V_ODD_STARTTIME) - g_dispdev_win_vert_ofs[id];
		p_dispdev_control(DISPDEV_IOCTRL_SET_DISPSIZE, &dev_io_ctrl);
		#endif
		// Open HDMI-Tx
		hdmitx2_set_config(HDMI_CONFIG_ID_VIDEO, g_dispdev_hdmi_vid[id]);
		hdmitx2_set_config(HDMI_CONFIG_ID_AUDIO, g_dispdev_aud_fmt[id]);
		hdmitx2_set_config(HDMI_CONFIG_ID_SOURCE, g_dispdev_src_fmt[id]);
		#if DISPDEVIOUSED
		p_dispdev_control(DISPDEV_IOCTRL_SET_LOAD,      NULL);

		dev_io_ctrl.SEL.SET_ENABLE.b_en = TRUE;
		p_dispdev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl);
		p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);
		#endif
		
		hdmitx2_check_video_stable();
		hdmitx2_set_config(HDMI_CONFIG_ID_OE,             TRUE);
		hdmitx2_set_config(HDMI_CONFIG_ID_AUDIO_STREAM_EN, TRUE);

		hdmitx2_set_config(HDMI_CONFIG_ID_SCDC_ENABLE,    ENABLE);

		// Wait TV Video Stable and then clear AV-Mute
		hdmitx2_set_config(HDMI_CONFIG_ID_AV_MUTE, FALSE);
	}

	dispdev_check_sample_rate(ENABLE);

	

	//hdmi2_state |= HDMI_DEV_OPEN;

	return E_OK;
}

/*
    Display Device API of Close HDMI Transmitter

    Display Device API of Close HDMI Transmitter

    @param[in] pDevIF   Please refer to dispdevctrl.h for details.

    @return
     - @b E_OK:     HDMI Close done and success.
*/
static ER dispdev_close_hdmi2(void)
{
	HDMI_ID id = HDMI_ID_3;
	
#if DISPDEVIOUSED
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	DISPDEV_IOCTRL          p_dispdev_control = p_hdmi_io_control[id];
#endif
	if (hdmitx2_is_opened() == FALSE) {
		return E_OK;
	}

	disphdmi_debug(("DEVHDMI: close\r\n"));

	hdmitx2_set_config(HDMI_CONFIG_ID_OE, FALSE);
	// Close HDMI
	hdmitx2_close();
#if DISPDEVIOUSED
	// Disable IDE
	dev_io_ctrl.SEL.SET_ENABLE.b_en = FALSE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_ENABLE, &dev_io_ctrl);
	p_dispdev_control(DISPDEV_IOCTRL_WAIT_FRAMEEND, NULL);

	dev_io_ctrl.SEL.SET_OUT_COMPONENT.comp0    = IDE_COMPONENT_R;
	dev_io_ctrl.SEL.SET_OUT_COMPONENT.comp1    = IDE_COMPONENT_G;
	dev_io_ctrl.SEL.SET_OUT_COMPONENT.comp2    = IDE_COMPONENT_B;
	dev_io_ctrl.SEL.SET_OUT_COMPONENT.b_bit_swap = FALSE;
	dev_io_ctrl.SEL.SET_OUT_COMPONENT.b_length  = FALSE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_OUT_COMPONENT, &dev_io_ctrl);

	dev_io_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_x = 0;
	dev_io_ctrl.SEL.SET_CONST_OFS.ui_win_const_ofs_y = 0;
	p_dispdev_control(DISPDEV_IOCTRL_SET_CONST_OFS, &dev_io_ctrl);

	// Close IDE clock
	dev_io_ctrl.SEL.SET_CLK_EN.b_clk_en = FALSE;
	p_dispdev_control(DISPDEV_IOCTRL_SET_CLK_EN, &dev_io_ctrl);
#endif
	dispdev_check_sample_rate(DISABLE);

	//hdmi2_state &= ~HDMI_DEV_OPEN;


	return E_OK;
}

/*
    Set HDMI Display Device IOCTRL function pointer
*/
static void dispdev_set_hdmi2_ioctrl(FP p_io_ctrl_func)
{
	HDMI_ID id = HDMI_ID_3;

	disphdmi_debug(("dispdev_set_hdmi2_ioctrl\r\n"));
	
#if DISPDEVIOUSED
	p_hdmi_io_control[id] = (DISPDEV_IOCTRL)p_io_ctrl_func;
#endif
}

/*
    PreGet HDMI size

    PreGet HDMI size

    @return void
*/
static ER dispdev_get_hdmi2_size(DISPDEV_GET_PRESIZE *t_size)
{
	HDMI_ID id = HDMI_ID_3;
	UINT32                  idx = 0;
#if DISPDEVIOUSED
	DISPDEV_IOCTRL_PARAM    dev_io_ctrl;
	DISPDEV_IOCTRL          p_dispdev_control = p_hdmi_io_control[id];


	disphdmi_debug(("getHDMISize START\r\n"));

	if (p_dispdev_control == NULL) {
		return E_PAR;
	}

	//
	//Search the mapping Video ID index
	//
	p_dispdev_control(DISPDEV_IOCTRL_GET_HDMIMODE, &dev_io_ctrl);
	for (idx = 0; idx < VIDMAX; idx++) {
		if (disp_video_timing[idx].VID == dev_io_ctrl.SEL.GET_HDMIMODE.video_id) {
			break;
		}
	}
#endif

	if (idx < VIDMAX) {
		t_size->ui_buf_width   = (disp_video_timing[idx].H_ENDTIME - disp_video_timing[idx].H_STARTTIME);
		t_size->ui_buf_height  = (disp_video_timing[idx].V_ODD_ENDTIME - disp_video_timing[idx].V_ODD_STARTTIME);
	} else {
		t_size->ui_buf_width   = 0;
		t_size->ui_buf_height  = 0;
	}


	return E_OK;
}

/*
    Set HDMI Display config before open
*/
static ER dispdev_preconfig_hdmi2(PDISP_HDMI_CFG config)
{
	HDMI_ID id = HDMI_ID_3;
	
	disphdmi_debug(("dispdev_preconfig_hdmi \r\n"));

	g_dispdev_aud_fmt[id] = (HDMI_AUDIOFMT)config->audio_fmt;
	g_dispdev_hdmi_vid[id] = (HDMI_VIDEOID)config->vid;
	g_dispdev_hdmi_mode[id] = (HDMI_MODE)config->output_mode;
	g_dispdev_src_fmt[id] = (HDMI_SRCFMT)config->source;

	return E_OK;
}

void hdmitx2_get_video_id(UINT32 *table)
{
	UINT32 i;
	UINT32 p_len = MAX_ID_CAP;
	UINT32 j;
	UINT32 ofs;

	for (i = 0; i < p_len; i++) {
		j = disp_video_id[i].video_id;
		ofs = j >> 5;
		//printk("j = %d, ofs = %d, shift = %d\r\n", j, ofs, (j % 32));
		table[ofs] |= 1 << (j % 32);
	}
}

void hdmitx2_get_video_id_cap(UINT32 *p_len, PHDMI_VDOIDCAP vdo_id_cap, BOOL copystr)
{
	UINT32 i;
	*p_len = MAX_ID_CAP;

	for (i = 0; i < *p_len; i++) {
		vdo_id_cap[i].video_id = disp_video_id[i].video_id;
		if (copystr == 1) {
			vdo_id_cap[i].info = disp_video_id[i].info;
		}
	}
}


#endif



/**
    Get HDMI Display Device Object

    Get HDMI Display Device Object. This Object is used to control the HDMI Transmitter.

    @return The pointer of HDMI Display Device object.
*/
PDISPDEV_OBJ dispdev_get_hdmi2p1_dev_obj(void)
{
	disphdmi_debug(("dispdev_get_hdmi2p1_dev_obj \r\n"));
	return &dispdev_hdmi2p1_obj;
}
	
#if DISPDEVIOUSED
PDISPDEVHDMI_CTRL dispdev_get_hdmi2p1_dev_control(void)
{
	disphdmi_debug(("dispdev_get_hdmi2p1_dev_control \r\n"));
	return &dispdev_hdmi2p1_control;
}
#endif

PDISPDEV_OBJ dispdev_get_hdmi_dev_obj(void)
{
	disphdmi_debug(("dispdev_get_hdmi_dev_obj \r\n"));
	return &dispdev_hdmi_obj;
}

#if DISPDEVIOUSED
PDISPDEVHDMI_CTRL dispdev_get_hdmi_dev_control(void)
{
	disphdmi_debug(("dispdev_get_hdmi_dev_control \r\n"));
	return &dispdev_hdmi_control;
}
#endif

PDISPDEV_OBJ dispdev_get_hdmi2_dev_obj(void)
{
	disphdmi_debug(("dispdev_get_hdmi2_dev_obj \r\n"));
	return &dispdev_hdmi2_obj;
}

#if DISPDEVIOUSED
PDISPDEVHDMI_CTRL dispdev_get_hdmi2_dev_control(void)
{
	disphdmi_debug(("dispdev_get_hdmi2_dev_control \r\n"));
	return &dispdev_hdmi2_control;
}
#endif


void  hdmitx_module_init(void)
{
	PDISP_OBJ p_disp_obj;

	p_disp_obj = disp_get_display_object(DISP_1);
	p_disp_obj->phdmidev_control[HDMI_ID_1] = dispdev_get_hdmi2p1_dev_control();
	p_disp_obj->phdmidev_control[HDMI_ID_2] = dispdev_get_hdmi_dev_control();
	p_disp_obj->phdmidev_control[HDMI_ID_3] = dispdev_get_hdmi2_dev_control();

	p_disp_obj = disp_get_display_object(DISP_2);
	p_disp_obj->phdmidev_control[HDMI_ID_1] = dispdev_get_hdmi2p1_dev_control();
	p_disp_obj->phdmidev_control[HDMI_ID_2] = dispdev_get_hdmi_dev_control();
	p_disp_obj->phdmidev_control[HDMI_ID_3] = dispdev_get_hdmi2_dev_control();


	p_disp_obj = disp_get_display_object(DISP_3);
	p_disp_obj->phdmidev_control[HDMI_ID_1] = dispdev_get_hdmi2p1_dev_control();
	p_disp_obj->phdmidev_control[HDMI_ID_2] = dispdev_get_hdmi_dev_control();
	p_disp_obj->phdmidev_control[HDMI_ID_3] = dispdev_get_hdmi2_dev_control();

	return;
}



