/**
 * @copyright (c) 2003 - 2022, Goodix Co., Ltd. All rights reserved.
 * 
 * @file    gh3x2x_demo.c
 * 
 * @brief   gh3x2x driver example
 * 
 * @author  Gooidx Iot Team
 * 
 */

/* includes */
#include "stdint.h"
#include "string.h"
#include "gh3x2x_demo.h"
#include "gh3x2x_demo_config.h"
#include "gh3x2x_demo_inner.h"
#include "gh3x2x_demo_soft_adt.h"
#include "gh3x2x_demo_version.h"
#include "gh3x2x_drv.h"

#if (__DRIVER_LIB_MODE__ == __DRV_LIB_WITH_ALGO__)
#include "gh3x2x_demo_algo_call.h"
#endif

#define GH3X2X_INT_PROCESS_SUCCESS              (0)         /**< gh3x2x interrupt process success */
#define GH3X2X_INT_PROCESS_REPEAT               (1)         /**< need process gh3x2x interrupt process again */


#if (__SUPPORT_HARD_ADT_CONFIG__)
#define __GH3X2X_ADT_EVENT_MASK__               (GH3X2X_IRQ_MSK_WEAR_ON_BIT | GH3X2X_IRQ_MSK_WEAR_OFF_BIT)
#else
#define __GH3X2X_ADT_EVENT_MASK__               (0)
#endif


#if (__ADT_ONLY_PARTICULAR_WM_CONFIG__)
GU16 g_usCurrentConfigListFifoWmBak = 0;
extern GU16 g_usCurrentFiFoWaterLine;
#endif


#if (__FUNC_TYPE_ECG_ENABLE__)
#define __GH3X2X_ECG_EVENT_MASK__               (GH3X2X_IRQ_MSK_LEAD_ON_DET_BIT)
#else
#define __GH3X2X_ECG_EVENT_MASK__               (0)
#endif

/* GH3X2X event that need to process */
#define __GH3X2X_EVENT_PROCESS_MASK__   (\
                                            __GH3X2X_ADT_EVENT_MASK__ | __GH3X2X_ECG_EVENT_MASK__ | \
                                            GH3X2X_IRQ_MSK_FIFO_WATERMARK_BIT | GH3X2X_IRQ_MSK_FIFO_FULL_BIT | \
                                            GH3X2X_IRQ_MSK_TUNNING_FAIL_BIT | GH3X2X_IRQ_MSK_TUNNING_DONE_BIT | \
                                            GH3X2X_IRQ_MSK_CHIP_RESET_BIT \
                                        )
#define DRV_LIB_REG_CFG_EMPTY           (0x00)    //there is no reg config array loaded into driver lib
#define DRV_LIB_REG_CFG_MIN_LEN         (0x5)     //minimum length of reg config array

int (*g_pPrintfUser)(const char *, ...) = 0;
int (*g_pSnprintfUser)(char *str, size_t size, const char *format, ...) = 0;




#if __GS_GYRO_ENABLE__
const GU8 g_uchGyroEnable = 1;
#else
const GU8 g_uchGyroEnable = 0;
#endif
/// gsensor fifo index
static GU16 gsensor_soft_fifo_buffer_index = 0;

/// gsensor fifo
static STGsensorRawdata gsensor_soft_fifo_buffer[__GSENSOR_DATA_BUFFER_SIZE__ + __GS_EXTRA_BUF_LEN__];
#if __CAP_ENABLE__
static GU16 cap_soft_fifo_buffer_index = 0;
static STCapRawdata cap_soft_fifo_buffer[__CAP_DATA_BUFFER_SIZE__];
#else
static GU16 cap_soft_fifo_buffer_index = 0;
static STCapRawdata* cap_soft_fifo_buffer = 0;
#endif
#if __TEMP_ENABLE__
static GU16 temp_soft_fifo_buffer_index = 0;
static STTempRawdata temp_soft_fifo_buffer[__TEMP_DATA_BUFFER_SIZE__];
#else
static GU16 temp_soft_fifo_buffer_index = 0;
static STTempRawdata* temp_soft_fifo_buffer = 0;
#endif

GU32 g_unDemoFuncMode = 0;
GU8 g_uchDemoWorkMode = GH3X2X_DEMO_WORK_MODE_MCU_ONLINE;
GU8 g_uchDemoAlgoEnableFlag = 1;
#if __SLOT_SEQUENCE_DYNAMIC_ADJUST_EN__
GU16 g_usGh3x2xSlotTime[8];
#endif
#if (__FUNC_TYPE_SOFT_ADT_ENABLE__)
GU8 g_uchHardAdtFuncStatus = 0;    //0: stop   1: start
GU8 g_uchVirtualAdtTimerCtrlStatus = 0;  //0: stop  1: running
#endif

#if (__SUPPORT_ELECTRODE_WEAR_STATUS_DUMP__)
GU8 g_uchGh3x2xElectrodeWearStatus;
#endif
EMWearDetectType g_uchWearDetectStatus = WEAR_DETECT_WEAR_ON;
/// save bg cancel value of every channel
GU8 g_puchGainBgCancelRecord[CHANNEL_MAP_ID_NUM];
GU8 g_puchTiaGainAfterSoftAgc[CHANNEL_MAP_ID_NUM];
/// save current of drv0 and drv1
GU16 g_pusDrvCurrentRecord[CHANNEL_MAP_ID_NUM];


#if __FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
#if __FUNC_TYPE_HR_ENABLE__
GU32 g_pun_HR_FrameIncompleteRawdata[GH3X2X_HR_CHNL_NUM];
GU32 g_un_HR_FrameIncompleteChnlMapBit;
GU8 g_puch_HR_FrameLastGain[GH3X2X_HR_CHNL_NUM];
GU32 g_un_HR_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_HR;
#endif
#if __FUNC_TYPE_HRV_ENABLE__
GU32 g_pun_HRV_FrameIncompleteRawdata[GH3X2X_HRV_CHNL_NUM];
GU32 g_un_HRV_FrameIncompleteChnlMapBit;
GU8 g_puch_HRV_FrameLastGain[GH3X2X_HRV_CHNL_NUM];
GU32 g_un_HRV_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_HRV;
#endif
#if __FUNC_TYPE_HSM_ENABLE__
GU32 g_pun_HSM_FrameIncompleteRawdata[GH3X2X_HSM_CHNL_NUM];
GU32 g_un_HSM_FrameIncompleteChnlMapBit;
GU8 g_puch_HSM_FrameLastGain[GH3X2X_HSM_CHNL_NUM];
GU32 g_un_HSM_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_HSM;
#endif
#if __FUNC_TYPE_SPO2_ENABLE__
GU32 g_pun_SPO2_FrameIncompleteRawdata[GH3X2X_SPO2_CHNL_NUM];
GU32 g_un_SPO2_FrameIncompleteChnlMapBit;
GU8 g_puch_SPO2_FrameLastGain[GH3X2X_SPO2_CHNL_NUM];
GU32 g_un_SPO2_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_SPO2;
#endif
#if __FUNC_TYPE_ECG_ENABLE__
GU32 g_pun_ECG_FrameIncompleteRawdata[GH3X2X_ECG_CHNL_NUM];
GU32 g_un_ECG_FrameIncompleteChnlMapBit;
GU8 g_puch_ECG_FrameLastGain[GH3X2X_ECG_CHNL_NUM];
GU32 g_un_ECG_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_ECG;
#endif
#if __FUNC_TYPE_PWTT_ENABLE__
GU32 g_pun_PWTT_FrameIncompleteRawdata[GH3X2X_PWTT_CHNL_NUM];
GU32 g_un_PWTT_FrameIncompleteChnlMapBit;
GU8 g_puch_PWTT_FrameLastGain[GH3X2X_PWTT_CHNL_NUM];
GU32 g_un_PWTT_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_PWTT;
#endif
#if __FUNC_TYPE_BT_ENABLE__
GU32 g_pun_BT_FrameIncompleteRawdata[GH3X2X_BT_CHNL_NUM];
GU32 g_un_BT_FrameIncompleteChnlMapBit;
GU8 g_puch_BT_FrameLastGain[GH3X2X_BT_CHNL_NUM];
GU32 g_un_BT_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_BT;
#endif
#if __FUNC_TYPE_RESP_ENABLE__
GU32 g_pun_RESP_FrameIncompleteRawdata[GH3X2X_RESP_CHNL_NUM];
GU32 g_un_RESP_FrameIncompleteChnlMapBit;
GU8 g_puch_RESP_FrameLastGain[GH3X2X_RESP_CHNL_NUM];
GU32 g_un_RESP_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_RESP;
#endif
#if __FUNC_TYPE_AF_ENABLE__
GU32 g_pun_AF_FrameIncompleteRawdata[GH3X2X_AF_CHNL_NUM];
GU32 g_un_AF_FrameIncompleteChnlMapBit;
GU8 g_puch_AF_FrameLastGain[GH3X2X_AF_CHNL_NUM];
GU32 g_un_AF_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_AF;
#endif
#if __FUNC_TYPE_BP_ENABLE__
GU32 g_pun_BP_FrameIncompleteRawdata[GH3X2X_BP_CHNL_NUM];
GU32 g_un_BP_FrameIncompleteChnlMapBit;
GU8 g_puch_BP_FrameLastGain[GH3X2X_BP_CHNL_NUM];
GU32 g_un_BP_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_BP;
#endif
#if __FUNC_TYPE_TEST_ENABLE__
GU32 g_pun_TEST1_FrameIncompleteRawdata[GH3X2X_TEST_CHNL_NUM];
GU32 g_un_TEST1_FrameIncompleteChnlMapBit;
GU8 g_puch_TEST1_FrameLastGain[GH3X2X_TEST_CHNL_NUM];
GU32 g_un_TEST1_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_TEST1;
GU32 g_pun_TEST2_FrameIncompleteRawdata[GH3X2X_TEST_CHNL_NUM];
GU32 g_un_TEST2_FrameIncompleteChnlMapBit;
GU8 g_puch_TEST2_FrameLastGain[GH3X2X_TEST_CHNL_NUM];
GU32 g_un_TEST2_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_TEST2;
#endif
#else
GU32 g_pun_MAIN_FUNC_FrameIncompleteRawdata[GH3X2X_MAIN_FUNC_CHNL_NUM];
GU32 g_un_MAIN_FUNC_FrameIncompleteChnlMapBit;
GU8 g_puch_MAIN_FUNC_FrameLastGain[GH3X2X_MAIN_FUNC_CHNL_NUM];
GU32 g_un_MAIN_FUNC_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_MAIN_FUNC;
#endif
#if __SUPPORT_HARD_ADT_CONFIG__
GU32 g_pun_ADT_FrameIncompleteRawdata[GH3X2X_ADT_CHNL_NUM];
GU32 g_un_ADT_FrameIncompleteChnlMapBit;
GU8 g_puch_ADT_FrameLastGain[GH3X2X_ADT_CHNL_NUM];
GU32 g_un_ADT_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_ADT;
#endif
#if __FUNC_TYPE_SOFT_ADT_ENABLE__
GU32 g_pun_SOFT_ADT_GREEN_FrameIncompleteRawdata[GH3X2X_SOFT_ADT_CHNL_NUM];
GU32 g_un_SOFT_ADT_GREEN_FrameIncompleteChnlMapBit;
GU8 g_puch_SOFT_ADT_GREEN_FrameLastGain[GH3X2X_SOFT_ADT_CHNL_NUM];
GU32 g_un_SOFT_ADT_GREEN_TimeStamp;
STGh3x2xDownSampleInfo g_stGh3x2xDownSampleInfo_SOFT_ADT_GREEN;
#endif




#if __FUNC_TYPE_HR_ENABLE__
GU8 g_pch_HR_ChnlMap[GH3X2X_HR_CHNL_NUM];
STGh3x2xFunctionInfo g_st_HR_FuncitonInfo;
STGh3x2xAlgoResult g_st_HR_AlgoRecordResult;
#endif
#if __FUNC_TYPE_HRV_ENABLE__
GU8 g_pch_HRV_ChnlMap[GH3X2X_HRV_CHNL_NUM];
STGh3x2xFunctionInfo g_st_HRV_FuncitonInfo;
STGh3x2xAlgoResult g_st_HRV_AlgoRecordResult;
#endif
#if __FUNC_TYPE_HSM_ENABLE__
GU8 g_pch_HSM_ChnlMap[GH3X2X_HSM_CHNL_NUM];
STGh3x2xFunctionInfo g_st_HSM_FuncitonInfo;
#endif
#if __FUNC_TYPE_SPO2_ENABLE__
GU8 g_pch_SPO2_ChnlMap[GH3X2X_SPO2_CHNL_NUM];
STGh3x2xFunctionInfo g_st_SPO2_FuncitonInfo;
STGh3x2xAlgoResult g_st_SPO2_AlgoRecordResult;
#endif
#if __FUNC_TYPE_ECG_ENABLE__
GU8 g_pch_ECG_ChnlMap[GH3X2X_ECG_CHNL_NUM];
STGh3x2xFunctionInfo g_st_ECG_FuncitonInfo;
#endif
#if __FUNC_TYPE_PWTT_ENABLE__
GU8 g_pch_PWTT_ChnlMap[GH3X2X_PWTT_CHNL_NUM];
STGh3x2xFunctionInfo g_st_PWTT_FuncitonInfo;
#endif
#if __FUNC_TYPE_BT_ENABLE__
GU8 g_pch_BT_ChnlMap[GH3X2X_BT_CHNL_NUM];
STGh3x2xFunctionInfo g_st_BT_FuncitonInfo;
#endif
#if __FUNC_TYPE_RESP_ENABLE__
GU8 g_pch_RESP_ChnlMap[GH3X2X_RESP_CHNL_NUM];
STGh3x2xFunctionInfo g_st_RESP_FuncitonInfo;
#endif
#if __FUNC_TYPE_AF_ENABLE__
GU8 g_pch_AF_ChnlMap[GH3X2X_AF_CHNL_NUM];
STGh3x2xFunctionInfo g_st_AF_FuncitonInfo;
#endif
#if __SUPPORT_HARD_ADT_CONFIG__
GU8 g_pch_ADT_ChnlMap[GH3X2X_ADT_CHNL_NUM];
STGh3x2xFunctionInfo g_st_ADT_FuncitonInfo;
#endif
#if __FUNC_TYPE_SOFT_ADT_ENABLE__
GU8 g_pch_SOFT_ADT_GREEN_ChnlMap[GH3X2X_SOFT_ADT_CHNL_NUM];
STGh3x2xFunctionInfo g_st_SOFT_ADT_GREEN_FuncitonInfo;
#endif
#if __FUNC_TYPE_SOFT_ADT_ENABLE__
GU8 g_pch_SOFT_ADT_IR_ChnlMap[GH3X2X_SOFT_ADT_CHNL_NUM];
STGh3x2xFunctionInfo g_st_SOFT_ADT_IR_FuncitonInfo;
#endif
#if __FUNC_TYPE_BP_ENABLE__
GU8 g_pch_BP_ChnlMap[GH3X2X_BP_CHNL_NUM];
STGh3x2xFunctionInfo g_st_BP_FuncitonInfo;
#endif
#if __FUNC_TYPE_TEST_ENABLE__
GU8 g_pch_TEST1_ChnlMap[GH3X2X_TEST_CHNL_NUM];
STGh3x2xFunctionInfo g_st_TEST1_FuncitonInfo;
GU8 g_pch_TEST2_ChnlMap[GH3X2X_TEST_CHNL_NUM];
STGh3x2xFunctionInfo g_st_TEST2_FuncitonInfo;
#endif

GU32 g_punGh3x2xFrameRawdata[GH3X2X_FUNC_CHNL_NUM_MAX];
#if __GS_GYRO_ENABLE__
GS16 g_psGh3x2xFrameGsensorData[6];
#else
GS16 g_psGh3x2xFrameGsensorData[3];
#endif
GU32 g_punGh3x2xFrameAgcInfo[GH3X2X_FUNC_CHNL_NUM_MAX];
GU32 g_punGh3x2xFrameFlag[8];
STGh3x2xAlgoResult g_stGh3x2xAlgoResult;

STCapRawdata g_pstGh3x2xFrameCapData;
STTempRawdata g_pstGh3x2xFrameTempData;



//HR data info
#if __FUNC_TYPE_HR_ENABLE__
const STGh3x2xFrameInfo g_stHR_FrameInfo = 
{
    .unFunctionID = GH3X2X_FUNCTION_HR,
    .pstFunctionInfo = &g_st_HR_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_HR_CHNL_NUM,
    .pchChnlMap = g_pch_HR_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .pstAlgoRecordResult = &g_st_HR_AlgoRecordResult,
#if !__FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
    .punIncompleteRawdata = g_pun_MAIN_FUNC_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_MAIN_FUNC_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_MAIN_FUNC_FrameLastGain,
    .punFrameCnt = &g_un_MAIN_FUNC_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_MAIN_FUNC,
#else
    .punIncompleteRawdata = g_pun_HR_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_HR_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_HR_FrameLastGain,
    .punFrameCnt = &g_un_HR_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_HR,
#endif
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xHR_FrameInfo = &g_stHR_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xHR_FrameInfo = 0;
#endif
//HRV data info
#if __FUNC_TYPE_HRV_ENABLE__
const STGh3x2xFrameInfo g_stHRV_FrameInfo =
{
    .unFunctionID = GH3X2X_FUNCTION_HRV,
    .pstFunctionInfo = &g_st_HRV_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_HRV_CHNL_NUM,
    .pchChnlMap = g_pch_HRV_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .pstAlgoRecordResult = &g_st_HRV_AlgoRecordResult,
#if !__FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
    .punIncompleteRawdata = g_pun_MAIN_FUNC_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_MAIN_FUNC_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_MAIN_FUNC_FrameLastGain,
    .punFrameCnt = &g_un_MAIN_FUNC_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_MAIN_FUNC,
#else
    .punIncompleteRawdata = g_pun_HRV_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_HRV_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_HRV_FrameLastGain,
    .punFrameCnt = &g_un_HRV_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_HRV,
#endif
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xHRV_FrameInfo = &g_stHRV_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xHRV_FrameInfo = 0;
#endif
//HSM data info
#if __FUNC_TYPE_HSM_ENABLE__
const STGh3x2xFrameInfo g_stHSM_FrameInfo =
{
    .unFunctionID = GH3X2X_FUNCTION_HSM,
    .pstFunctionInfo = &g_st_HSM_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_HSM_CHNL_NUM,
    .pchChnlMap = g_pch_HSM_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .pstAlgoRecordResult = 0,
#if !__FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
    .punIncompleteRawdata = g_pun_MAIN_FUNC_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_MAIN_FUNC_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_MAIN_FUNC_FrameLastGain,
    .punFrameCnt = &g_un_MAIN_FUNC_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_MAIN_FUNC,
#else
    .punIncompleteRawdata = g_pun_HSM_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_HSM_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_HSM_FrameLastGain,
    .punFrameCnt = &g_un_HSM_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_HSM,
#endif
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xHSM_FrameInfo = &g_stHSM_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xHSM_FrameInfo = 0;
#endif
//SPO2 data info
#if __FUNC_TYPE_SPO2_ENABLE__
const STGh3x2xFrameInfo g_stSPO2_FrameInfo = 
{
    .unFunctionID = GH3X2X_FUNCTION_SPO2,
    .pstFunctionInfo = &g_st_SPO2_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_SPO2_CHNL_NUM,
    .pchChnlMap = g_pch_SPO2_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .pstAlgoRecordResult = &g_st_SPO2_AlgoRecordResult,
#if !__FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
    .punIncompleteRawdata = g_pun_MAIN_FUNC_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_MAIN_FUNC_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_MAIN_FUNC_FrameLastGain,
    .punFrameCnt = &g_un_MAIN_FUNC_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_MAIN_FUNC,
#else
    .punIncompleteRawdata = g_pun_SPO2_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_SPO2_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_SPO2_FrameLastGain,
    .punFrameCnt = &g_un_SPO2_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_SPO2,
#endif
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xSPO2_FrameInfo = &g_stSPO2_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xSPO2_FrameInfo = 0;
#endif
//ECG data info
#if __FUNC_TYPE_ECG_ENABLE__
const STGh3x2xFrameInfo g_stECG_FrameInfo = 
{
    .unFunctionID = GH3X2X_FUNCTION_ECG,
    .pstFunctionInfo = &g_st_ECG_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_ECG_CHNL_NUM,
    .pchChnlMap = g_pch_ECG_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .pstAlgoRecordResult = 0,
#if !__FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
    .punIncompleteRawdata = g_pun_MAIN_FUNC_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_MAIN_FUNC_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_MAIN_FUNC_FrameLastGain,
    .punFrameCnt = &g_un_MAIN_FUNC_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_MAIN_FUNC,
#else
    .punIncompleteRawdata = g_pun_ECG_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_ECG_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_ECG_FrameLastGain,
    .punFrameCnt = &g_un_ECG_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_ECG,
#endif
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xECG_FrameInfo = &g_stECG_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xECG_FrameInfo = 0;
#endif
#if __FUNC_TYPE_PWTT_ENABLE__
const STGh3x2xFrameInfo g_stPWTT_FrameInfo = 
{
    .unFunctionID = GH3X2X_FUNCTION_PWTT,
    .pstFunctionInfo = &g_st_PWTT_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_PWTT_CHNL_NUM,
    .pchChnlMap = g_pch_PWTT_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .pstAlgoRecordResult = 0,
#if !__FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
    .punIncompleteRawdata = g_pun_MAIN_FUNC_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_MAIN_FUNC_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_MAIN_FUNC_FrameLastGain,
    .punFrameCnt = &g_un_MAIN_FUNC_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_MAIN_FUNC,
#else
    .punIncompleteRawdata = g_pun_PWTT_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_PWTT_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_PWTT_FrameLastGain,
    .punFrameCnt = &g_un_PWTT_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_PWTT,
#endif
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xECG_FrameInfo = &g_stECG_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xECG_FrameInfo = 0;
#endif
//BT data info
#if __FUNC_TYPE_BT_ENABLE__
const STGh3x2xFrameInfo g_stBT_FrameInfo =
{
    .unFunctionID = GH3X2X_FUNCTION_BT,
    .pstFunctionInfo = &g_st_BT_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_BT_CHNL_NUM,
    .pchChnlMap = g_pch_BT_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .pstAlgoRecordResult = 0,
#if !__FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
    .punIncompleteRawdata = g_pun_MAIN_FUNC_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_MAIN_FUNC_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_MAIN_FUNC_FrameLastGain,
    .punFrameCnt = &g_un_MAIN_FUNC_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_MAIN_FUNC,
#else
    .punIncompleteRawdata = g_pun_BT_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_BT_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_BT_FrameLastGain,
    .punFrameCnt = &g_un_BT_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_BT,
#endif
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xBT_FrameInfo = &g_stBT_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xBT_FrameInfo = 0;
#endif
//RESP data info
#if __FUNC_TYPE_RESP_ENABLE__
const STGh3x2xFrameInfo g_stRESP_FrameInfo =
{
    .unFunctionID = GH3X2X_FUNCTION_RESP,
    .pstFunctionInfo = &g_st_RESP_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_RESP_CHNL_NUM,
    .pchChnlMap = g_pch_RESP_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .pstAlgoRecordResult = 0,
#if !__FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
    .punIncompleteRawdata = g_pun_MAIN_FUNC_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_MAIN_FUNC_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_MAIN_FUNC_FrameLastGain,
    .punFrameCnt = &g_un_MAIN_FUNC_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_MAIN_FUNC,
#else
    .punIncompleteRawdata = g_pun_RESP_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_RESP_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_RESP_FrameLastGain,
    .punFrameCnt = &g_un_RESP_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_RESP,
#endif
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xRESP_FrameInfo = &g_stRESP_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xRESP_FrameInfo = 0;
#endif
//AF data info
#if __FUNC_TYPE_AF_ENABLE__
const STGh3x2xFrameInfo g_stAF_FrameInfo =
{
    .unFunctionID = GH3X2X_FUNCTION_AF,
    .pstFunctionInfo = &g_st_AF_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_AF_CHNL_NUM,
    .pchChnlMap = g_pch_AF_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .pstAlgoRecordResult = 0,
#if !__FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
    .punIncompleteRawdata = g_pun_MAIN_FUNC_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_MAIN_FUNC_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_MAIN_FUNC_FrameLastGain,
    .punFrameCnt = &g_un_MAIN_FUNC_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_MAIN_FUNC,
#else
    .punIncompleteRawdata = g_pun_AF_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_AF_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_AF_FrameLastGain,
    .punFrameCnt = &g_un_AF_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_AF,
#endif
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xAF_FrameInfo = &g_stAF_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xAF_FrameInfo = 0;
#endif
#if __FUNC_TYPE_BP_ENABLE__
const STGh3x2xFrameInfo g_stFPBP_FrameInfo =
{
    .unFunctionID = GH3X2X_FUNCTION_FPBP,
    .pstFunctionInfo = &g_st_BP_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_BP_CHNL_NUM,
    .pchChnlMap = g_pch_BP_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .pstAlgoRecordResult = 0,
#if !__FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
    .punIncompleteRawdata = g_pun_MAIN_FUNC_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_MAIN_FUNC_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_MAIN_FUNC_FrameLastGain,
    .punFrameCnt = &g_un_MAIN_FUNC_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_MAIN_FUNC,
#else
    .punIncompleteRawdata = g_pun_BP_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_BP_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_BP_FrameLastGain,
    .punFrameCnt = &g_un_BP_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_BP,
#endif
};
const STGh3x2xFrameInfo g_stPWA_FrameInfo =
{
    .unFunctionID = GH3X2X_FUNCTION_PWA,
    .pstFunctionInfo = &g_st_BP_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_PWA_CHNL_NUM,
    .pchChnlMap = g_pch_BP_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .pstAlgoRecordResult = 0,
#if !__FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
    .punIncompleteRawdata = g_pun_MAIN_FUNC_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_MAIN_FUNC_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_MAIN_FUNC_FrameLastGain,
    .punFrameCnt = &g_un_MAIN_FUNC_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_MAIN_FUNC,
#else
    .punIncompleteRawdata = g_pun_BP_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_BP_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_BP_FrameLastGain,
    .punFrameCnt = &g_un_BP_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_BP,
#endif
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xFPBP_FrameInfo = &g_stFPBP_FrameInfo;
//const STGh3x2xFrameInfo * const  g_pstGh3x2xPWA_FrameInfo = &g_stPWA_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xFPBP_FrameInfo = 0;
//const STGh3x2xFrameInfo * const  g_pstGh3x2xPWA_FrameInfo = 0;
#endif
#if __FUNC_TYPE_TEST_ENABLE__
const STGh3x2xFrameInfo g_stTEST1_FrameInfo =
{
    .unFunctionID = GH3X2X_FUNCTION_TEST1,
    .pstFunctionInfo = &g_st_TEST1_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_TEST_CHNL_NUM,
    .pchChnlMap = g_pch_TEST1_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .pstAlgoRecordResult = 0,
#if !__FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
    .punIncompleteRawdata = g_pun_MAIN_FUNC_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_MAIN_FUNC_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_MAIN_FUNC_FrameLastGain,
    .punFrameCnt = &g_un_MAIN_FUNC_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_MAIN_FUNC,
#else
    .punIncompleteRawdata = g_pun_TEST1_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_TEST1_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_TEST1_FrameLastGain,
    .punFrameCnt = &g_un_TEST1_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_TEST1,
#endif
};
const STGh3x2xFrameInfo g_stTEST2_FrameInfo =
{
    .unFunctionID = GH3X2X_FUNCTION_TEST2,
    .pstFunctionInfo = &g_st_TEST2_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_TEST_CHNL_NUM,
    .pchChnlMap = g_pch_TEST2_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .pstAlgoRecordResult = 0,
#if !__FUNC_RUN_SIMULTANEOUSLY_SUPPORT__
    .punIncompleteRawdata = g_pun_MAIN_FUNC_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_MAIN_FUNC_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_MAIN_FUNC_FrameLastGain,
    .punFrameCnt = &g_un_MAIN_FUNC_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_MAIN_FUNC,
#else
    .punIncompleteRawdata = g_pun_TEST2_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_TEST2_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_TEST2_FrameLastGain,
    .punFrameCnt = &g_un_TEST2_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_TEST2,
#endif
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xTEST1_FrameInfo = &g_stTEST1_FrameInfo;
//const STGh3x2xFrameInfo * const  g_pstGh3x2xTEST2_FrameInfo = &g_stTEST2_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xTEST1_FrameInfo = 0;
//const STGh3x2xFrameInfo * const  g_pstGh3x2xTEST2_FrameInfo = 0;
#endif
//ADT data info
#if __SUPPORT_HARD_ADT_CONFIG__
const STGh3x2xFrameInfo g_stADT_FrameInfo = 
{
    .unFunctionID = GH3X2X_FUNCTION_ADT,
    .pstFunctionInfo = &g_st_ADT_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_ADT_CHNL_NUM,
    .pchChnlMap = g_pch_ADT_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .punIncompleteRawdata = g_pun_ADT_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_ADT_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_ADT_FrameLastGain,
    .punFrameCnt = &g_un_ADT_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_ADT,
    .pstAlgoRecordResult = 0,
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xADT_FrameInfo = &g_stADT_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xADT_FrameInfo = 0;
#endif
//SOFT ADT data info
#if __FUNC_TYPE_SOFT_ADT_ENABLE__
const STGh3x2xFrameInfo g_stSOFT_ADT_GREEN_FrameInfo = 
{
    .unFunctionID = GH3X2X_FUNCTION_SOFT_ADT_GREEN,
    .pstFunctionInfo = &g_st_SOFT_ADT_GREEN_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_SOFT_ADT_CHNL_NUM,
    .pchChnlMap = g_pch_SOFT_ADT_GREEN_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .punIncompleteRawdata = g_pun_SOFT_ADT_GREEN_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_SOFT_ADT_GREEN_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_SOFT_ADT_GREEN_FrameLastGain,
    .punFrameCnt = &g_un_SOFT_ADT_GREEN_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_SOFT_ADT_GREEN,
    .pstAlgoRecordResult = 0,
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xSOFT_ADT_GREEN_FrameInfo = &g_stSOFT_ADT_GREEN_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xSOFT_ADT_GREEN_FrameInfo = 0;
#endif
#if __FUNC_TYPE_SOFT_ADT_ENABLE__
const STGh3x2xFrameInfo g_stSOFT_ADT_IR_FrameInfo = 
{
    .unFunctionID = GH3X2X_FUNCTION_SOFT_ADT_IR,
    .pstFunctionInfo = &g_st_SOFT_ADT_IR_FuncitonInfo,
    .pstAlgoResult   = &g_stGh3x2xAlgoResult,
    .uchFuntionChnlLimit = GH3X2X_SOFT_ADT_CHNL_NUM,
    .pchChnlMap = g_pch_SOFT_ADT_IR_ChnlMap,
    .punFrameRawdata = g_punGh3x2xFrameRawdata,
    .pusFrameGsensordata = g_psGh3x2xFrameGsensorData,
    .pstFrameCapdata = &g_pstGh3x2xFrameCapData,
    .pstFrameTempdata = &g_pstGh3x2xFrameTempData,
    .punFrameAgcInfo = g_punGh3x2xFrameAgcInfo,
    .punFrameFlag = g_punGh3x2xFrameFlag,
    .punIncompleteRawdata = g_pun_SOFT_ADT_GREEN_FrameIncompleteRawdata,
    .punIncompleteChnlMapBit = &g_un_SOFT_ADT_GREEN_FrameIncompleteChnlMapBit,
    .puchFrameLastGain = g_puch_SOFT_ADT_GREEN_FrameLastGain,
    .punFrameCnt = &g_un_SOFT_ADT_GREEN_TimeStamp,
    .pstDownSampleInfo = &g_stGh3x2xDownSampleInfo_SOFT_ADT_GREEN,
    .pstAlgoRecordResult = 0,
};
//const STGh3x2xFrameInfo * const  g_pstGh3x2xSOFT_ADT_IR_FrameInfo = &g_stSOFT_ADT_IR_FrameInfo;
#else
//const STGh3x2xFrameInfo * const  g_pstGh3x2xSOFT_ADT_IR_FrameInfo = 0;
#endif


//Function data info struct
const STGh3x2xFrameInfo * const  g_pstGh3x2xFrameInfo[GH3X2X_FUNC_OFFSET_MAX] = 
{
//adt
#if __SUPPORT_HARD_ADT_CONFIG__
    &g_stADT_FrameInfo,
#else
    0,
#endif
//hr
#if __FUNC_TYPE_HR_ENABLE__
    &g_stHR_FrameInfo,
#else
    0,
#endif
//hrv
#if __FUNC_TYPE_HRV_ENABLE__
    &g_stHRV_FrameInfo,
#else
    0,
#endif
//hsm
#if __FUNC_TYPE_HSM_ENABLE__
    &g_stHSM_FrameInfo,
#else
    0,
#endif
//fpbp
//pwa
#if __FUNC_TYPE_BP_ENABLE__
    &g_stFPBP_FrameInfo,
    &g_stPWA_FrameInfo,
#else
    0,
    0,
#endif   
//spo2
#if __FUNC_TYPE_SPO2_ENABLE__
    &g_stSPO2_FrameInfo,
#else
    0,
#endif
//ecg
#if __FUNC_TYPE_ECG_ENABLE__
    &g_stECG_FrameInfo,
#else
    0,
#endif
//pwtt
#if __FUNC_TYPE_PWTT_ENABLE__
    &g_stPWTT_FrameInfo,
#else
    0,
#endif
//soft adt green
#if __FUNC_TYPE_SOFT_ADT_ENABLE__
    &g_stSOFT_ADT_GREEN_FrameInfo,
#else
    0,
#endif
//bt
#if __FUNC_TYPE_BT_ENABLE__
    &g_stBT_FrameInfo,
#else
    0,
#endif
//resp
#if __FUNC_TYPE_RESP_ENABLE__
    &g_stRESP_FrameInfo,
#else
    0,
#endif
//af
#if __FUNC_TYPE_AF_ENABLE__
    &g_stAF_FrameInfo,
#else
    0,
#endif
#if __FUNC_TYPE_TEST_ENABLE__
    &g_stTEST1_FrameInfo,
    &g_stTEST2_FrameInfo,
#else
    0,
    0,
#endif
//soft adt ir
#if __FUNC_TYPE_SOFT_ADT_ENABLE__
    &g_stSOFT_ADT_IR_FrameInfo,
#else
    0,
#endif
    0, //GH3X2X_FUNCTION_RS0
    0, //GH3X2X_FUNCTION_RS1
    0, //GH3X2X_FUNCTION_RS2
    0, //GH3X2X_FUNCTION_LEAD_DET
};

/// read data buffer

#if __USER_DYNAMIC_DRV_BUF_EN__
static GU8 *g_puchGh3x2xReadRawdataBuffer = 0;
#if __GH3X2X_CASCADE_EN__
static GU8 *g_puchGh3x2xReadRawdataBufferSlaveChip = 0;
#endif
#else
#if __GH3X2X_CASCADE_EN__
static GU8 g_puchGh3x2xReadRawdataBuffer[2 * __GH3X2X_RAWDATA_BUFFER_SIZE__];
static GU8 g_puchGh3x2xReadRawdataBufferSlaveChip[__GH3X2X_RAWDATA_SLAVE_BUFFER_SIZE__];
#else
static GU8 g_puchGh3x2xReadRawdataBuffer[__GH3X2X_RAWDATA_BUFFER_SIZE__];
#endif
#endif

/// read data buffer len
GU16 g_usGh3x2xReadRawdataLen = 0;


extern void Gh3x2xDemoStopSamplingInner(GU32 unFuncMode);
extern void Gh3x2xDemoStartSamplingInner(GU32 unFuncMode, GU8 uchSwitchEnable);


/// driver lib demo init flag
static GU8 g_uchGh3x2xInitFlag = 0;


static GU8 g_uchGh3x2xInterruptProNotFinishFlag = 0;

#if (__NORMAL_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__ || __MIX_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__)
//hal_gh3x2x_int_handler_call_back is called, this value is 1
GU8 g_uchGh3x2xIntCallBackIsCalled = 0; 
#endif

#if (__SUPPORT_ENGINEERING_MODE__)
GU8 g_uchEngineeringModeStatus = 0;  // 0: is not in engineering mode   1: is in engineering mode
STGh3x2xEngineeringModeSampleParam *g_pstSampleParaGroup = 0;
GU8 g_uchEngineeringModeSampleParaGroupNum = 0;
#endif

/// current gh3x2x reg config array index
GU8 g_uchGh3x2xRegCfgArrIndex = DRV_LIB_REG_CFG_EMPTY;

/// int mode
GU8 g_uchGh3x2xIntMode = __INTERRUPT_PROCESS_MODE__;

#if __SUPPORT_SOFT_AGC_CONFIG__
//main chnl
STGh3x2xNewAgcMainChnlInfo  g_pstGh3x2xNewAgcMainChnlInfoEntity[GH3X2X_NEW_AGC_SLOT_NUM_LIMIT];
STGh3x2xNewAgcMainChnlInfo * const g_pstGh3x2xNewAgcMainChnlInfo = g_pstGh3x2xNewAgcMainChnlInfoEntity;
const GU8 g_uchNewAgcSlotNumLimit = GH3X2X_NEW_AGC_SLOT_NUM_LIMIT;
STGh3x2xNewAgcMainChnlSat g_pstGh3x2xNewAgcMainChnlSat;
STGh3x2xNewAgcMainChnlMeanInfo g_pstGh3x2xNewAgcMainChnlMeanInfoEntity[GH3X2X_NEW_AGC_SLOT_NUM_LIMIT];
STGh3x2xNewAgcMainChnlMeanInfo * const g_pstGh3x2xNewAgcMainChnlMeanInfo = g_pstGh3x2xNewAgcMainChnlMeanInfoEntity;

STGh3x2xNewAgcMainChnlIdaelAdjInfo g_pstGh3x2xNewAgcMainChnlIdaelAdjInfoEntity[GH3X2X_NEW_AGC_SLOT_NUM_LIMIT];
STGh3x2xNewAgcMainChnlIdaelAdjInfo * const g_pstGh3x2xNewAgcMainChnlIdaelAdjInfo = g_pstGh3x2xNewAgcMainChnlIdaelAdjInfoEntity;

const GU32 g_unNewAgcSpo2Trig_H_Thd = __GH3X2X_AGC_SPO2_TRIG_H_THD__;
const GU32 g_unNewAgcSpo2Trig_L_Thd = __GH3X2X_AGC_SPO2_TRIG_L_THD__;
const GU32 g_unNewAgcSpo2Ideal_value = __GH3X2X_AGC_SPO2_IDEAL_VALUE__;
const GU32 g_unNewAgcGeneTrig_H_Thd = __GH3X2X_AGC_GENERAL_TRIG_H_THD__;
const GU32 g_unNewAgcGeneTrig_L_Thd = __GH3X2X_AGC_GENERAL_TRIG_L_THD__;
const GU32 g_unNewAgcGeneIdeal_value = __GH3X2X_AGC_GENERAL_IDEAL_VALUE__;
//sub chnl
#if GH3X2X_NEW_AGC_SUB_CHNL_NUM_LIMIT > 0
STGh3x2xNewAgcSubChnlSlotInfo  g_pstGh3x2xNewAgcSubChnlSlotInfoEntity[GH3X2X_NEW_AGC_SLOT_NUM_LIMIT];
STGh3x2xNewAgcSubChnlRxInfo    g_pstGh3x2xNewAgcSubChnlRxInfoEntity[GH3X2X_NEW_AGC_SUB_CHNL_NUM_LIMIT];
STGh3x2xNewAgcSubChnlSlotInfo * const g_pstGh3x2xNewAgcSubChnlSlotInfo = g_pstGh3x2xNewAgcSubChnlSlotInfoEntity;
STGh3x2xNewAgcSubChnlRxInfo  * const g_pstGh3x2xNewAgcSubChnlRxInfo = g_pstGh3x2xNewAgcSubChnlRxInfoEntity;
#else
STGh3x2xNewAgcSubChnlSlotInfo  * const g_pstGh3x2xNewAgcSubChnlSlotInfo = 0;
STGh3x2xNewAgcSubChnlRxInfo  * const g_pstGh3x2xNewAgcSubChnlRxInfo = 0;
#endif
const GU8 g_uchNewAgcSubChnlNumLimit = GH3X2X_NEW_AGC_SUB_CHNL_NUM_LIMIT;
#else
STGh3x2xNewAgcMainChnlInfo * const g_pstGh3x2xNewAgcMainChnlInfo = 0;
STGh3x2xNewAgcSubChnlSlotInfo * const g_pstGh3x2xNewAgcSubChnlSlotInfo = 0;
STGh3x2xNewAgcSubChnlRxInfo * const g_pstGh3x2xNewAgcSubChnlRxInfo = 0;
const GU8 g_uchNewAgcSlotNumLimit = 0;
const GU8 g_uchNewAgcSubChnlNumLimit = 0;
#endif


#if __FUNC_TYPE_ECG_ENABLE__
#if __GH3X2X_CASCADE_EN__
STGh3x2xSoftLeadOffDetInfo  g_pstGh3x2xSoftLeadOffDetInfoEntity[2];
STGh3x2xRsInfo  g_pstGh3x2xEcgRsInfoEntity[2];
const GU8 g_uchGh3x2xEcgChnlNumLimit = 2;
#else
STGh3x2xSoftLeadOffDetInfo  g_pstGh3x2xSoftLeadOffDetInfoEntity[1];
STGh3x2xRsInfo  g_pstGh3x2xEcgRsInfoEntity[1];
const GU8 g_uchGh3x2xEcgChnlNumLimit = 1;
#endif
STGh3x2xSoftLeadOffDetInfo * const g_pstGh3x2xSoftLeadOffDetInfo = g_pstGh3x2xSoftLeadOffDetInfoEntity;
STGh3x2xRsInfo * const g_pstGh3x2xEcgRsInfo = g_pstGh3x2xEcgRsInfoEntity;
#else
STGh3x2xSoftLeadOffDetInfo * const g_pstGh3x2xSoftLeadOffDetInfo = 0;
STGh3x2xRsInfo * const g_pstGh3x2xEcgRsInfo = 0;
const GU8 g_uchGh3x2xEcgChnlNumLimit = 0;
#endif

void Gh3x2xGetConfigVersion(const STGh3x2xInitConfig *pstGh3x2xInitConfigParam)
{
    GU32 unCfgTimeStamp = 0;

    if (pstGh3x2xInitConfigParam->usConfigArrLen <= 1)
    {
        return;
    }
    else
    {
        for (GU16 usCnt = 0 ; usCnt < pstGh3x2xInitConfigParam->usConfigArrLen ; usCnt ++)
        {
            switch(pstGh3x2xInitConfigParam->pstRegConfigArr[usCnt].usRegAddr)
            {
            case 0x1006:
                unCfgTimeStamp |=  pstGh3x2xInitConfigParam->pstRegConfigArr[usCnt].usRegData;
                break;
            case 0x1008:
                unCfgTimeStamp |=  (pstGh3x2xInitConfigParam->pstRegConfigArr[usCnt].usRegData << 16);
                break;
            }
        }
    }
    if (unCfgTimeStamp != 0)
    {
        EXAMPLE_LOG("%s : Config Version : %x\n", __FUNCTION__, (unsigned int)unCfgTimeStamp);
    }
    else
    {
        EXAMPLE_LOG("%s : No Config Version !!!\n", __FUNCTION__);
    }
}

#if __GS_NONSYNC_READ_EN__
GU8 g_uchGh3x2xGsenosrExtralBufLen = __GS_EXTRA_BUF_LEN__;
GS16 g_sGh3x2xGsenorCurrentRemainPointNum;
GU16 g_usGsenorRefFunction = 0;
float g_fGh3x2xNonSyncGsensorStep;


void Gh3x2xNonSyncGsensorInit(void)
{
    g_sGh3x2xGsenorCurrentRemainPointNum = 0;
    EXAMPLE_LOG("[Non-Sync Gsensor]non-sync gsensor init.\r\n");
}

GU16 Gh3x2xGetCurrentRefFunction(void)
{
    GU16 usSampleRateMax = 0;
    GU16 usRefFucntion = 0;
    //find max sample rate function in opened function
    for(GU8 uchFunCnt = 0; uchFunCnt < GH3X2X_FUNC_OFFSET_MAX; uchFunCnt ++)
    {
        if(g_pstGh3x2xFrameInfo[uchFunCnt])
        {
            if(g_unDemoFuncMode & (((GU32)1)<< uchFunCnt))
            {
                if(usSampleRateMax < g_pstGh3x2xFrameInfo[uchFunCnt]->pstFunctionInfo->usSampleRate)
                {
                    usSampleRateMax = g_pstGh3x2xFrameInfo[uchFunCnt]->pstFunctionInfo->usSampleRate;
                    usRefFucntion = g_pstGh3x2xFrameInfo[uchFunCnt]->unFunctionID;
                }
            }
        }
    }
    
    EXAMPLE_LOG("[Non-Sync Gsensor]usRefFucntion = 0x%X.\r\n",usRefFucntion);
    //calculate step
    if(usSampleRateMax)
    {
        g_fGh3x2xNonSyncGsensorStep = __GS_SAMPLE_RATE_HZ__ / usSampleRateMax;
    }
    else
    {
        g_fGh3x2xNonSyncGsensorStep = 0;
        EXAMPLE_LOG("[Non-Sync Gsensor]Error !!! usSampleRateMax = 0.\r\n");
    }
    EXAMPLE_LOG("[Non-Sync Gsensor]g_fGh3x2xNonSyncGsensorStep = %.3f.\r\n",g_fGh3x2xNonSyncGsensorStep);
    
    EXAMPLE_LOG("[Non-Sync Gsensor]usRefFucntion = 0x%d,usSampleRateMax = %d.\r\n",usRefFucntion,usSampleRateMax);
    return usRefFucntion;
}

GU16 Gh3x2xGetGsensorNeedPointNum(GU16 usCurrentGsenosrPoint,GU8* puchReadFifoBuffer, GU16 usFifoBuffLen)
{
    GU8 uchFunctionOfst = 0xFF;
    GU16 usFrameNum = 0;
    GU16 usTempGsensorPointNum = 0;
    GU16 usTempGsensorPointNumFloat = 0;
    static GU16 usTempGsensorPointNumFractPart = 0;

    for(GU8 uchFunCnt = 0; uchFunCnt < GH3X2X_FUNC_OFFSET_MAX; uchFunCnt ++)
    {
        if (g_usGsenorRefFunction == (((GU32)1)<< uchFunCnt))
        {
            uchFunctionOfst = uchFunCnt;
            break;
        }
    }
    if(0xFF == uchFunctionOfst)
    {
        return 0;
    }

    //get ref function frame num in this fifo data
    if(g_pstGh3x2xFrameInfo[uchFunctionOfst])
    {
        usFrameNum = GH3x2xGetFrameNum(puchReadFifoBuffer,usFifoBuffLen,g_pstGh3x2xFrameInfo[uchFunctionOfst]);
    }
    
    

    usTempGsensorPointNumFloat = (float)usFrameNum * g_fGh3x2xNonSyncGsensorStep;
    usTempGsensorPointNum = (GU16)(usTempGsensorPointNumFloat + usTempGsensorPointNumFractPart);
    usTempGsensorPointNumFractPart = usTempGsensorPointNumFloat - (GU16)usTempGsensorPointNumFloat;

    if(usTempGsensorPointNum > (__GS_EXTRA_BUF_LEN__ + usCurrentGsenosrPoint))
    {
        usTempGsensorPointNum = __GS_EXTRA_BUF_LEN__ + usCurrentGsenosrPoint;
    }

    
    EXAMPLE_LOG("[Non-Sync Gsensor]GsensorNeedPointNum = %d.\r\n",usTempGsensorPointNum);

    return usTempGsensorPointNum;
}

GS16 Gh3x2xGetGsensorHeadPos(GU16 usCurrentGsenosrPoint,GU16 usNeedPoinNum)
{
    GS16 sPos1;
    GS16 sPos2;
    EXAMPLE_LOG("[Non-Sync Gsensor]usCurrentGsenosrPoint = %d.\r\n",usCurrentGsenosrPoint);
    sPos1 = (GS16)usCurrentGsenosrPoint - (GS16)usNeedPoinNum;
    sPos2= (GS16)0 - g_sGh3x2xGsenorCurrentRemainPointNum;

    if(sPos1 < sPos2)
    {
        GS16 sOldestPosi;
        GU8  uchExistOldData = 1;
        GS16 sDesPosi = sPos1;
        if(g_sGh3x2xGsenorCurrentRemainPointNum) //exist remain data
        {
            sOldestPosi = (GS16)0 - (GS16)g_sGh3x2xGsenorCurrentRemainPointNum;    
        }
        else if(usCurrentGsenosrPoint)   //use new gsensor data
        {
            sOldestPosi = 0; 
        }
        else
        {
            uchExistOldData = 0;
        }
        EXAMPLE_LOG("[Non-Sync Gsensor]Gsensor data is not enough,add oldest point num = %d, uchExistOldData = %d.****\r\n",sPos2 - sPos1,uchExistOldData);
        EXAMPLE_LOG("[Non-Sync Gsensor]GsensorHeadPos = %d.\r\n",sPos1);
        if(uchExistOldData)
        {
            for(sDesPosi = sPos1; sDesPosi < sOldestPosi;  sDesPosi++)  //copy oldest point value 
            {
                memcpy((GU8*)(gsensor_soft_fifo_buffer + __GS_EXTRA_BUF_LEN__ + sDesPosi),(GU8*)(gsensor_soft_fifo_buffer + __GS_EXTRA_BUF_LEN__ + sOldestPosi),sizeof(STGsensorRawdata));
            }
        }
        else  //copy 0
        {
            memset((GU8*)gsensor_soft_fifo_buffer+ __GS_EXTRA_BUF_LEN__ + sDesPosi,0, usNeedPoinNum * sizeof(STGsensorRawdata));
        }

        return sPos1;
    }
    EXAMPLE_LOG("[Non-Sync Gsensor]GsensorHeadPos = %d.\r\n",sPos2);
    return sPos2;
}

void Gh3x2xNonSyncGsenosrPostProcess(GU16 usCurrentGsenosrPoint,GU16 usNeedPoinNum ,GS16 sHeadPos)
{
    GS16 sRemainPointSrcStartPosi;
    GS16 sRemainPointDesStartPosi;
    GU16 usRemainPoinNum;
    sRemainPointSrcStartPosi = (GS16)usNeedPoinNum + sHeadPos;
    usRemainPoinNum = usCurrentGsenosrPoint - sRemainPointSrcStartPosi;
    EXAMPLE_LOG("[Non-Sync Gsensor]raw usRemainPoinNum = %d.\r\n",usRemainPoinNum);
    if(usRemainPoinNum > __GS_EXTRA_BUF_LEN__)
    {
        
        EXAMPLE_LOG("[Non-Sync Gsensor]******Gsensor data is too much, give up oldest point num = %d.****************\r\n",(usRemainPoinNum - __GS_EXTRA_BUF_LEN__));
        sRemainPointSrcStartPosi += (usRemainPoinNum - __GS_EXTRA_BUF_LEN__);  //give up oldest data
        usRemainPoinNum = __GS_EXTRA_BUF_LEN__;
    }
    sRemainPointDesStartPosi = ((GS16)0) - usRemainPoinNum;
    //copy remain data to extral buf
    if(usRemainPoinNum)
    {
        memcpy((GU8*)(gsensor_soft_fifo_buffer + __GS_EXTRA_BUF_LEN__ + sRemainPointDesStartPosi),(GU8*)(gsensor_soft_fifo_buffer + __GS_EXTRA_BUF_LEN__ + sRemainPointSrcStartPosi),usRemainPoinNum*sizeof(STGsensorRawdata));
    }
    //update g_sGh3x2xGsenorCurrentRemainPointNum
    g_sGh3x2xGsenorCurrentRemainPointNum = usRemainPoinNum;
    EXAMPLE_LOG("[Non-Sync Gsensor]GsenosrPostProcess: RemainPointNum= %d.\r\n",g_sGh3x2xGsenorCurrentRemainPointNum);
}
#endif

void Gh3x2xDemoHardConfigReset(const STGh3x2xInitConfig *pstGh3x2xInitConfigParam)
{
    EXAMPLE_LOG("[%s]:g_uchGh3x2xRegCfgArrIndex = %d\n", __FUNCTION__, g_uchGh3x2xRegCfgArrIndex);
    for (GU16 usArrCnt = 0; usArrCnt < (pstGh3x2xInitConfigParam)->usConfigArrLen ; usArrCnt ++)
    {
        if (((pstGh3x2xInitConfigParam)->pstRegConfigArr[usArrCnt].usRegAddr & 0xF000) == 0)
        {
            for (int uchSlotIndex = 0; uchSlotIndex < 8; uchSlotIndex++)
            {
                if (((pstGh3x2xInitConfigParam)->pstRegConfigArr[usArrCnt].usRegAddr) ==\
                        0x011E + (uchSlotIndex * 0x001C) + (0 * 0x0002)||
                    ((pstGh3x2xInitConfigParam)->pstRegConfigArr[usArrCnt].usRegAddr) ==\
                        0x011E + (uchSlotIndex * 0x001C) + (1 * 0x0002) ||
                    ((pstGh3x2xInitConfigParam)->pstRegConfigArr[usArrCnt].usRegAddr) ==\
                        0x0112 + (uchSlotIndex * 0x001C))
                {
                    EXAMPLE_LOG("[%s]:addr-value = 0x%04x-0x%04x\n", __FUNCTION__, (pstGh3x2xInitConfigParam)->pstRegConfigArr[usArrCnt].usRegAddr, (pstGh3x2xInitConfigParam)->pstRegConfigArr[usArrCnt].usRegData);
                    GH3X2X_WriteReg((pstGh3x2xInitConfigParam)->pstRegConfigArr[usArrCnt].usRegAddr,\
                                (pstGh3x2xInitConfigParam)->pstRegConfigArr[usArrCnt].usRegData);
                    if ((pstGh3x2xInitConfigParam)->pstRegConfigArr[usArrCnt].usRegData != \
                        GH3X2X_ReadReg((pstGh3x2xInitConfigParam)->pstRegConfigArr[usArrCnt].usRegAddr))
                    {
                        EXAMPLE_LOG("[%s]:reg verify error!!!\r\n", __FUNCTION__);
                    }
                }
            }
        }
    }
    GH3X2X_EnterLowPowerMode();
}

#if (__SUPPORT_HARD_ADT_CONFIG__)
/**
 * @fn     void Gh3x2xDemoWearEventProcess(u16* usEvent,EMWearDetectForceSwitch emForceSwitch)
 *
 * @brief  Process wear on/off event
 *
 * @attention   None
 *
 * @param[in]   usEvent             gh3x2x irq status
 * @param[in]   emForceSwitch       switch wear detect status by force or not
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xDemoWearEventProcess(GU16* usEvent,EMWearDetectForceSwitch emForceSwitch)
{
#if __FUNC_TYPE_SOFT_ADT_ENABLE__
    if (g_uchHardAdtFuncStatus == 0 && emForceSwitch != WEAR_DETECT_FORCE_SWITCH)
    {
        *usEvent &= ~(GH3X2X_IRQ_MSK_WEAR_OFF_BIT|GH3X2X_IRQ_MSK_WEAR_ON_BIT);
        return;
    }
#endif
    if ((g_unDemoFuncMode & GH3X2X_FUNCTION_ADT) == GH3X2X_FUNCTION_ADT)
    {
        if((0 != (*usEvent & GH3X2X_IRQ_MSK_WEAR_ON_BIT)) || (0 != (*usEvent & GH3X2X_IRQ_MSK_WEAR_OFF_BIT)))
        {
            if(0 != (*usEvent & GH3X2X_IRQ_MSK_WEAR_ON_BIT))
            {
                g_uchWearDetectStatus = WEAR_DETECT_WEAR_OFF;
                if(GH3X2X_RET_OK != GH3X2X_WearDetectSwitchTo(WEAR_DETECT_WEAR_OFF,emForceSwitch))
                {
                    *usEvent &= ~GH3X2X_IRQ_MSK_WEAR_ON_BIT;
                }
                else
                {
                    EXAMPLE_LOG("[%s]:switch to wear off\r\n", __FUNCTION__);
                }
            }
            else
            {
                g_uchWearDetectStatus = WEAR_DETECT_WEAR_ON;
                if(GH3X2X_RET_OK != GH3X2X_WearDetectSwitchTo(WEAR_DETECT_WEAR_ON,emForceSwitch))
                {
                    *usEvent &= ~GH3X2X_IRQ_MSK_WEAR_OFF_BIT;
                }
                else
                {
                    EXAMPLE_LOG("[%s]:switch to wear on\r\n", __FUNCTION__);
                }
                
            }
        }
    }
    else
    {
        *usEvent &= ~(GH3X2X_IRQ_MSK_WEAR_OFF_BIT|GH3X2X_IRQ_MSK_WEAR_ON_BIT);
    }
}
#endif

#if __FUNC_TYPE_SOFT_ADT_ENABLE__
void Gh3x2xDemoSoftAdtStatusSwtich(GU16* usEvent, GU8* uchEventEx)
{
    if ((g_unDemoFuncMode & GH3X2X_FUNCTION_ADT) == GH3X2X_FUNCTION_ADT)
    {
        if(g_uchWearDetectStatus == WEAR_DETECT_WEAR_OFF && g_uchVirtualAdtTimerCtrlStatus == 1)
        {
            if (Gh3x2x_GetAdtConfirmStatus() == 0)
            {
                EXAMPLE_LOG("wear on but not move!!!\r\n");
                *usEvent &= ~GH3X2X_IRQ_MSK_WEAR_ON_BIT;
                *uchEventEx = 0;
            }
            else
            {
                *usEvent |= GH3X2X_IRQ_MSK_WEAR_ON_BIT;
                GH3X2X_AdtFuncStopWithGsDetect();
                GH3X2X_STOP_ADT_TIMER();
                *uchEventEx = 1;
            }
        }
        if(0 != (*usEvent & GH3X2X_IRQ_MSK_WEAR_OFF_BIT))
        {
            GH3X2X_AdtFuncStartWithGsDetect();
            *uchEventEx = 0;
        }
    }
}
#endif

GCHAR* GH3X2X_GetFirmwareVersion(void)
{
#ifdef GOODIX_DEMO_PLANFORM
    return GetEvkVersion();
#else
    return GH3X2X_DEMO_VERSION_STRING;
#endif
}

GCHAR* GH3X2X_GetDemoVersion(void)
{
    return GH3X2X_DEMO_VERSION_STRING;
}

#if (__SUPPORT_PROTOCOL_ANALYZE__)
GCHAR* GH3X2X_GetEvkBootloaderVersion(void)
{
#ifdef GOODIX_DEMO_PLANFORM
    return GetBootloaderVersion();
#else
    return GH3X2X_DEMO_VERSION_STRING;
#endif
}
#endif

/**
 * @fn     void Gh3x2x_NormalizeGsensorSensitivity(STGsensorRawdata gsensor_buffer[], GU16 gsensor_point_num) 
 *
 * @brief  normalize gsensor sensitivity to  512LSB/g
 *
 * @attention   None
 *
 * @param[in]    gsensor_buffer          gsensor rawdata buffer
 * @param[in]   gsensor_point_num     gsensor rawdata point num
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2x_NormalizeGsensorSensitivity(STGsensorRawdata gsensor_buffer[], GU16 gsensor_point_num)
{
    GU8 uchRightShift;
    uchRightShift = __GS_SENSITIVITY_CONFIG__;
    for(GU16 uchCnt = 0; uchCnt < gsensor_point_num; uchCnt++)
    {
        gsensor_buffer[uchCnt].sXAxisVal >>= uchRightShift;
        gsensor_buffer[uchCnt].sYAxisVal >>= uchRightShift;
        gsensor_buffer[uchCnt].sZAxisVal >>= uchRightShift;
    }
}


void Gh3x2xFunctionCtrlModuleInit(void)
{
    g_unDemoFuncMode = 0;
#if (__FUNC_TYPE_SOFT_ADT_ENABLE__)
    g_uchHardAdtFuncStatus = 0;
#endif
}
/**
 * @fn     void Gh3x2xDemoFunctionSampleRateSet(GU32 unFunctionID,  GU16 usSampleRate)
 *
 * @brief  
 *
 * @attention   None
 *
 * @param[in]   unFunctionID   such as: GH3X2X_FUNCTION_HR   or  GH3X2X_FUNCTION_HR|GH3X2X_FUNCTION_HRV
 * @param[in]   usSampleRate   only 25Hz times supported  (25/50/75...)
 * @param[out]  None
 *
 * @return  None
 */
#if __SUPPORT_FUNCTION_SAMPLE_RATE_MODIFY__
void Gh3x2xDemoFunctionSampleRateSet(GU32 unFunctionID,  GU16 usSampleRate)
{
    if((0 == usSampleRate)||(0 !=(usSampleRate%25)))
    {
        EXAMPLE_LOG("%s : usSampleRate is invalid !!!\r\n", __FUNCTION__);
        while(1);
    }

    for(GU8 uchFunctionCnt = 0; uchFunctionCnt < GH3X2X_FUNC_OFFSET_MAX; uchFunctionCnt++)
    {
        if(unFunctionID&(((GU32)1)<<uchFunctionCnt))
        {
            if(g_pstGh3x2xFrameInfo[uchFunctionCnt])
            {
                g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstFunctionInfo->usSampleRateForUserSetting = usSampleRate;
            }
        }
    }
}
#endif

/**
 * @fn     void Gh3x2xDemoFunctionProcess(GU8* puchReadFifoBuffer, STGsensorRawdata *pstGsAxisValueArr, 
 *                                              GU16 usGsDataNum, EMGsensorSensitivity emGsSensitivity)
 *
 * @brief  Function process.This function will be called in Gh3x2xDemoInterruptProcess()
 *
 * @attention   None
 *
 * @param[in]   puchReadFifoBuffer      point to gh3x2x fifo data buffer
 * @param[in]   pstGsAxisValueArr       point to gsensor data buffer
 * @param[in]   usGsDataNum             gsensor data cnt
 * @param[in]   emGsSensitivity         sensitivity of gsensor
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xDemoFunctionProcess(GU8* puchReadFifoBuffer, GU16 usFifoBuffLen,STGsensorRawdata *pstGsAxisValueArr, GU16 usGsDataNum,
                                   STCapRawdata* pstCapValueArr,GU16 usCapDataNum,STTempRawdata* pstTempValueArr,GU16 usTempDataNum)
{
    for(GU8 uchFunCnt = 0; uchFunCnt < GH3X2X_FUNC_OFFSET_MAX; uchFunCnt ++)
    {
        if(g_pstGh3x2xFrameInfo[uchFunCnt])
        {
            
            if(g_unDemoFuncMode & (((GU32)1)<< uchFunCnt))
            {
                GH3x2xFunctionProcess(puchReadFifoBuffer,usFifoBuffLen,(GS16*)pstGsAxisValueArr,usGsDataNum,
                                        pstCapValueArr,usCapDataNum,pstTempValueArr,usTempDataNum,
                                        g_pstGh3x2xFrameInfo[uchFunCnt]);
            }
        }
    }
}





#if __USER_DYNAMIC_DRV_BUF_EN__
/**
 * @fn     GU8 Gh3x2xDemoMemCheckValid(void)
 *
 * @brief  check dynamic memory is valid or not
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  0: invalid  1: valid
 */
GU8 Gh3x2xDemoMemCheckValid(void)
{
    if(0 == g_puchGh3x2xReadRawdataBuffer)
    {
        EXAMPLE_LOG("%s : g_puchGh3x2xReadRawdataBuffer is invalid !!!\r\n", __FUNCTION__);
        return 0;
    }
    #if __GH3X2X_CASCADE_EN__
    if (GH3X2X_CascadeGetEcgEnFlag())
    {
        if(0 == g_puchGh3x2xReadRawdataBufferSlaveChip)
        {
            EXAMPLE_LOG("%s : g_puchGh3x2xReadRawdataBufferSlaveChip is invalid !!!\r\n", __FUNCTION__);
            return 0;
        }
    }
    #endif
    return 1;
}


void Gh3x2xDemoMemInit(void)
{
    if(0 == g_puchGh3x2xReadRawdataBuffer)
    {
        #if __GH3X2X_CASCADE_EN__
        g_puchGh3x2xReadRawdataBuffer = Gh3x2xMallocUser(2 * __GH3X2X_RAWDATA_BUFFER_SIZE__);
        #else
        g_puchGh3x2xReadRawdataBuffer = Gh3x2xMallocUser(__GH3X2X_RAWDATA_BUFFER_SIZE__);
        #endif
    }
    if(0 == Gh3x2xDemoMemCheckValid())
    {
        while(1);
    }
}
#endif
/**
 * @fn     int Gh3x2xDemoInit(void)
 *
 * @brief  Init GH3x2x module
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  error code
 */
int Gh3x2xDemoInit(void)
{
    GU8 buf[2];
    /// int mode default check
    if (g_uchGh3x2xIntMode == __MIX_INT_PROCESS_MODE__)
    {
        g_uchGh3x2xIntMode = __POLLING_INT_PROCESS_MODE__;
    }

    GS8 schret = GH3X2X_RET_OK;

    #if __EXAMPLE_LOG_TYPE__ == __EXAMPLE_LOG_METHOD_0__
    g_pSnprintfUser = snprintf;
    #elif __EXAMPLE_LOG_TYPE__ == __EXAMPLE_LOG_METHOD_1__
    GH3X2X_RegisterPrintf(&g_pPrintfUser);
    #endif

    #if __USER_DYNAMIC_DRV_BUF_EN__
    Gh3x2xDemoMemInit();
    #endif

    #ifdef GOODIX_DEMO_PLANFORM
    EXAMPLE_LOG("%s : Firmware Version : %s\r\n", __FUNCTION__, GH3X2X_GetFirmwareVersion());
    #endif
    EXAMPLE_LOG("%s : Democode Version : %s\r\n", __FUNCTION__, GH3X2X_GetDemoVersion());
    EXAMPLE_LOG("%s : DrvLib Version : %s\r\n", __FUNCTION__, GH3X2X_GetDriverLibVersion());
    #if (__SUPPORT_PROTOCOL_ANALYZE__)
    EXAMPLE_LOG("%s : Protocol Version : %s\r\n", __FUNCTION__, GH3X2X_GetProtocolVersion());
    #endif
    EXAMPLE_LOG("%s : Config Version : %s\r\n", __FUNCTION__, GH3X2X_GetVirtualRegVersion());
    #if (__DRIVER_LIB_MODE__ == __DRV_LIB_WITH_ALGO__)
    GCHAR algo_version[100] = {0};
    for (int i=0;i<GH3X2X_FUNC_OFFSET_MAX;i++)
    {
        GH3X2X_AlgoVersion(i, algo_version);
        EXAMPLE_LOG("%s : Algo Version : %s\r\n", __FUNCTION__, algo_version);
    }
    #endif

    /* Step 1: init communicate interface. */
    if(0 == g_uchGh3x2xInitFlag)
    {
        #if ( __GH3X2X_INTERFACE__ == __GH3X2X_INTERFACE_I2C__ )
        hal_gh3x2x_i2c_init();
        GH3X2X_RegisterI2cOperationFunc(__GH3X2X_I2C_DEVICE_ID__, hal_gh3x2x_i2c_write, hal_gh3x2x_i2c_read);
        #if __GH3X2X_CASCADE_EN__
        GH3X2X_RegisterI2cDeviceChip1(__GH3X2X_I2C_DEVICE_SLAVER_ID__);
        #endif
        #else //__GH3X2X_INTERFACE__ == __GH3X2X_INTERFACE_SPI__
        hal_gh3x2x_spi_init();
        // buf[0] = 0xaa;
        // hal_gh3x2x_spi_write(buf, 1);
        // hal_gh3x2x_spi_write_F1_and_read(buf, 2);

        #if (__GH3X2X_SPI_TYPE__ == __GH3X2X_SPI_TYPE_HARDWARE_CS__) 
        GH3X2X_RegisterSpiHwCsOperationFunc(hal_gh3x2x_spi_write, hal_gh3x2x_spi_write_F1_and_read);
        #else
        GH3X2X_RegisterSpiOperationFunc(hal_gh3x2x_spi_write, hal_gh3x2x_spi_read, hal_gh3x2x_spi_cs_ctrl);
        #endif
        #endif
    }

    #if __SUPPORT_HOOK_FUNC_CONFIG__
    /* set hook */
    GH3X2X_RegisterHookFunc(gh3x2x_init_hook_func, 
                            gh3x2x_sampling_start_hook_func,
                            gh3x2x_sampling_stop_hook_func, 
                            gh3x2x_get_rawdata_hook_func,
                            gh3x2x_frame_data_hook_func,
                            gh3x2x_reset_by_protocol_hook,
                            gh3x2x_config_set_start_hook,
                            gh3x2x_config_set_stop_hook,
                            gh3x2x_write_algo_config_hook);
    #endif

    EXAMPLE_LOG("xdg1\r\n");
    /* Step 2: Reset gh3x2x chip */
    GH3X2X_RegisterDelayUsCallback(Gh3x2x_BspDelayUs);

    #if __GH3X2X_CASCADE_EN__
    GH3X2X_SetChipSleepFlag(0);
    GH3X2X_SetChipSleepFlag(1);
    #else
    GH3X2X_SetChipSleepFlag(0);
    #endif
    
    
    #if __SUPPORT_HARD_RESET_CONFIG__    
    if(0 == g_uchGh3x2xInitFlag)
    {
        hal_gh3x2x_reset_pin_init();
    }    
    GH3X2X_RegisterResetPinControlFunc(hal_gh3x2x_reset_pin_ctrl);
    GH3X2X_HardReset();
    #else
    GH3X2X_SoftReset();
    #endif

    Gh3x2x_BspDelayMs(30);   
    #if __GH3X2X_CASCADE_EN__
    GH3X2X_CascadeOperationSlaverChip();
    Gh3x2x_CHECK_0x0088_Ex();
    Gh3x2x_CHECK_0x0088_Ex();
    GH3X2X_CascadeOperationMasterChip();
    #endif
    
    EXAMPLE_LOG("xdg2\r\n");
    /* Step 3: Init gh3x2x chip */
    g_uchGh3x2xRegCfgArrIndex = 0;
    schret = GH3X2X_Init(&g_stGh3x2xCfgListArr[0]);
    EXAMPLE_LOG("xdg3\r\n");
    if (GH3X2X_RET_OK == schret)
    {
        Gh3x2xGetConfigVersion(&g_stGh3x2xCfgListArr[0]);
        EXAMPLE_LOG("Gh3x2xDemoInit:init success\r\n");
    }
    else
    {
        g_uchGh3x2xRegCfgArrIndex = DRV_LIB_REG_CFG_EMPTY;
        EXAMPLE_LOG("Gh3x2xDemoInit:init fail, error code: %d\r\n", schret);
        return schret;
    }
    EXAMPLE_LOG("xdg4\r\n");
    GH3X2X_SetMaxNumWhenReadFifo(__GH3X2X_RAWDATA_BUFFER_SIZE__);
    GH3X2X_EnterLowPowerMode();
    EXAMPLE_LOG("xdg5\r\n");
    #if __GH3X2X_CASCADE_EN__
    GH3X2X_CascadeOperationSlaverChip();
    GH3X2X_EnterLowPowerMode();
    GH3X2X_CascadeOperationMasterChip();
    #endif

    /* Step 4: setup EX INT for GH3X2X INT pin */
    #if (__NORMAL_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__ || __MIX_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__)
    if (g_uchGh3x2xIntMode == __NORMAL_INT_PROCESS_MODE__)
    {
        g_uchGh3x2xIntCallBackIsCalled = 0;
        if(0 == g_uchGh3x2xInitFlag)
        {
            hal_gh3x2x_int_init();
        }
    }
    EXAMPLE_LOG("xdg6\r\n");
    #endif

    #if (__SUPPORT_PROTOCOL_ANALYZE__)
    Gh3x2x_HalSerialFifoInit();
    GH3X2X_UprotocolPacketMaxLenConfig(GH3X2X_UPROTOCOL_PAYLOAD_LEN_MAX);
    GH3X2X_RegisterGetFirmwareVersionFunc(GH3X2X_GetFirmwareVersion,
                                          GH3X2X_GetDemoVersion,
                                          GH3X2X_GetEvkBootloaderVersion);
    #endif
    EXAMPLE_LOG("xdg7\r\n");
    /* Step 6: Soft ADT init */
    #if (__FUNC_TYPE_SOFT_ADT_ENABLE__)
    {
        Gh3x2x_SetAdtConfirmPara(__GSENSOR_MOVE_THRESHOLD__, __GSENSOR_MOVE_CNT_THRESHOLD__, __GSENSOR_NOT_MOVE_CNT_THRESHOLD__);
    #if (__USE_POLLING_TIMER_AS_ADT_TIMER__)&&\
        (__POLLING_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__ || __MIX_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__)
    #else
        if (g_uchGh3x2xIntMode == __NORMAL_INT_PROCESS_MODE__)
        {
            Gh3x2xCreateAdtConfirmTimer();
        }
    #endif
    }
    #endif
    EXAMPLE_LOG("xdg8\r\n");
    Gh3x2xFunctionInfoForUserInit();
    Gh3x2xFunctionCtrlModuleInit();
    GH3X2X_ConfigLeadOffThr(LEADOFF_DET_IQ_AMP_THR_uV, LEADOFF_DET_IQ_AMP_DIFF_THR_uV,LEADOFF_DET_IQ_VAL_THR_mV);
    g_uchGh3x2xInitFlag = 1;
    #if __GH3X2X_MEM_POOL_CHECK_EN__
    g_uchGh3x2xMemPollHaveGetCheckSum = 0;
    #endif

    return (int)schret;
}

/**
 * @fn     void Gh3x2xInterruptModeSwitch(GU8 uchIntModeType)
 *
 * @brief  change Interrupt process mode of GH3x2x.
 *
 * @attention   do not input 2
 *
 * @param[in]   emIntModeType 0:int pin 1: polling
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xInterruptModeSwitch(GU8 uchIntModeType)
{
#if (__MIX_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__)
    EXAMPLE_LOG("[%s]:int process switch to = %d\r\n", __FUNCTION__, uchIntModeType);
    g_uchGh3x2xIntMode = uchIntModeType;
    g_uchGh3x2xInitFlag = 0;
#else
    EXAMPLE_LOG("[%s]:do not allow int mode switch!!!\r\n", __FUNCTION__);
#endif
}

GU8 Gh3x2xGetInterruptMode(void)
{
    return g_uchGh3x2xIntMode;
}

/**
 * @fn     GU8 Gh3x2xDemoInterruptProcess(void)
 *
 * @brief  Interrupt process of GH3x2x.
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xDemoInterruptProcess(void)
{
    if(0 == g_uchGh3x2xInitFlag)
    {
        EXAMPLE_LOG("[%s]:gh3x2x is not init!!!\r\n", __FUNCTION__);
        return;
    }

    GU8 uchRet = 0;
    GU16 usGotEvent = 0;
    GU8 uchIntRepeatNum = 0;
#if (__GH3X2X_CASCADE_EN__)
    GU16 usGotSlaverEvent = 0;
#endif
    GU8 uchEventEx = 0;     //bit0:  0: object/no objiect   1: living/nonliving object
                            //bit1:  0: None                1: chip0 lead on
                            //bit2:  0: None                1: chip0 lead off
                            //bit3:  0: None                1: chip1 lead on
                            //bit4:  0: None                1: chip1 lead off

    g_uchGh3x2xInterruptProNotFinishFlag = 1;

#if (__FUNC_TYPE_ECG_ENABLE__)
    STSoftLeadResult stLeadResult = {0, 0};
#endif

    if(GH3X2X_DEMO_WORK_MODE_MPT == g_uchDemoWorkMode)
    {
        return;
    }

#if __USER_DYNAMIC_DRV_BUF_EN__
    if(0 == Gh3x2xDemoMemCheckValid())    //dynamic mem is not valid
    {
        EXAMPLE_LOG("[%s]Warning, dynamic mem is not valid !!!\r\n",__FUNCTION__);
        return;
    }
#endif




#if __FUNC_TYPE_SOFT_ADT_ENABLE__
#if (__USE_POLLING_TIMER_AS_ADT_TIMER__)&&(__POLLING_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__ || __MIX_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__)
    if (g_uchGh3x2xIntMode == __POLLING_INT_PROCESS_MODE__ && 1 == g_uchVirtualAdtTimerCtrlStatus && GH3X2X_GetSoftWearOffDetEn())
    {
        Gh3x2xDemoMoveDetectTimerHandler();
    }
#endif
#endif

#if __GH_MSG_WITH_ALGO_LAYER_EN__
        GH3X2X_PRO_MSG_SOFT_WEAR_EVEVT();
#endif

    //check "Gh3x2xDemoInterruptProcess" call is legal or not
#if (__NORMAL_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__ || __MIX_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__)
    if (g_uchGh3x2xIntMode == __NORMAL_INT_PROCESS_MODE__)
    {
        if((0 == GH3X2X_GetSoftEvent())&&(0 == g_uchGh3x2xIntCallBackIsCalled))
        {
    #if (0 == __PLATFORM_WITHOUT_OS__)
            EXAMPLE_LOG("InterruptProcess: plese check hal_gh3x2x_int_handler_call_back is called or not !!!\r\n");
    #endif
            EXAMPLE_LOG("InterruptProcess:invalid call, do not process.\r\n");
            return;
        }
    }
#endif

#if (__NORMAL_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__ || __MIX_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__)
    if (g_uchGh3x2xIntMode == __NORMAL_INT_PROCESS_MODE__)
    {
        g_uchGh3x2xIntCallBackIsCalled = 0;
    }  
#endif

    do{
        uchRet = GH3X2X_INT_PROCESS_SUCCESS;
        usGotEvent = 0;
    #if __GH3X2X_CASCADE_EN__
         usGotSlaverEvent = 0;
    #endif

        /* Step 1: read irq status */

    #if (__FUNC_TYPE_ECG_ENABLE__)
        memset((GU8*)(&stLeadResult),0,sizeof(stLeadResult));
    #endif

        usGotEvent = GH3X2X_GetIrqStatus();
    #if (__GH3X2X_CASCADE_EN__)
         if (GH3X2X_CascadeGetEcgEnFlag())
         {
             GH3X2X_CascadeOperationSlaverChip();
             usGotSlaverEvent = GH3X2X_GetIrqStatus();
             GH3X2X_CascadeOperationMasterChip();
             if((usGotEvent|usGotSlaverEvent)&(GH3X2X_IRQ_MSK_WEAR_ON_BIT|GH3X2X_IRQ_MSK_WEAR_OFF_BIT|GH3X2X_IRQ_MSK_LEAD_ON_DET_BIT|GH3X2X_IRQ_MSK_LEAD_ON_DET_BIT))
             {
                EXAMPLE_LOG("usGotMasterEvent:0x%04x usGotSlaverEvent:0x%04x\r\n",usGotEvent, usGotSlaverEvent);
             }
         }
         else
         {
            EXAMPLE_LOG("usGotMasterEvent:0x%04x \r\n",usGotEvent);
         }
    #endif
        usGotEvent &= __GH3X2X_EVENT_PROCESS_MASK__;
        EXAMPLE_LOG("usGotEvent = 0x%x\r\n", usGotEvent);



        
    #if (__SUPPORT_HARD_ADT_CONFIG__)				 
        /* Step 2: if hard adt enabled, do wear event process */
        /* 1) if algo hard adt, delete adt event */
        #if __HARD_ADT_ALGO_CHECK_EN__
            usGotEvent &= (~__GH3X2X_ADT_EVENT_MASK__);
        #endif
        /* 2) check soft adt event */
        if(GH3X2X_GetSoftEvent() & GH3X2X_SOFT_EVENT_WEAR_OFF)
        {
            uchEventEx |= 0x1;
            usGotEvent |= GH3X2X_IRQ_MSK_WEAR_OFF_BIT;
            GH3X2X_ClearSoftEvent(GH3X2X_SOFT_EVENT_WEAR_OFF);
        }
        else if(GH3X2X_GetSoftEvent() & GH3X2X_SOFT_EVENT_WEAR_ON)
        {
            uchEventEx |= 0x1;
            usGotEvent |= GH3X2X_IRQ_MSK_WEAR_ON_BIT;
            GH3X2X_ClearSoftEvent(GH3X2X_SOFT_EVENT_WEAR_ON);
        }
        /* 3) adt event handle */
        Gh3x2xDemoWearEventProcess(&usGotEvent, WEAR_DETECT_DONT_FORCE_SWITCH);
    #if __FUNC_TYPE_SOFT_ADT_ENABLE__
        Gh3x2xDemoSoftAdtStatusSwtich(&usGotEvent, &uchEventEx);
    #endif
    #if (__EXAMPLE_LOG_TYPE__ > __EXAMPLE_LOG_DISABLE__)
        if(0 != (usGotEvent & GH3X2X_IRQ_MSK_WEAR_ON_BIT))
        {
            EXAMPLE_LOG("Got hardware wear on !!! \r\n");
        }
        if(0 != (usGotEvent & GH3X2X_IRQ_MSK_WEAR_OFF_BIT))
        {
            EXAMPLE_LOG("Got hardware wear off !!! \r\n");
        }
    #endif
    #endif

        /****** Start: Ecg Cascade Debug information********/
#if (__GH3X2X_CASCADE_EN__)
        if (GH3X2X_CascadeGetEcgEnFlag())
        {
            GH3X2X_CascadeOperationMasterChip();
            //EXAMPLE_LOG("Master: 0x00:%04x 0x108:%d  0x400:%04x 0x50c:%04x 0x088:%04x\r\n",GH3X2X_ReadReg(0x00),GH3X2X_ReadReg(0x108),GH3X2X_ReadReg(0x400),GH3X2X_ReadReg(0x74),GH3X2X_ReadReg(0x88));
            GH3X2X_CascadeOperationSlaverChip();
            //EXAMPLE_LOG("Slaver: 0x00:%04x 0x108:%d  0x400:%04x 0x50c:%04x 0x088:%04x\r\n",GH3X2X_ReadReg(0x00),GH3X2X_ReadReg(0x108),GH3X2X_ReadReg(0x400),GH3X2X_ReadReg(0x74),GH3X2X_ReadReg(0x88));
            GH3X2X_CascadeOperationMasterChip();
        }
        else
        {
            EXAMPLE_LOG("Master: 0x00:%04x 0x108:%d  0x400:%04x 0x50c:%04x 0x088:%04x\r\n",GH3X2X_ReadReg(0x00),GH3X2X_ReadReg(0x108),GH3X2X_ReadReg(0x400),GH3X2X_ReadReg(0x74),GH3X2X_ReadReg(0x88));
        }
#endif
        /****** End: Ecg Cascade Debug information********/

        /* Step 3: if it's FIFO interrupt, read gh3x2x fifo data, read gsensor data*/
    #if (__POLLING_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__ || __MIX_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__)
        if (g_uchGh3x2xIntMode == __POLLING_INT_PROCESS_MODE__)
        {
            usGotEvent |= GH3X2X_IRQ_MSK_FIFO_WATERMARK_BIT;
        }
    #endif
        if ((usGotEvent & (GH3X2X_IRQ_MSK_FIFO_WATERMARK_BIT | GH3X2X_IRQ_MSK_FIFO_FULL_BIT)) \
            || (GH3X2X_GetSoftEvent() & (GH3X2X_SOFT_EVENT_NEED_FORCE_READ_FIFO|GH3X2X_SOFT_EVENT_NEED_TRY_READ_FIFO)))
        {
            GU16 usFifoByteNum = ((GU16)4)*GH3X2X_ReadReg(GH3X2X_INT_FIFO_UR_REG_ADDR); //read fifo use
        #if (__GH3X2X_CASCADE_EN__)
            //vTaskDelay(5);
            GU16 usFifoByteNumSlaver = 0;
            if (GH3X2X_CascadeGetEcgEnFlag())
            {
                GH3X2X_CascadeOperationSlaverChip();
                usFifoByteNumSlaver = ((GU16)4)*GH3X2X_ReadReg(GH3X2X_INT_FIFO_UR_REG_ADDR);
                GH3X2X_CascadeOperationMasterChip();
            }

        #endif
        #if (__NORMAL_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__ || __MIX_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__)
            if (g_uchGh3x2xIntMode == __NORMAL_INT_PROCESS_MODE__)
            {
                GU8 uchIsForceRead = (GH3X2X_GetSoftEvent() & GH3X2X_SOFT_EVENT_NEED_FORCE_READ_FIFO);
                if(0 == uchIsForceRead)
                {
                    if(usFifoByteNum < ((GU16)4)*GH3X2X_GetCurrentFifoWaterLine())  //data is too less
                    {
                        usFifoByteNum = 0;   //no need read
                    }
                }
            }
        #endif

            GH3X2X_ClearSoftEvent(GH3X2X_SOFT_EVENT_NEED_FORCE_READ_FIFO|GH3X2X_SOFT_EVENT_NEED_TRY_READ_FIFO);
            g_usGh3x2xReadRawdataLen = 0;
            if (usFifoByteNum != 0)
            {
            #if (__DRIVER_LIB_MODE__ == __DRV_LIB_WITH_ALGO__)
                GH3X2X_TimestampSyncSetPpgIntFlag(1);
            #endif
            #if !(__GH3X2X_CASCADE_EN__)
                if (GH3X2X_RET_READ_FIFO_CONTINUE == GH3X2X_ReadFifodata(g_puchGh3x2xReadRawdataBuffer, &g_usGh3x2xReadRawdataLen, usFifoByteNum))
                {
                    GH3X2X_SetSoftEvent(GH3X2X_SOFT_EVENT_NEED_TRY_READ_FIFO);
                }
            #else
                if (GH3X2X_CascadeGetEcgEnFlag())
                {
                    //EXAMPLE_LOG("Master fifo:%d  Slaver Fifo:%d CurrentFifoWaterLine:%d\r\n", usFifoByteNum/4, usFifoByteNumSlaver/4, GH3X2X_GetCurrentFifoWaterLine());
                    if (usFifoByteNum > usFifoByteNumSlaver)
                    {
                        usFifoByteNum = usFifoByteNumSlaver;
                    }
                    if (usFifoByteNum > __GH3X2X_RAWDATA_SLAVE_BUFFER_SIZE__)
                    {
                        usFifoByteNum = __GH3X2X_RAWDATA_SLAVE_BUFFER_SIZE__;
                    }
                    if (GH3X2X_RET_READ_FIFO_CONTINUE == GH3X2X_CascadeReadFifodata(g_puchGh3x2xReadRawdataBuffer,g_puchGh3x2xReadRawdataBufferSlaveChip, &g_usGh3x2xReadRawdataLen ,usFifoByteNum, 0x08))
                    {
                        GH3X2X_SetSoftEvent(GH3X2X_SOFT_EVENT_NEED_TRY_READ_FIFO);
                    }
                }

                else
                {
                    if (GH3X2X_RET_READ_FIFO_CONTINUE == GH3X2X_ReadFifodata(g_puchGh3x2xReadRawdataBuffer, &g_usGh3x2xReadRawdataLen, usFifoByteNum))
                    {
                        GH3X2X_SetSoftEvent(GH3X2X_SOFT_EVENT_NEED_TRY_READ_FIFO);
                    }
                }
            #endif
            #if ((__SUPPORT_PROTOCOL_ANALYZE__)||(__SUPPORT_ALGO_INPUT_OUTPUT_DATA_HOOK_CONFIG__))
            #if (__SUPPORT_ELECTRODE_WEAR_STATUS_DUMP__)
                GH3X2X_ReadElectrodeWearDumpData();
            #endif
                GH3X2X_RecordDumpData();
            #endif
                GH3X2X_RecordTiaGainInfo();
            }
        }

    #if (__FUNC_TYPE_ECG_ENABLE__)
        /* extra step: ECG Gain calibration*/
        #if !(__GH3X2X_CASCADE_EN__)
        GH3X2X_CalibrateECGGain(g_puchGh3x2xReadRawdataBuffer,g_usGh3x2xReadRawdataLen);
        #else
        if (GH3X2X_CascadeGetEcgEnFlag())
        {
            GH3X2X_CascadeCalibrateECGGain(g_puchGh3x2xReadRawdataBuffer,g_usGh3x2xReadRawdataLen);
        }
        else
        {
            GH3X2X_CalibrateECGGain(g_puchGh3x2xReadRawdataBuffer,g_usGh3x2xReadRawdataLen);
        }
        #endif

        /* Step 4: if ECG enabled, do soft lead off process */
        #if (__GH3X2X_CASCADE_EN__)
        if (GH3X2X_CascadeGetEcgEnFlag())
        {
            usGotEvent = GH3X2X_CascadeLeadOnEventPreDeal(usGotEvent, usGotSlaverEvent,&uchEventEx);
        }
        #endif
        stLeadResult = GH3X2X_LeadHandle(usGotEvent, g_puchGh3x2xReadRawdataBuffer, &g_usGh3x2xReadRawdataLen);
        usGotEvent = stLeadResult.usEvent;

        if(stLeadResult.uchNeedForceReadFifo)
        {
            GH3X2X_SetSoftEvent(GH3X2X_SOFT_EVENT_NEED_FORCE_READ_FIFO);
        }
        GH3X2X_EcgRsHandle(g_puchGh3x2xReadRawdataBuffer, &g_usGh3x2xReadRawdataLen);
    #endif

        /* Step 5: do soft agc process*/
        Gh3x2x_UserHandleCurrentInfo();
    #if (__SUPPORT_ENGINEERING_MODE__)
        if(0 == g_uchEngineeringModeStatus)  //in engineering mode,  disable soft agc function
    #endif
            {
            if(0 != (usGotEvent & (GH3X2X_IRQ_MSK_TUNNING_FAIL_BIT | GH3X2X_IRQ_MSK_TUNNING_DONE_BIT)))
            {
//                GH3X2X_SoftLedADJAutoADJInt();
            }
        #if (__SUPPORT_SAMPLE_DEBUG_MODE__)
            if((g_usDumpMode & 0x3) != 0)
            {
                GH3X2X_LedAgcProcessExDump(g_puchGh3x2xReadRawdataBuffer);
            }
            else
        #endif
            {    
                GH3X2X_LedAgcProcess(g_puchGh3x2xReadRawdataBuffer, g_usGh3x2xReadRawdataLen);
            }
        }

        /* Extern Step: send fifo*/
    #if (__FIFO_PACKAGE_SEND_ENABLE__)
        if (GH3X2X_GetFifoPackageMode())
        {
            GH3X2X_SendRawdataFifoPackage(g_puchGh3x2xReadRawdataBuffer, g_usGh3x2xReadRawdataLen);
        }
    #endif

        /* Step 6: chip reset event process*/
        if(0 != (usGotEvent & GH3X2X_IRQ_MSK_CHIP_RESET_BIT))
        {
            if(0 == GH3x2x_GetChipResetRecoveringFlag())  //do nothing in recover period
            {
                EXAMPLE_LOG("Got chip reset event !!! ActiveChipResetFlag = %d, g_unDemoFuncMode = %d, recovering flag = %d\r\n",(int)GH3x2x_GetActiveChipResetFlag(), (int)g_unDemoFuncMode, (int)GH3x2x_GetChipResetRecoveringFlag());
                if((GH3x2x_GetActiveChipResetFlag())||(0 == g_unDemoFuncMode))
                {
                    Gh3x2xDemoSamplingControl(0xFFFFFFFF, UPROTOCOL_CMD_STOP);
                    GH3X2X_Init(g_stGh3x2xCfgListArr+g_uchGh3x2xRegCfgArrIndex); 
                }
                else
                {
                    //need recovery for chip reset
                    GU8 uchTryCnt = 5;   //retry max count
                    GS8 schret = GH3X2X_RET_GENERIC_ERROR;
                    GU32 unDemoFuncModeBeforeChipReset;
                    EXAMPLE_LOG("Gh3x2xDemoInterruptProcess:start chip reset recovery...\r\n");
                    GH3x2x_SetChipResetRecoveringFlag(1);

                    
                    unDemoFuncModeBeforeChipReset = g_unDemoFuncMode;
                    do{
                            #if __SUPPORT_HARD_RESET_CONFIG__
                                GH3X2X_HardReset();
                            #else
                                GH3X2X_SoftReset();
                            #endif 
                                Gh3x2x_BspDelayMs(30);
                                
                                Gh3x2xDemoStopSampling(g_unDemoFuncMode);
                                
                                schret = GH3X2X_Init(g_stGh3x2xCfgListArr+g_uchGh3x2xRegCfgArrIndex);
                                
                                if (GH3X2X_RET_OK == schret)
                                {
                                    EXAMPLE_LOG("Gh3x2xDemoInterruptProcess:recovery init success\r\n");
                                }
                                else
                                {
                                    //g_uchGh3x2xRegCfgArrIndex = DRV_LIB_REG_CFG_EMPTY;
                                    EXAMPLE_LOG("Gh3x2xDemoInterruptProcess:recovery init fail, error code: %d\r\n", schret);
                                }
                                uchTryCnt --;
                        }while((GH3X2X_RET_OK != schret)&&(0 < uchTryCnt));

                    GH3X2X_EnterLowPowerMode();
                #if __GH3X2X_CASCADE_EN__
                    GH3X2X_CascadeOperationSlaverChip();
                    GH3X2X_EnterLowPowerMode();
                    GH3X2X_CascadeOperationMasterChip();
                #endif

                #if (__SUPPORT_ENGINEERING_MODE__)
                    if(1 == g_uchEngineeringModeStatus)
                    {
                        Gh3x2xDemoStartSamplingForEngineeringMode(unDemoFuncModeBeforeChipReset, g_uchGh3x2xRegCfgArrIndex, g_pstSampleParaGroup, g_uchEngineeringModeSampleParaGroupNum);
                    }
                    else
                #endif
                    {
                        Gh3x2xDemoStartSampling(unDemoFuncModeBeforeChipReset);
                    }
                    GH3x2x_SetChipResetRecoveringFlag(0);
                }
            }
        }

    #if !(__GH3X2X_CASCADE_EN__)
        GH3X2X_EnterLowPowerMode();
        #else
        if (GH3X2X_CascadeGetEcgEnFlag())
        {
            GH3X2X_EnterLowPowerMode();
            GH3X2X_CascadeOperationSlaverChip();
            GH3X2X_EnterLowPowerMode();
            GH3X2X_CascadeOperationMasterChip();
        }
        else
        {
            GH3X2X_EnterLowPowerMode();
        }
    #endif

        /* Step 7: report event */
    #if (__SUPPORT_PROTOCOL_ANALYZE__)
    #if (__SUPPORT_SAMPLE_DEBUG_MODE__)
        if (GH3X2X_ElectrodeWearRevertDebugModeIsEnabled())
        {
            if (usGotEvent & GH3X2X_IRQ_MSK_WEAR_ON_BIT)
            {
                GH3X2X_AddElectrodeWearRevCnt();
            }
            usGotEvent &= ~(GH3X2X_IRQ_MSK_WEAR_ON_BIT | GH3X2X_IRQ_MSK_WEAR_OFF_BIT);
        }
    #endif
        Gh3x2xDemoReportEvent(usGotEvent,uchEventEx);
        if(uchEventEx&GH3X2X_EVENT_EX_BIT_CHIP0_LEAD_ON)
        {
            EXAMPLE_LOG("chip0 lead on\r\n");
            //while(1);
        }
        if(uchEventEx&GH3X2X_EVENT_EX_BIT_CHIP1_LEAD_ON)
        {
            EXAMPLE_LOG("chip1 lead on\r\n");
            //while(1);
        }
    #endif

        if ((usGotEvent & (GH3X2X_IRQ_MSK_FIFO_WATERMARK_BIT | GH3X2X_IRQ_MSK_FIFO_FULL_BIT)) \
            || (GH3X2X_GetSoftEvent() & GH3X2X_SOFT_EVENT_NEED_FORCE_READ_FIFO))
        {
        #if __GS_NONSYNC_READ_EN__
            GU16 usGsensorNeedPointNum;
            GS16 sGsensorHeadPosi;
        #endif
            if (GH3X2X_GetGsensorEnableFlag())
            {
            #if (0 == __GS_TIMESTAMP_READ_EN__)
            #if (__FUNC_TYPE_SOFT_ADT_ENABLE__)
                if (g_uchVirtualAdtTimerCtrlStatus == 0)
            #endif
                {
                    hal_gsensor_drv_get_fifo_data(gsensor_soft_fifo_buffer + __GS_EXTRA_BUF_LEN__, &gsensor_soft_fifo_buffer_index);        
                    Gh3x2x_NormalizeGsensorSensitivity(gsensor_soft_fifo_buffer + __GS_EXTRA_BUF_LEN__ ,gsensor_soft_fifo_buffer_index);
                }
            #endif
            }
            /*extra step: get cap and temp data from other sensor*/
            #if __CAP_ENABLE__
            if (GH3X2X_GetCapEnableFlag())
            {
                hal_cap_drv_get_fifo_data(cap_soft_fifo_buffer, &cap_soft_fifo_buffer_index);
            }
            #endif
            #if __TEMP_ENABLE__
            if (GH3X2X_GetTempEnableFlag())
            {
                hal_temp_drv_get_fifo_data(temp_soft_fifo_buffer, &temp_soft_fifo_buffer_index);
            }
            #endif
            /* Step 8: algorithm process*/
            #if __GH3X2X_MEM_POOL_CHECK_EN__
            Gh3x2xCheckMemPollBeforeAlgoCal();
            #endif

        #ifdef GOODIX_DEMO_PLANFORM
            if (((GH3X2X_DEMO_WORK_MODE_MCU_ONLINE == g_uchDemoWorkMode) ||\
                        (GH3X2X_DEMO_WORK_MODE_PASS_THROUGH == g_uchDemoWorkMode) ||\
                        (GH3X2X_DEMO_WORK_MODE_APP == g_uchDemoWorkMode) ||\
                        (GH3X2X_DEMO_WORK_MODE_EVK == g_uchDemoWorkMode)) &&\
                        ((g_usDumpMode & 0x3) == 0))
        #endif
            {
            #if __GS_NONSYNC_READ_EN__
                usGsensorNeedPointNum = Gh3x2xGetGsensorNeedPointNum(gsensor_soft_fifo_buffer_index, g_puchGh3x2xReadRawdataBuffer, g_usGh3x2xReadRawdataLen);
                sGsensorHeadPosi = Gh3x2xGetGsensorHeadPos(gsensor_soft_fifo_buffer_index,usGsensorNeedPointNum);    
                Gh3x2xDemoFunctionProcess(g_puchGh3x2xReadRawdataBuffer,g_usGh3x2xReadRawdataLen, gsensor_soft_fifo_buffer + __GS_EXTRA_BUF_LEN__ + sGsensorHeadPosi, usGsensorNeedPointNum,
                                                cap_soft_fifo_buffer,cap_soft_fifo_buffer_index,temp_soft_fifo_buffer,temp_soft_fifo_buffer_index);


                for(GU16 usPointCnt = 0; usPointCnt < usGsensorNeedPointNum; usPointCnt++)
                {
                    STGsensorRawdata * temp_gsensor = gsensor_soft_fifo_buffer + __GS_EXTRA_BUF_LEN__ + sGsensorHeadPosi;
                    EXAMPLE_LOG("[Gsensor show]x = %d,y = %d, z = %d\r\n",temp_gsensor[usPointCnt].sXAxisVal,temp_gsensor[usPointCnt].sYAxisVal,temp_gsensor[usPointCnt].sZAxisVal);
                }

                
                Gh3x2xNonSyncGsenosrPostProcess(gsensor_soft_fifo_buffer_index, usGsensorNeedPointNum, sGsensorHeadPosi); 
            #else
                Gh3x2xDemoFunctionProcess(g_puchGh3x2xReadRawdataBuffer,g_usGh3x2xReadRawdataLen, gsensor_soft_fifo_buffer + __GS_EXTRA_BUF_LEN__, gsensor_soft_fifo_buffer_index,
                                                cap_soft_fifo_buffer,cap_soft_fifo_buffer_index,temp_soft_fifo_buffer,temp_soft_fifo_buffer_index);
            #endif
            }
            #if __GH3X2X_MEM_POOL_CHECK_EN__
            Gh3x2xUpdataMemPollChkSumAfterAlgoCal();
            #endif
        }

        ///* Step 9: event hook */
    #if (__FUNC_TYPE_ECG_ENABLE__)
        if (usGotEvent & GH3X2X_IRQ_MSK_LEAD_ON_DET_BIT)
        {
            Gh3x2x_LeadOnEventHook();
            EXAMPLE_LOG("Lead on!!!\r\n");
        }
        if (usGotEvent & GH3X2X_IRQ_MSK_LEAD_OFF_DET_BIT)
        {
            Gh3x2x_LeadOffEventHook();
            EXAMPLE_LOG("Lead off!!!\r\n");
        }
    #endif

    #if (__SUPPORT_HARD_ADT_CONFIG__)
        if ((usGotEvent & GH3X2X_IRQ_MSK_WEAR_ON_BIT) || (usGotEvent & GH3X2X_IRQ_MSK_WEAR_OFF_BIT))
        {
        #if (__GH3X2X_CASCADE_EN__)
            Gh3x2x_WearEventCascadeEcgHandle(usGotEvent);
        #endif
            Gh3x2x_WearEventHook(usGotEvent, uchEventEx);
        }
    #endif

        if (GH3X2X_GetSoftEvent())  //have new soft event
        {
            uchRet = GH3X2X_INT_PROCESS_REPEAT;
            EXAMPLE_LOG("InterruptProcess: process need repeat.\r\n");
        }

        if (uchIntRepeatNum > 20)
        {
            EXAMPLE_LOG("InterruptProcess: 0 Warning!!!!GetEvent Error!!!0x%x\r\n", usGotEvent);
            EXAMPLE_LOG("InterruptProcess: 1 Warning!!!!GetEvent Error!!!0x%x\r\n", usGotEvent);
            EXAMPLE_LOG("InterruptProcess: 2 Warning!!!!GetEvent Error!!!0x%x\r\n", usGotEvent);
            g_uchGh3x2xInitFlag = 0;
            break;
        }
        else
        {
            uchIntRepeatNum++;
        }
    }while(GH3X2X_INT_PROCESS_REPEAT == uchRet);
    g_uchGh3x2xInterruptProNotFinishFlag = 0;
}

#if __GH3X2X_CASCADE_EN__
GU8 Gh3x2xCheckCfgValidForCascade(GU8 uchCfgIndex)
{
    if(GH3X2X_CascadeGetEcgEnFlag())
    {
        if(2 != uchCfgIndex)   //In cascade mode, only cfg2 is valid
        {
            return 0;
        }
    }
    else
    {
        if(2 == uchCfgIndex)   //Not in cascade mode, cfg2 is invalid
        {
            return 0;
        }
    }
    return 1;
}
#endif

/**
 * @fn     GS8 GH3x2xDemoSearchCfgListByFunc(GU8* puchCfgIndex, GU32 unFuncMode, GU32 unFuncModeFilter)
 *
 * @brief  Search reg config array by function mode
 *
 * @attention   None
 *
 * @param[in]   unFuncMode              function mode
 * @param[in]   unFuncModeFilter        ignored function mode
 * @param[out]  puchCfgIndex            reg config array index
 *
 * @return  GH3X2X_RET_OK:switch application mode success
 * @return  GH3X2X_RET_RESOURCE_ERROR:can't find corresponding reg config array
 */
GS8 GH3x2xDemoSearchCfgListByFunc(GU8* puchCfgIndex, GU32 unFuncMode, GU32 unFuncModeFilter)
{
    GS8  chRet = GH3X2X_RET_OK;
    GU8  uchIndex = 0;
    GU32 unFuncModeTemp = GH3X2X_NO_FUNCTION;
    //find reg config array that match unFuncMode
    EXAMPLE_LOG("target app mode:0x%x\r\n",unFuncMode);
    for (uchIndex = 0; uchIndex < __GH3X2X_CFG_LIST_MAX_NUM__; uchIndex++)
    {
        if (
            (((g_stGh3x2xCfgListArr+uchIndex)->usConfigArrLen) > DRV_LIB_REG_CFG_MIN_LEN)
            #if __GH3X2X_CASCADE_EN__
            &&(1 == Gh3x2xCheckCfgValidForCascade(uchIndex))
            #endif
            )
        {
            GH3X2X_DecodeRegCfgArr(&unFuncModeTemp,(g_stGh3x2xCfgListArr+uchIndex)->pstRegConfigArr, \
                                (g_stGh3x2xCfgListArr+uchIndex)->usConfigArrLen);

            
            EXAMPLE_LOG("search cfg app mode:0x%x\r\n",unFuncModeTemp);
                                
            if((unFuncMode | unFuncModeFilter) == ((unFuncModeTemp&unFuncMode) | unFuncModeFilter))
            {
                (*puchCfgIndex) = uchIndex;
                EXAMPLE_LOG("Got right cfg ,index = %d\r\n",uchIndex);
                break;
            }
        }
    }

    if (uchIndex >= __GH3X2X_CFG_LIST_MAX_NUM__)
    {
        chRet = GH3X2X_RET_RESOURCE_ERROR;
    }
    return chRet;
}


/**
 * @fn     GS8 Gh3x2xDemoArrayCfgSwitch(GU8 uchArrayCfgIndex)
 *
 * @brief  array cfg switch (call by user)
 *
 * @attention   you should switch array cfg manually before calling Gh3x2xDemoStartSampling
 *
 * @param[in]   uchArrayCfgIndex    0: gh3x2x_reg_list0    1: gh3x2x_reg_list0 ...
 * @param[out]  None
 *
 * @return  GH3X2X_RET_OK:switch application mode success
 * @return  GH3X2X_RET_RESOURCE_ERROR:can't find corresponding reg config array
 */
GS8 Gh3x2xDemoArrayCfgSwitch(GU8 uchArrayCfgIndex)
{
    GS8  chRet = GH3X2X_RET_OK;

    if(g_uchGh3x2xRegCfgArrIndex == uchArrayCfgIndex)
    {
        EXAMPLE_LOG("Current cfg is already cfg%d, no need switch.\r\n",(int)uchArrayCfgIndex);
    }
    else if(uchArrayCfgIndex < __GH3X2X_CFG_LIST_MAX_NUM__)
    {
        Gh3x2xDemoSamplingControl(0xFFFFFFFF, UPROTOCOL_CMD_STOP);
        #if __SUPPORT_HARD_RESET_CONFIG__
        GH3X2X_HardReset();
        #else
        GH3X2X_SoftReset();
        #endif   
        Gh3x2x_BspDelayMs(15);  
        #if __GH3X2X_CASCADE_EN__
        GH3X2X_CascadeOperationSlaverChip();
        GH3X2X_CascadeOperationMasterChip();
        #endif

        g_uchGh3x2xRegCfgArrIndex = uchArrayCfgIndex;
        if (GH3X2X_RET_OK == GH3X2X_Init(g_stGh3x2xCfgListArr + uchArrayCfgIndex))
        {
            EXAMPLE_LOG("cfg%d switch success !!!.\r\n",(int)uchArrayCfgIndex);
        }
        else
        {
            g_uchGh3x2xRegCfgArrIndex = DRV_LIB_REG_CFG_EMPTY;
            chRet = GH3X2X_RET_RESOURCE_ERROR;
            EXAMPLE_LOG("Error ! cfg%d switch fail !!!\r\n",(int)uchArrayCfgIndex);
        }
        GH3X2X_EnterLowPowerMode();
        #if __GH3X2X_CASCADE_EN__
        GH3X2X_CascadeOperationSlaverChip();
        GH3X2X_EnterLowPowerMode();
        GH3X2X_CascadeOperationMasterChip();
        #endif
        
    }
    else
    {
        EXAMPLE_LOG("Error! uchArrayCfgIndex(%d) is invalid !!!\r\n",(int)uchArrayCfgIndex);
        chRet = GH3X2X_RET_RESOURCE_ERROR;
    }
    return chRet;
}

void Gh3x2xSetCurrentSlotEnReg(GU32 unFuncMode)
{
    GU8 uchSlotEn = 0;

    #if __SLOT_SEQUENCE_DYNAMIC_ADJUST_EN__
    GU8  uchSlotSeq[8];
    GU8  uchValidSlotNum = 0;
    GU16 usStartReg;
    GU8 uchSlotSrDiver[8];
    GU8 uchSlotCfgHaveSortedFlag[8] = {0};
    GU8 uchSlotCfgIndexFast2Low[8];
    #endif
    for(GU8 uchFunCnt = 0; uchFunCnt < GH3X2X_FUNC_OFFSET_MAX; uchFunCnt ++)
    {
        if(g_pstGh3x2xFrameInfo[uchFunCnt])
        {
            if(g_unDemoFuncMode & (((GU32)1)<< uchFunCnt))
            {
                #if (__FUNC_TYPE_SOFT_ADT_ENABLE__)
                if((GH3X2X_FUNC_OFFSET_ADT != uchFunCnt)||(1 == g_uchHardAdtFuncStatus))
                #endif
                {
                    uchSlotEn |= g_pstGh3x2xFrameInfo[uchFunCnt]->pstFunctionInfo->uchSlotBit;
                }
            }
        }
    }
    #if __SLOT_SEQUENCE_DYNAMIC_ADJUST_EN__
    for(GU8 uchSlotCnt = 0; uchSlotCnt < 8; uchSlotCnt ++)
    {
        //get diver for every slot cfg
        uchSlotSrDiver[uchSlotCnt] = GH3x2xGetAgcReg(GH3X2X_AGC_REG_SR_MULIPLIPER, uchSlotCnt);
    }
    
    //EXAMPLE_LOG("%s : Slot cfg Sort fast to slow: \r\n", __FUNCTION__);
    for(GU8 uchSlotCnt = 0; uchSlotCnt < 8; uchSlotCnt ++)
    {
        //find min diver
        GU16 usCurrentMinDiver = 0xFFFF;
        GU8 uchCurrentMinDiverCfgIndex = 0xFF;
        for(GU8 uchSearchCnt = 0; uchSearchCnt < 8; uchSearchCnt ++)
        {
            if(0 == uchSlotCfgHaveSortedFlag[uchSearchCnt])
            {
                if(usCurrentMinDiver > uchSlotSrDiver[uchSearchCnt])
                {
                    usCurrentMinDiver = uchSlotSrDiver[uchSearchCnt];
                    uchCurrentMinDiverCfgIndex = uchSearchCnt;
                }
            }
        }
        if(0xFF != uchCurrentMinDiverCfgIndex)
        {
            uchSlotCfgHaveSortedFlag[uchCurrentMinDiverCfgIndex] = 1;
            uchSlotCfgIndexFast2Low[uchSlotCnt] = uchCurrentMinDiverCfgIndex;
            
            //EXAMPLE_LOG("%d \r\n", uchCurrentMinDiverCfgIndex);
        }
    }
    //EXAMPLE_LOG("\r\n");


    //find slot cfg from fastest to lowest
    for(GU8 uchSlotCnt = 0; uchSlotCnt < 8; uchSlotCnt ++)
    {
        GU8 uchCurrentFastSlotCfgIndex = uchSlotCfgIndexFast2Low[uchSlotCnt];
        for(GU8 uchSlotSearchCnt = 0; uchSlotSearchCnt < 8; uchSlotSearchCnt ++)
        {
            if(uchSlotEn&(1<<uchSlotSearchCnt))
            {
                if(uchSlotSearchCnt == uchCurrentFastSlotCfgIndex)
                {
                    uchSlotSeq[uchValidSlotNum] = uchSlotSearchCnt;
                    uchValidSlotNum ++;
                }
            }
        }
    }
    if(uchValidSlotNum < 8)
    {
        uchSlotSeq[uchValidSlotNum] = 8;
        uchValidSlotNum ++;
    }
    else
    {
        uchValidSlotNum = 8;
    }

    
    usStartReg = GH3X2X_ReadReg(0x0000);  //bak start reg
    GH3X2X_WriteReg(0x0000,usStartReg&(0xFFFE));   //stop sample
    //write 0x0100~0x0106 and 0x01EC
    for(GU8 uchSlotCnt = 0; uchSlotCnt < uchValidSlotNum; uchSlotCnt ++)
    {
        if(0 == (uchSlotCnt&0x01)) //0,2,4,6
        {
            GH3X2X_WriteRegBitField(0x0100 + (uchSlotCnt&0xFE), 0,3, uchSlotSeq[uchSlotCnt]);
        }
        else  //1,3,5,7
        {
            GH3X2X_WriteRegBitField(0x0100 + (uchSlotCnt&0xFE), 8,11,  uchSlotSeq[uchSlotCnt]);
        }
        if(uchSlotSeq[uchSlotCnt] < 8)
        {
            GH3X2X_WriteReg(0x01EC + uchSlotCnt*2, g_usGh3x2xSlotTime[uchSlotSeq[uchSlotCnt]]);
            //EXAMPLE_LOG("uchSlotCnt = %d, uchSlotSeq = %d, slottime = %d\r\n",uchSlotCnt,uchSlotSeq[uchSlotCnt],g_usGh3x2xSlotTime[uchSlotSeq[uchSlotCnt]]);
        }
    }
    GH3X2X_WriteReg(0x0000,usStartReg);   //recover sample reg
    #endif
    GH3X2X_SlotEnRegSet(uchSlotEn);

    #if __GH3X2X_CASCADE_EN__
    if(GH3X2X_CascadeGetEcgEnFlag())
    {
        GH3X2X_EcgCascadeSlotEnRegSet(uchSlotEn);
    }
    #endif

    
    EXAMPLE_LOG("Current uchSlotEn= 0x%x\r\n",uchSlotEn);
}

#define GH3X2X_ECG_CTRL_FLAG_START 1
#define GH3X2X_ECG_CTRL_FLAG_STOP  2

#define GH3X2X_HARD_ADT_CTRL_FLAG_START 1
#define GH3X2X_HARD_ADT_CTRL_FLAG_STOP  2
#if __SUPPORT_FUNCTION_SAMPLE_RATE_MODIFY__
void Gh3x2xSampleRateUpdate(GU32 unFuncMode)
{
    GU16 usLastSampleRate[8] = {0};  // last sample rate for every slot
    GU16 usTargetSampleRate[8] = {0};
    GU8  uchHaveStopSample = 0;
    GU16 usStartReg;


    //read sample rate reg
    for(GU8 uchSlotCnt = 0; uchSlotCnt < 8; uchSlotCnt ++)
    {
        usLastSampleRate[uchSlotCnt] = 1000/(GH3x2xGetAgcReg(GH3X2X_AGC_REG_SR_MULIPLIPER, uchSlotCnt) + 1);
    }


    
    //get every slot target sample rate
    for(GU8 uchFunctionCnt = 0; uchFunctionCnt < GH3X2X_FUNC_OFFSET_MAX; uchFunctionCnt++)
    {
        if((g_pstGh3x2xFrameInfo[uchFunctionCnt])&&(unFuncMode&((GU32)1<<uchFunctionCnt)))     //function is valid and opened
        {
            GU16 usTempTargetSampleRate = g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstFunctionInfo->usSampleRateForUserSetting;
            if(0 == usTempTargetSampleRate)
            {
                usTempTargetSampleRate = g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstFunctionInfo->usSampleRate;
            }
            GU8 uchTempSlotBit = g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstFunctionInfo->uchSlotBit;            
            EXAMPLE_LOG("[Gh3x2xSampleRateUpdate]FunctionId = 0x%X, slot bit = 0x%X,usSampleRateUser = %d\r\n",g_pstGh3x2xFrameInfo[uchFunctionCnt]->unFunctionID,uchTempSlotBit, usTempTargetSampleRate);
            for(GU8 uchSlotCnt = 0; uchSlotCnt < 8; uchSlotCnt ++)
            {
                if(uchTempSlotBit&(1<<uchSlotCnt))
                {
                    if(usTempTargetSampleRate > usTargetSampleRate[uchSlotCnt])  
                    {
                        usTargetSampleRate[uchSlotCnt] = usTempTargetSampleRate;   //store faster sample rate
                    }
                }
            }
        }
    }


    //update sample rate registers
    for(GU8 uchSlotCnt = 0; uchSlotCnt < 8; uchSlotCnt ++)
    {
        if((usTargetSampleRate[uchSlotCnt] != usLastSampleRate[uchSlotCnt])&&(0 != usTargetSampleRate[uchSlotCnt]))
        {
            if(0 == uchHaveStopSample)
            {
                usStartReg = GH3X2X_ReadReg(0x0000);  //bak start reg
                GH3X2X_WriteReg(0x0000,usStartReg&(0xFFFE));   //stop sample
                uchHaveStopSample = 1;
            }
            GH3x2xSetAgcReg(GH3X2X_AGC_REG_SR_MULIPLIPER, uchSlotCnt, (1000/usTargetSampleRate[uchSlotCnt]) - 1);
            EXAMPLE_LOG("[Gh3x2xSampleRateUpdate]Slot%d sample rate need change: Last = %d, Target = %d\r\n",uchSlotCnt, usLastSampleRate[uchSlotCnt], usTargetSampleRate[uchSlotCnt]);
        }
    }
    if(uchHaveStopSample)
    {
        GH3X2X_WriteReg(0x0000,usStartReg);   //recover sample reg
    }
    


    //update usSampleRate value,and down sample check
    for(GU8 uchFunctionCnt = 0; uchFunctionCnt < GH3X2X_FUNC_OFFSET_MAX; uchFunctionCnt++)
    {
        GU16 usSlowestSlotSampleRate = 0xFFFF;
        
        if((g_pstGh3x2xFrameInfo[uchFunctionCnt])&&(unFuncMode&((GU32)1<<uchFunctionCnt)))     //function is valid and opened
        {
            GU8 uchTempSlotBit = g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstFunctionInfo->uchSlotBit;
            //update update usSampleRate value
            if(g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstFunctionInfo->usSampleRateForUserSetting)
            {
                g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstFunctionInfo->usSampleRate = g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstFunctionInfo->usSampleRateForUserSetting;
            }
            //get slowest slot in  function (target sample rate)
            for(GU8 uchSlotCnt = 0; uchSlotCnt < 8; uchSlotCnt ++)
            {
                if(uchTempSlotBit&(1<<uchSlotCnt))
                {
                    if(usSlowestSlotSampleRate > usTargetSampleRate[uchSlotCnt])
                    {
                        usSlowestSlotSampleRate = usTargetSampleRate[uchSlotCnt];
                    }
                }
            }
            if(usSlowestSlotSampleRate > g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstFunctionInfo->usSampleRate)   //need down sample
            {
                g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstDownSampleInfo->uchDownSampleFactor = (usSlowestSlotSampleRate/g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstFunctionInfo->usSampleRate) - 1;
            }
            else
            {
                g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstDownSampleInfo->uchDownSampleFactor = 0;
            }
            g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstDownSampleInfo->uchDownSampleCnt = 0;    
            EXAMPLE_LOG("[Gh3x2xSampleRateUpdate]Down sample: FunctionID = 0x%X,: SlowestSlotSR = %d, Factor = %d\r\n",g_pstGh3x2xFrameInfo[uchFunctionCnt]->unFunctionID, usSlowestSlotSampleRate, g_pstGh3x2xFrameInfo[uchFunctionCnt]->pstDownSampleInfo->uchDownSampleFactor);

        }
        
    }
    
}
#endif

void GH3x2xMaserControlFunctionLog(GU32 unFuncMode, EMUprotocolParseCmdType emCmdType)
{
    if(UPROTOCOL_CMD_START == emCmdType)
    {
        EXAMPLE_LOG("Master start function = 0x%x\r\n",unFuncMode);
    }
    else
    {
        EXAMPLE_LOG("Master stop function = 0x%x\r\n",unFuncMode);
    }
}




/**
 * @fn     void Gh3x2xDemoSamplingControl(EMUprotocolParseCmdType emSwitch)
 *
 * @brief  Start/stop sampling of gh3x2x and sensor
 *
 * @attention   None
 *
 * @param[in]   unFuncMode      function that will be started or stopped
 * @param[in]   emSwitch        stop/start sampling
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xDemoSamplingControl(GU32 unFuncMode, EMUprotocolParseCmdType emSwitch)
{

    if ((GH3X2X_GetConfigFuncMode() & unFuncMode) != unFuncMode)
    {
        EXAMPLE_LOG("[%s]:config has no target function!!!config function = 0x%08x,unFuncMode = 0x%08x\r\n", __FUNCTION__, GH3X2X_GetConfigFuncMode(),unFuncMode);
        unFuncMode = (GH3X2X_GetConfigFuncMode() & unFuncMode);
    }
    if(unFuncMode == 0)
    {
        return;
    }
    #if (__FUNC_TYPE_ECG_ENABLE__)
    GU8 uchEcgCtrlFlag = 0;
    #endif
    #if __GS_NONSYNC_READ_EN__
    GU16 usTempFunction;
    #endif
    #if __FUNC_TYPE_ECG_ENABLE__
    GU32 unLastDemoFuncMode = g_unDemoFuncMode;
    #endif

    #if !(__GH3X2X_CASCADE_EN__)
    #else
    if (GH3X2X_CascadeGetEcgEnFlag())
    {
        GH3X2X_CascadeOperationSlaverChip();
        GH3X2X_CascadeOperationMasterChip();

    }
    else
    {
    }
    #endif
    for(GU8 uchFunCnt = 0; uchFunCnt < GH3X2X_FUNC_OFFSET_MAX; uchFunCnt ++)
    {
        if(g_pstGh3x2xFrameInfo[uchFunCnt])
        {
            if(unFuncMode & (((GU32)1)<< uchFunCnt))
            {
                if((UPROTOCOL_CMD_START == emSwitch)&&((g_unDemoFuncMode & (((GU32)1)<< uchFunCnt)) != (((GU32)1)<< uchFunCnt)))
                {
                    if(GH3X2X_FUNC_OFFSET_ADT == uchFunCnt)
                    {
                    #if (__FUNC_TYPE_SOFT_ADT_ENABLE__)
                        if (GH3X2X_GetSoftWearOffDetEn())
                        {
                            GH3X2X_AdtFuncStartWithGsDetect();
                            GH3X2X_START_ADT_TIMER();
                        }
                        else
                        {
                            GH3X2X_FunctionStart(g_pstGh3x2xFrameInfo[uchFunCnt]);
                            g_uchHardAdtFuncStatus = 1;
                        }
                        g_uchWearDetectStatus = WEAR_DETECT_WEAR_ON;
                        GH3X2X_WearDetectSwitchTo(WEAR_DETECT_WEAR_ON,WEAR_DETECT_FORCE_SWITCH);
                    #else
                        GH3X2X_FunctionStart(g_pstGh3x2xFrameInfo[uchFunCnt]);
                    #endif
                    }
                    else
                    {
                        GH3X2X_FunctionStart(g_pstGh3x2xFrameInfo[uchFunCnt]);
                    #if __FUNC_TYPE_ECG_ENABLE__
                        if(GH3X2X_FUNC_OFFSET_ECG == uchFunCnt)
                        {
                            uchEcgCtrlFlag = GH3X2X_ECG_CTRL_FLAG_START;
                        }
                    #endif
                    }
                }
                else if ((UPROTOCOL_CMD_STOP == emSwitch)&&((g_unDemoFuncMode & (((GU32)1)<< uchFunCnt)) == (((GU32)1)<< uchFunCnt)))
                {
                    if(GH3X2X_FUNC_OFFSET_ADT == uchFunCnt)
                    {
                    #if (__SUPPORT_HARD_ADT_CONFIG__)
                    #if (__FUNC_TYPE_SOFT_ADT_ENABLE__)
                        if (GH3X2X_GetSoftWearOffDetEn())
                        {
                            Gh3x2x_ResetMoveDetectByGsData();
                            GH3X2X_STOP_ADT_TIMER();
                        }
                        g_uchHardAdtFuncStatus = 0;
                    #endif
                        GH3X2X_FunctionStop(g_pstGh3x2xFrameInfo[uchFunCnt]);
                        g_uchWearDetectStatus = WEAR_DETECT_WEAR_ON;
                        GH3X2X_WearDetectSwitchTo(WEAR_DETECT_WEAR_ON,WEAR_DETECT_FORCE_SWITCH);
                    #endif
                    #if (__SUPPORT_PROTOCOL_ANALYZE__)
                        Gh3x2xDemoReportEvent(WEAR_DETECT_WEAR_OFF,0);
                    #endif
                    }
                    else
                    {
                        GH3X2X_FunctionStop(g_pstGh3x2xFrameInfo[uchFunCnt]);
                    #if __FUNC_TYPE_ECG_ENABLE__
                        if(GH3X2X_FUNC_OFFSET_ECG == uchFunCnt)
                        {
                            uchEcgCtrlFlag = GH3X2X_ECG_CTRL_FLAG_STOP;
                        }
                    #endif
                    }
                }
            }
        }
    }




    /* Enable gsensor sampling */
    if (UPROTOCOL_CMD_START == emSwitch)
    {
#if (__DRIVER_LIB_MODE__ == __DRV_LIB_WITH_ALGO__)
        GH3X2X_TimestampSyncPpgInit(unFuncMode);
#endif
        if (GH3X2X_GetGsensorEnableFlag() && (GH3X2X_NO_FUNCTION == g_unDemoFuncMode))
        {
            hal_gsensor_start_cache_data();
        }
#if (__CAP_ENABLE__)
        if (GH3X2X_GetCapEnableFlag() && ((unFuncMode & (GH3X2X_FUNCTION_SOFT_ADT_GREEN | GH3X2X_FUNCTION_SOFT_ADT_IR)) != GH3X2X_NO_FUNCTION))
        {
            GH3x2xReadCapFromFlash();
        }
        if (GH3X2X_GetCapEnableFlag() && (GH3X2X_NO_FUNCTION == g_unDemoFuncMode))
        {
            hal_cap_start_cache_data();
        }
#endif
#if (__TEMP_ENABLE__)
        if (GH3X2X_GetTempEnableFlag() && (GH3X2X_NO_FUNCTION == g_unDemoFuncMode))
        {
            hal_temp_start_cache_data();
        }
#endif
        g_unDemoFuncMode |= unFuncMode;
    }
    else
    {
        g_unDemoFuncMode &= ~unFuncMode;
        if (GH3X2X_GetGsensorEnableFlag()  && (GH3X2X_NO_FUNCTION == g_unDemoFuncMode))
        {
            hal_gsensor_stop_cache_data();
        }
#if (__CAP_ENABLE__)
        if (GH3X2X_GetCapEnableFlag() && (GH3X2X_NO_FUNCTION == g_unDemoFuncMode))
        {
            if((unFuncMode & (GH3X2X_FUNCTION_SOFT_ADT_GREEN | GH3X2X_FUNCTION_SOFT_ADT_IR)) != GH3X2X_NO_FUNCTION)
            {
                GH3x2xWriteCapToFlash();
            }
            hal_cap_stop_cache_data();
        }
#endif
#if (__TEMP_ENABLE__)
        if (GH3X2X_GetTempEnableFlag() && (GH3X2X_NO_FUNCTION == g_unDemoFuncMode))
        {
            hal_temp_stop_cache_data();
        }
#endif
        GH3X2X_SetSoftEvent(GH3X2X_SOFT_EVENT_NEED_FORCE_READ_FIFO);
    }
    
#if __FUNC_TYPE_ECG_ENABLE__
            if(unLastDemoFuncMode&(GH3X2X_FUNCTION_ADT|GH3X2X_FUNCTION_LEAD_DET))  //have adt/lead function before change
            {
                if(0 == (g_unDemoFuncMode&(GH3X2X_FUNCTION_ADT|GH3X2X_FUNCTION_LEAD_DET))) //have no adt/lead function after change
                {
                    GH3X2X_LeadDetEnInHardAdt(0);
                }
            }
            else  //have no adt/lead function before change
            {
                if((g_unDemoFuncMode&(GH3X2X_FUNCTION_ADT|GH3X2X_FUNCTION_LEAD_DET))) //have adt/lead function after change
                {
                    GH3X2X_LeadDetEnInHardAdt(1);
                }
            }
#endif



#if __ADT_ONLY_PARTICULAR_WM_CONFIG__
    if(GH3X2X_FUNCTION_ADT == g_unDemoFuncMode) // only ADT function is opening
    {
        g_usCurrentConfigListFifoWmBak = GH3X2X_ReadReg(0x000A);     //bak current reg
        GH3X2X_WriteReg(0x000A, __ADT_ONLY_PARTICULAR_WM_CONFIG__);  //set particular watermark for ADT only
        g_usCurrentFiFoWaterLine = __ADT_ONLY_PARTICULAR_WM_CONFIG__;
        EXAMPLE_LOG("set particular watermark for ADT only. value = %d.\r\n",(int)__ADT_ONLY_PARTICULAR_WM_CONFIG__);
    }
    else
    {
        if(g_usCurrentConfigListFifoWmBak)  // if bak value is exist
        {
            GH3X2X_WriteReg(0x000A, g_usCurrentConfigListFifoWmBak);  // recover the watermark
            g_usCurrentFiFoWaterLine = g_usCurrentConfigListFifoWmBak;
            g_usCurrentConfigListFifoWmBak = 0;                        //clear bak value
            EXAMPLE_LOG("recover watermark. value = %d.\r\n",(int)g_usCurrentConfigListFifoWmBak);
        }
    }
    GH3X2X_SetSoftEvent(GH3X2X_SOFT_EVENT_NEED_FORCE_READ_FIFO);  //need force read fifo after setting watermark
#endif





#if __SUPPORT_FUNCTION_SAMPLE_RATE_MODIFY__
    Gh3x2xSampleRateUpdate(g_unDemoFuncMode);
#endif
    Gh3x2xSetCurrentSlotEnReg(g_unDemoFuncMode);
    EXAMPLE_LOG("Current g_unDemoFuncMode= 0x%x\r\n",g_unDemoFuncMode);
#if (__FUNC_TYPE_ECG_ENABLE__)
    if(GH3X2X_ECG_CTRL_FLAG_START == uchEcgCtrlFlag)
    {
        GH3X2X_EcgSampleHookHandle(ECG_SAMPLE_EVENT_TYPE_SLOT,g_pstGh3x2xFrameInfo[GH3X2X_FUNC_OFFSET_ECG]->pstFunctionInfo->uchSlotBit);
    }
    else if(GH3X2X_ECG_CTRL_FLAG_STOP == uchEcgCtrlFlag)
    {
        GH3X2X_EcgSampleHookHandle(ECG_SAMPLE_EVENT_TYPE_SLOT,~(g_pstGh3x2xFrameInfo[GH3X2X_FUNC_OFFSET_ECG]->pstFunctionInfo->uchSlotBit));
    }
#endif

#if !(__GH3X2X_CASCADE_EN__)
    GH3X2X_EnterLowPowerMode();
    #else
    if (GH3X2X_CascadeGetEcgEnFlag())
    {
        GH3X2X_EnterLowPowerMode();
        GH3X2X_CascadeOperationSlaverChip();
        GH3X2X_EnterLowPowerMode();
        GH3X2X_CascadeOperationMasterChip();

    }
    else
    {
        GH3X2X_EnterLowPowerMode();
    }
#endif

#if __GS_NONSYNC_READ_EN__
    usTempFunction = Gh3x2xGetCurrentRefFunction();
    if(usTempFunction != g_usGsenorRefFunction)
    {
        g_usGsenorRefFunction = usTempFunction;
        Gh3x2xNonSyncGsensorInit();
    }
#endif

    if(GH3X2X_GetSoftEvent()&&(0 == g_uchGh3x2xInterruptProNotFinishFlag))   //avoid nesting
    {
        Gh3x2xDemoInterruptProcess();
    }
}

#if (__SUPPORT_HARD_ADT_CONFIG__)
void GH3X2X_StartHardAdtAndResetGsDetect(void)
{
    EXAMPLE_LOG("[%s]\r\n", __FUNCTION__);
    
    GH3X2X_FunctionStart(g_pstGh3x2xFrameInfo[GH3X2X_FUNC_OFFSET_ADT]);
#if (__FUNC_TYPE_SOFT_ADT_ENABLE__)
    g_uchHardAdtFuncStatus = 1;
    Gh3x2x_ResetMoveDetectByGsData();
#endif
    Gh3x2xSetCurrentSlotEnReg(GH3X2X_FUNCTION_ADT);
    
}




void GH3X2X_StopHardAdtAndStartGsDetect(void)
{
    EXAMPLE_LOG("[%s]\r\n", __FUNCTION__);
    GH3X2X_FunctionStop(g_pstGh3x2xFrameInfo[GH3X2X_FUNC_OFFSET_ADT]);
    GU16 usGotEvent = 0;
    usGotEvent |= GH3X2X_IRQ_MSK_WEAR_OFF_BIT;
    Gh3x2xDemoWearEventProcess(&usGotEvent,WEAR_DETECT_FORCE_SWITCH);
#if (__SUPPORT_PROTOCOL_ANALYZE__)
    Gh3x2xDemoReportEvent(usGotEvent,0);
#endif
#if (__FUNC_TYPE_SOFT_ADT_ENABLE__)
    g_uchHardAdtFuncStatus = 0;
    Gh3x2x_ResetMoveDetectByGsData();
#endif
    Gh3x2xSetCurrentSlotEnReg(g_unDemoFuncMode&(~GH3X2X_FUNCTION_ADT));
#if (__FUNC_TYPE_SOFT_ADT_ENABLE__)
    GH3X2X_START_ADT_TIMER();
#endif
}
#endif


void Gh3x2xFunctionSlotBitInit(void)
{
    for(GU8 uchFunctionCnt = 0; uchFunctionCnt < GH3X2X_FUNC_OFFSET_MAX; uchFunctionCnt++)
    {
        if(g_pstGh3x2xFrameInfo[uchFunctionCnt])
        {
            GH3x2xCalFunctionSlotBit(g_pstGh3x2xFrameInfo[uchFunctionCnt]);
        }
    }
}



void Gh3x2xEnterCascadeMode(void)
{
#if __GH3X2X_CASCADE_EN__
    Gh3x2xDemoStopSampling(0xFFFFFFFF);   //stop all function
#if __USER_DYNAMIC_DRV_BUF_EN__
    if(0 == g_puchGh3x2xReadRawdataBufferSlaveChip)
    {
        g_puchGh3x2xReadRawdataBufferSlaveChip = Gh3x2xMallocUser(__GH3X2X_RAWDATA_BUFFER_SIZE__);
    }
    if(0 == Gh3x2xDemoMemCheckValid())
    {
        while(1);
    }
#endif
    GH3X2X_CascadeSetEcgEnFlag(1);
    GH3X2X_CascadeOperationSlaverChip();
    GH3X2X_CascadeOperationMasterChip();

    g_uchGh3x2xRegCfgArrIndex = 2;
    if (GH3X2X_RET_OK != GH3X2X_Init(g_stGh3x2xCfgListArr+2))   //load ecg master and slave cfg
    {
        g_uchGh3x2xRegCfgArrIndex = DRV_LIB_REG_CFG_EMPTY;
    }
    GH3X2X_EnterLowPowerMode();
    GH3X2X_CascadeOperationSlaverChip();
    GH3X2X_EnterLowPowerMode();
    GH3X2X_CascadeOperationMasterChip();
#endif
}
void Gh3x2xExitCascadeMode(void)
{
#if __GH3X2X_CASCADE_EN__
    Gh3x2xDemoStopSampling(0xFFFFFFFF);   //stop all function
    GH3X2X_CascadeSetEcgEnFlag(0);
    if(0 == GH3X2X_GetSingleChipModeEnableFlag())
    {
        GH3X2X_CascadeOperationSlaverChip();
        GH3X2X_CascadeOperationMasterChip();
    }

    g_uchGh3x2xRegCfgArrIndex = 2;
    if (GH3X2X_RET_OK != GH3X2X_Init(g_stGh3x2xCfgListArr+0))
    {
        g_uchGh3x2xRegCfgArrIndex = DRV_LIB_REG_CFG_EMPTY;
    }
    GH3X2X_EnterLowPowerMode();
    if(0 == GH3X2X_GetSingleChipModeEnableFlag())
    {
        GH3X2X_CascadeOperationSlaverChip();
        GH3X2X_EnterLowPowerMode();
        GH3X2X_CascadeOperationMasterChip();
    }
#if __USER_DYNAMIC_DRV_BUF_EN__
    if(g_puchGh3x2xReadRawdataBufferSlaveChip)
    {
        Gh3x2xFreeUser(g_puchGh3x2xReadRawdataBufferSlaveChip);
        g_puchGh3x2xReadRawdataBufferSlaveChip = 0;
    }
#endif
#endif
}




/**
 * @fn     void Gh3x2xDemoStartSampling()
 *
 * @brief  Start sampling of GH3x2x
 *
 * @attention   None
 *
 * @param[in]   unFuncMode      function mode that will start
 *                              GH3X2X_FUNCTION_ADT/GH3X2X_FUNCTION_HR/GH3X2X_FUNCTION_SPO2/GH3X2X_FUNCTION_ECG and etc.
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xDemoStartSamplingInner(GU32 unFuncMode, GU8 uchSwitchEnable)
{
    GU32 unFuncModeTemp1 = GH3X2X_NO_FUNCTION;
    GU32 unFuncModeTemp2 = GH3X2X_NO_FUNCTION;
    GU32  unTargetFunMode;
    GU8  uchIndex = 0;
    GS8  chRet = GH3X2X_RET_OK;
#if __FUNC_TYPE_ECG_ENABLE__
    GU8 uchNeedSwitchToEcgCfg = 0;
#endif
#if __FUNC_TYPE_ECG_ENABLE__
    if(unFuncMode&GH3X2X_FUNCTION_LEAD_DET)
    {
        uchNeedSwitchToEcgCfg = 1;
    }
#endif

    EXAMPLE_LOG("[Gh3x2xDemoStartSampling] unFuncMode = 0x%X\r\n",(int)unFuncMode);

#if (__SUPPORT_HARD_ADT_CONFIG__)
    if (GH3X2X_FUNCTION_ADT == unFuncMode)
    {
        unFuncModeTemp2 = unFuncMode;
    }
    else
    {
        unFuncModeTemp2 = unFuncMode & (~GH3X2X_FUNCTION_ADT);
    }
#else
    unFuncModeTemp2 = unFuncMode;
#endif

    unTargetFunMode = unFuncMode;
#if __FUNC_TYPE_ECG_ENABLE__
    if(uchNeedSwitchToEcgCfg)
    {
        unFuncModeTemp2 |= GH3X2X_FUNCTION_ECG;  //add ecg function to demand function
        unTargetFunMode |= GH3X2X_FUNCTION_ECG;
    }
#endif

    //decode functions in current reg config array
    GH3X2X_DecodeRegCfgArr(&unFuncModeTemp1, (g_stGh3x2xCfgListArr + g_uchGh3x2xRegCfgArrIndex)->pstRegConfigArr, \
                            (g_stGh3x2xCfgListArr + g_uchGh3x2xRegCfgArrIndex)->usConfigArrLen);
    //if this function not exist in current reg cfg array,need switch reg cfg array
    if (
        ((unFuncModeTemp2 & unFuncModeTemp1) != unFuncModeTemp2)
        #if __GH3X2X_CASCADE_EN__
        ||(0 == Gh3x2xCheckCfgValidForCascade(g_uchGh3x2xRegCfgArrIndex))
        #endif
        )
    {
        if (uchSwitchEnable)
        {
            EXAMPLE_LOG("Cfg Error:Reg cfg file not exist!!!APP mode:0x%x\r\n",unFuncMode);
            EXAMPLE_LOG("Please check cfg switched manually is correct or not !!!\r\n");
        }
        else
        {
            chRet = GH3x2xDemoSearchCfgListByFunc(&uchIndex, unTargetFunMode, GH3X2X_FUNCTION_ADT);
            if (GH3X2X_RET_OK == chRet)
            {
                #if (__SUPPORT_HARD_ADT_CONFIG__)
                GU32 unDemoFuncModeBeforeReset = g_unDemoFuncMode;
                EMWearDetectType  uchWearDetectStatusBeforeReset = g_uchWearDetectStatus;
                EXAMPLE_LOG("unDemoFuncModeBeforeReset = %d\r\n",unDemoFuncModeBeforeReset);
                #endif
                Gh3x2xDemoSamplingControl(0xFFFFFFFF, UPROTOCOL_CMD_STOP);

            #if __SUPPORT_HARD_RESET_CONFIG__
                GH3X2X_HardReset();
            #else
                GH3X2X_SoftReset();
            #endif   

                Gh3x2x_BspDelayMs(15);
            #if !(__GH3X2X_CASCADE_EN__)
            #else
                GH3X2X_CascadeOperationSlaverChip();
                GH3X2X_CascadeOperationMasterChip();
            #endif
                g_uchGh3x2xRegCfgArrIndex = uchIndex;
                if (GH3X2X_RET_OK != GH3X2X_Init(g_stGh3x2xCfgListArr+uchIndex))
                {
                    g_uchGh3x2xRegCfgArrIndex = DRV_LIB_REG_CFG_EMPTY;
                }

                Gh3x2xFunctionCtrlModuleInit();

            #if (__SUPPORT_HARD_ADT_CONFIG__)
                //if ADT is running,recover ADT status
                EXAMPLE_LOG("temp unDemoFuncModeBeforeReset = %d\r\n",(unDemoFuncModeBeforeReset & GH3X2X_FUNCTION_ADT));
                if (GH3X2X_FUNCTION_ADT == (unDemoFuncModeBeforeReset & GH3X2X_FUNCTION_ADT))
                {
                    EXAMPLE_LOG("g_uchWearDetectStatus = %d\r\n",g_uchWearDetectStatus);
                    g_uchWearDetectStatus = uchWearDetectStatusBeforeReset;
                    unFuncMode |= GH3X2X_FUNCTION_ADT;
                    if(WEAR_DETECT_WEAR_OFF == g_uchWearDetectStatus)
                    {
                        GH3X2X_FunctionStart(g_pstGh3x2xFrameInfo[GH3X2X_FUNC_OFFSET_ADT]);
                    #if __FUNC_TYPE_SOFT_ADT_ENABLE__
                        g_uchHardAdtFuncStatus = 1;
                    #endif
                        g_uchWearDetectStatus = WEAR_DETECT_WEAR_OFF;
                        GH3X2X_WearDetectSwitchTo(WEAR_DETECT_WEAR_OFF,WEAR_DETECT_FORCE_SWITCH);
                    }
                }
            #endif

                GH3X2X_EnterLowPowerMode();
                #if __GH3X2X_CASCADE_EN__           //need enter low power mode after chip reset(master and slave chip may all be reset)
                GH3X2X_CascadeOperationSlaverChip();
                GH3X2X_EnterLowPowerMode();
                GH3X2X_CascadeOperationMasterChip();
                #endif
            }
            else
            {
                EXAMPLE_LOG("APP Mode Switch Error:Reg cfg file not exist!!!APP mode:0x%x\r\n",unFuncMode);
            }
        }
    }

    //function slot bit init
    Gh3x2xFunctionSlotBitInit();

#if (__SUPPORT_ENGINEERING_MODE__)
    if(1 == g_uchEngineeringModeStatus)
    {
        #if (__GH3X2X_CASCADE_EN__)
        if (GH3X2X_CascadeGetEcgEnFlag())
        {
            
            GH3X2X_CascadeOperationSlaverChip();
            GH3X2X_CascadeOperationMasterChip();
        }
        #endif
        GH3x2x_ChangeSampleParmForEngineeringMode(g_pstGh3x2xFrameInfo, unFuncMode, g_pstSampleParaGroup, g_uchEngineeringModeSampleParaGroupNum);
        GH3X2X_EnterLowPowerMode();
        #if (__GH3X2X_CASCADE_EN__)
        if (GH3X2X_CascadeGetEcgEnFlag())
        {
            
            GH3X2X_CascadeOperationSlaverChip();
            GH3X2X_EnterLowPowerMode();
            GH3X2X_CascadeOperationMasterChip();
        }
        #endif
    }
#endif
    Gh3x2xDemoSamplingControl(unFuncMode, UPROTOCOL_CMD_START);
}




void Gh3x2xDemoStartAlgoInner(GU32 unFuncMode)
{
#if (__DRIVER_LIB_MODE__ == __DRV_LIB_WITH_ALGO__)
    if(0 == GH3x2x_GetChipResetRecoveringFlag())
    {
        GH3X2X_AlgoInit(unFuncMode);
        #if __GH3X2X_MEM_POOL_CHECK_EN__
            Gh3x2xUpdataMemPollChkSumAfterAlgoCal();
        #endif
    }
#endif

#if __GH_MSG_WITH_ALGO_LAYER_EN__
    if(0 == GH3x2x_GetChipResetRecoveringFlag())
    {
        GH3X2X_SEND_MSG_ALGO_START(unFuncMode);
       
    }
#endif
}


/**
 * @fn     void Gh3x2xDemoStartSampling()
 *
 * @brief  Start sampling of GH3x2x
 *
 * @attention   None
 *
 * @param[in]   unFuncMode      function mode that will start
 *                              GH3X2X_FUNCTION_ADT/GH3X2X_FUNCTION_HR/GH3X2X_FUNCTION_SPO2/GH3X2X_FUNCTION_ECG and etc.
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xDemoStartSampling(GU32 unFuncMode)
{
    Gh3x2xDemoStartSamplingInner(unFuncMode, 0);
    Gh3x2xDemoStartAlgoInner(unFuncMode);
}

/**
 * @fn     void Gh3x2xDemoStartSamplingWithCfgSwitch(GU32 unFuncMode, GU8 uchArrayCfgIndex)
 *
 * @brief  Start sampling of GH3x2x
 *
 * @attention   None
 *
 * @param[in]   unFuncMode      function mode that will start
 *                              GH3X2X_FUNCTION_ADT/GH3X2X_FUNCTION_HR/GH3X2X_FUNCTION_SPO2/GH3X2X_FUNCTION_ECG and etc.
 * @param[in]   uchArrayCfgIndex      cfg id
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xDemoStartSamplingWithCfgSwitch(GU32 unFuncMode, GU8 uchArrayCfgIndex)
{
    if (uchArrayCfgIndex == DRV_LIB_REG_CFG_EMPTY)
    {
        EXAMPLE_LOG("[%s]:Array None!!!\r\n", __FUNCTION__);
    }
    Gh3x2xDemoArrayCfgSwitch(uchArrayCfgIndex);
    Gh3x2xDemoStartSamplingInner(unFuncMode, 1);
}





#if (__SUPPORT_ENGINEERING_MODE__)

/**
 * @fn     void Gh3x2xDemoStartSamplingForEngineeringMode()
 *
 * @brief  Start sampling of GH3x2x for engineering mode
 *
 * @attention   None
 *
 * @param[in]   unFuncMode      function mode that will start
 *                              GH3X2X_FUNCTION_ADT/GH3X2X_FUNCTION_HR/GH3X2X_FUNCTION_SPO2/GH3X2X_FUNCTION_ECG and etc.
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xDemoStartSamplingForEngineeringMode(GU32 unFuncMode, GU8 uchArrayCfgIndex, STGh3x2xEngineeringModeSampleParam *pstSampleParaGroup , GU8 uchSampleParaGroupNum)
{
    Gh3x2xDemoArrayCfgSwitch(uchArrayCfgIndex);
    g_pstSampleParaGroup = pstSampleParaGroup;
    g_uchEngineeringModeSampleParaGroupNum = uchSampleParaGroupNum;
    g_uchEngineeringModeStatus = 1;
    Gh3x2xDemoStartSamplingInner(unFuncMode, 1);
}
#else
void Gh3x2xDemoStartSamplingForEngineeringMode(GU32 unFuncMode, GU8 uchArrayCfgIndex, STGh3x2xEngineeringModeSampleParam *pstSampleParaGroup , GU8 uchSampleParaGroupNum){}
#endif



/**
 * @fn     void Gh3x2xDemoStopSampling(GU32 unFuncMode)
 *
 * @brief  Stop sampling of GH3x2x
 *
 * @attention   None
 *
 * @param[in]   unFuncMode      function mode that will start
 *                              GH3X2X_FUNCTION_ADT/GH3X2X_FUNCTION_HR/GH3X2X_FUNCTION_SPO2/GH3X2X_FUNCTION_ECG and etc.
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xDemoStopSamplingInner(GU32 unFuncMode)
{
    EXAMPLE_LOG("[Gh3x2xDemoStopSampling] unFuncMode = 0x%X\r\n",(int)unFuncMode);
    Gh3x2xDemoSamplingControl(unFuncMode, UPROTOCOL_CMD_STOP);
#if (__RESET_AGCINFO_BEFORE_SAMPLE__ && __SUPPORT_SOFT_AGC_CONFIG__)
    if ((g_unDemoFuncMode & (~GH3X2X_FUNCTION_ADT)) == 0)
    {
        Gh3x2xDemoHardConfigReset(g_stGh3x2xCfgListArr + g_uchGh3x2xRegCfgArrIndex);
    }
#endif
}


void Gh3x2xDemoStopAlgoInner(GU32 unFuncMode)
{
#if (__DRIVER_LIB_MODE__ == __DRV_LIB_WITH_ALGO__)
    if(0 == GH3x2x_GetChipResetRecoveringFlag())
    {
        GH3X2X_AlgoDeinit(unFuncMode);
    }
#endif

#if __GH_MSG_WITH_ALGO_LAYER_EN__
    if(0 == GH3x2x_GetChipResetRecoveringFlag())
    {
        GH3X2X_SEND_MSG_ALGO_STOP(unFuncMode);
    }
#endif
}




void Gh3x2xDemoStopSampling(GU32 unFuncMode)
{
    Gh3x2xDemoStopSamplingInner(unFuncMode);
    Gh3x2xDemoStopAlgoInner(unFuncMode);
}


#if 1
void GH3X2X_RedetectWearOn(void)
{
    if (g_uchWearDetectStatus == WEAR_DETECT_WEAR_OFF)   
    {
        GU16 usGotEvent = 0;
				EXAMPLE_LOG("[%s]switch to dectecting wear on\r\n", __FUNCTION__);
        usGotEvent |= GH3X2X_IRQ_MSK_WEAR_OFF_BIT;
        Gh3x2xDemoWearEventProcess(&usGotEvent,WEAR_DETECT_FORCE_SWITCH);

        #if __HARD_ADT_ALGO_CHECK_EN__
        EXAMPLE_LOG("[%s]reset adt algo.\r\n", __FUNCTION__);
        Gh3x2xDemoStopAlgoInner(GH3X2X_FUNCTION_ADT);
        Gh3x2xDemoStartAlgoInner(GH3X2X_FUNCTION_ADT);
        #endif
        
        
    }
                    
}
#endif



#if (__SUPPORT_ENGINEERING_MODE__)

/**
 * @fn     void Gh3x2xDemoStopSamplingForEngineeringMode(void)
 *
 * @brief  Stop sampling of GH3x2x for engineering mode
 *
 * @attention   None
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xDemoStopSamplingForEngineeringMode(void)
{
    Gh3x2xDemoStopSamplingInner(g_unDemoFuncMode);

    Gh3x2xDemoSamplingControl(0xFFFFFFFF, UPROTOCOL_CMD_STOP);
    
    #if __SUPPORT_HARD_RESET_CONFIG__
    GH3X2X_HardReset();
    #else
    GH3X2X_SoftReset();
    #endif   

    Gh3x2x_BspDelayMs(15);
    #if __GH3X2X_CASCADE_EN__
    GH3X2X_CascadeOperationSlaverChip();
    GH3X2X_CascadeOperationMasterChip();
    #endif
    g_uchGh3x2xRegCfgArrIndex = 0;
    if (GH3X2X_RET_OK != GH3X2X_Init(&g_stGh3x2xCfgListArr[0]))
    {
        g_uchGh3x2xRegCfgArrIndex = DRV_LIB_REG_CFG_EMPTY;
    }
    
    GH3X2X_EnterLowPowerMode();
    #if __GH3X2X_CASCADE_EN__
    GH3X2X_CascadeOperationSlaverChip();
    GH3X2X_EnterLowPowerMode();
    GH3X2X_CascadeOperationMasterChip();
    #endif

    Gh3x2xFunctionCtrlModuleInit();

    g_uchEngineeringModeStatus = 0;
}
#endif


/**
 * @fn     void Gh3x2xDemoMoveDetectTimerHandler(void)
 *
 * @brief  Move detect timer handler callback
 *
 * @attention   if use adt with confirm mode,must call this function to detect movement
 *
 * @param[in]   None
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xDemoMoveDetectTimerHandler(void)
{
#if (0 == __GS_TIMESTAMP_READ_EN__)
    #if (__FUNC_TYPE_SOFT_ADT_ENABLE__)
    //check CAP
    #if (__CAP_ENABLE__)
    hal_cap_drv_get_fifo_data(cap_soft_fifo_buffer, &cap_soft_fifo_buffer_index);
    #endif
    GH3X2X_MoveDetectByCapData(cap_soft_fifo_buffer, cap_soft_fifo_buffer_index);
    //check GS
    hal_gsensor_drv_get_fifo_data(gsensor_soft_fifo_buffer + __GS_EXTRA_BUF_LEN__, &gsensor_soft_fifo_buffer_index);
    Gh3x2x_NormalizeGsensorSensitivity(gsensor_soft_fifo_buffer + __GS_EXTRA_BUF_LEN__,gsensor_soft_fifo_buffer_index);
    GH3X2X_MoveDetectByGsData((GS16*)(gsensor_soft_fifo_buffer + __GS_EXTRA_BUF_LEN__), gsensor_soft_fifo_buffer_index);
    if (GH3X2X_GetSoftWearOffDetEn())
    {
        //EXAMPLE_LOG("move detecting...\r\n");
    }
#endif
#endif
}

/**
 * @fn     void Gh3x2xDemoSetFuncionFrequency(GU8 uchFunctionID, GU16 usFrequencyValue)
 *
 * @brief  Set function frequency
 *
 * @attention   None
 *
 * @param[in]   uchFunctionID
 * @param[in]   usFrequencyValue
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xDemoSetFuncionFrequency(GU8 uchFunctionID, GU16 usFrequencyValue)
{
    GU32 unFuncModeTemp = 0;
    
    GH3X2X_DecodeRegCfgArr(&unFuncModeTemp, (g_stGh3x2xCfgListArr + g_uchGh3x2xRegCfgArrIndex)->pstRegConfigArr, \
                            (g_stGh3x2xCfgListArr + g_uchGh3x2xRegCfgArrIndex)->usConfigArrLen);
    
    if (((1 << uchFunctionID) & unFuncModeTemp) == 0)
    {
        EXAMPLE_LOG("[%s]:Current cfg cannot support !!! cfgfunc = 0x%x, selectfuncs = 0x%x\r\n", __FUNCTION__, (int)unFuncModeTemp, (1 << uchFunctionID));
        return;
    }

    if (usFrequencyValue % 25 || usFrequencyValue > 1000)
    {
        EXAMPLE_LOG("[%s]:The SR is not a multiple of 25!!!srvalue = %d\r\n", __FUNCTION__, usFrequencyValue);
        return;
    }
    
    GU32 unDemoFuncModeBeforeChipReset = g_unDemoFuncMode;
    
    if (unDemoFuncModeBeforeChipReset != 0)
    {
        Gh3x2xDemoStopSampling(unDemoFuncModeBeforeChipReset);
        EXAMPLE_LOG("[%s]:Stop.\r\n", __FUNCTION__);
        Gh3x2x_BspDelayMs(30);
    }
    
    GH3X2X_ModifyFunctionFrequency(uchFunctionID, usFrequencyValue);
    EXAMPLE_LOG("[%s]:Change fs !!! func = 0x%x, fs = %d\r\n", __FUNCTION__, (1 << uchFunctionID), usFrequencyValue);
    
    if (unDemoFuncModeBeforeChipReset != 0)
    {
        Gh3x2x_BspDelayMs(30);
        Gh3x2xDemoStartSampling(unDemoFuncModeBeforeChipReset);
        EXAMPLE_LOG("[%s]:Restart.\r\n", __FUNCTION__);
    }
}

/**
 * @fn     void Gh3x2xDemoSetFuncionLedCurrent(GU8 uchFunctionID, GU16 usLedDrv0Current, GU16 usLedDrv1Current)
 *
 * @brief  Set function led current
 *
 * @attention   None
 *
 * @param[in]   uchFunctionID               function offset
 * @param[in]   usLedDrv0Current         0mA-200mA
 * @param[in]   usLedDrv1Current         0mA-200mA
 * @param[out]  None
 *
 * @return  None
 */
void Gh3x2xDemoSetFuncionLedCurrent(GU8 uchFunctionID, GU16 usLedDrv0Current, GU16 usLedDrv1Current)
{
    GU32 unFuncModeTemp = 0;
    
    GH3X2X_DecodeRegCfgArr(&unFuncModeTemp, (g_stGh3x2xCfgListArr + g_uchGh3x2xRegCfgArrIndex)->pstRegConfigArr, \
                            (g_stGh3x2xCfgListArr + g_uchGh3x2xRegCfgArrIndex)->usConfigArrLen);

    if (((1 << uchFunctionID) & unFuncModeTemp) == 0)
    {
        EXAMPLE_LOG("[%s]:Current cfg cannot support !!! cfgfunc = 0x%x, selectfuncs = 0x%x\r\n", __FUNCTION__, (int)unFuncModeTemp, (1 << uchFunctionID));
        return;
    }

    GH3X2X_ModifyFunctionLedCurrent(uchFunctionID, usLedDrv0Current, usLedDrv1Current);
    EXAMPLE_LOG("[%s]:Change led current !!! func = 0x%x, drv0 current = %d, drv1 current = %d\r\n",  __FUNCTION__, (1 << uchFunctionID), (int)usLedDrv0Current, (int)usLedDrv1Current);
}


#if (__SUPPORT_ELECTRODE_WEAR_STATUS_DUMP__)
void GH3X2X_ReadElectrodeWearDumpData(void)
{
    g_uchGh3x2xElectrodeWearStatus = GH3X2X_ReadRegBitField(0x050C, 1, 1);
}
#endif

void Gh3x2xSetFrameFlag2(const STGh3x2xFrameInfo * const pstFrameInfo)
{
    if(0 == (*(pstFrameInfo->punFrameCnt)))
    {
        pstFrameInfo->punFrameFlag[2] |= 0x02;
    }
    #if (__SUPPORT_ELECTRODE_WEAR_STATUS_DUMP__)
    if(g_uchGh3x2xElectrodeWearStatus)
    {
        pstFrameInfo->punFrameFlag[2] |= 0x04;
    }
    #endif
}


#if 0 == __FUNC_TYPE_ECG_ENABLE__
void GH3x2xSetEcgOutputFs(GU16 usEcgOutputFs){}
void GH3X2X_ECGResampleConfig(GU16 usEcgSampleRate){}
void GH3X2X_EcgSampleHookHandle(GU8 uchEventType, GU8 uchEventInfo){}
void GH3X2X_LeadDetEnInHardAdt(GU8 uchEn){}
void GH3X2X_LeadDetEnControl(GU8 uchEventInfo){}
void GH3X2X_SlaverSoftLeadPramSet(GU16 usRegVal, GU8 uchRegPosi){}
void GH3X2X_SlaverSoftLeadPramInit(void){}
GU8 GH3X2X_LeadOffDetect(GU8* puchFifoBuffer, GU16 usFifoBufferLen)
{
    return 0;
}
GS32 GH3X2X_LeadOffDetect2Init(void)
{
    return 0;
}
void GH3X2X_ConfigLeadOffThr(GU32 uIQAmpThr, GU32 uIQAmpDiffThr, GU32 uValThr) {}
GU8 GH3X2X_LeadOffDetect2(GU8 *puchFifoBuffer, GU16 *pusFifoBufferLen) { return GH3X2X_LeadOffDetect(puchFifoBuffer, *pusFifoBufferLen); }
#else
    #if (__SUPPORT_ECG_LEAD_OFF_DET_800HZ__ == 0)
GS32 GH3X2X_LeadOffDetect2Init(void)
{
    GH3X2X_LeadOffDetectInit();
    return 0;
}
    void GH3X2X_ConfigLeadOffThr(GU32 uIQAmpThr, GU32 uIQAmpDiffThr,GU32 uValThr){}
    GU8 GH3X2X_LeadOffDetect2(GU8* puchFifoBuffer, GU16* pusFifoBufferLen){ return GH3X2X_LeadOffDetect(puchFifoBuffer, *pusFifoBufferLen);}
    #endif
#endif

    
    
#if (0 == __SUPPORT_SOFT_AGC_CONFIG__)
void GH3X2X_LedAgcPramWrite(GU16 usRegAddr, GU16 usRegData)
{
}
void GH3X2X_LedAgcReset(void){}
void GH3X2X_LedAgcInit(void){}
void GH3X2X_LedAgcProcess(GU8* puchReadFifoBuffer, GU16 usFifoLen){}
GU8 GH3X2X_GetLedAgcState(void)
{
    return   0;
}
void GH3X2X_SoftLedADJAutoADJInt(void){}
#endif


void Gh3x2xPollingModePro(void)
{
#if (!(__NORMAL_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__ || __MIX_INT_PROCESS_MODE__ == __INTERRUPT_PROCESS_MODE__))
    GH3X2X_WriteReg(0x0502,0); //disable INT
#endif
}


#if 0 == GH3X2X_NEW_AGC_SUB_CHNL_NUM_LIMIT 
void Gh3x2xNewAgcSubChnlModuleReset(void){}
GU8 GH3x2xNewAgcSetNewSubChnl(GU8 uchSlotNo, GU8 uchRxEn){return 0;}
void GH3X2X_NewAgcSubChnlGainAdj(GU8* puchReadFifoBuffer, GU16 usFifoLen){};
void GH3x2xNewAgcSubChnlClearAnalysisCntAndSetGain(GU8 uchSlotNo, GU8 uchRxEn ,GU8 uchNextGain){};
void GH3x2xNewAgcSetSubChnlSlotInfo(GU8 uchSlotCnt, GU16 usSampleRate){};
#endif

void GH3x2xSlotTimeInfo(void)
{
#if 1 == __SLOT_SEQUENCE_DYNAMIC_ADJUST_EN__
    for(GU8 uchSlotCnt = 0; uchSlotCnt < 8; uchSlotCnt ++)
    {
        GU8 uchCfgIndex;
        //get slot time infomation
        if(0 == (uchSlotCnt&0x01)) //0,2,4,6
        {
            uchCfgIndex = GH3X2X_ReadRegBitField(0x0100 + (uchSlotCnt&0xFE), 0,3);
        }
        else  //1,3,5,7
        {
            uchCfgIndex = GH3X2X_ReadRegBitField(0x0100 + (uchSlotCnt&0xFE), 8,11);
        }
        if(uchCfgIndex < 8)
        {
            g_usGh3x2xSlotTime[uchCfgIndex] = GH3X2X_ReadReg(0x01EC + uchCfgIndex *2);
            EXAMPLE_LOG("[%s]:CfgIndex[%d]:Slot time = %d\r\n", __FUNCTION__, uchCfgIndex, g_usGh3x2xSlotTime[uchCfgIndex]);
        }
    }
#endif
}

#if (__GH3X2X_CASCADE_EN__)
GS8 Gh3x2xEcgCascadeCommunicationTest(void)
{
    GS8  chRet = GH3X2X_RET_OK;
    GH3X2X_CascadeOperationMasterChip();
    GU16 usChipReadyCode = GH3X2X_ReadReg(0x36);
    if (0xaa55 == usChipReadyCode)
    {
        EXAMPLE_LOG("Master Communication success!!! 0x36:0x%04x\r\n",usChipReadyCode);
    }
    else
    {
        EXAMPLE_LOG("Master Communication FAIL!!! 0x36:0x%04x\r\n",usChipReadyCode);
        chRet = GH3X2X_RET_COMM_ERROR;
    }
    
    
    GH3X2X_CascadeOperationSlaverChip();
    usChipReadyCode = GH3X2X_ReadReg(0x36);
    if (0xaa55 == usChipReadyCode)
    {
        EXAMPLE_LOG("Slaver Communication success!!! 0x36:0x%04x\r\n",usChipReadyCode);
    }
    else
    {
        EXAMPLE_LOG("Slaver Communication FAIL!!! 0x36:0x%04x\r\n",usChipReadyCode);
        chRet = GH3X2X_RET_COMM_ERROR;
    }
    GH3X2X_CascadeOperationMasterChip();
    
    return chRet;

}

#endif








#if 0 == __GH3X2X_CASCADE_EN__
GU8 GH3X2X_CascadeGetEcgEnFlag(void){return 0;}
void GH3X2X_LoadSlaveCfgForCascadeMode(void)
{}
void GH3X2X_CascadeEcgSlaverLeadDectDisEx(void)
{}
void GH3X2X_CascadeEcgSlaverFastRecTrigEx(void)
{}
void GH3X2X_SoftResetSlaveChip(void)
{}
void GH3X2X_StartSamplingSlaveChip(void)
{}
void GH3X2X_StopSamplingSlaveChip(void)
{}
#endif

#if (0 == __CURRENT_ECODE_CALIBRATE__)
GU16 GH3X2X_SetLedDrvCurrentByEcode(GU8 uchCurrent_mA, GU8 uchDrvNum, GS8 chECODE)
{
    GU8 uchDrFs[2] = {0};
    GH3x2x_GetLedCurrentFullScal(&uchDrFs[0], &uchDrFs[1]);
    GU16 usDrvReg = ((GU16)uchCurrent_mA) * 255 /uchDrFs[uchDrvNum];
    return usDrvReg;
}
void GH3X2X_SetConfigDrvEcode(const STGh3x2xInitConfig *pstGh3x2xInitConfigParam){}
#endif

//function __weak api
#if (0 == __FIFO_PACKAGE_SEND_ENABLE__)
GU8 GH3X2X_GetFifoPackageMode(void)
{
    return 0;
}
void GH3X2X_SendRawdataFifoPackage(GU8 *puchGh3x2xReadFifoData, GU16 usFifoReadByteNum){}
#endif
#if (0 == __CAP_ENABLE__)
GU8 GH3X2X_GetCapEnableFlag(void)
{
    return 0;
}
#endif
#if (0 == __TEMP_ENABLE__)
GU8 GH3X2X_GetTempEnableFlag(void)
{
    return 0;
}
#endif


/********END OF FILE********* Copyright (c) 2003 - 2022, Goodix Co., Ltd. ********/
