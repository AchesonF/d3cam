#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


// image: csv_gvsp_data_fetch(pStream, GVSP_PT_UNCOMPRESSED_IMAGE, pData, Length, imgInfo, NULL);
// raw: csv_gvsp_data_fetch(pStream, GVSP_PT_RAW_DATA, pData, Length, NULL, NULL);
// file: csv_gvsp_data_fetch(pStream, GVSP_PT_FILE, pData, Length, NULL, filename);

int csv_gvsp_data_fetch (struct gvsp_stream_t *pStream, uint16_t type,
	uint8_t *pData, uint64_t Length, PGX_FRAME_BUFFER imgInfo, char *filename)
{
	struct stream_list_t *cur = NULL;
	struct payload_data_t *pPD = NULL;

	switch (type) {
	case GVSP_PT_UNCOMPRESSED_IMAGE:
	case GVSP_PT_RAW_DATA:
	case GVSP_PT_FILE:
		break;
	default:
		return -1;
	}

	cur = (struct stream_list_t *)malloc(sizeof(struct stream_list_t));
	if (cur == NULL) {
		log_err("ERROR : malloc stream_list_t.");
		return -1;
	}
	memset(cur, 0, sizeof(struct stream_list_t));
	pPD = &cur->pd;

	pPD->payload_type = type;
	pPD->timestamp = utility_get_millisecond();
	pPD->payload_size = Length;

	switch (type) {
	case GVSP_PT_UNCOMPRESSED_IMAGE:
		pPD->pixelformat = (uint32_t)imgInfo->nPixelFormat;
		pPD->width = imgInfo->nWidth;
		pPD->height = imgInfo->nHeight;
		pPD->offset_x = imgInfo->nOffsetX;
		pPD->offset_y = imgInfo->nOffsetY;
		pPD->padding_x = 0;
		pPD->padding_y = 0;
		break;
	case GVSP_PT_RAW_DATA:
		break;
	case GVSP_PT_FILE:
		strcpy(pPD->filename, filename);
		break;
	}

	if (Length > 0) {
		pPD->payload = (uint8_t *)malloc(Length);
		if (NULL == pPD->payload) {
			log_err("ERROR : malloc payload. [%d].", Length);
			free(cur);
			return -1;
		}

		memcpy(pPD->payload, pData, Length);
	}

	pthread_mutex_lock(&pStream->mutex_stream);
	list_add_tail(&cur->list, &pStream->head_stream.list);
	pthread_mutex_unlock(&pStream->mutex_stream);

	pthread_cond_broadcast(&pStream->cond_stream);

	return 0;
}

static int csv_gvsp_sendto (int fd, struct sockaddr_in *peer, uint8_t *txbuf, uint32_t txlen)
{
	if ((fd <= 0)||(txlen == 0)
	  ||(0x00000000 == peer->sin_addr.s_addr)||(0x0000 == peer->sin_port)) {
		return 0;
	}

	log_hex(STREAM_UDP, txbuf, txlen, "gvsp send[%d] '%s:%d'", txlen, 
		inet_ntoa(peer->sin_addr), htons(peer->sin_port));

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

static int csv_gvsp_data_packet_leader (struct gvsp_stream_t *pStream, 
	struct payload_data_t *pPD)
{
	uint8_t len_hdr = 0;

#if USE_GVSP_EI_FLAG
	GVSP_PACKET_HEADER_V2 *pHdr2 = (GVSP_PACKET_HEADER_V2 *)pStream->bufSend;
	pHdr2->status			= htons(GEV_STATUS_SUCCESS);
	pHdr2->blockid_flag		= htons(0);	// resend flag ~bit15
	pHdr2->packet_format	= htonl((1<<31)|(GVSP_PACKET_FMT_LEADER<<24)); // EI=1 & Leader
	pHdr2->block_id_high	= htonl((pStream->block_id64>>32)&0xFFFFFFFF);
	pHdr2->block_id_low		= htonl(pStream->block_id64&0xFFFFFFFF);
	pHdr2->packet_id		= htonl(pStream->packet_id32);

	len_hdr = sizeof(GVSP_PACKET_HEADER_V2);
#else
	GVSP_PACKET_HEADER_V1 *pHdr1 = (GVSP_PACKET_HEADER_V1 *)pStream->bufSend;
	pHdr1->status			= htons(GEV_STATUS_SUCCESS);
	pHdr1->block_id			= htons(pStream->block_id);
	pHdr1->packet_fmt_id	= htonl((GVSP_PACKET_FMT_LEADER<<24)|(pStream->packet_id&GVSP_PACKET_ID_MAX));

	len_hdr = sizeof(GVSP_PACKET_HEADER_V1);
#endif

	switch (pPD->payload_type) {
	case GVSP_PT_UNCOMPRESSED_IMAGE: {
		    GVSP_IMAGE_DATA_LEADER* pDataLeader = (GVSP_IMAGE_DATA_LEADER*)(pStream->bufSend + len_hdr);
			pDataLeader->reserved		= 0;
		    pDataLeader->payload_type   = htons(GVSP_PT_UNCOMPRESSED_IMAGE);
		    pDataLeader->timestamp_high = htonl((pPD->timestamp>>32)&0xFFFFFFFF);
		    pDataLeader->timestamp_low  = htonl(pPD->timestamp&0xFFFFFFFF);
		    pDataLeader->pixel_format   = htonl(PixelType_Gvsp_Mono8);
		    pDataLeader->size_x         = htonl(pPD->width);
		    pDataLeader->size_y         = htonl(pPD->height);
		    pDataLeader->offset_x       = htonl(0);
		    pDataLeader->offset_y       = htonl(0);
		    pDataLeader->padding_x      = htons(0);
		    pDataLeader->padding_y      = htons(0);

		    pStream->lenSend = len_hdr + sizeof(GVSP_IMAGE_DATA_LEADER);
		}
		break;

	case GVSP_PT_RAW_DATA: {
		    GVSP_RAW_DATA_LEADER* pDataLeader = (GVSP_RAW_DATA_LEADER*)(pStream->bufSend + len_hdr);
		    pDataLeader->reserved       = 0;
		    pDataLeader->payload_type   = htons(GVSP_PT_RAW_DATA);
		    pDataLeader->timestamp_high = htonl((pPD->timestamp>>32)&0xFFFFFFFF);
		    pDataLeader->timestamp_low  = htonl(pPD->timestamp&0xFFFFFFFF);
		    pDataLeader->payload_size_high	= htonl((pPD->payload_size>>32)&0xFFFFFFFF);
		    pDataLeader->payload_size_low	= htonl(pPD->payload_size&0xFFFFFFFF);

		    pStream->lenSend = len_hdr + sizeof(GVSP_RAW_DATA_LEADER);
		}
		break;

	case GVSP_PT_FILE: {
		    GVSP_FILE_DATA_LEADER* pDataLeader = (GVSP_FILE_DATA_LEADER*)(pStream->bufSend + len_hdr);
		    pDataLeader->reserved       = 0;
		    pDataLeader->payload_type   = htons(GVSP_PT_FILE);
		    pDataLeader->timestamp_high = htonl((pPD->timestamp>>32)&0xFFFFFFFF);
		    pDataLeader->timestamp_low  = htonl(pPD->timestamp&0xFFFFFFFF);
		    pDataLeader->payload_size_high	= htonl((pPD->payload_size>>32)&0xFFFFFFFF);
		    pDataLeader->payload_size_low	= htonl(pPD->payload_size&0xFFFFFFFF);
			memset(pDataLeader->filename, 0, MAX_SIZE_FILENAME);
			strcpy(pDataLeader->filename, pPD->filename);

		    pStream->lenSend = len_hdr + sizeof(GVSP_FILE_DATA_LEADER);
		}
		break;

	default:
		return -1;
	}

	return csv_gvsp_sendto(pStream->fd, &pStream->peer_addr, pStream->bufSend, pStream->lenSend);
}

static int csv_gvsp_data_packet_payload (struct gvsp_stream_t *pStream, 
	uint8_t *pData, uint32_t length)
{
	uint8_t len_hdr = 0;

#if USE_GVSP_EI_FLAG
	GVSP_PACKET_HEADER_V2 *pHdr2 = (GVSP_PACKET_HEADER_V2 *)pStream->bufSend;
	pHdr2->status			= htons(GEV_STATUS_SUCCESS);
	pHdr2->blockid_flag		= htons(0);	// resend flag ~bit15
	pHdr2->packet_format	= htonl((1<<31)|(GVSP_PACKET_FMT_PAYLOAD_GENERIC<<24)); // EI=1 & payload
	pHdr2->block_id_high	= htonl((pStream->block_id64>>32)&0xFFFFFFFF);
	pHdr2->block_id_low		= htonl(pStream->block_id64&0xFFFFFFFF);
	pHdr2->packet_id		= htonl(pStream->packet_id32);

	len_hdr = sizeof(GVSP_PACKET_HEADER_V2);
#else
	GVSP_PACKET_HEADER_V1 *pHdr1 = (GVSP_PACKET_HEADER_V1 *)pStream->bufSend;
	pHdr1->status			= htons(GEV_STATUS_SUCCESS);
	pHdr1->block_id			= htons(pStream->block_id);
	pHdr1->packet_fmt_id	= htonl((GVSP_PACKET_FMT_PAYLOAD_GENERIC<<24)|(pStream->packet_id&GVSP_PACKET_ID_MAX));

	len_hdr = sizeof(GVSP_PACKET_HEADER_V1);
#endif

	memcpy(pStream->bufSend+len_hdr, pData, length);

	pStream->lenSend = len_hdr + length;

	return csv_gvsp_sendto(pStream->fd, &pStream->peer_addr, pStream->bufSend, pStream->lenSend);
}

static int csv_gvsp_data_packet_trailer (struct gvsp_stream_t *pStream, 
	struct payload_data_t *pPD)
{
	uint8_t len_hdr = 0;

#if USE_GVSP_EI_FLAG
	GVSP_PACKET_HEADER_V2 *pHdr2 = (GVSP_PACKET_HEADER_V2 *)pStream->bufSend;
	pHdr2->status			= htons(GEV_STATUS_SUCCESS);
	pHdr2->blockid_flag		= htons(0);	// resend flag ~bit15
	pHdr2->packet_format	= htonl((1<<31)|(GVSP_PACKET_FMT_TRAILER<<24)); // EI=1 & trailer
	pHdr2->block_id_high	= htonl((pStream->block_id64>>32)&0xFFFFFFFF);
	pHdr2->block_id_low		= htonl(pStream->block_id64&0xFFFFFFFF);
	pHdr2->packet_id		= htonl(pStream->packet_id32);

	len_hdr = sizeof(GVSP_PACKET_HEADER_V2);
#else
	GVSP_PACKET_HEADER_V1 *pHdr1 = (GVSP_PACKET_HEADER_V1 *)pStream->bufSend;
	pHdr1->status			= htons(GEV_STATUS_SUCCESS);
	pHdr1->block_id			= htons(pStream->block_id);
	pHdr1->packet_fmt_id	= htonl((GVSP_PACKET_FMT_TRAILER<<24)|(pStream->packet_id&GVSP_PACKET_ID_MAX));

	len_hdr = sizeof(GVSP_PACKET_HEADER_V1);
#endif

	switch (pPD->payload_type) {
	case GVSP_PT_UNCOMPRESSED_IMAGE: {
		    GVSP_IMAGE_DATA_TRAILER *pDataTrailer = (GVSP_IMAGE_DATA_TRAILER*)(pStream->bufSend + len_hdr);
		    pDataTrailer->reserved     = htons(0);
		    pDataTrailer->payload_type = htons(GVSP_PT_UNCOMPRESSED_IMAGE);
		    pDataTrailer->size_y       = htonl(pPD->height);

		    pStream->lenSend = len_hdr + sizeof(GVSP_IMAGE_DATA_TRAILER);
		}
		break;
	case GVSP_PT_RAW_DATA: {
		    GVSP_RAW_DATA_TRAILER *pDataTrailer = (GVSP_RAW_DATA_TRAILER*)(pStream->bufSend + len_hdr);
		    pDataTrailer->reserved     = htons(0);
		    pDataTrailer->payload_type = htons(GVSP_PT_RAW_DATA);

		    pStream->lenSend = len_hdr + sizeof(GVSP_RAW_DATA_TRAILER);
		}
		break;
	case GVSP_PT_FILE: {
		    GVSP_FILE_DATA_TRAILER *pDataTrailer = (GVSP_FILE_DATA_TRAILER*)(pStream->bufSend + len_hdr);
		    pDataTrailer->reserved     = htons(0);
		    pDataTrailer->payload_type = htons(GVSP_PT_FILE);

		    pStream->lenSend = len_hdr + sizeof(GVSP_FILE_DATA_TRAILER);
		}
		break;
	default:
		return -1;
	}

	return csv_gvsp_sendto(pStream->fd, &pStream->peer_addr, pStream->bufSend, pStream->lenSend);
}

static int csv_gvsp_data_dispatch (struct gvsp_stream_t *pStream, 
	struct payload_data_t *pPD)
{
	if ((NULL == pStream)||(NULL == pPD)) {
		return -1;
	}

	int ret = 0;
	struct channel_cfg_t *pCH = &gCSV->cfg.gigecfg.Channel[CAM_LEFT];
	uint8_t len_hdr = 0;
#if USE_GVSP_EI_FLAG
	len_hdr = sizeof(GVSP_PACKET_HEADER_V2);
#else
	len_hdr = sizeof(GVSP_PACKET_HEADER_V1);
#endif
	uint32_t packsize = (pCH->Cfg_PacketSize&0xFFFF) - 28 - len_hdr;

	uint8_t *pData = pPD->payload;
	uint64_t nLength = pPD->payload_size;

	pStream->packet_id = 0;
	pStream->packet_id32 = 0;
	ret = csv_gvsp_data_packet_leader(pStream, pPD);
	for (pData = pPD->payload; pData < pPD->payload+nLength; ) {
		pStream->packet_id++;
		pStream->packet_id32++;
		ret = csv_gvsp_data_packet_payload(pStream, pData, 
			(pData+packsize < pPD->payload+nLength) ? packsize : (nLength%packsize));
		pData += packsize;

		if (pCH->PacketDelay/1000) {
			usleep(pCH->PacketDelay/1000);
		}
	}

	pStream->packet_id++;
	pStream->packet_id32++;
	ret = csv_gvsp_data_packet_trailer(pStream, pPD);

	return ret;
}


static int csv_gvsp_client_open (struct gvsp_stream_t *pStream)
{
	int ret = 0;
	int fd = -1;

	if (pStream->fd > 0) {
		ret = close(pStream->fd);
		if (ret < 0) {
			log_err("ERROR : close %s.", pStream->name);
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

	log_info("OK : bind %s : '%d/udp' as fd(%d).", pStream->name, 
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
			log_err("ERROR : close %s.", pStream->name);
			return -1;
		}
		log_info("OK : close %s fd(%d).", pStream->name, pStream->fd);
		pStream->fd = -1;
	}

	return 0;
}

static void *csv_gvsp_client_loop (void *data)
{
	if (NULL == data) {
		goto exit_thr;
	}

	int ret = 0;
	struct gvsp_stream_t *pStream = (struct gvsp_stream_t *)data;

	if (pStream->idx >= TOTAL_CHANNELS) {
		log_warn("ERROR : over channels num.");
		goto exit_thr;
	}

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
	struct payload_data_t *pPD = NULL;

	while (gCSV->running) {
		list_for_each_safe(pos, n, &pStream->head_stream.list) {
			task = list_entry(pos, struct stream_list_t, list);
			if (task == NULL) {
				break;
			}

			pPD = &task->pd;

			csv_gvsp_data_dispatch(pStream, pPD);
			pStream->block_id++;
			pStream->block_id64++;

			if (NULL != pPD->payload) {
				free(pPD->payload);
				pPD->payload = NULL;
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

	csv_gvsp_client_close(pStream);

	pStream->thr_stream = 0;

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
		log_err("ERROR : mutex %s.", pStream->name_stream);
        return -1;
    }

    if (pthread_cond_init(&pStream->cond_stream, NULL) != 0) {
		log_err("ERROR : cond %s.", pStream->name_stream);
        return -1;
    }

	ret = pthread_create(&pStream->thr_stream, &attr, csv_gvsp_client_loop, (void *)pStream);
	if (ret < 0) {
		log_err("ERROR : create pthread %s.", pStream->name_stream);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p).", pStream->name_stream, pStream->thr_stream);
	}

	return ret;
}

static int csv_gvsp_client_thread_cancel (struct gvsp_stream_t *pStream)
{
	int ret = 0;
	void *retval = NULL;

	csv_gvsp_client_close(pStream);

	if (pStream->thr_stream <= 0) {
		return 0;
	}

	ret = pthread_cancel(pStream->thr_stream);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s.", pStream->name_stream);
	} else {
		log_info("OK : cancel pthread %s (%p).", pStream->name_stream, pStream->thr_stream);
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

	pGVSP->stream[CAM_LEFT].name = NAME_SOCKET_STREAM_CH0;
	pGVSP->stream[CAM_LEFT].name_stream = NAME_THREAD_STREAM_CH0;
	pGVSP->stream[CAM_RIGHT].name = NAME_SOCKET_STREAM_CH1;
	pGVSP->stream[CAM_RIGHT].name_stream = NAME_THREAD_STREAM_CH1;
	pGVSP->stream[CAM_DEPTH].name = NAME_SOCKET_STREAM_CH2;
	pGVSP->stream[CAM_DEPTH].name_stream = NAME_THREAD_STREAM_CH2;

	for (i = CAM_LEFT; i < TOTAL_CHANNELS; i++) {
		pStream = &pGVSP->stream[i];

		pStream->fd = -1;
		pStream->idx = i;
		pStream->block_id = 1;
		pStream->packet_id = 0;
		pStream->block_id64 = 1;
		pStream->packet_id32 = 0;
		pStream->lenSend = 0;
		INIT_LIST_HEAD(&pStream->head_stream.list);

		ret = csv_gvsp_client_thread(pStream);
	}

	return ret;
}

int csv_gvsp_deinit (void)
{
	int ret = 0, i = 0;
	struct gvsp_stream_t *pStream = NULL;

	for (i = CAM_LEFT; i < TOTAL_CHANNELS; i++) {
		pStream = &gCSV->gvsp.stream[i];
		ret |= csv_gvsp_client_thread_cancel(pStream);
	}

	return ret;
}




#ifdef __cplusplus
}
#endif


