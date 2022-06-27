#include "CameraParams.h"
#include "CsvStereoMatch.hpp"

#include "list.h"
#include "csv_type.h"
#include "csv_msg.h"
#include "csv_mvs.h"
#include "csv_mat.h"

#ifdef __cplusplus
extern "C" {
#endif


// 海康图像格式转换成 opencv Mat 格式
static int Convert2Mat (MV_FRAME_OUT_INFO_EX *pstImageInfo, 
	unsigned char *pData, Mat& outImgMat, bool needRGB)
{
	switch (pstImageInfo->enPixelType) {
	case PixelType_Gvsp_Mono8:
		outImgMat = cv::Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC1, pData);
		break;
	case PixelType_Gvsp_RGB8_Packed:
		if (needRGB) {		// rgb 格式
			outImgMat = cv::Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC3, pData);
		} else {			// Mono 格式，RGB 转 MONO
			Mat rgbMat = cv::Mat(pstImageInfo->nHeight, pstImageInfo->nWidth, CV_8UC3, pData);
			cvtColor(rgbMat, outImgMat, cv::COLOR_RGB2GRAY);
		}
		break;
	default:
		//log_info("ERROR : not support PixelType[%08X]", pstImageInfo->enPixelType);
		return -1;
		break;
	}

	if ( NULL == outImgMat.data ){
		//log_info("ERROR : null image out.");

		return -1;
	}

	return 0;
}


int msg_cameras_grab_image (struct msg_package_t *pMP, struct msg_ack_t *pACK)
{
	int ret = -1;
	int len_msg = 0;
	Mat left, right;
    int leftsize = 0, rightsize = 0;	// 左右图像的大小
	struct cam_spec_t *pCAMLEFT = &Cam[CAM_LEFT], *pCAMRIGHT = &Cam[CAM_RIGHT];

//csv_dlp_write(CLOSE_LOGO_LED);
//csv_dlp_write(OPEN_SYNC);

	ret = csv_mvs_cams_grab_both();
	if (ret == 0) {
		// TODO change side
		ret = Convert2Mat(&pCAMLEFT->imageInfo, pCAMLEFT->imgData, left, true);
		ret |= Convert2Mat(&pCAMRIGHT->imageInfo, pCAMRIGHT->imgData, right, true);

		if (ret == 0) {
			leftsize = left.cols*left.rows*left.channels();
			rightsize = right.cols*right.rows*right.channels();
			len_msg = sizeof(struct img_hdr_t)*2 + leftsize + rightsize;

			pACK->len_send = sizeof(struct msg_head_t)+len_msg;
			pACK->buf_send = (uint8_t *)malloc(pACK->len_send + 1);
			if (NULL == pACK->buf_send) {
				//log_err("ERROR : malloc send");
				return -1;
			}

			memset(pACK->buf_send, 0, pACK->len_send+1);

			unsigned char *pS = pACK->buf_send;
			struct msg_head_t *pHDR = (struct msg_head_t *)pS;
			pHDR->cmdtype = pMP->hdr.cmdtype;
			pHDR->length = len_msg+sizeof(pHDR->result);
			pHDR->result = 0;
			pS += sizeof(struct msg_head_t);

			struct img_hdr_t *pIMGHdr = (struct img_hdr_t *)pS;
			pIMGHdr->type = 0; // left
			pIMGHdr->cols = left.cols;
			pIMGHdr->rows = left.rows;
			pIMGHdr->channel = left.channels();
			pS += sizeof(struct img_hdr_t);

			pIMGHdr = (struct img_hdr_t *)pS;
			pIMGHdr->type = 1; // right
			pIMGHdr->cols = right.cols;
			pIMGHdr->rows = right.rows;
			pIMGHdr->channel = right.channels();
			pS += sizeof(struct img_hdr_t);

			memcpy(pS, left.data, leftsize);
			pS += leftsize;
			memcpy(pS, right.data, rightsize);

			return csv_msg_send(pACK);
		}
	}

	csv_msg_ack_package(pMP, pACK, NULL, 0, -1);

	return csv_msg_send(pACK);
}


#ifdef __cplusplus
}
#endif

