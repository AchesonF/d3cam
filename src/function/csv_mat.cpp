#include "CameraParams.h"
#include "CsvStereoMatch.hpp"

#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


// 海康图像格式转换成 opencv Mat 格式
static int Convert2Mat (MV_FRAME_OUT_INFO_EX *pstImageInfo, 
	unsigned char *pData, Mat& outImgMat, bool needRGB)
{
	int ret = 0;

	if ((NULL == pstImageInfo)||(NULL == pData)) {
		log_info("ERROR : null point.");
		return -1;
	}

	switch (pstImageInfo->enPixelType) {
	case PixelType_Gvsp_Mono8:
		outImgMat = Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC1, pData);
		break;
	case PixelType_Gvsp_RGB8_Packed:
		if (needRGB) {		// rgb 格式
			outImgMat = Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC3, pData);
		} else {			// Mono 格式，RGB 转 MONO
			Mat rgbMat = Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC3, pData);
			cvtColor(rgbMat, outImgMat, COLOR_RGB2GRAY);
		}
		break;
	default:
		log_info("ERROR : not support PixelType[%08X]", pstImageInfo->enPixelType);
		ret = -1;
		break;
	}

	if (NULL == outImgMat.data){
		log_info("ERROR : null image out.");

		ret = -1;
	}

	return ret;
}

/* 左右灰度图 */
int msg_cameras_grab_gray (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int len_msg = 0;
	Mat left, right;
    int leftsize = 0, rightsize = 0;
    char str_err[128] = {0};
    int len_err = 0;
    struct csv_mvs_t *pMVS = &gCSV->mvs;
	
	csv_dlp_just_write(DLP_CMD_BRIGHT);

	ret = csv_mvs_cams_grab_both(pMVS);
	if (ret == 0) {
		// TODO change side
		ret = Convert2Mat(&pMVS->Cam[CAM_LEFT].imgInfo, pMVS->Cam[CAM_LEFT].imgData, left, false);
		ret |= Convert2Mat(&pMVS->Cam[CAM_LEFT].imgInfo, pMVS->Cam[CAM_LEFT].imgData, right, false);

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
				log_err("ERROR : malloc send");
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
int msg_cameras_grab_rgb (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int len_msg = 0;
	Mat left, right;
    int leftsize = 0, rightsize = 0;
    char str_err[128] = {0};
    int len_err = 0;
    struct csv_mvs_t *pMVS = &gCSV->mvs;

	csv_dlp_just_write(DLP_CMD_BRIGHT);

	ret = csv_mvs_cams_grab_both(pMVS);
	if (ret == 0) {
		// TODO change side
		ret = Convert2Mat(&pMVS->Cam[CAM_LEFT].imgInfo, pMVS->Cam[CAM_LEFT].imgData, left, true);
		ret |= Convert2Mat(&pMVS->Cam[CAM_LEFT].imgInfo, pMVS->Cam[CAM_LEFT].imgData, right, true);

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
				// log_info("ERROR : unknown cmdtype");
				return -1;
			}

			pACK->len_send = sizeof(struct msg_head_t) + len_msg + 4; // add 4 for tool bug
			pACK->buf_send = (uint8_t *)malloc(pACK->len_send + 1);
			if (NULL == pACK->buf_send) {
				log_err("ERROR : malloc send");
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

int msg_cameras_grab_img_depth (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = 0, i = 0, errNum = 0;
	int nRet = MV_OK;
	int nFrames = 13;
	int idx = 1;
	uint8_t end_pc = 0;
	char img_name[256] = {0};
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM = NULL;
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;
	struct device_cfg_t *pDevC = &gCSV->cfg.devicecfg;

	// 13 高速光
	ret = csv_dlp_just_write(DLP_CMD_HIGHSPEED);

	pMVS->firstTimestamp = utility_get_microsecond();

	while (idx <= nFrames) {
		for (i = 0; i < pMVS->cnt_mvs; i++) {
			pCAM = &pMVS->Cam[i];

			if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
				errNum++;
				continue;
			}

			memset(&pCAM->imgInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
			nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
				pCAM->sizePayload.nCurValue, &pCAM->imgInfo, 3000);
			if (nRet == MV_OK) {
				log_info("OK : CAM '%s' [%d_%02d]: GetOneFrame[%d] %d x %d", pCAM->sn, idx, i, 
					pCAM->imgInfo.nFrameNum, pCAM->imgInfo.nWidth, pCAM->imgInfo.nHeight);

				if ((idx == nFrames)&&(i == 1)) {
					end_pc = 1;
				}
				memset(img_name, 0, 256);
				generate_image_filename(pPC->ImageSaveRoot, pPC->groupPointCloud, idx, i, 
					pDevC->SaveImageFormat, img_name);
				ret = save_image_to_file(&pCAM->imgInfo, pCAM->pHandle, pCAM->imgData, 
					pCAM->sizePayload.nCurValue, pDevC->SaveImageFormat, img_name, end_pc);
			} else {
				log_info("ERROR : CAM '%s' [%d_%02d]: GetOneFrameTimeout, [0x%08X]", 
					pCAM->sn, idx, i, nRet);
				errNum++;
				break;
			}
		}

		if (errNum > 0) {
			break;
		}

		idx++;
	}

	pMVS->lastTimestamp = utility_get_microsecond();
	log_debug("highspeed 13 take %ld us.", pMVS->lastTimestamp - pMVS->firstTimestamp);

	if (errNum > 0) {
		return -1;
	}

	ret = point_cloud_calc();

	return ret;
}

/* 填充随机数据 */
int msg_cameras_grab_urandom (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = 0;
	int len_msg = 0;
	Mat left, right;
    int leftsize = 0, rightsize = 0;	// 左右图像的大小
	int len_data = 1240*1616;
	MV_FRAME_OUT_INFO_EX pstImageInfo;
	pstImageInfo.enPixelType = PixelType_Gvsp_Mono8;
	pstImageInfo.nHeight = 1240;
	pstImageInfo.nWidth = 1616;

	uint8_t *pData = (uint8_t *)malloc(len_data);
	if (NULL == pData) {
		log_err("ERROR : malloc");
		return -1;
	}

	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) {
		log_err("ERROR : open urandom");
		return -1;
	}

	ret = read(fd, pData, len_data);
	if (ret < 0) {
		log_err("ERROR : read");
		free(pData);
		return -1;
	}
	close(fd);

	ret = Convert2Mat(&pstImageInfo, pData, left, false);
	ret |= Convert2Mat(&pstImageInfo, pData, right, false);

	leftsize = left.cols * left.rows * left.channels();
	rightsize = right.cols * right.rows * right.channels();
	len_msg = sizeof(struct img_hdr_t)*2 + leftsize + rightsize;

	pACK->len_send = sizeof(struct msg_head_t) + len_msg + 4; // add 4 for tool bug
	pACK->buf_send = (uint8_t *)malloc(pACK->len_send + 1);
	if (NULL == pACK->buf_send) {
		log_err("ERROR : malloc send");
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

