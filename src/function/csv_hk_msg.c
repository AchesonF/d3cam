#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(USE_HK_CAMS)
static int hk_msg_cameras_enum (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = 0;
	int len_msg = 0;
	char str_enums[1024] = {0};
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_hk_spec_t *pCAMLEFT = &pMVS->Cam[CAM_LEFT];
	struct cam_hk_spec_t *pCAMRIGHT = &pMVS->Cam[CAM_RIGHT];
	//struct cam_hk_spec_t *pCAMFRONT = &pMVS->Cam[CAM_FRONT];
	//struct cam_hk_spec_t *pCAMBACK = &pMVS->Cam[CAM_BACK];

	ret = csv_mvs_cams_enum(pMVS);
	if (ret <= 0) {
		csv_msg_ack_package(pMP, pACK, NULL, 0, -1);
	} else {
		memset(str_enums, 0, 1024);

		if (1 == ret) {
			len_msg = snprintf(str_enums, 1024, "%s", pCAMLEFT->sn);
		} else {
			len_msg = snprintf(str_enums, 1024, "%s,%s", pCAMLEFT->sn, pCAMRIGHT->sn);
		}

		if (len_msg > 0) {
			csv_msg_ack_package(pMP, pACK, str_enums, len_msg, 0);
		}
	}

	return csv_msg_send(pACK);
}

static int hk_msg_cameras_open (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;

	ret = csv_mvs_cams_open(&gCSV->mvs);
	if (ret == 0) {
		result = 0;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int hk_msg_cameras_close (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;

	ret = csv_mvs_cams_close(&gCSV->mvs);
	if (ret == 0) {
		result = 0;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}


static int hk_msg_cameras_exposure_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = 0;
	int len_msg = 0;
	char str_expo[1024] = {0};
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_hk_spec_t *pCAMLEFT = &pMVS->Cam[CAM_LEFT];
	struct cam_hk_spec_t *pCAMRIGHT = &pMVS->Cam[CAM_RIGHT];

	ret = csv_mvs_cams_exposure_get(pMVS);
	if (ret < 0) {
		csv_msg_ack_package(pMP, pACK, NULL, 0, -1);
	} else {
		len_msg = snprintf(str_expo, 1024, "%s:%f;%s:%f", 
			pCAMLEFT->sn, pCAMLEFT->expoTime.fCurValue,
			pCAMRIGHT->sn, pCAMRIGHT->expoTime.fCurValue);

		if (len_msg > 0) {
			csv_msg_ack_package(pMP, pACK, str_expo, len_msg, 0);
		}
	}

	return csv_msg_send(pACK);
}

static int hk_msg_cameras_exposure_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
//	int ret = -1;
	int result = 0;
/*	float *ExpoTime = (float *)pMP->payload;

	if (pMP->hdr.length == sizeof(float)) {
		ret = csv_mvs_cams_exposure_set(&gCSV->mvs, *ExpoTime);
		if (ret == 0) {
			gCSV->dlp.expoTime = *ExpoTime;

			ret = csv_xml_write_DeviceParameters();
			if (ret == 0) {
				result = 0;
			}
		}
	}
*/

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

int hk_msg_cameras_rgb_exposure_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int len_msg = 0;
	char str_expo[1024] = {0};
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_hk_spec_t *pCAMLEFT = &pMVS->Cam[CAM_LEFT];
	struct cam_hk_spec_t *pCAMRIGHT = &pMVS->Cam[CAM_RIGHT];

	len_msg = snprintf(str_expo, 1024, "%s:%f;%s:%f", 
		pCAMLEFT->sn, gCSV->dlp.expoTime,
		pCAMRIGHT->sn, gCSV->dlp.expoTime);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_expo, len_msg, 0);
	}


	return csv_msg_send(pACK);
}

static int hk_msg_cameras_rgb_exposure_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int result = -1;

	if (pMP->hdr.length == sizeof(float)) {
		result = 0;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}


static int hk_msg_cameras_gain_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int len_msg = 0;
	char str_gain[1024] = {0};
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_hk_spec_t *pCAMLEFT = &pMVS->Cam[CAM_LEFT];
	struct cam_hk_spec_t *pCAMRIGHT = &pMVS->Cam[CAM_RIGHT];

	ret = csv_mvs_cams_gain_get(pMVS);
	if (ret < 0) {
		csv_msg_ack_package(pMP, pACK, NULL, 0, -1);
	} else {
		len_msg = snprintf(str_gain, 1024, "%s:%f;%s:%f", 
			pCAMLEFT->sn, pCAMLEFT->gain.fCurValue,
			pCAMRIGHT->sn, pCAMRIGHT->gain.fCurValue);

		if (len_msg > 0) {
			csv_msg_ack_package(pMP, pACK, str_gain, len_msg, 0);
		}
	}

	return csv_msg_send(pACK);
}

static int hk_msg_cameras_gain_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;
	float *fGain = (float *)pMP->payload;

	if (pMP->hdr.length == sizeof(float)) {
		ret = csv_mvs_cams_gain_set(&gCSV->mvs, *fGain);
		if (ret == 0) {
			result = 0;
		}
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int hk_msg_cameras_rate_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int len_msg = 0;
	char str_rate[1024] = {0};
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_hk_spec_t *pCAMLEFT = &pMVS->Cam[CAM_LEFT];
	struct cam_hk_spec_t *pCAMRIGHT = &pMVS->Cam[CAM_RIGHT];

	len_msg = snprintf(str_rate, 1024, "%s:%f;%s:%f", 
		pCAMLEFT->sn, gCSV->dlp.rate,
		pCAMRIGHT->sn, gCSV->dlp.rate);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_rate, len_msg, 0);
	}

	return csv_msg_send(pACK);
}

static int hk_msg_cameras_rate_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;
	float *rate = (float *)pMP->payload;

	if (pMP->hdr.length == sizeof(float)) {
		gCSV->dlp.rate = *rate;

		ret = csv_xml_write_DeviceParameters();
		if (ret == 0) {
			result = 0;
		}
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int hk_msg_cameras_brightness_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int len_msg = 0;
	char str_bright[1024] = {0};
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_hk_spec_t *pCAMLEFT = &pMVS->Cam[CAM_LEFT];
	struct cam_hk_spec_t *pCAMRIGHT = &pMVS->Cam[CAM_RIGHT];

	len_msg = snprintf(str_bright, 1024, "%s:%f;%s:%f", 
		pCAMLEFT->sn, gCSV->dlp.brightness,
		pCAMRIGHT->sn, gCSV->dlp.brightness);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_bright, len_msg, 0);
	}

	return csv_msg_send(pACK);
}

static int hk_msg_cameras_brightness_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;
	float *bright = (float *)pMP->payload;

	if (pMP->hdr.length == sizeof(float)) {
		gCSV->dlp.brightness = *bright;

		ret = csv_xml_write_DeviceParameters();
		if (ret == 0) {
			result = 0;
		}
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int hk_msg_cameras_name_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;
	char str_sn[32] = {0}, str_name[32]= {0};

	if (pMP->hdr.length > 10) {	// string like: "${sn}:${name}"
		if (strstr((char *)pMP->payload, ":")) {
			int nget = 0;
			nget = sscanf((char *)pMP->payload, "%[^:]:%[^:]", str_sn, str_name);
			if (2 == nget) {
				ret = csv_mvs_cams_name_set(&gCSV->mvs, str_sn, str_name);
				if (ret == 0) {
					result = 0;
				}
			}
		}
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}


static int hk_msg_cameras_calibrate (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	struct csv_mvs_t *pMVS = &gCSV->mvs;

	pMVS->grab_type = GRAB_CALIBRATE;
	pthread_cond_broadcast(&pMVS->cond_grab);

	csv_msg_ack_package(pMP, pACK, NULL, 0, 0);

	return csv_msg_send(pACK);
}

static int hk_msg_cameras_pointcloud (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	struct csv_mvs_t *pMVS = &gCSV->mvs;

	pMVS->grab_type = GRAB_POINTCLOUD;
	pthread_cond_broadcast(&pMVS->cond_grab);

	csv_msg_ack_package(pMP, pACK, NULL, 0, 0);

	return csv_msg_send(pACK);
}

int hk_msg_cameras_grab_img_depth (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	csv_msg_ack_package(pMP, pACK, NULL, 0, 0);

	return csv_msg_send(pACK);
}

void csv_hk_msg_cmd_enroll (void)
{
	msg_command_add(CAMERA_ENMU, toSTR(CAMERA_ENMU), hk_msg_cameras_enum);
	msg_command_add(CAMERA_CONNECT, toSTR(CAMERA_CONNECT), hk_msg_cameras_open);
	msg_command_add(CAMERA_DISCONNECT, toSTR(CAMERA_DISCONNECT), hk_msg_cameras_close);
	msg_command_add(CAMERA_GET_EXPOSURE, toSTR(CAMERA_GET_EXPOSURE), hk_msg_cameras_exposure_get);
	msg_command_add(CAMERA_GET_GAIN, toSTR(CAMERA_GET_GAIN), hk_msg_cameras_gain_get);
	msg_command_add(CAMERA_GET_RATE, toSTR(CAMERA_GET_RATE), hk_msg_cameras_rate_get);
	msg_command_add(CAMERA_GET_BRIGHTNESS, toSTR(CAMERA_GET_BRIGHTNESS), hk_msg_cameras_brightness_get);

	msg_command_add(CAMERA_SET_EXPOSURE, toSTR(CAMERA_SET_EXPOSURE), hk_msg_cameras_exposure_set);
	msg_command_add(CAMERA_SET_GAIN, toSTR(CAMERA_SET_GAIN), hk_msg_cameras_gain_set);
	msg_command_add(CAMERA_SET_CAMERA_NAME, toSTR(CAMERA_SET_CAMERA_NAME), hk_msg_cameras_name_set);
	msg_command_add(CAMERA_SET_RGB_EXPOSURE, toSTR(CAMERA_SET_RGB_EXPOSURE), hk_msg_cameras_rgb_exposure_set);
	msg_command_add(CAMERA_SET_RATE, toSTR(CAMERA_SET_RATE), hk_msg_cameras_rate_set);
	msg_command_add(CAMERA_SET_BRIGHTNESS, toSTR(CAMERA_SET_BRIGHTNESS), hk_msg_cameras_brightness_set);

	msg_command_add(CAMERA_GET_GRAB_FLASH, toSTR(CAMERA_GET_GRAB_FLASH), hk_msg_cameras_grab_gray);
	msg_command_add(CAMERA_GET_GRAB_DEEP, toSTR(CAMERA_GET_GRAB_DEEP), hk_msg_cameras_grab_img_depth);
	msg_command_add(CAMERA_GET_GRAB_LEDOFF, toSTR(CAMERA_GET_GRAB_LEDOFF), hk_msg_cameras_grab_gray);
	msg_command_add(CAMERA_GET_GRAB_LEDON, toSTR(CAMERA_GET_GRAB_LEDON), hk_msg_cameras_grab_gray);
	//msg_command_add(CAMERA_GET_GRAB_RGB, toSTR(CAMERA_GET_GRAB_RGB), msg_cameras_grab_urandom);
	msg_command_add(CAMERA_GET_GRAB_RGB, toSTR(CAMERA_GET_GRAB_RGB), hk_msg_cameras_grab_rgb);
	msg_command_add(CAMERA_GET_GRAB_RGB_LEFT, toSTR(CAMERA_GET_GRAB_RGB_LEFT), hk_msg_cameras_grab_rgb);
	msg_command_add(CAMERA_GET_GRAB_RGB_RIGHT, toSTR(CAMERA_GET_GRAB_RGB_RIGHT), hk_msg_cameras_grab_rgb);
	msg_command_add(CAMERA_GET_CALIBRATE, toSTR(CAMERA_GET_CALIBRATE), hk_msg_cameras_calibrate);
	msg_command_add(CAMERA_GET_POINTCLOUD, toSTR(CAMERA_GET_POINTCLOUD), hk_msg_cameras_pointcloud);

}

#else

void csv_hk_msg_cmd_enroll (void)
{
}

#endif

#ifdef __cplusplus
}
#endif




