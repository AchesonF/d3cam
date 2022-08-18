
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

static void loadSrcImageEx(string &pathRoot, vector<vector<Mat>> &imgGroupList)
{
	int i = 0;
	char filename[512] = {0};
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;

	// 导入C1相机图像
	vector<Mat> src1list;
	for (i = 0; i < 13; i++) {
		memset(filename, 0, 512); // CSV_%03dC%dS00P%03d%s -> CSV_000C1S00P000.bmp
		snprintf(filename, 512, "CSV_%03dC1S00P%03d%s", pPC->groupPointCloud, 
			i+1, gCSV->cfg.devicecfg.strSuffix);

		string path = pathRoot + "/" + filename;

		cout << "Read Image : " << path << endl;

		Mat im = imread(path, IMREAD_GRAYSCALE);
		src1list.emplace_back(im);
	}
	imgGroupList.push_back(src1list);
	log_debug("left image num : %d", src1list.size());

	// 导入2相机图像
	vector<Mat> src2list;
	for (i = 0; i < 13; i++) {
		memset(filename, 0, 512); // CSV_%03dC%dS00P%03d%s -> CSV_000C2S00P000.bmp
		snprintf(filename, 512, "CSV_%03dC2S00P%03d%s", pPC->groupPointCloud, 
			i+1, gCSV->cfg.devicecfg.strSuffix);

		string path = pathRoot + "/" + filename;

		cout << "Read Image : " << path << endl;

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


int point_cloud_calc(void)
{
	static uint8_t create_once = 0;
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;

	if ((NULL == pPC->calibFile)
		||(!csv_file_isExist(pPC->calibFile))) {
		log_info("ERROR : calibFile null.");
		return -1;
	}

	if ((NULL == pPC->ImageSaveRoot)
		||(!csv_file_isExist(pPC->ImageSaveRoot))) {
		log_info("ERROR : ImageSaveRoot null.");
		return -1;
	}

	string version =  csvGetCreatePoint3DALgVersion();
	cout << version << endl;

	CsvCreatePoint3DParam param;
	param.calibXml = string(pPC->calibFile);
	param.modelPathFolder = "./";
	param.type = CSV_DataFormatType::FixPoint16bits;

	if (!create_once) {
		create_once = 1;
		CsvCreateLUT(param);
	}

	CsvSetCreatePoint3DParam(param); //set params

	CsvCreatePoint3DParam param0;
	CsvGetCreatePoint3DParam(param0);
	cout << "Calib XML : " << param0.calibXml << endl;
	cout << "Model Path Root : " << param0.modelPathFolder << endl;
	cout << param0.type << endl;

	string imgRoot = string(pPC->ImageSaveRoot);
	vector<vector<Mat>> imgGroupList;
	loadSrcImageEx(imgRoot, imgGroupList);

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

	pMVS->firstTimestamp = utility_get_microsecond();
	log_debug("create 3d @ %ld us.", pMVS->firstTimestamp);

	CsvCreatePoint3D(imageGroups, depthImage, &point3D);

	pMVS->lastTimestamp = utility_get_microsecond();
	log_debug("create3d take %ld us.", pMVS->lastTimestamp - pMVS->firstTimestamp);

	pMVS->firstTimestamp = utility_get_microsecond();
	string outfilepng = string(pPC->outDepthImage);
	Mat out1;
	ParseDepthImage2CVMat(depthImage, out1);

	Mat vdisp;
	normalize(out1, vdisp, 0, 256, NORM_MINMAX, CV_8U);
	imwrite(outfilepng, vdisp);
	pMVS->lastTimestamp = utility_get_microsecond();
	log_debug("save pointcloud take %ld us.", pMVS->lastTimestamp - pMVS->firstTimestamp);

#if 0
	Mat out2 = Mat(out1.rows, out1.cols, CV_32FC3, point3D.data());
	std::ofstream outfile(pPC->outFileXYZ);
	for (int i = 0; i < out2.rows; i++) {
		Vec3f *p0 = out2.ptr<Vec3f>(i);
		for (int j = 0; j < out2.cols; j++) {
			Vec3f p = p0[j];
			if (std::isnan(p[0]) || std::isnan(p[1]) || std::isnan(p[2])) {
				continue;
			}
			outfile << p[0] << " " << p[1] << " " << p[2] << " " << "\n";
		}
	}
	outfile.close();
#endif

	pPC->groupPointCloud++;

	return 0;
}




#ifdef __cplusplus
}
#endif

