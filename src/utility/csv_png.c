#include "inc_files.h"
#include "spng.h"
#include "csv_pointcloud.hpp"

#ifdef __cplusplus
extern "C" {
#endif

static int encode_image(void *image, size_t length, uint32_t width, 
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

int csv_png_push (char *filename, uint8_t *buf, 
	uint32_t length, uint32_t width, uint32_t height, uint8_t end_pc)
{
	if ((NULL == buf)||(NULL == filename)) {
		return -1;
	}

	struct csv_png_t *pPNG = &gCSV->png;

	struct pnglist_t *cur = NULL;
	struct png_package_t *pPP = NULL;

	cur = (struct pnglist_t *)malloc(sizeof(struct pnglist_t));
	if (cur == NULL) {
		log_err("ERROR : malloc msglist_t");
		return -1;
	}
	memset(cur, 0, sizeof(struct pnglist_t));
	pPP = &cur->pp;

	strncpy(pPP->filename, filename, MAX_LEN_FILE_NAME);
	pPP->width = width;
	pPP->height = height;
	pPP->length = length;
	pPP->end_pc = end_pc;

	if (length > 0) {
		pPP->payload = (uint8_t *)malloc(length);

		if (NULL == pPP->payload) {
			log_err("ERROR : malloc payload");
			return -1;
		}

		memcpy(pPP->payload, buf, length);
	}

	pthread_mutex_lock(&pPNG->mutex_png);
	list_add_tail(&cur->list, &pPNG->head_png.list);
	pthread_mutex_unlock(&pPNG->mutex_png);

	return 0;
}


static void *csv_png_loop (void *data)
{
	if (NULL == data) {
		return NULL;
	}

	struct csv_png_t *pPNG = (struct csv_png_t *)data;

	int ret = 0;

	struct list_head *pos = NULL, *n = NULL;
	struct pnglist_t *task = NULL;
	struct timeval now;
	struct timespec timeo;
	struct png_package_t *pPP = NULL;

	while (1) {
		list_for_each_safe(pos, n, &pPNG->head_png.list) {
			task = list_entry(pos, struct pnglist_t, list);
			if (task == NULL) {
				break;
			}

			pPP = &task->pp;

			encode_image(pPP->payload, pPP->length, pPP->width, pPP->height, 8, pPP->filename);

			if (pPP->end_pc) {
				sleep(1);
				point_cloud_calc();
			}

			if (NULL != pPP->payload) {
				free(pPP->payload);
				pPP->payload = NULL;
			}

			list_del(&task->list);
			free(task);
			task = NULL;
		}


		gettimeofday(&now, NULL);
		timeo.tv_sec = now.tv_sec + 10;
		timeo.tv_nsec = now.tv_usec * 1000;

		ret = pthread_cond_timedwait(&pPNG->cond_png, &pPNG->mutex_png, &timeo);
		if (ret == ETIMEDOUT) {
			// use timeo as a block and than retry.
		}
	}

	log_info("WARN : exit pthread %s", pPNG->name_png);

	pPNG->thr_png = 0;

	pthread_exit(NULL);

	return NULL;
}

static int csv_png_thread (struct csv_png_t *pPNG)
{
	int ret = -1;

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pPNG->mutex_png, NULL) != 0) {
		log_err("ERROR : mutex %s", pPNG->name_png);
        return -1;
    }

    if (pthread_cond_init(&pPNG->cond_png, NULL) != 0) {
		log_err("ERROR : cond %s", pPNG->name_png);
        return -1;
    }

	ret = pthread_create(&pPNG->thr_png, &attr, csv_png_loop, (void *)pPNG);
	if (ret < 0) {
		log_err("ERROR : create pthread %s", pPNG->name_png);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p)", pPNG->name_png, pPNG->thr_png);
	}

	//pthread_attr_destory(&attr);

	return ret;
}

static void csv_png_list_release (struct csv_png_t *pPNG)
{
	struct list_head *pos = NULL, *n = NULL;
	struct pnglist_t *task = NULL;

	list_for_each_safe(pos, n, &pPNG->head_png.list) {
		task = list_entry(pos, struct pnglist_t, list);
		if (task == NULL) {
			break;
		}

		if (NULL != task->pp.payload) {
			free(task->pp.payload);
		}

		list_del(&task->list);
		free(task);
		task = NULL;
	}
}

static int csv_png_thread_cancel (struct csv_png_t *pPNG)
{
	int ret = 0;
	void *retval = NULL;

	if (pPNG->thr_png <= 0) {
		return 0;
	}

	ret = pthread_cancel(pPNG->thr_png);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s", pPNG->name_png);
	} else {
		log_info("OK : cancel pthread %s", pPNG->name_png);
	}

	ret = pthread_join(pPNG->thr_png, &retval);

	return ret;
}



int csv_png_init (void)
{
	struct csv_png_t *pPNG = &gCSV->png;

	INIT_LIST_HEAD(&pPNG->head_png.list);

	pPNG->name_png = NAME_THREAD_PNG;

	csv_png_thread(pPNG);

	return 0;
}

int csv_png_deinit (void)
{
	int ret = 0;
	struct csv_png_t *pPNG = &gCSV->png;

	csv_png_list_release(pPNG);

	ret = csv_png_thread_cancel(pPNG);

	return ret;
}


#ifdef __cplusplus
}
#endif
