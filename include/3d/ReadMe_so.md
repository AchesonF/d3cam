## 接口说明

| 接口名称 | 功能 |
| -------- | ---- |
| CsvSetCreatePoint3DParam | 把结构CsvCreatePoint3DParam传入系统 |
| CsvGetCreatePoint3DParam | 导出结构CsvCreatePoint3DParam |
| CsvCreateLUT | 创建系统运行需要的查找表（LUT） |

传入参数是结构CsvCreatePoint3DParam；
系统第一次启动时运行一次。
CsvCreatePoint3D	传入结构光图像组，传出深度图和点云
输入参数：
inImages：CsvImageSimple类型的图像组（左右眼图像分组存放）
depthImage：传出的深度图（左眼坐标系）
point3D：传出的点云


## 版本说明
*****************************************************************************
V2.0.0
1、使用几何投影计算的方式改写了全部的查找同名点算法流程；
2、使用统一的内存管理单元，管理图像内存。统一分配，统一释放；
3、按照模块化要求，重构了整个软件；
*****************************************************************************

## 测试Demo

```cpp

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

void loadSrcImage002(std::string& pathRoot, std::vector<std::vector<cv::Mat>>& imgGroupList)
{
	std::string folder_2391 = "B9w-dlp600-3/";
	std::string folder_2023 = "B9w-dlp600-3/";

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
		std::string path = pathRoot + folder_2391 + C_2391[i];

		std::cout << "Read Image : " << path << std::endl;

		cv::Mat im = cv::imread(path, cv::IMREAD_GRAYSCALE);
		src1list.emplace_back(im);
	}
	imgGroupList.push_back(src1list);
	std::cout << "Read Image Num : " << src1list.size() << std::endl;

	// 导入2相机图像
	std::vector<cv::Mat> src2list;
	for (int i = 0; i < 13; i++) {
		std::string path = pathRoot + folder_2023 + C_2023[i];

		std::cout << "Read Image : " << path << std::endl;

		cv::Mat im = cv::imread(path, cv::IMREAD_GRAYSCALE);
		src2list.emplace_back(im);

	}
	imgGroupList.push_back(src2list);
	std::cout << "Read Image Num : " << src2list.size() << std::endl;
	return;
}

bool ParseDepthImage2CVMat(CSV::CsvImageSimple &depthImage, cv::Mat& out) {
	out = cv::Mat(depthImage.m_height, depthImage.m_width, CV_16U, depthImage.m_data.data());
	out.row(0) = cv::Scalar(0);
	return true;
}


int main(int argc, char* argv[])
{

	for(int i=0;i<argc;i++)
        std::cout << argv[i] << std::endl;
	
	std::string pathRoot = "/home/cosmosvision/markguo/";

	CSV::CsvCreatePoint3DParam param;

	param.calibXml = pathRoot + std::string("calibNew20220719.xml");
	param.modelPathFolder = "./";


	param.type = CSV::CSV_DataFormatType::FixPoint16bits;
	
	// 接口一：创建查找表。机器第一次运行，需要创建这些查找表模型。目录存放在 param.modelPathFolder 定义
	if (std::atoi(argv[1]) == 1){
		CsvCreateLUT(param);
		return 1;
	}
	
	// 接口二：正常使用时调用的流程，共有三个接口
	// 1 - CsvSetCreatePoint3DParam ：设置参数
	// 2 - CsvGetCreatePoint3DParam ：导出内部的参数
	// 3 - CsvCreatePoint3D ：输入图像组，得到深度图和点云
	CsvSetCreatePoint3DParam(param); //set params

	CSV::CsvCreatePoint3DParam param0;
	CsvGetCreatePoint3DParam(param0);
	std::cout << "Calib XML : " << param0.calibXml << std::endl;
	std::cout << "Model Path Root : " << param0.modelPathFolder << std::endl;
	std::cout << param0.type << std::endl;

	std::string imgRoot = pathRoot; 
	std::vector<std::vector<cv::Mat>> imgGroupList;

	loadSrcImage002(imgRoot, imgGroupList);

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
	bool result = CsvCreatePoint3D(imageGroups, depthImage, &point3D);


	//output Point Cloud to file
	
#if 1
	cv::Mat out0;
	ParseDepthImage2CVMat(depthImage, out0);
	cv::Mat vdisp;
	cv::normalize(out0, vdisp, 0, 256, cv::NORM_MINMAX, CV_8U);
	std::string f0001 = "./range.png";
	cv::imwrite(f0001, vdisp);
#endif

#if 1
	cv::Mat out = cv::Mat(out0.rows, out0.cols, CV_32FC3, point3D.data());
	std::ofstream outfile("./pointcloud0.xyz");
	for (int i = 0; i < out.rows; i++) {
		cv::Vec3f *p0 = out.ptr<cv::Vec3f>(i);
		for (int j = 0; j < out.cols; j++) {
			cv::Vec3f p = p0[j];
			if (std::isnan(p[0]) || std::isnan(p[1]) || std::isnan(p[2])) {
				continue;
			}
			outfile << p[0] << " " << p[1] << " " << p[2] << " " << "\n";
		}
	}
	outfile.close();
#endif

	return 0;

}

```