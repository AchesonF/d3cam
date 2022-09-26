#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(USE_GX_CAMS)

bool libInit = false;

#define GX_WAIT_TIMEO		(6)		// x
#define GX_WAIT_PERIOD		(2)		// 2s

static void GetErrorString (GX_STATUS emErrorStatus)
{
	char *error_info = NULL;
	size_t size = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (!libInit) {
		return;
	}

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
	error_info = malloc(size+1);
	if (NULL == error_info) {
		log_err("ERROR : malloc errorinfo");
		return ;
	}

	memset(error_info, 0, size+1);

	// Get error description
	emStatus = GXGetLastError(&emErrorStatus, error_info, &size);
	if (emStatus != GX_STATUS_SUCCESS) {
		log_info("ERROR : GXGetLastError");
	} else {
		log_info("\"%s\".", error_info);
	}

	// Realease error resources
	if (NULL != error_info) {
		free(error_info);
		error_info = NULL;
	}
}

static GX_STATUS GetFeatureName (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, 
	char *pszName, size_t *pnSize)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXGetFeatureName(hDevice, emFeatureID, pszName, pnSize);
		if (GX_STATUS_SUCCESS != emStatus) {
			log_info("ERROR : GXGetFeatureName '%s' errcode[%d].",pszName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

static GX_STATUS IsImplemented (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, bool *pbIsImplemented)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXIsImplemented(hDevice, emFeatureID, pbIsImplemented);
		if (GX_STATUS_SUCCESS != emStatus) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXIsImplemented '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS GetIntRange (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, GX_INT_RANGE *pIntRange)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXGetIntRange(hDevice, emFeatureID, pIntRange);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXGetIntRange '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS GetInt (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, int64_t *pnValue)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXGetInt(hDevice, emFeatureID, pnValue);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXGetInt '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS SetInt (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, int64_t pnValue)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXSetInt(hDevice, emFeatureID, pnValue);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXSetInt '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS GetFloatRange (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, GX_FLOAT_RANGE *pFloatRange)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXGetFloatRange(hDevice, emFeatureID, pFloatRange);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXGetFloatRange '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS GetFloat (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, double *pdValue)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXGetFloat(hDevice, emFeatureID, pdValue);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXGetFloat '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS SetFloat (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, double pdValue)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXSetFloat(hDevice, emFeatureID, pdValue);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXSetFloat '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS GetEnumEntryNums (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, uint32_t *pnEntryNums)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXGetEnumEntryNums(hDevice, emFeatureID, pnEntryNums);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXGetEnumEntryNums '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS GetEnumDescription (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, 
	GX_ENUM_DESCRIPTION *pEnumDescription, size_t *pBufferSize)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXGetEnumDescription(hDevice, emFeatureID, pEnumDescription, pBufferSize);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXGetEnumDescription '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS GetEnum (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, int64_t *pnValue)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXGetEnum(hDevice, emFeatureID, pnValue);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXGetEnum '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS SetEnum (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, int64_t pnValue)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXSetEnum(hDevice, emFeatureID, pnValue);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXSetEnum '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS GetBool (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, bool *pbValue)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXGetBool(hDevice, emFeatureID, pbValue);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXGetBool '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS SetBool (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, bool pbValue)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXSetBool(hDevice, emFeatureID, pbValue);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXSetBool '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS GetStringLength (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, size_t *pnSize)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXGetStringLength(hDevice, emFeatureID, pnSize);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXGetStringLength '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS GetString (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, char *pszContent, size_t *pnSize)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXGetString(hDevice, emFeatureID, pszContent, pnSize);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXGetString '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS SetString (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, char *pszContent)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXSetString(hDevice, emFeatureID, pszContent);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXSetString '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS GetBufferLength (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, size_t *pnSize)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXGetBufferLength(hDevice, emFeatureID, pnSize);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXGetBufferLength '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS GetBuffer (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, uint8_t *pBuffer, size_t *pnSize)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXGetBuffer(hDevice, emFeatureID, pBuffer, pnSize);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXGetBuffer '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS SetBuffer (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, uint8_t *pBuffer, size_t nSize)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXSetBuffer(hDevice, emFeatureID, pBuffer, nSize);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXSetBuffer '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}

GX_STATUS SendCommand (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (libInit) {
		emStatus = GXSendCommand(hDevice, emFeatureID);
		if (emStatus != GX_STATUS_SUCCESS) {
			char chFeatureName[64] = {0};
			size_t nSize = 64;
			GetFeatureName(hDevice, emFeatureID, chFeatureName, &nSize);

			log_info("ERROR : GXSendCommand '%s' errcode[%d].", chFeatureName, emStatus);
			GetErrorString(emStatus);
		}
	} else {
		emStatus = GX_STATUS_NOT_INIT_API;
	}

	return emStatus;
}
/*
void PixelConvert (uint8_t *pImageBuf, uint8_t *pImageRGBBuf, uint32_t nImageWidth, 
	uint32_t nImageHeight, uint32_t nPixelFormat, DX_PIXEL_COLOR_FILTER PixelColorFilter)
{
	switch (nPixelFormat) {
	case GX_PIXEL_FORMAT_BAYER_GR12:
	case GX_PIXEL_FORMAT_BAYER_RG12:
	case GX_PIXEL_FORMAT_BAYER_GB12:
	case GX_PIXEL_FORMAT_BAYER_BG12:
		DxRaw16toRaw8(pImageBuf, m_pImgRaw8Buffer, nImageWidth, nImageHeight, DX_BIT_4_11);	
		DxRaw8toRGB24(m_pImgRaw8Buffer, pImageRGBBuf, nImageWidth, nImageHeight, 
			RAW2RGB_NEIGHBOUR, PixelColorFilter, true);
		break;

	case GX_PIXEL_FORMAT_BAYER_GR10:
	case GX_PIXEL_FORMAT_BAYER_RG10:
	case GX_PIXEL_FORMAT_BAYER_GB10:
	case GX_PIXEL_FORMAT_BAYER_BG10:
		DxRaw16toRaw8(pImageBuf, m_pImgRaw8Buffer, nImageWidth, nImageHeight, DX_BIT_2_9);
		DxRaw8toRGB24(m_pImgRaw8Buffer, pImageRGBBuf, nImageWidth, nImageHeight, 
			RAW2RGB_NEIGHBOUR, PixelColorFilter, TRUE);	
		break;

	case GX_PIXEL_FORMAT_BAYER_GR8:
	case GX_PIXEL_FORMAT_BAYER_RG8:
	case GX_PIXEL_FORMAT_BAYER_GB8:
	case GX_PIXEL_FORMAT_BAYER_BG8:
		DxRaw8toRGB24(pImageBuf,pImageRGBBuf, nImageWidth, nImageHeight, 
			RAW2RGB_NEIGHBOUR, PixelColorFilter, TRUE);	
		break;

	case GX_PIXEL_FORMAT_MONO12:
		DxRaw16toRaw8(pImageBuf, m_pImgRaw8Buffer, nImageWidth, nImageHeight, DX_BIT_4_11);	
		DxRaw8toRGB24(m_pImgRaw8Buffer, pImageRGBBuf, nImageWidth, nImageHeight, 
			RAW2RGB_NEIGHBOUR, NONE, TRUE);
		break;

	case GX_PIXEL_FORMAT_MONO10:
		DxRaw16toRaw8(pImageBuf, m_pImgRaw8Buffer, nImageWidth, nImageHeight, DX_BIT_4_11);	
		DxRaw8toRGB24(m_pImgRaw8Buffer, pImageRGBBuf, nImageWidth, nImageHeight,
			RAW2RGB_NEIGHBOUR, NONE, TRUE);
		break;

	case GX_PIXEL_FORMAT_MONO8:
		DxRaw8toRGB24(pImageBuf, pImageRGBBuf, nImageWidth, nImageHeight,
			RAW2RGB_NEIGHBOUR, NONE, TRUE);

	default:
		break;
	}
}
*/

// Convert frame date to suitable pixel format
int PixelFormatConvert(PGX_FRAME_BUFFER pFrameBuffer, uint8_t *ImageBuf, int64_t PayloadSize)
{
	VxInt32 emDXStatus = DX_OK;

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

int SaveBMP ()
{


	return 0;
}

static int csv_gx_lib (uint8_t action)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if (GX_LIB_OPEN == action) {
		emStatus = GXInitLib();
		if (GX_STATUS_SUCCESS == emStatus) {
			libInit = true;
		}
		log_info("Galaxy library version : %s", GXGetLibVersion());
	} else if (GX_LIB_CLOSE == action) {
		emStatus = GXCloseLib();
		if (GX_STATUS_SUCCESS == emStatus) {
			libInit = false;
		}
	}

	if(GX_STATUS_SUCCESS != emStatus) {
		GetErrorString(emStatus);
		return -1;
	}

	return 0;
}

static int csv_gx_search (struct csv_gx_t *pGX)
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	uint32_t ui32DeviceNum = 0;

	if (!libInit) {
		return -1;
	}

	pGX->cnt_gx = 0;

	//Get device enumerated number
	emStatus = GXUpdateDeviceList(&ui32DeviceNum, 1000);
	if (GX_STATUS_SUCCESS != emStatus) {
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
	size_t nSize = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	struct cam_gx_spec_t *pCAM = NULL;

	if (!libInit) {
		return -1;
	}

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

		if (GX_STATUS_SUCCESS != emStatus) {
			GetErrorString(emStatus);
			ret = -1;
			continue;
		}

		if (NULL == pCAM->hDevice) {
			log_info("cam[%d] open null", i);
			ret = -1;
			continue;
		}

		pCAM->expoTime = 10000.0f;
		pCAM->gain = 0.0f;
		pCAM->PayloadSize = 0;
		pCAM->PixelColorFilter = false;
		pCAM->pMonoImageBuf = NULL;
		memset(pCAM->vendor, 0, SIZE_CAM_STR);
		memset(pCAM->model, 0, SIZE_CAM_STR);
		memset(pCAM->serial, 0, SIZE_CAM_STR);
		memset(pCAM->version, 0, SIZE_CAM_STR);
		memset(pCAM->userid, 0, SIZE_CAM_STR);

		emStatus = GetStringLength(pCAM->hDevice, GX_STRING_DEVICE_VENDOR_NAME, &nSize);
		if ((GX_STATUS_SUCCESS == emStatus)&&(nSize > 0)) {
			GetString(pCAM->hDevice, GX_STRING_DEVICE_VENDOR_NAME, pCAM->vendor, &nSize);
		}

		emStatus = GetStringLength(pCAM->hDevice, GX_STRING_DEVICE_MODEL_NAME, &nSize);
		if ((GX_STATUS_SUCCESS == emStatus)&&(nSize > 0)) {
			GetString(pCAM->hDevice, GX_STRING_DEVICE_MODEL_NAME, pCAM->model, &nSize);
		}

		emStatus = GetStringLength(pCAM->hDevice, GX_STRING_DEVICE_SERIAL_NUMBER, &nSize);
		if ((GX_STATUS_SUCCESS == emStatus)&&(nSize > 0)) {
			GetString(pCAM->hDevice, GX_STRING_DEVICE_SERIAL_NUMBER, pCAM->serial, &nSize);
		}

		emStatus = GetStringLength(pCAM->hDevice, GX_STRING_DEVICE_VERSION, &nSize);
		if ((GX_STATUS_SUCCESS == emStatus)&&(nSize > 0)) {
			GetString(pCAM->hDevice, GX_STRING_DEVICE_VERSION, pCAM->version, &nSize);
		}

		emStatus = GetStringLength(pCAM->hDevice, GX_STRING_DEVICE_USERID, &nSize);
		if ((GX_STATUS_SUCCESS == emStatus)&&(nSize > 0)) {
			GetString(pCAM->hDevice, GX_STRING_DEVICE_USERID, pCAM->userid, &nSize);
		}

		log_info("CAM[%d] : '%s' - '%s' (%s).", i, pCAM->model, pCAM->serial, pCAM->userid);

		emStatus = IsImplemented(pCAM->hDevice, GX_ENUM_PIXEL_COLOR_FILTER, &pCAM->PixelColorFilter);
		if (GX_STATUS_SUCCESS == emStatus) {
			if (pCAM->PixelColorFilter) {
				log_info("CAM[%d] : PixelColorFilter is color camera.", i);
			}
		}

		emStatus = GetInt(pCAM->hDevice, GX_INT_PAYLOAD_SIZE, &pCAM->PayloadSize);
		if ((GX_STATUS_SUCCESS == emStatus)&&(pCAM->PayloadSize > 0)) {
			pCAM->pMonoImageBuf = malloc(pCAM->PayloadSize + 1);
			if (NULL == pCAM->pMonoImageBuf) {
				log_err("ERROR : malloc MonoImageBuf");
				ret = -1;
				continue;
			}

			memset(pCAM->pMonoImageBuf, 0, pCAM->PayloadSize + 1);
		}

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

	if (!libInit) {
		return -1;
	}

	for (i = 0; i < pGX->cnt_gx; i++) {
		pCAM = &pGX->Cam[i];
		if ((NULL == pCAM)||(!pCAM->opened)||(NULL == pCAM->hDevice)) {
			continue;
		}

		emStatus = GXCloseDevice(pCAM->hDevice);
		if (GX_STATUS_SUCCESS != emStatus) {
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
		pCAM->PixelColorFilter = false;
		memset(pCAM->vendor, 0, SIZE_CAM_STR);
		memset(pCAM->model, 0, SIZE_CAM_STR);
		memset(pCAM->serial, 0, SIZE_CAM_STR);
		memset(pCAM->version, 0, SIZE_CAM_STR);
		memset(pCAM->userid, 0, SIZE_CAM_STR);
	}

	return ret;
}

int csv_gx_acquisition (struct csv_gx_t *pGX, uint8_t state)
{
	int i = 0, ret = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	struct cam_gx_spec_t *pCAM = NULL;

	if (!libInit) {
		return -1;
	}

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

		if (GX_STATUS_SUCCESS != emStatus) {
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
	//GX_STATUS emStatus = GX_STATUS_SUCCESS;
	struct cam_gx_spec_t *pCAM = NULL;

	if (!libInit) {
		return -1;
	}

	for (i = 0; i < pGX->cnt_gx; i++) {
		pCAM = &pGX->Cam[i];
		if ((NULL == pCAM)||(!pCAM->opened)||(NULL == pCAM->hDevice)) {
			continue;
		}

		GXSetEnum(pCAM->hDevice, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_SINGLE_FRAME);
//		GXSetAcqusitionBufferNumber(pCAM->hDevice, ACQ_BUFFER_NUM);

		SetEnum(pCAM->hDevice, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON);
		SetEnum(pCAM->hDevice, GX_ENUM_TRIGGER_ACTIVATION, GX_TRIGGER_ACTIVATION_RISINGEDGE);
		SetEnum(pCAM->hDevice, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE2);

		SetEnum(pCAM->hDevice, GX_ENUM_EXPOSURE_MODE, GX_EXPOSURE_MODE_TIMED);
		SetEnum(pCAM->hDevice, GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_OFF);
		SetFloat(pCAM->hDevice, GX_FLOAT_EXPOSURE_TIME, pCAM->expoTime);

		SetEnum(pCAM->hDevice, GX_ENUM_GAIN_AUTO,GX_GAIN_AUTO_OFF);
		SetFloat(pCAM->hDevice, GX_FLOAT_GAIN, pCAM->gain);

/*
		bool bStreamTransferSize = false;
		bool bStreamTransferNumberUrb = false;

		IsImplemented(pCAM->hDevice, GX_DS_INT_STREAM_TRANSFER_SIZE, &bStreamTransferSize);
		if (bStreamTransferSize) {
			// Set size of data transfer block
			SetInt(pCAM->hDevice, GX_DS_INT_STREAM_TRANSFER_SIZE, ACQ_TRANSFER_SIZE);
		}

		IsImplemented(pCAM->hDevice, GX_DS_INT_STREAM_TRANSFER_NUMBER_URB, &bStreamTransferNumberUrb);
		if (bStreamTransferNumberUrb) {
			// Set qty. of data transfer block
			SetInt(pCAM->hDevice, GX_DS_INT_STREAM_TRANSFER_NUMBER_URB, ACQ_TRANSFER_NUMBER_URB);
		}
*/

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

	if (!libInit) {
		return -1;
	}

	int ret = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	struct cam_gx_spec_t *pCAM = &pGX->Cam[idx];

	if ((NULL == pCAM)||(!pCAM->opened)||(NULL == pCAM->hDevice)) {
		return -1;
	}

	emStatus = GXDQBuf(pCAM->hDevice, &pCAM->pFrameBuffer, 2000);
	if (GX_STATUS_SUCCESS != emStatus) {
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
	if (GX_STATUS_SUCCESS != emStatus) {
		GetErrorString(emStatus);
		return -1;
	}

	return 0;
}

int csv_gx_cams_grab_both (struct csv_gx_t *pGX)
{
	int ret = -1, i = 0;
	int errNum = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	struct cam_gx_spec_t *pCAM = NULL;

	if (pGX->cnt_gx < 2) {
		return -1;
	}

    for (i = 0; i < pGX->cnt_gx; i++) {
		pCAM = &pGX->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->hDevice)||(NULL == pCAM->pMonoImageBuf)) {
			errNum++;
			continue;
		}

		memset(pCAM->pMonoImageBuf, 0x00, pCAM->PayloadSize);

		emStatus = GXDQBuf(pCAM->hDevice, &pCAM->pFrameBuffer, 2000);
		if (GX_STATUS_SUCCESS != emStatus) {
			GetErrorString(emStatus);
			errNum++;
			continue;
		}

		if (GX_FRAME_STATUS_SUCCESS != pCAM->pFrameBuffer->nStatus) {
			log_warn("ERROR : Abnormal Acquisition %d", pCAM->pFrameBuffer->nStatus);
			errNum++;
			continue;
		}

		ret = PixelFormatConvert(pCAM->pFrameBuffer, pCAM->pMonoImageBuf, pCAM->PayloadSize);
		if (0 == ret) {
			SavePPMFile(pCAM->pFrameBuffer->nWidth, pCAM->pFrameBuffer->nHeight);
		}

		emStatus = GXQBuf(pCAM->hDevice, pCAM->pFrameBuffer);
		if (GX_STATUS_SUCCESS != emStatus) {
			GetErrorString(emStatus);
			errNum++;
			continue;
		}

	}

	if (0 == errNum) {
		ret = 0;
	}

	if ((2 == pGX->cnt_gx)&&(0 == ret)) {
		log_info("OK : GetFrame L[%s][%lld] R[%s][%lld].", 
			pGX->Cam[CAM_LEFT].serial, pGX->Cam[CAM_LEFT].pFrameBuffer->nFrameID,
			pGX->Cam[CAM_RIGHT].serial, pGX->Cam[CAM_RIGHT].pFrameBuffer->nFrameID);
	}

	return ret;
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

	libInit = false;
	pGX->cnt_gx = 0;
	pGX->name_gx = NAME_THREAD_GX;

	ret = csv_gx_thread(pGX);

	return ret;
}


int csv_gx_deinit (void)
{
	int ret = 0;
	struct csv_gx_t *pGX = &gCSV->gx;

	libInit = false;
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





