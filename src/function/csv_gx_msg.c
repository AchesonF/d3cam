#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(USE_GX_CAMS)

static int gx_msg_cameras_enum (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int len_msg = 0;
	char str_enums[1024] = {0};
	struct csv_gx_t *pGX = &gCSV->gx;
	struct cam_gx_spec_t *pCAMLEFT = &pGX->Cam[CAM_LEFT];
	struct cam_gx_spec_t *pCAMRIGHT = &pGX->Cam[CAM_RIGHT];

	if (pGX->cnt_gx <= 0) {
		csv_msg_ack_package(pMP, pACK, NULL, 0, -1);
	} else {
		memset(str_enums, 0, 1024);
		if (1 == pGX->cnt_gx) {
			len_msg = snprintf(str_enums, 1024, "%s", pCAMLEFT->serial);
		} else if (2 <= pGX->cnt_gx) {
			len_msg = snprintf(str_enums, 1024, "%s,%s", pCAMLEFT->serial, pCAMRIGHT->serial);
		}

		if (len_msg > 0) {
			csv_msg_ack_package(pMP, pACK, str_enums, len_msg, 0);
		}
	}

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_open (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1, result = -1;

	struct csv_gx_t *pGX = &gCSV->gx;
	struct cam_gx_spec_t *pCAMLEFT = &pGX->Cam[CAM_LEFT];
	struct cam_gx_spec_t *pCAMRIGHT = &pGX->Cam[CAM_RIGHT];

	if (pCAMLEFT->opened && pCAMRIGHT->opened) {
		ret = csv_gx_acquisition(GX_ACQUISITION_START);
		if (0 == ret) {
			result = 0;
		}
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_close (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1, result = -1;

	ret = csv_gx_acquisition(GX_ACQUISITION_STOP);
	if (0 == ret) {
		result = 0;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_exposure_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int len_msg = 0;
	char str_expo[1024] = {0};
	struct csv_gx_t *pGX = &gCSV->gx;
	struct cam_gx_spec_t *pCAMLEFT = &pGX->Cam[CAM_LEFT];
	struct cam_gx_spec_t *pCAMRIGHT = &pGX->Cam[CAM_RIGHT];

	len_msg = snprintf(str_expo, 1024, "%s:%f;%s:%f", 
		pCAMLEFT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_NORMAL].expoTime,
		pCAMRIGHT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_NORMAL].expoTime);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_expo, len_msg, 0);
	}

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_exposure_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int result = 0;
	float *ExpoTime = (float *)pMP->payload;

	if (pMP->hdr.length == sizeof(float)) {
		gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_NORMAL].expoTime = *ExpoTime;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_gain_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int len_msg = 0;
	char str_gain[1024] = {0};
	struct csv_gx_t *pGX = &gCSV->gx;
	struct cam_gx_spec_t *pCAMLEFT = &pGX->Cam[CAM_LEFT];
	struct cam_gx_spec_t *pCAMRIGHT = &pGX->Cam[CAM_RIGHT];

	len_msg = snprintf(str_gain, 1024, "%s:%f;%s:%f", 
		pCAMLEFT->serial, pCAMLEFT->gain,
		pCAMRIGHT->serial, pCAMRIGHT->gain);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_gain, len_msg, 0);
	}

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_gain_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int result = 0;
	//float *fGain = (float *)pMP->payload;

	if (pMP->hdr.length == sizeof(float)) {

	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_rate_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int len_msg = 0;
	char str_rate[1024] = {0};
	struct csv_gx_t *pGX = &gCSV->gx;
	struct cam_gx_spec_t *pCAMLEFT = &pGX->Cam[CAM_LEFT];
	struct cam_gx_spec_t *pCAMRIGHT = &pGX->Cam[CAM_RIGHT];

	len_msg = snprintf(str_rate, 1024, "%s:%f;%s:%f", 
		pCAMLEFT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_NORMAL].rate,
		pCAMRIGHT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_NORMAL].rate);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_rate, len_msg, 0);
	}

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_rate_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int result = 0;
	float *rate = (float *)pMP->payload;

	if (pMP->hdr.length == sizeof(float)) {
		gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_NORMAL].rate = *rate;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_brightness_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int len_msg = 0;
	char str_bright[1024] = {0};
	struct csv_gx_t *pGX = &gCSV->gx;
	struct cam_gx_spec_t *pCAMLEFT = &pGX->Cam[CAM_LEFT];
	struct cam_gx_spec_t *pCAMRIGHT = &pGX->Cam[CAM_RIGHT];

	len_msg = snprintf(str_bright, 1024, "%s:%f;%s:%f", 
		pCAMLEFT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_NORMAL].brightness,
		pCAMRIGHT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_NORMAL].brightness);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_bright, len_msg, 0);
	}

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_brightness_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int result = 0;
	float *bright = (float *)pMP->payload;

	if (pMP->hdr.length == sizeof(float)) {
		gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_NORMAL].brightness = *bright;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_name_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;
	char str_sn[32] = {0}, str_name[32]= {0};

	if (pMP->hdr.length > 10) {	// string like: "${sn}:${name}"
		if (strstr((char *)pMP->payload, ":")) {
			int nget = 0;
			nget = sscanf((char *)pMP->payload, "%[^:]:%[^:]", str_sn, str_name);
			if (2 == nget) {
				//SetString(pCAM->hDevice, GX_STRING_DEVICE_USERID, str_name);
				if (ret == 0) {
					result = 0;
				}
			}
		}
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_rgb_exposure_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int result = -1;

	if (pMP->hdr.length == sizeof(float)) {
		result = 0;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_demarcate (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;

	ret = csv_gx_cams_demarcate(&gCSV->gx);

	if (ret == 0) {
		result = 0;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_highspeed (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;

	ret = csv_gx_cams_highspeed(&gCSV->gx);

	if (ret == 0) {
		result = 0;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}


void csv_gx_msg_cmd_enroll (void)
{
	msg_command_add(CAMERA_ENMU, toSTR(CAMERA_ENMU), gx_msg_cameras_enum);
	msg_command_add(CAMERA_CONNECT, toSTR(CAMERA_CONNECT), gx_msg_cameras_open);
	msg_command_add(CAMERA_DISCONNECT, toSTR(CAMERA_DISCONNECT), gx_msg_cameras_close);
	msg_command_add(CAMERA_GET_EXPOSURE, toSTR(CAMERA_GET_EXPOSURE), gx_msg_cameras_exposure_get);
	msg_command_add(CAMERA_SET_EXPOSURE, toSTR(CAMERA_SET_EXPOSURE), gx_msg_cameras_exposure_set);
	msg_command_add(CAMERA_GET_GAIN, toSTR(CAMERA_GET_GAIN), gx_msg_cameras_gain_get);
	msg_command_add(CAMERA_SET_GAIN, toSTR(CAMERA_SET_GAIN), gx_msg_cameras_gain_set);
	msg_command_add(CAMERA_GET_RATE, toSTR(CAMERA_GET_RATE), gx_msg_cameras_rate_get);
	msg_command_add(CAMERA_SET_RATE, toSTR(CAMERA_SET_RATE), gx_msg_cameras_rate_set);
	msg_command_add(CAMERA_GET_BRIGHTNESS, toSTR(CAMERA_GET_BRIGHTNESS), gx_msg_cameras_brightness_get);
	msg_command_add(CAMERA_SET_BRIGHTNESS, toSTR(CAMERA_SET_BRIGHTNESS), gx_msg_cameras_brightness_set);
	msg_command_add(CAMERA_SET_CAMERA_NAME, toSTR(CAMERA_SET_CAMERA_NAME), gx_msg_cameras_name_set);
	msg_command_add(CAMERA_SET_RGB_EXPOSURE, toSTR(CAMERA_SET_RGB_EXPOSURE), gx_msg_cameras_rgb_exposure_set);

	msg_command_add(CAMERA_GET_GRAB_FLASH, toSTR(CAMERA_GET_GRAB_FLASH), gx_msg_cameras_grab_gray);
	msg_command_add(CAMERA_GET_GRAB_DEEP, toSTR(CAMERA_GET_GRAB_DEEP), gx_msg_cameras_grab_img_depth);
	msg_command_add(CAMERA_GET_GRAB_LEDOFF, toSTR(CAMERA_GET_GRAB_LEDOFF), gx_msg_cameras_grab_gray);
	msg_command_add(CAMERA_GET_GRAB_LEDON, toSTR(CAMERA_GET_GRAB_LEDON), gx_msg_cameras_grab_gray);
	msg_command_add(CAMERA_GET_GRAB_RGB, toSTR(CAMERA_GET_GRAB_RGB), gx_msg_cameras_grab_rgb);
	msg_command_add(CAMERA_GET_GRAB_RGB_LEFT, toSTR(CAMERA_GET_GRAB_RGB_LEFT), gx_msg_cameras_grab_rgb);
	msg_command_add(CAMERA_GET_GRAB_RGB_RIGHT, toSTR(CAMERA_GET_GRAB_RGB_RIGHT), gx_msg_cameras_grab_rgb);
	msg_command_add(0x00005009, toSTR(demarcate), gx_msg_cameras_demarcate);
	msg_command_add(0x0000500A, toSTR(highspeed), gx_msg_cameras_highspeed);

}

#else

void csv_gx_msg_cmd_enroll (void)
{
}

#endif

#ifdef __cplusplus
}
#endif


