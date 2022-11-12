
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

bool CsvC1RangeImageTo3DCloudXYZ(Mat& inRangeImage16, vector<float>& out3DXYZ)
{
	// the first row [minRange, scale, fx, fy, u0, v0, Rotate_rectified_to_camera1[3x3]] All is Float

	vector<double> calibInfo(2 + 4 + 3 * 3);
	memcpy((void*)calibInfo.data(), (void*)inRangeImage16.ptr(0, 0), sizeof(double) * calibInfo.size());
	double minRange = calibInfo[0];
	double scale = calibInfo[1];
	double maxRange = 65536.0 / scale + minRange;

	double fx = calibInfo[2];
	double fy = calibInfo[3];
	double u0 = calibInfo[4];
	double v0 = calibInfo[5];
	Mat R_rectify_to_camera1 = (Mat_<double>(3, 3) <<
		calibInfo[6], calibInfo[7], calibInfo[8],
		calibInfo[9], calibInfo[10], calibInfo[11],
		calibInfo[12], calibInfo[13], calibInfo[14]);

	Mat K = (Mat_<double>(3, 3) <<
		fx, 0, u0,
		0, fy, v0,
		0, 0, 1.0);
	Mat H_rectify_to_camera1 = K * R_rectify_to_camera1;
	Mat H_inv = H_rectify_to_camera1.inv();

	//cout << "K : " << endl << K << endl;
	//cout << "R_rectify_to_camera1 : " << endl << R_rectify_to_camera1 << endl;
	//cout << "H_inv : " << endl << H_inv << endl;

	int rows = inRangeImage16.rows, cols = inRangeImage16.cols;
	out3DXYZ.reserve(rows * cols * 3); // reserve space for vector

	for (int r = 1; r < rows; r++) {
		unsigned short *z = inRangeImage16.ptr<unsigned short>(r);
		//#pragma omp parallel for
		for (int c = 0; c < cols; c++) {
			if (z[c] > 0) {
				//float scale = (float)( / (inMaxRange - inMinRange));
				//ushort z = (ushort)((z0 - m_minRange) / m_maxRange * 65536.0);
				double w = ((double)z[c] * maxRange) / 65536.0 + minRange;
				double u = (double)c * w;
				double v = (double)r * w;
				Mat uvw = (Mat_<double>(3, 1) << u, v, w);
				Mat xyz = H_inv * uvw; // inv([K1] * [R_frREC2C1]) * [UVW]

				//std::cout << "z :" << z[c] << " r : " << r << " c : " << c << std::endl;
				//std::cout << "xyz :" << std::endl << xyz << std::endl;
				out3DXYZ.push_back((float)xyz.at<double>(0, 0));
				out3DXYZ.push_back((float)xyz.at<double>(1, 0));
				out3DXYZ.push_back((float)xyz.at<double>(2, 0));
			}
		}
	}

	return true;
}

string str3dErrCode (int errcode)
{
	string str_err = "";

	switch (errcode) {
	case CSV_OK:
		str_err = " : OK";
		break;
	case CSV_WARNING:
		str_err = toSTR(CSV_WARNING)" : warning";
		break;

	case CSV_ERR_UNKNOWN:
		str_err = toSTR(CSV_ERR_UNKNOWN)" : err unknown";
		break;

	case CSV_ERR_IS_RUNNING:
		str_err = toSTR(CSV_ERR_IS_RUNNING)" : err is running";
		break;

	case CSV_ERR_READ_LUT_FILE:
		str_err = toSTR(CSV_ERR_READ_LUT_FILE)" : err bad lut file";
		break;

	case CSV_ERR_LOAD_MODEL:
		str_err = toSTR(CSV_ERR_LOAD_MODEL)" : err load xml";
		break;

	case CSV_ERR_MODEL_NOT_READY:
		str_err = toSTR(CSV_ERR_MODEL_NOT_READY)" : err not load xml";
		break;

	case CSV_ERR_NEW_MEMORY:
		str_err = toSTR(CSV_ERR_NEW_MEMORY)" : err malloc memory";
		break;

	case CSV_ERR_LOAD_IMAGE:
		str_err = toSTR(CSV_ERR_LOAD_IMAGE)" : err load images";
		break;

	case CSV_ERR_CHECK_GPU:
		str_err = toSTR(CSV_ERR_CHECK_GPU)" : err gpu crash";
		break;

	case CSV_ERR_RELOAD_GPU:
		str_err = toSTR(CSV_ERR_RELOAD_GPU)" : err reload gpu";
		break;

	default:
		str_err = "unknown errcode";
		break;
	}

	return str_err;
}

static int csv_3d_save_depth (Mat& depthImgMat)
{
	struct pointcloud_conf_t *pPC = &gCSV->cfg.pointcloudcfg;

	if (pPC->saveXYZ) {
		uint64_t f_timestamp = 0;
		unsigned int cloudNum = 0;
		f_timestamp = utility_get_microsecond();

		ofstream outfile(pPC->outFileXYZ);
		vector<float> vector3DXYZParseFromDepthImage;
		CsvC1RangeImageTo3DCloudXYZ(depthImgMat, vector3DXYZParseFromDepthImage);

		for (size_t i = 0; i < vector3DXYZParseFromDepthImage.size(); i+=3) {
			float x = vector3DXYZParseFromDepthImage[i + 0];
			float y = vector3DXYZParseFromDepthImage[i + 1];
			float z = vector3DXYZParseFromDepthImage[i + 2];
			outfile << x << " " << y << " " << z << " " << "\n";
			cloudNum++;
		}
		outfile.close();
		log_debug("Depth Image To Cloud Dot Number : ", cloudNum);

		log_debug("save pointcloud take %ld us.", utility_get_microsecond() - f_timestamp);
	}

	Mat vdisp;
	string outfilepng = string(pPC->outDepthImage);
	depthImgMat.row(0) = Scalar(0); // remove the first row
	normalize(depthImgMat, vdisp, 0, 256, NORM_MINMAX, CV_8U);
	if (!outfilepng.empty()) {
		imwrite(outfilepng, vdisp);
	} else {
		log_warn("ERROR : filename 'outDepthImage' null.");
		return -1;
	}

	return 0;
}

static int csv_gvsp_depth_up (Mat& depthImgMat)
{
	struct gvsp_stream_t *pStream = &gCSV->gvsp.stream[CAM_DEPTH];
	Mat vdisp;
	GX_FRAME_BUFFER FrameBuffer;
	depthImgMat.row(0) = Scalar(0); // remove the first row
	normalize(depthImgMat, vdisp, 0, 256, NORM_MINMAX, CV_8U);

	FrameBuffer.nPixelFormat = GX_PIXEL_FORMAT_MONO8;
	FrameBuffer.nHeight = vdisp.rows;
	FrameBuffer.nWidth = vdisp.cols;
	FrameBuffer.nOffsetX = 0;
	FrameBuffer.nOffsetY = 0;

	return csv_gvsp_data_fetch(pStream, GVSP_PT_UNCOMPRESSED_IMAGE, vdisp.data, 
		vdisp.rows*vdisp.cols, &FrameBuffer, NULL);
}

static int csv_msg_depth_ack (Mat& depthImgMat)
{
	int len_msg = 0;
	int depthMatsize = 0;
	struct msg_ack_t *pACK = &gCSV->msg.ack;
	depthMatsize = depthImgMat.cols * depthImgMat.rows * 2;
	len_msg = sizeof(struct img_hdr_t) + depthMatsize;


	pACK->len_send = sizeof(struct msg_head_t) + len_msg + 4; // add 4 for tool bug
	pACK->buf_send = (uint8_t *)malloc(pACK->len_send + 1);
	if (NULL == pACK->buf_send) {
		log_err("ERROR : malloc send.");
		return -1;
	}

	memset(pACK->buf_send, 0, pACK->len_send+1);

	unsigned char *pS = pACK->buf_send;
	struct msg_head_t *pHDR = (struct msg_head_t *)pS;
	pHDR->cmdtype = CAMERA_GET_GRAB_DEEP;
	pHDR->length = len_msg + sizeof(pHDR->result);
	pHDR->result = 0;
	pS += sizeof(struct msg_head_t);

	struct img_hdr_t *pIMGHdr = (struct img_hdr_t *)pS;
	pIMGHdr->type = 3;
	pIMGHdr->cols = depthImgMat.cols;
	pIMGHdr->rows = depthImgMat.rows;
	pIMGHdr->channel = 2;	// 深度图数据是1通道CV_16S占两个字节
	pS += sizeof(struct img_hdr_t);
	memcpy(pS, depthImgMat.data, depthMatsize);

	return csv_msg_send(pACK);
}

int csv_3d_calc (eSEND_TO_t eSendTo)
{
	int ret = -1;
	uint64_t f_timestamp = 0;
	struct pointcloud_conf_t *pPC = &gCSV->cfg.pointcloudcfg;

	if ((NULL == pPC->calibFile)
		||(!csv_file_isPath(pPC->calibFile, S_IFREG))) {
		log_warn("ERROR : calibFile null.");
		return -1;
	}

	if ((NULL == pPC->PCImageRoot)
		||(!csv_file_isPath(pPC->PCImageRoot, S_IFDIR))) {
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
	ret = CsvCreatePoint3D(imageGroups, depthImage, &point3D);
	log_debug("create3d : take %ld us.", utility_get_microsecond() - f_timestamp);

	if (CSV_OK != ret) {
		log_info("ERROR : CsvCreatePoint3D retcode[%d] : %s.", ret, str3dErrCode(ret).c_str());
		return ret;
	}

	Mat u16Img(depthImage.m_height, depthImage.m_width, CV_16U, depthImage.m_data);

	switch (eSendTo) {
	case SEND_TO_INTERFACE:
		ret = csv_msg_depth_ack(u16Img);
		break;

	case SEND_TO_FILE:
		ret = csv_3d_save_depth(u16Img);
		break;

	case SEND_TO_STREAM:
		ret = csv_gvsp_depth_up(u16Img);
		break;
	default :
		return -1;
	}

	return ret;
}

int csv_3d_init (void)
{
	int ret = CSV_OK;
	struct pointcloud_conf_t *pPC = &gCSV->cfg.pointcloudcfg;

	if ((NULL == pPC->calibFile)||(!csv_file_isPath(pPC->calibFile, S_IFREG))) {
		log_warn("ERROR : calibFile null.");
		return -1;
	}

	if ((NULL == pPC->PCImageRoot)||(!csv_file_isPath(pPC->PCImageRoot, S_IFDIR))) {
		log_warn("ERROR : Point Cloud Image Root not exist.");
		return -1;
	}

	CsvCreatePoint3DParam param;
	param.calibXml = string(pPC->calibFile);
	param.modelPathFolder = string(pPC->ModelRoot);

	if (!pPC->initialized) {
		ret = CsvCreateLUT(param);
		if (CSV_OK == ret) {
			pPC->initialized = 1;
			log_info("OK : Create LUT done.");
			csv_xml_write_PointCloudParameters();
		} else {
			log_info("ERROR : CsvCreateLUT retcode[%d] : %s.", ret, str3dErrCode(ret).c_str());
			return ret;
		}
	}

	param.type = CSV_DataFormatType::FixPoint16bits;

	ret = CsvSetCreatePoint3DParam(param);
	if (CSV_OK != ret) {
		log_warn("ERROR : CsvSetCreatePoint3DParam retcode[%d] : %s.",  ret, str3dErrCode(ret).c_str());
		return ret;
	}

	log_info("OK : 3d init.");

	return 0;
}


#ifdef __cplusplus
}
#endif

