#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


static int csv_msg_ack_package (struct msg_package_t *pMP, struct msg_ack_t *pACK, 
	char *content, int len, int retcode)
{
	pACK->len_send = len+sizeof(struct msg_head_t);
	pACK->buf_send = (uint8_t *)malloc(pACK->len_send + 1);
	if (NULL == pACK->buf_send) {
		log_err("ERROR : malloc send");
		return -1;
	}

	memset(pACK->buf_send, 0, pACK->len_send+1);

	uint8_t *pS = pACK->buf_send;
	struct msg_head_t *pHDR = (struct msg_head_t *)pS;
	pHDR->cmdtype = pMP->hdr.cmdtype;
	pHDR->length = len+sizeof(retcode);
	pHDR->result = retcode;

	if (NULL != content) {
		memcpy(pS+sizeof(struct msg_head_t), content, len);
	}

	return pACK->len_send;
}


int csv_msg_send (struct msg_ack_t *pACK)
{
	int ret = -1;

	if (NULL != pACK->buf_send) {
		ret = csv_tcp_local_send(pACK->buf_send, pACK->len_send);
		free(pACK->buf_send);
		pACK->buf_send = NULL;
		pACK->len_send = 0;
	}

	return ret;
}


static int msg_cameras_enum (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = 0;
	int len_msg = 0;
	char str_enums[1024] = {0};
	struct csv_mvs_t *pMVS = &gCSV->mvs;

	pACK->len_send = 0;

	ret = csv_mvs_cams_enum();
	if (ret <= 0) {
		csv_msg_ack_package(pMP, pACK, NULL, 0, -1);
	} else {
		memset(str_enums, 0, 1024);
		switch (gCSV->cfg.device_param.device_type) {
		case CAM1_LIGHT2:
			len_msg = snprintf(str_enums, 1024, "%s", pMVS->cam[0].serialNum);
			break;

		case CAM4_LIGHT1:
			len_msg = snprintf(str_enums, 1024, "%s,%s,%s,%s", pMVS->cam[0].serialNum, 
				pMVS->cam[1].serialNum, pMVS->cam[2].serialNum, pMVS->cam[3].serialNum);
			break;
		case CAM2_LIGHT1:
		case RDM_LIGHT:
			len_msg = snprintf(str_enums, 1024, "%s,%s", pMVS->cam[0].serialNum, 
				pMVS->cam[1].serialNum);
		default:
			break;
		}

		if (len_msg > 0) {
			csv_msg_ack_package(pMP, pACK, str_enums, len_msg, 0);
		}
	}

	return csv_msg_send(pACK);
}

static int msg_cameras_open (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;

	pACK->len_send = 0;

	ret = csv_mvs_cams_open();
	if (ret == 0) {
		result = 0;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int msg_cameras_close (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;

	pACK->len_send = 0;

	ret = csv_mvs_cams_close();
	if (ret == 0) {
		result = 0;
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}


static int msg_cameras_exposure_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = 0;
	int len_msg = 0;
	char str_expo[1024] = {0};
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM0 = &pMVS->cam[0], *pCAM1 = &pMVS->cam[1];

	pACK->len_send = 0;

	ret = csv_mvs_cams_exposure_get();
	if (ret < 0) {
		csv_msg_ack_package(pMP, pACK, NULL, 0, -1);
	} else {
		len_msg = snprintf(str_expo, 1024, "%s:%f;%s:%f", 
			pCAM0->serialNum, pCAM0->exposureTime.fCurValue,
			pCAM0->serialNum, pCAM0->exposureTime.fCurValue);

		if (len_msg > 0) {
			csv_msg_ack_package(pMP, pACK, str_expo, len_msg, 0);
		}
	}

	return csv_msg_send(pACK);
}

static int msg_cameras_exposure_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;
	float *ExpoTime = (float *)pMP->payload;

	pACK->len_send = 0;

	if (pMP->hdr.length == sizeof(float)) {
		ret = csv_mvs_cams_exposure_set(*ExpoTime);
		if (ret == 0) {
			result = 0;

			// TODO :send to dlp
		}
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int msg_cameras_gain_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int len_msg = 0;
	char str_gain[1024] = {0};
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM0 = &pMVS->cam[0], *pCAM1 = &pMVS->cam[1];

	pACK->len_send = 0;

	ret = csv_mvs_cams_gain_get();
	if (ret < 0) {
		csv_msg_ack_package(pMP, pACK, NULL, 0, -1);
	} else {
		len_msg = snprintf(str_gain, 1024, "%s:%f;%s:%f", 
			pCAM0->serialNum, pCAM0->camGain.fCurValue,
			pCAM0->serialNum, pCAM0->camGain.fCurValue);

		if (len_msg > 0) {
			csv_msg_ack_package(pMP, pACK, str_gain, len_msg, 0);
		}
	}

	return csv_msg_send(pACK);
}

static int msg_cameras_gain_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;
	float *fGain = (float *)pMP->payload;

	pACK->len_send = 0;

	if (pMP->hdr.length == sizeof(float)) {
		ret = csv_mvs_cams_gain_set(*fGain);
		if (ret == 0) {
			result = 0;
		}
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}


static int msg_cameras_calibrate_file_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int len_msg = 0;
	char *str_cali = NULL;

	pACK->len_send = 0;
	ret = csv_file_get_size(FILE_CAMERA_CALIBRATE, &len_msg);
	if (ret == 0) {
		if (len_msg > 0) {
			str_cali = (char *)malloc(len_msg);
			if (NULL == str_cali) {
				log_err("ERROR : malloc cali");
				ret = -1;
			}

			if (ret == 0) {
				ret = csv_file_read_string(FILE_CAMERA_CALIBRATE, str_cali, len_msg);
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
		if (strstr((char *)str_cali, "cam_Q")) {
			ret = csv_file_write_data(FILE_CAMERA_CALIBRATE, str_cali, pMP->hdr.length);
			if (ret == 0) {
				result = 0;
				// TODO source it
			}
		}
	}

	pACK->len_send = 0;

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int msg_cameras_name_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;
	char str_sn[32] = {0}, str_name[32]= {0};

	if (pMP->hdr.length > 10) {	// string like: "${sn}:${name}"
		if (strstr(pMP->payload, ":")) {
			int nget = 0;
			nget = sscanf(pMP->payload, "%[^:]:%[^:]", str_sn, str_name);
			if (2 == nget) {
				ret = csv_mvs_cams_name_set(str_sn, str_name);
				if (ret == 0) {
					result = 0;
				}
			}
		}
	}

	pACK->len_send = 0;

	csv_msg_ack_package(pMP, pACK, NULL, 0, result);

	return csv_msg_send(pACK);
}

static int msg_led_delay_set (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int result = -1;
	int *delay = (int *)pMP->payload;

	pACK->len_send = 0;

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

static int msg_sys_info_get (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = 0;
	int len_msg = 0;
	char str_info[1024] = {0};

	pACK->len_send = 0;

	len_msg = snprintf(str_info, 1024, "gcc:%s, buildtime:%s %s, devicetype=%d",
		COMPILER_VERSION, BUILD_DATE, BUILD_TIME, gCSV->cfg.device_param.device_type);

	if (len_msg > 0) {
		csv_msg_ack_package(pMP, pACK, str_info, len_msg, 0);
	}

	return csv_msg_send(pACK);
}


static struct msg_command_list *msg_command_malloc (void)
{
	struct msg_command_list *cur = NULL;

	cur = (struct msg_command_list *)malloc(sizeof(struct msg_command_list));
	if (cur == NULL) {
		log_err("ERROR : malloc msg_command_list");
		return NULL;
	}
	memset(cur, 0, sizeof(struct msg_command_list));

	return cur;
}

static void msg_command_add (struct csv_msg_t *pMSG,
	csv_cmd_e cmdtype, char *cmdname, 
	int (*func)(struct msg_package_t *pMP, struct msg_ack_t *pACK))
{
	struct msg_command_list *cur = NULL;

	cur = msg_command_malloc();
	if (cur != NULL) {
		cur->command.cmdtype = cmdtype;
		cur->command.cmdname = cmdname;
		cur->command.func = func,

		list_add_tail(&cur->list, &pMSG->head_cmd.list);
	}
}

static void csv_msg_cmd_register (struct csv_msg_t *pMSG)
{
	msg_command_add(pMSG, CAMERA_ENMU, toSTR(CAMERA_ENMU), msg_cameras_enum);
	msg_command_add(pMSG, CAMERA_CONNECT, toSTR(CAMERA_CONNECT), msg_cameras_open);
	msg_command_add(pMSG, CAMERA_DISCONNECT, toSTR(CAMERA_DISCONNECT), msg_cameras_close);
	msg_command_add(pMSG, CAMERA_GET_EXPOSURE, toSTR(CAMERA_GET_EXPOSURE), msg_cameras_exposure_get);
	msg_command_add(pMSG, CAMERA_GET_GAIN, toSTR(CAMERA_GET_GAIN), msg_cameras_gain_get);
	msg_command_add(pMSG, CAMERA_GET_CALIB_FILE, toSTR(CAMERA_GET_CALIB_FILE), msg_cameras_calibrate_file_get);
	msg_command_add(pMSG, CAMERA_SET_EXPOSURE, toSTR(CAMERA_SET_EXPOSURE), msg_cameras_exposure_set);
	msg_command_add(pMSG, CAMERA_SET_GAIN, toSTR(CAMERA_SET_GAIN), msg_cameras_gain_set);
	msg_command_add(pMSG, CAMERA_SET_CALIB_FILE, toSTR(CAMERA_SET_CALIB_FILE), msg_cameras_calibrate_file_set);
	msg_command_add(pMSG, CAMERA_SET_CAMERA_NAME, toSTR(CAMERA_SET_CAMERA_NAME), msg_cameras_name_set);
	msg_command_add(pMSG, CAMERA_SET_LED_DELAY_TIME, toSTR(CAMERA_SET_LED_DELAY_TIME), msg_led_delay_set);
	msg_command_add(pMSG, SYS_SOFT_INFO, toSTR(SYS_SOFT_INFO), msg_sys_info_get);



	// todo add more cmd


	log_info("OK : register message commands.");
}

static void csv_msg_cmd_unregister (struct csv_msg_t *pMSG)
{
	struct list_head *pos = NULL, *n = NULL;
	struct msg_command_list *task = NULL;

	list_for_each_safe(pos, n, &pMSG->head_cmd.list) {
		task = list_entry(pos, struct msg_command_list, list);
		if (task == NULL) {
			break;
		}

		list_del(&task->list);
		free(task);
		task = NULL;
	}

	log_info("OK : unregister message commands.");
}

static int csv_msg_push (struct csv_msg_t *pMSG, uint8_t *buf, uint32_t len)
{
	if ((NULL == pMSG)||(NULL == buf)||(len < sizeof(struct msg_head_t))) {
		return -1;
	}

	struct msglist_t *cur = NULL;
	struct msg_package_t *pMP = NULL;
	struct msg_head_t *pHDR = NULL;
	pHDR = (struct msg_head_t *)buf;

	cur = (struct msglist_t *)malloc(sizeof(struct msglist_t));
	if (cur == NULL) {
		log_err("ERROR : malloc msglist_t");
		return -1;
	}
	memset(cur, 0, sizeof(struct msglist_t));
	pMP = &cur->mp;

	memcpy(&pMP->hdr, pHDR, sizeof(struct msg_head_t));
	if (pHDR->length > 0) {
		pMP->payload = (uint8_t *)malloc(pHDR->length);

		if (NULL == pMP->payload) {
			log_err("ERROR : malloc payload");
			return -1;
		}

		memcpy(pMP->payload, buf+sizeof(struct msg_head_t), pHDR->length);
	}

	pthread_mutex_lock(&pMSG->mutex_msg);
	list_add_tail(&cur->list, &pMSG->head_msg.list);
	pthread_mutex_unlock(&pMSG->mutex_msg);

	pthread_cond_broadcast(&pMSG->cond_msg);

	return 0;
}

int csv_msg_check (uint8_t *buf, uint32_t len)
{
	if ((NULL == buf)||(len < sizeof(struct msg_head_t))) {
		return -1;
	}

	int ret = 0;
	int len_need = 0;
	struct msg_head_t *pHDR = NULL;
	pHDR = (struct msg_head_t *)buf;

	log_debug("MSG: 0x%08X, len %d, code %d", pHDR->cmdtype, pHDR->length, pHDR->result);

	if (pHDR->length > 0) {
		len_need = pHDR->length - (len - sizeof(struct msg_head_t));
	}

	if (len_need > 0) {
		ret = csv_tcp_local_recv(buf+len, len_need);
		if (ret <= 0) {
			log_info("ERROR : recv %d", ret);
			return -1;
		}
	}

	ret = csv_msg_push(&gCSV->msg, buf, len);

	return ret;
}

static int csv_msg_execute (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	struct list_head *pos = NULL;
	struct msg_command_list *task = NULL;
	struct msg_command_t *pCMD = NULL;

//	log_debug("msg 0x%08X", pMP->hdr.cmdtype);

	list_for_each_prev(pos, &gCSV->msg.head_cmd.list) {
		task = list_entry(pos, struct msg_command_list, list);
		if (task == NULL) {
			break;
		}

		pCMD = &task->command;

		if (pMP->hdr.cmdtype == pCMD->cmdtype) {
			log_debug("match '%s'", pCMD->cmdname);

			return pCMD->func(pMP, pACK);
		}
	}

	log_debug("WARN : unknown msg 0x%08X", pMP->hdr.cmdtype);

	return -1;
}

static void *csv_msg_loop (void *data)
{
	if (NULL == data) {
		return NULL;
	}

	struct csv_msg_t *pMSG = (struct csv_msg_t *)data;

	int ret = 0;

	struct list_head *pos = NULL, *n = NULL;
	struct msglist_t *task = NULL;
	struct timeval now;
	struct timespec timeo;
	struct msg_package_t *pMP = NULL;

	while (1) {
		list_for_each_safe(pos, n, &pMSG->head_msg.list) {
			task = list_entry(pos, struct msglist_t, list);
			if (task == NULL) {
				break;
			}

			pMP = &task->mp;

			ret = csv_msg_execute(pMP, &pMSG->ack);
			if (ret < 0) {
				// error msg
			}

			if (NULL != pMP->payload) {
				free(pMP->payload);
				pMP->payload = NULL;
			}

			list_del(&task->list);
			free(task);
			task = NULL;
		}


		gettimeofday(&now, NULL);
		timeo.tv_sec = now.tv_sec + 5;
		timeo.tv_nsec = now.tv_usec * 1000;

		ret = pthread_cond_timedwait(&pMSG->cond_msg, &pMSG->mutex_msg, &timeo);
		if (ret == ETIMEDOUT) {
			// use timeo as a block and than retry.
		}
	}

	log_info("WARN : exit pthread %s", pMSG->name_msg);

	pMSG->thr_msg = 0;

	pthread_exit(NULL);

	return NULL;
}

static int csv_msg_thread (struct csv_msg_t *pMSG)
{
	int ret = -1;

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pMSG->mutex_msg, NULL) != 0) {
		log_err("ERROR : mutex %s", pMSG->name_msg);
        return -1;
    }

    if (pthread_cond_init(&pMSG->cond_msg, NULL) != 0) {
		log_err("ERROR : cond %s", pMSG->name_msg);
        return -1;
    }

	ret = pthread_create(&pMSG->thr_msg, &attr, csv_msg_loop, (void *)pMSG);
	if (ret < 0) {
		log_err("ERROR : create pthread %s", pMSG->name_msg);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p)", pMSG->name_msg, pMSG->thr_msg);
	}

	//pthread_attr_destory(&attr);

	return ret;
}

static void csv_msg_list_release (struct csv_msg_t *pMSG)
{
	struct list_head *pos = NULL, *n = NULL;
	struct msglist_t *task = NULL;

	list_for_each_safe(pos, n, &pMSG->head_msg.list) {
		task = list_entry(pos, struct msglist_t, list);
		if (task == NULL) {
			break;
		}

		if (NULL != task->mp.payload) {
			free(task->mp.payload);
		}

		list_del(&task->list);
		free(task);
		task = NULL;
	}
}

static int csv_msg_thread_cancel (struct csv_msg_t *pMSG)
{
	int ret = 0;
	void *retval = NULL;

	if (pMSG->thr_msg <= 0) {
		return 0;
	}

	ret = pthread_cancel(pMSG->thr_msg);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s", pMSG->name_msg);
	} else {
		log_info("OK : cancel pthread %s", pMSG->name_msg);
	}

	ret = pthread_join(pMSG->thr_msg, &retval);

	return ret;
}


int csv_msg_init (void)
{
	struct csv_msg_t *pMSG = &gCSV->msg;

	pMSG->ack.len_send = 0;
	pMSG->name_msg = NAME_THREAD_MSG;

	INIT_LIST_HEAD(&pMSG->head_cmd.list);
	INIT_LIST_HEAD(&pMSG->head_msg.list);

	csv_msg_cmd_register(pMSG);

	return csv_msg_thread(pMSG);
}


int csv_msg_deinit (void)
{
	int ret = 0;
	struct csv_msg_t *pMSG = &gCSV->msg;

	csv_msg_list_release(pMSG);

	csv_msg_cmd_unregister(pMSG);

	ret = csv_msg_thread_cancel(pMSG);

	return ret;
}


#ifdef __cplusplus
}
#endif


