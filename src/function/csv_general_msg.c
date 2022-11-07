#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


static int msg_cameras_calibrate_file_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	uint32_t len_msg = 0;
	char *str_cali = NULL;

	ret = csv_file_get_size(FILE_CALIB_XML_NEW, &len_msg);
	if (ret == 0) {
		if (len_msg > 0) {
			str_cali = (char *)malloc(len_msg);
			if (NULL == str_cali) {
				log_err("ERROR : malloc cali.");
				ret = -1;
			}

			if (ret == 0) {
				ret = csv_file_read_string(FILE_CALIB_XML_NEW, str_cali, len_msg);
			}
		}
	}

	if (ret < 0) {
		csv_msg_ack_package(pMP, pACK, NULL, 0, -1);
	} else {
		if (len_msg > 0) {
			csv_msg_ack_package(pMP, pACK, str_cali, len_msg, 0);
			if (NULL != str_cali) {
				free(str_cali);
			}
		}
	}

	return csv_msg_send(pACK);
}

static int msg_cameras_calibrate_file_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;
	uint8_t *str_cali = pMP->payload;

	if (pMP->hdr.length > 0) {
		if (strstr((char *)str_cali, "calibrationBoardSize")) {
			ret = csv_file_write_data(FILE_CALIB_XML_NEW, str_cali, pMP->hdr.length);
			if (ret == 0) {
				result = 0;
				// TODO mv FILE_CALIB_XML_NEW FILE_CALIB_XML
			}
		}
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int msg_led_delay_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;
	//int *delay = (int *)pMP->payload;

	if (pMP->hdr.length == sizeof(int)) {
		ret = 0;
		if (ret == 0) {
			result = 0;

			// TODO : delay var
		}
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int msg_set_errlog_level (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;

	uint8_t *p = pMP->payload;
	int *enable = (int *)p;
	p += 4;
	int *level = (int *)p;

	ret = csv_udp_set_level((uint8_t)*enable, (uint8_t)*level);
	if (ret == 0) {
		result = 0;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int msg_sys_info_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	//int ret = 0;
	int len_msg = 0;
	char str_info[1024] = {0};

	len_msg = snprintf(str_info, 1024, "gcc:%s, buildtime:%s %s, devicetype=0",
		COMPILER_VERSION, BUILD_DATE, BUILD_TIME);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_info, len_msg, 0);
	}

	return csv_msg_send(pACK);
}

static int msg_heartbeat_cfg_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
//	int ret = -1;
	int result = -1;
/*
	uint8_t *p = pMP->payload;
	uint32_t *enable = (uint32_t *)p;
	p += 4;
	uint32_t *period = (uint32_t *)p;
*/
	// todo

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int msg_heartbeat_cfg_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int len_msg = 0;
	char str_hbcfg[64] = {0};

	len_msg = snprintf(str_hbcfg, 64, "%d:%d", gFILE.beat_enable, gFILE.beat_period);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_hbcfg, len_msg, 0);
	}

	return csv_msg_send(pACK);
}

static int msg_heartbeat_kick (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	csv_msg_ack_package(pMP, pACK, NULL, 0, 0);

	return csv_msg_send(pACK);
}

static int msg_log_server_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;

	uint8_t *p = pMP->payload;
	uint32_t *ipaddr = (uint32_t *)p;
	p += 4;
	uint32_t *port = (uint32_t *)p;

	ret = csv_udp_set_server(*ipaddr, (uint16_t)*port);
	if (ret == 0) {
		result = 0;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int msg_log_server_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int len_msg = 0;
	char str_serv[64] = {0};

	len_msg = snprintf(str_serv, 64, "%s:%d", gUDP.ip, gUDP.port);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_serv, len_msg, 0);
	}

	return csv_msg_send(pACK);
}

void csv_general_msg_cmd_enroll (void)
{
	msg_command_add(CAMERA_GET_CALIB_FILE, toSTR(CAMERA_GET_CALIB_FILE), msg_cameras_calibrate_file_get);
	msg_command_add(CAMERA_SET_CALIB_FILE, toSTR(CAMERA_SET_CALIB_FILE), msg_cameras_calibrate_file_set);
	msg_command_add(CAMERA_SET_LED_DELAY_TIME, toSTR(CAMERA_SET_LED_DELAY_TIME), msg_led_delay_set);
	msg_command_add(SYS_SET_ERRLOG_LEVEL, toSTR(SYS_SET_ERRLOG_LEVEL), msg_set_errlog_level);
	msg_command_add(SYS_SOFT_INFO, toSTR(SYS_SOFT_INFO), msg_sys_info_get);

	msg_command_add(SYS_HEARTBEAT_CFG_SET, toSTR(SYS_HEARTBEAT_CFG_SET), msg_heartbeat_cfg_set);
	msg_command_add(SYS_HEARTBEAT_CFG_GET, toSTR(SYS_HEARTBEAT_CFG_GET), msg_heartbeat_cfg_get);
	msg_command_add(SYS_HEARTBEAT_KICK, toSTR(SYS_HEARTBEAT_KICK), msg_heartbeat_kick);
	msg_command_add(SYS_LOG_SERVER_SET, toSTR(SYS_LOG_SERVER_SET), msg_log_server_set);
	msg_command_add(SYS_LOG_SERVER_GET, toSTR(SYS_LOG_SERVER_GET), msg_log_server_get);
}


#ifdef __cplusplus
}
#endif


