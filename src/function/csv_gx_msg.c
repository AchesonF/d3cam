#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(USE_GX_CAMS)

void csv_gx_msg_cmd_enroll (void)
{
/*
	msg_command_add(CAMERA_ENMU, toSTR(CAMERA_ENMU), msg_gx_cameras_enum);
	msg_command_add(CAMERA_CONNECT, toSTR(CAMERA_CONNECT), msg_gx_cameras_open);
	msg_command_add(CAMERA_DISCONNECT, toSTR(CAMERA_DISCONNECT), msg_gx_cameras_close);
	msg_command_add(CAMERA_GET_EXPOSURE, toSTR(CAMERA_GET_EXPOSURE), msg_gx_cameras_exposure_get);
	msg_command_add(CAMERA_GET_GAIN, toSTR(CAMERA_GET_GAIN), msg_gx_cameras_gain_get);
	msg_command_add(CAMERA_GET_RATE, toSTR(CAMERA_GET_RATE), msg_gx_cameras_rate_get);
	msg_command_add(CAMERA_GET_BRIGHTNESS, toSTR(CAMERA_GET_BRIGHTNESS), msg_gx_cameras_brightness_get);

	msg_command_add(CAMERA_SET_EXPOSURE, toSTR(CAMERA_SET_EXPOSURE), msg_gx_cameras_exposure_set);
	msg_command_add(CAMERA_SET_GAIN, toSTR(CAMERA_SET_GAIN), msg_gx_cameras_gain_set);
	msg_command_add(CAMERA_SET_CAMERA_NAME, toSTR(CAMERA_SET_CAMERA_NAME), msg_gx_cameras_name_set);
	msg_command_add(CAMERA_SET_RGB_EXPOSURE, toSTR(CAMERA_SET_RGB_EXPOSURE), msg_gx_cameras_rgb_exposure_set);
	msg_command_add(CAMERA_SET_RATE, toSTR(CAMERA_SET_RATE), msg_gx_cameras_rate_set);
	msg_command_add(CAMERA_SET_BRIGHTNESS, toSTR(CAMERA_SET_BRIGHTNESS), msg_gx_cameras_brightness_set);

	msg_command_add(CAMERA_GET_GRAB_FLASH, toSTR(CAMERA_GET_GRAB_FLASH), msg_gx_cameras_grab_gray);
	msg_command_add(CAMERA_GET_GRAB_DEEP, toSTR(CAMERA_GET_GRAB_DEEP), msg_gx_cameras_grab_img_depth);
	//msg_command_add(CAMERA_GET_GRAB_DEEP, toSTR(CAMERA_GET_GRAB_DEEP), msg_gx_cameras_highspeed);
	msg_command_add(CAMERA_GET_GRAB_LEDOFF, toSTR(CAMERA_GET_GRAB_LEDOFF), msg_gx_cameras_grab_gray);
	msg_command_add(CAMERA_GET_GRAB_LEDON, toSTR(CAMERA_GET_GRAB_LEDON), msg_gx_cameras_grab_gray);
	//msg_command_add(CAMERA_GET_GRAB_RGB, toSTR(CAMERA_GET_GRAB_RGB), msg_gx_cameras_grab_urandom);
	msg_command_add(CAMERA_GET_GRAB_RGB, toSTR(CAMERA_GET_GRAB_RGB), msg_gx_cameras_grab_rgb);
	//msg_command_add(CAMERA_GET_GRAB_RGB, toSTR(CAMERA_GET_GRAB_RGB), msg_gx_cameras_demarcate);
	msg_command_add(CAMERA_GET_GRAB_RGB_LEFT, toSTR(CAMERA_GET_GRAB_RGB_LEFT), msg_gx_cameras_grab_rgb);
	msg_command_add(CAMERA_GET_GRAB_RGB_RIGHT, toSTR(CAMERA_GET_GRAB_RGB_RIGHT), msg_gx_cameras_grab_rgb);
*/
}

#else

void csv_gx_msg_cmd_enroll (void)
{
}

#endif

#ifdef __cplusplus
}
#endif


