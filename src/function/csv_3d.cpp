
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

void loadSrcImageEx(string &pathRoot, vector<vector<Mat>> &imgGroupList)
{
	int i = 0;
	char filename[512] = {0};
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;

	// 导入C1相机图像
	vector<Mat> src1list;
	for (i = 0; i < 13; i++) {
		memset(filename, 0, 512); // CSV_%03dC%dS00P%03d%s -> CSV_000C1S00P000.bmp
		snprintf(filename, 512, "CSV_%03dC1S00P%03d.bmp", pPC->groupPointCloud, i+1);

		string path = pathRoot + "/" + filename;

		log_debug("Read Image : %s", path.c_str());

		Mat im = imread(path, IMREAD_GRAYSCALE);
		src1list.emplace_back(im);
	}
	imgGroupList.push_back(src1list);
	log_debug("left image num : %d", src1list.size());

	// 导入2相机图像
	vector<Mat> src2list;
	for (i = 0; i < 13; i++) {
		memset(filename, 0, 512); // CSV_%03dC%dS00P%03d%s -> CSV_000C2S00P000.bmp
		snprintf(filename, 512, "CSV_%03dC2S00P%03d.bmp", pPC->groupPointCloud, i+1);

		string path = pathRoot + "/" + filename;

		log_debug("Read Image : %s", path.c_str());

		Mat im = imread(path, IMREAD_GRAYSCALE);
		src2list.emplace_back(im);
	}

	imgGroupList.push_back(src2list);
	log_debug("right image num : %d", src2list.size());

	return;
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

int csv_3d_calc (void)
{
	uint64_t f_timestamp = 0;
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;

	if ((NULL == pPC->calibFile)
		||(!csv_file_isExist(pPC->calibFile))) {
		log_warn("ERROR : calibFile null.");
		return -1;
	}

	if ((NULL == pPC->ImageSaveRoot)
		||(!csv_file_isExist(pPC->ImageSaveRoot))) {
		log_warn("ERROR : ImageSaveRoot null.");
		return -1;
	}

	if (!pPC->initialized) {
		csv_3d_init();
	}

	vector<vector<Mat>> imgGroupList;
	if (pPC->test_bmp) {
		string imgRoot = string("data/test_bmps");
		pPC->groupPointCloud = 1;
		loadSrcImageEx(imgRoot, imgGroupList);
	} else {
		string imgRoot = string(pPC->ImageSaveRoot);
		loadSrcImageEx(imgRoot, imgGroupList);
	}

	vector<vector<CsvImageSimple>> imageGroups;
	CsvImageSimple depthImage;
	vector<float> point3D;

	int rows = imgGroupList[0][0].rows, cols = imgGroupList[0][0].cols;
	for (unsigned int g = 0; g < imgGroupList.size(); g++) {
		vector<CsvImageSimple> imgs;
		for (unsigned int i = 0; i < imgGroupList[g].size(); i++) {
			CsvImageSimple img(rows, cols, 1, imgGroupList[g][i].data);
			imgs.emplace_back(img);
		}
		imageGroups.emplace_back(imgs);
	}

	f_timestamp = utility_get_microsecond();
	log_debug("create 3d @ %ld us.", f_timestamp);

	CsvCreatePoint3D(imageGroups, depthImage, &point3D);

	log_debug("create3d take %ld us.", utility_get_microsecond() - f_timestamp);

	f_timestamp = utility_get_microsecond();

	Mat out1;
	ParseDepthImage2CVMat(depthImage, out1);

	log_debug("depthimage take %ld us.", utility_get_microsecond() - f_timestamp);

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

	//pPC->groupPointCloud++;

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

	if ((NULL == pPC->ImageSaveRoot)||(!csv_file_isExist(pPC->ImageSaveRoot))) {
		log_warn("ERROR : ImageSaveRoot null.");
		return -1;
	}

	CsvCreatePoint3DParam param;
	param.calibXml = string(pPC->calibFile);
	param.modelPathFolder = "./";
	param.type = CSV_DataFormatType::FixPoint16bits;

	if (!pPC->initialized) {
		pPC->initialized = 1;

		ret = CsvCreateLUT(param);
		if (ret) {
			log_info("OK : Create LUT done.");
			csv_xml_write_PointCloudParameters();
		} else {
			log_info("ERROR : Create LUT failed.");
		}
	}

	CsvSetCreatePoint3DParam(param); //set params

	CsvCreatePoint3DParam param0;
	CsvGetCreatePoint3DParam(param0);
	log_info("calib xml : %s", param0.calibXml.c_str());

	return 0;
}


#ifdef __cplusplus
}
#endif

