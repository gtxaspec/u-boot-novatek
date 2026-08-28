/*
    HDMI TX module EDID Parser

    HDMI TX module EDID Parser.

    @file       hdmitx2p1_edid.c
    @ingroup    mIDrvDisp_HDMITx
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#include "hdmitx_int.h"
#include "hdmitx_reg.h"



#define HDMITX_EXTRA_NUM 25  // ET 17 + STI 8


// The last one is used to store the valid VID number
static UINT8        g_svd_vid_hdmi2p1[HDMITX_EDIDNUM + 1];
static UINT8        g_svd_vid_extra_hdmi2p1[HDMITX_EXTRA_NUM]; // et/mt/sti support list
static UINT8        g_svd_vid_cnt_hdmi2p1;
static UINT8        g_svd_hdmi2p1_vic[8];//1.4b extended 4K format
static UINT8        g_svd_3d_cap_hdmi2p1[HDMITX_EDIDNUM + 1];
static UINT8        g_recommend_vid_hdmi2p1[2];
static UINT32       gui_vsdb_h20cap_hdmi2p1;//store the HDMI2.0 VSDB byte 4-7
static UINT32       gui_hdmi2p1_edid_info   = 0x00;
static UINT32       gui_native_edid_info = 0x00;
static UINT32       hdmi2p1_ext_blk = 1;
static BOOL			hdmi2p1_edid_error = 0;

static BOOL         gb_valid_resolution = FALSE;

extern I2C_SESSION  hdmi_i2c_ses;
extern BOOL         hdmi_i2c_select;
#ifdef __KERNEL__
extern HDMI_I2C_OBJ hdmi_i2c_obj[1];
#else
extern HDMI_I2C_OBJ hdmi_i2c_obj[2];
#endif
extern UINT32       gui_manufacturer_id;
extern BOOL         b_hdmi_i2c_opened;

static  ER      hdmitx2p1_ddc_read(UINT32 addr, UINT32 segm, UINT32 offset, UINT32 cnt);
static  UINT32  hdmitx2p1_parse_edid_header(void);
static  UINT32  hdmitx2p1_parse_edid_version(void);
static  UINT32  hdmitx2p1_verify_edid_checksum(UINT32 blk_num);
static  void    hdmitx2p1_parse_etmt(void);
static  void    hdmitx2p1_parse_sti(void);
static  void    hdmitx2p1_parse_dtd(UINT32 ui_addr);
static  void    hdmitx2p1_parse_dtd_in_block_one(void);
static  void    hdmitx2p1_parse_dtd_in_ext_block(UINT32 ofs);
static  void    hdmitx2p1_parse_native_format(UINT32 blk_num);
static  UINT32  hdmitx2p1_parse_ext_block(UINT32 blk_num);
static  void    hdmitx2p1_parse_audio_std(UINT32 stdaddr, UINT32 stdreg);
static  void    hdmitx2p1_parse_video_std(UINT32 stdaddr, UINT32 stdreg);
static  void    hdmitx2p1_parse_std(UINT32 ofs);

static UINT8    hdmitx2p1_ddc_read_byte(UINT8 segm, UINT8 offset);
static ER       hdmitx2p1_ddc_read_byte2(UINT32 segm, UINT32 offset, UINT32 *pui_data);

#if 1
/*
    We check if the EDID header is correct sequence: {0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00}.
    The EDID parser would be stop if the header sequence check fail.
*/
static UINT32 hdmitx2p1_parse_edid_header(void)
{
	UINT8   i;

	// The EDID header: {0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00}
	if ((hdmitx2p1_ddc_read_byte(0x00, 0x00) == 0x00) && (hdmitx2p1_ddc_read_byte(0x00, 0x07) == 0x00)) {
		for (i = 1; i < 7; i++) {
			if (hdmitx2p1_ddc_read_byte(0x00, i) != 0xff) {
				hdmitx_edidmsg(("hdmitx2p1_Parse_EDIDHeader: Non-Legal EDID Header! FAIL!\r\n"));
				return 1;
			}
		}
	} else {
		hdmitx_edidmsg(("hdmitx2p1_Parse_EDIDHeader: Non-Legal EDID Header! FAIL!\r\n"));
		return 1;
	}

	// Get the Manufacturer-ID from the EDID header. This information can be used also in the CEC application.
	gui_manufacturer_id = (hdmitx2p1_ddc_read_byte(0x00, 0x08) << 8) + hdmitx2p1_ddc_read_byte(0x00, 0x09);

	hdmitx_edidmsg(("hdmitx2p1_Parse_EDIDHeader: Legal EDID Header! ManID=0x%04X\r\n", gui_manufacturer_id));
	return 0;

}

static UINT32 hdmitx2p1_parse_edid_version(void)
{
	// Check if the EDID version code is larger than V1.3. (hdmi compatible)
	// Assert fail if the version code is old.
	if ((hdmitx2p1_ddc_read_byte(0x00, 0x12) == 1) && (hdmitx2p1_ddc_read_byte(0x00, 0x13) >= 3)) {
		return 0;
	} else {
		return 1;
	}
}

/*
    Check the CheckSum value is correct for the certain EDID Block.
*/
static UINT32 hdmitx2p1_verify_edid_checksum(UINT32 blk_num)
{
	UINT32 ofs, i;
	UINT32 CRC = 0;

	ofs = 0x80 * (blk_num & 0x01);

	// Sum the first 127 Bytes of the specified EDID block.
	for (i = 0; i < 127; i++) {
		CRC += hdmitx2p1_ddc_read_byte((blk_num >> 1), i + ofs);
	}

	CRC &= 0xFF;
	CRC = 256 - CRC;
	CRC &= 0xFF;

	hdmitx_edidmsg(("CRC = 0x%02X\r\n", CRC));

	// Check if the CheckSum is correct with the 128th Byte.
	if (hdmitx2p1_ddc_read_byte((blk_num >> 1), ofs + 0x7F) != CRC) {
		return 1;
	} else {
		return 0;
	}

}

static void hdmitx2p1_parse_product_hotfix(UINT32 mrl)
{
	UINT8 name[13];
	int i = 0;

	for (i = 0; i < 13; i++) {
		name[i] = hdmitx2p1_ddc_read_byte(0x00, (mrl + 6 + i));
		/*0x0a CR, 0x20 space*/
		if ((name[i] == 0x0a) || (name[i] == 0x20)) {
			name[i] = 0x00;
		}
	}

	DBG_WRN("[2p1] parse_product_name = %s\r\n",name);
/*	
	g_hdmi_hotfix_id = -1;
	
	for (i = 0; i < HDMI_HOTFIXMAX; i++) {	
		if (memcmp((const void *)(&g_hdmi_hotfix_table[i].product_name), (const void *)(&name), strlen(g_hdmi_hotfix_table[i].product_name)) == 0) {
			DBG_WRN("[2p1] hdmi_hotfix_table %d parse match %s : %ld\r\n", i, (char *)&g_hdmi_hotfix_table[i].product_name[0], strlen(g_hdmi_hotfix_table[i].product_name));
			g_hdmi_hotfix_id = g_hdmi_hotfix_table[i].product_id;
			break;
		}
	}
*/	

}


// Parsing Monitor Range Limits Packet
// Please refer to the spec of the CEA-861 for details.
static void hdmitx2p1_parse_mrl(UINT32 mrl)
{
	UINT32 max_v_rate, max_h_rate, max_pix_clk;

	max_v_rate = hdmitx2p1_ddc_read_byte(0x00, mrl + 6); //mrl[6];
	max_h_rate = hdmitx2p1_ddc_read_byte(0x00, mrl + 8); //mrl[8];
	max_pix_clk = hdmitx2p1_ddc_read_byte(0x00, mrl + 9); //mrl[9];

	hdmitx_edidmsg(("Monitor Max-V-rate = %d hz\r\n", max_v_rate));
	hdmitx_edidmsg(("Monitor Max-H-rate = %d kHz\r\n", max_h_rate));
	hdmitx_edidmsg(("Monitor Max-PixClock = %d MHz\r\n", (max_pix_clk * 10)));

	// These information is checked for DVI device.
	// If the display pixel-clock/Max-H-rate/Max-V-Rate abilities are larger than 720P60/1080I60/1080P60.
	// Then we think the display can afford these input resolutions.
	if (gb_valid_resolution && (max_v_rate >= 60) && (max_h_rate >= 33) && ((max_pix_clk * 10) >= 80)) {
		gui_hdmi2p1_edid_info |= (HDMITX_DTD640X480P60 | HDMITX_DTD720X480P60 | HDMITX_DTD1080I60 | HDMITX_DTD720X480I60);

		if (max_h_rate >= 45) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD720P60;
		}

		if (((max_pix_clk * 10) >= 140) && (max_h_rate >= 67)) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD1080P60;
		}
	}

}

// Parsing Established Timing(ET) Manufacturer Timing(MT) 
static void hdmitx2p1_parse_etmt()
{
	UINT8 ET_1, ET_2, MT;
	UINT32 ui_addr,i=0;

	ui_addr = 0x23; // ET/MT timing ofs is 0x23/0x24/0x25

	hdmitx_edidmsg(("\r\n"));
	hdmitx_edidmsg(("==========================================\r\n"));
	hdmitx_edidmsg(("Parsing Established Timings 1\r\n"));
	ET_1 = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF));     
	hdmitx_edidmsg(("Established Timings 1 is 0x%x \r\n",ET_1));

	if(ET_1 & (1<<0))hdmitx_edidmsg((" 800 x 600 @60hz (VESA)\r\n")); g_svd_vid_extra_hdmi2p1[i] = HDMI_800X600P60;
	if(ET_1 & (1<<1))hdmitx_edidmsg((" 800 x 600 @56hz (VESA)\r\n")); g_svd_vid_extra_hdmi2p1[i] = HDMI_800X600P56; i++;
	if(ET_1 & (1<<2))hdmitx_edidmsg((" 640 x 480 @75hz (VESA)\r\n")); g_svd_vid_extra_hdmi2p1[i] = HDMI_640X480P75; i++;
	if(ET_1 & (1<<3))hdmitx_edidmsg((" 640 x 480 @72hz (VESA)\r\n")); g_svd_vid_extra_hdmi2p1[i] = HDMI_640X480P72; i++;
	if(ET_1 & (1<<4))hdmitx_edidmsg((" 640 x 480 @67hz (Apple, MacII)\r\n")); g_svd_vid_extra_hdmi2p1[i] = 0;
	if(ET_1 & (1<<5))hdmitx_edidmsg((" 640 x 480 @60hz (IBM, VGA)\r\n")); g_svd_vid_extra_hdmi2p1[i] = HDMI_640X480P60; i++;
	if(ET_1 & (1<<6))hdmitx_edidmsg((" 720 x 400 @88hz (IBM, XGA2)\r\n")); g_svd_vid_extra_hdmi2p1[i] = 0;
	if(ET_1 & (1<<7))hdmitx_edidmsg((" 720 x 400 @70hz (IBM, VGA)\r\n")); g_svd_vid_extra_hdmi2p1[i] = 0;
	hdmitx_edidmsg(("==========================================\r\n"));

	hdmitx_edidmsg(("\r\n"));
	hdmitx_edidmsg(("==========================================\r\n"));
	hdmitx_edidmsg(("Parsing Established Timings 2\r\n"));
	ET_2 = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF)+1);     
	hdmitx_edidmsg(("Established Timings 2 is 0x%x \r\n",ET_2));

	if(ET_2 & (1<<0))hdmitx_edidmsg((" 1280 x 1024 @75hz (VESA)\r\n")); g_svd_vid_extra_hdmi2p1[i] = 0;
	if(ET_2 & (1<<1))hdmitx_edidmsg((" 1024 x 768  @75hz (VESA)\r\n")); g_svd_vid_extra_hdmi2p1[i] = HDMI_1024X768P75; i++;
	if(ET_2 & (1<<2))hdmitx_edidmsg((" 1024 x 768  @70hz (VESA)\r\n")); g_svd_vid_extra_hdmi2p1[i] = HDMI_1024X768P70; i++;
	if(ET_2 & (1<<3))hdmitx_edidmsg((" 1024 x 768  @60hz (VESA)\r\n")); g_svd_vid_extra_hdmi2p1[i] = HDMI_1024X768P60; i++;
	if(ET_2 & (1<<4))hdmitx_edidmsg((" 1024 x 768  @87hz (IBM)\r\n")); g_svd_vid_extra_hdmi2p1[i] = 0;
	if(ET_2 & (1<<5))hdmitx_edidmsg(("  832 x 624  @75hz (Apple, MacII)\r\n")); g_svd_vid_extra_hdmi2p1[i] = 0;
	if(ET_2 & (1<<6))hdmitx_edidmsg(("  800 x 600  @75hz (VESA)\r\n")); g_svd_vid_extra_hdmi2p1[i] = HDMI_800X600P75; i++;
	if(ET_2 & (1<<7))hdmitx_edidmsg(("  800 x 600  @72hz (VESA)\r\n"));	g_svd_vid_extra_hdmi2p1[i] = HDMI_800X600P72; i++;
	hdmitx_edidmsg(("==========================================\r\n"));
	
	hdmitx_edidmsg(("\r\n"));
	hdmitx_edidmsg(("==========================================\r\n"));
	hdmitx_edidmsg(("Parsing Manufacturer Timings \r\n"));
	MT = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF)+2);     
	hdmitx_edidmsg(("Manufacturer Timings is 0x%x \r\n",MT));	

	if(MT & (1<<7))hdmitx_edidmsg(("  1152 x 870  @75hz (Apple, MacII)\r\n")); g_svd_vid_extra_hdmi2p1[16] = 0;
	hdmitx_edidmsg(("==========================================\r\n"));
	hdmitx_edidmsg(("\r\n"));

}

// Parsing Standard Timing Identification (STI)
static void hdmitx2p1_parse_sti()
{
	UINT8 STI_B1,STI_B2;
	UINT32 ui_addr,i,x,y,ratio_x,ratio_y,hz,j;

	ui_addr = 0x26; // STI timing ofs is 0x26~0x35

	for(j=0;j<HDMITX_EXTRA_NUM;j++){
		if(g_svd_vid_extra_hdmi2p1[j]==0) {
			//hdmitx_edidmsg(("j = %d\r\n",j));
			break; // find idx for un-used id
		}
	}

	hdmitx_edidmsg(("\r\n"));
	hdmitx_edidmsg(("==========================================\r\n"));
	hdmitx_edidmsg(("Parsing Standard Timing Identification\r\n"));

	for(i=0;i<8;i++){

		STI_B1 = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF)+i*2); 
		STI_B2 = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF)+i*2+1); 

		if (STI_B1 == 0x01 && STI_B2 == 0x01)
			continue;

		if (STI_B1 == 0) {
			hdmitx_edidmsg(("invalid standard timing (0 horiz)\n"));
			continue;
		}

		x = (STI_B1 + 31) * 8;
		switch ((STI_B2 >> 6) & 0x3) {
		case 0x00:
			y = x * 10 / 16;
			ratio_x = 16;
			ratio_y = 10;
			break;
		case 0x01:
			y = x * 3 / 4;
			ratio_x = 4;
			ratio_y = 3;
			break;
		case 0x02:
			y = x * 4 / 5;
			ratio_x = 5;
			ratio_y = 4;
			break;
		case 0x03:
			y = x * 9 / 16;
			ratio_x = 16;
			ratio_y = 9;
			break;
		}
		hz = 60 + (STI_B2 & 0x3f);

		if((x == 1280) && (y == 1024) && (hz == 60)){ g_svd_vid_extra_hdmi2p1[j]=HDMI_1280X1024P60; j++;}
		if((x == 1600) && (y == 1200) && (hz == 60)){ g_svd_vid_extra_hdmi2p1[j]=HDMI_1600X1200P60; j++;}
		if((x == 2560) && (y == 1440) && (hz == 60)){ g_svd_vid_extra_hdmi2p1[j]=HDMI_2560X1440P60; j++;}
		if((x == 1440) && (y ==  900) && (hz == 60)){ g_svd_vid_extra_hdmi2p1[j]=HDMI_1440X900P60; j++;}
		if((x == 1680) && (y == 1050) && (hz == 60)){ g_svd_vid_extra_hdmi2p1[j]=HDMI_1680X1050P60; j++;}
		if((x == 1920) && (y == 1200) && (hz == 60)){ g_svd_vid_extra_hdmi2p1[j]=HDMI_1920X1200P60; j++;}
		if((x == 1280) && (y == 1024) && (hz == 75)){ g_svd_vid_extra_hdmi2p1[j]=HDMI_1280X1024P75; j++;}
		if((x == 1400) && (y == 1050) && (hz == 60)){ g_svd_vid_extra_hdmi2p1[j]=HDMI_1400X1050P60; j++;}
		if((x == 1280) && (y ==  800) && (hz == 60)){ g_svd_vid_extra_hdmi2p1[j]=HDMI_1280X800P60; j++;}
		

		hdmitx_edidmsg((" %d x %d @ %dHz (%d : %d Aspect Ratio)\r\n",x,y,hz,ratio_x,ratio_y));
	
	}
	hdmitx_edidmsg(("==========================================\r\n"));	
	hdmitx_edidmsg(("\r\n"));	
}


/*
    Parse the EDID Detailed-Timing-Descriptor (DTD).
    The EDID may contain several DTDs in the EDID,
    these information can help the determine the display abilities and timings.
    The detailed information of the DTD data struture is listed in the spec CEA-861.
*/
static void hdmitx2p1_parse_dtd(UINT32 ui_addr)
{
	UINT32  temp;
	UINT32  temp1, temp2;
	UINT32  h_resolution, v_resolution, h_blanking, v_blanking, h_sync_ofs, h_sync_width, v_sync_ofs, v_sync_width; 
	UINT32  h_image_size,v_image_size, h_border, v_border;
	UINT32  pixel_clk, v_total;
	UINT32   hz,i,k=0,i_mode;


	temp = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 1);            //DTD[1];
	temp = temp << 8;
	pixel_clk = temp + hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 0); //DTD[0];

	temp = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 4);            //DTD[4];
	temp = (temp << 4) & 0x0f00;
	h_resolution = temp + hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 2); //DTD[2];

	temp = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 4);            //DTD[4];
	temp = (temp << 8) & 0x0f00;
	h_blanking = temp + hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 3); //DTD[3];

	temp = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 7);            //DTD[7];
	temp = (temp << 4) & 0x0f00;
	v_resolution = temp + hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 5); //DTD[5];

	temp = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 7);            //DTD[7];
	temp = (temp << 8) & 0x0f00;
	v_blanking = temp + hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 6); //DTD[6];

	temp = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 11);           //DTD[11];
	temp = (temp << 2) & 0x0300;
	h_sync_ofs = temp + hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 8);     //DTD[8];

	temp = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 11);           //DTD[11];
	temp = (temp << 4) & 0x0300;
	h_sync_width = temp + hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 9);     //DTD[9];

	temp = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 11);           //DTD[11];
	temp = (temp << 2) & 0x0030;
	v_sync_ofs = temp + (hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 10) >> 4); //DTD[10]

	temp = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 11);           //DTD[11];
	temp = (temp << 4) & 0x0030;
	v_sync_width = temp + (hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 8) & 0x0f); //DTD[8]

	temp = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 14);           //DTD[14];
	h_image_size = (temp << 4) & 0x0f00;
	//H_image_size = temp + hdmitx2p1_ddc_read_byte(ui_addr>>8, (ui_addr&0xFF)+12);  //DTD[12];

	temp = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 14);           //DTD[14];
	v_image_size = (temp << 8) & 0x0f00;
	//V_image_size = temp + hdmitx2p1_ddc_read_byte(ui_addr>>8, (ui_addr&0xFF)+13);  //DTD[13];

	h_border = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 15);
	v_border = hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 16);

	
	hdmitx_edidmsg(("-------------------------------------------------\r\n"));
	hdmitx_edidmsg(("Detailed timing  : Clock %d Hz, %d mm x %d mm \r\n",pixel_clk*10000, h_image_size, v_image_size));
	hdmitx_edidmsg(("H/V-active       : %d/%d\r\n",h_resolution, v_resolution));
	hdmitx_edidmsg(("H/V-blanking     : %d/%d\r\n",h_blanking, v_blanking));
	hdmitx_edidmsg(("H/V-sync-width   : %d/%d\r\n",h_sync_width, v_sync_width));
	hdmitx_edidmsg(("H/V-sync-ofs     : %d/%d\r\n",h_sync_ofs, v_sync_ofs));
	hdmitx_edidmsg(("H/V-border       : %d/%d\r\n",h_border, v_border));



	if (hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 17) & 0x80) {
		//DTD[17]
		i_mode = 1;
		hdmitx_edidmsg(("Interlaced Mode\r\n"));
	} else {
		i_mode = 0;
		hdmitx_edidmsg(("Non-interlaced Mode\r\n"));
	}

	/*
	if(DTD[17] & 0x60)
	    hdmitx_edidmsg(("Table 3.17 for defenition\r\n "));
	else
	    hdmitx_edidmsg(("Normal Display\n\r "));
	*/

	if (hdmitx2p1_ddc_read_byte(ui_addr >> 8, (ui_addr & 0xFF) + 17) & 0x10) {
		//DTD[17]
		hdmitx_edidmsg(("Digital TV\r\n"));
	} else {
		hdmitx_edidmsg(("Analog TV\r\n"));
	}

	temp1 = h_resolution + h_blanking;
	v_total = v_resolution + v_blanking;
	temp1 = temp1 * v_total;
	temp2 = pixel_clk;
	temp2 = temp2 * 10000;

	//hz = Pixel-Clock / (H-total*V-total);
	if (temp1 == 0) {
		hz = 0;
	} else {
		hz = temp2 / temp1;
	}

	//Let 720P can be outputed during DVI connection, we check 1280X720P60/1920x1080I60/1920x1080P60.
	if ((v_resolution >= 720) && (h_resolution >= 1280)) {
		gb_valid_resolution = TRUE;
	}

	if ((hz == 59) || (hz == 60)) {
		#if HDMITX_DEBUG
		hz = 60;
		#endif

		if (v_resolution == 540) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD1080I60;
			hdmitx_edidmsg(("Support 1080I60\r\n"));
		} else if (v_resolution == 1080) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD1080P60;
			hdmitx_edidmsg(("Support 1080P60\r\n"));
		} else if (v_resolution == 720) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD720P60;
			hdmitx_edidmsg(("Support 720P60\r\n"));
		} else if ((h_resolution == 640) && (v_resolution == 480)) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD640X480P60;
			hdmitx_edidmsg(("Support 640x480P60\r\n"));
		} else if ((h_resolution == 720) && (v_resolution == 480)) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD720X480P60;
			hdmitx_edidmsg(("Support 720x480P60\r\n"));
		} else if ((h_resolution == 720) && (v_resolution == 240)) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD720X480I60;
			hdmitx_edidmsg(("Support 720x480I60\r\n"));
		} else if ((v_resolution == 2160) && (pixel_clk >= 29700) && (h_resolution >= 4096)) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD4096X2160P30;
			hdmitx_edidmsg(("Support 4096x2160P60\r\n"));
		} else if ((v_resolution == 2160) && (pixel_clk >= 29700) && (h_resolution >= 3840)) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD3840X2160P30;
			hdmitx_edidmsg(("Support 3840x2160P60\r\n"));
		} else {
			hdmitx_edidmsg(("Non-Supported Format: frame rate=(%d), H-res=(%d), V-res=(%d) \r\n", hz, h_resolution, v_resolution));
		}

	} else if (hz == 50) {
		if (v_resolution == 540) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD1080I50;
			hdmitx_edidmsg(("Support 1080I50\r\n"));
		} else if (v_resolution == 1080) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD1080P50;
			hdmitx_edidmsg(("Support 1080P50\r\n"));
		} else if (v_resolution == 720) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD720P50;
			hdmitx_edidmsg(("Support 720P50\r\n"));
		} else if (v_resolution == 576) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD576P50;
			hdmitx_edidmsg(("Support 576P50\r\n"));
		} else if (v_resolution == 288) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD576I50;
			hdmitx_edidmsg(("Support 576I50\r\n"));
		} else if ((v_resolution == 2160) && (pixel_clk >= 29700) && (h_resolution >= 4096)) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD4096X2160P30;
			hdmitx_edidmsg(("Support 4096x2160P30\r\n"));
		} else if ((v_resolution == 2160) && (pixel_clk >= 29700) && (h_resolution >= 3840)) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD3840X2160P30;
			hdmitx_edidmsg(("Support 3840x2160P30\r\n"));
		} else {
			hdmitx_edidmsg(("Non-Supported Format: frame rate=(%d), H-res=(%d), V-res=(%d) \r\n", hz, h_resolution, v_resolution));
		}
	} else if (hz == 30) {
		if ((v_resolution == 2160) && (pixel_clk >= 29700) && (h_resolution >= 4096)) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD4096X2160P30;
			hdmitx_edidmsg(("Support 4096x2160P30\r\n"));
		} else if ((v_resolution == 2160) && (pixel_clk >= 29700) && (h_resolution >= 3840)) {
			gui_hdmi2p1_edid_info |= HDMITX_DTD3840X2160P30;
			hdmitx_edidmsg(("Support 3840x2160P30\r\n"));
		} else {
			hdmitx_edidmsg(("Non-Supported Format: frame rate=(%d), H-res=(%d), V-res=(%d) \r\n", hz, h_resolution, v_resolution));
		}
	} else {
		DBG_WRN("[2p1] Non-Supported Frame Rate (%d)hz \r\n", hz);
	}

	/*parsing recommend vid
	if(ui_addr == 0x36){ // fist DTD is recommend vid
		hdmitx_edidmsg(("Find recommend Vid in first DTD\r\n"));
		hdmitx_edidmsg(("hz=%d h_resolution=%d v_resolution=%d i_mode=%d\r\n",hz,h_resolution,v_resolution,i_mode));
		for(i=0;i<MAX_ID_CAP;i++){

			if((disp_video_id[i].hz == hz) && 
				(disp_video_id[i].v_active == v_resolution) &&
				(disp_video_id[i].h_active == h_resolution) &&
				(disp_video_id[i].interalce == i_mode)){

				if(k<2){//only 2 recommend vid
					g_recommend_vid_hdmi2p1[k] = disp_video_id[i].video_id;
					hdmitx_edidmsg(("Find recommend Vid = %d\r\n",g_recommend_vid_hdmi2p1[k]));
					k++;
				}
			}
		}
	}*/

	hdmitx_edidmsg(("-------------------------------------------------\r\n\r\n"));

}

/*
    Parse the DTD information the EDID first Block.
*/
static void hdmitx2p1_parse_dtd_in_block_one(void)
{

	hdmitx_edidmsg(("\n\rParse the first DTD in Block one:\n\r"));

	// The first DTD is fixed at offset 0x36.
	hdmitx2p1_parse_dtd(0x36);

	// Check if the second DTD is existed or not.
	hdmitx_edidmsg(("\n\rParse the Second DTD in Block one:\n\r"));
	if ((hdmitx2p1_ddc_read_byte(0x00, 0x48) == 0) && (hdmitx2p1_ddc_read_byte(0x00, 0x49) == 0) && (hdmitx2p1_ddc_read_byte(0x00, 0x4A) == 0)) {
		if (hdmitx2p1_ddc_read_byte(0x00, 0x4B) == 0xFD) {
			hdmitx_edidmsg(("Paring Monitor Range Limits Packets! -->\n\r"));
			hdmitx2p1_parse_mrl(0x48);
		} else {
			hdmitx_edidmsg(("The second DTD in Block one is not used to descript video timing.\n\r"));
		}
		
		if (hdmitx2p1_ddc_read_byte(0x00, 0x4B) == 0xFC) {
			hdmitx_edidmsg(("Paring Monitor product name! -->\n\r"));
			hdmitx2p1_parse_product_hotfix(0x48);
		}		
		
	} else {
		// Parse the second DTD in the first EDID Block.
		hdmitx2p1_parse_dtd(0x48);
	}

	// Check if the third DTD is existed or not.
	hdmitx_edidmsg(("\n\rParse the Third DTD in Block one:\n\r"));
	if ((hdmitx2p1_ddc_read_byte(0x00, 0x5A) == 0) && (hdmitx2p1_ddc_read_byte(0x00, 0x5B) == 0) && (hdmitx2p1_ddc_read_byte(0x00, 0x5C) == 0)) {
		if (hdmitx2p1_ddc_read_byte(0x00, 0x5D) == 0xFD) {
			hdmitx_edidmsg(("Paring Monitor Range Limits Packets! -->\n\r"));
			hdmitx2p1_parse_mrl(0x5A);
		} else {
			hdmitx_edidmsg(("The third DTD in Block one is not used to descript video timing.\n\r"));
		}

		if (hdmitx2p1_ddc_read_byte(0x00, 0x5D) == 0xFC) {
			hdmitx_edidmsg(("Paring Monitor product name! -->\n\r"));
			hdmitx2p1_parse_product_hotfix(0x5A);
		}
		
	} else {
		// Parse the third DTD in the first EDID Block.
		hdmitx2p1_parse_dtd(0x5A);
	}

	hdmitx_edidmsg(("\n\rParse the Fourth DTD in Block one:\n\r"));
	if ((hdmitx2p1_ddc_read_byte(0x00, 0x6C) == 0) && (hdmitx2p1_ddc_read_byte(0x00, 0x6D) == 0) && (hdmitx2p1_ddc_read_byte(0x00, 0x6E) == 0)) {
		if (hdmitx2p1_ddc_read_byte(0x00, 0x6F) == 0xFD) {
			hdmitx_edidmsg(("Paring Monitor Range Limits Packets! -->\n\r"));
			hdmitx2p1_parse_mrl(0x6C);
		} else {
			hdmitx_edidmsg(("The fourth DTD in Block one is not used to descript video timing.\n\r"));
		}

		if (hdmitx2p1_ddc_read_byte(0x00, 0x6F) == 0xFC) {
			hdmitx_edidmsg(("Paring Monitor product name! -->\n\r"));
			hdmitx2p1_parse_product_hotfix(0x6C);
		}
		
	} else {
		hdmitx2p1_parse_dtd(0x6C);
	}

}


static void hdmitx2p1_parse_dtd_in_ext_block(UINT32 ofs)
{
	UINT32 dtd_begin_addr;

	dtd_begin_addr = hdmitx2p1_ddc_read_byte((ofs >> 8), 0x02 + (ofs & 0xFF)) + ofs;

	while (dtd_begin_addr < (0x6c + ofs)) {
		if ((hdmitx2p1_ddc_read_byte((dtd_begin_addr >> 8), (dtd_begin_addr) & 0xFF) == 0) && (hdmitx2p1_ddc_read_byte((dtd_begin_addr >> 8), (dtd_begin_addr + 1) & 0xFF) == 0) && (hdmitx2p1_ddc_read_byte((dtd_begin_addr >> 8), (dtd_begin_addr + 2) & 0xFF) == 0)) {
			hdmitx_edidmsg(("This DTD in Extension Block is not used to descript video timing.(%d)\n\r", dtd_begin_addr));
		} else {
			hdmitx_edidmsg(("\n\rParse the DTD in Extension Block :\n\r"));
			hdmitx2p1_parse_dtd(dtd_begin_addr);
		}

		dtd_begin_addr = dtd_begin_addr + 18;
	}
}


static void hdmitx2p1_parse_native_format(UINT32 blk_num)
{
	UINT8   temp;
	UINT32  ofs;

	ofs = 0x80 * (blk_num & 0x01);

	temp = hdmitx2p1_ddc_read_byte((blk_num >> 1), 0x03 + ofs) & 0xf0;

	if (temp & 0x80) {
		gui_hdmi2p1_edid_info |= HDMITX_TV_UNDERSCAN;
		hdmitx_edidmsg(("DTV supports underscan.\n\r"));
	}
	if (temp & 0x40) {
		hdmitx_edidmsg(("DTV supports BasicAudio.\n\r"));
	}

	if (temp & 0x20) {
#if HDMITX_DEBUG
	#if !HDMITX_DEBUG_FORCE_YUV422
		gui_hdmi2p1_edid_info |= HDMITX_TV_YCBCR444;
	#endif
#else
		gui_hdmi2p1_edid_info |= HDMITX_TV_YCBCR444;
#endif
		hdmitx_edidmsg(("DTV supports YCbCr 4:4:4.\r\n"));
	}
	if (temp & 0x10) {
		gui_hdmi2p1_edid_info |= HDMITX_TV_YCBCR422;
		hdmitx_edidmsg(("DTV supports YCbCr 4:2:2.\r\n"));
	}

}


static UINT32 hdmitx2p1_parse_ext_block(UINT32 blk_num)
{
	UINT8   c;
	UINT32  ofs;
	UINT8   a0, a1;
	UINT8   size;
	UINT8   i, j;
	UINT8   only420_vic;

	ofs = 0x80 * blk_num;

	{
		c = hdmitx2p1_ddc_read_byte((ofs >> 8), ofs & 0xFF);
		if (c == 0x02) {
			hdmitx2p1_parse_dtd_in_ext_block(ofs);
			hdmitx2p1_parse_std(ofs);
		} else {
			DBG_ERR("[2p1] Extension block tag code error!\r\n");
			return HDMITX_ER_TAGCODE;
		}
	}

#if 1
	{
		for (i = 0; i < (128-1);) {
			a0 = hdmitx2p1_ddc_read_byte((ofs >> 8), (ofs + i) & 0xFF);
			i++;
			if ((a0 & 0xE0) == 0xE0) {
				a1 = hdmitx2p1_ddc_read_byte((ofs >> 8), (ofs + i) & 0xFF);
				i++;
				if (((a1 & 0xFF) == 0x0E)) {
					size = (a0 & 0x1F) - 1;
					for (j = 0; j < size; j++) {
						only420_vic = hdmitx2p1_ddc_read_byte((ofs >> 8), (ofs + i) & 0xFF);
						hdmitx_edidmsg(("420 VIC = %d\n", only420_vic));
						g_svd_vid_hdmi2p1[g_svd_vid_cnt_hdmi2p1+j] = only420_vic;
						i++;
					}
					if (size > 0) {
						g_svd_vid_hdmi2p1[HDMITX_EDIDNUM] = g_svd_vid_hdmi2p1[HDMITX_EDIDNUM]+size;
						break;
					}
				}
			}
		}
	}
	hdmitx_edidmsg(("hdmitx2p1_parse_ext_block, g_svd_vid_hdmi2p1[HDMITX_EDIDNUM] = %d\n", g_svd_vid_hdmi2p1[HDMITX_EDIDNUM]));
#endif
	return E_OK;
}

static void hdmitx2p1_parse_audio_std(UINT32 stdaddr, UINT32 stdreg)
{
	UINT32 i, audio_format, std_reg_tmp, std_addr_tmp;
	UINT8 temp;

	std_reg_tmp  = stdreg & 0x1f;
	std_addr_tmp = stdaddr + 1;
	i           = 0;

	while (i < std_reg_tmp) {
		audio_format = (hdmitx2p1_ddc_read_byte(std_addr_tmp >> 8, std_addr_tmp & 0xFF) & 0x78) >> 3;

		temp = hdmitx2p1_ddc_read_byte((std_addr_tmp + 1) >> 8, (std_addr_tmp + 1) & 0xFF);

		if (temp & 0x01) {
			hdmitx_edidmsg(("TV Audio Support 32Khz sampling rate! \r\n"));
			gui_hdmi2p1_edid_info |= HDMITX_ADO_32KHZ;
		}
		if (temp & 0x02) {
			hdmitx_edidmsg(("TV Audio Support 44.1Khz sampling rate! \r\n"));
			gui_hdmi2p1_edid_info |= HDMITX_ADO_44KHZ;
		}
		if (temp & 0x04) {
			hdmitx_edidmsg(("TV Audio Support 48Khz sampling rate! \r\n"));
			gui_hdmi2p1_edid_info |= HDMITX_ADO_48KHZ;
		}
		if (temp & 0x78) {
			hdmitx_edidmsg(("TV Audio Support sampling rate over 48Khz! \r\n"));
			gui_hdmi2p1_edid_info |= HDMITX_ADO_MORE;
		}

		if (audio_format == 1) {

			temp = hdmitx2p1_ddc_read_byte((std_addr_tmp + 2) >> 8, (std_addr_tmp + 2) & 0xFF);

			if (temp & 0x01) {
				hdmitx_edidmsg(("TV Audio support 16bits \r\n"));
			}
			if (temp & 0x02) {
				hdmitx_edidmsg(("TV Audio support 20bits \r\n"));
			}
			if (temp & 0x04) {
				hdmitx_edidmsg(("TV Audio support 24bits \r\n"));
			}
		} else {
			hdmitx_edidmsg(("Un-Known TV Audio Format Code! \r\n"));
		}

		i = i + 3;
		std_addr_tmp = std_addr_tmp + 3;
	}
}

static void hdmitx2p1_parse_video_std(UINT32 stdaddr, UINT32 stdreg)
{
	UINT32 i, std_reg_tmp, std_addr_tmp;


	std_reg_tmp  = stdreg & 0x1f;
	g_svd_vid_cnt_hdmi2p1 = std_reg_tmp;
	std_addr_tmp = stdaddr + 1;
	i           = 0;

	while (i < std_reg_tmp) {
		g_svd_vid_hdmi2p1[i] = hdmitx2p1_ddc_read_byte(std_addr_tmp >> 8, std_addr_tmp & 0xFF) & 0x7F;
		hdmitx_edidmsg(("Supported Video ID:  (%d)\r\n", g_svd_vid_hdmi2p1[i]));
		i = i + 1;
		std_addr_tmp = std_addr_tmp + 1;
	}

	g_svd_vid_hdmi2p1[HDMITX_EDIDNUM] = std_reg_tmp;
}

static void hdmitx2p1_parse_vendor_std(UINT32 stdaddr, UINT32 stdreg)
{
	UINT32  std_reg_tmp, std_addr_tmp, HDMIVICLEN_LOC;	
	UINT8   temp, temp2;
	UINT32  ieee_id;
	UINT32  HDMI_VIC_LEN;
#ifdef HDMI_CEC_EN
	UINT32  phy_addr = 0x00;
#endif

	std_reg_tmp  = stdreg & 0x1f;
	std_addr_tmp = stdaddr;


	ieee_id = (UINT32)hdmitx2p1_ddc_read_byte((std_addr_tmp + 1) >> 8, (std_addr_tmp + 1) & 0xFF) + ((UINT32)hdmitx2p1_ddc_read_byte((std_addr_tmp + 2) >> 8, (std_addr_tmp + 2) & 0xFF) << 8) + ((UINT32)hdmitx2p1_ddc_read_byte((std_addr_tmp + 3) >> 8, (std_addr_tmp + 3) & 0xFF) << 16);

	if ((ieee_id == 0x000C03) || (ieee_id == 0xC45DD8)) {
		hdmitx_edidmsg(("TV is HDMI Device!! \r\n"));
		gui_hdmi2p1_edid_info |= HDMITX_TV_HDMI;
	}

	if (ieee_id == 0x000C03) {
#ifdef HDMI_CEC_EN
		phy_addr  =  hdmitx2p1_ddc_read_byte((std_addr_tmp + 5) >> 8, (std_addr_tmp + 5) & 0xFF);
		phy_addr |= (hdmitx2p1_ddc_read_byte((std_addr_tmp + 4) >> 8, (std_addr_tmp + 4) & 0xFF) << 8);

		// Set CEC Physical Address from VSDB
		hdmicec_set_config(CEC_CONFIG_ID_PHY_ADDR, phy_addr);
#endif

#if HDMITX_DEBUG
		if (std_reg_tmp >= 6) {
			temp = hdmitx2p1_ddc_read_byte((std_addr_tmp + 6) >> 8, (std_addr_tmp + 6) & 0xFF);

			if (temp & 0x78) {
				hdmitx_edidmsg(("HDMI Device supports Deep Color!!! flag= 0x%X \r\n", (hdmitx2p1_ddc_read_byte((std_addr_tmp + 6) >> 8, (std_addr_tmp + 6) & 0xFF) & 0x78)));
			}

			if (temp & 0x80) {
				hdmitx_edidmsg(("HDMI Support_AI bit is SET!!!  \r\n"));
			}
		}
#endif

		if (std_reg_tmp >= 8) {
			UINT32  video_start_ofs, video_start_base;
			UINT8   HDMI_3D_LEN, k;

			video_start_base = (std_addr_tmp + 8);
			temp = hdmitx2p1_ddc_read_byte(video_start_base >> 8, video_start_base & 0xFF);

			//HDMI_Video_present
			if (temp & 0x20) {
				if (temp & 0x80) {
					if (temp & 0x40) {
						video_start_ofs = 5;
						HDMIVICLEN_LOC = 14;
					} else {
						video_start_ofs = 3;
						HDMIVICLEN_LOC = 12;
					}
				} else {
					video_start_ofs  = 1;
					HDMIVICLEN_LOC = 10;
				}

				temp = hdmitx2p1_ddc_read_byte((video_start_base + video_start_ofs) >> 8, (video_start_base + video_start_ofs) & 0xFF);
				//3D_present
				if (temp & 0x80) {
					g_svd_3d_cap_hdmi2p1[HDMITX_EDIDNUM] = 1;
				} else {
					g_svd_3d_cap_hdmi2p1[HDMITX_EDIDNUM] = 0;
				}

				switch (temp & 0x60) {
				// 3D_Multi_present = 01
				case 0x20: {
				video_start_ofs += 1;
				temp = hdmitx2p1_ddc_read_byte((video_start_base + video_start_ofs) >> 8, (video_start_base + video_start_ofs) & 0xFF);

				video_start_ofs += ((temp >> 5) + 1);
				HDMI_3D_LEN = (temp & 0x1F); //2
				// 2 is 3D_Struc_All
				if (HDMI_3D_LEN >= 2) {
					HDMI_3D_LEN -= 2;
				}

				temp = hdmitx2p1_ddc_read_byte((video_start_base + video_start_ofs) >> 8, (video_start_base + video_start_ofs) & 0xFF);
				if (temp & 0x1) {
					for (k = 0; k < 16; k++) {
						g_svd_3d_cap_hdmi2p1[k] |= HDMI3D_FRAMEPACKING;
					}
				}

				if (temp & 0x40) {
					for (k = 0; k < 16; k++) {
						g_svd_3d_cap_hdmi2p1[k] |= HDMI3D_TOPANDBOTTOM;
					}
				}

				video_start_ofs += 1;
				temp = hdmitx2p1_ddc_read_byte((video_start_base + video_start_ofs) >> 8, (video_start_base + video_start_ofs) & 0xFF);
				if (temp & 0x01) {
					for (k = 0; k < 16; k++) {
						g_svd_3d_cap_hdmi2p1[k] |= HDMI3D_SIDEBYSIDE_HALF;
					}
				}

				video_start_ofs += 1;
				}
				break;

				case 0x40: { // 3D_Multi_present = 10
				UINT16 MASK_3D = 0;

				video_start_ofs += 1;
				temp = hdmitx2p1_ddc_read_byte((video_start_base + video_start_ofs) >> 8, (video_start_base + video_start_ofs) & 0xFF);


				video_start_ofs += ((temp >> 5) + 1);
				HDMI_3D_LEN = (temp & 0x1F); //2
				// 4 is 3D_Struc_All & 3D_Mask
				if (HDMI_3D_LEN >= 4) {
					HDMI_3D_LEN -= 4;
				}


				MASK_3D = (hdmitx2p1_ddc_read_byte((video_start_base + video_start_ofs + 2) >> 8, (video_start_base + video_start_ofs + 2) & 0xFF) << 8);
				MASK_3D += hdmitx2p1_ddc_read_byte((video_start_base + video_start_ofs + 3) >> 8, (video_start_base + video_start_ofs + 3) & 0xFF);


				temp = hdmitx2p1_ddc_read_byte((video_start_base + video_start_ofs) >> 8, (video_start_base + video_start_ofs) & 0xFF);
				if (temp & 0x1) {
					for (k = 0; k < 16; k++) {
					if (MASK_3D & (0x1 << k)) {
						g_svd_3d_cap_hdmi2p1[k] |= HDMI3D_FRAMEPACKING;
					}
					}
				}

				if (temp & 0x40) {
					for (k = 0; k < 16; k++) {
					if (MASK_3D & (0x1 << k)) {
						g_svd_3d_cap_hdmi2p1[k] |= HDMI3D_TOPANDBOTTOM;
					}
					}
				}

				video_start_ofs += 1;
				temp = hdmitx2p1_ddc_read_byte((video_start_base + video_start_ofs) >> 8, (video_start_base + video_start_ofs) & 0xFF);
				if (temp & 0x01) {
					for (k = 0; k < 16; k++) {
					if (MASK_3D & (0x1 << k)) {
						g_svd_3d_cap_hdmi2p1[k] |= HDMI3D_SIDEBYSIDE_HALF;
					}
					}
				}
				video_start_ofs += 3;
			}
			break;

			//case 0x00:
			//case 0x60:
			default : {
			video_start_ofs += 1;
			temp = hdmitx2p1_ddc_read_byte((video_start_base + video_start_ofs) >> 8, (video_start_base + video_start_ofs) & 0xFF);

			video_start_ofs += ((temp >> 5) + 1);
			HDMI_3D_LEN = (temp & 0x1F); //2
			}
			break;

			}

			hdmitx_edidmsg(("HDMI_3D_LEN = %d   video_start_ofs= %d\r\n", HDMI_3D_LEN, video_start_ofs));

			temp2 = video_start_ofs;
			while (video_start_ofs < (HDMI_3D_LEN + temp2)) {
				temp = hdmitx2p1_ddc_read_byte((video_start_base + video_start_ofs) >> 8, (video_start_base + video_start_ofs) & 0xFF);

				if (((temp & 0xF) == 0x0) || ((temp & 0xF) == 0x6) || ((temp & 0xF) == 0x8)) {
					// Frame Packing
					if ((temp & 0xF) == 0x0) {
						g_svd_3d_cap_hdmi2p1[(temp >> 4)] |= HDMI3D_FRAMEPACKING;
						video_start_ofs++;
					}

					// Top and Bottom
					if ((temp & 0xF) == 0x6) {
						g_svd_3d_cap_hdmi2p1[(temp >> 4)] |= HDMI3D_TOPANDBOTTOM;
						video_start_ofs++;
					}

					// Side by Side Half
					if ((temp & 0xF) == 0x8) {
						g_svd_3d_cap_hdmi2p1[(temp >> 4)] |= HDMI3D_SIDEBYSIDE_HALF;
						video_start_ofs += 2;
					}
				} else {
					hdmitx_edidmsg(("HDMI illegal 3D Parser Type = 0x%02X\r\n", (temp & 0xF)));
					video_start_ofs++;
					//return;
				}
			}

			if (std_reg_tmp >= HDMIVICLEN_LOC) {
				temp = hdmitx2p1_ddc_read_byte((std_addr_tmp + HDMIVICLEN_LOC) >> 8, (std_addr_tmp + HDMIVICLEN_LOC) & 0xFF);
				HDMI_VIC_LEN = (temp>>5)&0x7;
				hdmitx_edidmsg(("GOT HDMI14B HDMI_VIC LEN = %d\r\n", HDMI_VIC_LEN));
				for (k = 0; k < HDMI_VIC_LEN; k++) {
					g_svd_hdmi2p1_vic[k] = hdmitx2p1_ddc_read_byte(((std_addr_tmp + HDMIVICLEN_LOC)+k+1) >> 8, ((std_addr_tmp + HDMIVICLEN_LOC)+k+1) & 0xFF);;
					hdmitx_edidmsg(("HDMI14B HDMI_VIC = %d\r\n", g_svd_hdmi2p1_vic[k]));
				}
			}

			}


		}
	} else if (ieee_id == 0xC45DD8) {
		gui_vsdb_h20cap_hdmi2p1  = (UINT32)hdmitx2p1_ddc_read_byte((std_addr_tmp + 4) >> 8, (std_addr_tmp + 4) & 0xFF);
		gui_vsdb_h20cap_hdmi2p1 += (UINT32)hdmitx2p1_ddc_read_byte((std_addr_tmp + 5) >> 8, (std_addr_tmp + 5) & 0xFF) << 8;
		gui_vsdb_h20cap_hdmi2p1 += (UINT32)hdmitx2p1_ddc_read_byte((std_addr_tmp + 6) >> 8, (std_addr_tmp + 6) & 0xFF) << 16;
		gui_vsdb_h20cap_hdmi2p1 += (UINT32)hdmitx2p1_ddc_read_byte((std_addr_tmp + 7) >> 8, (std_addr_tmp + 7) & 0xFF) << 24;
		hdmitx_edidmsg(("HDMI 2.0 VSDB Cap=0x%08X\r\n", gui_vsdb_h20cap_hdmi2p1));
	}

}


static void hdmitx2p1_parse_std(UINT32 ofs)
{
	UINT32  dtd_begin_addr;
	UINT32  stdaddr, stdreg;

	stdaddr = 0x04 + ofs;

	dtd_begin_addr = hdmitx2p1_ddc_read_byte((ofs >> 8), 0x02 + (ofs & 0xFF)) + ofs;

	while (stdaddr < dtd_begin_addr) {
		stdreg = hdmitx2p1_ddc_read_byte(stdaddr >> 8, stdaddr & 0xFF);

		switch (stdreg & 0xE0) {
		case 0x20:
			hdmitx2p1_parse_audio_std(stdaddr, stdreg);
			break;
		case 0x40:
			hdmitx2p1_parse_video_std(stdaddr, stdreg);
			break;
		//case 0x80:
		//    ANX7150_Parse_SpeakerSTD();
		//   break;
		case 0x60:
			//HDMI VSDB
			hdmitx2p1_parse_vendor_std(stdaddr, stdreg);
			break;
		default:
			break;
		}

		stdaddr = stdaddr + (stdreg & 0x1f) + 0x01;
	}
}



static ER hdmitx2p1_ddc_read(UINT32 addr, UINT32 segm, UINT32 offset, UINT32 cnt)
{

}

static ER hdmitx2p1_ddc_read_byte2(UINT32 segm, UINT32 offset, UINT32 *pui_data)
{
}


static UINT8 hdmitx2p1_ddc_read_byte(UINT8 segm, UINT8 offset)
{

}


static ER hdmitx2p1_parse_edid(UINT32 ext_blk)
{
	UINT32 i;

	//
	// Parsing Block 1
	//
	// Phase-1: Parsing EDID Header
	if (hdmitx2p1_parse_edid_header()) {
		DBG_ERR("[2p1] BAD EDID Header, Stop parsing \n\r");
		return HDMITX_ER_BADHEADER;
	}

	// Phase-2: Parsing EDID Version
	if (hdmitx2p1_parse_edid_version()) {
		gui_hdmi2p1_edid_info |= HDMITX_TV_UNDERSCAN; // go to DVI path
		DBG_ERR("[2p1] EDID does not support 861B, Stop parsing\n\r");
		return HDMITX_ER_NOSUPPORT_VERSION;
	}

	// Phase-3: Verify EDID CheckSum <Block 1>
	if (hdmitx2p1_verify_edid_checksum(0)) {
		DBG_ERR("[2p1] EDID Block-1 check sum error, Stop parsing\n");
		//return HDMITX_ER_CRC;
	}

	//Phase-4: Parsing ET/MT
	hdmitx2p1_parse_etmt();

	//Phase-5: Parsing STI
	hdmitx2p1_parse_sti();

	//Phase-6: Parsing DTD
	hdmitx2p1_parse_dtd_in_block_one();

	// Check if Extension block exist
	if (hdmitx2p1_ddc_read_byte(0x00, 0x7E) == 0) {
		DBG_ERR("[2p1] No EDID extension blocks.\n");
		ext_blk = 0;
	}

	//
	// Parsing Extention Blocks
	//
	for (i = 1; i <= ext_blk; i++) {
		hdmitx_edidmsg(("===========================\r\n"));
		hdmitx_edidmsg(("=Start Parsing Ext-Block %d=\r\n", i));
		hdmitx_edidmsg(("===========================\r\n"));


		// Verify EDID CheckSum
		if (hdmitx2p1_verify_edid_checksum(i)) {
			DBG_ERR("[2p1] EDID Ext-Block-%d check sum error, Skip this block.\n\r", i);
			//continue;
		}

		// Parsing Data Collection Block
		if (hdmitx2p1_parse_ext_block(i) != E_OK) {
			continue;
		}

		// Parsing Native format
		hdmitx2p1_parse_native_format(i);

	}

	gui_native_edid_info = gui_hdmi2p1_edid_info;
	DBG_IND("HDMI EDID Parsing Results Flag: 0X%08X\r\n", gui_hdmi2p1_edid_info);


	if ((gui_hdmi2p1_edid_info & HDMITX_TV_HDMI) == 0x00) {
		// DVI device can support RGB only.
		gui_hdmi2p1_edid_info &= ~(HDMITX_TV_YCBCR444 | HDMITX_TV_YCBCR422);
		// Force Underscan at DVI mode.
		gui_hdmi2p1_edid_info |= HDMITX_TV_UNDERSCAN;
	}

	return E_OK;
}

#endif


ER hdmitx2p1_scdc_write(UINT32 offset, UINT32 data)
{
	return E_OK;
}


ER hdmitx2p1_scdc_read(UINT32 offset, UINT32 *pdata)
{
	return E_OK;
}

/*
    Initialize DDC to get EDID information

    Initialize DDC to get EDID information. The input parameter should specify the DDC used I2C channel is available or not.
    If the I2C channel is not available, the DDC read and EDID parser process would be skipped and output the warning message.

    @param[in] b_ddc_en   DDC used I2C channel is available or not.

    @return void
*/
void hdmitx2p1_init_ddc(BOOL b_ddc_en)
{
	UINT32 i, ext_blk = 0;
	int temp = 0;


	b_hdmi_edid_msg |= print_edid;

	gui_hdmi2p1_edid_info     = 0x0;
	gui_native_edid_info   = 0x0;
	gui_manufacturer_id   = 0x0;
	gb_valid_resolution   = FALSE;
	hdmi2p1_edid_error       = 0;

	memset(g_svd_vid_hdmi2p1,    0x00, (HDMITX_EDIDNUM + 1));
	memset(g_svd_hdmi2p1_vic, 0x00, 8);
	memset(g_svd_3d_cap_hdmi2p1, 0x00, (HDMITX_EDIDNUM + 1));
	gui_vsdb_h20cap_hdmi2p1 = 0;
	
	if (b_ddc_en && (!skip_edid)) {

		if (hdmitx2p1_check_hotplug() != TRUE) {
			DBG_ERR("[2p1] hdmi cable is unplug while %s , skip DDC init\n",__FUNCTION__);
			hdmi2p1_edid_error = 1;
			return;
		}	

		/* sony monitor needs a little delay for i2c ready */		
		if (hdmi_ddc_init_delay != 0) {			
			DELAY_MS(hdmi_ddc_init_delay);		
		}
		

#if HDMITX_SCDC_RRTEST
		hdmitx2p1_scdc_write(0x30, 0x00);
#endif

		temp = hdmitx2p1_ddc_read_byte2(0x00, 0x7E, &ext_blk); 
		if (temp != E_OK) {
			DBG_ERR("[2p1] DDC Abort!0 %d\r\n", temp);
			return;
		}


		if (hdmi_cts_test == 0) {
			if (ext_blk > 2) {
				DBG_ERR("[2p1] org ExtBlk:%d, set to 1 \n", ext_blk);
				ext_blk = 1;
				vos_util_delay_ms(100); /* delay for a while to have the sink ready. */
			}
		}


		
		if (ext_blk == 0x0) {
			// DVI Device
			if (hdmitx2p1_ddc_read(HDMI_DDCSLAVE_ADDR, 0x00, 0x00, HDMITX_EDID_BLKSIZE) != E_OK) {
				DBG_ERR("[2p1] DDC Abort!1\r\n");
				return;
			}
			hdmitx_edidmsg(("==ext_blk== %d %d\r\n", ext_blk, hdmi2p1_ext_blk));
			ext_blk = hdmi2p1_ext_blk;
		} else {
			// HDMI Device
			// Read EDID first 2 blocks
			if (hdmitx2p1_ddc_read(HDMI_DDCSLAVE_ADDR, 0x00, 0x00, HDMITX_EDID_BLKSIZE * 2) != E_OK) {
				DBG_ERR("[2p1] DDC Abort!2\r\n");
				return;
			}
		}


		if (ext_blk > 0x1) {
			UINT32 ofs;

			ofs = (ext_blk - 1) >> 1;
			for (i = 0; i < ofs; i++) {
				if (hdmitx2p1_ddc_read(HDMI_DDCSLAVE_ADDR, i + 1, 0x00, HDMITX_EDID_BLKSIZE * 2) != E_OK) {
					DBG_ERR("[2p1] DDC Abort!3\r\n");
					return;
				}

			}
			// Last One Block
			if ((ext_blk - 1) & 0x1) {
				if (hdmitx2p1_ddc_read(HDMI_DDCSLAVE_ADDR, ofs + 1, 0x00, HDMITX_EDID_BLKSIZE) != E_OK) {
					DBG_ERR("[2p1] DDC Abort!4\r\n");
					return;
				}
			}
		}
		
		hdmitx_edidmsg(("==== ext_blk total num:[%d] ====\r\n", ext_blk));
		// Parsing all the Ext-EDID
		hdmitx2p1_parse_edid(ext_blk);


#if HDMITX_DEBUG
#if HDMITX_DEBUG_FORCE_RGB
		gui_hdmi2p1_edid_info &= ~HDMITX_TV_YCBCR444;
		gui_hdmi2p1_edid_info &= ~HDMITX_TV_YCBCR422;
#endif

#if HDMITX_DEBUG_FORCE_YUV
		gui_hdmi2p1_edid_info |= (HDMITX_TV_YCBCR444 | HDMITX_TV_YCBCR422);
		gui_hdmi2p1_edid_info |= HDMITX_TV_HDMI;
#endif

#if HDMITX_DEBUG_FORCE_OVSCAN
		gui_hdmi2p1_edid_info &= ~HDMITX_TV_UNDERSCAN;
		gui_hdmi2p1_edid_info |= HDMITX_ADO_32KHZ;//Prevent Edid= 0x0
#endif
#endif

	} else {
		DBG_WRN("[2p1] Skip InitDDC\r\n");
	}
}


/*
    Get HDMI 2.0 VSDB Capability information

    Get HDMI 2.0 VSDB Capability information

    @return The parsing results bit-wise Flags.
*/
UINT32 hdmitx2p1_get_hdmi_2_vsdb_cap(void)
{
	return gui_vsdb_h20cap_hdmi2p1;
}

#if 1

/**
    Dump HDMI Information

    Dump HDMI debug information. The information includes EDID native and result flags
    and also the video format supported by the TV/Display from EDID descriptions.

    @return void
*/
void hdmitx2p1_dump_info(void)
{
	UINT32 i;

	if (hdmitx2p1_is_opened() == FALSE) {
		return;
	}

	//
	//Dump Information
	//
	DBG_DUMP("\r\n");
	DBG_DUMP("HDMI Configuration EDID Flag: 0X%08X\r\n", gui_hdmi2p1_edid_info);
	DBG_DUMP("HDMI Native EDID Parsing Results Flag: 0X%08X\r\n", gui_native_edid_info);
	DBG_DUMP("\r\n");

	DBG_DUMP("HDMI Supported Short Video Descriptor Format Code\r\n");
	for (i = 0; i < g_svd_vid_hdmi2p1[HDMITX_EDIDNUM]; i++) {
		if (i <= HDMITX_EDIDNUM) {
			DBG_DUMP("%d  ", g_svd_vid_hdmi2p1[i]);
		}

		if ((i == 16) || (i == 32) || (i == 48)) {
			DBG_DUMP("\r\n");
		}
	}
}


/**
    Get HDMI EDID information

    Get HDMI EDID Parsing results flags.
    This information is used in the HDMI display device object to control the hdmi functionality.

    @return The parsing results bit-wise Flags.
*/
UINT32 hdmitx2p1_get_edid_info(void)
{
	if(hdmi2p1_edid_error)
		return 0;

	return gui_hdmi2p1_edid_info;
}


/**
    Verify HDMI Video ID Validation

    This api would verify the user desired video format is supported by TV/Display or not.
    The supported list is from the TV/Display's EDID's Short Timing Descriptor(STD).
    If the desired VID is not supported, the basic format would be outputed.(720x480P60 / 640x480P60)

    @param[in,out] vid_code    Video-ID defined in CEA-861D, please refer to hdmitx2p1.h for details.

    @return void
*/
void hdmitx2p1_verify_video_id(HDMI_VIDEOID *vid_code)
{
	UINT32 valid, i;

	valid = 0;

	for (i = 0; i < g_svd_vid_hdmi2p1[HDMITX_EDIDNUM]; i++) {
		if (g_svd_vid_hdmi2p1[i] == *vid_code) {
			valid = 1;
		}
	}


/*
	if (valid == 0) {
		if ((*vid_code == HDMI_1280X720P60) || (*vid_code == HDMI_1280X720P50)) {
			if (gui_hdmi2p1_edid_info & (HDMITX_DTD720P60 | HDMITX_DTD720P50)) {
				valid = 1;
			}
		} else if ((*vid_code == HDMI_1920X1080I60) || (*vid_code == HDMI_1920X1080I50)) {
			if (gui_hdmi2p1_edid_info & (HDMITX_DTD1080I60 | HDMITX_DTD1080I50)) {
				valid = 1;
			}
		} else if ((*vid_code == HDMI_1920X1080P60) || (*vid_code == HDMI_1920X1080P50)) {
			if (gui_hdmi2p1_edid_info & (HDMITX_DTD1080P60 | HDMITX_DTD1080P50)) {
				valid = 1;
			}
		}
	}

	if (valid == 0) {
		// Basic Format
		for (i = 0; i < HDMITX_EDIDNUM; i++) {
			if ((valid == 0) && ((g_svd_vid_hdmi2p1[i] == HDMI_720X480P60) || (g_svd_vid_hdmi2p1[i] == HDMI_720X480P60_16X9) || (g_svd_vid_hdmi2p1[i] == HDMI_640X480P60))) {
				valid = 1;
				if (g_svd_vid_hdmi2p1[i] == HDMI_720X480P60) {
					*vid_code = HDMI_720X480P60;
				} else if (g_svd_vid_hdmi2p1[i] == HDMI_720X480P60_16X9) {
					*vid_code = HDMI_720X480P60_16X9;
				} else if (g_svd_vid_hdmi2p1[i] == HDMI_640X480P60) {
					*vid_code = HDMI_640X480P60;
				}
			}
		}
		if (valid == 0) {
			*vid_code = HDMI_640X480P60;
		}

		DBG_ERR("[2p1] \r\nDesired Video Format Sink not support!! Output HDMI Basic format! (%d)\r\n\n", *vid_code);
	}
*/
}

/**
    Get the HDMI Video Format Support Ability

    This api is used to get the HDMI Video Format Support Ability inlcudes the 2D/3D format supporting capability.
    The user provided "vdo_ability" buffer should contains at least 30 entries in safety.

    @param[out] p_len            Number of video formats supported by the TV/Display.
    @param[out] vdo_ability     The structure of the supported video format and its 3D ability.

    @return void
*/
void hdmitx2p1_get_video_ability(UINT32 *p_len, PHDMI_VDOABI vdo_ability)
{
	UINT32 i, k = 0, j = 0;
	UINT32 size;


#if !(HDMITX_DEBUG && HDMITX_DEBUG_SKIP_EDID)
	if ((b_hdmi_i2c_opened == FALSE) && (hdmitx2p1_check_hotplug())) {
		if (hdmi_i2c_obj[hdmi_i2c_select].open(&hdmi_i2c_ses) == E_OK) {
			b_hdmi_i2c_opened = TRUE;

			hdmitx2p1_init_ddc(b_hdmi_i2c_opened);

			if (hdmi_i2c_obj[hdmi_i2c_select].close(hdmi_i2c_ses) == E_OK) {
				b_hdmi_i2c_opened = FALSE;
			}
		} else {
			b_hdmi_i2c_opened = FALSE;
		}
	} else {
		hdmitx2p1_init_ddc(b_hdmi_i2c_opened);
	}
#endif

	if(hdmi2p1_edid_error) {
		*p_len = 0;
		return;
	}

	hdmitx_edidmsg(("hdmitx2p1_get_video_ability, *p_len = %d, g_svd_vid_hdmi2p1[HDMITX_EDIDNUM] = %d\n", *p_len, g_svd_vid_hdmi2p1[HDMITX_EDIDNUM]));

	size = *p_len;
	*p_len = (UINT32) g_svd_vid_hdmi2p1[HDMITX_EDIDNUM];

	/*hdmitx_edidmsg(("hdmitx2p1_get_video_ability: Length=%d  :\r\n", *p_len));
	if (*p_len == 0) {
		*p_len = 26;
		DBG_ERR("[2p1] hdmitx2p1_get_video_ability, user *p_len == 0, reset *p_len to %d\n", *p_len);
	}*/

	for (i = 0; i < 8; i++) {
		if (g_svd_hdmi2p1_vic[i] != 0)
			(*p_len)++;
	}

	for(i=0;i<HDMITX_EXTRA_NUM;i++){
		if (g_svd_vid_extra_hdmi2p1[i] != 0)
			(*p_len)++;		
	}
	
	if (*p_len > size)
		*p_len = size;
	/*
	if (*p_len > HDMITX_ABI_LEN_MAX)
		*p_len = HDMITX_ABI_LEN_MAX;
*/
	hdmitx_edidmsg(("hdmitx2p1_get_video_ability: Length=%d  :\r\n", *p_len));
	for (i = 0; i < *p_len; i++) {
		vdo_ability[i].video_id      = g_svd_vid_hdmi2p1[i];
		vdo_ability[i].vdo_3d_ability = g_svd_3d_cap_hdmi2p1[i];

		//3D_present
		if (g_svd_3d_cap_hdmi2p1[HDMITX_EDIDNUM]) {
			if ((vdo_ability[i].video_id == HDMI_1920X1080P24) || (vdo_ability[i].video_id == HDMI_1280X720P60) || (vdo_ability[i].video_id == HDMI_1280X720P50)) {
				vdo_ability[i].vdo_3d_ability |= (HDMI3D_FRAMEPACKING | HDMI3D_TOPANDBOTTOM);
			}

			if ((vdo_ability[i].video_id == HDMI_1920X1080I60) || (vdo_ability[i].video_id == HDMI_1920X1080I50)) {
				vdo_ability[i].vdo_3d_ability |= HDMI3D_SIDEBYSIDE_HALF;
			}
		}

		if ((vdo_ability[i].video_id == 0) && (k < 8) && (g_svd_hdmi2p1_vic[k] == 1)) {
			vdo_ability[i].video_id = HDMI14B_3840X2160P30;
			k++;
		}
		if ((vdo_ability[i].video_id == 0) && (k < 8) && (g_svd_hdmi2p1_vic[k] == 2)) {
			vdo_ability[i].video_id = HDMI14B_3840X2160P25;
			k++;
		}
		if ((vdo_ability[i].video_id == 0) && (k < 8) && (g_svd_hdmi2p1_vic[k] == 3)) {
			vdo_ability[i].video_id = HDMI14B_3840X2160P24;
			k++;
		}
		if ((vdo_ability[i].video_id == 0) && (k < 8) && (g_svd_hdmi2p1_vic[k] == 4)) {
			vdo_ability[i].video_id = HDMI14B_4096X2160P24;
			k++;
		}

		if ((vdo_ability[i].video_id == 0) && (j < HDMITX_EXTRA_NUM) && (g_svd_vid_extra_hdmi2p1[j] != 0)) {
			vdo_ability[i].video_id = g_svd_vid_extra_hdmi2p1[j];
			j++;
		}			
	}

	/*put recommend vid to first*/
	if(g_recommend_vid_hdmi2p1[1] != 0){
		for (i = 0; i < *p_len; i++) {
			if(vdo_ability[i].video_id == g_recommend_vid_hdmi2p1[1]){
				for(j=i;j>0;j--){
					vdo_ability[j].video_id = vdo_ability[j-1].video_id;
				}
				vdo_ability[0].video_id = g_recommend_vid_hdmi2p1[1];
				break;
			}
		}
	}

	if(g_recommend_vid_hdmi2p1[0] != 0){
		for (i = 0; i < *p_len; i++) {
			if(vdo_ability[i].video_id == g_recommend_vid_hdmi2p1[0]){
				for(j=i;j>0;j--){
					vdo_ability[j].video_id = vdo_ability[j-1].video_id;
				}
				vdo_ability[0].video_id = g_recommend_vid_hdmi2p1[0];
				break;
			}
		}
	}
	
	for (i = 0; i < *p_len; i++) {
		hdmitx_edidmsg(("  VID=(%d)   3D_Cap=0x%X \r\n", vdo_ability[i].video_id, vdo_ability[i].vdo_3d_ability));
	}

	if ((*p_len == 0) && (!(gui_hdmi2p1_edid_info & HDMITX_TV_HDMI)) &&
		(gui_hdmi2p1_edid_info & 0x3000373F)) {

		i = 0;
		if (gui_hdmi2p1_edid_info & HDMITX_DTD4096X2160P30) {
			vdo_ability[i++].video_id = HDMI_4096X2160P30;
			(*p_len)++;
		}
		if (gui_hdmi2p1_edid_info & HDMITX_DTD3840X2160P30) {
			vdo_ability[i++].video_id = HDMI_3840X2160P30;
			(*p_len)++;
		}
		if (gui_hdmi2p1_edid_info & HDMITX_DTD1080P60) {
			vdo_ability[i++].video_id = HDMI_1920X1080P60;
			(*p_len)++;
		}
		if (gui_hdmi2p1_edid_info & HDMITX_DTD1080P50) {
			vdo_ability[i++].video_id = HDMI_1920X1080P50;
			(*p_len)++;
		}
		if (gui_hdmi2p1_edid_info & HDMITX_DTD1080I60) {
			vdo_ability[i++].video_id = HDMI_1920X1080I60;
			(*p_len)++;
		}
		if (gui_hdmi2p1_edid_info & HDMITX_DTD720P60) {
			vdo_ability[i++].video_id = HDMI_1280X720P60;
			(*p_len)++;
		}
		if (gui_hdmi2p1_edid_info & HDMITX_DTD720P50) {
			vdo_ability[i++].video_id = HDMI_1280X720P50;
			(*p_len)++;
		}
		if (gui_hdmi2p1_edid_info & HDMITX_DTD1080I50) {
			vdo_ability[i++].video_id = HDMI_1920X1080I50;
			(*p_len)++;
		}
		if (gui_hdmi2p1_edid_info & HDMITX_DTD720X480P60) {
			vdo_ability[i++].video_id = HDMI_720X480P60;
			(*p_len)++;
		}
		if (gui_hdmi2p1_edid_info & HDMITX_DTD576P50) {
			vdo_ability[i++].video_id = HDMI_720X576P50;
			(*p_len)++;
		}
		if (gui_hdmi2p1_edid_info & HDMITX_DTD640X480P60) {
			vdo_ability[i++].video_id = HDMI_640X480P60;
			(*p_len)++;
		}
		if (gui_hdmi2p1_edid_info & HDMITX_DTD720X480I60) {
			vdo_ability[i++].video_id = HDMI_720X480I60;
			(*p_len)++;
		}
		if (gui_hdmi2p1_edid_info & HDMITX_DTD576I50) {
			vdo_ability[i++].video_id = HDMI_720X576I50;
			(*p_len)++;
		}
	}

	hdmitx2p1_edid_unlock();


}

void hdmitx2p1_get_edid_raw_data(UINT32 *len, UINT8 *data)
{
	int i;


	if(data == NULL){
		DBG_ERR("[2p1] EDID parsing Buffer NULL Error!\r\n");
		hdmitx2p1_edid_unlock();
		return;
	}
	
	if(*len > HDMITX_EDID_BLKSIZE*(hdmi2p1_ext_blk+1)){
		*len = HDMITX_EDID_BLKSIZE*(hdmi2p1_ext_blk+1);
	}

	for(i=0; i<(int)*len; i++){
		if (hdmitx2p1_check_hotplug() != TRUE) {
			DBG_ERR("[2p1] hdmi cable is unplug while %s. \n",__FUNCTION__);
			hdmitx2p1_edid_unlock();
			return;
		}		
		*(data+i) = hdmitx2p1_ddc_read_byte(0x00,0x00+i);
	}

	
	return;
}

/**
    Check if the HDMI RX's DDC Channel is available or not.

    This api is used to check if the HDMI Rx is available or not.

    @return
     - @b TRUE:  The HDMI RX's DDC Channel is available.
     - @b FALSE: The HDMI RX's DDC Channel is NOT available.
*/
BOOL hdmitx2p1_check_ddc(void)
{
	UINT32 ext_blk;

	if (b_hdmi_i2c_opened == FALSE) {
		if (hdmi_i2c_obj[hdmi_i2c_select].open(&hdmi_i2c_ses) != E_OK) {
			DBG_ERR("[2p1] I2C Open Error!\r\n");
			return FALSE;
		}

		b_hdmi_i2c_opened = TRUE;
	}


	if (hdmitx2p1_ddc_read_byte2(0x00, 0x7E, &ext_blk) != E_OK) {
		DBG_ERR("[2p1] DDC NACK\r\n");
		return FALSE;
	}

	return TRUE;
}

#endif



