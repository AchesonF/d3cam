
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
using namespace cv;

string to_zero_lead(const int value, const unsigned precision)
{
	ostringstream oss;
	oss << setw(precision) << setfill('0') << value;
	return oss.str();
}

static void loadSrcImageEx(string &pathRoot, vector<vector<Mat>> &imgGroupList)
{
	// 导入C1相机图像
	vector<Mat> src1list;
	for (int i = 0; i < 13; i++) {
		string path = pathRoot + "/" + gCSV->cfg.pointcloudcfg.imgPrefixNameL 
			+ to_zero_lead(i + 1, 3) + gCSV->cfg.devicecfg.strSuffix;

		cout << "Read Image : " << path << endl;

		Mat im = imread(path, IMREAD_GRAYSCALE);
		src1list.emplace_back(im);
	}
	imgGroupList.push_back(src1list);
	cout << "Read Image Num : " << src1list.size() << endl;

	// 导入2相机图像
	vector<Mat> src2list;
	for (int i = 0; i < 13; i++) {
		string path = pathRoot + "/" + gCSV->cfg.pointcloudcfg.imgPrefixNameR 
		+ to_zero_lead(i + 1, 3) + gCSV->cfg.devicecfg.strSuffix;

		cout << "Read Image : " << path << endl;

		Mat im = imread(path, IMREAD_GRAYSCALE);
		src2list.emplace_back(im);

	}
	imgGroupList.push_back(src2list);
	cout << "Read Image Num : " << src2list.size() << endl;
	return;
}

int point_cloud_calc(void)
{
	if ((NULL == gCSV->cfg.calibcfg.calibFile)
		||(!csv_file_isExist(gCSV->cfg.calibcfg.calibFile))) {
		log_info("ERROR : calibFile null");
		return -1;
	}

	if ((NULL == gCSV->cfg.pointcloudcfg.imgRoot)
		||(!csv_file_isExist(gCSV->cfg.pointcloudcfg.imgRoot))) {
		log_info("ERROR : imgRoot null");
		return -1;
	}

	string version =  CSV::csvGetCreatePoint3DALgVersion();
	cout << version << endl;

	CSV::CsvCreatePoint3DParam param;
	param.calibXml = string(gCSV->cfg.calibcfg.calibFile);
	param.type = CSV::CSV_DataFormatType::FixPoint64bits;

	CsvSetCreatePoint3DParam(param); //set params

	CSV::CsvCreatePoint3DParam param0;
	CsvGetCreatePoint3DParam(param0);
	cout << param0.calibXml << endl;
	cout << param0.type << endl;

	string imgRoot = string(gCSV->cfg.pointcloudcfg.imgRoot);
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

	gCSV->mvs.firstTimestamp = utility_get_microsecond();
	log_debug("create 3d @ %ld us.", gCSV->mvs.firstTimestamp);

	bool result = csvCreatePoint3D(imageGroups, pointCloud);

	gCSV->mvs.lastTimestamp = utility_get_microsecond();
	log_debug("create3d take %ld us. %d", gCSV->mvs.lastTimestamp - gCSV->mvs.firstTimestamp, result);

	Mat out = Mat(rows, cols, CV_32FC3, pointCloud.m_point3DData.data());
	ofstream outfile(gCSV->cfg.pointcloudcfg.outFileXYZ);
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

	return 0;

}




#ifdef __cplusplus
}
#endif

#endif