/*******************************************************************************
* Copyright 2011 Broadcom Corporation.	All rights reserved.
*
* @file drivers/video/broadcom/lcd/nt35510.h
*
* Unless you and Broadcom execute a separate written software license agreement
* governing use of this software, this software is licensed to you under the
* terms of the GNU General Public License version 2, available at
* http://www.gnu.org/copyleft/gpl.html (the "GPL").
*
* Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a license
* other than the GPL, without Broadcom's express prior written consent.
*******************************************************************************/

#ifndef __NT35510_H__
#define __NT35510_H__

#include "display_drv.h"
#include "lcd.h"

#define NT35510_CMD_SLPIN	0x10
#define NT35510_CMD_SLPOUT	0x11
#define NT35510_CMD_DISPOFF	0x28
#define NT35510_CMD_DISPON	0x29
#define NT35510_CMD_DSTBON	0x4F
#define NT35510_CMD_RDID1      0xDA
#define NT35510_CMD_RDID2      0xDB
#define NT35510_CMD_RDID3      0xDC

#define NT35510_UPDT_WIN_SEQ_LEN 13 /* (6 + 6 + 1) */

__initdata struct DSI_COUNTER nt35510_timing[] = {
	/* LP Data Symbol Rate Calc - MUST BE FIRST RECORD */
	{"ESC2LP_RATIO", DSI_C_TIME_ESC2LPDT, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0x0000003F, 1, 1, 0},
	/* SPEC:  min =  100[us] + 0[UI] */
	/* SET:   min = 1000[us] + 0[UI]                             <= */
	{"HS_INIT", DSI_C_TIME_HS, 0,
		0, 100000, 0, 0, 0, 0, 0, 0, 0x00FFFFFF, 0, 0, 0},
	/* SPEC:  min = 1[ms] + 0[UI] */
	/* SET:   min = 1[ms] + 0[UI] */
	{"HS_WAKEUP", DSI_C_TIME_HS, 0,
		0, 1000000, 0, 0, 0, 0, 0, 0, 0x00FFFFFF, 0, 0, 0},
	/* SPEC:  min = 1[ms] + 0[UI] */
	/* SET:   min = 1[ms] + 0[UI] */
	{"LP_WAKEUP", DSI_C_TIME_ESC, 0,
		0, 1000000, 0, 0, 0, 0, 0, 0, 0x00FFFFFF, 1, 1, 0},
	/* SPEC:  min = 0[ns] +  8[UI] */
	/* SET:   min = 0[ns] + 12[UI]                               <= */
	{"HS_CLK_PRE", DSI_C_TIME_HS, 0,
		0, 10, 8, 0, 0, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min = 38[ns] + 0[UI]   max= 95[ns] + 0[UI] */
	/* SET:   min = 68[ns] + 0[UI]   max= 95[ns] + 0[UI]         <= */
	{"HS_CLK_PREPARE", DSI_C_TIME_HS, DSI_C_HAS_MAX,
		0, 75, 0, 0, 0, 95, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min = 262[ns] + 0[UI] */
	/* SET:   min = 314[ns] + 0[UI]                              <= */
	{"HS_CLK_ZERO", DSI_C_TIME_HS, 0,
		0, 300, 0, 0, 0, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min =  60[ns] + 52[UI] */
	/* SET:   min =  72[ns] + 52[UI]                             <= */
	{"HS_CLK_POST", DSI_C_TIME_HS, 0,
		0, 70, 128, 0, 0, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min =  60[ns] + 0[UI] */
	/* SET:   min =  72[ns] + 0[UI]                              <= */
	{"HS_CLK_TRAIL", DSI_C_TIME_HS, 0,
		0, 300, 0, 0, 0, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min =  50[ns] + 0[UI] */
	/* SET:   min =  60[ns] + 0[UI]                              <= */
	{"HS_LPX", DSI_C_TIME_HS, 0,
		0, 70, 0, 0, 0, 75, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min = 40[ns] + 4[UI]      max= 85[ns] + 6[UI] */
	/* SET:   min = 60[ns] + 4[UI]      max= 85[ns] + 6[UI]      <= */
	{"HS_PRE", DSI_C_TIME_HS, DSI_C_HAS_MAX,
		0, 60, 4, 0, 0, 85, 6, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min = 105[ns] + 6[UI] */
	/* SET:   min = 125[ns] + 6[UI]                              <= */
	{"HS_ZERO", DSI_C_TIME_HS, 0,
		0, 280, 6, 0, 0, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min = max(0[ns]+32[UI],60[ns]+16[UI])  n=4 */
	/* SET:   min = max(0[ns]+32[UI],60[ns]+16[UI])  n=4 */
	{"HS_TRAIL", DSI_C_TIME_HS, DSI_C_MIN_MAX_OF_2,
		0, 60, 32, 100, 16, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* SPEC:  min = 100[ns] + 0[UI] */
	/* SET:   min = 120[ns] + 0[UI]                              <= */
	{"HS_EXIT", DSI_C_TIME_HS, 0,
		0, 300, 0, 0, 0, 0, 0, 0, 0x000001FF, 0, 0, 0},
	/* min = 50[ns] + 0[UI] */
	/* LP esc counters are speced in LP LPX units.
	   LP_LPX is calculated by chal_dsi_set_timing
	   and equals LP data clock */
	{"LPX", DSI_C_TIME_ESC, 0,
		1, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1, 0},
	/* min = 4*[Tlpx]  max = 4[Tlpx], set to 4 */
	{"LP-TA-GO", DSI_C_TIME_ESC, 0,
		4, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1, 0},
	/* min = 1*[Tlpx]  max = 2[Tlpx], set to 2 */
	{"LP-TA-SURE", DSI_C_TIME_ESC, 0,
		2, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1, 0},
	/* min = 5*[Tlpx]  max = 5[Tlpx], set to 5 */
	{"LP-TA-GET", DSI_C_TIME_ESC, 0,
		5, 0, 0, 0, 0, 0, 0, 0, 0x000000FF, 1, 1, 0},
};

__initdata DISPCTRL_REC_T nt35510_scrn_on[] = {
	{DISPCTRL_WR_CMND, NT35510_CMD_DISPON},
	{DISPCTRL_LIST_END, 0}
};

__initdata DISPCTRL_REC_T nt35510_scrn_off[] = {
	{DISPCTRL_WR_CMND, NT35510_CMD_DISPOFF},
	{DISPCTRL_LIST_END, 0}
};

__initdata DISPCTRL_REC_T nt35510_id[] = {
	{DISPCTRL_WR_CMND, NT35510_CMD_RDID1},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_CMND, NT35510_CMD_RDID2},
	{DISPCTRL_WR_DATA, 0x80},
	{DISPCTRL_WR_CMND, NT35510_CMD_RDID3},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_LIST_END, 0}
};

__initdata DISPCTRL_REC_T nt35510_slp_in[] = {
	{DISPCTRL_WR_CMND, NT35510_CMD_DISPOFF},
	{DISPCTRL_WR_CMND, NT35510_CMD_SLPIN},
	{DISPCTRL_SLEEP_MS, 120},
	{DISPCTRL_WR_CMND, NT35510_CMD_DSTBON},
	{DISPCTRL_WR_DATA, 1},
	{DISPCTRL_LIST_END, 0}
};

__initdata DISPCTRL_REC_T nt35510_slp_out[] = {
	{DISPCTRL_WR_CMND, NT35510_CMD_SLPOUT},
	{DISPCTRL_SLEEP_MS, 120},
	{DISPCTRL_LIST_END, 0}
};

__initdata DISPCTRL_REC_T nt35510_init_panel_cmd[] = {
	{DISPCTRL_WR_CMND, 0x11},
	{DISPCTRL_SLEEP_MS, 120},	
	/* + User Set */
	{DISPCTRL_WR_CMND, 0xF0},  /* Manufacture Command Set Selection */
	{DISPCTRL_WR_DATA, 0x55},
	{DISPCTRL_WR_DATA, 0xAA},
	{DISPCTRL_WR_DATA, 0x52},
	{DISPCTRL_WR_DATA, 0x08},
	{DISPCTRL_WR_DATA, 0x01},

	{DISPCTRL_WR_CMND, 0xB0},
	{DISPCTRL_WR_DATA, 0x09},
	{DISPCTRL_WR_DATA, 0x09},
	{DISPCTRL_WR_DATA, 0x09},

	{DISPCTRL_WR_CMND, 0xB1},
	{DISPCTRL_WR_DATA, 0x09},
	{DISPCTRL_WR_DATA, 0x09},
	{DISPCTRL_WR_DATA, 0x09},

	{DISPCTRL_WR_CMND, 0xB3},
	{DISPCTRL_WR_DATA, 0x05},
	{DISPCTRL_WR_DATA, 0x05},
	{DISPCTRL_WR_DATA, 0x05},

	{DISPCTRL_WR_CMND, 0xB5},
	{DISPCTRL_WR_DATA, 0x0b},
	{DISPCTRL_WR_DATA, 0x0b},
	{DISPCTRL_WR_DATA, 0x0b},
	
	{DISPCTRL_WR_CMND, 0xB6},
	{DISPCTRL_WR_DATA, 0x34},
	{DISPCTRL_WR_DATA, 0x34},
	{DISPCTRL_WR_DATA, 0x34},

	{DISPCTRL_WR_CMND, 0xB7},
	{DISPCTRL_WR_DATA, 0x24},
	{DISPCTRL_WR_DATA, 0x24},
	{DISPCTRL_WR_DATA, 0x24},

	{DISPCTRL_WR_CMND, 0xB9},
	{DISPCTRL_WR_DATA, 0x25},
	{DISPCTRL_WR_DATA, 0x25},
	{DISPCTRL_WR_DATA, 0x25},

	{DISPCTRL_WR_CMND, 0xBA},
	{DISPCTRL_WR_DATA, 0x24},
	{DISPCTRL_WR_DATA, 0x24},
	{DISPCTRL_WR_DATA, 0x24},

	{DISPCTRL_WR_CMND, 0xBC},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xA3},
	{DISPCTRL_WR_DATA, 0x00},

	{DISPCTRL_WR_CMND, 0xBD},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xA3},
	{DISPCTRL_WR_DATA, 0x00},
	
	{DISPCTRL_WR_CMND, 0xBF},
	{DISPCTRL_WR_DATA, 0x01},

	{DISPCTRL_WR_CMND, 0xC2},
	{DISPCTRL_WR_DATA, 0x01},

	{DISPCTRL_WR_CMND, 0xD1},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x27},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x47},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x62},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x92},	
	{DISPCTRL_WR_DATA, 0x00},
       {DISPCTRL_WR_DATA, 0xA2},
        
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xCE},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xEF},
	{DISPCTRL_WR_DATA, 0x01},
       {DISPCTRL_WR_DATA, 0x23},  
	{DISPCTRL_WR_DATA, 0x01},
    	{DISPCTRL_WR_DATA, 0x4B},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0x8B},
	
	{DISPCTRL_WR_DATA, 0x01},
       {DISPCTRL_WR_DATA, 0xBB},
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x09},	
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x49},
	{DISPCTRL_WR_DATA, 0x02},
       {DISPCTRL_WR_DATA, 0x4B},
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x82},
	
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0xBC},	
	{DISPCTRL_WR_DATA, 0x02},
       {DISPCTRL_WR_DATA, 0xDF},
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0x12},
	{DISPCTRL_WR_DATA, 0x03},	
	{DISPCTRL_WR_DATA, 0x3A},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x5A},
	
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0x80},        
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x98},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xB3},
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0xC4},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xC9},
	
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xEF},

	{DISPCTRL_WR_CMND, 0xD2},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x27},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x47},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x62},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x92},	
	{DISPCTRL_WR_DATA, 0x00},
       {DISPCTRL_WR_DATA, 0xA2},
        
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xCE},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xEF},
	{DISPCTRL_WR_DATA, 0x01},
       {DISPCTRL_WR_DATA, 0x23},  
	{DISPCTRL_WR_DATA, 0x01},
    	{DISPCTRL_WR_DATA, 0x4B},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0x8B},
	
	{DISPCTRL_WR_DATA, 0x01},
       {DISPCTRL_WR_DATA, 0xBB},
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x09},	
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x49},
	{DISPCTRL_WR_DATA, 0x02},
       {DISPCTRL_WR_DATA, 0x4B},
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x82},
	
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0xBC},	
	{DISPCTRL_WR_DATA, 0x02},
       {DISPCTRL_WR_DATA, 0xDF},
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0x12},
	{DISPCTRL_WR_DATA, 0x03},	
	{DISPCTRL_WR_DATA, 0x3A},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x5A},
	
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0x80},        
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x98},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xB3},
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0xC4},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xC9},
	
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xEF},
	
	{DISPCTRL_WR_CMND, 0xD3},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x27},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x47},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x62},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x92},	
	{DISPCTRL_WR_DATA, 0x00},
       {DISPCTRL_WR_DATA, 0xA2},
        
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xCE},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xEF},
	{DISPCTRL_WR_DATA, 0x01},
       {DISPCTRL_WR_DATA, 0x23},  
	{DISPCTRL_WR_DATA, 0x01},
    	{DISPCTRL_WR_DATA, 0x4B},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0x8B},
	
	{DISPCTRL_WR_DATA, 0x01},
       {DISPCTRL_WR_DATA, 0xBB},
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x09},	
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x49},
	{DISPCTRL_WR_DATA, 0x02},
       {DISPCTRL_WR_DATA, 0x4B},
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x82},
	
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0xBC},	
	{DISPCTRL_WR_DATA, 0x02},
       {DISPCTRL_WR_DATA, 0xDF},
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0x12},
	{DISPCTRL_WR_DATA, 0x03},	
	{DISPCTRL_WR_DATA, 0x3A},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x5A},
	
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0x80},        
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x98},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xB3},
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0xC4},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xC9},
	
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xEF},
	
	{DISPCTRL_WR_CMND, 0xD4},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x27},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x47},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x62},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x92},	
	{DISPCTRL_WR_DATA, 0x00},
       {DISPCTRL_WR_DATA, 0xA2},
        
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xCE},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xEF},
	{DISPCTRL_WR_DATA, 0x01},
       {DISPCTRL_WR_DATA, 0x23},  
	{DISPCTRL_WR_DATA, 0x01},
    	{DISPCTRL_WR_DATA, 0x4B},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0x8B},
	
	{DISPCTRL_WR_DATA, 0x01},
       {DISPCTRL_WR_DATA, 0xBB},
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x09},	
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x49},
	{DISPCTRL_WR_DATA, 0x02},
       {DISPCTRL_WR_DATA, 0x4B},
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x82},
	
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0xBC},	
	{DISPCTRL_WR_DATA, 0x02},
       {DISPCTRL_WR_DATA, 0xDF},
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0x12},
	{DISPCTRL_WR_DATA, 0x03},	
	{DISPCTRL_WR_DATA, 0x3A},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x5A},
	
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0x80},        
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x98},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xB3},
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0xC4},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xC9},
	
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xEF},

	{DISPCTRL_WR_CMND, 0xD5},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x27},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x47},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x62},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x92},	
	{DISPCTRL_WR_DATA, 0x00},
       {DISPCTRL_WR_DATA, 0xA2},
        
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xCE},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xEF},
	{DISPCTRL_WR_DATA, 0x01},
       {DISPCTRL_WR_DATA, 0x23},  
	{DISPCTRL_WR_DATA, 0x01},
    	{DISPCTRL_WR_DATA, 0x4B},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0x8B},
	
	{DISPCTRL_WR_DATA, 0x01},
       {DISPCTRL_WR_DATA, 0xBB},
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x09},	
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x49},
	{DISPCTRL_WR_DATA, 0x02},
       {DISPCTRL_WR_DATA, 0x4B},
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x82},
	
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0xBC},	
	{DISPCTRL_WR_DATA, 0x02},
       {DISPCTRL_WR_DATA, 0xDF},
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0x12},
	{DISPCTRL_WR_DATA, 0x03},	
	{DISPCTRL_WR_DATA, 0x3A},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x5A},
	
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0x80},        
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x98},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xB3},
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0xC4},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xC9},
	
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xEF},

	{DISPCTRL_WR_CMND, 0xD6},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x27},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x47},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x62},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x92},	
	{DISPCTRL_WR_DATA, 0x00},
       {DISPCTRL_WR_DATA, 0xA2},
        
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xCE},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0xEF},
	{DISPCTRL_WR_DATA, 0x01},
       {DISPCTRL_WR_DATA, 0x23},  
	{DISPCTRL_WR_DATA, 0x01},
    	{DISPCTRL_WR_DATA, 0x4B},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0x8B},
	
	{DISPCTRL_WR_DATA, 0x01},
       {DISPCTRL_WR_DATA, 0xBB},
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x09},	
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x49},
	{DISPCTRL_WR_DATA, 0x02},
       {DISPCTRL_WR_DATA, 0x4B},
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0x82},
	
	{DISPCTRL_WR_DATA, 0x02},
	{DISPCTRL_WR_DATA, 0xBC},	
	{DISPCTRL_WR_DATA, 0x02},
       {DISPCTRL_WR_DATA, 0xDF},
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0x12},
	{DISPCTRL_WR_DATA, 0x03},	
	{DISPCTRL_WR_DATA, 0x3A},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x5A},
	
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0x80},        
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x98},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xB3},
	{DISPCTRL_WR_DATA, 0x03},
       {DISPCTRL_WR_DATA, 0xC4},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xC9},
	
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0xEF},
	/* skip gama setting for test purpose */

	{DISPCTRL_WR_CMND, 0xF0},
	{DISPCTRL_WR_DATA, 0x55},
	{DISPCTRL_WR_DATA, 0xAA},
	{DISPCTRL_WR_DATA, 0x52},
	{DISPCTRL_WR_DATA, 0x08},
	{DISPCTRL_WR_DATA, 0x00},

	{DISPCTRL_WR_CMND, 0xB1},
	{DISPCTRL_WR_DATA, 0x4c},
	{DISPCTRL_WR_DATA, 0x04},

	{DISPCTRL_WR_CMND, 0xB6},
	{DISPCTRL_WR_DATA, 0x0A},

	{DISPCTRL_WR_CMND, 0xB7},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x00},

	{DISPCTRL_WR_CMND, 0xB8},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0x05},
	{DISPCTRL_WR_DATA, 0x05},
	{DISPCTRL_WR_DATA, 0x05},

	{DISPCTRL_WR_CMND, 0xBA},
	{DISPCTRL_WR_DATA, 0x01},

	{DISPCTRL_WR_CMND, 0xBD},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0x84},
	{DISPCTRL_WR_DATA, 0x07},
	{DISPCTRL_WR_DATA, 0x32},
	{DISPCTRL_WR_DATA, 0x00},

	{DISPCTRL_WR_CMND, 0xBE},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0x84},
	{DISPCTRL_WR_DATA, 0x07},
	{DISPCTRL_WR_DATA, 0x31},
	{DISPCTRL_WR_DATA, 0x00},

	{DISPCTRL_WR_CMND, 0xBF},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0x84},
	{DISPCTRL_WR_DATA, 0x07},
	{DISPCTRL_WR_DATA, 0x31},
	{DISPCTRL_WR_DATA, 0x00},

	{DISPCTRL_WR_CMND, 0xCC},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x00},
	
	{DISPCTRL_WR_CMND, 0x35},
	{DISPCTRL_WR_DATA, 0x00},

	{DISPCTRL_WR_CMND, 0x36},
	{DISPCTRL_WR_DATA, 0x02},
	
	{DISPCTRL_WR_CMND, 0x2A},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x01},
	{DISPCTRL_WR_DATA, 0xDF},

	{DISPCTRL_WR_CMND, 0x2B},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x00},
	{DISPCTRL_WR_DATA, 0x03},
	{DISPCTRL_WR_DATA, 0x1F},

	{DISPCTRL_WR_CMND, 0x3A},
	{DISPCTRL_WR_DATA, 0x77},

	{DISPCTRL_WR_CMND, 0x29},
	{DISPCTRL_SLEEP_MS, 10},

	{DISPCTRL_WR_CMND, 0x53},
	{DISPCTRL_WR_DATA, 0x2C},

	{DISPCTRL_LIST_END, 0}
};

void nt35510_winset(char *msgData, DISPDRV_WIN_t *p_win)
{
	int i = 0;
	msgData[i++] = 5; /* Length of the sequence below */
	msgData[i++] = MIPI_DCS_SET_COLUMN_ADDRESS;
	msgData[i++] = (p_win->l & 0xFF00) >> 8;
	msgData[i++] = (p_win->l & 0x00FF);
	msgData[i++] = (p_win->r & 0xFF00) >> 8;
	msgData[i++] = (p_win->r & 0x00FF);

	msgData[i++] = 5; /* Length of the sequence below */
	msgData[i++] = MIPI_DCS_SET_PAGE_ADDRESS;
	msgData[i++] = (p_win->t & 0xFF00) >> 8;
	msgData[i++] = (p_win->t & 0x00FF);
	msgData[i++] = (p_win->b & 0xFF00) >> 8;
	msgData[i++] = (p_win->b & 0x00FF);
	msgData[i++] = 0;

	if (i != NT35510_UPDT_WIN_SEQ_LEN)
		pr_err("nt35510_winset msg len incorrect!\n");
}

__initdata struct lcd_config nt35510_cfg = {
	.name = "NT35510",
	.mode_supp = LCD_CMD_ONLY,
	.phy_timing = &nt35510_timing[0],
	.max_lanes = 2,
	.max_hs_bps = 550000000,
	.max_lp_bps = 5000000,
	.phys_width = 52,
	.phys_height = 87,
	.init_cmd_seq = &nt35510_init_panel_cmd[0],
	.init_vid_seq = NULL,
	.slp_in_seq = &nt35510_slp_in[0],
	.slp_out_seq = &nt35510_slp_out[0],
	.scrn_on_seq = &nt35510_scrn_on[0],
	.scrn_off_seq = &nt35510_scrn_off[0],
	.id_seq = &nt35510_id[0],
	.verify_id = false,
	.updt_win_fn = nt35510_winset,
	.updt_win_seq_len = NT35510_UPDT_WIN_SEQ_LEN,
	.vid_cmnds = false,
	.vburst = false,
	.cont_clk = false,
	.hs = 0,
	.hbp = 0,
	.hfp = 0,
	.vs = 0,
	.vbp = 0,
	.vfp = 0,
};

#endif
