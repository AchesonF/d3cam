
#include "inc_files.h"

#include "csvCreatePoint3D.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include <ratio>
#include <chrono>
#include <opencv2/opencv.hpp>


#ifdef __cplusplus
extern "C" {
#endif

using namespace std;
using namespace std::chrono;
using namespace CSV;
using namespace cv;

vector<CsvImageSimple> leftCamList;
vector<CsvImageSimple> rightCamList;

int csv_3d_load_img (uint8_t rl, int rows, int cols, uint8_t *data)
{
	CsvImageSimple img(rows, cols, 1, data);

	if (CAM_LEFT == rl) {
		leftCamList.emplace_back(img);
	} else if (CAM_RIGHT == rl) {
		rightCamList.emplace_back(img);
	}

	return 0;
}

int csv_3d_clear_img (uint8_t rl)
{
	if (CAM_LEFT == rl) {
		leftCamList.clear();
	} else if (CAM_RIGHT == rl) {
		rightCamList.clear();
	}

	return 0;
}


bool ParseDepthImage2CVMat(CsvImageSimple &depthImage, Mat& out) {
	out = Mat(depthImage.m_height, depthImage.m_width, CV_16U, depthImage.m_data.data());
	out.row(0) = Scalar(0);
	return true;
}

int csv_save_pointXYZ (Mat& out, vector<float> *point3D)
{
	if (point3D != NULL) {
		vector<float> &vecPoint3D = *point3D;

		Mat out2 = Mat(out.rows, out.cols, CV_32FC3, vecPoint3D.data());
		ofstream outfile(gCSV->cfg.pointcloudcfg.outFileXYZ);

		for (int i = 0; i < out2.rows; i++) {
			Vec3f *p0 = out2.ptr<Vec3f>(i);
			for (int j = 0; j < out2.cols; j++) {
				Vec3f p = p0[j];
				if (p[2] < 0.01f) {
					continue;
				}
				outfile << p[0] << " " << p[1] << " " << p[2] << " " << "\n";
			}
		}
		outfile.close();

		return 0;
	}

	return -1;
}

int csv_3d_calc (uint8_t what)
{
	int ret = -1;
	bool d3calc = false;
	uint64_t f_timestamp = 0;
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;

	if ((NULL == pPC->calibFile)
		||(!csv_file_isExist(pPC->calibFile))) {
		log_warn("ERROR : calibFile null.");
		return -1;
	}

	if ((NULL == pPC->PCImageRoot)
		||(!csv_file_isExist(pPC->PCImageRoot))) {
		log_warn("ERROR : Point Cloud Image Root not exist.");
		return -1;
	}

	if (!pPC->initialized) {
		ret = csv_3d_init();
		if (ret < 0) {
			return -1;
		}
	}

	vector<vector<CsvImageSimple>> imageGroups;
	CsvImageSimple depthImage;
	vector<float> point3D;

	imageGroups.emplace_back(leftCamList);
	imageGroups.emplace_back(rightCamList);

	f_timestamp = utility_get_microsecond();
	log_debug("create 3d @ %ld us.", f_timestamp);

	d3calc = CsvCreatePoint3D(imageGroups, depthImage, &point3D);

	log_debug("create3d %d : take %ld us.", d3calc, utility_get_microsecond() - f_timestamp);

	f_timestamp = utility_get_microsecond();

	if (false == d3calc) {
		return -1;
	}

	Mat out1;
	ParseDepthImage2CVMat(depthImage, out1);

	log_debug("depthimage take %ld us.", utility_get_microsecond() - f_timestamp);

	switch (what) {
	case DEPTH_TO_FILE:
		if (pPC->saveDepthImage) {
			Mat vdisp;
			string outfilepng = string(pPC->outDepthImage);
			normalize(out1, vdisp, 0, 256, NORM_MINMAX, CV_8U);
			imwrite(outfilepng, vdisp);

			pthread_cond_broadcast(&gCSV->gx.cond_wait_depth);
		}

		if (pPC->saveXYZ) {
			f_timestamp = utility_get_microsecond();

			csv_save_pointXYZ(out1, &point3D);

			log_debug("save pointcloud take %ld us.", utility_get_microsecond() - f_timestamp);
		}
		break;

	case DEPTH_TO_STREAM: {
			struct gvsp_stream_t *pStream = &gCSV->gvsp.stream[CAM_DEPTH];
			Mat vdisp;
			GX_FRAME_BUFFER FrameBuffer;
			normalize(out1, vdisp, 0, 256, NORM_MINMAX, CV_8U);
			FrameBuffer.nPixelFormat = GX_PIXEL_FORMAT_MONO8;
			FrameBuffer.nHeight = vdisp.rows;
			FrameBuffer.nWidth = vdisp.cols;
			FrameBuffer.nOffsetX = 0;
			FrameBuffer.nOffsetY = 0;
			csv_gvsp_data_fetch(pStream, GVSP_PT_UNCOMPRESSED_IMAGE, vdisp.data, 
				vdisp.rows*vdisp.cols, &FrameBuffer, NULL);
		}
		break;
	}

	return 0;
}

int csv_3d_init (void)
{
	bool ret = false;
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;

	if ((NULL == pPC->calibFile)||(!csv_file_isExist(pPC->calibFile))) {
		log_warn("ERROR : calibFile null.");
		return -1;
	}

	if ((NULL == pPC->PCImageRoot)||(!csv_file_isExist(pPC->PCImageRoot))) {
		log_warn("ERROR : Point Cloud Image Root not exist.");
		return -1;
	}

	CsvCreatePoint3DParam param;
	param.calibXml = string(pPC->calibFile);
	param.modelPathFolder = string(pPC->ModelRoot);
	param.type = CSV_DataFormatType::FixPoint16bits;

	if (!pPC->initialized) {
		ret = CsvCreateLUT(param);
		if (ret) {
			pPC->initialized = 1;
			log_info("OK : Create LUT done.");
			csv_xml_write_PointCloudParameters();
		} else {
			log_info("ERROR : CsvCreateLUT.");
			return -1;
		}
	}

	ret = CsvSetCreatePoint3DParam(param); //set params
	if (!ret) {
		log_warn("ERROR : CsvSetCreatePoint3DParam");
		return -1;
	}

	CsvCreatePoint3DParam param0;
	ret = CsvGetCreatePoint3DParam(param0);
	if (!ret) {
		log_warn("ERROR : CsvGetCreatePoint3DParam");
		return -1;
	}

	log_info("calib xml : %s", param0.calibXml.c_str());

	return 0;
}


#ifdef __cplusplus
}
#endif

