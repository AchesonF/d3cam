#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


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

		//log_hex(pData, 1024, "SN:%s", camSerialNumber);
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
			log_debug("[cam device %d]: ", i);
			pDevInfo = pDevList->pDeviceInfo[i];
			if (NULL == pDevInfo) {
				log_info("ERROR : cam device info");
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
			log_info("OK : create pthread 'cam%d' @ (%p)", idx, tid);
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

	return 0;
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

	return csv_mvs_thread(pMVS);
}

int csv_mvs_deinit (void)
{
	return csv_mvs_thread_cancel(&gCSV->mvs);
}

#ifdef __cplusplus
}
#endif

