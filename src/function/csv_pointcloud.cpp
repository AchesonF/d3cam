#include <iostream>

#include "inc_files.h"

#if (USING_POINTCLOUD3D==1)

#include "csvCreatePoint3D.hpp"

#ifdef __cplusplus
extern "C" {
#endif

struct pointcloud_head_t {
	int			idx;
	int			side;
	int			width;
	int			height;
};


int csv_point_cloud_grab (void)
{
	int ret = 0, i = 0;
	int nRet = MV_OK;
	int nFrames = 13;
	int idx = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM = NULL;
	uint8_t *pPC = NULL, *pbuf = NULL;
	struct pointcloud_head_t *phdr = NULL;

	MVCC_INTVALUE stParam;
	memset(&stParam, 0, sizeof(MVCC_INTVALUE));

	// 前提是必须保证两个相机是一样的参数
	nRet = MV_CC_GetIntValue(Cam[CAM_LEFT].pHandle, "PayloadSize", &stParam);
	if (MV_OK != nRet) {
		log_info("ERROR : CAM '%s' get PayloadSize failed. [0x%08X]", pCAM->serialNum, nRet);
		return -1;
	}

	pPC = (uint8_t *)malloc((sizeof(struct pointcloud_head_t)+stParam.nCurValue)*nFrames*pMVS->cnt_mvs);
	if (NULL == pPC) {
		log_err("ERROR : malloc pc");
		return -1;
	}

	pbuf = pPC;

	// 1 黑色
	ret = csv_dlp_just_write(0);

	for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &Cam[i];
		if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
			continue;
		}

		pCAM->imgData = (uint8_t *)malloc(stParam.nCurValue);
		if (pCAM->imgData == NULL){
			log_err("ERROR : malloc imgData");
			return -1;
		}
		memset(pCAM->imgData, 0x00, stParam.nCurValue);

		memset(&pCAM->imageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
		nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
			stParam.nCurValue, &pCAM->imageInfo, 3000);
		if (nRet == MV_OK) {
			log_info("OK : CAM '%s' [%d_%02d]: GetOneFrame[%d] %d x %d", pCAM->serialNum, idx, i, 
				pCAM->imageInfo.nFrameNum, pCAM->imageInfo.nWidth, pCAM->imageInfo.nHeight);

			phdr = (struct pointcloud_head_t *)pbuf;
			phdr->idx = 0;
			phdr->side = i;
			phdr->width = pCAM->imageInfo.nWidth;
			phdr->height = pCAM->imageInfo.nHeight;
			pbuf += sizeof(struct pointcloud_head_t);

			memcpy(pbuf, pCAM->imgData, stParam.nCurValue);
			pbuf += stParam.nCurValue;
		} else {
			log_info("ERROR : CAM '%s' [%d_%02d]: GetOneFrameTimeout, [0x%08X]", 
				pCAM->serialNum, idx, i, nRet);
		}

	}

	idx++;

	// 2-9 格雷码
	ret = csv_dlp_just_write(1);

	while (idx < 9) {
		for (i = 0; i < pMVS->cnt_mvs; i++) {
			pCAM = &Cam[i];
			if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
				continue;
			}
			//memset(pCAM->imgData, 0x00, stParam.nCurValue);
			memset(&pCAM->imageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
			nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
				stParam.nCurValue, &pCAM->imageInfo, 3000);
			if (nRet == MV_OK) {
				log_info("OK : CAM '%s' [%d_%02d]: GetOneFrame[%d] %d x %d", pCAM->serialNum, idx, i, 
					pCAM->imageInfo.nFrameNum, pCAM->imageInfo.nWidth, pCAM->imageInfo.nHeight);

				phdr = (struct pointcloud_head_t *)pbuf;
				phdr->idx = idx;
				phdr->side = i;
				phdr->width = pCAM->imageInfo.nWidth;
				phdr->height = pCAM->imageInfo.nHeight;
				pbuf += sizeof(struct pointcloud_head_t);

				memcpy(pbuf, pCAM->imgData, stParam.nCurValue);
				pbuf += stParam.nCurValue;
			} else {
				log_info("ERROR : CAM '%s' [%d_%02d]: GetOneFrameTimeout, [0x%08X]", 
					pCAM->serialNum, idx, i, nRet);
			}

		}

		idx++;
	}

	// 10-13 相移
	ret = csv_dlp_just_write(2);

	while (idx < 13) {
		for (i = 0; i < pMVS->cnt_mvs; i++) {
			pCAM = &Cam[i];
			if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
				continue;
			}
			//memset(pCAM->imgData, 0x00, stParam.nCurValue);
			memset(&pCAM->imageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
			nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
				stParam.nCurValue, &pCAM->imageInfo, 3000);
			if (nRet == MV_OK) {
				log_info("OK : CAM '%s' [%d_%02d]: GetOneFrame[%d] %d x %d", pCAM->serialNum, idx, i, 
					pCAM->imageInfo.nFrameNum, pCAM->imageInfo.nWidth, pCAM->imageInfo.nHeight);

				phdr = (struct pointcloud_head_t *)pbuf;
				phdr->idx = idx;
				phdr->side = i;
				phdr->width = pCAM->imageInfo.nWidth;
				phdr->height = pCAM->imageInfo.nHeight;
				pbuf += sizeof(struct pointcloud_head_t);

				memcpy(pbuf, pCAM->imgData, stParam.nCurValue);
				pbuf += stParam.nCurValue;
			} else {
				log_info("ERROR : CAM '%s' [%d_%02d]: GetOneFrameTimeout, [0x%08X]", 
					pCAM->serialNum, idx, i, nRet);
			}

		}

		idx++;
	}

//	CSV::CsvPoint3DCloud pointCloud;
//	bool result = CSV::csvCreatePoint3D(pPC, pointCloud);
//	log_info("point cloud  result : %d", result);

	for (i = 0; i < pMVS->cnt_mvs; i++) {
		pCAM = &Cam[i];

		if (NULL != pCAM->imgData){
			free(pCAM->imgData);
			pCAM->imgData = NULL;
		}
	}

	if (NULL != pPC) {
		free(pPC);
	}

	return ret;
}



int testPoint3DCloud (void)
{
	CSV::CsvCreatePoint3DParam param;
	param.calibXml = std::string("./CalibInv.xml");
	param.type = CSV::CSV_DataFormatType::FixPoint64bits;

	//CSV::CsvCreatePoint3DParam param0;
	csvGetCreatePoint3DParam(param);
	std::cout << param.calibXml << std::endl;
	std::cout << param.type << std::endl;

	std::vector<std::vector<CSV::CsvImageSimple>> imageGroups;
	CSV::CsvPoint3DCloud pointCloud;

	bool result = CSV::csvCreatePoint3D(imageGroups, pointCloud);
	std::cout << result << std::endl;

	return 0;
}



#ifdef __cplusplus
}
#endif

#endif