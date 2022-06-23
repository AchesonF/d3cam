#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


static int csv_msg_ack_package (struct msg_package_t *pMP, char *content, int len, int retcode)
{
	struct msg_send_t *pACK = &gCSV->msg.ack;

	uint8_t *pS = pACK->buf_send;
	struct msg_head_t *pHDR = (struct msg_head_t *)pS;
	pHDR->cmdtype = pMP->hdr.cmdtype;
	pHDR->length = len+sizeof(retcode); // len;
	pHDR->result = retcode;

	if (NULL != content) {
		memcpy(pS+sizeof(struct msg_head_t), content, len);
	}

	pACK->len_send = len+sizeof(struct msg_head_t);

	return pACK->len_send;
}





static int msg_cameras_enum (struct msg_package_t *pMP)
{
	int ret = 0;
	int len_msg = 0;
	char str_enums[1024] = {0};
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct msg_send_t *pACK = &gCSV->msg.ack;

	pACK->len_send = 0;

	ret = csv_mvs_cams_enum();
	if (ret <= 0) {
		csv_msg_ack_package(pMP, NULL, 0, -1);
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
			csv_msg_ack_package(pMP, str_enums, len_msg, 0);
		}
	}

	return csv_tcp_local_send(pACK->buf_send, pACK->len_send);
}

static int msg_cameras_open (struct msg_package_t *pMP)
{
	int ret = 0;
	int result = 0;
	struct msg_send_t *pACK = &gCSV->msg.ack;

	pACK->len_send = 0;

	ret = csv_mvs_cams_open();
	if (ret < 0) {
		result = -1;
	} else {
		result = 0;
	}

	csv_msg_ack_package(pMP, NULL, 0, result);

	return csv_tcp_local_send(pACK->buf_send, pACK->len_send);
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
	csv_cmd_e cmdtype, int (*func)(struct msg_package_t *pMP))
{
	struct msg_command_list *cur = NULL;

	cur = msg_command_malloc();
	if (cur != NULL) {
		cur->command.cmdtype = cmdtype;
		cur->command.func = func,

		list_add_tail(&cur->list, &pMSG->head_cmd.list);
	}
}

static void csv_msg_cmd_register (struct csv_msg_t *pMSG)
{
	msg_command_add(pMSG, CAMERA_ENMU, msg_cameras_enum);
	msg_command_add(pMSG, CAMERA_CONNECT, msg_cameras_open);



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

		memcpy(pMP->payload, buf+pHDR->length, pHDR->length);
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

static int csv_msg_execute (struct msg_package_t *pMP)
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
			return pCMD->func(pMP);
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

			ret = csv_msg_execute(pMP);
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


