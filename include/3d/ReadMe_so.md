## 接口说明

| 接口名称 | 功能 |
| -------- | ---- |
| CsvSetCreatePoint3DParam | 把结构CsvCreatePoint3DParam传入系统 |
| CsvGetCreatePoint3DParam | 导出结构CsvCreatePoint3DParam |
| CsvCreateLUT | 创建系统运行需要的查找表（LUT） 传入参数是结构CsvCreatePoint3DParam；函数目标：生成正常运行时需要的查找表。调用方法：根据输入的xml文件，运行一次生成查找表即可。 |
| CsvCreatePoint3D	| 传入结构光图像组，传出深度图和点云。输入参数：inImages：CsvImageSimple类型的图像组（左右眼图像分组存放） depthImage：传出的深度图（左眼坐标系）point3D：传出的点云 |


## 版本说明
*****************************************************************************
V1.3.0
1、移植查找同名点流程到GPU；
2、移植点云转深度图流程到GPU；

*****************************************************************************
V1.2.0
1、使用几何投影计算的方式改写了全部的查找同名点算法流程；
2、使用统一的内存管理单元，管理图像内存。统一分配，统一释放；
3、按照模块化要求，重构了整个软件；

*****************************************************************************

## 调用流程

![flow](flow.png)


## 测试Demo

```cpp

//#include <omp.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <opencv2/opencv.hpp>

#include "csvCreatePoint3D.hpp"


std::string to_zero_lead(const int value, const unsigned precision)
{
	std::ostringstream oss;
	oss << std::setw(precision) << std::setfill('0') << value;
	return oss.str();
}


void loadSrcImage005(std::string& pathRoot, std::string& folder, std::vector<std::vector<cv::Mat>>& imgGroupList)
{
	//std::string folder_2391 = "B9w-dlp600-3\\";
	//std::string folder_2023 = "B9w-dlp600-3\\";

	std::vector<std::string> C_2391 = {
		"CSV_001C1S00P001.bmp",
		"CSV_001C1S00P002.bmp",
		"CSV_001C1S00P003.bmp",
		"CSV_001C1S00P004.bmp",
		"CSV_001C1S00P005.bmp",
		"CSV_001C1S00P006.bmp",
		"CSV_001C1S00P007.bmp",
		"CSV_001C1S00P008.bmp",
		"CSV_001C1S00P009.bmp",
		"CSV_001C1S00P010.bmp",
		"CSV_001C1S00P011.bmp",
		"CSV_001C1S00P012.bmp",
		"CSV_001C1S00P013.bmp"
	};
	std::vector<std::string> C_2023 = {
		"CSV_001C2S00P001.bmp",
		"CSV_001C2S00P002.bmp",
		"CSV_001C2S00P003.bmp",
		"CSV_001C2S00P004.bmp",
		"CSV_001C2S00P005.bmp",
		"CSV_001C2S00P006.bmp",
		"CSV_001C2S00P007.bmp",
		"CSV_001C2S00P008.bmp",
		"CSV_001C2S00P009.bmp",
		"CSV_001C2S00P010.bmp",
		"CSV_001C2S00P011.bmp",
		"CSV_001C2S00P012.bmp",
		"CSV_001C2S00P013.bmp"
	};
	// 导入C1相机图像
	std::vector<cv::Mat> src1list;
	for (int i = 0; i < 13; i++) {
		std::string path = pathRoot + folder + C_2391[i];

		std::cout << "Read Image : " << path << std::endl;

		cv::Mat im = cv::imread(path, cv::IMREAD_GRAYSCALE);
		src1list.emplace_back(im);
	}
	imgGroupList.push_back(src1list);
	std::cout << "Read Image Num : " << src1list.size() << std::endl;

	// 导入2相机图像
	std::vector<cv::Mat> src2list;
	for (int i = 0; i < 13; i++) {
		std::string path = pathRoot + folder + C_2023[i];

		std::cout << "Read Image : " << path << std::endl;

		cv::Mat im = cv::imread(path, cv::IMREAD_GRAYSCALE);
		src2list.emplace_back(im);

	}
	imgGroupList.push_back(src2list);
	std::cout << "Read Image Num : " << src2list.size() << std::endl;
	return;
}

std::vector<std::string> dataFolders = {
//"B10w-dlp700-4",
//"B10w-dlp800-4",
//"B1w-dlp700",
//"B2w-dlp600",
//"B2w-dlp700",
//"B2w-dlp700-2",
//"B3w-dlp600-2",
//"B3w-dlp700",
//"B3w-dlp700-2",
//"B3w-dlp800-2",
//"B4w-dlp600",
//"B4w-dlp600-2",
//"B4w-dlp700",
//"B4w-dlp700-2",
//"B4w-dlp800-2",
//"B5w-dlp600-2",
//"B5w-dlp700-2",
//"B5w-dlp700-3",
//"B5w-dlp800-2",
//"B6w-dlp700-3",
//"B7w-dlp500-4",
//"B7w-dlp600-4",
//"B7w-dlp700-3",
//"B7w-dlp700-4",
//"B7w-dlp800-4",
//"B8w-dlp600-4",
//"B8w-dlp700-3",
//"B8w-dlp700-4",
//"B9w-dlp500-3",
"B9w-dlp600-3"
//"B9w-dlp700-3",
//"B9w-dlp700-4"
};

bool ParseDepthImage2CVMat(CSV::CsvImageSimple &depthImage, cv::Mat& out) {
	out = cv::Mat(depthImage.m_height, depthImage.m_width, CV_16U, depthImage.m_data.data());
	out.row(0) = cv::Scalar(0);
	return true;
}


int main(int argc, char* argv[])
{
	int inputCommand = 1;
	for (int i = 0; i < argc; i++)
		cout << argv[i] << endl;
	if (argc == 2) {
		inputCommand = std::atoi(argv[1]);
	}


  std::string modelFolder = "./";
	std::string pathRoot = "/home/cosmosvision/markguo/";

	CSV::CsvCreatePoint3DParam param;
	param.calibXml = pathRoot + std::string("calibNew20220719.xml");
	param.modelPathFolder = modelFolder;


	param.type = CSV::CSV_DataFormatType::FixPoint16bits;

  if (inputCommand == 0){
    std::cout << "Create LUT Begin ..." << std::endl;
    CsvCreateLUT(param);
    std::cout << "Create LUT Over" << std::endl;
    return 1;
  }


	CsvSetCreatePoint3DParam(param); //set params

	CSV::CsvCreatePoint3DParam param0;
	CsvGetCreatePoint3DParam(param0);
	std::cout << "Calib XML : " << param0.calibXml << std::endl;
	std::cout << "Model Path Root : " << param0.modelPathFolder << std::endl;
	std::cout << param0.type << std::endl;

	for (size_t iFileGroup = 0; iFileGroup < dataFolders.size(); iFileGroup++) {
		std::string folder = dataFolders[iFileGroup] + "/";

		std::string imgRoot = pathRoot;
		std::vector<std::vector<cv::Mat>> imgGroupList;

		loadSrcImage005(imgRoot, folder, imgGroupList);

		std::vector<std::vector<CSV::CsvImageSimple>> imageGroups;


		int rows = imgGroupList[0][0].rows, cols = imgGroupList[0][0].cols;
		for (int g = 0; g < imgGroupList.size(); g++) {
			std::vector<CSV::CsvImageSimple> imgs;
			for (int i = 0; i < imgGroupList[g].size(); i++) {
				CSV::CsvImageSimple img(rows, cols, 1, imgGroupList[g][i].data);
				imgs.emplace_back(img);
			}
			imageGroups.emplace_back(imgs);
		}

		CSV::CsvImageSimple depthImage;
		std::vector<float> point3D;

		CsvTimer timer;

		bool result = CsvCreatePoint3D(imageGroups, depthImage, &point3D);

		std::cout << "CsvCreatePoint3D Timer ms :" << timer.elapsed() << std::endl;

		///////////////////////////////////////
		std::string outPngFilename = modelFolder + "Range_" + std::to_string(iFileGroup) + ".png";
		cv::Mat outPng;
		ParseDepthImage2CVMat(depthImage, outPng);
		cv::Mat vdisp;
		cv::normalize(outPng, vdisp, 0, 256, cv::NORM_MINMAX, CV_8U);
		cv::imwrite(outPngFilename, vdisp);

		std::string outPointCloudFilename = modelFolder + "PointCloud_" + std::to_string(iFileGroup) + ".xyz";
		cv::Mat out3D = cv::Mat(rows, cols, CV_32FC3, point3D.data());
		std::ofstream outfile(outPointCloudFilename);
		for (int i = 0; i < out3D.rows; i++) {
			cv::Vec3f *p0 = out3D.ptr<cv::Vec3f>(i);
			for (int j = 0; j < out3D.cols; j++) {
				cv::Vec3f p = p0[j];
				if (std::isnan(p[0]) || std::isnan(p[1]) || std::isnan(p[2])) {
					continue;
				}
				outfile << p[0] << " " << p[1] << " " << p[2] << " " << "\n";
			}
		}
		outfile.close();

	}


	return 0;

}

```