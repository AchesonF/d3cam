#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


int csv_msg_ack_package (struct msg_package_t *pMP, struct msg_ack_t *pACK, 
	char *content, int len, int retcode)
{
	pACK->len_send = len+sizeof(struct msg_head_t);
	pACK->buf_send = (uint8_t *)malloc(pACK->len_send + 1);
	if (NULL == pACK->buf_send) {
		log_err("ERROR : malloc send.");
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

static struct msg_command_list *msg_command_malloc (void)
{
	struct msg_command_list *cur = NULL;

	cur = (struct msg_command_list *)malloc(sizeof(struct msg_command_list));
	if (cur == NULL) {
		log_err("ERROR : malloc msg_command_list.");
		return NULL;
	}
	memset(cur, 0, sizeof(struct msg_command_list));

	return cur;
}

void msg_command_add (csv_cmd_e cmdtype, char *cmdname, 
	int (*func)(struct msg_package_t *pMP, struct msg_ack_t *pACK))
{
	struct csv_msg_t *pMSG = &gCSV->msg;
	struct msg_command_list *cur = NULL;

	cur = msg_command_malloc();
	if (cur != NULL) {
		cur->command.cmdtype = cmdtype;
		cur->command.cmdname = cmdname;
		cur->command.func = func;

		list_add_tail(&cur->list, &pMSG->head_cmd.list);
	}
}

static void csv_msg_cmd_enroll (void)
{
	csv_general_msg_cmd_enroll();

	csv_hk_msg_cmd_enroll();
	csv_gx_msg_cmd_enroll();

	log_info("OK : 'enroll' message commands.");
}

static void csv_msg_cmd_disroll (void)
{
	struct csv_msg_t *pMSG = &gCSV->msg;
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

	log_info("OK : 'disroll' message commands.");
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
		log_err("ERROR : malloc msglist_t.");
		return -1;
	}
	memset(cur, 0, sizeof(struct msglist_t));
	pMP = &cur->mp;

	memcpy(&pMP->hdr, pHDR, sizeof(struct msg_head_t));
	if (pHDR->length > 0) {
		pMP->payload = (uint8_t *)malloc(pHDR->length);

		if (NULL == pMP->payload) {
			log_err("ERROR : malloc payload.");
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

	log_debug("MSG[0x%08X] + %d", pHDR->cmdtype, pHDR->length);

	if (pHDR->length > 0) {
		len_need = pHDR->length - (len - sizeof(struct msg_head_t));
	}

	if (len_need > 0) {
		ret = csv_tcp_local_recv(buf+len, len_need);
		if (ret == -1) {
			log_err("ERROR : recv (%d)", ret);
			return -1;
		} else if (ret == 0) {
			log_warn("WARN : recv end-of-file.");
			return -2;
		} else if (ret < 0) {
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

	list_for_each(pos, &gCSV->msg.head_cmd.list) {
		task = list_entry(pos, struct msg_command_list, list);
		if (task == NULL) {
			break;
		}

		pCMD = &task->command;

		if (pMP->hdr.cmdtype == pCMD->cmdtype) {
			log_debug("deal '%s'", pCMD->cmdname);

			return pCMD->func(pMP, pACK);
		}
	}

	log_debug("WARN : unknown msg[0x%08X].", pMP->hdr.cmdtype);

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

	log_alert("ALERT : exit pthread %s", pMSG->name_msg);

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
		log_err("ERROR : mutex %s.", pMSG->name_msg);
        return -1;
    }

    if (pthread_cond_init(&pMSG->cond_msg, NULL) != 0) {
		log_err("ERROR : cond %s.", pMSG->name_msg);
        return -1;
    }

	ret = pthread_create(&pMSG->thr_msg, &attr, csv_msg_loop, (void *)pMSG);
	if (ret < 0) {
		log_err("ERROR : create pthread %s.", pMSG->name_msg);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p).", pMSG->name_msg, pMSG->thr_msg);
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

	csv_msg_list_release(pMSG);

	if (pMSG->thr_msg <= 0) {
		return 0;
	}

	ret = pthread_cancel(pMSG->thr_msg);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s.", pMSG->name_msg);
	} else {
		log_info("OK : cancel pthread %s (%p).", pMSG->name_msg, pMSG->thr_msg);
	}

	ret = pthread_join(pMSG->thr_msg, &retval);

	pMSG->thr_msg = 0;

	return ret;
}


int csv_msg_init (void)
{
	struct csv_msg_t *pMSG = &gCSV->msg;

	pMSG->ack.len_send = 0;
	pMSG->name_msg = NAME_THREAD_MSG;

	INIT_LIST_HEAD(&pMSG->head_cmd.list);
	INIT_LIST_HEAD(&pMSG->head_msg.list);

	csv_msg_cmd_enroll();

	return csv_msg_thread(pMSG);
}


int csv_msg_deinit (void)
{
	int ret = 0;
	struct csv_msg_t *pMSG = &gCSV->msg;

	csv_msg_cmd_disroll();

	ret = csv_msg_thread_cancel(pMSG);

	return ret;
}


#ifdef __cplusplus
}
#endif


