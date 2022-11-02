#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#if 1
	snprintf(str_cmd, 256, "ftp-upload -h %s %s/CSV_%03d*", 
		inet_ntoa(gCSV->tcpl.peer.sin_addr), path, group);
#else
	snprintf(str_cmd, 256, "ftp-upload -v -h %s %s/CSV_%03d*", 
		inet_ntoa(gCSV->tcpl.peer.sin_addr), path, group);
#endif

	return system(str_cmd);
}

int csv_img_clear (char *path)
{
	char str_cmd[256] = {0};

	if ((NULL == path)||(strcmp(path, "/") == 0)) {
		return -1;
	}

	memset(str_cmd, 0, 256);

	snprintf(str_cmd, 256, "rm -rf %s/*", path); // be careful for root directory "/"

	return system(str_cmd);
}

int csv_img_push (char *filename, uint8_t *pRawData, uint32_t length, 
	uint32_t width, uint32_t height, uint8_t pos, uint8_t action, uint8_t lastpic)
{
	if ((NULL == pRawData)||(NULL == filename)) {
		return -1;
	}

	struct csv_img_t *pIMG = &gCSV->img;
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
	pIPK->action = action;
	pIPK->position = pos;
	pIPK->lastPic = lastpic;

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

void csv_img_list_release (struct csv_img_t *pIMG)
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

	while (gCSV->running) {
		list_for_each_safe(pos, n, &pIMG->head_img.list) {
			task = list_entry(pos, struct img_list_t, list);
			if (task == NULL) {
				break;
			}

			pIPK = &task->ipk;

			csv_mat_img_save(pIPK->height, pIPK->width, pIPK->payload, pIPK->filename);

			if (pIPK->lastPic) {
				switch (pIPK->action) {
				case ACTION_CALIBRATION:
					if (pDevC->ftpupload) {
						csv_img_sender(pCALIB->CalibImageRoot, pCALIB->groupCalibrate);
					}

					if (++pCALIB->groupCalibrate == 0) {
						pCALIB->groupCalibrate = 1;
					}
					csv_xml_write_CalibParameters();
					break;
				case ACTION_POINTCLOUD:
					if (pDevC->ftpupload) {
						usleep(500000); //wait for 3d calc
						csv_img_sender(pPC->PCImageRoot, pPC->groupPointCloud);
					}

					if (++pPC->groupPointCloud == 0) {
						pPC->groupPointCloud = 1;
					}
					csv_xml_write_PointCloudParameters();
					break;
				}

				gCSV->gx.busying = false;
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

	csv_img_list_release(pIMG);

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


