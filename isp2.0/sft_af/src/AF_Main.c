 /*--------------------------------------------------------------------------
 Confidential
                    SiliconFile Technologies Inc.

                   Copyright (c) 2015 SFT Limited
                          All rights reserved
 *
 * Project  : SPRD_AF
 * Filename : AF_Main.c
 *
 * $Revision: v1.00
 * $Author 	: silencekjj 
 * $Date 	: 2015.02.11
 *
 * --------------------------------------------------------------------------
 * Feature 	: DW9804(VCM IC)
 * Note 	: vim(ts=4, sw=4)
 * ------------------------------------------------------------------------ */
#ifndef	__AF_MAIN_C__
#define	__AF_MAIN_C__

/* ------------------------------------------------------------------------ */

#include "../inc/AF_Lib.h"
#include "../inc/AF_Main.h"
#include "../inc/AF_Control.h"
#include <stdio.h>

/* ------------------------------------------------------------------------ */

///////////////////////////////////////////////////////////
///////////////////////Get FV()////////////////////////////
///////////////////////////////////////////////////////////

BYTE AF_GetFv(sft_af_handle_t handle)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;
#ifdef FILTER_SPRD
	DWORD 		dwValue[150]= {0,};
#else
	DWORD 		dwValue[50]= {0,};
#endif
	BYTE i,j,k;

	af_ops->cb_get_afm_type1_statistic(handle, &dwValue[0]);
	af_ops->cb_get_afm_type2_statistic(handle, &dwValue[25]);
#ifdef FILTER_SPRD
	af_ops->cb_get_sp_afm_statistic(handle, &dwValue[50]);
#endif
	//for(i=0;i<6;i++){
	//	for(j=2;j<3;j++){
//			for(k=0;k<150;k++){
//				SFT_AF_LOGE("DDBBGG : %0.8x\n",dwValue[k]);
//			}
//		}
//	}

	if(afv->dwError == dwValue[0])
		return 1;
	else
		afv->dwError = dwValue[0];

	for(i=0;i<10;i++){
		dwValue[10+i] = dwValue[25+i];
		fv->dwFvDbg[0][i][afv->bDbgCnt] = dwValue[i];
		fv->dwFvDbg[1][i][afv->bDbgCnt] = dwValue[10+i];
	}
#ifdef FILTER_SPRD
	for(i=0;i<10;i++){
		dwValue[20+i] = dwValue[50+i];
		dwValue[30+i] = dwValue[75+i];
		fv->dwFvDbg[2][i][afv->bDbgCnt] = dwValue[20+i];
		fv->dwFvDbg[3][i][afv->bDbgCnt] = dwValue[30+i];
	}
	for(i=0;i<10;i++){
		dwValue[40+i] = dwValue[100+i];
		dwValue[50+i] = dwValue[125+i];
		fv->dwFvDbg[4][i][afv->bDbgCnt] = dwValue[40+i];
		fv->dwFvDbg[5][i][afv->bDbgCnt] = dwValue[50+i];
	}
#endif
	fv->wMtDbg[afv->bDbgCnt] = afCtl->wMtValue;
	afv->bDbgCnt++;
	if(afv->bDbgCnt == 100)
		afv->bDbgCnt = 0;

//	if(afCtl->bCtl[7] & ENB7_DEBUG_AF_ORG){
//		;
//	}else{
//		for(i=0;i<2;i++){
//			k = i * REGION_NUM_ISP;
//			for(j=0;j<10;j++){
//				// First Scan
//				;
//				// Fine Scan
//				;
//			}
//		}
//	}
	//for(i=0;i<5;i++){
	//	SFT_AF_LOGE("SAF_DBG = %x, %x",fv->dwFirstFv[0][0][i],fv->dwFirstFv[1][0][i]);
	//	SFT_AF_LOGE("SAF_DBG Org = %x",dwValue[i]);
	//}
	AF_SaveFv(afCtl, afv, fv, &dwValue[0], af_ops);
	//SFT_AF_LOGE("SAF_DBG CurStep %d", afv->bCurStep);
	return 0;
}
 
void AF_SetFvTh(sft_af_handle_t handle, BYTE bCAF)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;
	WORD 		i, j, bScene=0;
	struct win_coord	region[25];
	WORD 	wSize[2]; // Addr : First Regs of Region1
	SFT_AF_LOGE("CAF_Touch 11 %x", afCtl->bCtl[1]);

	if(bCAF){
		goto GET_CUR_SCENE;
		afv->bStatus[1] = 0;
	}
	// Get Scene	
	if(afCtl->bCtl[4] & ENB4_MANUAL_SCENE){
		bScene = afCtl->bCtl[4] & ENB4_SEL_SCENE;
		if(bScene == 3)		
			bScene = 2;
	}else{
		GET_CUR_SCENE:
		af_ops->cb_get_cur_env_mode(handle, &bScene);
	}
	
	// Save Scene 
	afv->bStatus[1]  |= bScene;

	// VCM Initial
//	AF_Write_I2C(handle, afCtl->bDacId, 0x02, afCtl->bVCM_Ctl[bScene][0]&0x03);
//	AF_Write_I2C(handle, afCtl->bDacId, 0x06, afCtl->bVCM_Ctl[bScene][0]&0xC0);
//	AF_Write_I2C(handle, afCtl->bDacId, 0x07, afCtl->bVCM_Ctl[bScene][1]);

	// Filter Initial
	af_ops->cb_set_afm_spsmd_type(handle, afCtl->bSetFilter[bScene] & FILTER_SPSMD_TYPE);

	if(afCtl->bSetFilter[bScene] & FILTER_SPSMD_RGT_BOT)
		af_ops->cb_set_afm_spsmd_rtgbot_enable(handle, TRUE);
	else
		af_ops->cb_set_afm_spsmd_rtgbot_enable(handle, FALSE);

	if(afCtl->bSetFilter[bScene] & FILTER_SPSMD_DIAGONAL)
		af_ops->cb_set_afm_spsmd_diagonal_enable(handle, TRUE);
	else
		af_ops->cb_set_afm_spsmd_diagonal_enable(handle, FALSE);

	if(afCtl->bSetFilter[bScene] & FILTER_SPSMD_CAL_MODE)
		af_ops->cb_set_afm_spsmd_cal_mode(handle, TRUE);
	else
		af_ops->cb_set_afm_spsmd_cal_mode(handle, FALSE);
	
	if(afCtl->bSetFilter[bScene] & FILTER_SOBEL_SIZE)
		af_ops->cb_set_afm_sobel_type(handle, TRUE);
	else
		af_ops->cb_set_afm_sobel_type(handle, FALSE);
	
	if(afCtl->bSetFilter[bScene] & FILTER_SEL_FILTER1)
		af_ops->cb_set_afm_sel_filter1(handle, TRUE);
	else
		af_ops->cb_set_afm_sel_filter1(handle, FALSE);

	if(afCtl->bSetFilter[bScene] & FILTER_SEL_FILTER2)
		af_ops->cb_set_afm_sel_filter2(handle, TRUE);
	else
		af_ops->cb_set_afm_sel_filter2(handle, FALSE);

	// Set Step Max/Min/Size
	//if(afCtl->bCtl[0] & ENB0_USE_OTP){			
	//	if(gOtp.bAfCtl1 & 0x02)
	//		afv.bSetStep[FIRST_MIN_STEP] = gOtp.bAfMinVal;
	//	if(gOtp.OtpData.bAfCtl1 & 0x04)
	//		afv.bSetStep[FIRST_MAX_STEP] = gOtp.bAfMaxVal;
	//	afv.bSetStep[FIRST_SIZE_STEP] = afCtl->bStep[bScene][FIRST_SIZE_STEP];	 
	//}else{
		for(i=0;i<3;i++)							
			afv->bSetStep[i] = afCtl->bStep[bScene][i];	 
	//}

	if(afCtl->bCtl[6] & ENB6_ADT_FPS_AF)
		AF_ADT_FPS_AF(afCtl, afv);

	for(i=0;i<2;i++){
		for(j=0;j<REGION_NUM;j++){
			afv->dwMaxFv[i][j] 	= (DWORD)afCtl->bMaxFvTh[bScene][i][j] << afCtl->bMaxFvRatio[bScene][i][j];
			if(afCtl->bCtl[1] & ENB1_TOUCH){
				afv->dwMinFv[i][j] 	= (DWORD)afCtl->bTouchMinFvTh[bScene][i][j] << afCtl->bMinFvRatio[bScene][i][j];
				if(j == REGION2)
					break;
			}else			
				afv->dwMinFv[i][j] 	= (DWORD)afCtl->bMinFvTh[bScene][i][j] << afCtl->bMinFvRatio[bScene][i][j];
		}
	}

	// Set Clip Ths
	if((bScene == DAK_SCENE)&&(afv->bCAFInfo[0] < 14)){
		af_ops->cb_set_afm_spsmd_threshold(handle, afCtl->wMaskClipTh[3][0], afCtl->wMaskClipTh[3][1]); 
		af_ops->cb_set_afm_sobel_threshold(handle, afCtl->wMaskClipTh[3][2], afCtl->wMaskClipTh[3][3]); 
	//	afCtl->bOffset = 0x60;
	}else{
		af_ops->cb_set_afm_spsmd_threshold(handle, afCtl->wMaskClipTh[bScene][0], afCtl->wMaskClipTh[bScene][1]); 
		af_ops->cb_set_afm_sobel_threshold(handle, afCtl->wMaskClipTh[bScene][2], afCtl->wMaskClipTh[bScene][3]); 
	//	afCtl->bOffset = 0x00;
	}
// 	for(i=0;i<8;i++){
// 		;//(*(volatile BYTE XDATA*)(ISP_REG_ADDR+0x2455+i)) = afCtl->bMaskClipTh[bScene][i];
// 		if(i>3){
// 			if(afCtl->bCtl[1] & ENB1_TOUCH)
// 				;//(*(volatile BYTE XDATA*)(ISP_REG_ADDR+0x2455+i)) = 0;
// 		}
// 	}


	// Set Window
	if(afCtl->bCtl[1] & ENB1_TOUCH){
		SFT_AF_LOGE("REGION Touch");
		af_ops->cb_set_active_win(handle, 0x003F);
		af_ops->cb_get_isp_size(handle, &wSize[1], &wSize[0]);	//handle, width, height
		AF_Touch_Initial(afCtl, afv, af_cxt->win_pos[0], &region[0], &wSize[0]);
		af_ops->cb_set_afm_win(handle, region);
		SFT_AF_LOGE("TOUCH_DBG %d %d", afCtl->wTouchPoint[AXIS_X], afCtl->wTouchPoint[AXIS_Y]);

		SFT_AF_LOGE("TOUCH_DBG X:%d, Y:%d", region[4].start_x + ((region[4].end_x - region[4].start_x)/2), region[4].start_y + ((region[4].end_y - region[4].start_y)/2));
		SFT_AF_LOGE("TOUCH_DBG X:%d-%d, Y:%d-%d", region[4].start_x, region[4].end_x, region[4].start_y, region[4].end_y);
	//}else if(afv->bCAFStatus[1] & STATUS0_CAF_FD_AF){
	//	af_ops->cb_set_active_win(handle, 0x001F);
	//	af_ops->cb_get_isp_size(handle, &wSize[1], &wSize[0]);	//handle, width, height
	//	if(afv->bCAFStatus[1] & STATUS0_CAF_FD_AF){
	//		af_cxt->win_pos[0].start_x	= afv->fd_Data.wRectFaceSize[afv->fd_Data.bBigSizeHist[0]][RECT_LEFT  ];
	//		af_cxt->win_pos[0].start_y	= afv->fd_Data.wRectFaceSize[afv->fd_Data.bBigSizeHist[0]][RECT_TOP   ];
	//		af_cxt->win_pos[0].end_x	= afv->fd_Data.wRectFaceSize[afv->fd_Data.bBigSizeHist[0]][RECT_RIGHT ];  
	//		af_cxt->win_pos[0].end_y	= afv->fd_Data.wRectFaceSize[afv->fd_Data.bBigSizeHist[0]][RECT_BOTTOM]; 
	//	}
	//	AF_Touch_Initial(afCtl, afv, af_cxt->win_pos[0], &region[0], &wSize[0]);
	//	af_ops->cb_set_afm_win(handle, region);
	}else{
		if(bCAF)
			i = 0x003F;
		else
			i = afCtl->wEnbRegion[bScene];
		af_ops->cb_set_active_win( handle, i);
		af_ops->cb_set_afm_win(handle, afCtl->winRegion[bScene]);			
	}
	SFT_AF_LOGE("REGION SAF %x", i);

	if(!bCAF){
		AF_Start_Single(afCtl, afv, fv);
		AF_CalcPreFrameSkipCnt(afCtl, afv);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
BYTE AF_ISR(sft_af_handle_t handle)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct 	sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;
	BYTE 	i, bScene;
	WORD	wSize[2], wFDSize[2];
	BYTE bTransfer[4] = {0,};

	SFT_AF_LOGE("######## ISR START ########");
	SFT_AF_LOGE("######## DBG_MOTOR : Pos %x ########", afCtl->wMtValue );

//	AF_Main_ISR(afCtl, afv, fv,  bControl);
	afv->bFrameCount++;
	if(afv->bFrameCount > 200)
		afv->bFrameCount = 200;

	if(afv->bStatus[5] & FLAG5_MOTOR_MOVING){
		goto MANUAL_MT;
	}

	if(afCtl->bCtl[0] & ENB0_AF){
		af_ops->cb_get_cur_env_mode(handle, &bScene);
		if(AF_Prepare(afCtl, afv, af_cxt->cur_fps, af_cxt->ae_cur_lum, bScene)){
			SFT_AF_LOGE("DBG_FRAMESKIP %d, %d, %x",afv->bFrameSkip, afCtl->bRecordTouchFrameSkip, afv->bCAFStatus[1]);
			goto MANUAL_MT;
		}
		SFT_AF_LOGE("AE_LUM %d", afv->bCAFInfo[0]);

		if(afv->bState == STATE_OVER){
			if(afv->bWaitCapture){
				afv->bWaitCapture--;
				if(afv->bWaitCapture)
					goto MANUAL_MT;
			}

			if(afCtl->bCtl[1] & ENB1_TOUCH){
				AF_CAF_Restart(afCtl, afv, fv, 1);
				af_ops->cb_unlock_ae(handle);
				af_ops->cb_unlock_awb(handle);
			}
			//afv->bISRCtrl = 0;
			SFT_AF_LOGE("CAF_Touch_Startx %x", afCtl->bCtl[1]);
			AF_Over(afCtl, afv);
			SFT_AF_LOGE("CAF_Touch_Startx %x", afCtl->bCtl[1]);
			if(afCtl->bCtl[7] & ENB7_WRITE_AF_DEBUG)
				AF_Debug(afCtl, afv, fv);
			SFT_AF_LOGE("SAF_DBG AF_Finished %x, %x, Mt:%x",afv->bStatus[2], afv->bStatus[3], afCtl->wMtValue);
			SFT_AF_LOGE("SAF_DBG AF_Finished R1 Dn %x Up %x Pk %x R2 Dn %x Up %x Pk %x",afv->bSac[0],afv->bSac[1],afv->bSac[2],afv->bSac[6],afv->bSac[7],afv->bSac[8]);

			afv->bCAFStatus[0] = STATE_CAF_WAIT_LOCK;
			afv->bCac[0] = AF_CAF_Calc_FPS_Time(afCtl, afv, fv, 3500); // bCAFRestartCnt
			afv->bCac[1] = 2;	

			afv->bSystem = 0;
			if(afv->bStatus[0] & FLAG0_AF_FAIL)
				af_ops->cb_af_finish_notice(handle, 0);
			else
				af_ops->cb_af_finish_notice(handle, 1);
			return 1;
		}

		SFT_AF_LOGE("CAF_Touch %x, %x, %x - %x", afv->bCAFStatus[0], afv->bCAFStatus[1], afv->bCAFStatus[2], afCtl->bCtl[1]);
		if(afv->bState == STATE_NOTHING){
			if(afCtl->bCtl[0] & ENB0_SAF){
				if(afv->bFrameCount < afCtl->bWaitFirstStart)
					return;

				if(afCtl->bCtl[1] & ENB1_AF_START){	
					if(afCtl->bCtl[1] & ENB1_TOUCH){
						SFT_AF_LOGE("CAF_Touch_Ctl Start %x %x", afCtl->bCtl[1], afv->bCAFStatus[2]);
						if(afv->bCAFStatus[2] & STATUS1_CAF_RECORD){
							SFT_AF_LOGE("CAF_Touch_Ctl Start %x %x", afCtl->bCtl[1], afv->bCAFStatus[2]);
							afv->bCAFStatus[2] 	|= STATUS1_CAF_RECORD_TOUCH;
							goto START_TOUCH_CAF;
						}
					}

					AF_ResetReg(afv, fv);
					AF_SetFvTh(handle, 0);
					AF_CalcStepFirst(afCtl, afv, bScene);

					//	afv->bState = STATE_INITIAL;
					//	AF_InitAF(afCtl, afv, af_ops->cb_get_ae_status(handle), af_ops->cb_get_ae_lum(handle), af_ops->cb_get_awb_status(handle));
					if(afCtl->bCtl[0] & ENB0_AE_AWB_STOP){
						afv->bStatus[0] |= FLAG0_AE_AWB_STOP;
						af_ops->cb_lock_ae(handle);
						af_ops->cb_lock_awb(handle);
					}

					afv->bState = STATE_FIRST_PREFRAMESKIP;
					goto MANUAL_MT;
				}else if(afCtl->bCtl[2] & ENB2_MTINIT_FIRST){	
					afCtl->bCtl[2] &= ~ENB2_MTINIT_FIRST; 

					if(afv->bDbgCnt == 100)
						afv->bDbgCnt = 0;
					if(afv->bStatus[4] & FLAG4_INIT_POS_MOTOR){
						afv->bStatus[4] &= ~FLAG4_INIT_POS_MOTOR; 
						afCtl->wMtValue = afv->wPreLastMtValue;	
					}else{
						afv->bStatus[4] |= FLAG4_INIT_POS_MOTOR;
						afCtl->wMtValue = afCtl->bCtl[1] & ENB1_MACRO_MODE ? afv->bSetStep[FIRST_MAX_STEP]<<2 : afv->bSetStep[FIRST_MIN_STEP]<<2;
					}
					goto MANUAL_MT;
				}
			}
			if((afCtl->bCtl[1] & ENB1_FDAF) && (!afv->bState) && (afv->bCAFStatus[0] & STATE_CAF_LOCK)){
				// Get Info
				afv->fd_Data.bFace_Num = af_cxt->fd_info.face_num;
				af_ops->cb_get_isp_size(handle, &wSize[AXIS_X], &wSize[AXIS_Y]);	//handle, width, height
				wFDSize[AXIS_X] = af_cxt->fd_info.frame_width;
				wFDSize[AXIS_Y] = af_cxt->fd_info.frame_height;
				for(i=0;i<afv->fd_Data.bFace_Num;i++){
					afv->fd_Data.wRectFaceSize[i][RECT_LEFT  ]	= af_cxt->fd_info.face_info[i].sx; 
					afv->fd_Data.wRectFaceSize[i][RECT_TOP   ]	= af_cxt->fd_info.face_info[i].sy;
					afv->fd_Data.wRectFaceSize[i][RECT_RIGHT ]	= af_cxt->fd_info.face_info[i].ex;
					afv->fd_Data.wRectFaceSize[i][RECT_BOTTOM]	= af_cxt->fd_info.face_info[i].ey;
				}
				
				// Check	
				switch(AF_FD(afCtl, afv, &wSize[0], &wFDSize[0], af_ops))
				{
					case FD_NOTHING:
						goto END_FD;
						break;
					case FD_SCANNING:
						//goto START_TOUCH_CAF;
						break;
				}
			}
			END_FD:

			if((afCtl->bCtl[1] & ENB1_CAF) && (!afv->bState)){
				if(afv->bWaitCapture){
					afv->bWaitCapture--;
					if(afv->bWaitCapture)
						goto MANUAL_MT;
				}
				AF_GetFv(handle);
				AF_CAF(afCtl, afv, fv, af_ops->cb_get_ae_status(handle), af_ops);
				SFT_AF_LOGE("ROC_DBG State %x,  %x", afv->bCAFStatus[0], afv->bCAFStatus[1]);

				if(afv->bCAFStatus[0] & STATE_CAF_SCAN_INIT){
					START_TOUCH_CAF:
					AF_SetFvTh(handle, 1);
					if((afv->bCAFStatus[2] & STATUS1_CAF_RECORD_TOUCH) || (afv->bCAFStatus[2] & STATUS1_CAF_FDAF) ){
						AF_Touch_Record(afCtl, afv, fv);
						AF_CAF_Scan_Init(afCtl, afv, fv, bScene, af_ops);
						SFT_AF_LOGE("CAF_Touch_Ctl Start %x %x", afCtl->bCtl[1], afv->bCAFStatus[2]);
					}
					af_ops->cb_af_move_start_notice(handle);
			//		af_ops->cb_lock_ae(handle);
			//		af_ops->cb_lock_awb(handle);

					afv->bCAFStatus[0] = STATE_CAF_SCANNING;
				}else if(afv->bCAFStatus[0] & STATE_CAF_SCAN_FINISH){
			//		af_ops->cb_unlock_ae(handle);
			//		af_ops->cb_unlock_awb(handle);
				//	if(afv->bStatus[0] & FLAG0_AF_FAIL)
				//		af_ops->cb_af_finish_notice(handle, 0);
				//	else
				//		af_ops->cb_af_finish_notice(handle, 1);
			
				//	afv->bISRCtrl = 0;
					if(!(afv->bCAFStatus[2] & STATUS1_CAF_RECORD)){
						if(afv->bScene[0] == IND_SCENE)
							afv->bWaitCapture = 5;
						else if(afv->bScene[0] == DAK_SCENE)
							afv->bWaitCapture = 4;
						else
							afv->bWaitCapture = 6;
					}
					afv->bStatus[0] &= 0x3F;
					if(afCtl->bCtl[7] & ENB7_WRITE_AF_DEBUG)
						AF_Debug(afCtl, afv, fv);

					afv->bCAFStatus[0] = STATE_CAF_WAIT_LOCK;
				}else if(afv->bCAFStatus[0] & STATE_CAF_WAIT_LOCK){		
					if(afv->bWaitCapture)
						return;
					af_ops->cb_af_finish_notice(handle, 0);
					if(afv->bCAFStatus[2] & STATUS1_CAF_RECORD_TOUCH){
						afv->bCAFStatus[2] &= 0x7F;	// Erase STATUS1_CAF_RECORD_TOUCH
						AF_Mode_Record(afCtl, afv, 1);
						AF_Control_CAF(afCtl, afv, fv, 0);
						SFT_AF_LOGE("CAF_Touch_Ctl End %x %x", afCtl->bCtl[1], afv->bCAFStatus[2]);
					}else if(afv->bCAFStatus[2] & STATUS1_CAF_RECORD_TOUCH){
						afv->bCAFStatus[2] &= 0xEF;	// Erase STATUS1_CAF_RECORD_TOUCH
						AF_Mode_Record(afCtl, afv, 1);
						AF_Control_CAF(afCtl, afv, fv, 0);
						SFT_AF_LOGE("CAF_Touch_Ctl End %x %x", afCtl->bCtl[1], afv->bCAFStatus[2]);
					}
				}

				goto MANUAL_MT;
			}else{
				return;
			}
 		}

		if(afv->bState & STATE_PREFRAMESKIP){
			AF_CheckPreFrameSkip(afv);
			if(afv->bState & STATE_FIRST_PREFRAMESKIP){
				afCtl->wMtValue = afv->wFirstMt[0];
				goto MANUAL_MT;
			}
		}

		if(afv->bState & STATE_SCAN){
			SFT_AF_LOGE( "First Scan %x", afv->bState);
			if(afCtl->bCtl[2] & ENB2_CANCEL){
				afv->bState = STATE_OVER;
				return;
			}

			AF_GetFv(handle);
			
			if(afv->bState & STATE_FIRSTSCAN){				
				AF_FirstScan(afCtl, afv, fv, af_ops);
			}else{
				AF_FineScan(afCtl, afv, fv, af_ops); 
			}
		}

		if(afv->bState & STATE_FAIL_RETURN){
			AF_Fail_Return(afCtl, afv, 0);
			//AF_SoftLanding(handle);
			SFT_AF_LOGE( "Fail_Return %x, %d", afCtl->wMtValue, af_ops->cb_get_ae_lum(handle));
			goto MANUAL_MT;
		}

		if(afv->bState & STATE_INITIAL){
		//	if(afCtl->bCtl[1] & ENB1_TOUCH){
		//		afCtl->bCtl[1] &= ~ENB1_TOUCH;
		//	}

			// 1 : Stable 0 : Unstable
			AF_InitAF(afCtl, afv, af_ops->cb_get_ae_status(handle), af_ops->cb_get_ae_lum(handle), af_ops->cb_get_awb_status(handle));
			if(afv->bStatus[0] & FLAG0_AE_AWB_STOP){
				af_ops->cb_lock_ae(handle);
				af_ops->cb_lock_awb(handle);
			}
		}

		MANUAL_MT:
		AF_Control_Core(handle, afCtl, afv, 1, af_ops);
//		af_ops->cb_af_pos_update(handle, afCtl->wMtValue);
//		bTransfer[0] = 0x02;
//		af_ops->cd_sp_read_i2c(handle, 0x0c, &bTransfer[0], 1); 
//		bTransfer[2] = 0x03;
//		af_ops->cd_sp_read_i2c(handle, 0x0c, &bTransfer[2], 1); 
//		bTransfer[3] = 0x04;
//		af_ops->cd_sp_read_i2c(handle, 0x0c, &bTransfer[3], 1); 
//		SFT_AF_LOGE("CAF_READ %x %x %x %x", bTransfer[0], bTransfer[1], bTransfer[2], bTransfer[3]);

//		if(afv->bState)
//			SFT_AF_LOGE("######## State_DBG : State %d Mt %x ########",afv->bState, afCtl->wMtValue );
	//	FINISH_ISR:
	//	if(afv->bStatus[0] & 0x03){
	//		if(afv->bStatus[0] & FLAG0_AF_FAIL)
	//			return ISR_FINISHED_AF_FAIL;
	//		else
	//			return ISR_FINISHED_AF_PASS;
	//	}
		SFT_AF_LOGE( "Finish %d Frame... ", afv->bTotalAF );
	}
}

void AF_SoftLanding(sft_af_handle_t handle)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;
	BYTE 		i;

	afCtl->bCtl[6] |= ENB6_SOFTLANDING;
//	afCtl->wMtValue = afCtl->wRange[0] + 0x80;
//	usleep(afCtl->bSoftLand_Dly1 * 1000);
//	afCtl->wMtValue = afCtl->wRange[0];
//	usleep(afCtl->bSoftLand_Dly1 * 1000);
	afCtl->wMtValue &= 0x3E0;
	for(i=0;i<254;i++){
		if(!afCtl->wMtValue)
			break;		
		
		if(afCtl->wMtValue > afCtl->bSoftLand_Mt1)
			afCtl->wMtValue -= afCtl->bSoftLand_Mt1;
		else
			afCtl->wMtValue = 0;

		SFT_AF_LOGE( "SOFT_LAND %x", afCtl->wMtValue);
		AF_Control_Core(handle, afCtl, afv, 1, af_ops);
		af_ops->cb_af_pos_update(handle, afCtl->wMtValue);
		usleep(afCtl->bSoftLand_Dly1 * 1000);
	
	//	if(afCtl->wMtValue > (afv->bSetStep[MIN_STEP]<<2))
	//		afCtl->wMtValue -= afCtl->bSoftLand_Mt1;
	//	else{
	//		AF_Control_Core(handle, afCtl, afv, 1, af_ops);
	//		af_ops->cb_af_pos_update(handle, afCtl->wMtValue);
	//		return;
	//	}

	//	SFT_AF_LOGE( "SOFT_LAND %x", afCtl->wMtValue);
	//	AF_Control_Core(handle, afCtl, afv, 1, af_ops);
	//	af_ops->cb_af_pos_update(handle, afCtl->wMtValue);
	//	usleep(afCtl->bSoftLand_Dly1 * 1000);
	
	}
}

void AF_SetDefault(sft_af_handle_t handle)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	AFDATA 		*afCtl 	= &af_cxt->afCtl;
	AFDATA_RO 	*afv 	= &af_cxt->afv;
	AFDATA_FV	*fv 	= &af_cxt->fv;
	GLOBAL_OTP	*gOtp 	= &af_cxt->gOtp;
	struct sft_af_ctrl_ops	*af_ops = &af_cxt->af_ctrl_ops;
	BYTE bTransfer[2] = {0,}, i=1, j=1;
	int wTmp;

	memset(afCtl, 0, sizeof(AFDATA));
	memset(afv,   0, sizeof(AFDATA_RO));
	memset(fv,    0, sizeof(AFDATA_FV));

//	af_ops->cb_af_move_start_notice(handle); 
   	
	AF_LoadReg(afCtl, afv, fv);

	if(afCtl->bCtl[0] & ENB0_USE_OTP){
		bTransfer[0] = 0x00;
		bTransfer[1] = 0x00;
		af_ops->cb_sp_write_i2c(handle, 0x58, &bTransfer[0], 2);
		usleep(1000);

		bTransfer[0] = 0x01;
		bTransfer[1] = 0x0c;
		af_ops->cd_sp_read_i2c(handle, 0x58, &bTransfer[0], 2);
		afCtl->wRange[1] = bTransfer[0];
		bTransfer[0] = 0x01;
		bTransfer[1] = 0x0d;
		af_ops->cd_sp_read_i2c(handle, 0x58, &bTransfer[0], 2);
		afCtl->wRange[1] += bTransfer[0]<<8;
		if(!afCtl->wRange[1] || (afCtl->wRange[1] >= 0x3FF)){
			afCtl->wRange[1] = afCtl->bStep[OUT_SCENE][FIRST_MAX_STEP]<<2;
			i = 0;
		}

		bTransfer[0] = 0x01;
		bTransfer[1] = 0x10;
		af_ops->cd_sp_read_i2c(handle, 0x58, &bTransfer[0], 2);
		afCtl->wRange[0] = bTransfer[0];
		bTransfer[0] = 0x01;
		bTransfer[1] = 0x11;
		af_ops->cd_sp_read_i2c(handle, 0x58, &bTransfer[0], 2);
		afCtl->wRange[0] += bTransfer[0]<<8;
		if(!afCtl->wRange[0] || (afCtl->wRange[0] >= 0x3FF)){
			afCtl->wRange[0] = afCtl->bStep[OUT_SCENE][FIRST_MIN_STEP]<<2;
			j = 0;
		}
		
		if(i){
			wTmp = (afCtl->wRange[1] / 4) + afCtl->bStepOffset[1];
			if(wTmp > 0xFF)
				wTmp = 0xFF;
			afCtl->bStep[OUT_SCENE][FIRST_MAX_STEP ]	= wTmp;	//0xB6; 
			afCtl->bStep[IND_SCENE][FIRST_MAX_STEP ]	= wTmp;	//0xB6; 
			afCtl->bStep[DAK_SCENE][FIRST_MAX_STEP ]	= wTmp;	//0x9C; 
		}

		
		if(j){
			wTmp = (afCtl->wRange[0] / 4) - afCtl->bStepOffset[0];
			if(wTmp < 0)
				wTmp = 0;
			afCtl->bStep[OUT_SCENE][FIRST_MIN_STEP ]	= wTmp;	//0x2C;	   
			afCtl->bStep[IND_SCENE][FIRST_MIN_STEP ]	= wTmp;	//0x2C;	  	
			afCtl->bStep[DAK_SCENE][FIRST_MIN_STEP ]	= wTmp;	//0x2C;	 //0x3C 	
		}
		SFT_AF_LOGE("######## CAF_DBG Read %x, %x ########", afCtl->wRange[0], afCtl->wRange[1]);
		SFT_AF_LOGE("######## CAF_DBG Read %x, %x ########", afCtl->bStep[OUT_SCENE][FIRST_MIN_STEP], afCtl->bStep[OUT_SCENE][FIRST_MAX_STEP]);
	}

	//usleep(100);
	//bTransfer[0] = 0x02;
	//bTransfer[1] = 0x01;
	//af_ops->cb_sp_write_i2c(handle, 0x0c, &bTransfer[0], 2); 
	//bTransfer[0] = 0x02;
	//bTransfer[1] = 0x00;
	//af_ops->cb_sp_write_i2c(handle, 0x0c, &bTransfer[0], 2); 
	//usleep(100);
	bTransfer[0] = 0x02;
	bTransfer[1] = 0x02;
	af_ops->cb_sp_write_i2c(handle, 0x0c, &bTransfer[0], 2); 
	usleep(100);
	bTransfer[0] = 0x06;
	bTransfer[1] = 0x61;
	af_ops->cb_sp_write_i2c(handle, 0x0c, &bTransfer[0], 2);  
	usleep(100);
	bTransfer[0] = 0x07;
	bTransfer[1] = 0x38;
	af_ops->cb_sp_write_i2c(handle, 0x0c, &bTransfer[0], 2);  

//	afCtl->wMtValue = afCtl->wRange[0];
//	afCtl->wMtValue = afCtl->wRange[0];
//	AF_Control_Core(handle, afCtl, afv, 1, af_ops);
//	af_ops->cb_af_pos_update(handle, afCtl->wMtValue);
}

#endif
/* ------------------------------------------------------------------------ */
/* end of this file															*/
/* ------------------------------------------------------------------------ */
