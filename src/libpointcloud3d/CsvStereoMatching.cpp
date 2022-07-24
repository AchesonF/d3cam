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
		min_disparity = (Q23 / settingZmin - Q33) / Q32;		// ��С�Ӳ�
		max_disparity = (Q23 / settingZmax - Q33) / Q32;		// ����Ӳ�

		phaseOffsetThreshold = 2.1;
		return true;
	}

	void CsvStereoMatching::ComputeCost(const cv::Mat& img_left, const cv::Mat& img_right, cv::Mat& disp_left) {

		// ���Ҿ�����λ����С������ͼ���Ӧ��Ϊͬ���㣨���ھ�����λ��ŷ�Ͼ��룩
		for (int y = 0; y < height_; y++) {
			const float* img_left_ = img_left.ptr<float>(y);
			const float* img_right_ = img_right.ptr<float>(y);
			float* disp_left_ = disp_left.ptr<float>(y);
			if (y == 547) {
				int kk = 0;
			}
			for (int x = 0; x < width_; x++) {
				float leftPhase = img_left_[x];
				// �ҵ���С����ֵ��Ӧ���Ӳ�
				float minDist = std::numeric_limits<float>::max();
				int minD = min_disparity; //��ͼ��С�����ƫ������
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
				float disparity = (float)minD; //���������Ӳ� d;
				// 1 - ��λ�����
				if (std::abs(leftPhase - rightPhase) > phaseOffsetThreshold) {
					disparity = invalidDisparity;
				}
				//else if (std::abs() > xThreshold) {
				//	disparity = invalidDisparity;
				//}
				// 2 - ������ X-axis ��Χ��

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
		invalidDisparity = std::numeric_limits<float>::infinity(); //���������ɺ������

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

		// �洢�ֲ������ڵ�����
		std::vector<float> wnd_data;
		wnd_data.reserve(size);
		const float* in = (const float*)inM.data;
		float* out = (float*)outM.data;

		for (int i = 0; i < height; i++) {			
			for (int j = 0; j < width; j++) {
				wnd_data.clear();

				// ��ȡ�ֲ���������
				for (int r = -radius; r <= radius; r++) {
					for (int c = -radius; c <= radius; c++) {
						const int row = i + r;
						const int col = j + c;
						if (row >= 0 && row < height && col >= 0 && col < width) {
							wnd_data.push_back(in[row * width + col]);
						}
					}
				}

				// ����
				std::sort(wnd_data.begin(), wnd_data.end());
				// ȡ��ֵ
				out[i * width + j] = wnd_data[wnd_data.size() / 2];
			}
		}
	}
}