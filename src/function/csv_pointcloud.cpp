
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

		log_debug("imread : %s", path);

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

		log_debug("imread : %s", path);

		Mat im = imread(path, IMREAD_GRAYSCALE);
		src2list.emplace_back(im);
	}

	imgGroupList.push_back(src2list);
	log_debug("right image num : %d", src2list.size());

	return;
}

int point_cloud_calc(void)
{
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct pointcloud_cfg_t *pPC = &gCSV->cfg.pointcloudcfg;

	if ((NULL == pPC->calibFile)
		||(!csv_file_isExist(pPC->calibFile))) {
		log_info("ERROR : calibFile null");
		return -1;
	}

	if ((NULL == pPC->ImageSaveRoot)
		||(!csv_file_isExist(pPC->ImageSaveRoot))) {
		log_info("ERROR : ImageSaveRoot null");
		return -1;
	}

	string version =  CSV::csvGetCreatePoint3DALgVersion();
	cout << version << endl;

	CSV::CsvCreatePoint3DParam param;
	param.calibXml = string(pPC->calibFile);
	param.type = CSV::CSV_DataFormatType::FixPoint64bits;

	CsvSetCreatePoint3DParam(param); //set params

	CSV::CsvCreatePoint3DParam param0;
	CsvGetCreatePoint3DParam(param0);
	cout << param0.calibXml << endl;
	cout << param0.type << endl;

	string imgRoot = string(pPC->ImageSaveRoot);
	vector<vector<Mat>> imgGroupList;
	loadSrcImageEx(imgRoot, imgGroupList);

	vector<vector<CSV::CsvImageSimple>> imageGroups;
	CSV::CsvPoint3DCloud pointCloud;

	int rows = imgGroupList[0][0].rows, cols = imgGroupList[0][0].cols;
	for (unsigned int g = 0; g < imgGroupList.size(); g++) {
		vector<CSV::CsvImageSimple> imgs;
		for (unsigned int i = 0; i < imgGroupList[g].size(); i++) {
			CSV::CsvImageSimple img;
			img.m_height = rows;
			img.m_width = cols;
			img.m_widthStep = img.m_width;
			img.m_channel = 1;
			img.m_data = imgGroupList[g][i].data;
			imgs.emplace_back(img);
		}
		imageGroups.emplace_back(imgs);
	}

	pMVS->firstTimestamp = utility_get_microsecond();
	log_debug("create 3d @ %ld us.", pMVS->firstTimestamp);

	csvCreatePoint3D(imageGroups, pointCloud);

	pMVS->lastTimestamp = utility_get_microsecond();
	log_debug("create3d take %ld us.", pMVS->lastTimestamp - pMVS->firstTimestamp);

	pMVS->firstTimestamp = utility_get_microsecond();
	Mat out = Mat(rows, cols, CV_32FC3, pointCloud.m_point3DData.data());
	ofstream outfile(pPC->outFileXYZ);
	for (int i = 0; i < out.rows; i++) {
		Vec3f *p0 = out.ptr<Vec3f>(i);
		for (int j = 0; j < out.cols; j++) {
			Vec3f p = p0[j];
			if (isnan(p[0]) || isnan(p[1]) || isnan(p[2])){
				continue;
			}
			outfile << p[0] << " " << p[1] << " " << p[2] << " " << "\n";
		}
	}
	outfile.close();

	pMVS->lastTimestamp = utility_get_microsecond();
	log_debug("save pointcloud take %ld us.", pMVS->lastTimestamp - pMVS->firstTimestamp);

	pPC->groupPointCloud++;

	return 0;
}




#ifdef __cplusplus
}
#endif

