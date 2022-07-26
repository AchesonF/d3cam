#ifndef __CSV_MEASURE3D_HPP__
#define __CSV_MEASURE3D_HPP__

#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include "csvCreatePoint3D.hpp"
#include "CsvType.hpp"
//#include "VisionHeadParam.h"

using namespace std;
using namespace cv;

namespace CSV {
	class CsvMeasure3D {
	public:
		CsvMeasure3D() { initialize(); }
		~CsvMeasure3D() {}

		void initialize();
		int loadModel(const string &filename);
		int pullStripeImages(vector<vector<Mat>>& inImageGroup); //fringe images : (2+7+4)*2=26
		int pushPointCloud(Mat& outPointCloud); //push out point cloud
		
		void csvMakeAtan2Map();
		void csvMakeg2dMap();
		void unwarpGrayCodePhaseShift();
		void stereoRectifyPhaseImage();
		void calcPointCloud();

		bool checkExchangeCameraPose();
		void getOpenCVParams(Mat& M1, Mat& D1, Mat& M2, Mat& D2, Mat& R, Mat& T, bool isExchage);

		unsigned int csvg2d(unsigned int x);

		void getMeanImage(Mat& src, Mat& mean);
		void getBinaryImage(Mat& src, Mat& binary, Mat& mean);
		void csvWrapPhase(const vector<Mat>& src_list, Mat& wrap_phase, Mat& atanMap);
		void csvUnwrapPhase(const vector<Mat>& binary_list, Mat& unwrap_phase, const Mat& wrap_phase, Mat& g2dmap, Mat& grayCode);
		void csvBilinearMap(Mat& src, Mat& dst, Mat& mapx, Mat& mapy); //Bilinear filtering

		void csvStereoRectify(Mat& M1, Mat& D1, Mat& M2, Mat& D2, Size imageSize,
			Mat& R, Mat& T, Mat& R1, Mat& R2, Mat& P1, Mat& P2, Mat& Q);
		void csvInitUndistortRectifyMap(Mat& cameraMatrix, Mat& distCoeffs,
			Mat& R, Mat& newCameraMatrix, Size size, Mat& map1, Mat& map2);
		void CsvReprojectImageTo3D(const Mat& disparity, Mat& out3D, const Mat& Q);
		void MedianFilter(const Mat& inM, Mat& outM, const int wnd_size);

	private:
//		shared_ptr<VisionHeadParam> vhead_param_; // 视觉头参数
		int m_idxC1, m_idxC2;
		Mat m_moveRange; // columu vector : x / y / z / dx / dy / dz

		string m_modelfileName;
		Mat M1, D1, M2, D2, R, T, m_Q;
		Mat atanMap, g2dmap;
		vector<vector<Mat>> imgGroupList;
		vector<Mat> unwrapPhaseList; //绝对相位图
		vector<Mat> undistortPhaseList; //
		Mat m_pointCloud;
	private:
	};
}
#endif //__CSV_MEASURE3D_HPP__


