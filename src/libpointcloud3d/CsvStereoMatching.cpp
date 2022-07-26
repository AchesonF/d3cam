#include <iostream>
#include <limits>
#include <algorithm>
#include "CsvStereoMatching.hpp"

namespace CSV
{
	CsvStereoMatching::CsvStereoMatching() {

	}
	CsvStereoMatching::~CsvStereoMatching() {

	}

	bool CsvStereoMatching::Initialize(cv::Mat& Q) {

		std::cout << std::endl << Q << std::endl;

		CV_Assert(Q.type() == CV_64F && Q.cols == 4 && Q.rows == 4);

		settingZmin = 900;						// Measurement setting depth Minimum
		settingZmax = 1200;						// Measurement setting Depth Max

		double Q23 = Q.at<double>(2, 3); // focal length
		double Q32 = Q.at<double>(3, 2);
		double Q33 = Q.at<double>(3, 3);

		//double d = 207;
		//double pw = 1.0f / (d * Q32 + Q33);
		//double z = Q23 * pw;
		min_disparity = (Q23 / settingZmin - Q33) / Q32;		// 最小视差
		max_disparity = (Q23 / settingZmax - Q33) / Q32;		// 最大视差

		phaseOffsetThreshold = 2.1;
		return true;
	}

	void CsvStereoMatching::ComputeCost(const cv::Mat& img_left, const cv::Mat& img_right, cv::Mat& disp_left) {

		// 查找绝对相位差最小的左右图像对应点为同名点（基于绝对相位的欧氏距离）
		for (int y = 0; y < height_; y++) {
			const float* img_left_ = img_left.ptr<float>(y);
			const float* img_right_ = img_right.ptr<float>(y);
			float* disp_left_ = disp_left.ptr<float>(y);
			/*if (y == 547) {
				int kk = 0;
			}*/
			for (int x = 0; x < width_; x++) {
				float leftPhase = img_left_[x];
				// 找到最小代价值对应的视差
				float minDist = std::numeric_limits<float>::max();
				int minD = min_disparity; //右图最小距离的偏移像素
				float rightPhase = std::numeric_limits<float>::max();
				for (int d = min_disparity; d < max_disparity; d++) {
					if (x - d < 0 || x - d >= width_) {
						continue;
					}
					float right = img_right_[x - d];
					float dist = std::abs(leftPhase - right);
					if (minDist > dist) {
						minDist = dist;
						minD = d;
						rightPhase = right;
					}
				}
				float disparity = (float)minD; //计算整数视差 d;
				// 1 - 相位差过滤
				if (std::abs(leftPhase - rightPhase) > phaseOffsetThreshold) {
					disparity = invalidDisparity;
				}
				//else if (std::abs() > xThreshold) {
				//	disparity = invalidDisparity;
				//}
				// 2 - 搜索在 X-axis 范围内

				// 2 - 
				disp_left_[x] = disparity;
			}
		}
	}
	bool CsvStereoMatching::Match(const cv::Mat& img_left, const cv::Mat& img_right, cv::Mat& disp_left) {

		width_ = img_left.cols;
		height_ = img_left.rows;
		const int disp_range = max_disparity - min_disparity;
		if (disp_range <= 0) {
			return false;
		}
		invalidDisparity = std::numeric_limits<float>::infinity(); //不是数即可后面过滤

		cv::Mat disparity(disp_left.size(), CV_32FC1, cv::Scalar(invalidDisparity));
		ComputeCost(img_left, img_right, disparity);

		MedianFilter(disparity, disp_left, 5);

		return true;
	}


	void CsvStereoMatching::MedianFilter(const cv::Mat& inM, cv::Mat& outM, const int wnd_size)
	{
		int width = inM.cols;
		int height = inM.rows;

		const int radius = wnd_size / 2;
		const int size = wnd_size * wnd_size;

		// 存储局部窗口内的数据
		std::vector<float> wnd_data;
		wnd_data.reserve(size);
		const float* in = (const float*)inM.data;
		float* out = (float*)outM.data;

		for (int i = 0; i < height; i++) {			
			for (int j = 0; j < width; j++) {
				wnd_data.clear();

				// 获取局部窗口数据
				for (int r = -radius; r <= radius; r++) {
					for (int c = -radius; c <= radius; c++) {
						const int row = i + r;
						const int col = j + c;
						if (row >= 0 && row < height && col >= 0 && col < width) {
							wnd_data.push_back(in[row * width + col]);
						}
					}
				}

				// 排序
				std::sort(wnd_data.begin(), wnd_data.end());
				// 取中值
				out[i * width + j] = wnd_data[wnd_data.size() / 2];
			}
		}
	}
}