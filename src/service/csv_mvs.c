#include "inc_files.h"

#include "csv_pointcloud.hpp"

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

	if ((2 == pMVS->cnt_mvs)&&(gCSV->cfg.device_param.camera_leftright_type)) {
		MV_CC_DEVICE_INFO *pTemp = pDevList->pDeviceInfo[0];
		pDevList->pDeviceInfo[0] = pDevList->pDeviceInfo[1];
		pDevList->pDeviceInfo[1] = pTemp;
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
	char *str_suffix = NULL;

	switch (suffix) {
	case SUFFIX_PNG:
		str_suffix = ".png";
		break;
	case SUFFIX_JPG:
		str_suffix = ".jpg";
		break;
	case SUFFIX_BMP:
	default:
		str_suffix = ".bmp";
		break;
	}

	if (idx == 0) {
		snprintf(img_file, 128, "%s/CSV_%03dC%d%s", 
			path, group, lr+1, str_suffix);
	} else {
		snprintf(img_file, 128, "%s/CSV_%03dC%dS00P%03d%s", 
			path, group, lr+1, idx, str_suffix);
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
		log_info("ERROR : img name error");
		return -1;
	}

	if ((NULL == stImageInfo)||(NULL == handle)||(NULL == pData)) {
		log_info("ERROR : null point");
		return -1;
	}

	pDataForSaveImage = (uint8_t*)malloc(stImageInfo->nWidth * stImageInfo->nHeight * 4 + 2048);
	if (NULL == pDataForSaveImage) {
		log_err("ERROR : malloc DataForSaveImage");
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
		log_info("ERROR : SaveImageEx2 failed. [0x%08X]", nRet);
		ret = -1;
		goto exit;
	}

	ret = csv_file_write_data(img_name, pDataForSaveImage, stSaveParam.nImageLen);
	if (ret < 0) {
		log_info("ERROR : write file '%s'.", img_name);
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
		log_info("ERROR : img name error");
		return -1;
	}

	if ((NULL == stImageInfo)||(NULL == handle)||(NULL == pData)) {
		log_info("ERROR : null point");
		return -1;
	}

	pDataForSaveImage = (uint8_t*)malloc(nData);
	if (NULL == pDataForSaveImage) {
		log_err("ERROR : malloc DataForSaveImage");
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
		log_info("ERROR : SaveImageEx2 failed. [0x%08X]", nRet);
		ret = -1;
		goto exit;
	}

	ret = csv_file_write_data(img_name, pDataForSaveImage, stSaveParam.nImageLen);
	if (ret < 0) {
		log_info("ERROR : write file '%s'.", img_name);
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
	struct cam_spec_t *pCAM = NULL;
	struct calib_param_t *pCALIB = &gCSV->cfg.calib_param;

	if (pMVS->grabing) {
		return -1;
	}

	if ((NULL == pCALIB->path)
		||(!csv_file_isExist(pCALIB->path))) {
		log_info("ERROR : cali img path null");
		return -1;
	}

	pMVS->grabing = true;

	// 1 亮光
	ret = csv_dlp_just_write(CMD_BRIGHT);

	for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &pMVS->Cam[i];
		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			continue;
		}

		memset(&pCAM->imageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
		nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
			pCAM->stParam.nCurValue, &pCAM->imageInfo, 3000);
		if (nRet == MV_OK) {
			log_info("OK : CAM '%s' [%d_%02d]: GetOneFrame[%d] %d x %d", pCAM->serialNum, idx, i, 
				pCAM->imageInfo.nFrameNum, pCAM->imageInfo.nWidth, pCAM->imageInfo.nHeight);

			memset(img_name, 0, 256);
			generate_image_filename(pCALIB->path, pCALIB->groupDemarcate, idx, i, 
				gCSV->cfg.device_param.img_type, img_name);
			ret = save_image_to_file(&pCAM->imageInfo, pCAM->pHandle, 
				pCAM->imgData, pCAM->stParam.nCurValue, gCSV->cfg.device_param.img_type, img_name, 0);
		} else {
			log_info("ERROR : CAM '%s' [%d_%02d]: GetOneFrameTimeout, [0x%08X]", 
				pCAM->serialNum, idx, i, nRet);
				if (MV_E_NODATA == nRet) {
					log_info("ERROR : CAM '%s' NO DATA.", pCAM->serialNum);
					pMVS->grabing = false;
					return -1;
				}
		}

	}

	idx++;

	// 22 标定
	ret = csv_dlp_just_write(CMD_DEMARCATE);

	while (idx < nFrames) {
		for (i = 0; i < pMVS->cnt_mvs; i++) {
			pCAM = &pMVS->Cam[i];
			if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
				continue;
			}

			memset(&pCAM->imageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
			nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
				pCAM->stParam.nCurValue, &pCAM->imageInfo, 3000);
			if (nRet == MV_OK) {
				log_info("OK : CAM '%s' [%d_%02d]: GetOneFrame[%d] %d x %d", pCAM->serialNum, idx, i, 
					pCAM->imageInfo.nFrameNum, pCAM->imageInfo.nWidth, pCAM->imageInfo.nHeight);

				memset(img_name, 0, 256);
				generate_image_filename(pCALIB->path, pCALIB->groupDemarcate, idx, i, 
					gCSV->cfg.device_param.img_type, img_name);
				ret = save_image_to_file(&pCAM->imageInfo, pCAM->pHandle, 
					pCAM->imgData, pCAM->stParam.nCurValue, gCSV->cfg.device_param.img_type, img_name, 0);
			} else {
				log_info("ERROR : CAM '%s' [%d_%02d]: GetOneFrameTimeout, [0x%08X]", 
					pCAM->serialNum, idx, i, nRet);
				if (MV_E_NODATA == nRet) {
					log_info("ERROR : CAM '%s' NO DATA.", pCAM->serialNum);
					pMVS->grabing = false;
					return -1;
				}
			}

		}

		idx++;
	}

	if (SUFFIX_PNG == gCSV->cfg.device_param.img_type) {
		pthread_cond_broadcast(&gCSV->png.cond_png);
	}

	pCALIB->groupDemarcate++;
	csv_xml_write_CalibParameters();

	pMVS->grabing = false;

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
	struct cam_spec_t *pCAM = NULL;
	struct pointcloud_param_t *pPC = &gCSV->cfg.pointcloud_param;

	if (pMVS->grabing) {
		return -1;
	}

	pMVS->grabing = true;

	// 13 高速光
	ret = csv_dlp_just_write(CMD_HIGH_SPEED);

	while (idx <= nFrames) {
		for (i = 0; i < pMVS->cnt_mvs; i++) {
			pCAM = &pMVS->Cam[i];
			if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
				continue;
			}

			memset(&pCAM->imageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
			nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
				pCAM->stParam.nCurValue, &pCAM->imageInfo, 3000);
			if (nRet == MV_OK) {
				log_info("OK : CAM '%s' [%d_%02d]: GetOneFrame[%d] %d x %d", pCAM->serialNum, idx, i, 
					pCAM->imageInfo.nFrameNum, pCAM->imageInfo.nWidth, pCAM->imageInfo.nHeight);

				if ((idx == nFrames)&&(i == 1)) {
					end_pc = 1;
				}
				memset(img_name, 0, 256);
				generate_image_filename(pPC->imgRoot, 1, idx, i, 
					gCSV->cfg.device_param.img_type, img_name);
				ret = save_image_to_file(&pCAM->imageInfo, pCAM->pHandle, pCAM->imgData, 
					pCAM->stParam.nCurValue, gCSV->cfg.device_param.img_type, img_name, end_pc);
			} else {
				log_info("ERROR : CAM '%s' [%d_%02d]: GetOneFrameTimeout, [0x%08X]", 
					pCAM->serialNum, idx, i, nRet);

				if (MV_E_NODATA == nRet) {
					log_info("ERROR : CAM '%s' NO DATA.", pCAM->serialNum);
					pMVS->grabing = false;
					return -1;
				}
			}

		}

		idx++;
	}

	if (SUFFIX_PNG == gCSV->cfg.device_param.img_type) {
		pthread_cond_broadcast(&gCSV->png.cond_png);
	} else {
		if (pPC->enable) {
			ret = point_cloud_calc();
		}
	}

	pMVS->grabing = false;

	return ret;
}

static void *csv_mvs_grab_loop (void *data)
{
	if (NULL == data) {
		log_info("ERROR : critical failed.");
		return NULL;
	}

	int ret = 0;
	struct csv_mvs_t *pMVS = (struct csv_mvs_t *)data;

	while (1) {
		pthread_mutex_lock(&pMVS->mutex_grab);
		ret = pthread_cond_wait(&pMVS->cond_grab, &pMVS->mutex_grab);
		pthread_mutex_unlock(&pMVS->mutex_grab);
		if (ret != 0) {
			log_err("ERROR : pthread_cond_wait %s", pMVS->name_grab);
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

	log_info("WARN : exit pthread %s", pMVS->name_grab);

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
		log_err("ERROR : mutex %s", pMVS->name_grab);
        return -1;
    }

    if (pthread_cond_init(&pMVS->cond_grab, NULL) != 0) {
		log_err("ERROR : cond %s", pMVS->name_grab);
        return -1;
    }

	ret = pthread_create(&pMVS->thr_grab, &attr, csv_mvs_grab_loop, (void *)pMVS);
	if (ret < 0) {
		log_err("ERROR : create pthread %s", pMVS->name_grab);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p)", pMVS->name_grab, pMVS->thr_grab);
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
		log_err("ERROR : pthread_cancel %s", pMVS->name_grab);
	} else {
		log_info("OK : cancel pthread %s", pMVS->name_grab);
	}

	ret = pthread_join(pMVS->thr_grab, &retval);

	return ret;
}


static void *csv_mvs_loop (void *data)
{
	if (NULL == data) {
		log_info("ERROR : critical failed.");
		return NULL;
	}

	int ret = 0, i = 0;
	struct csv_mvs_t *pMVS = (struct csv_mvs_t *)data;
	//sleep(5);	// waiting for stable.

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

static int csv_mvs_thread (struct csv_mvs_t *pMVS)
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

static int csv_mvs_thread_cancel (struct csv_mvs_t *pMVS)
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
	int ret = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;

	pMVS->cnt_mvs = 0;
	pMVS->name_mvs = NAME_THREAD_MVS;
	pMVS->name_grab = NAME_THREAD_GRAB;

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

