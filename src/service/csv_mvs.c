#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAMERA_NUM             2

void PrintDeviceInfo (MV_CC_DEVICE_INFO *pstMVDevInfo)
{
    if (NULL == pstMVDevInfo) {
        printf("The Pointer of pstMVDevInfo is NULL!\n");
        return ;
    }

	switch (pstMVDevInfo->nTLayerType) {
	case MV_GIGE_DEVICE:
	{
		MV_GIGE_DEVICE_INFO *pstGigEInfo = &pstMVDevInfo->SpecialInfo.stGigEInfo;

        // ch:打印当前相机ip和用户自定义名字 | en:print current ip and user defined name
        printf("Device Model Name: %s\n", pstGigEInfo->chModelName);
        //printf("CurrentIp: %s\n" , inet_ntoa(swap32(pstGigEInfo->nCurrentIp)));
//        printf("UserDefinedName: %s\n\n" , pstGigEInfo->chUserDefinedName);
	}
		break;
	case MV_USB_DEVICE:
	{
		MV_USB3_DEVICE_INFO *pstUsb3VInfo = &pstMVDevInfo->SpecialInfo.stUsb3VInfo;
        printf("Device Model Name: %s\n", pstUsb3VInfo->chModelName);
//        printf("UserDefinedName: %s\n\n", pstUsb3VInfo->chUserDefinedName);
	}
		break;
	default:
		printf("Not support.\n");
		break;
	}

}

static void csv_grab_end (void *pUser)
{
	int nRet = MV_OK;

	// 停止取流
	nRet = MV_CC_StopGrabbing(pUser);
	if (MV_OK != nRet) {
		printf("MV_CC_StopGrabbing fail! nRet [%x]\n", nRet);
		return ;
	}

	// 关闭设备
	nRet = MV_CC_CloseDevice(pUser);
	if (MV_OK != nRet) {
		printf("MV_CC_CloseDevice fail! nRet [%x]\n", nRet);
		return ;
	}

	// 销毁句柄
	nRet = MV_CC_DestroyHandle(pUser);
	if (MV_OK != nRet) {
		printf("MV_CC_DestroyHandle fail! nRet [%x]\n", nRet);
		return ;
	}
}

static void *WorkThread (void* pUser)
{
	int nRet = MV_OK;

	MVCC_STRINGVALUE stStringValue = {0};
	char camSerialNumber[256] = {0};
	nRet = MV_CC_GetStringValue(pUser, "DeviceSerialNumber", &stStringValue);
	if (MV_OK == nRet) {
		memcpy(camSerialNumber, stStringValue.chCurValue, sizeof(stStringValue.chCurValue));
	} else {
		printf("Get DeviceUserID Failed! nRet = [%x]\n", nRet);
	}

	// ch:获取数据包大小
	MVCC_INTVALUE stParam;
	memset(&stParam, 0, sizeof(MVCC_INTVALUE));
	nRet = MV_CC_GetIntValue(pUser, "PayloadSize", &stParam);
	if (MV_OK != nRet) {
		printf("Get PayloadSize fail! nRet [0x%x]\n", nRet);
		return NULL;
	}

	MV_FRAME_OUT_INFO_EX stImageInfo = {0};
	memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
	unsigned char * pData = (unsigned char *)malloc(sizeof(unsigned char) * stParam.nCurValue);
	if (NULL == pData) {
		return NULL;
	}

	unsigned int nDataSize = stParam.nCurValue;

	while(1) {
		if(gCSV->mvs.bExit) {
			break;
		}
			
		nRet = MV_CC_GetOneFrameTimeout(pUser, pData, nDataSize, &stImageInfo, 1000);
		if (nRet == MV_OK) {
			printf("Cam Serial Number[%s]:GetOneFrame, Width[%d], Height[%d], nFrameNum[%d]\n", 
				camSerialNumber, stImageInfo.nWidth, stImageInfo.nHeight, stImageInfo.nFrameNum);
		} else {
			printf("cam[%s]:Get One Frame failed![%x]\n", camSerialNumber, nRet);
		}

		log_hex(pData, 1024, "SN:%s", camSerialNumber);

		break;
    }

	csv_grab_end(pUser);

	pthread_exit(NULL);
}

static void *csv_mvs_loop (void *data)
{
	if (NULL == data) {
		return NULL;
	}

	int ret = 0;
    int nRet = MV_OK, i = 0, ret_exit = 0;

	void* handle[CAMERA_NUM] = {NULL};
	struct csv_mvs_t *pMVS = (struct csv_mvs_t *)data;
	MV_CC_DEVICE_INFO_LIST stDeviceList;
	MV_CC_DEVICE_INFO *pDeviceInfo = NULL;

	memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

	do {
		// enum device
		nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
		if (MV_OK != nRet) {
			log_debug("MV_CC_EnumDevices fail! nRet [%x]", nRet);
			ret_exit = -1;
		    break;
		}

		log_info("nDeviceNum %d", stDeviceList.nDeviceNum);

		if (stDeviceList.nDeviceNum > 0) {
			for (i = 0; i < stDeviceList.nDeviceNum; i++) {
				log_info("[device %d]: ", i);
				pDeviceInfo = stDeviceList.pDeviceInfo[i];
				if (NULL == pDeviceInfo) {
					ret_exit = -1;
					break;
				}
				PrintDeviceInfo(pDeviceInfo);
			}
		} else {
			printf("Find No Devices!\n");
			break;
		}

		if (stDeviceList.nDeviceNum < CAMERA_NUM) {
			printf("only have %d camera\n", stDeviceList.nDeviceNum);
			ret_exit = -2;
			goto exit;
		}

		for (i = 0; i < CAMERA_NUM; i++) {
			nRet = MV_CC_CreateHandle(&handle[i], stDeviceList.pDeviceInfo[i]);
			if (MV_OK != nRet) {
				printf("MV_CC_CreateHandle fail! nRet [%x]\n", nRet);
				MV_CC_DestroyHandle(handle[i]);
				ret_exit = -3;
				goto exit;
			}

			nRet = MV_CC_OpenDevice(handle[i], MV_ACCESS_Exclusive, 0);
			if (MV_OK != nRet) {
				printf("MV_CC_OpenDevice fail! nRet [%x]\n", nRet);
				MV_CC_DestroyHandle(handle[i]);
				ret_exit = -4;
				goto exit;
			}

			// ch:探测网络最佳包大小(只对GigE相机有效) 
			if (stDeviceList.pDeviceInfo[i]->nTLayerType == MV_GIGE_DEVICE) {
				int nPacketSize = MV_CC_GetOptimalPacketSize(handle[i]);
				if (nPacketSize > 0) {
 					nRet = MV_CC_SetIntValue(handle[i],"GevSCPSPacketSize",nPacketSize);
					if (nRet != MV_OK) {
						printf("Warning: Set Packet Size fail nRet [0x%x]!\n", nRet);
					}
				} else {
					printf("Warning: Get Packet Size fail nRet [0x%x]!\n", nPacketSize);
				}
			}
		}

		for (i = 0; i < CAMERA_NUM; i++) {
			// 设置触发模式为off
			nRet = MV_CC_SetEnumValue(handle[i], "TriggerMode", MV_TRIGGER_MODE_OFF);
			if (MV_OK != nRet) {
				printf("Cam[%d]: MV_CC_SetTriggerMode fail! nRet [%x]\n", i, nRet);
			}

				// 开始取流
			nRet = MV_CC_StartGrabbing(handle[i]);
			if (MV_OK != nRet) {
				printf("Cam[%d]: MV_CC_StartGrabbing fail! nRet [%x]\n",i, nRet);
				ret_exit = -5;
				goto exit;
			}

			pthread_t nThreadID;
			nRet = pthread_create(&nThreadID, NULL, WorkThread, handle[i]);
			if (nRet != 0) {
				printf("Cam[%d]: thread create failed.ret = %d\n",i, nRet);
				ret_exit = -6;
				goto exit;
			}
		}


		ret = pthread_cond_wait(&pMVS->cond_mvs, &pMVS->mutex_mvs);
		if (ret != 0) {
			log_err("ERROR : pthread_cond_wait");
			break;
		}

		// todo exit work threads / refresh threads
	} while (1);

exit:

	log_info("WARN : exit pthread %s (%d)", pMVS->name_mvs, ret_exit);
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

	return 0;
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


	return 0;
}

#ifdef __cplusplus
}
#endif

