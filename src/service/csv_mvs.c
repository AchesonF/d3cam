#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static int csv_mvs_camera_deinit (struct cam_spec_t *pCAM)
{
	int nRet = MV_OK;

	if ((NULL == pCAM)||(NULL == pCAM->pHandle) || (!pCAM->opened)) {
		return -1;
	}

	log_info("CAM %s detach handle.", pCAM->serialNum);

	if (pCAM->grabbing) {
		nRet = MV_CC_StopGrabbing(pCAM->pHandle);
		if (MV_OK != nRet) {
			log_info("ERROR : CAM '%s' StopGrabbing failed. [0x%08X]", pCAM->serialNum, nRet);
		}
	}

	nRet = MV_CC_CloseDevice(pCAM->pHandle);
	if (MV_OK != nRet) {
		log_info("ERROR : CAM '%s' CloseDevice failed. [0x%08X]", pCAM->serialNum, nRet);
	}

	nRet = MV_CC_DestroyHandle(pCAM->pHandle);
	if (MV_OK != nRet) {
		log_info("ERROR : CAM '%s' DestroyHandle failed. [0x%08X]", pCAM->serialNum, nRet);
	}

	pCAM->pHandle = NULL;
	pCAM->opened = false;
	pCAM->grabbing = false;
	pCAM->stParam.nCurValue = 0;

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
	struct cam_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pDevInfo = pDevList->pDeviceInfo[i];
		if (NULL == pDevInfo) {
			log_info("ERROR : CAM[%d] critical null", i);
			break;
		}

		pCAM = &pMVS->Cam[i];
		log_info("Initialling CAM '%s' : '%s'", pCAM->modelName, pCAM->serialNum);

		pCAM->grabbing = false;

		nRet = MV_CC_CreateHandle(&pCAM->pHandle, pDevInfo);
		if (MV_OK != nRet) {
			pCAM->pHandle = NULL;
			log_info("ERROR : CreateHandle failed. [0x%08X]", nRet);
			continue;
		}

		pCAM->opened = true;

		nRet = MV_CC_OpenDevice(pCAM->pHandle, MV_ACCESS_Exclusive, 0);
		if (MV_OK != nRet) {
			log_info("ERROR : OpenDevice failed. [0x%08X]", nRet);
			nRet = MV_CC_DestroyHandle(pCAM->pHandle);
			if (MV_OK != nRet) {
				log_info("ERROR : DestroyHandle failed. [0x%08X]", nRet);
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
		if (strstr(pCAM->modelName, "UC") != NULL) {	// 彩色
			enValue = PixelType_Gvsp_RGB8_Packed;
		}

		nRet = MV_CC_SetPixelFormat(pCAM->pHandle, enValue);
		if (MV_OK != nRet) {
			if (enValue == PixelType_Gvsp_RGB8_Packed) {
				log_info("ERROR : SetPixelFormat 'RGB8_Packed'. [0x%08X]", nRet);
			} else if (enValue == PixelType_Gvsp_Mono8) {
				log_info("ERROR : SetPixelFormat 'Mono8'. [0x%08X]", nRet);
			}
		}

		nRet = MV_CC_SetBoolValue(pCAM->pHandle, "AcquisitionFrameRateEnable", false);
		if (MV_OK != nRet) {
			log_info("ERROR : SetBoolValue 'AcquisitionFrameRateEnable' failed. [0x%08X]", nRet);
		}

		// 设置触发模式为 ON
		nRet = MV_CC_SetEnumValue(pCAM->pHandle, "TriggerMode", MV_TRIGGER_MODE_ON);
		if (MV_OK != nRet) {
			log_info("ERROR : SetEnumValue 'TriggerMode' failed. [0x%08X]", nRet);
		}

		// 设置触发源 line0
		nRet = MV_CC_SetEnumValue(pCAM->pHandle, "TriggerSource", MV_TRIGGER_SOURCE_LINE0);
		if (MV_OK != nRet) {
			log_info("ERROR : SetEnumValue 'TriggerSource' failed. [0x%08X]", nRet);
		}

		nRet = MV_CC_GetFloatValue(pCAM->pHandle, "ExposureTime", &pCAM->exposureTime);
		if (MV_OK == nRet) {
			log_info("OK ：GetFloatValue 'ExposureTime' : %f [%f, %f]", pCAM->exposureTime.fCurValue, 
				pCAM->exposureTime.fMin, pCAM->exposureTime.fMax);
		}

		nRet = MV_CC_GetFloatValue(pCAM->pHandle, "Gain", &pCAM->gain);
		if (MV_OK == nRet) {
			log_info("OK : GetFloatValue 'Gain' : %f [%f, %f]", pCAM->gain.fCurValue, 
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
		nRet = MV_CC_GetIntValue(pCAM->pHandle, "PayloadSize", &pCAM->stParam);
		if (MV_OK != nRet) {
			log_info("ERROR : GetIntValue 'PayloadSize' failed. [0x%08X]", nRet);
		}

		MV_CC_GetIntValue(pCAM->pHandle, "Width", &struIntWidth);
		MV_CC_GetIntValue(pCAM->pHandle, "Height", &struIntHeight);
		log_info("OK : GetIntValue 'Width x Height' = [%d x %d]", 
			struIntWidth.nCurValue, struIntHeight.nCurValue);

		pCAM->imgData = (uint8_t *)malloc(pCAM->stParam.nCurValue);
		if (pCAM->imgData == NULL){
			log_err("ERROR : malloc imgData");
			csv_mvs_camera_deinit(pCAM);
			continue;
		}
		memset(pCAM->imgData, 0x00, pCAM->stParam.nCurValue);

    }

	if (pMVS->Cam[CAM_LEFT].stParam.nCurValue != pMVS->Cam[CAM_RIGHT].stParam.nCurValue) {
		log_info("ERROR : CAMS cannot be coupled. PayloadSize : %d vs %d", 
			pMVS->Cam[CAM_LEFT].stParam.nCurValue,
			pMVS->Cam[CAM_RIGHT].stParam.nCurValue);
		return -1;
	}

	return 0;
}

static int csv_mvs_cameras_search (struct csv_mvs_t *pMVS)
{
	int nRet = 0, i = 0;
	MV_CC_DEVICE_INFO_LIST *pDevList = &pMVS->stDeviceList;
	MV_CC_DEVICE_INFO *pDevInfo = NULL;
	struct cam_spec_t *pCAM = NULL;

	pMVS->cnt_mvs = 0;
	memset(pDevList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

	// enum device
	//nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, pDevList);
	nRet = MV_CC_EnumDevices(MV_USB_DEVICE, pDevList);
	if (MV_OK != nRet) {
		log_info("ERROR : EnumDevices failed [0x%08X]", nRet);
		return -1;
	}

	if (pDevList->nDeviceNum > 0) {
		log_info("OK : found %d CAM devices.", pDevList->nDeviceNum);
	} else {
		log_info("WARN : NO CAM devices.");
		return 0;
	}

	pMVS->cnt_mvs = pDevList->nDeviceNum;

	if (pDevList->nDeviceNum > TOTAL_CAMS) {
		pMVS->cnt_mvs = TOTAL_CAMS;
		log_info("Only USE first %d CAM devices.", TOTAL_CAMS);
	}

	for (i = 0; i < pMVS->cnt_mvs; i++) {
		log_info("CAM[%d] : ", i);

		pDevInfo = pDevList->pDeviceInfo[i];
		if (NULL == pDevInfo) {
			log_info("ERROR : CAM pDeviceInfo");
			continue;
		}

		pCAM = &pMVS->Cam[i];

		switch (pDevInfo->nTLayerType) {
		case MV_GIGE_DEVICE: {
			MV_GIGE_DEVICE_INFO *pGgInfo = &pDevInfo->SpecialInfo.stGigEInfo;
			sprintf(pCAM->modelName, "%s", (char *)pGgInfo->chModelName);
			sprintf(pCAM->serialNum, "%s", (char *)pGgInfo->chSerialNumber);

			log_info("GIGE : '%s' - '%s'", pCAM->modelName, pCAM->serialNum);
			//log_info("OK : UserDefinedName[%d] %s", i, (char *)pGgInfo->chUserDefinedName);
		}
			break;
		case MV_USB_DEVICE: {
			MV_USB3_DEVICE_INFO *pU3Info = &pDevInfo->SpecialInfo.stUsb3VInfo;

			sprintf(pCAM->modelName, "%s", (char *)pU3Info->chModelName);
			sprintf(pCAM->serialNum, "%s", (char *)pU3Info->chSerialNumber);

			log_info("USB3 : '%s' - '%s'", pCAM->modelName, pCAM->serialNum);
			//log_info("OK : UserDefinedName[%d] %s", i, (char *)pU3Info->chUserDefinedName);
		}
			break;
		default:
			log_info("WARN : [%d] not support type : 0x%08X", i, pDevInfo->nTLayerType);
			break;
		}
	}

	// TODO sort/switch side

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
	struct cam_spec_t *pCAM = NULL;

	for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			errNum++;
			continue;
		}

		nRet = MV_CC_SetCommandValue(pCAM->pHandle, "DeviceReset");
		if (MV_OK != nRet) {
			log_info("ERROR : SetCommandValue 'DeviceReset' failed. [0x%08X]", nRet);
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
	struct cam_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if (NULL == pCAM->pHandle) {
			errNum++;
			continue;
		}

		if (!pCAM->grabbing) {
			log_info("StartGrabbing CAM '%s' : '%s'", pCAM->modelName, pCAM->serialNum);

			// 准备开始取数据流
			nRet = MV_CC_StartGrabbing(pCAM->pHandle);
			if (MV_OK != nRet) {
				log_info("ERROR : StartGrabbing failed. [0x%08X]", nRet);
				errNum++;
	        }
			pCAM->grabbing = true;
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
	struct cam_spec_t *pCAM = NULL;

	for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if (NULL == pCAM->pHandle) {
			errNum++;
			continue;
		}

		if (pCAM->grabbing) {
			nRet = MV_CC_StopGrabbing(pCAM->pHandle);
			if (MV_OK != nRet) {
				log_info("ERROR : CAM '%s' StopGrabbing failed. [0x%08X]", pCAM->serialNum, nRet);
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
	struct cam_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			errNum++;
			continue;
		}

		nRet = MV_CC_GetFloatValue(pCAM->pHandle, "ExposureTime", &pCAM->exposureTime);
		if (MV_OK == nRet) {
			log_info("OK：CAM '%s' get ExposureTime : %f [%f, %f]", pCAM->serialNum, 
				pCAM->exposureTime.fCurValue, pCAM->exposureTime.fMin, pCAM->exposureTime.fMax);
		} else {
			errNum++;
		}
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
	struct cam_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			errNum++;
			continue;
		}
/*
		if (pCAM->exposureTime.fMax < fExposureTime) {
			fExposureTime = pCAM->exposureTime.fMax;
		}

		if (pCAM->exposureTime.fMin > fExposureTime) {
			fExposureTime = pCAM->exposureTime.fMin;
		}

		nRet = MV_CC_SetFloatValue(pCAM->pHandle, "ExposureTime", fExposureTime);
		if (MV_OK == nRet) {
			log_info("OK : CAM '%s' set ExposureTime : %f", pCAM->serialNum, fExposureTime);
		} else {
			log_info("ERROR : CAM '%s' set ExposureTime failed. [0x%08X]", pCAM->serialNum, nRet);
			errNum++;
		}
*/
		if (pCAM->exposureTime.fMax < gCSV->cfg.device_param.exposure_time) {
			gCSV->cfg.device_param.exposure_time = pCAM->exposureTime.fMax;
		}

		if (pCAM->exposureTime.fMin > gCSV->cfg.device_param.exposure_time) {
			gCSV->cfg.device_param.exposure_time = pCAM->exposureTime.fMin;
		}

		nRet = MV_CC_SetFloatValue(pCAM->pHandle, "ExposureTime", gCSV->cfg.device_param.exposure_time);
		if (MV_OK == nRet) {
			log_info("OK : CAM '%s' set ExposureTime : %f", pCAM->serialNum, gCSV->cfg.device_param.exposure_time);
		} else {
			log_info("ERROR : CAM '%s' set ExposureTime failed. [0x%08X]", pCAM->serialNum, nRet);
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
	struct cam_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			errNum++;
			continue;
		}

		nRet = MV_CC_GetFloatValue(pCAM->pHandle, "Gain", &pCAM->gain);
		if (MV_OK != nRet) {
			log_info("ERROR : CAM '%s' get gain failed. [0x%08X]", pCAM->serialNum, nRet);
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
	struct cam_spec_t *pCAM = NULL;

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
			log_info("ERROR : CAM '%s' set gain failed. [0x%08X]", pCAM->serialNum, nRet);
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
	struct cam_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			errNum++;
			continue;
		}

		if (strstr(pCAM->serialNum, camSNum)) {
			nRet = MV_CC_SetStringValue(pCAM->pHandle, "DeviceUserID", (char*)strValue);
			if (MV_OK == nRet) {
				log_info("OK ：CAM '%s' set DeviceUserID.", pCAM->serialNum);

				return 0;
            } else {
				log_info("ERROR : CAM '%s' set DeviceUserID Failed. [0x%08X]", pCAM->serialNum, nRet);

				return -1;
			}
		}
	}

	log_info("WARN : CAM '%s' not found.", camSNum);

	return -1;
}

/* 抓取左右相机的图片 */
int csv_mvs_cams_grab_both (struct csv_mvs_t *pMVS)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct cam_spec_t *pCAM = NULL;

	if (pMVS->grabing) {
		return -1;
	}

	pMVS->grabing = true;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->pHandle)||(NULL == pCAM->imgData)) {
			errNum++;
			continue;
		}

		memset(pCAM->imgData, 0x00, pCAM->stParam.nCurValue);
		memset(&pCAM->imageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
		nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
			pCAM->stParam.nCurValue, &pCAM->imageInfo, 3000);
		if (nRet == MV_OK) {
			log_info("OK : CAM '%s' GetOneFrame[%d] %d x %d", pCAM->serialNum, 
				pCAM->imageInfo.nFrameNum, pCAM->imageInfo.nWidth, pCAM->imageInfo.nHeight);
		} else {
			log_info("ERROR : CAM '%s' GetOneFrameTimeout [0x%08X]", pCAM->serialNum, nRet);
			errNum++;
		}
	}

	if (errNum > 0) {
		pMVS->grabing = false;
		return -1;
	}

	pMVS->grabing = false;
	return nRet;
}


static void *csv_mvs_loop (void *data)
{
	if (NULL == data) {
		log_info("ERROR : critical failed.");
		return NULL;
	}

	int ret = 0, i = 0;
	struct csv_mvs_t *pMVS = (struct csv_mvs_t *)data;
	sleep(5);	// waiting for stable.

	while (1) {
		ret = csv_mvs_cameras_search(pMVS);
		if (ret > 0) {
			csv_mvs_cameras_init(pMVS);
		}

		pthread_mutex_lock(&pMVS->mutex_mvs);
		ret = pthread_cond_wait(&pMVS->cond_mvs, &pMVS->mutex_mvs);
		pthread_mutex_unlock(&pMVS->mutex_mvs);
		if (ret != 0) {
			log_err("ERROR : pthread_cond_wait");
			break;
		}

	    for (i = 0; i < pMVS->cnt_mvs; i++) {
			csv_mvs_camera_deinit(&pMVS->Cam[i]);
		}
		pMVS->cnt_mvs = 0;

		sleep(5);
	}

	log_info("WARN : exit pthread %s", pMVS->name_mvs);

	pMVS->thr_mvs = 0;

	pthread_exit(NULL);

	return NULL;
}

int csv_mvs_thread (struct csv_mvs_t *pMVS)
{
	int ret = -1;

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pMVS->mutex_mvs, NULL) != 0) {
		log_err("ERROR : mutex %s", pMVS->name_mvs);
        return -1;
    }

    if (pthread_cond_init(&pMVS->cond_mvs, NULL) != 0) {
		log_err("ERROR : cond %s", pMVS->name_mvs);
        return -1;
    }

	ret = pthread_create(&pMVS->thr_mvs, &attr, csv_mvs_loop, (void *)pMVS);
	if (ret < 0) {
		log_err("ERROR : create pthread %s", pMVS->name_mvs);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p)", pMVS->name_mvs, pMVS->thr_mvs);
	}

	//pthread_attr_destory(&attr);

	return ret;
}

int csv_mvs_thread_cancel (struct csv_mvs_t *pMVS)
{
	int ret = 0;
	void *retval = NULL;

	if (pMVS->thr_mvs <= 0) {
		return 0;
	}

	ret = pthread_cancel(pMVS->thr_mvs);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s", pMVS->name_mvs);
	} else {
		log_info("OK : cancel pthread %s", pMVS->name_mvs);
	}

	ret = pthread_join(pMVS->thr_mvs, &retval);

	return ret;
}

int csv_mvs_init (void)
{
	struct csv_mvs_t *pMVS = &gCSV->mvs;

	pMVS->cnt_mvs = 0;
	pMVS->name_mvs = NAME_THREAD_MVS;

	return csv_mvs_thread(pMVS);
}

int csv_mvs_deinit (void)
{
	int i = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		csv_mvs_camera_deinit(&pMVS->Cam[i]);
	}

	return csv_mvs_thread_cancel(pMVS);
}

#ifdef __cplusplus
}
#endif

