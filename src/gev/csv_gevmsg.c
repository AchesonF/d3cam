#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static int csv_gevmsg_sendto (struct csv_gevmsg_t *pGVMSG, uint8_t *txbuf, uint32_t txlen)
{
	if ((pGVMSG->fd <= 0)||(txlen == 0)
	  ||(0x00000000 == pGVMSG->peer_addr.sin_addr.s_addr)
	  ||(0x0000 == pGVMSG->peer_addr.sin_port)) {
		return 0;
	}

	log_hex(STREAM_UDP, txbuf, txlen, "gvmsg send[%d] '%s:%d'", txlen, 
		inet_ntoa(pGVMSG->peer_addr.sin_addr), htons(pGVMSG->peer_addr.sin_port));

	return sendto(pGVMSG->fd, txbuf, txlen, 0, 
		(struct sockaddr *)&pGVMSG->peer_addr, sizeof(struct sockaddr_in));
}

int csv_gevmsg_package (struct csv_gevmsg_t *pGVMSG)
{


	pGVMSG->ReqId = (pGVMSG->ReqId+GVCP_REQ_ID_INIT)/0xFFFF+GVCP_REQ_ID_INIT;

	return 0;
}

int csv_gevmsg_push (struct csv_gevmsg_t *pGVMSG, uint8_t *buf, uint32_t len)
{
	if ((NULL == pGVMSG)||(NULL == buf)||(len < GVCP_GVCP_HEADER_LEN)) {
		return -1;
	}

	struct gevmsg_list_t *cur = NULL;
	struct message_info_t *pMI = NULL;

	cur = (struct gevmsg_list_t *)malloc(sizeof(struct gevmsg_list_t));
	if (cur == NULL) {
		log_err("ERROR : malloc gevmsg_list_t.");
		return -1;
	}
	memset(cur, 0, sizeof(struct gevmsg_list_t));
	pMI = &cur->mi;

	pMI->acked = 0;
	pMI->retries = 0;
	pMI->lendata = len;
	memcpy(pMI->data, buf, len);

	pthread_mutex_lock(&pGVMSG->mutex_gevmsg);
	list_add_tail(&cur->list, &pGVMSG->head_gevmsg.list);
	pthread_mutex_unlock(&pGVMSG->mutex_gevmsg);

	pthread_cond_broadcast(&pGVMSG->cond_gevmsg);

	return 0;
}

static int csv_gevmsg_update (struct csv_gevmsg_t *pGVMSG, uint16_t RegID)
{
	struct list_head *pos = NULL;
	struct gevmsg_list_t *task = NULL;
	struct message_info_t *pMI = NULL;

	list_for_each(pos, &pGVMSG->head_gevmsg.list) {
		task = list_entry(pos, struct gevmsg_list_t, list);
		if (task == NULL) {
			break;
		}

		pMI = &task->mi;

		if (RegID == pMI->ReqId) {
			pMI->acked = 1;
			return 0;
		}
	}

	return -1;
}

int csv_gevmsg_udp_reading_trigger (struct csv_gevmsg_t *pGVMSG)
{
	int nRecv = 0;
	socklen_t from_len = sizeof(struct sockaddr_in);

	nRecv = recvfrom(pGVMSG->fd, pGVMSG->bufRecv, GVCP_MAX_MSG_LEN, 0, 
		(struct sockaddr *)&pGVMSG->from_addr, &from_len);
	if (nRecv < 0) {
		log_err("ERROR : recvfrom %s", pGVMSG->name);
		return -1;
	} else if (nRecv == 0) {
		return 0;
	}

	pGVMSG->lenRecv = nRecv;

	struct gev_conf_t *pGC = &gCSV->cfg.gigecfg;
	if (pGC->MessageAddress == pGVMSG->from_addr.sin_addr.s_addr) {
		ACK_MSG_HEADER *pHeader = (ACK_MSG_HEADER *)pGVMSG->bufRecv;
		ACK_MSG_HEADER Ackheader;

		Ackheader.nStatus = ntohs(pHeader->nStatus);
		Ackheader.nAckMsgValue = ntohs(pHeader->nAckMsgValue);
		Ackheader.nLength = ntohs(pHeader->nLength);
		Ackheader.nAckId = ntohs(pHeader->nAckId);

		if ((GEV_STATUS_SUCCESS == Ackheader.nStatus)
			&&(GEV_EVENT_ACK == Ackheader.nAckMsgValue)) {
			return csv_gevmsg_update(pGVMSG, Ackheader.nAckId);
		}
	}

	return -1;
}

static int csv_gevmsg_client_open (struct csv_gevmsg_t *pGVMSG)
{
	int ret = 0;
	int fd = -1;

	if (pGVMSG->fd > 0) {
		ret = close(pGVMSG->fd);
		if (ret < 0) {
			log_err("ERROR : close %s.", pGVMSG->name);
		}
		pGVMSG->fd = -1;
	}

	struct sockaddr_in local_addr;
	socklen_t sin_size = sizeof(struct sockaddr);

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 ) {
		log_err("ERROR : socket %s.", pGVMSG->name);

		return -1;
	}

	memset((void*)&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(0);
	bzero(&(local_addr.sin_zero), 8);

	ret = bind(fd, (struct sockaddr*)&local_addr, sizeof(struct sockaddr));
	if(ret < 0) {
		log_err("ERROR : bind %s.", pGVMSG->name);

		return -1;
	}

	int val = IP_PMTUDISC_DO; /* don't fragment */
	ret = setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));
	if (ret < 0) {
		log_err("ERROR : setsockopt 'IP_MTU_DISCOVER'.");

		return -1;
	}

	getsockname(fd, (struct sockaddr *)&local_addr, &sin_size);

	pGVMSG->fd = fd;
	pGVMSG->port = ntohs(local_addr.sin_port);

	log_info("OK : bind %s : '%d/udp' as fd(%d).", pGVMSG->name, 
		pGVMSG->port, pGVMSG->fd);

	csv_gev_reg_value_update(REG_MessageChannelSourcePort, pGVMSG->port);
	gCSV->cfg.gigecfg.MessageSourcePort = pGVMSG->port;

	return 0;
}

static int csv_gevmsg_client_close (struct csv_gevmsg_t *pGVMSG)
{
	if (NULL == pGVMSG) {
		return -1;
	}

	if (pGVMSG->fd > 0) {
		if (close(pGVMSG->fd) < 0) {
			log_err("ERROR : close %s.", pGVMSG->name);
			return -1;
		}
		log_info("OK : close %s fd(%d).", pGVMSG->name, pGVMSG->fd);
		pGVMSG->fd = -1;
	}

	return 0;
}

static void *csv_gevmsg_client_loop (void *data)
{
	if (NULL == data) {
		goto exit_thr;
	}

	int ret = 0;
	uint8_t need_clear = false;
	struct csv_gevmsg_t *pGVMSG = (struct csv_gevmsg_t *)data;
	struct timeval now;
	struct timespec timeo;
	struct list_head *pos = NULL, *n = NULL;
	struct gevmsg_list_t *task = NULL;
	struct message_info_t *pMI = NULL;
	struct gev_conf_t *pGC = &gCSV->cfg.gigecfg;

	while (gCSV->running) {
		list_for_each_safe(pos, n, &pGVMSG->head_gevmsg.list) {
			task = list_entry(pos, struct gevmsg_list_t, list);
			if (task == NULL) {
				break;
			}

			pMI = &task->mi;
			if (!pMI->acked) {
				if (0 == pGC->MessageRetryCount) {
					csv_gevmsg_sendto(pGVMSG, pMI->data, pMI->lendata);
					need_clear = true;
				} else {
					if (pMI->retries++ <= pGC->MessageRetryCount) {
						csv_gevmsg_sendto(pGVMSG, pMI->data, pMI->lendata);
					} else {
						need_clear = true;
					}
				}
			} else {
				need_clear = true;
			}

			if (need_clear) {
				list_del(&task->list);
				free(task);
				task = NULL;
			}

			if (pGC->MessageTimeout > 0) {
				usleep(pGC->MessageTimeout*1000);
			}
		}

		gettimeofday(&now, NULL);
		if ((!list_empty(&pGVMSG->head_gevmsg.list))&&(pGC->MessageTimeout > 0)) {
			timeo.tv_sec = now.tv_sec + pGC->MessageTimeout/1000;
			timeo.tv_nsec = (now.tv_usec+(pGC->MessageTimeout%1000)*1000) * 1000;
		} else {
			timeo.tv_sec = now.tv_sec + 5;
			timeo.tv_nsec = now.tv_usec * 1000;
		}

		ret = pthread_cond_timedwait(&pGVMSG->cond_gevmsg, &pGVMSG->mutex_gevmsg, &timeo);
		if (ret == ETIMEDOUT) {
			// use timeo as a block and than retry.
		}
	}

exit_thr:

	log_alert("ALERT : exit pthread %s.", pGVMSG->name_gevmsg);

	pGVMSG->thr_gevmsg = 0;

	pthread_exit(NULL);

	return NULL;
}

static int csv_gevmsg_client_thread (struct csv_gevmsg_t *pGVMSG)
{
	int ret = -1;

	if (NULL == pGVMSG) {
		return -1;
	}

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pGVMSG->mutex_gevmsg, NULL) != 0) {
		log_err("ERROR : mutex %s.", pGVMSG->name_gevmsg);
        return -1;
    }

    if (pthread_cond_init(&pGVMSG->cond_gevmsg, NULL) != 0) {
		log_err("ERROR : cond %s.", pGVMSG->name_gevmsg);
        return -1;
    }

	ret = pthread_create(&pGVMSG->thr_gevmsg, &attr, csv_gevmsg_client_loop, (void *)pGVMSG);
	if (ret < 0) {
		log_err("ERROR : create pthread %s.", pGVMSG->name_gevmsg);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p).", pGVMSG->name_gevmsg, pGVMSG->thr_gevmsg);
	}

	return ret;
}

static int csv_gevmsg_client_thread_cancel (struct csv_gevmsg_t *pGVMSG)
{
	int ret = 0;
	void *retval = NULL;

	if (pGVMSG->thr_gevmsg <= 0) {
		return 0;
	}

	ret = pthread_cancel(pGVMSG->thr_gevmsg);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s.", pGVMSG->name_gevmsg);
	} else {
		log_info("OK : cancel pthread %s (%p).", pGVMSG->name_gevmsg, pGVMSG->thr_gevmsg);
	}

	ret = pthread_join(pGVMSG->thr_gevmsg, &retval);

	pGVMSG->thr_gevmsg = 0;

	return ret;
}

int csv_gevmsg_init (void)
{
	int ret = 0;
	struct csv_gevmsg_t *pGVMSG = &gCSV->gvmsg;

	pGVMSG->fd = -1;
	pGVMSG->name = NAME_SOCKET_GEV_MESSAGE;
	pGVMSG->name_gevmsg = NAME_THREAD_GEV_MESSAGE;
	pGVMSG->ReqId = GVCP_REQ_ID_INIT;
	pGVMSG->port = 0;
	INIT_LIST_HEAD(&pGVMSG->head_gevmsg.list);

	ret = csv_gevmsg_client_open(pGVMSG);
	ret |= csv_gevmsg_client_thread(pGVMSG);

	return ret;
}

int csv_gevmsg_deinit (void)
{
	int ret = 0;
	struct csv_gevmsg_t *pGVMSG = &gCSV->gvmsg;

	ret = csv_gevmsg_client_close(pGVMSG);
	ret |= csv_gevmsg_client_thread_cancel(&gCSV->gvmsg);

	return ret;
}


#ifdef __cplusplus
}
#endif



