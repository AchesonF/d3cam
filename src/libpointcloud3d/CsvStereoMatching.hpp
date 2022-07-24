#ifndef _CSV_STEREO_MATCHING_HPP
#define _CSV_STEREO_MATCHING_HPP

#include <vector>
#include <opencv2/opencv.hpp>

namespace CSV
{
	class CsvStereoMatching
	{
	public:
		CsvStereoMatching();
		~CsvStereoMatching();
		//
		bool Initialize(cv::Mat& Q);
		bool Match(const cv::Mat& img_left, const cv::Mat& img_right, cv::Mat& disp_left);
		void ComputeCost(const cv::Mat& img_left, const cv::Mat& img_right, cv::Mat& disp_left);
		void MedianFilter(const cv::Mat& inM, cv::Mat& outM, const int wnd_size);

	private:
		int width_, height_; // image width and height
		float invalidDisparity;
		//
		int settingZmin;					// Minimum drawing depth (Term : mm)
		int settingZmax;					// Maximum depth during drawing
		//
		int min_disparity;		// ��С�Ӳ�
		int	max_disparity;		// ����Ӳ�

		float phaseOffsetThreshold;	// ͬ������λ���ƫ����ֵ
	};
}

#endif //_CSV_STEREO_MATCHING_HPP