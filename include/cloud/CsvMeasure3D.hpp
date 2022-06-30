#ifndef __CSV_MEASURE3D_HPP__
#define __CSV_MEASURE3D_HPP__

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include "csvCreatePoint3D.hpp"
#include "CsvType.hpp"

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

		unsigned int csvg2d(unsigned int x);

		void getMeanImage(cv::Mat& src, cv::Mat& mean);
		void getBinaryImage(cv::Mat& src, cv::Mat& binary, cv::Mat& mean);
		void csvWrapPhase(const std::vector<cv::Mat>& src_list, cv::Mat& wrap_phase, cv::Mat& atanMap);
		void csvUnwrapPhase(const std::vector<cv::Mat>& binary_list, cv::Mat& unwrap_phase, const cv::Mat& wrap_phase, cv::Mat& g2dmap, cv::Mat& grayCode);
		void csvBilinearMap(cv::Mat& src, cv::Mat& dst, cv::Mat& mapx, cv::Mat& mapy); //Bilinear filtering

	private:
		std::string m_modelfileName;
		cv::Mat M1, D1, M2, D2, R, T, m_Q;
		cv::Mat atanMap, g2dmap;
		std::vector<std::vector<cv::Mat>> imgGroupList;
		std::vector<cv::Mat> unwrapPhaseList; //������λͼ
		std::vector<cv::Mat> undistortPhaseList; //
		cv::Mat m_pointCloud;

		//debug
		std::string root;
	private:
	};
}
#endif //__CSV_MEASURE3D_HPP__
