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
		ret = csv_gx_cams_acquisition(GX_START_ACQ);
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

	ret = csv_gx_cams_acquisition(GX_STOP_ACQ);
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
		pCAMLEFT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_BRIGHT].expoTime,
		pCAMRIGHT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_BRIGHT].expoTime);

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
		gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_BRIGHT].expoTime = *ExpoTime;
		csv_xml_write_DeviceParameters();
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_exposure_calib_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int len_msg = 0;
	char str_expo[1024] = {0};
	struct csv_gx_t *pGX = &gCSV->gx;
	struct cam_gx_spec_t *pCAMLEFT = &pGX->Cam[CAM_LEFT];
	struct cam_gx_spec_t *pCAMRIGHT = &pGX->Cam[CAM_RIGHT];

	len_msg = snprintf(str_expo, 1024, "%s:%f;%s:%f", 
		pCAMLEFT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_CALIB].expoTime,
		pCAMRIGHT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_CALIB].expoTime);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_expo, len_msg, 0);
	}

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_exposure_calib_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int result = 0;
	float *ExpoTime = (float *)pMP->payload;

	if (pMP->hdr.length == sizeof(float)) {
		gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_CALIB].expoTime = *ExpoTime;
		csv_xml_write_DeviceParameters();
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_exposure_pc_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int len_msg = 0;
	char str_expo[1024] = {0};
	struct csv_gx_t *pGX = &gCSV->gx;
	struct cam_gx_spec_t *pCAMLEFT = &pGX->Cam[CAM_LEFT];
	struct cam_gx_spec_t *pCAMRIGHT = &pGX->Cam[CAM_RIGHT];

	len_msg = snprintf(str_expo, 1024, "%s:%f;%s:%f", 
		pCAMLEFT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_POINTCLOUD].expoTime,
		pCAMRIGHT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_POINTCLOUD].expoTime);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_expo, len_msg, 0);
	}

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_exposure_pc_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int result = 0;
	float *ExpoTime = (float *)pMP->payload;

	if (pMP->hdr.length == sizeof(float)) {
		gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_POINTCLOUD].expoTime = *ExpoTime;
		csv_xml_write_DeviceParameters();
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
		pCAMLEFT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_POINTCLOUD].rate,
		pCAMRIGHT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_POINTCLOUD].rate);

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
		gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_POINTCLOUD].rate = *rate;
		csv_xml_write_DeviceParameters();
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
		pCAMLEFT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_CALIB].brightness,
		pCAMRIGHT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_CALIB].brightness);

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
		gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_CALIB].brightness = *bright;
		csv_xml_write_DeviceParameters();
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_brightness_pc_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int len_msg = 0;
	char str_bright[1024] = {0};
	struct csv_gx_t *pGX = &gCSV->gx;
	struct cam_gx_spec_t *pCAMLEFT = &pGX->Cam[CAM_LEFT];
	struct cam_gx_spec_t *pCAMRIGHT = &pGX->Cam[CAM_RIGHT];

	len_msg = snprintf(str_bright, 1024, "%s:%f;%s:%f", 
		pCAMLEFT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_POINTCLOUD].brightness,
		pCAMRIGHT->serial, gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_POINTCLOUD].brightness);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_bright, len_msg, 0);
	}

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_brightness_pc_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int result = 0;
	float *bright = (float *)pMP->payload;

	if (pMP->hdr.length == sizeof(float)) {
		gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_POINTCLOUD].brightness = *bright;
		csv_xml_write_DeviceParameters();
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

static int gx_msg_cameras_calibrate (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
    char str_err[128] = {0};
    int len_err = 0;
	struct csv_gx_t *pGX = &gCSV->gx;

	if (pGX->busying) {
		ret = -2;
	} else {
		pGX->grab_type = GRAB_CALIB_PICS;
		//ret = csv_gx_cams_calibrate(pGX);
		pGX->sendTo = SEND_TO_FILE;

		ret = csv_gx_grab_prepare(pGX, gCSV->cfg.calibcfg.CalibImageRoot);
		if (0 == ret) {
			ret = csv_gx_grab_bright_trigger(pGX, CAM_STATUS_CALIB_BRIGHT);
			if (0 == ret) {
				ret = csv_gx_grab_stripe_trigger(pGX, DLP_CMD_CALIB);
			}
		}
	}

	if (ret == 0) {
		csv_msg_ack_package(pMP, pACK, NULL, 0, 0);
	} else if (-2 == ret) {
		len_err = snprintf(str_err, 128, "Cams busy now.");
		csv_msg_ack_package(pMP, pACK, str_err, len_err, -1);
	} else {
		len_err = snprintf(str_err, 128, "Cams grab failed.");
		csv_msg_ack_package(pMP, pACK, str_err, len_err, -1);
	}

	return csv_msg_send(pACK);
}

static int gx_msg_cameras_pointcloud (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
    char str_err[128] = {0};
    int len_err = 0;
	struct csv_gx_t *pGX = &gCSV->gx;

	if (pGX->busying) {
		ret = -2;
	} else {
		pGX->grab_type = GRAB_DEPTHIMAGE_PICS;
		//ret = csv_gx_cams_pointcloud(pGX, SEND_TO_FILE);
		pGX->sendTo = SEND_TO_FILE;

		ret = csv_gx_grab_prepare(pGX, gCSV->cfg.pointcloudcfg.PCImageRoot);
		if (0 == ret) {
			ret = csv_gx_grab_bright_trigger(pGX, CAM_STATUS_DEPTH_BRIGHT);
			if (0 == ret) {
				ret = csv_gx_grab_stripe_trigger(pGX, DLP_CMD_POINTCLOUD);
				if (0 == ret) {
					ret = csv_3d_calc(pGX->sendTo);
				}
			}
		}
	}

	if (ret == 0) {
		csv_msg_ack_package(pMP, pACK, NULL, 0, 0);
	} else if (-2 == ret) {
		len_err = snprintf(str_err, 128, "Cams busy now.");
		csv_msg_ack_package(pMP, pACK, str_err, len_err, -1);
	} else {
		len_err = snprintf(str_err, 128, "Cams depth failed.");
		csv_msg_ack_package(pMP, pACK, str_err, len_err, -1);
	}

	return csv_msg_send(pACK);
}

static int gx_msg_camera_hdrimage (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
    char str_err[128] = {0};
    int len_err = 0;
	struct csv_gx_t *pGX = &gCSV->gx;

	if (pGX->busying) {
		ret = -2;
	} else {
		pGX->grab_type = GRAB_HDRIMAGE_PICS;
		ret = csv_gx_cams_hdrimage(pGX);
	}

	if (ret == 0) {
		csv_msg_ack_package(pMP, pACK, NULL, 0, 0);
	} else if (-2 == ret) {
		len_err = snprintf(str_err, 128, "Cams busy now.");
		csv_msg_ack_package(pMP, pACK, str_err, len_err, -1);
	} else {
		len_err = snprintf(str_err, 128, "Cams grab failed.");
		csv_msg_ack_package(pMP, pACK, str_err, len_err, -1);
	}

	return csv_msg_send(pACK);
}


void csv_gx_msg_cmd_enroll (void)
{
	msg_command_add(CAMERA_ENMU, toSTR(CAMERA_ENMU), gx_msg_cameras_enum);
	msg_command_add(CAMERA_CONNECT, toSTR(CAMERA_CONNECT), gx_msg_cameras_open);
	msg_command_add(CAMERA_DISCONNECT, toSTR(CAMERA_DISCONNECT), gx_msg_cameras_close);
	msg_command_add(CAMERA_GET_EXPOSURE, toSTR(CAMERA_GET_EXPOSURE), gx_msg_cameras_exposure_get);
	msg_command_add(CAMERA_SET_EXPOSURE, toSTR(CAMERA_SET_EXPOSURE), gx_msg_cameras_exposure_set);
	msg_command_add(CAMERA_GET_EXPOSURE_CALIB, toSTR(CAMERA_GET_EXPOSURE_CALIB), gx_msg_cameras_exposure_calib_get);
	msg_command_add(CAMERA_SET_EXPOSURE_CALIB, toSTR(CAMERA_SET_EXPOSURE_CALIB), gx_msg_cameras_exposure_calib_set);
	msg_command_add(CAMERA_GET_EXPOSURE_PC, toSTR(CAMERA_GET_EXPOSURE_PC), gx_msg_cameras_exposure_pc_get);
	msg_command_add(CAMERA_SET_EXPOSURE_PC, toSTR(CAMERA_SET_EXPOSURE_PC), gx_msg_cameras_exposure_pc_set);
	msg_command_add(CAMERA_GET_GAIN, toSTR(CAMERA_GET_GAIN), gx_msg_cameras_gain_get);
	msg_command_add(CAMERA_SET_GAIN, toSTR(CAMERA_SET_GAIN), gx_msg_cameras_gain_set);
	msg_command_add(CAMERA_GET_RATE, toSTR(CAMERA_GET_RATE), gx_msg_cameras_rate_get);
	msg_command_add(CAMERA_SET_RATE, toSTR(CAMERA_SET_RATE), gx_msg_cameras_rate_set);
	msg_command_add(CAMERA_GET_BRIGHTNESS, toSTR(CAMERA_GET_BRIGHTNESS), gx_msg_cameras_brightness_get);
	msg_command_add(CAMERA_SET_BRIGHTNESS, toSTR(CAMERA_SET_BRIGHTNESS), gx_msg_cameras_brightness_set);
	msg_command_add(CAMERA_GET_BRIGHTNESS_PC, toSTR(CAMERA_GET_BRIGHTNESS_PC), gx_msg_cameras_brightness_pc_get);
	msg_command_add(CAMERA_SET_BRIGHTNESS_PC, toSTR(CAMERA_SET_BRIGHTNESS_PC), gx_msg_cameras_brightness_pc_set);
	msg_command_add(CAMERA_SET_CAMERA_NAME, toSTR(CAMERA_SET_CAMERA_NAME), gx_msg_cameras_name_set);
	msg_command_add(CAMERA_SET_RGB_EXPOSURE, toSTR(CAMERA_SET_RGB_EXPOSURE), gx_msg_cameras_rgb_exposure_set);

	msg_command_add(CAMERA_GET_GRAB_FLASH, toSTR(CAMERA_GET_GRAB_FLASH), gx_msg_cameras_grab_gray);
	msg_command_add(CAMERA_GET_GRAB_DEEP, toSTR(CAMERA_GET_GRAB_DEEP), gx_msg_cameras_grab_img_depth);
	msg_command_add(CAMERA_GET_GRAB_LEDOFF, toSTR(CAMERA_GET_GRAB_LEDOFF), gx_msg_cameras_grab_gray);
	msg_command_add(CAMERA_GET_GRAB_LEDON, toSTR(CAMERA_GET_GRAB_LEDON), gx_msg_cameras_grab_gray);
	msg_command_add(CAMERA_GET_GRAB_RGB, toSTR(CAMERA_GET_GRAB_RGB), gx_msg_cameras_grab_rgb);
	msg_command_add(CAMERA_GET_GRAB_RGB_LEFT, toSTR(CAMERA_GET_GRAB_RGB_LEFT), gx_msg_cameras_grab_rgb);
	msg_command_add(CAMERA_GET_GRAB_RGB_RIGHT, toSTR(CAMERA_GET_GRAB_RGB_RIGHT), gx_msg_cameras_grab_rgb);
	msg_command_add(CAMERA_GET_CALIBRATE, toSTR(CAMERA_GET_CALIBRATE), gx_msg_cameras_calibrate);
	msg_command_add(CAMERA_GET_POINTCLOUD, toSTR(CAMERA_GET_POINTCLOUD), gx_msg_cameras_pointcloud);
	msg_command_add(CAMERA_GET_HDRIMAGE, toSTR(CAMERA_GET_HDRIMAGE), gx_msg_camera_hdrimage);

}

#else

void csv_gx_msg_cmd_enroll (void)
{
}

#endif

#ifdef __cplusplus
}
#endif


