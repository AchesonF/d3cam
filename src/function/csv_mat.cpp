#include "CameraParams.h"
#include "CsvStereoMatch.hpp"

#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

// 换成 opencv Mat 格式

static int Convert2Mat (uint32_t nPixelType, uint32_t nWidth, uint32_t nHeight, 
	unsigned char *pData, Mat& outImgMat, bool needRGB)
{
	int ret = 0;

	if (NULL == pData) {
		log_warn("ERROR : null image.");
		return -1;
	}

	switch (nPixelType) {
	case PixelType_Gvsp_Mono8: // GX_PIXEL_FORMAT_MONO8
		outImgMat = Mat(nHeight, nWidth, CV_8UC1, pData);
		break;
	case PixelType_Gvsp_RGB8_Packed:	// GX_PIXEL_FORMAT_RGB8
		if (needRGB) {		// rgb 格式
			outImgMat = Mat(nHeight, nWidth, CV_8UC3, pData);
		} else {			// Mono 格式，RGB 转 MONO
			Mat rgbMat = Mat(nHeight, nWidth, CV_8UC3, pData);
			cvtColor(rgbMat, outImgMat, COLOR_RGB2GRAY);
		}
		break;
	default:
		log_warn("ERROR : not support PixelType[%08X].", nPixelType);
		ret = -1;
		break;
	}

	if (NULL == outImgMat.data){
		log_warn("ERROR : null image out.");

		ret = -1;
	}

	return ret;
}


#if defined(USE_HK_CAMS)

/* 左右灰度图 */
int hk_msg_cameras_grab_gray (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int len_msg = 0;
	Mat left, right;
    int leftsize = 0, rightsize = 0;
    char str_err[128] = {0};
    int len_err = 0;
    struct csv_mvs_t *pMVS = &gCSV->mvs;
	MV_FRAME_OUT_INFO_EX *pimgInfoL = NULL, *pimgInfoR = NULL;

	csv_dlp_just_write(DLP_CMD_BRIGHT);

	ret = csv_mvs_cams_grab_both(pMVS);
	if (ret == 0) {
		pimgInfoL = &pMVS->Cam[CAM_LEFT].imgInfo;
		pimgInfoR = &pMVS->Cam[CAM_RIGHT].imgInfo;
		ret = Convert2Mat(pimgInfoL->enPixelType, pimgInfoL->nWidth, pimgInfoL->nHeight, pMVS->Cam[CAM_LEFT].imgData, left, false);
		ret |= Convert2Mat(pimgInfoR->enPixelType, pimgInfoR->nWidth, pimgInfoR->nHeight, pMVS->Cam[CAM_LEFT].imgData, right, false);

		if (ret == 0) {
			if (left.channels() > 1){
				cvtColor(left, left, COLOR_RGB2GRAY);
			}

			if (right.channels() > 1){
				cvtColor(right, right, COLOR_RGB2GRAY);
			}

			leftsize = left.cols * left.rows * left.channels();
			rightsize = right.cols * right.rows * right.channels();
			len_msg = sizeof(struct img_hdr_t)*2 + leftsize + rightsize;

			pACK->len_send = sizeof(struct msg_head_t) + len_msg + 4; // add 4 for tool bug
			pACK->buf_send = (uint8_t *)malloc(pACK->len_send + 1);
			if (NULL == pACK->buf_send) {
				log_err("ERROR : malloc send.");
				return -1;
			}

			memset(pACK->buf_send, 0, pACK->len_send+1);

			unsigned char *pS = pACK->buf_send;
			struct msg_head_t *pHDR = (struct msg_head_t *)pS;
			pHDR->cmdtype = pMP->hdr.cmdtype;
			pHDR->length = len_msg + sizeof(pHDR->result);
			pHDR->result = 0;
			pS += sizeof(struct msg_head_t);

			struct img_hdr_t *pIMGHdr = (struct img_hdr_t *)pS;
			pIMGHdr->type = CAM_LEFT;
			pIMGHdr->cols = left.cols;
			pIMGHdr->rows = left.rows;
			pIMGHdr->channel = left.channels();
			pS += sizeof(struct img_hdr_t);

			pIMGHdr = (struct img_hdr_t *)pS;
			pIMGHdr->type = CAM_RIGHT;
			pIMGHdr->cols = right.cols;
			pIMGHdr->rows = right.rows;
			pIMGHdr->channel = right.channels();
			pS += sizeof(struct img_hdr_t);

			memcpy(pS, left.data, leftsize);
			pS += leftsize;

			memcpy(pS, right.data, rightsize);

			return csv_msg_send(pACK);
		}
	} else if (-2 == ret) {
		len_err = snprintf(str_err, 128, "Cams busy now.");
	} else {
		len_err = snprintf(str_err, 128, "Cams grab failed.");
	}

	csv_msg_ack_package(pMP, pACK, str_err, len_err, -1);

	return csv_msg_send(pACK);
}

/* RGB 原图 */
int hk_msg_cameras_grab_rgb (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int len_msg = 0;
	Mat left, right;
    int leftsize = 0, rightsize = 0;
    char str_err[128] = {0};
    int len_err = 0;
    struct csv_mvs_t *pMVS = &gCSV->mvs;
	MV_FRAME_OUT_INFO_EX *pimgInfoL = NULL, *pimgInfoR = NULL;
	csv_dlp_just_write(DLP_CMD_BRIGHT);

	ret = csv_mvs_cams_grab_both(pMVS);
	if (ret == 0) {
		pimgInfoL = &pMVS->Cam[CAM_LEFT].imgInfo;
		pimgInfoR = &pMVS->Cam[CAM_RIGHT].imgInfo;
		ret = Convert2Mat(pimgInfoL->enPixelType, pimgInfoL->nWidth, pimgInfoL->nHeight, pMVS->Cam[CAM_LEFT].imgData, left, true);
		ret |= Convert2Mat(pimgInfoR->enPixelType, pimgInfoR->nWidth, pimgInfoR->nHeight, pMVS->Cam[CAM_LEFT].imgData, right, true);

		if (ret == 0) {
			leftsize = left.cols * left.rows * left.channels();
			rightsize = right.cols * right.rows * right.channels();

			switch (pMP->hdr.cmdtype) {
			case CAMERA_GET_GRAB_RGB:
				len_msg = sizeof(struct img_hdr_t)*2 + leftsize + rightsize;
				break;
			case CAMERA_GET_GRAB_RGB_LEFT:
				len_msg = sizeof(struct img_hdr_t) + leftsize;
				break;
			case CAMERA_GET_GRAB_RGB_RIGHT:
				len_msg = sizeof(struct img_hdr_t) + rightsize;
				break;
			default:
				// log_warn("ERROR : unknown cmdtype");
				return -1;
			}

			pACK->len_send = sizeof(struct msg_head_t) + len_msg + 4; // add 4 for tool bug
			pACK->buf_send = (uint8_t *)malloc(pACK->len_send + 1);
			if (NULL == pACK->buf_send) {
				log_err("ERROR : malloc send.");
				return -1;
			}

			memset(pACK->buf_send, 0, pACK->len_send+1);

			unsigned char *pS = pACK->buf_send;
			struct msg_head_t *pHDR = (struct msg_head_t *)pS;
			pHDR->cmdtype = pMP->hdr.cmdtype;
			pHDR->length = len_msg + sizeof(pHDR->result);
			pHDR->result = 0;
			pS += sizeof(struct msg_head_t);

			struct img_hdr_t *pIMGHdr = (struct img_hdr_t *)pS;
			switch (pMP->hdr.cmdtype) {
			case CAMERA_GET_GRAB_RGB:
				pIMGHdr->type = CAM_LEFT;
				pIMGHdr->cols = left.cols;
				pIMGHdr->rows = left.rows;
				pIMGHdr->channel = left.channels();
				pS += sizeof(struct img_hdr_t);

				pIMGHdr = (struct img_hdr_t *)pS;
				pIMGHdr->type = CAM_RIGHT;
				pIMGHdr->cols = right.cols;
				pIMGHdr->rows = right.rows;
				pIMGHdr->channel = right.channels();
				pS += sizeof(struct img_hdr_t);

				memcpy(pS, left.data, leftsize);
				pS += leftsize;

				memcpy(pS, right.data, rightsize);
				break;

			case CAMERA_GET_GRAB_RGB_LEFT:
				pIMGHdr->type = CAM_LEFT;
				pIMGHdr->cols = left.cols;
				pIMGHdr->rows = left.rows;
				pIMGHdr->channel = left.channels();
				pS += sizeof(struct img_hdr_t);

				memcpy(pS, left.data, leftsize);
				break;

			case CAMERA_GET_GRAB_RGB_RIGHT:
				pIMGHdr = (struct img_hdr_t *)pS;
				pIMGHdr->type = CAM_RIGHT;
				pIMGHdr->cols = right.cols;
				pIMGHdr->rows = right.rows;
				pIMGHdr->channel = right.channels();
				pS += sizeof(struct img_hdr_t);

				memcpy(pS, left.data, leftsize);
				break;
			default:
				return -1;
			}

			return csv_msg_send(pACK);
		}
	} else if (-2 == ret) {
		len_err = snprintf(str_err, 128, "Cams busy now.");
	} else {
		len_err = snprintf(str_err, 128, "Cams grab failed.");
	}

	csv_msg_ack_package(pMP, pACK, str_err, len_err, -1);

	return csv_msg_send(pACK);
}

#elif defined(USE_GX_CAMS)

int gx_msg_cameras_grab_gray (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int len_msg = 0;
	Mat left, right;
    int leftsize = 0, rightsize = 0;
    char str_err[128] = {0};
    int len_err = 0;
    struct csv_gx_t *pGX = &gCSV->gx;
	PGX_FRAME_BUFFER  pFrameL = NULL, pFrameR = NULL;

	csv_dlp_just_write(DLP_CMD_BRIGHT);

	ret = csv_gx_cams_grab_both(pGX);
	if (ret == 0) {
		pFrameL = pGX->Cam[CAM_LEFT].pFrameBuffer;
		pFrameR = pGX->Cam[CAM_RIGHT].pFrameBuffer;

		ret = Convert2Mat(pFrameL->nPixelFormat, pFrameL->nWidth, pFrameL->nHeight, pGX->Cam[CAM_LEFT].pMonoImageBuf, left, false);
		ret |= Convert2Mat(pFrameR->nPixelFormat, pFrameR->nWidth, pFrameR->nHeight, pGX->Cam[CAM_LEFT].pMonoImageBuf, right, false);

		if (ret == 0) {
			if (left.channels() > 1){
				cvtColor(left, left, COLOR_RGB2GRAY);
			}

			if (right.channels() > 1){
				cvtColor(right, right, COLOR_RGB2GRAY);
			}

			leftsize = left.cols * left.rows * left.channels();
			rightsize = right.cols * right.rows * right.channels();
			len_msg = sizeof(struct img_hdr_t)*2 + leftsize + rightsize;

			pACK->len_send = sizeof(struct msg_head_t) + len_msg + 4; // add 4 for tool bug
			pACK->buf_send = (uint8_t *)malloc(pACK->len_send + 1);
			if (NULL == pACK->buf_send) {
				log_err("ERROR : malloc send.");
				return -1;
			}

			memset(pACK->buf_send, 0, pACK->len_send+1);

			unsigned char *pS = pACK->buf_send;
			struct msg_head_t *pHDR = (struct msg_head_t *)pS;
			pHDR->cmdtype = pMP->hdr.cmdtype;
			pHDR->length = len_msg + sizeof(pHDR->result);
			pHDR->result = 0;
			pS += sizeof(struct msg_head_t);

			struct img_hdr_t *pIMGHdr = (struct img_hdr_t *)pS;
			pIMGHdr->type = CAM_LEFT;
			pIMGHdr->cols = left.cols;
			pIMGHdr->rows = left.rows;
			pIMGHdr->channel = left.channels();
			pS += sizeof(struct img_hdr_t);

			pIMGHdr = (struct img_hdr_t *)pS;
			pIMGHdr->type = CAM_RIGHT;
			pIMGHdr->cols = right.cols;
			pIMGHdr->rows = right.rows;
			pIMGHdr->channel = right.channels();
			pS += sizeof(struct img_hdr_t);

			memcpy(pS, left.data, leftsize);
			pS += leftsize;

			memcpy(pS, right.data, rightsize);

			return csv_msg_send(pACK);
		}
	} else if (-2 == ret) {
		len_err = snprintf(str_err, 128, "Cams busy now.");
	} else {
		len_err = snprintf(str_err, 128, "Cams grab failed.");
	}

	csv_msg_ack_package(pMP, pACK, str_err, len_err, -1);

	return csv_msg_send(pACK);
}

/* RGB 原图 */
int gx_msg_cameras_grab_rgb (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int len_msg = 0;
	Mat left, right;
    int leftsize = 0, rightsize = 0;
    char str_err[128] = {0};
    int len_err = 0;
    struct csv_gx_t *pGX = &gCSV->gx;
    PGX_FRAME_BUFFER pFrameL = NULL, pFrameR = NULL;

	csv_dlp_just_write(DLP_CMD_BRIGHT);

	ret = csv_gx_cams_grab_both(pGX);
	if (ret == 0) {
		pFrameL = pGX->Cam[CAM_LEFT].pFrameBuffer;
		pFrameR = pGX->Cam[CAM_RIGHT].pFrameBuffer;

		ret = Convert2Mat(pFrameL->nPixelFormat, pFrameL->nWidth, pFrameL->nHeight, pGX->Cam[CAM_LEFT].pMonoImageBuf, left, true);
		ret |= Convert2Mat(pFrameR->nPixelFormat, pFrameR->nWidth, pFrameR->nHeight, pGX->Cam[CAM_LEFT].pMonoImageBuf, right, true);

		if (ret == 0) {
			leftsize = left.cols * left.rows * left.channels();
			rightsize = right.cols * right.rows * right.channels();

			switch (pMP->hdr.cmdtype) {
			case CAMERA_GET_GRAB_RGB:
				len_msg = sizeof(struct img_hdr_t)*2 + leftsize + rightsize;
				break;
			case CAMERA_GET_GRAB_RGB_LEFT:
				len_msg = sizeof(struct img_hdr_t) + leftsize;
				break;
			case CAMERA_GET_GRAB_RGB_RIGHT:
				len_msg = sizeof(struct img_hdr_t) + rightsize;
				break;
			default:
				// log_warn("ERROR : unknown cmdtype");
				return -1;
			}

			pACK->len_send = sizeof(struct msg_head_t) + len_msg + 4; // add 4 for tool bug
			pACK->buf_send = (uint8_t *)malloc(pACK->len_send + 1);
			if (NULL == pACK->buf_send) {
				log_err("ERROR : malloc send.");
				return -1;
			}

			memset(pACK->buf_send, 0, pACK->len_send+1);

			unsigned char *pS = pACK->buf_send;
			struct msg_head_t *pHDR = (struct msg_head_t *)pS;
			pHDR->cmdtype = pMP->hdr.cmdtype;
			pHDR->length = len_msg + sizeof(pHDR->result);
			pHDR->result = 0;
			pS += sizeof(struct msg_head_t);

			struct img_hdr_t *pIMGHdr = (struct img_hdr_t *)pS;
			switch (pMP->hdr.cmdtype) {
			case CAMERA_GET_GRAB_RGB:
				pIMGHdr->type = CAM_LEFT;
				pIMGHdr->cols = left.cols;
				pIMGHdr->rows = left.rows;
				pIMGHdr->channel = left.channels();
				pS += sizeof(struct img_hdr_t);

				pIMGHdr = (struct img_hdr_t *)pS;
				pIMGHdr->type = CAM_RIGHT;
				pIMGHdr->cols = right.cols;
				pIMGHdr->rows = right.rows;
				pIMGHdr->channel = right.channels();
				pS += sizeof(struct img_hdr_t);

				memcpy(pS, left.data, leftsize);
				pS += leftsize;

				memcpy(pS, right.data, rightsize);
				break;

			case CAMERA_GET_GRAB_RGB_LEFT:
				pIMGHdr->type = CAM_LEFT;
				pIMGHdr->cols = left.cols;
				pIMGHdr->rows = left.rows;
				pIMGHdr->channel = left.channels();
				pS += sizeof(struct img_hdr_t);

				memcpy(pS, left.data, leftsize);
				break;

			case CAMERA_GET_GRAB_RGB_RIGHT:
				pIMGHdr = (struct img_hdr_t *)pS;
				pIMGHdr->type = CAM_RIGHT;
				pIMGHdr->cols = right.cols;
				pIMGHdr->rows = right.rows;
				pIMGHdr->channel = right.channels();
				pS += sizeof(struct img_hdr_t);

				memcpy(pS, left.data, leftsize);
				break;
			default:
				return -1;
			}

			return csv_msg_send(pACK);
		}
	} else if (-2 == ret) {
		len_err = snprintf(str_err, 128, "Cams busy now.");
	} else {
		len_err = snprintf(str_err, 128, "Cams grab failed.");
	}

	csv_msg_ack_package(pMP, pACK, str_err, len_err, -1);

	return csv_msg_send(pACK);
}


int gx_msg_cameras_grab_img_depth (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1, ret_timeo = 0;
	struct csv_gx_t *pGX = &gCSV->gx;
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;
	int len_msg = 0;
    char str_err[128] = {0};
    int len_err = 0;
    Mat depthImg;
    int depthMatsize = 0;

	ret = csv_gx_cams_highspeed(pGX);

	struct timeval now;
	struct timespec timeo;
	gettimeofday(&now, NULL);
	timeo.tv_sec = now.tv_sec + 5;
	timeo.tv_nsec = now.tv_usec * 1000;

	ret = pthread_cond_timedwait(&pGX->cond_wait_depth, &pGX->mutex_wait_depth, &timeo);
	if (ret == ETIMEDOUT) {
		ret_timeo = -1;
	}

	if (ret_timeo < 0) {
		len_err = snprintf(str_err, 128, "Cams busy now.");
	} else {
		depthImg = imread(pPC->outDepthImage);
		depthMatsize = depthImg.cols * depthImg.rows *2;
		len_msg = sizeof(struct img_hdr_t) + depthMatsize;

		pACK->len_send = sizeof(struct msg_head_t) + len_msg + 4; // add 4 for tool bug
		pACK->buf_send = (uint8_t *)malloc(pACK->len_send + 1);
		if (NULL == pACK->buf_send) {
			log_err("ERROR : malloc send.");
			csv_msg_ack_package(pMP, pACK, NULL, 0, -1);
			return -1;
		}

		memset(pACK->buf_send, 0, pACK->len_send+1);

		unsigned char *pS = pACK->buf_send;
		struct msg_head_t *pHDR = (struct msg_head_t *)pS;
		pHDR->cmdtype = pMP->hdr.cmdtype;
		pHDR->length = len_msg + sizeof(pHDR->result);
		pHDR->result = 0;
		pS += sizeof(struct msg_head_t);

		struct img_hdr_t *pIMGHdr = (struct img_hdr_t *)pS;
		pIMGHdr->type = 3;
		pIMGHdr->cols = depthImg.cols;
		pIMGHdr->rows = depthImg.rows;
		pIMGHdr->channel = 2;	// 深度图数据是1通道CV_16S占两个字节
		pS += sizeof(struct img_hdr_t);

		memcpy(pS, depthImg.data, depthMatsize);

		return csv_msg_send(pACK);
	}

	csv_msg_ack_package(pMP, pACK, str_err, len_err, -1);

	return csv_msg_send(pACK);
}

#endif

/* 填充随机数据 */
int msg_cameras_grab_urandom (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = 0;
	int len_msg = 0;
	Mat left, right;
    int leftsize = 0, rightsize = 0;	// 左右图像的大小
	int len_data = 1240*1616;

	uint8_t *pData = (uint8_t *)malloc(len_data);
	if (NULL == pData) {
		log_err("ERROR : malloc data.");
		return -1;
	}

	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) {
		log_err("ERROR : open urandom.");
		return -1;
	}

	ret = read(fd, pData, len_data);
	if (ret < 0) {
		log_err("ERROR : read.");
		free(pData);
		return -1;
	}
	close(fd);

	ret = Convert2Mat(PixelType_Gvsp_Mono8, 1240, 1616, pData, left, false);
	ret |= Convert2Mat(PixelType_Gvsp_Mono8, 1240, 1616, pData, right, false);

	leftsize = left.cols * left.rows * left.channels();
	rightsize = right.cols * right.rows * right.channels();
	len_msg = sizeof(struct img_hdr_t)*2 + leftsize + rightsize;

	pACK->len_send = sizeof(struct msg_head_t) + len_msg + 4; // add 4 for tool bug
	pACK->buf_send = (uint8_t *)malloc(pACK->len_send + 1);
	if (NULL == pACK->buf_send) {
		log_err("ERROR : malloc send.");
		return -1;
	}

	memset(pACK->buf_send, 0, pACK->len_send+1);

	unsigned char *pS = pACK->buf_send;
	struct msg_head_t *pHDR = (struct msg_head_t *)pS;
	pHDR->cmdtype = pMP->hdr.cmdtype;
	pHDR->length = len_msg + sizeof(pHDR->result);
	pHDR->result = 0;
	pS += sizeof(struct msg_head_t);

	struct img_hdr_t *pIMGHdr = (struct img_hdr_t *)pS;
	pIMGHdr->type = CAM_LEFT;
	pIMGHdr->cols = left.cols;
	pIMGHdr->rows = left.rows;
	pIMGHdr->channel = left.channels();
	pS += sizeof(struct img_hdr_t);

	pIMGHdr = (struct img_hdr_t *)pS;
	pIMGHdr->type = CAM_RIGHT;
	pIMGHdr->cols = right.cols;
	pIMGHdr->rows = right.rows;
	pIMGHdr->channel = right.channels();
	pS += sizeof(struct img_hdr_t);
	memcpy(pS, left.data, leftsize);

	pS += leftsize;
	memcpy(pS, right.data, rightsize);

	if (NULL != pData) {
		free(pData);
	}

	return csv_msg_send(pACK);
}


#ifdef __cplusplus
}
#endif

