#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


static uint8_t dlp_ctrl_cmd[TOTAL_DLP_CMD][LEN_DLP_CTRL] = {
	{0x36, 0xAA, 0x01, 0x2D, 0xFF, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0xAA, 0xAA},	// PINSTRIPE
	{0x36, 0xAA, 0x02, 0x2D, 0xFF, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0xAA, 0xAA},	// DEMARCATE
	{0x36, 0xAA, 0x03, 0x2D, 0xFF, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0xAA, 0xAA},	// WIDISTRIPE
	{0x36, 0xAA, 0x04, 0x2D, 0xFF, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0xAA, 0xAA},	// FOCUS
	{0x36, 0xAA, 0x05, 0x2D, 0xFF, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0xAA, 0xAA},	// BRIGHT
	{0x36, 0xAA, 0x06, 0x2D, 0xFF, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0xAA, 0xAA},	// SINGLE_SINE
	{0x36, 0xAA, 0x07, 0x2D, 0xFF, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0xAA, 0xAA},	// SINGLE_WIDESTRIPE_SINE
	{0x36, 0xAA, 0x21, 0x04, 0xFF, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0xAA, 0xAA},	// FOCUS_BRIGHT
	{0x36, 0xAA, 0x21, 0x05, 0xFF, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0xAA, 0xAA},	// LIGHT_BRIGHT
	{0x36, 0xAA, 0x21, 0x06, 0xFF, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0xAA, 0xAA},	// PINSTRIPE_SINE_BRIGHT
	{0x36, 0xAA, 0x21, 0x07, 0xFF, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0xAA, 0xAA}	// WIDESTRIPE_SINE_BRIGHT
};

static int csv_dlp_write (struct csv_dlp_t *pDLP, uint8_t idx)
{
	int ret = 0;

	if (pDLP->fd <= 0) {
		return -1;
	}

	ret = csv_tty_write(pDLP->fd, dlp_ctrl_cmd[idx], sizeof(dlp_ctrl_cmd[idx]));
	if (ret < 0) {
		log_err("ERROR : %s write failed.", pDLP->name);
		return -1;
	}

	log_hex(STREAM_TTY, dlp_ctrl_cmd[idx], ret, "DLP write");

	return ret;
}

int csv_dlp_read (struct csv_dlp_t *pDLP)
{
	int ret = 0;

	if (pDLP->fd <= 0) {
		return -1;
	}

	ret = csv_tty_read(pDLP->fd, pDLP->rbuf, SIZE_DLP_BUFF);
	if (ret < 0) {
		log_err("ERROR : %s read failed.", pDLP->name);
		return -1;
	}

	pDLP->rlen = ret;

	log_hex(STREAM_TTY, pDLP->rbuf, ret, "DLP read");

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
	} while ((ret < 0)||(++cnt_snd < 3));

	if (ret < 0) {
		return -1;
	}

	ret = pthread_cond_timedwait(&pDLP->cond_dlp, &pDLP->mutex_dlp, &timeo);
	if (ret == ETIMEDOUT) {
		log_info("ERROR : dlp read timeo.");
		return -1;
	}

	ret = -1;
	if (pDLP->rlen == LEN_DLP_CTRL+2) { // need to add checksum
		switch (pDLP->rbuf[0]) {
		case 0x01:
			log_info("OK : dlp cmd ctrl");
			ret = 0;
			break;
		case 0x02:
			log_info("ERROR : dlp cannot trigger out");
			break;
		case 0x03:
			log_info("ERROR : dlp crc");
			break;
		case 0x04:
			log_info("ERROR : dlp light");
			break;
		case 0x05:
			log_info("ERROR : dlp device info");
			break;
		case 0x06:
			log_info("ERROR : dlp param");
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

	struct csv_dlp_t *pDLP = (struct csv_dlp_t *)data;

	while (1) {
		csv_dlp_read(pDLP);

		if (strstr(pDLP->rbuf, "alive")) {
			// dlp alive. do nothing
		} else {
			pthread_cond_broadcast(&pDLP->cond_dlp);
		}
	}

	log_info("WARN : exit pthread %s", pDLP->name_dlp);

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
		log_err("ERROR : mutex %s", pDLP->name_dlp);
        return -1;
    }

    if (pthread_cond_init(&pDLP->cond_dlp, NULL) != 0) {
		log_err("ERROR : cond %s", pDLP->name_dlp);
        return -1;
    }

	ret = pthread_create(&pDLP->thr_dlp, &attr, csv_dlp_loop, (void *)pDLP);
	if (ret < 0) {
		log_err("ERROR : create pthread %s", pDLP->name_dlp);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p)", pDLP->name_dlp, pDLP->thr_dlp);
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
		log_err("ERROR : pthread_cancel %s", pDLP->name_dlp);
	} else {
		log_info("OK : cancel pthread %s", pDLP->name_dlp);
	}

	ret = pthread_join(pDLP->thr_dlp, &retval);

	return ret;
}

int csv_dlp_init (void)
{
	int ret = 0;
	struct csv_dlp_t *pDLP = &gCSV->dlp;
	struct csv_tty_param_t *pParam = &pDLP->param;


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

	ret = csv_tty_init(pDLP->dev, pParam);
	if (ret <= 0) {
		log_info("ERROR : init %s.", pDLP->name);
		return -1;
	}

	pDLP->fd = ret;

	log_info("OK : init %s as fd(%d).", pDLP->name, pDLP->fd);

	return csv_dlp_thread(pDLP);
}


int csv_dlp_deinit (void)
{
	int ret = 0;
	struct csv_dlp_t *pDLP = &gCSV->dlp;

	ret = csv_dlp_thread_cancel(pDLP);

	if (pDLP->fd > 0) {
		if (close(pDLP->fd) < 0) {
			log_err("ERROR : close %s : fd(%d) failed.", pDLP->name, pDLP->fd);
			return -1;
		}

		pDLP->fd = -1;
		log_info("OK : close %s : fd(%d).", pDLP->name, pDLP->fd);
	}

	return ret;
}


#ifdef __cplusplus
}
#endif


