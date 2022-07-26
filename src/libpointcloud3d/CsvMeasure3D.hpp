#ifndef __CSV_MEASURE3D_HPP__
#define __CSV_MEASURE3D_HPP__

#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include "csvCreatePoint3D.hpp"
#include "CsvType.hpp"
#include "VisionHeadParam.h"

#include "MacroCommon.h"

namespace CSV {
	class CsvMeasure3D {
	public:
		CsvMeasure3D() { initialize(); }
		~CsvMeasure3D() {}

		void initialize();
		//int laserRecogClassify(const std::vector<double> &point3D, std::vector<CsvLaserRecogResult>& result);
		int loadModel(const std::string &filename);
		int pullStripeImages(std::vector<std::vector<cv::Mat>>& inImageGroup); //fringe images : (2+7+4)*2=26
		int pushPointCloud(cv::Mat& outPointCloud); //push out point cloud
		
		void csvMakeAtan2Map();
		void csvMakeg2dMap();
		void unwarpGrayCodePhaseShift();
		void stereoRectifyPhaseImage();
		void calcPointCloud();

		bool checkExchangeCameraPose();
		void getOpenCVParams(cv::Mat& M1, cv::Mat& D1, cv::Mat& M2, cv::Mat& D2, cv::Mat& R, cv::Mat& T, bool isExchage);

		unsigned int csvg2d(unsigned int x);

		void getMeanImage(cv::Mat& src, cv::Mat& mean);
		void getBinaryImage(cv::Mat& src, cv::Mat& binary, cv::Mat& mean);
		void csvWrapPhase(const std::vector<cv::Mat>& src_list, cv::Mat& wrap_phase, cv::Mat& atanMap);
		void csvUnwrapPhase(const std::vector<cv::Mat>& binary_list, cv::Mat& unwrap_phase, const cv::Mat& wrap_phase, cv::Mat& g2dmap, cv::Mat& grayCode);
		void csvBilinearMap(cv::Mat& src, cv::Mat& dst, cv::Mat& mapx, cv::Mat& mapy); //Bilinear filtering

		void csvStereoRectify(cv::Mat& M1, cv::Mat& D1, cv::Mat& M2, cv::Mat& D2, cv::Size imageSize,
			cv::Mat& R, cv::Mat& T, cv::Mat& R1, cv::Mat& R2, cv::Mat& P1, cv::Mat& P2, cv::Mat& Q);
		void csvInitUndistortRectifyMap(cv::Mat& cameraMatrix, cv::Mat& distCoeffs,
			cv::Mat& R, cv::Mat& newCameraMatrix, cv::Size size, cv::Mat& map1, cv::Mat& map2);
		void CsvReprojectImageTo3D(const cv::Mat& disparity, cv::Mat& out3D, const cv::Mat& Q);
		void MedianFilter(const cv::Mat& inM, cv::Mat& outM, const int wnd_size);

#if (DEBUG_IMAGE_WRITE_FILE == 1)
		void writeVerticalCanvas(cv::Mat& img1r, cv::Mat& img2r, cv::Size& imageSize);
		void stereoRectifyGrayImage();
#endif

	private:
		std::shared_ptr<VisionHeadParam> vhead_param_; // 视觉头参数
		int m_idxC1, m_idxC2;
		cv::Mat m_moveRange; // columu vector : x / y / z / dx / dy / dz

		std::string m_modelfileName;
		cv::Mat M1, D1, M2, D2, R, T, m_Q;
		cv::Mat atanMap, g2dmap;
		std::vector<std::vector<cv::Mat>> imgGroupList;
		std::vector<cv::Mat> unwrapPhaseList; //绝对相位图
		std::vector<cv::Mat> undistortPhaseList; //
		cv::Mat m_pointCloud;
	private:
	};
}
#endif //__CSV_MEASURE3D_HPP__
