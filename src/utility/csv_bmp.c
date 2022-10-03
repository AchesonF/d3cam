#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

int gray_raw2bmp (uint8_t *pRawData, uint32_t nWidth, uint32_t nHeight, char *pBmpName)
{
	struct bitmap_file_header_t file_h;
	struct bitmap_info_header_t info_h;
	uint32_t dwBmpSize = 0,dwRawSize = 0, dwLine = 0;
	int lCount = 0, i = 0;
	FILE *fp = NULL;
	struct rgbquad_t rgbPal[256];

    fp = fopen(pBmpName, "wb");
	if (fp == NULL) {
		log_err("ERROR : open '%s'.", pBmpName);

		return -1;
	}
 
    file_h.bfType = 0x4D42;
    file_h.bfReserved1 = 0;
    file_h.bfReserved2 = 0;
    file_h.bfOffBits = sizeof(rgbPal) + sizeof(file_h) + sizeof(info_h);
    info_h.biSize = sizeof(struct bitmap_info_header_t);
    info_h.biWidth = nWidth;
    info_h.biHeight = nHeight;
    info_h.biPlanes = 1;
    info_h.biBitCount = 8;
    info_h.biCompression = 0;
    info_h.biXPelsPerMeter = 0;
    info_h.biYPelsPerMeter = 0;
    info_h.biClrUsed = 0;
    info_h.biClrImportant = 0;

    dwLine = ((((info_h.biWidth * info_h.biBitCount) + 31) & ~31) >> 3);
    dwBmpSize = dwLine * info_h.biHeight;
    info_h.biSizeImage = dwBmpSize;
    file_h.bfSize = dwBmpSize + file_h.bfOffBits + 2;

    dwRawSize = info_h.biWidth * info_h.biHeight;

    if (pRawData) {
        for (i = 0; i < 256; i++) {
            rgbPal[i].rgbRed = (uint8_t)(i % 256);
            rgbPal[i].rgbGreen = rgbPal[i].rgbRed;
            rgbPal[i].rgbBlue = rgbPal[i].rgbRed;
            rgbPal[i].rgbReserved = 0;
        }

        fwrite((char*)&file_h, 1, sizeof(file_h), fp);
        fwrite((char*)&info_h, 1, sizeof(info_h), fp);
        fwrite((char*)rgbPal, 1, sizeof(rgbPal), fp);

#if 1
		// 上下颠倒
        lCount = dwRawSize;
        for (lCount -= (long)info_h.biWidth; lCount >= 0; lCount -= (long)info_h.biWidth) {
			fwrite((pRawData + lCount), 1, (long)dwLine, fp);
        }
#else
        for (lCount = 0; lCount < dwRawSize; lCount += info_h.biWidth) {
			fwrite((pRawData + lCount), 1, (long)dwLine, fp);
        }
#endif
    }

    fclose(fp);

    return 0;
}

int csv_bmp_push (char *filename, uint8_t *pRawData, 	uint32_t length, 
	uint32_t width, uint32_t height)
{
	if ((NULL == pRawData)||(NULL == filename)) {
		return -1;
	}

	struct csv_bmp_t *pBMP = &gCSV->bmp;

	struct bmp_list_t *cur = NULL;
	struct bmp_package_t *pBP = NULL;

	cur = (struct bmp_list_t *)malloc(sizeof(struct bmp_list_t));
	if (cur == NULL) {
		log_err("ERROR : malloc bmp_list_t");
		return -1;
	}
	memset(cur, 0, sizeof(struct bmp_list_t));
	pBP = &cur->bp;

	strncpy(pBP->filename, filename, MAX_LEN_FILE_NAME);
	pBP->width = width;
	pBP->height = height;
	pBP->length = length;

	if (length > 0) {
		pBP->payload = (uint8_t *)malloc(length);

		if (NULL == pBP->payload) {
			log_err("ERROR : malloc payload.");
			return -1;
		}

		memcpy(pBP->payload, pRawData, length);
	}

	pthread_mutex_lock(&pBMP->mutex_bmp);
	list_add_tail(&cur->list, &pBMP->head_bmp.list);
	pthread_mutex_unlock(&pBMP->mutex_bmp);

	return 0;
}


static void *csv_bmp_loop (void *data)
{
	if (NULL == data) {
		return NULL;
	}

	struct csv_bmp_t *pBMP = (struct csv_bmp_t *)data;
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;

	int ret = 0;

	struct list_head *pos = NULL, *n = NULL;
	struct bmp_list_t *task = NULL;
	struct timeval now;
	struct timespec timeo;
	struct bmp_package_t *pBP = NULL;

	while (1) {
		list_for_each_safe(pos, n, &pBMP->head_bmp.list) {
			task = list_entry(pos, struct bmp_list_t, list);
			if (task == NULL) {
				break;
			}

			pBP = &task->bp;

			gray_raw2bmp(pBP->payload, pBP->width, pBP->height, pBP->filename);

			if (NULL != pBP->payload) {
				free(pBP->payload);
				pBP->payload = NULL;
			}

			list_del(&task->list);
			free(task);
			task = NULL;
		}


		gettimeofday(&now, NULL);
		timeo.tv_sec = now.tv_sec + 10;
		timeo.tv_nsec = now.tv_usec * 1000;

		ret = pthread_cond_timedwait(&pBMP->cond_bmp, &pBMP->mutex_bmp, &timeo);
		if (ret == ETIMEDOUT) {
			// use timeo as a block and than retry.
		}
	}

	log_warn("WARN : exit pthread %s.", pBMP->name_bmp);

	pBMP->thr_bmp = 0;

	pthread_exit(NULL);

	return NULL;
}

static int csv_bmp_thread (struct csv_bmp_t *pBMP)
{
	int ret = -1;

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pBMP->mutex_bmp, NULL) != 0) {
		log_err("ERROR : mutex %s.", pBMP->name_bmp);
        return -1;
    }

    if (pthread_cond_init(&pBMP->cond_bmp, NULL) != 0) {
		log_err("ERROR : cond %s.", pBMP->name_bmp);
        return -1;
    }

	ret = pthread_create(&pBMP->thr_bmp, &attr, csv_bmp_loop, (void *)pBMP);
	if (ret < 0) {
		log_err("ERROR : create pthread %s.", pBMP->name_bmp);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p).", pBMP->name_bmp, pBMP->thr_bmp);
	}

	//pthread_attr_destory(&attr);

	return ret;
}

static void csv_bmp_list_release (struct csv_bmp_t *pBMP)
{
	struct list_head *pos = NULL, *n = NULL;
	struct bmp_list_t *task = NULL;

	list_for_each_safe(pos, n, &pBMP->head_bmp.list) {
		task = list_entry(pos, struct bmp_list_t, list);
		if (task == NULL) {
			break;
		}

		if (NULL != task->bp.payload) {
			free(task->bp.payload);
		}

		list_del(&task->list);
		free(task);
		task = NULL;
	}
}

static int csv_bmp_thread_cancel (struct csv_bmp_t *pBMP)
{
	int ret = 0;
	void *retval = NULL;

	if (pBMP->thr_bmp <= 0) {
		return 0;
	}

	ret = pthread_cancel(pBMP->thr_bmp);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s.", pBMP->name_bmp);
	} else {
		log_info("OK : cancel pthread %s (%p).", pBMP->name_bmp, pBMP->thr_bmp);
	}

	ret = pthread_join(pBMP->thr_bmp, &retval);

	pBMP->thr_bmp = 0;

	return ret;
}

int csv_bmp_init (void)
{
	struct csv_bmp_t *pBMP = &gCSV->bmp;

	INIT_LIST_HEAD(&pBMP->head_bmp.list);

	pBMP->name_bmp = NAME_THREAD_BMP;

	csv_bmp_thread(pBMP);

	return 0;
}

int csv_bmp_deinit (void)
{
	int ret = 0;
	struct csv_bmp_t *pBMP = &gCSV->bmp;

	csv_bmp_list_release(pBMP);

	ret = csv_bmp_thread_cancel(pBMP);

	return ret;
}

#ifdef __cplusplus
}
#endif


