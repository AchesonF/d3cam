#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

struct cam_spec_t Cam[TOTAL_CAMS];

/* 枚举相机 */
int csv_mvs_cams_enum (void)
{
	int nRet = MV_OK, i = 0;

	struct csv_mvs_t *pMVS = &gCSV->mvs;
	MV_CC_DEVICE_INFO_LIST *pDevList = &pMVS->stDeviceList;
	MV_CC_DEVICE_INFO *pDevInfo = NULL;
	struct cam_spec_t *pCAM = NULL;

	memset(pDevList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

	//nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE|MV_USB_DEVICE, pDevList);
	nRet = MV_CC_EnumDevices(MV_USB_DEVICE, pDevList);
	if (MV_OK != nRet){
		log_info("ERROR : EnumDevices failed. [0x%08X]", nRet);
		return -1;
	}

	if (pDevList->nDeviceNum > 0) {
		log_info("OK : found %d CAM devices.", pDevList->nDeviceNum);
	} else {
		log_info("WARN : NO CAM devices.");
	}

	pMVS->cnt_mvs = pDevList->nDeviceNum;

	if (pDevList->nDeviceNum > TOTAL_CAMS) {
		pMVS->cnt_mvs = TOTAL_CAMS;
		log_info("only use first %d CAM devices.", TOTAL_CAMS);
	}

	for (i = 0; i < pMVS->cnt_mvs; i++){
		pDevInfo = pDevList->pDeviceInfo[i];
		if (NULL == pDevInfo) {
			break;
		}

		pCAM = &Cam[i];

		switch (pDevInfo->nTLayerType) {
		case MV_USB_DEVICE: {
			MV_USB3_DEVICE_INFO *pU3Info = &pDevInfo->SpecialInfo.stUsb3VInfo;

			sprintf(pCAM->modelName, "%s", (char *)pU3Info->chModelName);
			sprintf(pCAM->serialNum, "%s", (char *)pU3Info->chSerialNumber);

			log_info("OK : UserDefinedName[%d] %s", i, (char *)pU3Info->chUserDefinedName);
		}
			break;
		case MV_GIGE_DEVICE: {
			MV_GIGE_DEVICE_INFO *pGgInfo = &pDevInfo->SpecialInfo.stGigEInfo;
			sprintf(pCAM->modelName, "%s", (char *)pGgInfo->chModelName);
			sprintf(pCAM->serialNum, "%s", (char *)pGgInfo->chSerialNumber);

			log_info("OK : UserDefinedName[%d] %s", i, (char *)pGgInfo->chUserDefinedName);
		}
			break;
		default:
			log_info("WARN : [%d] not support type : 0x%08X", i, pDevInfo->nTLayerType);
			break;
		}
	}
    
	return pMVS->cnt_mvs;
}

int csv_mvs_cams_reset (void)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM = NULL;

	for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->cameraHandle)) {
			errNum++;
			continue;
		}

		nRet = MV_CC_SetCommandValue(pCAM->cameraHandle, "DeviceReset");
		nRet = MV_CC_CloseDevice(pCAM->cameraHandle);
		if (MV_OK != nRet) {
			log_info("ERROR : '%s' CloseDevice failed. [0x%08X]", pCAM->serialNum, nRet);
			errNum++;
			continue;
        }

		nRet = MV_CC_DestroyHandle(pCAM->cameraHandle);
		if (MV_OK != nRet) {
			log_info("ERROR : '%s' DestroyHandle failed. [0x%08X]", pCAM->serialNum, nRet);
			errNum++;
		} else {
			pCAM->cameraHandle = NULL;
			pCAM->opened = false;
		}
	}

	if (errNum > 0) {
		return -1;
	}

	return 0;
}

int csv_mvs_cams_open (void)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;

	struct csv_mvs_t *pMVS = &gCSV->mvs;
	MV_CC_DEVICE_INFO_LIST *pDevList = &pMVS->stDeviceList;
	MV_CC_DEVICE_INFO *pDevInfo = NULL;
	struct cam_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pDevInfo = pDevList->pDeviceInfo[i];
		if (NULL == pDevInfo) {
			break;
		}

		pCAM = &Cam[i];
		if (pCAM->opened) {
			continue;
		}

		log_info("Opening '%s' : '%s'", pCAM->modelName, pCAM->serialNum);

		nRet = MV_CC_CreateHandle(&pCAM->cameraHandle, pDevInfo);
		if (MV_OK != nRet) {
			pCAM->cameraHandle = NULL;
			log_info("ERROR : '%s' CreateHandle failed. [0x%08X]", pCAM->serialNum, nRet);
			continue;
		}

		pCAM->opened = true;

		nRet = MV_CC_OpenDevice(pCAM->cameraHandle, MV_ACCESS_Exclusive, 0);
		if (MV_OK != nRet) {
			MV_CC_DestroyHandle(pCAM->cameraHandle);
			pCAM->opened = false;
			errNum++;
			log_info("ERROR : '%s' OpenDevice failed. [0x%08X]", pCAM->serialNum, nRet);
			continue;
		}

        // 设置最佳包长（针对网口）
        if (pDevInfo->nTLayerType == MV_GIGE_DEVICE) {
            int nPacketSize = MV_CC_GetOptimalPacketSize(pCAM->cameraHandle);
            if (nPacketSize > 0) {
                nRet = MV_CC_SetIntValue(pCAM->cameraHandle, "GevSCPSPacketSize", nPacketSize);
            }
        }
        
        // 获取相机的曝光时间
		nRet = MV_CC_GetFloatValue(pCAM->cameraHandle, "ExposureTime", &pCAM->exposureTime);
		if (MV_OK != nRet){
			errNum++;
		}

		// 获取相机的增益数据
		nRet = MV_CC_GetFloatValue(pCAM->cameraHandle, "Gain", &pCAM->gain);
		if (MV_OK != nRet){
			errNum++;
		}

		// todo
		nRet = MV_CC_SetFloatValue(pCAM->cameraHandle, "ExposureTime", 20000);
		if (MV_OK == nRet) {
			log_info("OK : set ExposureTime : %f", 20000);
		} else{
			errNum++;
		}

		// 设置抓取图片的像素格式
		uint32_t enValue = PixelType_Gvsp_Mono8;	// 黑白相机
		if (strstr(pCAM->modelName, "UC") != NULL){	// 彩色相机
			enValue = PixelType_Gvsp_RGB8_Packed;
		}

		nRet = MV_CC_SetPixelFormat(pCAM->cameraHandle, enValue);
		if (MV_OK != nRet) {
			if (enValue == PixelType_Gvsp_RGB8_Packed) {
				log_info("ERROR : SetPixelFormat failed PixelType_Gvsp_RGB8_Packed");
			} else if (enValue == PixelType_Gvsp_Mono8) {
				log_info("ERROR : SetPixelFormat failed PixelType_Gvsp_Mono8");
			}
		}

		nRet = MV_CC_SetBoolValue(pCAM->cameraHandle, "AcquisitionFrameRateEnable", false);
		if (MV_OK != nRet){
			log_info("ERROR : AcquisitionFrameRateEnable failed.[0x%08X]", nRet);
			errNum++;
		}

		int devicetype = RDM_LIGHT;

		// ROI 处理结构光图像不能被16整除的问题
		if (RDM_LIGHT != devicetype) {
			MVCC_INTVALUE struIntValue = {0};
			nRet = MV_CC_GetIntValue(pCAM->cameraHandle, "Width", &struIntValue);

			int img_cut_off = struIntValue.nCurValue%16;
			if (img_cut_off > 0) {	// 图像为非16Bytes对齐图像
			    nRet = MV_CC_SetIntValue(pCAM->cameraHandle, "OffsetX", img_cut_off/2);
			    nRet = MV_CC_SetIntValue(pCAM->cameraHandle, "Width", struIntValue.nCurValue-img_cut_off);
			}

			if (MV_OK != nRet) {
				log_info("ERROR : SetOffsetX failed. [0x%08X]", nRet);
				errNum++;
			}
		}

		// 设置触发模式为 ON
		nRet = MV_CC_SetEnumValue(pCAM->cameraHandle, "TriggerMode", MV_TRIGGER_MODE_ON);
		if (MV_OK != nRet){
			log_info("ERROR : SetTriggerMode failed. [0x%08X]", nRet);
			errNum++;
		}

		// 设置触发源 line0
		nRet = MV_CC_SetEnumValue(pCAM->cameraHandle, "TriggerSource", MV_TRIGGER_SOURCE_LINE0);
		if (MV_OK != nRet){
			log_info("ERROR : SetTriggerSource failed. [0x%08X]", nRet);
			errNum++;
		}

        // 准备开始取数据流
		nRet = MV_CC_StartGrabbing(pCAM->cameraHandle);
		if (MV_OK != nRet){
			log_info("ERROR : StartGrabbing failed. [0x%08X]", nRet);
			errNum++;
        }
    }

	if (errNum > 0){
		return -1;
	}

	return 0;
}

int csv_mvs_cams_close (void)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM = NULL;

	for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->cameraHandle)) {
			//errNum++;
			continue;
		}

		nRet = MV_CC_CloseDevice(pCAM->cameraHandle);
		if (MV_OK != nRet) {
			log_info("ERROR : '%s' CloseDevice failed. [0x%08X]", pCAM->serialNum, nRet);
			errNum++;
		}

		nRet = MV_CC_DestroyHandle(pCAM->cameraHandle);
		if (MV_OK != nRet) {
			log_info("ERROR : '%s' DestroyHandle failed. [0x%08X]", pCAM->serialNum, nRet);
			errNum++;
		}

		pCAM->cameraHandle = NULL;
		pCAM->opened = false;
    }

	if (errNum > 0) {
		return -1;
	}

	return 0;
}

/* 获取相机曝光参数 */
int csv_mvs_cams_exposure_get (void)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->cameraHandle)) {
			errNum++;
			continue;
		}

		nRet = MV_CC_GetFloatValue(pCAM->cameraHandle, "ExposureTime", &pCAM->exposureTime);
		if (MV_OK == nRet){
			log_info("OK：'%s' get ExposureTime : %f [%f, %f]", pCAM->serialNum, 
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
int csv_mvs_cams_exposure_set (float fExposureTime)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->cameraHandle)) {
			errNum++;
			continue;
		}

		if (pCAM->exposureTime.fMax < fExposureTime) {
			fExposureTime = pCAM->exposureTime.fMax;
		}

		if (pCAM->exposureTime.fMin > fExposureTime) {
			fExposureTime = pCAM->exposureTime.fMin;
		}

		nRet = MV_CC_SetFloatValue(pCAM->cameraHandle, "ExposureTime", fExposureTime);
		if (MV_OK == nRet) {
			log_info("OK : '%s' set ExposureTime : %f", pCAM->serialNum, fExposureTime);
		} else {
			log_info("ERROR : '%s' set ExposureTime failed. [0x%08X]", pCAM->serialNum, nRet);
			errNum++;
		}
	}

	if (errNum > 0) {
		return -1;
	}

	return 0;
}

/* 获取相机增益参数 */
int csv_mvs_cams_gain_get (void)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->cameraHandle)) {
			errNum++;
			continue;
		}

		nRet = MV_CC_GetFloatValue(pCAM->cameraHandle, "Gain", &pCAM->gain);
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
int csv_mvs_cams_gain_set (float fGain)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->cameraHandle)) {
			errNum++;
			continue;
		}

		if (pCAM->gain.fMax < fGain) {
			fGain = pCAM->gain.fMax;
		}

		if (pCAM->gain.fMin > fGain) {
			fGain = pCAM->gain.fMin;
		}

		nRet = MV_CC_SetFloatValue(pCAM->cameraHandle, "Gain", fGain);
		if (MV_OK != nRet){
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
int csv_mvs_cams_name_set (char *camSNum, char *strValue)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->cameraHandle)) {
			errNum++;
			continue;
		}

		if (strstr(pCAM->serialNum, camSNum)) {
			nRet = MV_CC_SetStringValue(pCAM->cameraHandle, "DeviceUserID", (char*)strValue);
			if (MV_OK == nRet){
				log_info("OK ： '%s' set DeviceUserID.", pCAM->serialNum);

				return 0;
            } else {
				log_info("ERROR : '%s' set DeviceUserID Failed. [0x%08X]", pCAM->serialNum, nRet);

				return -1;
			}
		}
	}

	log_info("WARN : '%s' not found.", camSNum);

	return -1;
}

/* 抓取左右相机的图片 */
int csv_mvs_cams_grab_both (void)
{
	int nRet = MV_OK, i = 0;
	int errNum = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM = &Cam[0];
	MVCC_INTVALUE stParam;

	memset(&stParam, 0, sizeof(MVCC_INTVALUE));

	// 前提是必须保证两个相机是一样的参数
	nRet = MV_CC_GetIntValue(pCAM->cameraHandle, "PayloadSize", &stParam);
	if (MV_OK != nRet) {
		log_info("ERROR : '%s' get PayloadSize failed. [0x%08X]", pCAM->serialNum, nRet);
		return -1;
	}

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->cameraHandle)) {
			errNum++;
			continue;
		}

		if (pCAM->imgData != NULL) {
			memset(pCAM->imgData, 0x00, stParam.nCurValue);
		} else {
			pCAM->imgData = (uint8_t *)malloc(stParam.nCurValue);
			if (pCAM->imgData == NULL){
				log_err("ERROR : malloc imgData");
				return -1;
			}
		}

		memset(&pCAM->imageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
		nRet = MV_CC_GetOneFrameTimeout(pCAM->cameraHandle, pCAM->imgData, 
			stParam.nCurValue, &pCAM->imageInfo, 3000);
		if (nRet == MV_OK) {
			log_info("OK : '%s' GetOneFrame[%d] %d x %d", pCAM->serialNum, 
				pCAM->imageInfo.nFrameNum, pCAM->imageInfo.nWidth, pCAM->imageInfo.nHeight);
		} else {
			errNum++;
		}
	}

	if (errNum > 0) {
		return -1;
	}

	return 0;
}


static int csv_mvs_show_device_info (MV_CC_DEVICE_INFO *pDevInfo)
{
	if (NULL == pDevInfo) {
		return -1;
	}

	switch (pDevInfo->nTLayerType) {
	case MV_GIGE_DEVICE:
	{
		MV_GIGE_DEVICE_INFO *pstGigEInfo = &pDevInfo->SpecialInfo.stGigEInfo;
        log_debug("Model Name: %s", pstGigEInfo->chModelName);
	}
		break;
	case MV_USB_DEVICE:
	{
		MV_USB3_DEVICE_INFO *pstUsb3VInfo = &pDevInfo->SpecialInfo.stUsb3VInfo;
        log_debug("Model Name: %s", pstUsb3VInfo->chModelName);
	}
		break;
	default:
		log_info("ERROR : Not support layer[%d].", pDevInfo->nTLayerType);
		break;
	}

	return 0;
}

static void csv_mvs_end_grab (void *pUser)
{
	int nRet = MV_OK;

	// 停止取流
	nRet = MV_CC_StopGrabbing(pUser);
	if (MV_OK != nRet) {
		log_info("ERROR : MV_CC_StopGrabbing failed. 0x%08", nRet);
	}

	// 关闭设备
	nRet = MV_CC_CloseDevice(pUser);
	if (MV_OK != nRet) {
		log_info("ERROR : MV_CC_CloseDevice failed. 0x%08", nRet);
	}

	// 销毁句柄
	nRet = MV_CC_DestroyHandle(pUser);
	if (MV_OK != nRet) {
		log_info("ERROR : MV_CC_DestroyHandle failed. 0x%08", nRet);
	}
}

static void *csv_mvs_cam_loop (void *data)
{
	int nRet = MV_OK;
	int *index = (int *)data;
	int idx = *index;

	struct csv_mvs_t *pMVS = &gCSV->mvs;
	void* handle = NULL;
	//void **handle = &pMVS->handle[idx];
	MV_CC_DEVICE_INFO *pDevInfo = pMVS->stDeviceList.pDeviceInfo[idx];

	nRet = MV_CC_CreateHandle(&handle, pDevInfo);
	if (MV_OK != nRet) {
		log_info("ERROR : MV_CC_CreateHandle Cam[%d] failed. 0x%08X", idx, nRet);
		MV_CC_DestroyHandle(handle);
		goto exit;
	}

	nRet = MV_CC_OpenDevice(handle, MV_ACCESS_Exclusive, 0);
	if (MV_OK != nRet) {
		log_info("ERROR : MV_CC_OpenDevice Cam[%d] failed. 0x%08X", idx, nRet);
		MV_CC_DestroyHandle(handle);
		goto exit;
	}

	// 设置触发模式为off
	nRet = MV_CC_SetEnumValue(handle, "TriggerMode", MV_TRIGGER_MODE_OFF);
	if (MV_OK != nRet) {
		log_info("ERROR : MV_CC_SetTriggerMode Cam[%d] failed. 0x%08X", idx, nRet);
		goto out;
	}


	nRet = MV_CC_SetExposureAutoMode(handle, MV_EXPOSURE_AUTO_MODE_CONTINUOUS);
	if (MV_OK != nRet) {
		log_info("ERROR : MV_CC_SetExposure Cam[%d] failed. 0x%08X", idx, nRet);
		goto out;
	}


	// 开始取流
	nRet = MV_CC_StartGrabbing(handle);
	if (MV_OK != nRet) {
		log_info("ERROR : MV_CC_StartGrabbing Cam[%d] failed. 0x%08X", idx, nRet);
		goto out;
	}


	MVCC_STRINGVALUE stStringValue = {0};
	char camSerialNumber[256] = {0};
	nRet = MV_CC_GetStringValue(handle, "DeviceSerialNumber", &stStringValue);
	if (MV_OK == nRet) {
		memcpy(camSerialNumber, stStringValue.chCurValue, sizeof(stStringValue.chCurValue));
	}

	// ch:获取数据包大小
	MVCC_INTVALUE stParam;
	memset(&stParam, 0, sizeof(MVCC_INTVALUE));
	nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
	if (MV_OK != nRet) {
		log_info("ERROR : Get PayloadSize failed, 0x%08X", nRet);
		goto out;
	}

	MV_FRAME_OUT_INFO_EX stImageInfo = {0};
	memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
	uint8_t *pData = (uint8_t *)malloc(stParam.nCurValue);
	if (NULL == pData) {
		log_info("ERROR : malloc");
		goto out;
	}

	while(1) {
		if(gCSV->mvs.bExit) {
			break;
		}
		
		nRet = MV_CC_GetOneFrameTimeout(handle, pData, stParam.nCurValue, &stImageInfo, 1000);
		if (MV_OK != nRet) {
			log_info("ERROR : Cam SN:%s failed 0x%08X", camSerialNumber, nRet);
			continue;
		}

		log_debug("Cam SN:%s Frame[%d] = %d x %d", camSerialNumber, 
			stImageInfo.nFrameNum, stImageInfo.nWidth, stImageInfo.nHeight);

		//log_hex(STREAM_DATA, pData, 1024, "SN:%s", camSerialNumber);
		//break;
		sleep(1);

unsigned char *pDataForSaveImage = NULL;
char img_filename[64] = {0};
memset(img_filename, 0, 64);
snprintf(img_filename, 64, "SN_%s_%ld.bmp", camSerialNumber, utility_get_millisecond());

pDataForSaveImage = (unsigned char*)malloc(stImageInfo.nWidth * stImageInfo.nHeight * 4 + 2048);
if (NULL == pDataForSaveImage) {
	break;
}
// 填充存图参数
// fill in the parameters of save image
MV_SAVE_IMAGE_PARAM_EX stSaveParam;
memset(&stSaveParam, 0, sizeof(MV_SAVE_IMAGE_PARAM_EX));
// 从上到下依次是：输出图片格式，输入数据的像素格式，提供的输出缓冲区大小，图像宽，
// 图像高，输入数据缓存，输出图片缓存，JPG编码质量
// Top to bottom are：
stSaveParam.enImageType = MV_Image_Bmp; 
stSaveParam.enPixelType = stImageInfo.enPixelType; 
stSaveParam.nBufferSize = stImageInfo.nWidth * stImageInfo.nHeight * 4 + 2048;
stSaveParam.nWidth      = stImageInfo.nWidth; 
stSaveParam.nHeight     = stImageInfo.nHeight; 
stSaveParam.pData       = pData;
stSaveParam.nDataLen    = stImageInfo.nFrameLen;
stSaveParam.pImageBuffer = pDataForSaveImage;
stSaveParam.nJpgQuality = 80;

nRet = MV_CC_SaveImageEx2(handle, &stSaveParam);
if(MV_OK != nRet) {
    printf("failed in MV_CC_SaveImage,nRet[%x]\n", nRet);
    break;
}

FILE* fp = fopen(img_filename, "wb");
if (NULL == fp) {
    printf("fopen failed\n");
    break;
}

fwrite(pDataForSaveImage, 1, stSaveParam.nImageLen, fp);
fclose(fp);
printf("save image succeed\n");

break;
    }

out:

	csv_mvs_end_grab(handle);

exit:

	log_info("OK : exit thread");

	pthread_exit(NULL);
}

static int csv_mvs_device_prepare (struct csv_mvs_t *pMVS)
{
	int nRet = 0, i = 0;
	MV_CC_DEVICE_INFO_LIST	*pDevList = &pMVS->stDeviceList;
	MV_CC_DEVICE_INFO *pDevInfo = NULL;

	memset(pDevList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

	// enum device
	nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, pDevList);
	if (MV_OK != nRet) {
		log_debug("MV_CC_EnumDevices fail! nRet [%x]", nRet);
		return -1;
	}

	if (pDevList->nDeviceNum > 0) {
		log_info("Found Cam nDeviceNum : %d", pDevList->nDeviceNum);
		for (i = 0; i < pDevList->nDeviceNum; i++) {
			log_debug("[CAM device %d]: ", i);
			pDevInfo = pDevList->pDeviceInfo[i];
			if (NULL == pDevInfo) {
				log_info("ERROR : CAM device info");
				return -2;
			}
			csv_mvs_show_device_info(pDevInfo);
		}
	} else {
		log_info("No Cam Devices!");
		return -3;
	}

	pMVS->cnt_mvs = pDevList->nDeviceNum;

	// todo sort

	return 0;
}

static void *csv_mvs_loop (void *data)
{
	if (NULL == data) {
		return NULL;
	}

	int ret = 0;
    int i = 0;

	struct csv_mvs_t *pMVS = (struct csv_mvs_t *)data;

	do {
		sleep(2);	// waiting for stable.
		ret = csv_mvs_device_prepare(pMVS);
		if (ret < 0) {
			goto wait;
		}


		// TODO 不开线程处理

		pMVS->bExit = 0;
		for (i = 0; i < pMVS->cnt_mvs; i++) {
			pthread_t tid;

			int idx = i;
			ret = pthread_create(&tid, NULL, csv_mvs_cam_loop, &idx);
			if (ret != 0) {
				log_err("ERROR : pthread_create Cam[%d] failed. ret = %d", idx, ret);
				continue;
			}
			log_info("OK : create pthread 'CAM%d' @ (%p)", idx, tid);
		}

wait:

		ret = pthread_cond_wait(&pMVS->cond_mvs, &pMVS->mutex_mvs);
		if (ret != 0) {
			log_err("ERROR : pthread_cond_wait");
			break;
		}

		pMVS->bExit = 1;
		// todo exit work threads / refresh threads
	} while (1);

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
	struct csv_mvs_t *pMVS = &gCSV->mvs;

	pMVS->cnt_mvs = 0;
	pMVS->bExit = 0;
	pMVS->name_mvs = NAME_THREAD_MVS;

	//return csv_mvs_thread(pMVS);
	return 0;
}

int csv_mvs_deinit (void)
{
	int i = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM = NULL;

    for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &Cam[i];

		if (NULL != pCAM->imgData) {
			free(pCAM->imgData);
			pCAM->imgData = NULL;
		}
	}

	//return csv_mvs_thread_cancel(&gCSV->mvs);
	return 0;
}

#ifdef __cplusplus
}
#endif

