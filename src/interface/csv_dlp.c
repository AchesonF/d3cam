#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static uint8_t csv_dlp_pick_code (uint8_t idx)
{
	uint8_t cmd = CMD_POINTCLOUD;

	switch (idx) {
	case DLP_CMD_CALIB:
		cmd = CMD_CALIB;
		break;
	case DLP_CMD_BRIGHT:
		cmd = CMD_BRIGHT;
		break;
	case DLP_CMD_HDRI:
		cmd = CMD_HDRI;
		break;

	case DLP_CMD_POINTCLOUD:
	default:
		cmd = CMD_POINTCLOUD;
		break;
	}

	return cmd;
}

static int csv_dlp_encode (struct csv_dlp_t *pDLP, uint8_t idx)
{
	if (idx >= TOTAL_DLP_CMDS) {
		return -1;
	}

	uint8_t cmd = 0;
	cmd = csv_dlp_pick_code(idx);

	pDLP->tbuf[0] = DLP_HEAD_A;
	pDLP->tbuf[1] = DLP_HEAD_B;
	pDLP->tbuf[2] = cmd;
	pDLP->tbuf[3] = (uint8_t)pDLP->rate;
	u16_to_u8v(swap16((uint16_t)pDLP->brightness), &pDLP->tbuf[4]);
	u32_to_u8v(swap32((uint32_t)pDLP->expoTime), &pDLP->tbuf[6]);
	pDLP->tbuf[10] = 0xAA;	// no crc check
	pDLP->tbuf[11] = 0xAA;

	pDLP->tlen = 12;

	return pDLP->tlen;
}

int csv_dlp_just_write (uint8_t idx)
{
	int ret = 0;
	struct csv_dlp_t *pDLP = &gCSV->dlp;

	if (idx >= TOTAL_DLP_CMDS) {
		return -1;
	}

	if (pDLP->fd <= 0) {
		return -1;
	}

	// update parameter before encode
	struct dlp_conf_t *pDlpcfg = &gCSV->cfg.devicecfg.dlpcfg[idx];

#if defined(USE_HK_CAMS)
	csv_mvs_cams_exposure_set(&gCSV->mvs, pDlpcfg->expoTime);
#elif defined(USE_GX_CAMS)
	csv_gx_cams_exposure_time_selector(pDlpcfg->expoTime);
#endif

	pDLP->expoTime = pDlpcfg->expoTime;
	pDLP->rate = pDlpcfg->rate;
	pDLP->brightness = pDlpcfg->brightness;

	ret = csv_dlp_encode(pDLP, idx);
	if (ret < 0) {
		return -1;
	}

	ret = csv_tty_write(pDLP->fd, pDLP->tbuf, pDLP->tlen);
	if (ret < 0) {
		log_err("ERROR : %s write failed.", pDLP->name);
		ret = csv_tty_deinit(pDLP->fd, pDLP->name);
		pDLP->fd = -1;
		ret = csv_tty_init(pDLP->dev, &pDLP->param);
		if (ret <= 0) {
			return -1;
		}
		pDLP->fd = ret;
		ret = csv_tty_write(pDLP->fd, pDLP->tbuf, pDLP->tlen);
	}

	if (ret > 0) {
		log_hex(STREAM_TTY, pDLP->tbuf, ret, "DLP write");
	}

	return ret;
}

static int csv_dlp_write (struct csv_dlp_t *pDLP, uint8_t idx)
{
	int ret = 0;

	if (pDLP->fd <= 0) {
		return -1;
	}

	ret = csv_dlp_encode(pDLP, idx);
	if (ret < 0) {
		return -1;
	}

	ret = csv_tty_write(pDLP->fd, pDLP->tbuf, pDLP->tlen);
	if (ret < 0) {
		log_err("ERROR : %s write failed.", pDLP->name);
		return -1;
	}

	if (ret > 0) {
		log_hex(STREAM_TTY, pDLP->tbuf, ret, "DLP write");
	}

	return ret;
}

static int csv_dlp_read (struct csv_dlp_t *pDLP)
{
	int ret = 0;

	pDLP->rlen = 0;

	if (pDLP->fd <= 0) {
		return -1;
	}

	ret = csv_tty_read(pDLP->fd, pDLP->rbuf, SIZE_DLP_BUFF);
	if (ret < 0) {
		log_err("ERROR : %s read failed.", pDLP->name);
		return -1;
	}

	pDLP->rlen = ret;

	if (ret > 0) {
		log_hex(STREAM_TTY, pDLP->rbuf, ret, "DLP read");
	}

	return ret;
}

int csv_dlp_write_and_read (uint8_t idx)
{
	int ret = 0;
	int cnt_snd = 0;
	struct csv_dlp_t *pDLP = &gCSV->dlp;
	struct timeval now;
	struct timespec timeo;

	gettimeofday(&now, NULL);
	timeo.tv_sec = now.tv_sec + 1;		// wait 1s
	timeo.tv_nsec = now.tv_usec * 1000;

	do {
		ret = csv_dlp_write(pDLP, idx);
	} while ((ret < 0)&&(++cnt_snd < 3));

	if (ret < 0) {
		return -1;
	}

	ret = pthread_cond_timedwait(&pDLP->cond_dlp, &pDLP->mutex_dlp, &timeo);
	if (ret == ETIMEDOUT) {
		log_warn("ERROR : dlp read timeo.");
		return -1;
	}

	ret = -1;
	if (pDLP->rlen == LEN_DLP_CTRL+2) { // need to add checksum
		switch (pDLP->rbuf[0]) {
		case 0x01:
			log_info("OK : dlp cmd ctrl.");
			ret = 0;
			break;
		case 0x02:
			log_warn("ERROR : dlp cannot trigger out.");
			break;
		case 0x03:
			log_warn("ERROR : dlp crc.");
			break;
		case 0x04:
			log_warn("ERROR : dlp light.");
			break;
		case 0x05:
			log_warn("ERROR : dlp device info.");
			break;
		case 0x06:
			log_warn("ERROR : dlp param.");
			break;
		}
	}

	return ret;
}

static void *csv_dlp_loop (void *data)
{
	if (NULL == data) {
		return NULL;
	}

	int ret = 0;

	struct csv_dlp_t *pDLP = (struct csv_dlp_t *)data;

	while (gCSV->running) {
		ret = csv_dlp_read(pDLP);

		if (ret <= 0) {
			continue;
		}

		if (strstr((char *)pDLP->rbuf, "alive")) {
			// dlp alive. do nothing
		} else {
			pthread_cond_broadcast(&pDLP->cond_dlp);
		}
	}

	log_alert("ALERT : exit pthread %s.", pDLP->name_dlp);

	pDLP->thr_dlp = 0;

	pthread_exit(NULL);

	return NULL;
}

static int csv_dlp_thread (struct csv_dlp_t *pDLP)
{
	int ret = -1;

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pDLP->mutex_dlp, NULL) != 0) {
		log_err("ERROR : mutex %s.", pDLP->name_dlp);
        return -1;
    }

    if (pthread_cond_init(&pDLP->cond_dlp, NULL) != 0) {
		log_err("ERROR : cond %s.", pDLP->name_dlp);
        return -1;
    }

	ret = pthread_create(&pDLP->thr_dlp, &attr, csv_dlp_loop, (void *)pDLP);
	if (ret < 0) {
		log_err("ERROR : create pthread %s.", pDLP->name_dlp);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p).", pDLP->name_dlp, pDLP->thr_dlp);
	}

	return ret;
}

static int csv_dlp_thread_cancel (struct csv_dlp_t *pDLP)
{
	int ret = 0;
	void *retval = NULL;

	if (pDLP->thr_dlp <= 0) {
		return 0;
	}

	ret = pthread_cancel(pDLP->thr_dlp);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s.", pDLP->name_dlp);
	} else {
		log_info("OK : cancel pthread %s (%p).", pDLP->name_dlp, pDLP->thr_dlp);
	}

	ret = pthread_join(pDLP->thr_dlp, &retval);

	pDLP->thr_dlp = 0;

	return ret;
}

int csv_dlp_init (void)
{
	int fd = 0;
	struct csv_dlp_t *pDLP = &gCSV->dlp;
	struct csv_tty_param_t *pParam = &pDLP->param;
	struct dlp_conf_t *pDlpcfg = &gCSV->cfg.devicecfg.dlpcfg[DLP_CMD_POINTCLOUD];

	pDLP->dev = DEV_TTY_DLP;
	pDLP->name = NAME_DEV_DLP;
	pDLP->fd = -1;
	pDLP->rlen = 0;
	pDLP->name_dlp = NAME_THREAD_DLP;

	pParam->baudrate = DEFAULT_DLP_BAUDRATE;
	pParam->databits = 8;
	pParam->stopbits = 1;
	pParam->parity = 'n';
	pParam->flowcontrol = 0;
	pParam->delay = 1;

	pDLP->rate = pDlpcfg->rate;
	pDLP->brightness = pDlpcfg->brightness;
	pDLP->expoTime = pDlpcfg->expoTime;

	fd = csv_tty_init(pDLP->dev, pParam);
	if (fd <= 0) {
		log_warn("ERROR : init %s.", pDLP->name);
		return -1;
	}

	pDLP->fd = fd;

	log_info("OK : init %s as fd(%d).", pDLP->name, pDLP->fd);

	return csv_dlp_thread(pDLP);
}


int csv_dlp_deinit (void)
{
	int ret = 0;
	struct csv_dlp_t *pDLP = &gCSV->dlp;

	ret = csv_dlp_thread_cancel(pDLP);
	ret = csv_tty_deinit(pDLP->fd, pDLP->name);

	if (0 == ret) {
		pDLP->fd = -1;
	}

	return ret;
}


#ifdef __cplusplus
}
#endif


