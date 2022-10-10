#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

int csv_gvsp_cam_grab_fetch (struct gvsp_stream_t *pStream, 
	uint8_t *imgData, uint32_t imgLength, MV_FRAME_OUT_INFO_EX *imgInfo)
{
	struct stream_list_t *cur = NULL;
	struct image_info_t *pIMI = NULL;

	cur = (struct stream_list_t *)malloc(sizeof(struct stream_list_t));
	if (cur == NULL) {
		log_err("ERROR : malloc stream_list_t.");
		return -1;
	}
	memset(cur, 0, sizeof(struct stream_list_t));
	pIMI = &cur->img;

	pIMI->pixelformat = (uint32_t)imgInfo->enPixelType;
	pIMI->length = imgLength;
	pIMI->width = imgInfo->nWidth;
	pIMI->height = imgInfo->nHeight;
	pIMI->offset_x = imgInfo->nOffsetX;
	pIMI->offset_y = imgInfo->nOffsetY;
	pIMI->padding_x = 0;
	pIMI->padding_y = 0;

	if (imgLength > 0) {
		pIMI->payload = (uint8_t *)malloc(imgLength);
		if (NULL == pIMI->payload) {
			log_err("ERROR : malloc payload. [%d].", imgLength);
			return -1;
		}
	}

	pthread_mutex_lock(&pStream->mutex_stream);
	list_add_tail(&cur->list, &pStream->head_stream.list);
	pthread_mutex_unlock(&pStream->mutex_stream);

	pthread_cond_broadcast(&pStream->cond_stream);

	return 0;
}

#if defined(USE_HK_CAMS)
static int csv_gvsp_hk_cam_grab_running (struct gvsp_stream_t *pStream, struct cam_hk_spec_t *pCAM)
{
	int nRet = MV_OK;

	if ((!pCAM->opened)||(NULL == pCAM->pHandle)||pCAM->grabbing) {
		return -1;
	}

	if (!pCAM->grabbing) {
		nRet = MV_CC_SetEnumValue(pCAM->pHandle, "TriggerMode", MV_TRIGGER_MODE_OFF);
		if (MV_OK != nRet) {
			log_warn("ERROR : SetEnumValue 'TriggerMode' errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
		}

		nRet = MV_CC_StartGrabbing(pCAM->pHandle);
		if (MV_OK != nRet) {
			log_warn("ERROR : StartGrabbing errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
			return -1;
		}

		pCAM->grabbing = true;
	}

	pStream->block_id64 = 1;

	while (1) {
		if (pStream->grab_status != GRAB_STATUS_RUNNING) {
			break;
		}

		memset(&pCAM->imgInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
		nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
			pCAM->sizePayload.nCurValue, &pCAM->imgInfo, 3000);
		if (nRet != MV_OK) {
			log_warn("ERROR : CAM '%s' : GetOneFrameTimeout, errcode[0x%08X]:'%s'.", 
				pCAM->sn, nRet, strMsg(nRet));
			continue;
		}
		log_debug("OK : CAM '%s' : GetOneFrame[%d] %d x %d", pCAM->sn, 
			pCAM->imgInfo.nFrameNum, pCAM->imgInfo.nWidth, pCAM->imgInfo.nHeight);

		csv_gvsp_cam_grab_fetch(pStream, pCAM->imgData, pCAM->sizePayload.nCurValue, &pCAM->imgInfo);

	}

	if (pCAM->grabbing) {
		nRet = MV_CC_StopGrabbing(pCAM->pHandle);
		if (MV_OK != nRet) {
			log_warn("ERROR : CAM '%s' StopGrabbing errcode[0x%08X]:'%s'.", 
					pCAM->sn, nRet, strMsg(nRet));
		}

		nRet = MV_CC_SetEnumValue(pCAM->pHandle, "TriggerMode", MV_TRIGGER_MODE_ON);
		if (MV_OK != nRet) {
			log_warn("ERROR : SetEnumValue 'TriggerMode' errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
		}

		pCAM->grabbing = false;
	}

	return 0;
}
#elif defined(USE_GX_CAMS)
static int csv_gvsp_gx_cam_grab_running (struct gvsp_stream_t *pStream, struct cam_gx_spec_t *pCAM)
{

	return 0;
}
#endif
static void *csv_gvsp_cam_grab_loop (void *pData)
{
	if (NULL == pData) {
		goto exit_thr;
	}

	struct gvsp_stream_t *pStream = (struct gvsp_stream_t *)pData;

	if (pStream->idx >= TOTAL_CAMS) {
		goto exit_thr;
	}

	int ret = 0, nRet = MV_OK;
	struct timeval now;
	struct timespec timeo;

	while (1) {
		gettimeofday(&now, NULL);
		timeo.tv_sec = now.tv_sec + 5;
		timeo.tv_nsec = now.tv_usec * 1000;

		ret = pthread_cond_timedwait(&pStream->cond_gevgrab, &pStream->mutex_gevgrab, &timeo);
		if (ret == ETIMEDOUT) {
			// use timeo as a block and than retry.
		}

		sleep(1);	// add dealy to ignore quickly start_stop

		if (GRAB_STATUS_RUNNING == pStream->grab_status) {
#if defined(USE_HK_CAMS)
			nRet = csv_gvsp_hk_cam_grab_running(pStream, &gCSV->mvs.Cam[pStream->idx]);
#elif defined(USE_GX_CAMS)
			nRet = csv_gvsp_gx_cam_grab_running(pStream, &gCSV->gx.Cam[pStream->idx]);
#endif

			if (nRet == -1) {
				log_warn("ERROR : gev grab running failed.");
				pStream->grab_status = GRAB_STATUS_NONE;
			}
		}
	}

exit_thr:

	pStream->thr_gevgrab = 0;

	log_alert("ALERT : exit pthread '%s'", pStream->name_gevgrab);

	pthread_exit(NULL);

	return NULL;
}

static int csv_gvsp_cam_grab_thread (struct gvsp_stream_t *pStream)
{
	int ret = -1;

	if (NULL == pStream) {
		return -1;
	}

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pStream->mutex_gevgrab, NULL) != 0) {
		log_err("ERROR : mutex '%s'.", pStream->name_gevgrab);
        return -1;
    }

    if (pthread_cond_init(&pStream->cond_gevgrab, NULL) != 0) {
		log_err("ERROR : cond '%s'.", pStream->name_gevgrab);
        return -1;
    }

	ret = pthread_create(&pStream->thr_gevgrab, &attr, csv_gvsp_cam_grab_loop, (void *)pStream);
	if (ret < 0) {
		log_err("ERROR : create pthread '%s'.", pStream->name_gevgrab);
		pStream->grab_status = GRAB_STATUS_NONE;
		return -1;
	} else {
		log_info("OK : create pthread '%s' @ (%p).", pStream->name_gevgrab, pStream->thr_gevgrab);
	}

	return ret;
}

static int csv_gvsp_cam_grab_thread_cancel (struct gvsp_stream_t *pStream)
{
	int ret = 0;
	void *retval = NULL;

	if (pStream->thr_gevgrab <= 0) {
		return 0;
	}

	ret = pthread_cancel(pStream->thr_gevgrab);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel '%s'.", pStream->name_gevgrab);
	} else {
		log_info("OK : cancel pthread '%s' (%p).", pStream->name_gevgrab, pStream->thr_gevgrab);
	}

	ret = pthread_join(pStream->thr_gevgrab, &retval);

	pStream->thr_gevgrab = 0;
	pStream->grab_status = GRAB_STATUS_NONE;

	return ret;
}


static int csv_gvsp_sendto (int fd, struct sockaddr_in *peer, uint8_t *txbuf, uint32_t txlen)
{
	if ((fd <= 0)||(txlen == 0)
	  ||(0x00000000 == peer->sin_addr.s_addr)||(0x0000 == peer->sin_port)) {
		return 0;
	}

//	log_hex(STREAM_UDP, txbuf, txlen, "gvsp send[%d] '%s:%d'", txlen, 
//		inet_ntoa(peer->sin_addr), htons(peer->sin_port));

	return sendto(fd, txbuf, txlen, 0, (struct sockaddr *)&peer, sizeof(struct sockaddr_in));
}

int csv_gvsp_packet_test (struct gvsp_stream_t *pStream, 
	uint8_t *pData, uint32_t length)
{
    GVSP_TEST_PACKET *pHdr = (GVSP_TEST_PACKET *)pStream->bufSend;
    pHdr->dontcare16		= htons(0);
	pHdr->packetid16		= htons(0);
    pHdr->dontcare32		= htonl(0);

	memcpy(pStream->bufSend+sizeof(GVSP_TEST_PACKET), pData, length);

    pStream->lenSend = sizeof(GVSP_TEST_PACKET) + length;

	return csv_gvsp_sendto(pStream->fd, &pStream->peer_addr, pStream->bufSend, pStream->lenSend);
}

static int csv_gvsp_packet_leader (struct gvsp_stream_t *pStream, 
	struct image_info_t *pIMI)
{
    GVSP_PACKET_HEADER *pHdr = (GVSP_PACKET_HEADER *)pStream->bufSend;
    pHdr->status			= htons(GEV_STATUS_SUCCESS);
	pHdr->blockid_flag		= htons(0);	// resend flag ~bit15
    pHdr->packet_format		= htonl((1<<31)|(GVSP_PACKET_FMT_LEADER<<24)); // EI=1 & Leader
	pHdr->block_id_high		= htonl((pStream->block_id64>>32)&0xFFFFFFFF);
	pHdr->block_id_low		= htonl(pStream->block_id64&0xFFFFFFFF);
	pHdr->packet_id			= htonl(pStream->packet_id32);

    GVSP_IMAGE_DATA_LEADER* pDataLeader = (GVSP_IMAGE_DATA_LEADER*)(pStream->bufSend + sizeof(GVSP_PACKET_HEADER));
    pDataLeader->reserved       = 0;
    pDataLeader->payload_type   = htons(GVSP_PT_UNCOMPRESSED_IMAGE);
    pDataLeader->timestamp_high = htonl(0);  // TODO
    pDataLeader->timestamp_low  = htonl(0);
    pDataLeader->pixel_format   = htonl(PixelType_Gvsp_Mono8);
    pDataLeader->size_x         = htonl(pIMI->width);
    pDataLeader->size_y         = htonl(pIMI->height);
    pDataLeader->offset_x       = htonl(0);
    pDataLeader->offset_y       = htonl(0);
    pDataLeader->padding_x      = htons(0);
    pDataLeader->padding_y      = htons(0);

    pStream->lenSend = sizeof(GVSP_PACKET_HEADER) + sizeof(GVSP_IMAGE_DATA_LEADER);

	return csv_gvsp_sendto(pStream->fd, &pStream->peer_addr, pStream->bufSend, pStream->lenSend);
}

static int csv_gvsp_packet_payload (struct gvsp_stream_t *pStream, 
	uint8_t *pData, uint32_t length)
{
    GVSP_PACKET_HEADER *pHdr = (GVSP_PACKET_HEADER *)pStream->bufSend;
    pHdr->status			= htons(GEV_STATUS_SUCCESS);
	pHdr->blockid_flag		= htons(0);	// resend flag ~bit15
    pHdr->packet_format		= htonl((1<<31)|(GVSP_PACKET_FMT_LEADER<<24)); // EI=1 & Leader
	pHdr->block_id_high		= htonl((pStream->block_id64>>32)&0xFFFFFFFF);
	pHdr->block_id_low		= htonl(pStream->block_id64&0xFFFFFFFF);
	pHdr->packet_id			= htonl(pStream->packet_id32);

	memcpy(pStream->bufSend+sizeof(GVSP_PACKET_HEADER), pData, length);

    pStream->lenSend = sizeof(GVSP_PACKET_HEADER) + length;

	return csv_gvsp_sendto(pStream->fd, &pStream->peer_addr, pStream->bufSend, pStream->lenSend);
}

static int csv_gvsp_packet_trailer (struct gvsp_stream_t *pStream, 
	struct image_info_t *pIMI)
{
    GVSP_PACKET_HEADER *pHdr = (GVSP_PACKET_HEADER *)pStream->bufSend;
    pHdr->status			= htons(GEV_STATUS_SUCCESS);
	pHdr->blockid_flag		= htons(0);	// resend flag ~bit15
    pHdr->packet_format		= htonl((1<<31)|(GVSP_PACKET_FMT_LEADER<<24)); // EI=1 & Leader
	pHdr->block_id_high		= htonl((pStream->block_id64>>32)&0xFFFFFFFF);
	pHdr->block_id_low		= htonl(pStream->block_id64&0xFFFFFFFF);
	pHdr->packet_id			= htonl(pStream->packet_id32);

    GVSPIMAGEDATATRAILER *pDataTrailer = (GVSPIMAGEDATATRAILER*)(pStream->bufSend + sizeof(GVSP_PACKET_HEADER));
    pDataTrailer->reserved     = htons(0);
    pDataTrailer->payload_type = htons(GVSP_PT_UNCOMPRESSED_IMAGE);
    pDataTrailer->size_y       = htonl(pIMI->height);

    pStream->lenSend = sizeof(GVSP_PACKET_HEADER) + sizeof(GVSPIMAGEDATATRAILER);

	return csv_gvsp_sendto(pStream->fd, &pStream->peer_addr, pStream->bufSend, pStream->lenSend);
}

static int csv_gvsp_image_dispatch (struct gvsp_stream_t *pStream, 
	struct image_info_t *pIMI)
{
	if ((NULL == pStream)||(NULL == pIMI)||(pStream->idx >= TOTAL_CAMS)) {
		return -1;
	}

	int ret = 0;
	struct channel_cfg_t *pCH = &gCSV->cfg.gigecfg.Channel[pStream->idx];
	uint32_t packsize = (pCH->Cfg_PacketSize&0xFFFF) - 28 - sizeof(GVSP_PACKET_HEADER);
	uint8_t *pData = pIMI->payload;

	pStream->packet_id32 = 0;
	ret = csv_gvsp_packet_leader(pStream, pIMI);

	for (pData = pIMI->payload; pData < pIMI->payload+pIMI->length; ) {
		if (pStream->grab_status != GRAB_STATUS_RUNNING) {
			return -1;
		}

		pStream->packet_id32++;
		ret = csv_gvsp_packet_payload(pStream, pData, 
			(pData+packsize < pIMI->payload+pIMI->length) ? packsize : (pIMI->length%packsize));
		pData += packsize;

		if (pCH->PacketDelay/1000) {
			usleep(pCH->PacketDelay/1000);
		}
	}

	pStream->packet_id32++;
	ret = csv_gvsp_packet_trailer(pStream, pIMI);

	return ret;
}

static int csv_gvsp_client_open (struct gvsp_stream_t *pStream)
{
	int ret = 0;
	int fd = -1;

	if (pStream->fd > 0) {
		ret = close(pStream->fd);
		if (ret < 0) {
			log_err("ERROR : close '%s'.", pStream->name);
		}
		pStream->fd = -1;
	}

	struct sockaddr_in local_addr;
	socklen_t sin_size = sizeof(struct sockaddr);

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 ) {
		log_err("ERROR : socket %s.", pStream->name);

		return -1;
	}

	memset((void*)&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(0);
	bzero(&(local_addr.sin_zero), 8);

	ret = bind(fd, (struct sockaddr*)&local_addr, sizeof(struct sockaddr));
	if(ret < 0) {
		log_err("ERROR : bind %s.", pStream->name);

		return -1;
	}

	int val = IP_PMTUDISC_DO; /* don't fragment */
	ret = setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));
	if (ret < 0) {
		log_err("ERROR : setsockopt 'IP_MTU_DISCOVER'.");

		return -1;
	}

	int send_len = (64<<10);	// 64K
	ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &send_len, sizeof(send_len));
	if (ret < 0) {
		log_err("ERROR : setsockopt 'SO_SNDBUF'.");

		return -1;
	}

	getsockname(fd, (struct sockaddr *)&local_addr, &sin_size);

	pStream->fd = fd;
	pStream->port = ntohs(local_addr.sin_port);

	log_info("OK : bind '%s' : '%d/udp' as fd(%d).", pStream->name, 
		pStream->port, pStream->fd);

	return 0;
}

static int csv_gvsp_client_close (struct gvsp_stream_t *pStream)
{
	if (NULL == pStream) {
		return -1;
	}

	if (pStream->fd > 0) {
		if (close(pStream->fd) < 0) {
			log_err("ERROR : close '%s'.", pStream->name);
			return -1;
		}
		pStream->fd = -1;
	}

	return 0;
}

static void *csv_gvsp_client_loop (void *data)
{
	if (NULL == data) {
		goto exit_thr;
	}

	struct gvsp_stream_t *pStream = (struct gvsp_stream_t *)data;

	int ret = 0;

	ret = csv_gvsp_client_open(pStream);
	if (ret < 0) {
		goto exit_thr;
	}

	csv_gev_reg_value_update(REG_StreamChannelSourcePort0+0x40*pStream->idx, pStream->port);
	gCSV->cfg.gigecfg.Channel[pStream->idx].SourcePort = pStream->port;

	struct timeval now;
	struct timespec timeo;
	struct list_head *pos = NULL, *n = NULL;
	struct stream_list_t *task = NULL;
	struct image_info_t *pIMI = NULL;

	while (1) {
		list_for_each_safe(pos, n, &pStream->head_stream.list) {
			task = list_entry(pos, struct stream_list_t, list);
			if (task == NULL) {
				break;
			}

			pIMI = &task->img;

			if (pStream->grab_status == GRAB_STATUS_RUNNING) {
				csv_gvsp_image_dispatch(pStream, pIMI);
				pStream->block_id64++;
			}

			if (NULL != pIMI->payload) {
				free(pIMI->payload);
				pIMI->payload = NULL;
			}

			list_del(&task->list);
			free(task);
			task = NULL;
		}


		gettimeofday(&now, NULL);
		timeo.tv_sec = now.tv_sec + 5;
		timeo.tv_nsec = now.tv_usec * 1000;

		ret = pthread_cond_timedwait(&pStream->cond_stream, &pStream->mutex_stream, &timeo);
		if (ret == ETIMEDOUT) {
			// use timeo as a block and than retry.
		}
	}

exit_thr:

	log_alert("ALERT : exit pthread %s.", pStream->name_stream);

	pStream->thr_stream = 0;
	csv_gvsp_client_close(pStream);

	pthread_exit(NULL);

	return NULL;
}

static int csv_gvsp_client_thread (struct gvsp_stream_t *pStream)
{
	int ret = -1;

	if (NULL == pStream) {
		return -1;
	}

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pStream->mutex_stream, NULL) != 0) {
		log_err("ERROR : mutex '%s'.", pStream->name_stream);
        return -1;
    }

    if (pthread_cond_init(&pStream->cond_stream, NULL) != 0) {
		log_err("ERROR : cond '%s'.", pStream->name_stream);
        return -1;
    }

	ret = pthread_create(&pStream->thr_stream, &attr, csv_gvsp_client_loop, (void *)pStream);
	if (ret < 0) {
		log_err("ERROR : create pthread '%s'.", pStream->name_stream);
		return -1;
	} else {
		log_info("OK : create pthread '%s' @ (%p).", pStream->name_stream, pStream->thr_stream);
	}

	return ret;
}

static int csv_gvsp_client_thread_cancel (struct gvsp_stream_t *pStream)
{
	int ret = 0;
	void *retval = NULL;

	if (pStream->thr_stream <= 0) {
		return 0;
	}

	csv_gvsp_client_close(pStream);

	ret = pthread_cancel(pStream->thr_stream);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel '%s'.", pStream->name_stream);
	} else {
		log_info("OK : cancel pthread '%s' (%p).", pStream->name_stream, pStream->thr_stream);
	}

	ret = pthread_join(pStream->thr_stream, &retval);

	pStream->thr_stream = 0;

	return ret;
}


int csv_gvsp_init (void)
{
	int ret = 0, i = 0;
	struct csv_gvsp_t *pGVSP = &gCSV->gvsp;
	struct gvsp_stream_t *pStream = NULL;

	pGVSP->stream[CAM_LEFT].name = "stream_"toSTR(CAM_LEFT);
	pGVSP->stream[CAM_LEFT].name_stream = "thr_"toSTR(CAM_LEFT);
	pGVSP->stream[CAM_LEFT].name_gevgrab = "grab_"toSTR(CAM_LEFT);
	pGVSP->stream[CAM_RIGHT].name = "stream_"toSTR(CAM_RIGHT);
	pGVSP->stream[CAM_RIGHT].name_stream = "thr_"toSTR(CAM_RIGHT);
	pGVSP->stream[CAM_RIGHT].name_gevgrab = "grab_"toSTR(CAM_RIGHT);
	for (i = 0; i < TOTAL_CAMS; i++) {
		pStream = &pGVSP->stream[i];
		pStream->idx = i;
		pStream->fd = -1;
		pStream->grab_status = GRAB_STATUS_NONE;
		pStream->block_id64 = 1;
		pStream->packet_id32 = 0;
		INIT_LIST_HEAD(&pStream->head_stream.list);
		ret |= csv_gvsp_client_thread(pStream);
		ret |= csv_gvsp_cam_grab_thread(pStream);
	}

	return ret;
}

int csv_gvsp_deinit (void)
{
	int ret = 0, i = 0;
	struct csv_gvsp_t *pGVSP = &gCSV->gvsp;
	struct gvsp_stream_t *pStream = NULL;

	for (i = 0; i < TOTAL_CAMS; i++) {
		pStream = &pGVSP->stream[i];
		ret |= csv_gvsp_cam_grab_thread_cancel(pStream);
		ret |= csv_gvsp_client_thread_cancel(pStream);
	}

	return ret;
}




#ifdef __cplusplus
}
#endif


