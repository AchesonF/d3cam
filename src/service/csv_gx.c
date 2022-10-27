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

#define GxErrStr(emErrorStatus) do {log_info("errcode[%d]", emErrorStatus); \
	GetErrorString(emErrorStatus);} while(0)

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

GX_STATUS IsImplemented (GX_DEV_HANDLE hDevice, GX_FEATURE_ID emFeatureID, bool *pbIsImplemented)
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
static int PixelFormatConvert (PGX_FRAME_BUFFER pFrameBuffer, uint8_t *ImageBuf, int64_t PayloadSize)
{
	VxInt32 emDXStatus = DX_OK;

	switch (pFrameBuffer->nPixelFormat) {
	case GX_PIXEL_FORMAT_MONO8:
		memcpy(ImageBuf, pFrameBuffer->pImgBuf, PayloadSize);
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
		log_debug("ERROR : PixelFormat [0x%08X] not supported.", pFrameBuffer->nPixelFormat);
		return -1;
    }

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
		log_info("Galaxy library version : '%s'", GXGetLibVersion());
	} else if (GX_LIB_CLOSE == action) {
		emStatus = GXCloseLib();
		if (GX_STATUS_SUCCESS == emStatus) {
			libInit = false;
		}
	}

	if (GX_STATUS_SUCCESS != emStatus) {
		GxErrStr(emStatus);
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
		GxErrStr(emStatus);
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
	char cfg_name[256] = {0};

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
			GxErrStr(emStatus);
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
		pCAM->PixelColorFilter = GX_COLOR_FILTER_NONE;
		pCAM->LinkThroughputLimit = 200000000ll;
		pCAM->FrameRate = 60.0f;
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

		log_info("CAM[%d] : '%s' - '%s' - '%s'.", i, pCAM->model, pCAM->serial, pCAM->version);

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


		GetFloatRange(pCAM->hDevice, GX_FLOAT_EXPOSURE_TIME, &pCAM->expoTimeRange);
		GetFloatRange(pCAM->hDevice, GX_FLOAT_GAIN, &pCAM->gainRange);
		GetEnum(pCAM->hDevice, GX_ENUM_PIXEL_FORMAT, &pCAM->PixelFormat);

		if (gCSV->cfg.devicecfg.exportCamsCfg) {
			memset(cfg_name, 0, 256);
			snprintf(cfg_name, 256, "%s_%s.ini", pCAM->model, pCAM->serial);
			emStatus = GXExportConfigFile(pCAM->hDevice, cfg_name);
			if (GX_STATUS_SUCCESS != emStatus) {
				GxErrStr(emStatus);
			}
		}
	}

	if ((TOTAL_CAMS != pGX->cnt_gx)||(pGX->Cam[CAM_LEFT].PayloadSize != pGX->Cam[CAM_RIGHT].PayloadSize)) {
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
			GxErrStr(emStatus);
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
		pCAM->PixelColorFilter = GX_COLOR_FILTER_NONE;
		memset(pCAM->vendor, 0, SIZE_CAM_STR);
		memset(pCAM->model, 0, SIZE_CAM_STR);
		memset(pCAM->serial, 0, SIZE_CAM_STR);
		memset(pCAM->version, 0, SIZE_CAM_STR);
		memset(pCAM->userid, 0, SIZE_CAM_STR);
	}

	return ret;
}

int csv_gx_acquisition (uint8_t state)
{
	int i = 0, ret = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	struct csv_gx_t *pGX = &gCSV->gx;
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
			//emStatus = GXStreamOn(pCAM->hDevice);
			emStatus = SendCommand(pCAM->hDevice, GX_COMMAND_ACQUISITION_START);
		} else if (GX_ACQUISITION_STOP == state) {
			//emStatus = GXStreamOff(pCAM->hDevice);
			emStatus = SendCommand(pCAM->hDevice, GX_COMMAND_ACQUISITION_STOP);
		}

		if (GX_STATUS_SUCCESS != emStatus) {
			GxErrStr(emStatus);
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

	// GXExportConfigFile();
	// GXImportConfigFile();

	for (i = 0; i < pGX->cnt_gx; i++) {
		pCAM = &pGX->Cam[i];
		if ((NULL == pCAM)||(!pCAM->opened)||(NULL == pCAM->hDevice)) {
			continue;
		}

		// GX_ACQUISITION_MODE_ENTRY
		SetEnum(pCAM->hDevice, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS);

		// GX_TRIGGER_MODE_ENTRY
		SetEnum(pCAM->hDevice, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON);
		// GX_TRIGGER_ACTIVATION_ENTRY
		SetEnum(pCAM->hDevice, GX_ENUM_TRIGGER_ACTIVATION, GX_TRIGGER_ACTIVATION_RISINGEDGE);
		// GX_TRIGGER_SOURCE_ENTRY
		SetEnum(pCAM->hDevice, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE2);

		// GX_EXPOSURE_MODE_ENTRY
		SetEnum(pCAM->hDevice, GX_ENUM_EXPOSURE_MODE, GX_EXPOSURE_MODE_TIMED);
		// GX_EXPOSURE_AUTO_ENTRY
		SetEnum(pCAM->hDevice, GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_OFF);
		SetFloat(pCAM->hDevice, GX_FLOAT_EXPOSURE_TIME, pCAM->expoTime);

		// GX_GAIN_AUTO_ENTRY
		SetEnum(pCAM->hDevice, GX_ENUM_GAIN_AUTO,GX_GAIN_AUTO_OFF);
		SetFloat(pCAM->hDevice, GX_FLOAT_GAIN, pCAM->gain);

		SetBool(pCAM->hDevice, GX_BOOL_CHUNKMODE_ACTIVE, false);
		SetBool(pCAM->hDevice, GX_BOOL_CHUNK_ENABLE, false);

		// GX_DEVICE_LINK_THROUGHPUT_LIMIT_MODE_ENTRY
		SetEnum(pCAM->hDevice, GX_ENUM_DEVICE_LINK_THROUGHPUT_LIMIT_MODE, GX_DEVICE_LINK_THROUGHPUT_LIMIT_MODE_ON);
		//GetInt(pCAM->hDevice, GX_INT_DEVICE_LINK_CURRENT_THROUGHPUT, &nLinkThroughputVal);
		SetInt(pCAM->hDevice, GX_INT_DEVICE_LINK_THROUGHPUT_LIMIT, pCAM->LinkThroughputLimit);

		// 使能采集帧率调节模式
		SetEnum(pCAM->hDevice, GX_ENUM_ACQUISITION_FRAME_RATE_MODE, GX_ACQUISITION_FRAME_RATE_MODE_ON);
		// 设置采集帧率
		SetFloat(pCAM->hDevice, GX_FLOAT_ACQUISITION_FRAME_RATE, pCAM->FrameRate);
	}

	return ret;
}

static int csv_gx_cams_deinit (struct csv_gx_t *pGX)
{

	return 0;
}

int csv_gx_cams_exposure_set (float fExposureTime)
{
	int ret = -1, i = 0;
	int errNum = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	struct csv_gx_t *pGX = &gCSV->gx;
	struct cam_gx_spec_t *pCAM = NULL;

	if ((!libInit)||(pGX->cnt_gx < 2)) {
		return -1;
	}

    for (i = 0; i < pGX->cnt_gx; i++) {
		pCAM = &pGX->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->hDevice)||(NULL == pCAM->pMonoImageBuf)) {
			errNum++;
			continue;
		}

		emStatus = SetFloat(pCAM->hDevice, GX_FLOAT_EXPOSURE_TIME, (double)fExposureTime);
		if (GX_STATUS_SUCCESS != emStatus) {
			errNum++;
		}
	}

	if (0 == errNum) {
		ret = 0;
	}

	return ret;
}


int csv_gx_cams_grab_both (struct csv_gx_t *pGX)
{
	int ret = -1, i = 0;
	int errNum = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	struct cam_gx_spec_t *pCAM = NULL;

	if ((!libInit)||(pGX->cnt_gx < 2)) {
		return -1;
	}

    for (i = 0; i < pGX->cnt_gx; i++) {
		pCAM = &pGX->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->hDevice)||(NULL == pCAM->pMonoImageBuf)) {
			errNum++;
			continue;
		}

		memset(pCAM->pMonoImageBuf, 0x00, pCAM->PayloadSize);

		emStatus = GXDQBuf(pCAM->hDevice, &pCAM->pFrameBuffer, 3000);
		if (GX_STATUS_SUCCESS != emStatus) {
			GxErrStr(emStatus);
			errNum++;
			continue;
		}

		if (GX_FRAME_STATUS_SUCCESS != pCAM->pFrameBuffer->nStatus) {
			log_warn("ERROR : Abnormal Acquisition %d", pCAM->pFrameBuffer->nStatus);
			errNum++;
			continue;
		}

		PixelFormatConvert(pCAM->pFrameBuffer, pCAM->pMonoImageBuf, pCAM->PayloadSize);

		emStatus = GXQBuf(pCAM->hDevice, pCAM->pFrameBuffer);
		if (GX_STATUS_SUCCESS != emStatus) {
			GxErrStr(emStatus);
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

int csv_gx_cams_calibrate (struct csv_gx_t *pGX)
{
	int ret = 0, i = 0;
	int idx = 1;
	char img_name[256] = {0};
	uint8_t lastpic = 0;
	struct cam_gx_spec_t *pCAM = NULL;
	struct calib_conf_t *pCALIB = &gCSV->cfg.calibcfg;
	struct device_cfg_t *pDevC = &gCSV->cfg.devicecfg;
	int errNum = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if ((!libInit)||(pGX->cnt_gx < 2)) {
		return -1;
	}

	if ((NULL == pCALIB->path)
		||(!csv_file_isExist(pCALIB->path))) {
		log_warn("ERROR : cali img path null.");
		return -1;
	}

	if (pGX->busying) {
		return -2;
	}

	pGX->busying = true;

	csv_img_clear(pCALIB->path);

	ret = csv_gx_acquisition(GX_ACQUISITION_START);

	// 1 常亮
	ret = csv_dlp_just_write(DLP_CMD_BRIGHT);

	for (i = 0; i < pGX->cnt_gx; i++) {
		pCAM = &pGX->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->hDevice)||(NULL == pCAM->pMonoImageBuf)) {
			errNum++;
			continue;
		}

		//memset(pCAM->pMonoImageBuf, 0x00, pCAM->PayloadSize);

		emStatus = GXDQBuf(pCAM->hDevice, &pCAM->pFrameBuffer, 2000);
		if (GX_STATUS_SUCCESS != emStatus) {
			log_warn("ERROR : CAM '%s' GXDQBuf errcode[%d].", pCAM->serial, emStatus);
			GxErrStr(emStatus);
			errNum++;
			break;
		}

		if (GX_FRAME_STATUS_SUCCESS != pCAM->pFrameBuffer->nStatus) {
			log_warn("ERROR : Abnormal Acquisition %d", pCAM->pFrameBuffer->nStatus);
			errNum++;
			continue;
		}

		ret = PixelFormatConvert(pCAM->pFrameBuffer, pCAM->pMonoImageBuf, pCAM->PayloadSize);
		if ((0 == ret)&&(pDevC->SaveImageFile)) {
			memset(img_name, 0, 256);
			csv_img_generate_filename(pCALIB->path, pCALIB->groupCalibrate, 0, i, img_name);
			csv_img_push(img_name, pCAM->pMonoImageBuf, pCAM->PayloadSize, 
				pCAM->pFrameBuffer->nWidth, pCAM->pFrameBuffer->nHeight, i, pGX->action_type, 0);
		}

		emStatus = GXQBuf(pCAM->hDevice, pCAM->pFrameBuffer);
		if (GX_STATUS_SUCCESS != emStatus) {
			GxErrStr(emStatus);
			errNum++;
			continue;
		}

	}

	// 22 标定条纹
	ret = csv_dlp_just_write(DLP_CMD_CALIB);

	while (idx <= NUM_PICS_CALIB) {
		for (i = 0; i < pGX->cnt_gx; i++) {
			pCAM = &pGX->Cam[i];

			if ((!pCAM->opened)||(NULL == pCAM->hDevice)||(NULL == pCAM->pMonoImageBuf)) {
				errNum++;
				continue;
			}

			//memset(pCAM->pMonoImageBuf, 0x00, pCAM->PayloadSize);

			emStatus = GXDQBuf(pCAM->hDevice, &pCAM->pFrameBuffer, 2000);
			if (GX_STATUS_SUCCESS != emStatus) {
				log_warn("ERROR : CAM '%s' GXDQBuf errcode[%d].", pCAM->serial, emStatus);
				GxErrStr(emStatus);
				errNum++;
				break;
			}

			if (GX_FRAME_STATUS_SUCCESS != pCAM->pFrameBuffer->nStatus) {
				log_warn("ERROR : Abnormal Acquisition %d", pCAM->pFrameBuffer->nStatus);
				errNum++;
				continue;
			}

			ret = PixelFormatConvert(pCAM->pFrameBuffer, pCAM->pMonoImageBuf, pCAM->PayloadSize);
			if ((0 == ret)&&(pDevC->SaveImageFile)) {
				memset(img_name, 0, 256);
				csv_img_generate_filename(pCALIB->path, pCALIB->groupCalibrate, idx, i, img_name);
				if ((NUM_PICS_CALIB == idx)&&(CAM_RIGHT == i)) {
					lastpic = 1;
				}
				csv_img_push(img_name, pCAM->pMonoImageBuf, pCAM->PayloadSize, 
					pCAM->pFrameBuffer->nWidth, pCAM->pFrameBuffer->nHeight, i, pGX->action_type, lastpic);
			}

			emStatus = GXQBuf(pCAM->hDevice, pCAM->pFrameBuffer);
			if (GX_STATUS_SUCCESS != emStatus) {
				GxErrStr(emStatus);
				errNum++;
				continue;
			}
		}

		if (errNum) {
			break;
		}

		idx++;
	}

//	ret = csv_gx_acquisition(GX_ACQUISITION_STOP);
	pthread_cond_broadcast(&gCSV->img.cond_img);

	if (0 != errNum) {
		pGX->busying = false;
		return -1;
	}

	return ret;
}

int csv_gx_cams_pointcloud (struct csv_gx_t *pGX)
{
	int ret = 0, i = 0;
	int idx = 1;
	char img_name[256] = {0};
	uint8_t lastpic = 0;
	struct cam_gx_spec_t *pCAM = NULL;
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;
	struct device_cfg_t *pDevC = &gCSV->cfg.devicecfg;
	int errNum = 0;
	GX_STATUS emStatus = GX_STATUS_SUCCESS;

	if ((!libInit)||(pGX->cnt_gx < 2)) {
		return -1;
	}

	if (pGX->busying) {
		return -2;
	}

	pGX->busying = true;

	csv_img_clear(pPC->ImageSaveRoot);

	ret = csv_gx_acquisition(GX_ACQUISITION_START);

	// 1 常亮
	ret = csv_dlp_just_write(DLP_CMD_BRIGHT);

	for (i = 0; i < pGX->cnt_gx; i++) {
		pCAM = &pGX->Cam[i];

		if ((!pCAM->opened)||(NULL == pCAM->hDevice)||(NULL == pCAM->pMonoImageBuf)) {
			errNum++;
			continue;
		}

		//memset(pCAM->pMonoImageBuf, 0x00, pCAM->PayloadSize);

		emStatus = GXDQBuf(pCAM->hDevice, &pCAM->pFrameBuffer, 2000);
		if (GX_STATUS_SUCCESS != emStatus) {
			log_warn("ERROR : CAM '%s' GXDQBuf errcode[%d].", pCAM->serial, emStatus);
			GxErrStr(emStatus);
			errNum++;
			break;
		}

		if (GX_FRAME_STATUS_SUCCESS != pCAM->pFrameBuffer->nStatus) {
			log_warn("ERROR : Abnormal Acquisition %d", pCAM->pFrameBuffer->nStatus);
			errNum++;
			continue;
		}

		ret = PixelFormatConvert(pCAM->pFrameBuffer, pCAM->pMonoImageBuf, pCAM->PayloadSize);
		if ((0 == ret)&&(pDevC->SaveImageFile)) {
			memset(img_name, 0, 256);
			csv_img_generate_filename(pPC->ImageSaveRoot, pPC->groupPointCloud, 0, i, img_name);
			csv_img_push(img_name, pCAM->pMonoImageBuf, pCAM->PayloadSize, 
				pCAM->pFrameBuffer->nWidth, pCAM->pFrameBuffer->nHeight, i, pGX->action_type, 0);
		}

		emStatus = GXQBuf(pCAM->hDevice, pCAM->pFrameBuffer);
		if (GX_STATUS_SUCCESS != emStatus) {
			GxErrStr(emStatus);
			errNum++;
			continue;
		}

	}

	// 13 条纹
	ret = csv_dlp_just_write(DLP_CMD_POINTCLOUD);

	while (idx <= NUM_PICS_POINTCLOUD) {
		for (i = 0; i < pGX->cnt_gx; i++) {
			pCAM = &pGX->Cam[i];

			if ((!pCAM->opened)||(NULL == pCAM->hDevice)||(NULL == pCAM->pMonoImageBuf)) {
				errNum++;
				continue;
			}

			//memset(pCAM->pMonoImageBuf, 0x00, pCAM->PayloadSize);

			emStatus = GXDQBuf(pCAM->hDevice, &pCAM->pFrameBuffer, 2000);
			if (GX_STATUS_SUCCESS != emStatus) {
				log_warn("ERROR : CAM '%s' GXDQBuf errcode[%d].", pCAM->serial, emStatus);
				GxErrStr(emStatus);
				errNum++;
				break;
			}

			if (GX_FRAME_STATUS_SUCCESS != pCAM->pFrameBuffer->nStatus) {
				log_warn("ERROR : Abnormal Acquisition %d", pCAM->pFrameBuffer->nStatus);
				errNum++;
				continue;
			}

			ret = PixelFormatConvert(pCAM->pFrameBuffer, pCAM->pMonoImageBuf, pCAM->PayloadSize);
			if ((0 == ret)&&(pDevC->SaveImageFile)) {
				memset(img_name, 0, 256);
				csv_img_generate_filename(pPC->ImageSaveRoot, pPC->groupPointCloud, idx, i, img_name);
				if ((NUM_PICS_POINTCLOUD == idx)&&(CAM_RIGHT == i)) {
					lastpic = 1;
					csv_img_generate_depth_filename(pPC->ImageSaveRoot, pPC->groupPointCloud, pPC->outDepthImage);
				}
				csv_img_push(img_name, pCAM->pMonoImageBuf, pCAM->PayloadSize, 
					pCAM->pFrameBuffer->nWidth, pCAM->pFrameBuffer->nHeight, i, pGX->action_type, lastpic);
			}

			emStatus = GXQBuf(pCAM->hDevice, pCAM->pFrameBuffer);
			if (GX_STATUS_SUCCESS != emStatus) {
				GxErrStr(emStatus);
				errNum++;
				continue;
			}
		}

		if (errNum) {
			break;
		}

		idx++;
	}

//	ret = csv_gx_acquisition(GX_ACQUISITION_STOP);
	pthread_cond_broadcast(&gCSV->img.cond_img);

	if (0 != errNum) {
		pGX->busying = false;
		return -1;
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

	csv_3d_init();

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

	pthread_mutex_init(&pGX->mutex_wait_depth, NULL);
	pthread_cond_init(&pGX->cond_wait_depth, NULL);

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
	pGX->action_type = ACTION_NONE;
	pGX->busying = false;

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

#endif

#ifdef __cplusplus
}
#endif





