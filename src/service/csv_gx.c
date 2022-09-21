#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(USE_GX_CAMS)

#define GX_WAIT_TIMEO		(6)		// x
#define GX_WAIT_PERIOD		(2)		// 2s

static void GetErrorString (GX_STATUS emErrorStatus)
{
	char *error_info = NULL;
	size_t size = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	// Get length of error description
	emStatus = GXGetLastError(&emErrorStatus, NULL, &size);
	if (emStatus != GX_STATUS_SUCCESS) {
		log_info("ERROR : GXGetLastError");
		return;
	}

	if (size <= 0) {
		return;
	}

	// Alloc error resources
	error_info = malloc(size);
	if (NULL == error_info) {
		log_err("ERROR : malloc errorinfo");
		return ;
	}

	// Get error description
	emStatus = GXGetLastError(&emErrorStatus, error_info, &size);
	if (emStatus != GX_STATUS_SUCCESS) {
		log_info("ERROR : GXGetLastError");
	} else {
		log_info("%s", error_info);
	}

	// Realease error resources
	if (NULL != error_info) {
		free(error_info);
		error_info = NULL;
	}
}

static GX_STATUS GxGetString (GX_DEV_HANDLE hDevice, GX_FEATURE_ID id, char *dst)
{
	size_t nSize = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if ((NULL == dst)||(NULL == hDevice)) {
		return -1;
	}

    emStatus = GXGetStringLength(hDevice, id, &nSize);
	if (emStatus != GX_STATUS_SUCCESS) {
		GetErrorString(emStatus);
		return emStatus;
	}

	char *str = malloc(nSize);
	if (NULL == str) {
		log_err("ERROR : malloc str");
		return -1;
	}

	emStatus = GXGetString(hDevice, id, str, &nSize);
	if (emStatus != GX_STATUS_SUCCESS) {
		GetErrorString(emStatus);
		goto out;
	}

	memset(dst, 0, strlen(dst));
	strncpy(dst, str, strlen(dst));

out:

	if (NULL != str) {
		free(str);
		str = NULL;
	}

	return emStatus;
}

// Convert frame date to suitable pixel format
int PixelFormatConvert(PGX_FRAME_BUFFER pFrameBuffer, uint8_t *ImageBuf, int64_t PayloadSize)
{
	VxInt32 emDXStatus = DX_OK;

	// Convert RAW8 or RAW16 image to RGB24 image
	switch (pFrameBuffer->nPixelFormat) {
	case GX_PIXEL_FORMAT_MONO8: {
		memcpy(ImageBuf, pFrameBuffer->pImgBuf, PayloadSize);
#if 1
		char rawfile[128] = {0};
		char bmpfile[128] = {0};
		uint64_t stamp = utility_get_millisecond();
		memset(rawfile, 0, 128);
		memset(bmpfile, 0, 128);
		snprintf(rawfile, 128, "./data/%d_%d_%ld.raw", pFrameBuffer->nWidth, 
			pFrameBuffer->nHeight, stamp);
		snprintf(bmpfile, 128, "./data/%d_%d_%ld.bmp", pFrameBuffer->nWidth, 
			pFrameBuffer->nHeight, stamp);
		csv_file_write_data(rawfile, (uint8_t *)pFrameBuffer->pImgBuf, (uint32_t)PayloadSize);
		gray_raw2bmp((uint8_t *)pFrameBuffer->pImgBuf, pFrameBuffer->nWidth, 
			pFrameBuffer->nHeight, bmpfile);
#endif
		}
		break;

	case GX_PIXEL_FORMAT_MONO10:
	case GX_PIXEL_FORMAT_MONO12:
		// Convert to the Raw8 image
		emDXStatus = DxRaw16toRaw8((uint8_t *)pFrameBuffer->pImgBuf, 
			ImageBuf, pFrameBuffer->nWidth, pFrameBuffer->nHeight, DX_BIT_2_9);
		if (emDXStatus != DX_OK) {
			log_warn("ERROR : DxRaw16toRaw8 errcode[%d]", emDXStatus);
			return -1;
		}
		break;

	default:
		log_debug("ERROR : PixelFormat [%d] not supported.", pFrameBuffer->nPixelFormat);
		return -1;
    }

	return 0;
}

// Save PPM image
int SavePPMFile (uint32_t ui32Width, uint32_t ui32Height)
{
/*
	char szName[64] = {0};
	static int nRawFileIndex = 0;

	if (g_pMonoImageBuf != NULL) {
		FILE* phImageFile = NULL;
		snprintf(szName, 64, "Frame_%d.ppm", nRawFileIndex++);
		phImageFile = fopen(szName, "wb");
		if (phImageFile == NULL) {
		    printf("Create or Open %s failed!\n", szName);
		    return;
		}
		fprintf(phImageFile, "P5\n%u %u 255\n", ui32Width, ui32Height);
		fwrite(g_pMonoImageBuf, 1, g_nPayloadSize, phImageFile);
		fclose(phImageFile);
		phImageFile = NULL;
		printf("Save %s succeed\n", szName);
    }
*/
	return 0;
}

static int csv_gx_lib (uint8_t action)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (GX_LIB_OPEN == action) {
		emStatus = GXInitLib();
		log_info("Galaxy library version : %s", GXGetLibVersion());
	} else if (GX_LIB_CLOSE == action) {
		emStatus = GXCloseLib();
	}

	if(emStatus != GX_STATUS_SUCCESS) {
		GetErrorString(emStatus);
		return -1;
	}

	return 0;
}

static int csv_gx_search (struct csv_gx_t *pGX)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	uint32_t ui32DeviceNum = 0;

	pGX->cnt_gx = 0;

	//Get device enumerated number
	emStatus = GXUpdateDeviceList(&ui32DeviceNum, 1000);
	if (emStatus != GX_STATUS_SUCCESS) {
		GetErrorString(emStatus);
		return 0;
	}

	if (ui32DeviceNum > 0) {
		log_info("OK : Found %d CAM device(s).", ui32DeviceNum);
	} else {
    	log_warn("WARN : NO CAM device found.");
		return 0;
	}

	pGX->cnt_gx = ui32DeviceNum;

	if (ui32DeviceNum > TOTAL_CAMS) {
		pGX->cnt_gx = TOTAL_CAMS;
		log_notice("Only USE first %d CAM device(s).", TOTAL_CAMS);
	}

	return pGX->cnt_gx;
}

static int csv_gx_open (struct csv_gx_t *pGX)
{
	int i = 0, ret = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	struct cam_gx_spec_t *pCAM = NULL;

	for (i = 0; i < pGX->cnt_gx; i++) {
		pCAM = &pGX->Cam[i];
		if ((NULL == pCAM)||(pCAM->opened)) {
			continue;
		}

		// switch r <-> l
		if (gCSV->cfg.devicecfg.SwitchCams) {
			if (2 == pGX->cnt_gx) {
				if (i == 0) {
					emStatus = GXOpenDeviceByIndex(2, &pCAM->hDevice);
				} else if (i == 1) {
					emStatus = GXOpenDeviceByIndex(1, &pCAM->hDevice);
				}
			}
		} else {
			emStatus = GXOpenDeviceByIndex(i+1, &pCAM->hDevice);
		}

		if (emStatus != GX_STATUS_SUCCESS) {
			GetErrorString(emStatus);
			ret = -1;
			continue;
		}

		if (NULL == pCAM->hDevice) {
			log_info("cam[%d] open null", i);
			ret = -1;
			continue;
		}

		emStatus = GxGetString(pCAM->hDevice, GX_STRING_DEVICE_VENDOR_NAME, pCAM->vendor);

		emStatus = GxGetString(pCAM->hDevice, GX_STRING_DEVICE_MODEL_NAME, pCAM->model);

		emStatus = GxGetString(pCAM->hDevice, GX_STRING_DEVICE_SERIAL_NUMBER, pCAM->serial);

		emStatus = GxGetString(pCAM->hDevice, GX_STRING_DEVICE_VERSION, pCAM->version);

		emStatus = GxGetString(pCAM->hDevice, GX_STRING_DEVICE_USERID, pCAM->userid);

		log_info("CAM[%d] : '%s' - '%s' (%s).", i, pCAM->model, pCAM->serial, pCAM->userid);

		emStatus = GXIsImplemented(pCAM->hDevice, GX_ENUM_PIXEL_COLOR_FILTER, &pCAM->ColorFilter);
		if (emStatus == GX_STATUS_SUCCESS) {
			if (pCAM->ColorFilter) {
				log_info("CAM[%d] : ColorFilter %d is color camera, not support.", i, pCAM->ColorFilter);
				continue;
			}
		}

		emStatus = GXGetInt(pCAM->hDevice, GX_INT_PAYLOAD_SIZE, &pCAM->PayloadSize);

		pCAM->pMonoImageBuf = malloc(pCAM->PayloadSize + 1);
		if (NULL == pCAM->pMonoImageBuf) {
			log_err("ERROR : malloc MonoImageBuf");
			ret = -1;
			continue;
		}

		memset(pCAM->pMonoImageBuf, 0, pCAM->PayloadSize + 1);

		pCAM->opened = true;
	}

	if ((pGX->cnt_gx != TOTAL_CAMS)||(pGX->Cam[CAM_LEFT].PayloadSize != pGX->Cam[CAM_RIGHT].PayloadSize)) {
		log_warn("ERROR : CAMs not coupled. PayloadSize : %d vs %d.", 
			pGX->Cam[CAM_LEFT].PayloadSize, pGX->Cam[CAM_RIGHT].PayloadSize);
		return -1;
	}

	return ret;
}

static int csv_gx_close (struct csv_gx_t *pGX)
{
	int i = 0, ret = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	struct cam_gx_spec_t *pCAM = NULL;

	for (i = 0; i < pGX->cnt_gx; i++) {
		pCAM = &pGX->Cam[i];
		if ((NULL == pCAM)||(!pCAM->opened)||(NULL == pCAM->hDevice)) {
			continue;
		}

		emStatus = GXCloseDevice(pCAM->hDevice);
		if (emStatus != GX_STATUS_SUCCESS) {
			GetErrorString(emStatus);
			ret = -1;
			continue;
		}

		if (NULL != pCAM->pMonoImageBuf) {
			free(pCAM->pMonoImageBuf);
			pCAM->pMonoImageBuf = NULL;
		}

		pCAM->hDevice = NULL;
		pCAM->opened = false;
		pCAM->PayloadSize = 0;
		pCAM->ColorFilter = false;
		memset(pCAM->vendor, 0, SIZE_CAM_STR);
		memset(pCAM->model, 0, SIZE_CAM_STR);
		memset(pCAM->serial, 0, SIZE_CAM_STR);
		memset(pCAM->version, 0, SIZE_CAM_STR);
	}

	return ret;
}

int csv_gx_acquisition (struct csv_gx_t *pGX, uint8_t state)
{
	int i = 0, ret = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	struct cam_gx_spec_t *pCAM = NULL;

	for (i = 0; i < pGX->cnt_gx; i++) {
		pCAM = &pGX->Cam[i];
		if ((NULL == pCAM)||(!pCAM->opened)||(NULL == pCAM->hDevice)) {
			continue;
		}

		if (GX_ACQUISITION_START == state) {
			emStatus = GXStreamOn(pCAM->hDevice);
		} else if (GX_ACQUISITION_STOP == state) {
			emStatus = GXStreamOff(pCAM->hDevice);
		}

		if (emStatus != GX_STATUS_SUCCESS) {
			GetErrorString(emStatus);
			//ret = -1;
			continue;
		}
	}

	return ret;
}

static int csv_gx_cams_init (struct csv_gx_t *pGX)
{
	int i = 0, ret = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	struct cam_gx_spec_t *pCAM = NULL;
	bool bStreamTransferSize = false;
	bool bStreamTransferNumberUrb = false;

	for (i = 0; i < pGX->cnt_gx; i++) {
		pCAM = &pGX->Cam[i];
		if ((NULL == pCAM)||(!pCAM->opened)||(NULL == pCAM->hDevice)) {
			continue;
		}

//		emStatus = GXSetEnum(pCAM->hDevice, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_SINGLE_FRAME);

		emStatus = GXSetEnum(pCAM->hDevice, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON);

		emStatus = GXSetEnum(pCAM->hDevice, GX_ENUM_TRIGGER_ACTIVATION, GX_TRIGGER_ACTIVATION_RISINGEDGE);

		emStatus = GXSetEnum(pCAM->hDevice, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE0);

		emStatus = GXSetAcqusitionBufferNumber(pCAM->hDevice, ACQ_BUFFER_NUM);

		emStatus = GXSetEnum(pCAM->hDevice, GX_ENUM_EXPOSURE_MODE, GX_EXPOSURE_MODE_TRIGGERWIDTH);

		emStatus = GXIsImplemented(pCAM->hDevice, GX_DS_INT_STREAM_TRANSFER_SIZE, &bStreamTransferSize);
		if (bStreamTransferSize) {
			// Set size of data transfer block
			emStatus = GXSetInt(pCAM->hDevice, GX_DS_INT_STREAM_TRANSFER_SIZE, ACQ_TRANSFER_SIZE);
		}

		emStatus = GXIsImplemented(pCAM->hDevice, GX_DS_INT_STREAM_TRANSFER_NUMBER_URB, &bStreamTransferNumberUrb);
		if (bStreamTransferNumberUrb) {
			// Set qty. of data transfer block
			emStatus = GXSetInt(pCAM->hDevice, GX_DS_INT_STREAM_TRANSFER_NUMBER_URB, ACQ_TRANSFER_NUMBER_URB);
		}


	}

	return ret;
}

static int csv_gx_cams_deinit (struct csv_gx_t *pGX)
{

	return 0;
}

int csv_gx_get_frame (struct csv_gx_t *pGX, uint8_t idx)
{
	if (idx >= TOTAL_CAMS) {
		return -1;
	}

	int ret = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	struct cam_gx_spec_t *pCAM = &pGX->Cam[idx];

	if ((NULL == pCAM)||(!pCAM->opened)||(NULL == pCAM->hDevice)) {
		return -1;
	}

	emStatus = GXDQBuf(pCAM->hDevice, &pCAM->pFrameBuffer, 1000);
	if (emStatus != GX_STATUS_SUCCESS) {
		GetErrorString(emStatus);
		return -1;
	}

	if (GX_FRAME_STATUS_SUCCESS != pCAM->pFrameBuffer->nStatus) {
		log_warn("ERROR : Abnormal Acquisition %d", pCAM->pFrameBuffer->nStatus);
		return -1;
	}

	log_debug("OK : Frame [%s][%d].", pCAM->serial, pCAM->pFrameBuffer->nFrameID);

	ret = PixelFormatConvert(pCAM->pFrameBuffer, pCAM->pMonoImageBuf, pCAM->PayloadSize);
	if (0 == ret) {
		SavePPMFile(pCAM->pFrameBuffer->nWidth, pCAM->pFrameBuffer->nHeight);
	}

	emStatus = GXQBuf(pCAM->hDevice, pCAM->pFrameBuffer);
	if (emStatus != GX_STATUS_SUCCESS) {
		GetErrorString(emStatus);
		return -1;
	}

	return 0;
}


static void *csv_gx_loop (void *data)
{
	if (NULL == data) {
		log_warn("ERROR : critical failed.");
		return NULL;
	}

	int ret = 0, timeo = 0;
	struct csv_gx_t *pGX = (struct csv_gx_t *)data;

	csv_gx_lib(GX_LIB_OPEN);

	while (1) {
		do {
			sleep(GX_WAIT_PERIOD);
			ret = csv_gx_search(pGX);
			if (ret < TOTAL_CAMS) {
				log_info("Search times[%d] not enough cams.", timeo+1);
			}
		} while ((ret < TOTAL_CAMS)&&(++timeo < GX_WAIT_TIMEO));

		if (timeo >= GX_WAIT_TIMEO) {
			log_warn("ERROR : waiting too long time. Should plug off/on cams or reboot system.");
		}
		timeo = 0;

		if (ret > 0) {
			csv_gx_open(pGX);
			csv_gx_cams_init(pGX);
		}

		pthread_mutex_lock(&pGX->mutex_gx);
		ret = pthread_cond_wait(&pGX->cond_gx, &pGX->mutex_gx);
		pthread_mutex_unlock(&pGX->mutex_gx);
		if (ret != 0) {
			log_err("ERROR : pthread_cond_wait.");
			break;
		}

		csv_gx_cams_deinit(pGX);
		csv_gx_close(pGX);
		pGX->cnt_gx = 0;

	}

	log_alert("ALERT : exit pthread %s.", pGX->name_gx);

	pGX->thr_gx = 0;

	pthread_exit(NULL);

	return NULL;
}

static int csv_gx_thread (struct csv_gx_t *pGX)
{
	int ret = -1;

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pGX->mutex_gx, NULL) != 0) {
		log_err("ERROR : mutex %s.", pGX->name_gx);
        return -1;
    }

    if (pthread_cond_init(&pGX->cond_gx, NULL) != 0) {
		log_err("ERROR : cond %s.", pGX->name_gx);
        return -1;
    }

	ret = pthread_create(&pGX->thr_gx, &attr, csv_gx_loop, (void *)pGX);
	if (ret < 0) {
		log_err("ERROR : create pthread %s.", pGX->name_gx);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p).", pGX->name_gx, pGX->thr_gx);
	}

	return ret;
}

static int csv_gx_thread_cancel (struct csv_gx_t *pGX)
{
	int ret = 0;
	void *retval = NULL;

	if (pGX->thr_gx <= 0) {
		return 0;
	}

	csv_gx_cams_deinit(pGX);
	csv_gx_close(pGX);
	csv_gx_lib(GX_LIB_CLOSE);

	ret = pthread_cancel(pGX->thr_gx);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s.", pGX->name_gx);
	} else {
		log_info("OK : cancel pthread %s (%p).", pGX->name_gx, pGX->thr_gx);
	}

	ret = pthread_join(pGX->thr_gx, &retval);

	pGX->thr_gx = 0;

	return ret;
}


int csv_gx_init (void)
{
	int ret = 0;
	struct csv_gx_t *pGX = &gCSV->gx;

	pGX->cnt_gx = 0;
	pGX->name_gx = NAME_THREAD_GX;

	ret = csv_gx_thread(pGX);

	return ret;
}


int csv_gx_deinit (void)
{
	int ret = 0;
	struct csv_gx_t *pGX = &gCSV->gx;

	ret = csv_gx_thread_cancel(pGX);

	return ret;
}

#else

int csv_gx_init (void)
{
	return 0;
}

int csv_gx_deinit (void)
{
	return 0;
}

#endif

#ifdef __cplusplus
}
#endif





