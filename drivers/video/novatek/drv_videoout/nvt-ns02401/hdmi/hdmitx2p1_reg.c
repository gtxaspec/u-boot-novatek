
#include "hdmitx_int.h"
#include "hdmitx2p1_reg.h"
#include "hdmitx2p1_int.h"





/*
    For HDMI reg Control
*/
void hdmitx2p1_write_reg(UINT32 id, UINT32 page, UINT32 ui_offset, UINT32 ui_value)
{
	if((page != HTX_LINK_CTRL0) &&
		(page != HTX_LINK_CTRL1) &&
		(page != HTX_LINK_CTRL2) &&
		(page != HTX_PPS_CTRL) &&
		(page != HTX_EMP_CTRL) &&
		(page != HTX_PHY_CTRL) &&
		(page != HTX_FRL_CTRL)
	){
		//DBG_WRN("illegal reg page [%d]\r\n",page);
		return;
	}

	if(id != 0){
		DBG_WRN("illegal hdmi2.1 id [%d]\r\n",id);
		return;		
	}


	//DBG_WRN("wr :[P][O]/[RO][V] = [0x%x][0x%x]/[0x%x][0x%x]\r\n",page,ui_offset,REG_MAP_2P1(page,ui_offset),ui_value);
	
	HDMI2P1_SETREG(REG_MAP_2P1(page,ui_offset),ui_value);
}

void hdmitx2p1_write_reg_bit(UINT32 id, UINT32 page, UINT32 ui_offset, UINT32 mask, UINT32 ui_value)
{
	UINT32 reg_val;
	if((page != HTX_LINK_CTRL0) &&
		(page != HTX_LINK_CTRL1) &&
		(page != HTX_LINK_CTRL2) &&
		(page != HTX_PPS_CTRL) &&
		(page != HTX_EMP_CTRL) &&
#if !_FPGA_EMULATION_
		(page != HTX_PHY_CTRL) &&
#endif		
		(page != HTX_FRL_CTRL)
	){
		//DBG_WRN("illegal reg page [%d]\r\n",page);
		return;
	}

	if(id != 0){
		DBG_WRN("illegal hdmi2.1 id [%d]\r\n",id);
		return;		
	}

	reg_val = HDMI2P1_GETREG(REG_MAP_2P1(page,ui_offset));

    if (ui_value == TRUE) {//1
        reg_val |= mask;
    }
    else {
        reg_val &= ~mask;
    }

	//DBG_WRN("wrb:[P][O]/[RO][M][V] = [0x%x][0x%x]/[0x%x][0x%x][0x%x]\r\n",page,ui_offset,REG_MAP_2P1(page,ui_offset),mask,reg_val);
	
	HDMI2P1_SETREG(REG_MAP_2P1(page,ui_offset),reg_val);
}

void hdmitx2p1_write_reg_mask(UINT32 id, UINT32 page, UINT32 ui_offset, UINT32 mask, UINT32 ui_value)
{
	UINT32 reg_temp, val_temp;
	
	if((page != HTX_LINK_CTRL0) &&
		(page != HTX_LINK_CTRL1) &&
		(page != HTX_LINK_CTRL2) &&
		(page != HTX_PPS_CTRL) &&
		(page != HTX_EMP_CTRL) &&
#if !_FPGA_EMULATION_
		(page != HTX_PHY_CTRL) &&
#endif		
		(page != HTX_FRL_CTRL)
	){
		//DBG_WRN("illegal reg page [%d]\r\n",page);
		return;
	}

	if(id != 0){
		DBG_WRN("illegal hdmi2.1 id [%d]\r\n",id);
		return;		
	}

	reg_temp = HDMI2P1_GETREG(REG_MAP_2P1(page,ui_offset));
	reg_temp= reg_temp & ~mask;
	val_temp = ui_value & mask;
	val_temp= reg_temp|val_temp;


	//DBG_WRN("set reg bit:[page][ofs]/[remap-ofs][bit][val] = [0x%x][0x%x]/[0x%x][0x%x][0x%x]\r\n",page,ui_offset,REG_MAP_2P1(page,ui_offset),mask,val_temp);
	
	HDMI2P1_SETREG(REG_MAP_2P1(page,ui_offset),val_temp);
}



/*
    For HDMI reg Control
*/
UINT32 hdmitx2p1_read_reg(UINT32 id, UINT32 page, UINT32 ui_offset)
{
	UINT32 ui_value;
	
	if((page != HTX_LINK_CTRL0) &&
		(page != HTX_LINK_CTRL1) &&
		(page != HTX_LINK_CTRL2) &&
		(page != HTX_PPS_CTRL) &&
		(page != HTX_EMP_CTRL) &&
#if !_FPGA_EMULATION_
		(page != HTX_PHY_CTRL) &&
#endif		
		(page != HTX_FRL_CTRL)
	){
		//DBG_WRN("illegal reg page [%d]\r\n",page);
		return 0;
	}

	if(id != 0){
		DBG_WRN("illegal hdmi2.1 id [%d]\r\n",id);
		return 0;		
	}	

	ui_value = HDMI2P1_GETREG(REG_MAP_2P1(page,ui_offset));

	//DBG_WRN("get reg:[page][ofs]/[remap-ofs][val] = [0x%x][0x%x]/[0x%x][0x%x]\r\n",page,ui_offset,REG_MAP_2P1(page,ui_offset),ui_value);

	return ui_value;

}
