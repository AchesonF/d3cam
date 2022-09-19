#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MVS_WAIT_TIMEO		(6)		// x
#define MVS_WAIT_PERIOD		(2)		// 2s

char *strMsg (int errcode)
{
	char *str_err = "";

	switch (errcode) {
	// 正确码定义
	case MV_OK:					///< 0x00000000  ///< 成功，无错误
		str_err = toSTR(MV_OK)" : Successed, no error";
		break;

	// 通用错误码定义:范围0x80000000-0x800000FF
	case MV_E_HANDLE:			///< 0x80000000  ///< 错误或无效的句柄
		str_err = toSTR(MV_E_HANDLE)" : Error or invalid handle";
		break;
	case MV_E_SUPPORT:			///< 0x80000001  ///< 不支持的功能
		str_err = toSTR(MV_E_SUPPORT)" : Not supported function";
		break;
	case MV_E_BUFOVER:			///< 0x80000002  ///< 缓存已满    
		str_err = toSTR(MV_E_BUFOVER)" : Buffer overflow";
		break;
	case MV_E_CALLORDER:		///< 0x80000003  ///< 函数调用顺序错误
		str_err = toSTR(MV_E_CALLORDER)" : Function calling order error";
		break;
	case MV_E_PARAMETER:		///< 0x80000004  ///< 错误的参数  
		str_err = toSTR(MV_E_PARAMETER)" : Incorrect parameter";
		break;
	case MV_E_RESOURCE:			///< 0x80000006  ///< 资源申请失败
		str_err = toSTR(MV_E_RESOURCE)" : Applying resource failed";
		break;
	case MV_E_NODATA:			///< 0x80000007  ///< 无数据      
		str_err = toSTR(MV_E_NODATA)" : No data";
		break;
	case MV_E_PRECONDITION:		///< 0x80000008  ///< 前置条件有误，或运行环境已发生变化
		str_err = toSTR(MV_E_PRECONDITION)" : Precondition error, or running environment changed";
		break;
	case MV_E_VERSION:			///< 0x80000009  ///< 版本不匹配  
		str_err = toSTR(MV_E_VERSION)" : Version mismatches";
		break;
	case MV_E_NOENOUGH_BUF:		///< 0x8000000A  ///< 传入的内存空间不足
		str_err = toSTR(MV_E_NOENOUGH_BUF)" : Insufficient memory";
		break;
	case MV_E_ABNORMAL_IMAGE:	///< 0x8000000B  ///< 异常图像，可能是丢包导致图像不完整
		str_err = toSTR(MV_E_ABNORMAL_IMAGE)" : Abnormal image, maybe incomplete image because of lost packet";
		break;
	case MV_E_LOAD_LIBRARY:		///< 0x8000000C  ///< 动态导入DLL失败
		str_err = toSTR(MV_E_LOAD_LIBRARY)" : Load library failed";
		break;
	case MV_E_NOOUTBUF:			///< 0x8000000D  ///< 没有可输出的缓存
		str_err = toSTR(MV_E_NOOUTBUF)" : No Avaliable Buffer";
		break;
	case MV_E_UNKNOW:			///< 0x800000FF  ///< 未知的错误  
		str_err = toSTR(MV_E_UNKNOW)" : Unknown error";
		break;

	// GenICam系列错误:范围0x80000100-0x800001FF
	case MV_E_GC_GENERIC:		///< 0x80000100  ///< 通用错误    
		str_err = toSTR(MV_E_GC_GENERIC)" : General error";
		break;
	case MV_E_GC_ARGUMENT:		///< 0x80000101  ///< 参数非法    
		str_err = toSTR(MV_E_GC_ARGUMENT)" : Illegal parameters";
		break;
	case MV_E_GC_RANGE:			///< 0x80000102  ///< 值超出范围  
		str_err = toSTR(MV_E_GC_RANGE)" : The value is out of range";
		break;
	case MV_E_GC_PROPERTY:		///< 0x80000103  ///< 属性        
		str_err = toSTR(MV_E_GC_PROPERTY)" : Property";
		break;
	case MV_E_GC_RUNTIME:		///< 0x80000104  ///< 运行环境有问题
		str_err = toSTR(MV_E_GC_RUNTIME)" : Running environment error";
		break;
	case MV_E_GC_LOGICAL:		///< 0x80000105  ///< 逻辑错误    
		str_err = toSTR(MV_E_GC_LOGICAL)" : Logical error";
		break;
	case MV_E_GC_ACCESS:		///< 0x80000106  ///< 节点访问条件有误
		str_err = toSTR(MV_E_GC_ACCESS)" : Node accessing condition error";
		break;
	case MV_E_GC_TIMEOUT:		///< 0x80000107  ///< 超时        
		str_err = toSTR(MV_E_GC_TIMEOUT)" : Timeout";
		break;
	case MV_E_GC_DYNAMICCAST:	///< 0x80000108  ///< 转换异常    
		str_err = toSTR(MV_E_GC_DYNAMICCAST)" : Transformation exception";
		break;
	case MV_E_GC_UNKNOW:		///< 0x800001FF  ///< GenICam未知错误
		str_err = toSTR(MV_E_GC_UNKNOW)" : GenICam unknown error";
		break;

	// GigE_STATUS对应的错误码:范围0x80000200-0x800002FF
	case MV_E_NOT_IMPLEMENTED:	///< 0x80000200  ///< 命令不被设备支持
		str_err = toSTR(MV_E_NOT_IMPLEMENTED)" : The command is not supported by device";
		break;
	case MV_E_INVALID_ADDRESS:	///< 0x80000201  ///< 访问的目标地址不存在
		str_err = toSTR(MV_E_INVALID_ADDRESS)" : The target address being accessed does not exist";
		break;
	case MV_E_WRITE_PROTECT:	///< 0x80000202  ///< 目标地址不可写
		str_err = toSTR(MV_E_WRITE_PROTECT)" : The target address is not writable";
		break;
	case MV_E_ACCESS_DENIED:	///< 0x80000203  ///< 设备无访问权限
		str_err = toSTR(MV_E_ACCESS_DENIED)" : No permission";
		break;
	case MV_E_BUSY:				///< 0x80000204  ///< 设备忙，或网络断开
		str_err = toSTR(MV_E_BUSY)" : Device is busy, or network disconnected";
		break;
	case MV_E_PACKET:			///< 0x80000205  ///< 网络包数据错误
		str_err = toSTR(MV_E_PACKET)" : Network data packet error";
		break;
	case MV_E_NETER:			///< 0x80000206  ///< 网络相关错误
		str_err = toSTR(MV_E_NETER)" : Network error";
		break;
	case MV_E_IP_CONFLICT:		///< 0x80000221  ///< 设备IP冲突  
		str_err = toSTR(MV_E_IP_CONFLICT)" : Device IP conflict";
		break;

	// USB_STATUS对应的错误码:范围0x80000300-0x800003FF
	case MV_E_USB_READ:			///< 0x80000300      ///< 读usb出错
		str_err = toSTR(MV_E_USB_READ)" : Reading USB error";
		break;
	case MV_E_USB_WRITE:		///< 0x80000301      ///< 写usb出错
		str_err = toSTR(MV_E_USB_WRITE)" : Writing USB error";
		break;
	case MV_E_USB_DEVICE:		///< 0x80000302      ///< 设备异常
		str_err = toSTR(MV_E_USB_DEVICE)" : Device exception";
		break;
	case MV_E_USB_GENICAM:		///< 0x80000303      ///< GenICam相关错误 
		str_err = toSTR(MV_E_USB_GENICAM)" : GenICam error";
		break;
	case MV_E_USB_BANDWIDTH:	///< 0x80000304      ///< 带宽不足  该错误码新增
		str_err = toSTR(MV_E_USB_BANDWIDTH)" : Insufficient bandwidth, this error code is newly added";
		break;
	case MV_E_USB_DRIVER:		///< 0x80000305      ///< 驱动不匹配或者未装驱动
		str_err = toSTR(MV_E_USB_DRIVER)" : Driver mismatch or unmounted drive";
		break;
	case MV_E_USB_UNKNOW:		///< 0x800003FF      ///< USB未知的错误
		str_err = toSTR(MV_E_USB_UNKNOW)" : USB unknown error";
		break;

	// 升级时对应的错误码:范围0x80000400-0x800004FF
	case MV_E_UPG_FILE_MISMATCH:		///< 0x80000400 ///< 升级固件不匹配
		str_err = toSTR(MV_E_UPG_FILE_MISMATCH)" : Firmware mismatches";
		break;
	case MV_E_UPG_LANGUSGE_MISMATCH:	///< 0x80000401 ///< 升级固件语言不匹配
		str_err = toSTR(MV_E_UPG_LANGUSGE_MISMATCH)" : Firmware language mismatches";
		break;
	case MV_E_UPG_CONFLICT:		///< 0x80000402 ///< 升级冲突（设备已经在升级了再次请求升级即返回此错误）
		str_err = toSTR(MV_E_UPG_CONFLICT)" : Upgrading conflicted (repeated upgrading requests during device upgrade)";
		break;
	case MV_E_UPG_INNER_ERR:	///< 0x80000403 ///< 升级时相机内部出现错误
		str_err = toSTR(MV_E_UPG_INNER_ERR)" : Camera internal error during upgrade";
		break;
	case MV_E_UPG_UNKNOW:		///< 0x800004FF ///< 升级时未知错误
		str_err = toSTR(MV_E_UPG_UNKNOW)" : Unknown error during upgrade";
		break;
	default:
		str_err = " ";
		break;
	}

	return str_err;
}

static int csv_mvs_camera_deinit (struct cam_hk_spec_t *pCAM)
{
	int nRet = MV_OK;

	if ((NULL == pCAM)||(NULL == pCAM->pHandle) || (!pCAM->opened)) {
		return -1;
	}

	log_info("CAM '%s' detach handle.", pCAM->sn);

	if (pCAM->grabbing) {
		nRet = MV_CC_StopGrabbing(pCAM->pHandle);
		if (MV_OK != nRet) {
			log_warn("ERROR : CAM '%s' StopGrabbing errcode[0x%08X]:'%s'.", 
				pCAM->sn, nRet, strMsg(nRet));
		}
	}

	nRet = MV_CC_CloseDevice(pCAM->pHandle);
	if (MV_OK != nRet) {
		log_warn("ERROR : CAM '%s' CloseDevice errcode[0x%08X]:'%s'.", 
			pCAM->sn, nRet, strMsg(nRet));
	}

	nRet = MV_CC_DestroyHandle(pCAM->pHandle);
	if (MV_OK != nRet) {
		log_warn("ERROR : CAM '%s' DestroyHandle errcode[0x%08X]:'%s'.", 
			pCAM->sn, nRet, strMsg(nRet));
	}

	pCAM->pHandle = NULL;
	pCAM->opened = false;
	pCAM->grabbing = false;
	pCAM->sizePayload.nCurValue = 0;

	if (NULL != pCAM->imgData) {
		free(pCAM->imgData);
		pCAM->imgData = NULL;
	}

	return 0;
}

static int csv_mvs_cameras_init (struct csv_mvs_t *pMVS)
{
	int nRet = MV_OK, i = 0;
	MV_CC_DEVICE_INFO_LIST *pDevList = &pMVS->stDeviceList;
	MV_CC_DEVICE_INFO *pDevInfo = NULL;
	struct cam_hk_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pDevInfo = pDevList->pDeviceInfo[i];
		if (NULL == pDevInfo) {
			log_warn("ERROR : CAM[%d] critical null.", i);
			break;
		}

		pCAM = &pMVS->Cam[i];
		log_info("Initialling CAM '%s' : '%s'.", pCAM->model, pCAM->sn);

		pCAM->grabbing = false;

		nRet = MV_CC_CreateHandle(&pCAM->pHandle, pDevInfo);
		if (MV_OK != nRet) {
			pCAM->pHandle = NULL;
			log_warn("ERROR : CreateHandle errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
			continue;
		}

		pCAM->opened = true;

		nRet = MV_CC_OpenDevice(pCAM->pHandle, MV_ACCESS_Exclusive, 0);
		if (MV_OK != nRet) {
			log_warn("ERROR : OpenDevice errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
			nRet = MV_CC_DestroyHandle(pCAM->pHandle);
			if (MV_OK != nRet) {
				log_warn("ERROR : DestroyHandle errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
			}

			pCAM->opened = false;

			continue;
		}

        // 设置最佳包长（针对网口）
        if (pDevInfo->nTLayerType == MV_GIGE_DEVICE) {
            int nPacketSize = MV_CC_GetOptimalPacketSize(pCAM->pHandle);
            if (nPacketSize > 0) {
                nRet = MV_CC_SetIntValue(pCAM->pHandle, "GevSCPSPacketSize", nPacketSize);
            }
        }

		// 设置抓取图片的像素格式
		uint32_t enValue = PixelType_Gvsp_Mono8;		// 黑白
		if (strstr(pCAM->model, "UC") != NULL) {	// 彩色
			enValue = PixelType_Gvsp_RGB8_Packed;
		}

		nRet = MV_CC_SetPixelFormat(pCAM->pHandle, enValue);
		if (MV_OK != nRet) {
			if (enValue == PixelType_Gvsp_RGB8_Packed) {
				log_warn("ERROR : SetPixelFormat 'RGB8_Packed'. errcode[0x%08X]:'%s'.", 
					nRet, strMsg(nRet));
			} else if (enValue == PixelType_Gvsp_Mono8) {
				log_warn("ERROR : SetPixelFormat 'Mono8'. errcode[0x%08X]:'%s'.", 
					nRet, strMsg(nRet));
			}
		}

		nRet = MV_CC_SetBoolValue(pCAM->pHandle, "AcquisitionFrameRateEnable", false);
		if (MV_OK != nRet) {
			log_warn("ERROR : SetBoolValue 'AcquisitionFrameRateEnable' errcode[0x%08X]:'%s'.", 
				nRet, strMsg(nRet));
		}

		// 设置触发模式为 ON
		nRet = MV_CC_SetEnumValue(pCAM->pHandle, "TriggerMode", MV_TRIGGER_MODE_ON);
		if (MV_OK != nRet) {
			log_warn("ERROR : SetEnumValue 'TriggerMode' errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
		}

		// 设置触发源 line0
		nRet = MV_CC_SetEnumValue(pCAM->pHandle, "TriggerSource", MV_TRIGGER_SOURCE_LINE0);
		if (MV_OK != nRet) {
			log_warn("ERROR : SetEnumValue 'TriggerSource' errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
		}

		nRet = MV_CC_GetFloatValue(pCAM->pHandle, "ExposureTime", &pCAM->expoTime);
		if (MV_OK == nRet) {
			log_info("OK ：GetFloatValue 'ExposureTime' : %f [%f, %f].", pCAM->expoTime.fCurValue, 
				pCAM->expoTime.fMin, pCAM->expoTime.fMax);
		}

		nRet = MV_CC_GetFloatValue(pCAM->pHandle, "Gain", &pCAM->gain);
		if (MV_OK == nRet) {
			log_info("OK : GetFloatValue 'Gain' : %f [%f, %f].", pCAM->gain.fCurValue, 
				pCAM->gain.fMin, pCAM->gain.fMax);
		}

		MVCC_INTVALUE struIntWidth = {0}, struIntHeight = {0};
		MV_CC_GetIntValue(pCAM->pHandle, "Width", &struIntWidth);

		// 宽16倍对齐，偏移设置
		int overplus = struIntWidth.nCurValue%16;
		if (overplus > 0) {	// 非 16 字节对齐
		    MV_CC_SetIntValue(pCAM->pHandle, "OffsetX", overplus/2);
		    MV_CC_SetIntValue(pCAM->pHandle, "Width", struIntWidth.nCurValue-overplus);
		}

		// 需先确认相机参数一致 W*H
		nRet = MV_CC_GetIntValue(pCAM->pHandle, "PayloadSize", &pCAM->sizePayload);
		if (MV_OK != nRet) {
			log_warn("ERROR : GetIntValue 'PayloadSize' errcode[0x%08X]:'%s'.", 
				nRet, strMsg(nRet));
		}

		MV_CC_GetIntValue(pCAM->pHandle, "Width", &struIntWidth);
		MV_CC_GetIntValue(pCAM->pHandle, "Height", &struIntHeight);
		log_info("OK : GetIntValue 'Width x Height' = [%d x %d].", 
			struIntWidth.nCurValue, struIntHeight.nCurValue);

		pCAM->imgData = (uint8_t *)malloc(pCAM->sizePayload.nCurValue+1);
		if (pCAM->imgData == NULL){
			log_err("ERROR : malloc imgData.");
			csv_mvs_camera_deinit(pCAM);
			continue;
		}
		memset(pCAM->imgData, 0x00, pCAM->sizePayload.nCurValue+1);

    }

	if (pMVS->Cam[CAM_LEFT].sizePayload.nCurValue != pMVS->Cam[CAM_RIGHT].sizePayload.nCurValue) {
		log_warn("ERROR : CAMs not coupled. PayloadSize : %d vs %d.", 
			pMVS->Cam[CAM_LEFT].sizePayload.nCurValue,
			pMVS->Cam[CAM_RIGHT].sizePayload.nCurValue);
		return -1;
	}

	return 0;
}

static int csv_mvs_cameras_search (struct csv_mvs_t *pMVS)
{
	int nRet = 0, i = 0;
	MV_CC_DEVICE_INFO_LIST *pDevList = &pMVS->stDeviceList;
	MV_CC_DEVICE_INFO *pDevInfo = NULL;
	struct cam_hk_spec_t *pCAM = NULL;

	pMVS->cnt_mvs = 0;
	memset(pDevList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

	// enum device
	nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, pDevList);
	//nRet = MV_CC_EnumDevices(MV_USB_DEVICE, pDevList);
	if (MV_OK != nRet) {
		log_warn("ERROR : EnumDevices errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
		return -1;
	}

	if (pDevList->nDeviceNum > 0) {
		log_info("OK : Found %d CAM device(s).", pDevList->nDeviceNum);
	} else {
		log_warn("WARN : NO CAM device found.");
		return 0;
	}

	pMVS->cnt_mvs = pDevList->nDeviceNum;

	if (pDevList->nDeviceNum > TOTAL_CAMS) {
		pMVS->cnt_mvs = TOTAL_CAMS;
		log_notice("Only USE first %d CAM device(s).", TOTAL_CAMS);
	}

	if ((2 == pMVS->cnt_mvs)&&(gCSV->cfg.devicecfg.SwitchCams)) {
		MV_CC_DEVICE_INFO *pTemp = pDevList->pDeviceInfo[0];
		pDevList->pDeviceInfo[0] = pDevList->pDeviceInfo[1];
		pDevList->pDeviceInfo[1] = pTemp;
	}

	for (i = 0; i < pMVS->cnt_mvs; i++) {
		pDevInfo = pDevList->pDeviceInfo[i];
		if (NULL == pDevInfo) {
			log_warn("ERROR : CAM pDeviceInfo.");
			continue;
		}

		pCAM = &pMVS->Cam[i];

		switch (pDevInfo->nTLayerType) {
		case MV_GIGE_DEVICE: {
			MV_GIGE_DEVICE_INFO *pGgInfo = &pDevInfo->SpecialInfo.stGigEInfo;
			sprintf(pCAM->model, "%s", (char *)pGgInfo->chModelName);
			sprintf(pCAM->sn, "%s", (char *)pGgInfo->chSerialNumber);

			log_info("CAM[%d] : GIGE : '%s' - '%s' (%s).", i, pCAM->model, pCAM->sn, 
				(char *)pGgInfo->chUserDefinedName);
		}
			break;
		case MV_USB_DEVICE: {
			MV_USB3_DEVICE_INFO *pU3Info = &pDevInfo->SpecialInfo.stUsb3VInfo;

			sprintf(pCAM->model, "%s", (char *)pU3Info->chModelName);
			sprintf(pCAM->sn, "%s", (char *)pU3Info->chSerialNumber);

			log_info("CAM[%d] : USB3 : '%s' - '%s' (%s).", i, pCAM->model, pCAM->sn, 
				(char *)pU3Info->chUserDefinedName);
		}
			break;
		default:
			log_warn("WARN : [%d] not support type : 0x%08X.", i, pDevInfo->nTLayerType);
			break;
		}
	}

	csv_gev_reg_value_update(REG_NumberofStreamChannels, pMVS->cnt_mvs);

	return pMVS->cnt_mvs;
}

int csv_mvs_cams_enum (struct csv_mvs_t *pMVS)
{
	return pMVS->cnt_mvs;
}

int csv_mvs_cams_reset (struct csv_mvs_t *pMVS)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct cam_hk_spec_t *pCAM = NULL;

	for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			errNum++;
			continue;
		}

		nRet = MV_CC_SetCommandValue(pCAM->pHandle, "DeviceReset");
		if (MV_OK != nRet) {
			log_warn("ERROR : SetCommandValue 'DeviceReset' errcode[0x%08X]:'%s'.", 
				nRet, strMsg(nRet));
		}

		csv_mvs_camera_deinit(pCAM);
	}

	if (errNum > 0) {
		return -1;
	}

	return 0;
}

int csv_mvs_cams_open (struct csv_mvs_t *pMVS)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct cam_hk_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if (NULL == pCAM->pHandle) {
			errNum++;
			continue;
		}

		if (!pCAM->grabbing) {
			nRet = MV_CC_SetEnumValue(pCAM->pHandle, "TriggerMode", MV_TRIGGER_MODE_ON);
			if (MV_OK != nRet) {
				log_warn("ERROR : SetEnumValue 'TriggerMode' errcode[0x%08X]:'%s'.", 
					nRet, strMsg(nRet));
				errNum++;
			}

			log_info("StartGrabbing CAM '%s' : '%s'.", pCAM->model, pCAM->sn);

			// 准备开始取数据流
			nRet = MV_CC_StartGrabbing(pCAM->pHandle);
			if (MV_OK != nRet) {
				log_warn("ERROR : StartGrabbing errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
				errNum++;
	        }
			//pCAM->grabbing = true;
		}
    }

	if (errNum > 0){
		return -1;
	}

	return 0;
}

int csv_mvs_cams_close (struct csv_mvs_t *pMVS)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct cam_hk_spec_t *pCAM = NULL;

	for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if (NULL == pCAM->pHandle) {
			errNum++;
			continue;
		}

		if (pCAM->grabbing) {
			nRet = MV_CC_StopGrabbing(pCAM->pHandle);
			if (MV_OK != nRet) {
				log_warn("ERROR : CAM '%s' StopGrabbing errcode[0x%08X]:'%s'.", 
					pCAM->sn, nRet, strMsg(nRet));
			}
			pCAM->grabbing = false;
		}
    }

	if (errNum > 0) {
		return -1;
	}

	return 0;
}

/* 获取相机曝光参数 */
int csv_mvs_cams_exposure_get (struct csv_mvs_t *pMVS)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct cam_hk_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			errNum++;
			continue;
		}

		nRet = MV_CC_GetFloatValue(pCAM->pHandle, "ExposureTime", &pCAM->expoTime);
		if (MV_OK == nRet) {
			log_info("OK : CAM '%s' get ExposureTime : %f [%f, %f].", pCAM->sn, 
				pCAM->expoTime.fCurValue, pCAM->expoTime.fMin, pCAM->expoTime.fMax);
		} else {
			errNum++;
		}

		gCSV->dlp.expoTime = pCAM->expoTime.fCurValue;
	}

	if (errNum > 0) {
		return -1;
	}

	return 0;
}

/* 设置相机曝光参数 */
int csv_mvs_cams_exposure_set (struct csv_mvs_t *pMVS, float fExposureTime)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct cam_hk_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			errNum++;
			continue;
		}

		if (pCAM->expoTime.fMax < fExposureTime) {
			fExposureTime = pCAM->expoTime.fMax;
		}

		if (pCAM->expoTime.fMin > fExposureTime) {
			fExposureTime = pCAM->expoTime.fMin;
		}

		nRet = MV_CC_SetFloatValue(pCAM->pHandle, "ExposureTime", fExposureTime);
		if (MV_OK == nRet) {
			log_info("OK : CAM '%s' set ExposureTime : %f.", pCAM->sn, fExposureTime);
		} else {
			log_warn("ERROR : CAM '%s' set ExposureTime errcode[0x%08X]:'%s'.", 
				pCAM->sn, nRet, strMsg(nRet));
			errNum++;
		}
	}

	if (errNum > 0) {
		return -1;
	}

	return 0;
}

/* 获取相机增益参数 */
int csv_mvs_cams_gain_get (struct csv_mvs_t *pMVS)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct cam_hk_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			errNum++;
			continue;
		}

		nRet = MV_CC_GetFloatValue(pCAM->pHandle, "Gain", &pCAM->gain);
		if (MV_OK != nRet) {
			log_warn("ERROR : CAM '%s' get gain errcode[0x%08X]:'%s'.", 
				pCAM->sn, nRet, strMsg(nRet));
			errNum++;
		}
	}

	if (errNum > 0) {
		return -1;
	}

	return 0;
}

/* 设置相机增益参数 */
int csv_mvs_cams_gain_set (struct csv_mvs_t *pMVS, float fGain)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct cam_hk_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			errNum++;
			continue;
		}

		if (pCAM->gain.fMax < fGain) {
			fGain = pCAM->gain.fMax;
		}

		if (pCAM->gain.fMin > fGain) {
			fGain = pCAM->gain.fMin;
		}

		nRet = MV_CC_SetFloatValue(pCAM->pHandle, "Gain", fGain);
		if (MV_OK != nRet) {
			log_warn("ERROR : CAM '%s' set gain errcode[0x%08X]:'%s'.", 
				pCAM->sn, nRet, strMsg(nRet));
			errNum++;
		}
	}

	if (errNum > 0) {
		return -1;
	}

	return 0;
}

/* 设置用户自定义的相机名称参数 */
int csv_mvs_cams_name_set (struct csv_mvs_t *pMVS, char *camSNum, char *strValue)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct cam_hk_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			errNum++;
			continue;
		}

		if (strstr(pCAM->sn, camSNum)) {
			nRet = MV_CC_SetStringValue(pCAM->pHandle, "DeviceUserID", (char*)strValue);
			if (MV_OK == nRet) {
				log_info("OK ：CAM '%s' set DeviceUserID.", pCAM->sn);

				return 0;
            } else {
				log_warn("ERROR : CAM '%s' set DeviceUserID errcode[0x%08X]:'%s'.", 
					pCAM->sn, nRet, strMsg(nRet));

				return -1;
			}
		}
	}

	log_warn("WARN : CAM '%s' not found.", camSNum);

	return -1;
}

/* 抓取左右相机的图片 */
int csv_mvs_cams_grab_both (struct csv_mvs_t *pMVS)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct cam_hk_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if (pCAM->grabbing) {
			return -2;
		}

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)||(NULL == pCAM->imgData)) {
			errNum++;
			nRet = -1;
			continue;
		}

		pCAM->grabbing = true;

		memset(pCAM->imgData, 0x00, pCAM->sizePayload.nCurValue);
		memset(&pCAM->imgInfo, 0x00, sizeof(MV_FRAME_OUT_INFO_EX));
		nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
			pCAM->sizePayload.nCurValue, &pCAM->imgInfo, 3000);
		if (nRet == MV_OK) {
			log_info("OK : CAM '%s' GetOneFrame[%d] %d x %d.", pCAM->sn, 
				pCAM->imgInfo.nFrameNum, pCAM->imgInfo.nWidth, pCAM->imgInfo.nHeight);
		} else {
			log_warn("ERROR : CAM '%s' GetOneFrameTimeout errcode[0x%08X]:'%s'.", 
				pCAM->sn, nRet, strMsg(nRet));
			errNum++;
		}

		pCAM->grabbing = false;
	}

	if (errNum > 0) {
		return -1;
	}

	return nRet;
}

/* *path : 路径
group : 次数
idx : 编号
lr : 左右
suffix : 后缀类型
*img_file : 生成名
*/
static int generate_image_filename (char *path, uint16_t group, 
	int idx, int lr, uint8_t suffix, char *img_file)
{
	switch (suffix) {
	case SUFFIX_PNG:
		gCSV->cfg.devicecfg.strSuffix = ".png";
		break;
	case SUFFIX_JPG:
		gCSV->cfg.devicecfg.strSuffix = ".jpg";
		break;
	case SUFFIX_BMP:
	default:
		gCSV->cfg.devicecfg.strSuffix = ".bmp";
		break;
	}

	if (idx == 0) {
		snprintf(img_file, 128, "%s/CSV_%03dC%d%s", 
			path, group, lr+1, gCSV->cfg.devicecfg.strSuffix);
	} else {
		snprintf(img_file, 128, "%s/CSV_%03dC%dS00P%03d%s", 
			path, group, lr+1, idx, gCSV->cfg.devicecfg.strSuffix);
	}

	log_debug("img : '%s'", img_file);

	return 0;
}


static int save_image_to_bmp (MV_FRAME_OUT_INFO_EX *stImageInfo, void *handle,
	uint8_t *pData, char *img_name)
{
	int nRet = MV_OK;
	int ret = 0;
	uint8_t *pDataForSaveImage = NULL;

	if ((NULL == img_name)&&(!isprint(img_name[0]))) {
		log_warn("ERROR : img name error.");
		return -1;
	}

	if ((NULL == stImageInfo)||(NULL == handle)||(NULL == pData)) {
		log_warn("ERROR : null point.");
		return -1;
	}

	pDataForSaveImage = (uint8_t*)malloc(stImageInfo->nWidth * stImageInfo->nHeight * 4 + 2048);
	if (NULL == pDataForSaveImage) {
		log_err("ERROR : malloc DataForSaveImage.");
		return -1;
	}

	// 填充存图参数
	// fill in the parameters of save image
	MV_SAVE_IMAGE_PARAM_EX stSaveParam;
	memset(&stSaveParam, 0, sizeof(MV_SAVE_IMAGE_PARAM_EX));
	// 从上到下依次是：输出图片格式，输入数据的像素格式，提供的输出缓冲区大小，图像宽，
	// 图像高，输入数据缓存，输出图片缓存，JPG编码质量
	// Top to bottom are：
	stSaveParam.enImageType = MV_Image_Bmp; 
	stSaveParam.enPixelType = stImageInfo->enPixelType; 
	stSaveParam.nBufferSize = stImageInfo->nWidth * stImageInfo->nHeight * 4 + 2048;
	stSaveParam.nWidth      = stImageInfo->nWidth; 
	stSaveParam.nHeight     = stImageInfo->nHeight; 
	stSaveParam.pData       = pData;
	stSaveParam.nDataLen    = stImageInfo->nFrameLen;
	stSaveParam.pImageBuffer = pDataForSaveImage;
	stSaveParam.nJpgQuality = 80;

	nRet = MV_CC_SaveImageEx2(handle, &stSaveParam);
	if (MV_OK != nRet) {
		log_warn("ERROR : SaveImageEx2 errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
		ret = -1;
		goto exit;
	}

	ret = csv_file_write_data(img_name, pDataForSaveImage, stSaveParam.nImageLen);
	if (ret < 0) {
		log_warn("ERROR : write file '%s'.", img_name);
	}

exit:

	if (NULL != pDataForSaveImage) {
		free(pDataForSaveImage);
	}

	return ret;
}

static int save_image_to_jpeg (MV_FRAME_OUT_INFO_EX *stImageInfo, void *handle,
	uint8_t *pData, uint32_t nData, char *img_name)
{
	int nRet = MV_OK;
	int ret = 0;
	uint8_t *pDataForSaveImage = NULL;

	if ((NULL == img_name)&&(!isprint(img_name[0]))) {
		log_warn("ERROR : img name error.");
		return -1;
	}

	if ((NULL == stImageInfo)||(NULL == handle)||(NULL == pData)) {
		log_warn("ERROR : null point.");
		return -1;
	}

	pDataForSaveImage = (uint8_t*)malloc(nData);
	if (NULL == pDataForSaveImage) {
		log_err("ERROR : malloc DataForSaveImage.");
		return -1;
	}

	// 填充存图参数
	// fill in the parameters of save image
	MV_SAVE_IMAGE_PARAM_EX stSaveParam;
	memset(&stSaveParam, 0, sizeof(MV_SAVE_IMAGE_PARAM_EX));
	// 从上到下依次是：输出图片格式，输入数据的像素格式，提供的输出缓冲区大小，图像宽，
	// 图像高，输入数据缓存，输出图片缓存，JPG编码质量
	// Top to bottom are：
	stSaveParam.enImageType = MV_Image_Jpeg; 
	stSaveParam.enPixelType = stImageInfo->enPixelType; 
	stSaveParam.nBufferSize = nData;
	stSaveParam.nWidth      = stImageInfo->nWidth; 
	stSaveParam.nHeight     = stImageInfo->nHeight; 
	stSaveParam.pData       = pData;
	stSaveParam.nDataLen    = stImageInfo->nFrameLen;
	stSaveParam.pImageBuffer = pDataForSaveImage;
	stSaveParam.nJpgQuality = 90;

	nRet = MV_CC_SaveImageEx2(handle, &stSaveParam);
	if (MV_OK != nRet) {
		log_warn("ERROR : SaveImageEx2 errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
		ret = -1;
		goto exit;
	}

	ret = csv_file_write_data(img_name, pDataForSaveImage, stSaveParam.nImageLen);
	if (ret < 0) {
		log_warn("ERROR : write file '%s'.", img_name);
	}

exit:

	if (NULL != pDataForSaveImage) {
		free(pDataForSaveImage);
	}

	return ret;
}

static int save_image_to_file (MV_FRAME_OUT_INFO_EX *stImageInfo, void *handle,
	uint8_t *pData, uint32_t nData, uint8_t suffix, char *img_name, uint8_t end)
{
	int ret = 0;

	switch (suffix) {
	case SUFFIX_JPG:
		ret = save_image_to_jpeg(stImageInfo, handle, pData, nData, img_name);
		break;
	case SUFFIX_PNG:
		csv_png_push(img_name, pData, nData, stImageInfo->nWidth, stImageInfo->nHeight, end);
		break;
	case SUFFIX_BMP:
	default:
		ret = save_image_to_bmp(stImageInfo, handle, pData, img_name);
		break;
	}

	return ret;
}

static int csv_mvs_cams_demarcate (struct csv_mvs_t *pMVS)
{
	int ret = 0, i = 0;
	int nRet = MV_OK;
	int nFrames = 23;
	int idx = 0;
	char img_name[256] = {0};
	struct cam_hk_spec_t *pCAM = NULL;
	struct calib_conf_t *pCALIB = &gCSV->cfg.calibcfg;
	struct device_cfg_t *pDevC = &gCSV->cfg.devicecfg;

	if ((NULL == pCALIB->path)
		||(!csv_file_isExist(pCALIB->path))) {
		log_warn("ERROR : cali img path null.");
		return -1;
	}

	// 1 亮光
	ret = csv_dlp_just_write(DLP_CMD_BRIGHT);

	for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];
		if (pCAM->grabbing) {
			return -2;
		}

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			continue;
		}

		pCAM->grabbing = true;

		memset(&pCAM->imgInfo, 0x00, sizeof(MV_FRAME_OUT_INFO_EX));
		nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
			pCAM->sizePayload.nCurValue, &pCAM->imgInfo, 3000);
		if (nRet == MV_OK) {
			log_info("OK : CAM '%s' [%d_%02d]: GetOneFrame[%d] %d x %d.", pCAM->sn, idx, i, 
				pCAM->imgInfo.nFrameNum, pCAM->imgInfo.nWidth, pCAM->imgInfo.nHeight);

			memset(img_name, 0, 256);
			generate_image_filename(pCALIB->path, pCALIB->groupDemarcate, idx, i, 
				pDevC->SaveImageFormat, img_name);
			ret = save_image_to_file(&pCAM->imgInfo, pCAM->pHandle, 
				pCAM->imgData, pCAM->sizePayload.nCurValue, pDevC->SaveImageFormat, img_name, 0);
		} else {
			log_warn("ERROR : CAM '%s' [%d_%02d]: GetOneFrameTimeout, errcode[0x%08X]:'%s'.", 
				pCAM->sn, idx, i, nRet, strMsg(nRet));
				if (MV_E_NODATA == nRet) {
					pCAM->grabbing = false;
					return -1;
				}
		}

		pCAM->grabbing = false;
	}

	idx++;

	// 22 标定
	ret = csv_dlp_just_write(DLP_CMD_DEMARCATE);

	while (idx < nFrames) {
		for (i = 0; i < pMVS->cnt_mvs; i++) {
			pCAM = &pMVS->Cam[i];

			if (pCAM->grabbing) {
				return -2;
			}

			if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
				continue;
			}

			pCAM->grabbing = true;

			memset(&pCAM->imgInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
			nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
				pCAM->sizePayload.nCurValue, &pCAM->imgInfo, 3000);
			if (nRet == MV_OK) {
				log_info("OK : CAM '%s' [%d_%02d]: GetOneFrame[%d] %d x %d.", pCAM->sn, idx, i, 
					pCAM->imgInfo.nFrameNum, pCAM->imgInfo.nWidth, pCAM->imgInfo.nHeight);

				memset(img_name, 0, 256);
				generate_image_filename(pCALIB->path, pCALIB->groupDemarcate, idx, i, 
					pDevC->SaveImageFormat, img_name);
				ret = save_image_to_file(&pCAM->imgInfo, pCAM->pHandle, 
					pCAM->imgData, pCAM->sizePayload.nCurValue, pDevC->SaveImageFormat, img_name, 0);
			} else {
				log_warn("ERROR : CAM '%s' [%d_%02d]: GetOneFrameTimeout, errcode[0x%08X]:'%s'.", 
					pCAM->sn, idx, i, nRet, strMsg(nRet));
				if (MV_E_NODATA == nRet) {
					pCAM->grabbing = false;
					return -1;
				}
			}

			pCAM->grabbing = false;
		}

		idx++;
	}

	if (SUFFIX_PNG == pDevC->SaveImageFormat) {
		pthread_cond_broadcast(&gCSV->png.cond_png);
	}

	pCALIB->groupDemarcate++;
	csv_xml_write_CalibParameters();

	return ret;
}

static int csv_mvs_cams_highspeed (struct csv_mvs_t *pMVS)
{
	int ret = 0, i = 0;
	int nRet = MV_OK;
	int nFrames = 13;
	int idx = 1;
	uint8_t end_pc = 0;
	char img_name[256] = {0};
	struct cam_hk_spec_t *pCAM = NULL;
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;
	struct device_cfg_t *pDevC = &gCSV->cfg.devicecfg;

	// 13 高速光
	ret = csv_dlp_just_write(DLP_CMD_HIGHSPEED);

	pMVS->firstTimestamp = utility_get_microsecond();

	while (idx <= nFrames) {
		for (i = 0; i < pMVS->cnt_mvs; i++) {
			pCAM = &pMVS->Cam[i];

			if (pCAM->grabbing) {
				return -2;
			}

			if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
				continue;
			}

			pCAM->grabbing = true;

			memset(&pCAM->imgInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
			nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
				pCAM->sizePayload.nCurValue, &pCAM->imgInfo, 3000);
			if (nRet == MV_OK) {
				log_info("OK : CAM '%s' [%d_%02d]: GetOneFrame[%d] %d x %d.", pCAM->sn, idx, i, 
					pCAM->imgInfo.nFrameNum, pCAM->imgInfo.nWidth, pCAM->imgInfo.nHeight);

				if ((idx == nFrames)&&(i == 1)) {
					end_pc = 1;
				}
				memset(img_name, 0, 256);
				generate_image_filename(pPC->ImageSaveRoot, pPC->groupPointCloud, idx, i, 
					pDevC->SaveImageFormat, img_name);
				ret = save_image_to_file(&pCAM->imgInfo, pCAM->pHandle, pCAM->imgData, 
					pCAM->sizePayload.nCurValue, pDevC->SaveImageFormat, img_name, end_pc);
			} else {
				log_warn("ERROR : CAM '%s' [%d_%02d]: GetOneFrameTimeout, errcode[0x%08X]:'%s'.", 
					pCAM->sn, idx, i, nRet, strMsg(nRet));

				if (MV_E_NODATA == nRet) {
					pCAM->grabbing = false;
					return -1;
				}
			}

			pCAM->grabbing = false;
		}

		idx++;
	}

	pMVS->lastTimestamp = utility_get_microsecond();
	log_debug("highspeed 13 take %ld us.", pMVS->lastTimestamp - pMVS->firstTimestamp);

	if (SUFFIX_PNG == pDevC->SaveImageFormat) {
		pthread_cond_broadcast(&gCSV->png.cond_png);
	} else {
		if (pPC->enable) {
			ret = csv_3d_calc();
		} else {
			pPC->groupPointCloud++;
		}
	}

	return ret;
}

int csv_mvs_cams_img_depth (struct csv_mvs_t *pMVS)
{
	int ret = 0, i = 0, errNum = 0;
	int nRet = MV_OK;
	int nFrames = 13;
	int idx = 1;
	uint8_t end_pc = 0;
	char img_name[256] = {0};
	struct cam_hk_spec_t *pCAM = NULL;
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;
	struct device_cfg_t *pDevC = &gCSV->cfg.devicecfg;

	// 13 高速光
	ret = csv_dlp_just_write(DLP_CMD_HIGHSPEED);

	pMVS->firstTimestamp = utility_get_microsecond();

	while (idx <= nFrames) {
		for (i = 0; i < pMVS->cnt_mvs; i++) {
			pCAM = &pMVS->Cam[i];

			if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
				errNum++;
				continue;
			}

			memset(&pCAM->imgInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
			nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
				pCAM->sizePayload.nCurValue, &pCAM->imgInfo, 3000);
			if (nRet == MV_OK) {
				log_info("OK : CAM '%s' [%d_%02d]: GetOneFrame[%d] %d x %d.", pCAM->sn, idx, i, 
					pCAM->imgInfo.nFrameNum, pCAM->imgInfo.nWidth, pCAM->imgInfo.nHeight);

				if ((idx == nFrames)&&(i == 1)) {
					end_pc = 1;
				}
				memset(img_name, 0, 256);
				generate_image_filename(pPC->ImageSaveRoot, pPC->groupPointCloud, idx, i, 
					pDevC->SaveImageFormat, img_name);
				ret = save_image_to_file(&pCAM->imgInfo, pCAM->pHandle, pCAM->imgData, 
					pCAM->sizePayload.nCurValue, pDevC->SaveImageFormat, img_name, end_pc);
			} else {
				log_warn("ERROR : CAM '%s' [%d_%02d]: GetOneFrameTimeout, errcode[0x%08X]:'%s'.", 
					pCAM->sn, idx, i, nRet, strMsg(nRet));
				errNum++;
				break;
			}
		}

		if (errNum > 0) {
			break;
		}

		idx++;
	}

	pMVS->lastTimestamp = utility_get_microsecond();
	log_debug("highspeed 13 take %ld us.", pMVS->lastTimestamp - pMVS->firstTimestamp);

	if (errNum > 0) {
		return -1;
	}

	ret = csv_3d_calc();

	return ret;
}

static void *csv_mvs_grab_loop (void *data)
{
	if (NULL == data) {
		log_warn("ERROR : critical failed.");
		return NULL;
	}

	int ret = 0;
	struct csv_mvs_t *pMVS = (struct csv_mvs_t *)data;

	while (1) {
		pthread_mutex_lock(&pMVS->mutex_grab);
		ret = pthread_cond_wait(&pMVS->cond_grab, &pMVS->mutex_grab);
		pthread_mutex_unlock(&pMVS->mutex_grab);
		if (ret != 0) {
			log_err("ERROR : pthread_cond_wait %s.", pMVS->name_grab);
			break;
		}
		ret = csv_mvs_cams_open(pMVS);
		switch (pMVS->grab_type) {
		case GRAB_DEMARCATE:
			csv_mvs_cams_demarcate(pMVS);
			break;
		case GRAB_HIGHSPEED:
			csv_mvs_cams_highspeed(pMVS);
			break;
		default:
			break;
		}
		ret = csv_mvs_cams_close(pMVS);
	}

	log_warn("WARN : exit pthread %s.", pMVS->name_grab);

	pMVS->thr_grab = 0;

	pthread_exit(NULL);

	return NULL;
}

static int csv_mvs_grab_thread (struct csv_mvs_t *pMVS)
{
	int ret = -1;

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pMVS->mutex_grab, NULL) != 0) {
		log_err("ERROR : mutex %s.", pMVS->name_grab);
        return -1;
    }

    if (pthread_cond_init(&pMVS->cond_grab, NULL) != 0) {
		log_err("ERROR : cond %s.", pMVS->name_grab);
        return -1;
    }

	ret = pthread_create(&pMVS->thr_grab, &attr, csv_mvs_grab_loop, (void *)pMVS);
	if (ret < 0) {
		log_err("ERROR : create pthread %s.", pMVS->name_grab);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p).", pMVS->name_grab, pMVS->thr_grab);
	}

	//pthread_attr_destory(&attr);

	return ret;
}

static int csv_mvs_grab_thread_cancel (struct csv_mvs_t *pMVS)
{
	int ret = 0;
	void *retval = NULL;

	if (pMVS->thr_grab <= 0) {
		return 0;
	}

	ret = pthread_cancel(pMVS->thr_grab);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s.", pMVS->name_grab);
	} else {
		log_info("OK : cancel pthread %s (%p).", pMVS->name_grab, pMVS->thr_grab);
	}

	ret = pthread_join(pMVS->thr_grab, &retval);

	pMVS->thr_grab = 0;

	return ret;
}


static void *csv_mvs_loop (void *data)
{
	if (NULL == data) {
		log_warn("ERROR : critical failed.");
		return NULL;
	}

	int ret = 0, i = 0, timeo = 0;
	struct csv_mvs_t *pMVS = (struct csv_mvs_t *)data;

	sleep(2);
	csv_3d_init();

	while (1) {
		do {
			sleep(MVS_WAIT_PERIOD);
			ret = csv_mvs_cameras_search(pMVS);
			if (ret < TOTAL_CAMS) {
				log_info("Search times[%d] not enough cams.", timeo+1);
			}
		} while ((ret < TOTAL_CAMS)&&(++timeo < MVS_WAIT_TIMEO));

		timeo = 0;

		if (ret > 0) {
			csv_mvs_cameras_init(pMVS);
		}

		pthread_mutex_lock(&pMVS->mutex_mvs);
		ret = pthread_cond_wait(&pMVS->cond_mvs, &pMVS->mutex_mvs);
		pthread_mutex_unlock(&pMVS->mutex_mvs);
		if (ret != 0) {
			log_err("ERROR : pthread_cond_wait.");
			break;
		}

	    for (i = 0; i < pMVS->cnt_mvs; i++) {
			csv_mvs_camera_deinit(&pMVS->Cam[i]);
		}
		pMVS->cnt_mvs = 0;

	}

	log_warn("WARN : exit pthread %s.", pMVS->name_mvs);

	pMVS->thr_mvs = 0;

	pthread_exit(NULL);

	return NULL;
}

static int csv_mvs_thread (struct csv_mvs_t *pMVS)
{
	int ret = -1;

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pMVS->mutex_mvs, NULL) != 0) {
		log_err("ERROR : mutex %s.", pMVS->name_mvs);
        return -1;
    }

    if (pthread_cond_init(&pMVS->cond_mvs, NULL) != 0) {
		log_err("ERROR : cond %s.", pMVS->name_mvs);
        return -1;
    }

	ret = pthread_create(&pMVS->thr_mvs, &attr, csv_mvs_loop, (void *)pMVS);
	if (ret < 0) {
		log_err("ERROR : create pthread %s.", pMVS->name_mvs);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p).", pMVS->name_mvs, pMVS->thr_mvs);
	}

	//pthread_attr_destory(&attr);

	return ret;
}

static int csv_mvs_thread_cancel (struct csv_mvs_t *pMVS)
{
	int ret = 0;
	void *retval = NULL;

	if (pMVS->thr_mvs <= 0) {
		return 0;
	}

	ret = pthread_cancel(pMVS->thr_mvs);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s.", pMVS->name_mvs);
	} else {
		log_info("OK : cancel pthread %s (%p).", pMVS->name_mvs, pMVS->thr_mvs);
	}

	ret = pthread_join(pMVS->thr_mvs, &retval);

	pMVS->thr_mvs = 0;

	return ret;
}



int csv_mvs_init (void)
{
	int ret = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;

	pMVS->cnt_mvs = 0;
	pMVS->name_mvs = NAME_THREAD_MVS;
	pMVS->name_grab = NAME_THREAD_GRAB;
	pMVS->firstTimestamp = 0;
	pMVS->lastTimestamp = 0;

	ret = csv_mvs_thread(pMVS);
	ret |= csv_mvs_grab_thread(pMVS);

	return ret;
}

int csv_mvs_deinit (void)
{
	int ret = 0, i = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		ret |= csv_mvs_camera_deinit(&pMVS->Cam[i]);
	}

	ret |= csv_mvs_grab_thread_cancel(pMVS);
	ret |= csv_mvs_thread_cancel(pMVS);

	return ret;
}

#ifdef __cplusplus
}
#endif

