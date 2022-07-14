
#include "inc_files.h"

#if (USING_POINTCLOUD3D==1)

#include "csvCreatePoint3D.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include <ratio>
#include <chrono>
#include <opencv2/opencv.hpp>

#include "csvCreatePoint3D.hpp"

#ifdef __cplusplus
extern "C" {
#endif

using namespace std;
using namespace std::chrono;
using namespace CSV;

string to_zero_lead(const int value, const unsigned precision)
{
	ostringstream oss;
	oss << setw(precision) << setfill('0') << value;
	return oss.str();
}

static void loadSrcImageEx(string &pathRoot, vector<vector<cv::Mat>> &imgGroupList)
{
	// 导入C1相机图像
	vector<cv::Mat> src1list;
	for (int i = 0; i < 13; i++) {
		string path = pathRoot + gCSV->cfg.pointcloud_param.imgPrefixNameL 
			+ to_zero_lead(i + 1, 3) + ".png";

		cout << "Read Image : " << path << endl;

		cv::Mat im = cv::imread(path, cv::IMREAD_GRAYSCALE);
		src1list.emplace_back(im);
	}
	imgGroupList.push_back(src1list);
	cout << "Read Image Num : " << src1list.size() << endl;

	// 导入2相机图像
	vector<cv::Mat> src2list;
	for (int i = 0; i < 13; i++) {
		string path = pathRoot + gCSV->cfg.pointcloud_param.imgPrefixNameR 
		+ to_zero_lead(i + 1, 3) + ".png";

		cout << "Read Image : " << path << endl;

		cv::Mat im = cv::imread(path, cv::IMREAD_GRAYSCALE);
		src2list.emplace_back(im);

	}
	imgGroupList.push_back(src2list);
	cout << "Read Image Num : " << src2list.size() << endl;
	return;
}

int point_cloud_calc(void)
{
	if ((NULL == gCSV->cfg.pointcloud_param.calibFile)
		||(!csv_file_isExist(gCSV->cfg.pointcloud_param.calibFile))) {
		log_info("ERROR : calibFile null");
		return -1;
	}

	if ((NULL == gCSV->cfg.pointcloud_param.imgRoot)
		||(!csv_file_isExist(gCSV->cfg.pointcloud_param.imgRoot))) {
		log_info("ERROR : imgRoot null");
		return -1;
	}

	string version =  CSV::csvGetCreatePoint3DALgVersion();
	cout << version << endl;

	CSV::CsvCreatePoint3DParam param;
	param.calibXml = string(gCSV->cfg.pointcloud_param.calibFile);
	//param.outfile = string(gCSV->cfg.pointcloud_param.outFileXYZ);
	param.type = CSV::CSV_DataFormatType::FixPoint64bits;

	csvSetCreatePoint3DParam(param); //set params

	CSV::CsvCreatePoint3DParam param0;
	csvGetCreatePoint3DParam(param0);
	cout << param0.calibXml << endl;
	cout << param0.type << endl;

	string imgRoot = string(gCSV->cfg.pointcloud_param.imgRoot);
	vector<vector<cv::Mat>> imgGroupList;
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

	steady_clock::time_point t1 = steady_clock::now();
	bool result = csvCreatePoint3D(imageGroups, pointCloud);
	steady_clock::time_point t2 = steady_clock::now();
	duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
	cout << "It took me " << time_span.count() << " seconds."  << result << endl;

	return 0;

}




#ifdef __cplusplus
}
#endif

#endif