#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static int gray_raw2bmp (uint8_t *pRawData, uint32_t nWidth, uint32_t nHeight, uint8_t flip, char *pBmpName)
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

		if (flip) {
			// 上下颠倒
	        lCount = dwRawSize;
	        for (lCount -= (long)info_h.biWidth; lCount >= 0; lCount -= (long)info_h.biWidth) {
				fwrite((pRawData + lCount), 1, (long)dwLine, fp);
	        }
		} else {
	        for (lCount = 0; lCount < dwRawSize; lCount += info_h.biWidth) {
				fwrite((pRawData + lCount), 1, (long)dwLine, fp);
	        }
		}
    }

    fclose(fp);

    return 0;
}

static int gray_raw2png(void *image, size_t length, uint32_t width, 
	uint32_t height, int bit_depth, char *out_file)
{
    int fmt;
    int ret = 0;
    spng_ctx *ctx = NULL;
    struct spng_ihdr ihdr = {0}; /* zero-initialize to set valid defaults */

    /* Creating an encoder context requires a flag */
    ctx = spng_ctx_new(SPNG_CTX_ENCODER);

    /* Encode to internal buffer managed by the library */
    spng_set_option(ctx, SPNG_ENCODE_TO_BUFFER, 1);

    /* Alternatively you can set an output FILE* or stream with spng_set_png_file() or spng_set_png_stream() */

    /* Set image properties, this determines the destination image format */
    ihdr.width = width;
    ihdr.height = height;
    ihdr.color_type = 0;//color_type;
    ihdr.bit_depth = bit_depth;
    /* Valid color type, bit depth combinations: https://www.w3.org/TR/2003/REC-PNG-20031110/#table111 */

    spng_set_ihdr(ctx, &ihdr);

    /* When encoding fmt is the source format */
    /* SPNG_FMT_PNG is a special value that matches the format in ihdr */
    fmt = SPNG_FMT_PNG;

    /* SPNG_ENCODE_FINALIZE will finalize the PNG with the end-of-file marker */
    ret = spng_encode_image(ctx, image, length, fmt, SPNG_ENCODE_FINALIZE);

    if(ret)
    {
        printf("spng_encode_image() error: %s\n", spng_strerror(ret));
        goto encode_error;
    }

    size_t png_size;
    void *png_buf = NULL;

    /* Get the internal buffer of the finished PNG */
    png_buf = spng_get_png_buffer(ctx, &png_size, &ret);

    if(png_buf == NULL)
    {
        printf("spng_get_png_buffer() error: %s\n", spng_strerror(ret));
    } else {
		csv_file_write_data(out_file, png_buf, png_size);
    }

    /* User owns the buffer after a successful call */
    free(png_buf);

encode_error:

    spng_ctx_free(ctx);

    return ret;
}

/* *path : 路径
group : 次数
idx : 编号
lr : 左右
suffix : 后缀类型
*img_file : 生成名
*/
int csv_img_generate_filename (char *path, uint16_t group, 
	int idx, int lr, char *img_file)
{
	if (idx == 0) {
		snprintf(img_file, 128, "%s/CSV_%03dC%d%s", path, group, 
			lr+1, gCSV->cfg.devicecfg.strSuffix);
	} else {
		snprintf(img_file, 128, "%s/CSV_%03dC%dS00P%03d%s", path, group, 
			lr+1, idx, gCSV->cfg.devicecfg.strSuffix);
	}

	log_debug("img : '%s'", img_file);

	return 0;
}

int csv_img_generate_depth_filename (char *path, uint16_t group, char *img_file)
{
	snprintf(img_file, 128, "%s/CSV_%03d_depth.png", path, group);

	log_debug("img : '%s'", img_file);

	return 0;
}

// 由 ftp-upload 发送至 tcp 连接端所在的 ftp 服务器（匿名登陆）
static int csv_img_sender (char *path, uint16_t group)
{
	char str_cmd[256] = {0};

	memset(str_cmd, 0, 256);
#if 0
	snprintf(str_cmd, 256, "ftp-upload -h %s %s/CSV_%03d*", 
		inet_ntoa(gCSV->tcpl.peer.sin_addr), path, group);
#else
	snprintf(str_cmd, 256, "ftp-upload -v -h %s %s/CSV_%03d*", 
		inet_ntoa(gCSV->tcpl.peer.sin_addr), path, group);
#endif

	return system(str_cmd);
}

int csv_img_push (char *filename, uint8_t *pRawData, uint32_t length, 
	uint32_t width, uint32_t height, uint8_t pos, uint8_t action, uint8_t lastpic)
{
	if ((NULL == pRawData)||(NULL == filename)) {
		return -1;
	}

	struct csv_img_t *pIMG = &gCSV->img;
	struct device_cfg_t *pDevC = &gCSV->cfg.devicecfg;

	struct img_list_t *cur = NULL;
	struct img_package_t *pIPK = NULL;

	cur = (struct img_list_t *)malloc(sizeof(struct img_list_t));
	if (cur == NULL) {
		log_err("ERROR : malloc img_list_t");
		return -1;
	}
	memset(cur, 0, sizeof(struct img_list_t));
	pIPK = &cur->ipk;

	strncpy(pIPK->filename, filename, MAX_LEN_FILE_NAME);
	pIPK->width = width;
	pIPK->height = height;
	pIPK->length = length;
	pIPK->position = pos;
	pIPK->action = action;
	pIPK->lastPic = lastpic;

	if (CAM_LEFT == pos) {
		if (pDevC->flip_left) {
			pIPK->flip = 1;
		}
	} else {
		if (pDevC->flip_right) {
			pIPK->flip = 1;
		}
	}

	if (length > 0) {
		pIPK->payload = (uint8_t *)malloc(length);

		if (NULL == pIPK->payload) {
			log_err("ERROR : malloc payload.");
			return -1;
		}

		memcpy(pIPK->payload, pRawData, length);
	}

	pthread_mutex_lock(&pIMG->mutex_img);
	list_add_tail(&cur->list, &pIMG->head_img.list);
	pthread_mutex_unlock(&pIMG->mutex_img);

	return 0;
}


static void *csv_img_loop (void *data)
{
	if (NULL == data) {
		return NULL;
	}

	struct csv_img_t *pIMG = (struct csv_img_t *)data;
	struct device_cfg_t *pDevC = &gCSV->cfg.devicecfg;
	struct calib_conf_t *pCALIB = &gCSV->cfg.calibcfg;
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;

	int ret = 0;

	struct list_head *pos = NULL, *n = NULL;
	struct img_list_t *task = NULL;
	struct timeval now;
	struct timespec timeo;
	struct img_package_t *pIPK = NULL;

	while (1) {
		list_for_each_safe(pos, n, &pIMG->head_img.list) {
			task = list_entry(pos, struct img_list_t, list);
			if (task == NULL) {
				break;
			}

			pIPK = &task->ipk;

			switch (pDevC->SaveImageFormat) {
			case SUFFIX_PNG:
				gray_raw2png(pIPK->payload, pIPK->length, pIPK->width, pIPK->height, 8, pIPK->filename);
				break;
			case SUFFIX_BMP:
			default:
				gray_raw2bmp(pIPK->payload, pIPK->width, pIPK->height, pIPK->flip, pIPK->filename);
				break;
			}

			if (pIPK->lastPic) {
				switch (pIPK->action) {
				case ACTION_CALIBRATION:
					if (pDevC->ftpupload) {
						csv_img_sender(pCALIB->path, pCALIB->groupCalibrate);
					}

					if (++pCALIB->groupCalibrate > MAX_SAVE_IMG_GROUPS) {
						pCALIB->groupCalibrate = 1;
					}
					csv_xml_write_CalibParameters();
					break;
				case ACTION_POINTCLOUD:
					ret = csv_3d_calc();
					if (0 == ret) {
						if (pDevC->ftpupload) {
							csv_img_sender(pPC->ImageSaveRoot, pPC->groupPointCloud);
						}

						if (++pPC->groupPointCloud > MAX_SAVE_IMG_GROUPS) {
							pPC->groupPointCloud = 1;
						}
						csv_xml_write_PointCloudParameters();
					}
					break;
				}
			}

			if (NULL != pIPK->payload) {
				free(pIPK->payload);
				pIPK->payload = NULL;
			}

			list_del(&task->list);
			free(task);
			task = NULL;
		}


		gettimeofday(&now, NULL);
		timeo.tv_sec = now.tv_sec + 10;
		timeo.tv_nsec = now.tv_usec * 1000;

		ret = pthread_cond_timedwait(&pIMG->cond_img, &pIMG->mutex_img, &timeo);
		if (ret == ETIMEDOUT) {
			// use timeo as a block and than retry.
		}
	}

	log_warn("WARN : exit pthread %s.", pIMG->name_img);

	pIMG->thr_img = 0;

	pthread_exit(NULL);

	return NULL;
}

static int csv_img_thread (struct csv_img_t *pIMG)
{
	int ret = -1;

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pIMG->mutex_img, NULL) != 0) {
		log_err("ERROR : mutex %s.", pIMG->name_img);
        return -1;
    }

    if (pthread_cond_init(&pIMG->cond_img, NULL) != 0) {
		log_err("ERROR : cond %s.", pIMG->name_img);
        return -1;
    }

	ret = pthread_create(&pIMG->thr_img, &attr, csv_img_loop, (void *)pIMG);
	if (ret < 0) {
		log_err("ERROR : create pthread %s.", pIMG->name_img);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p).", pIMG->name_img, pIMG->thr_img);
	}

	//pthread_attr_destory(&attr);

	return ret;
}

static void csv_img_list_release (struct csv_img_t *pIMG)
{
	struct list_head *pos = NULL, *n = NULL;
	struct img_list_t *task = NULL;

	list_for_each_safe(pos, n, &pIMG->head_img.list) {
		task = list_entry(pos, struct img_list_t, list);
		if (task == NULL) {
			break;
		}

		if (NULL != task->ipk.payload) {
			free(task->ipk.payload);
		}

		list_del(&task->list);
		free(task);
		task = NULL;
	}
}

static int csv_img_thread_cancel (struct csv_img_t *pIMG)
{
	int ret = 0;
	void *retval = NULL;

	if (pIMG->thr_img <= 0) {
		return 0;
	}

	ret = pthread_cancel(pIMG->thr_img);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s.", pIMG->name_img);
	} else {
		log_info("OK : cancel pthread %s (%p).", pIMG->name_img, pIMG->thr_img);
	}

	ret = pthread_join(pIMG->thr_img, &retval);

	pIMG->thr_img = 0;

	return ret;
}

int csv_img_init (void)
{
	struct csv_img_t *pIMG = &gCSV->img;

	INIT_LIST_HEAD(&pIMG->head_img.list);

	pIMG->name_img = NAME_THREAD_IMG;

	csv_img_thread(pIMG);

	return 0;
}

int csv_img_deinit (void)
{
	int ret = 0;
	struct csv_img_t *pIMG = &gCSV->img;

	csv_img_list_release(pIMG);

	ret = csv_img_thread_cancel(pIMG);

	return ret;
}

#ifdef __cplusplus
}
#endif


