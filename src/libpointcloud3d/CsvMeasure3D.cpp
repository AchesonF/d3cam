#include <fstream> //debug
#include <limits>
#include <algorithm>
#include "CsvMeasure3D.hpp"
#include "CsvErrorType.hpp"
#include "CsvStereoMatching.hpp"
#include "CalibFileInfo.hpp"
//#include "spdlog_common.h"

using namespace std;

namespace CSV {

	void CsvMeasure3D::initialize() {
		cout << "Begin Measure 3D ..." << endl;
	}

	// only load param file now !!!
	int CsvMeasure3D::loadModel(const std::string &filename) {
		initialize();
		
		std::ifstream infile(filename); // check if file exist
		if (!infile.good()) {
			cout << "NO XML file: " << filename << endl;
			return ERR_LOAD_CALIB_FILE_FAILED;
		}
		cout << "Get XML file: " << filename << endl;

		m_modelfileName = filename;
		vhead_param_ = std::make_shared<VisionHeadParam>();
		if (CSV_OK != (CsvStatus)vhead_param_->load(filename))
		{
			return ERR_CREATE_LUT_FAILED;
		}

		bool isExchage = checkExchangeCameraPose();
		cout << "XML File Exchange: " << isExchage << endl;

		getOpenCVParams(M1, D1, M2, D2, R, T, isExchage);

#if (DEBUG_IMAGE_WRITE_FILE == 1)
		std::cout << "M1 : " << std::endl << M1 << std::endl;
		std::cout << "D1 : " << std::endl << D1 << std::endl;
		std::cout << "M2 : " << std::endl << M2 << std::endl;
		std::cout << "D1 : " << std::endl << D1 << std::endl;
		std::cout << "R : " << std::endl << R << std::endl;
		std::cout << "T : " << std::endl << T << std::endl;
#endif

		return CSV_OK;
	}

	//fringe images : (2+7+4)*2=26
	int CsvMeasure3D::pullStripeImages(std::vector<std::vector<cv::Mat>>& inImageGroup) {
		imgGroupList = inImageGroup; //input images group

#if (DEBUG_IMAGE_WRITE_FILE == 1)
		for (int group = 0; group < inImageGroup.size(); group++) {
			for (int i = 0; i < inImageGroup[group].size(); i++) {
				std::string filename1 = "./inC" + std::to_string(group + 1) + "_" + std::to_string(i + 1) + ".png";
				cv::Mat out1 = inImageGroup[group][i];
				cv::imwrite(filename1, out1);
			}
		}
#endif

		csvMakeAtan2Map();
		csvMakeg2dMap();
		unwarpGrayCodePhaseShift(); //计算绝对相位
		stereoRectifyPhaseImage(); //stereoRectify
		calcPointCloud();
		return CSV_OK;
	}

	//push out point cloud
	int CsvMeasure3D::pushPointCloud(cv::Mat& outPointCloud) {
		outPointCloud = m_pointCloud;


#if (DEBUG_IMAGE_WRITE_FILE == 1)
		//float rangeX0 = m_moveRange.at<double>(0, 0) - m_moveRange.at<double>(3, 0) * 0.5;
		//float rangeX1 = m_moveRange.at<double>(0, 0) + m_moveRange.at<double>(3, 0) * 0.5;
		//float rangeY0 = m_moveRange.at<double>(1, 0) - m_moveRange.at<double>(4, 0) * 0.5;
		//float rangeY1 = m_moveRange.at<double>(1, 0) + m_moveRange.at<double>(4, 0) * 0.5;
		//float rangeZ0 = m_moveRange.at<double>(2, 0) - m_moveRange.at<double>(5, 0) * 0.5;
		//float rangeZ1 = m_moveRange.at<double>(2, 0) + m_moveRange.at<double>(5, 0) * 0.5;

		std::ofstream outfile("./pointcloud.xyz");
		for (int i = 0; i < m_pointCloud.rows; i++) {
			cv::Vec3f *p0 = m_pointCloud.ptr<cv::Vec3f>(i);
			for (int j = 0; j < m_pointCloud.cols; j++) {
				cv::Vec3f p = p0[j];
				if (std::isnan(p[0]) || std::isnan(p[1]) || std::isnan(p[2])) {
					continue;
				}
				//if ((p[0] <= rangeX0 || p[0] >= rangeX1) ||  // x
				//	(p[1] <= rangeY0 || p[1] >= rangeY1) ||  // y
				//	(p[2] <= rangeZ0 || p[2] >= rangeZ1)) {    // z
				//	continue;
				//}
				outfile << p[0] << " " << p[1] << " " << p[2] << " " << "\n";
			}
		}
		outfile.close();
#endif

		return CSV_OK;
	}


	bool CsvMeasure3D::checkExchangeCameraPose() {
		//
		cv::Mat R_fromWtoC1, T_fromWtoC1; // from world to c1
		cv::Mat R_fromWtoC2, T_fromWtoC2; // from world to c2
		cv::Mat R_fromC1toC2, T_fromC1toC2;
		{
			const double* tdata = &(vhead_param_->camparam_list[0].exparam.getRotMat()(0));
			double ttdata[9];
			memcpy(ttdata, tdata, sizeof(double) * 9);
			cv::Mat Rmat(3, 3, CV_64FC1, ttdata);
			R_fromWtoC1 = Rmat.clone();
			CSV::Point3d m = vhead_param_->camparam_list[0].exparam.getTrans();
			T_fromWtoC1 = (cv::Mat_<double>(3, 1) << m.x, m.y, m.z);
		}
		{
			const double* tdata = &(vhead_param_->camparam_list[1].exparam.getRotMat()(0));
			double ttdata[9];
			memcpy(ttdata, tdata, sizeof(double) * 9);
			cv::Mat Rmat(3, 3, CV_64FC1, ttdata);
			R_fromWtoC2 = Rmat.clone();
			CSV::Point3d m = vhead_param_->camparam_list[1].exparam.getTrans();
			T_fromWtoC2 = (cv::Mat_<double>(3, 1) << m.x, m.y, m.z);
		}

		//OpenCV立体标定中的的R表示右相机相对于左相机的旋转 R_fromC1_toC2
		cv::Mat R = R_fromWtoC2 * R_fromWtoC1.t();
		cv::Mat T = T_fromWtoC2 - R * T_fromWtoC1;

		bool isExchangeCameraPose = false;
		if (T.at<double>(0, 0) < 0.0) { //C2 要在 C1 的 X 正方向，否则就是反的
			isExchangeCameraPose = true;
		}

		return isExchangeCameraPose;
	}

	void CsvMeasure3D::getOpenCVParams(cv::Mat& M1, cv::Mat& D1, cv::Mat& M2, cv::Mat& D2, cv::Mat& R, cv::Mat& T, bool isExchage) {
		// CSV change to OpenCV
		/*
		| fx  0   cx |
		| 0   fy  cy |
		| 0   0   1  |
		*/
		int idxC1 = 0, idxC2 = 1;
		if (isExchage) {
			idxC1 = 1;
			idxC2 = 0;
		}
		m_idxC1 = idxC1;
		m_idxC2 = idxC2;
		{
			double d[INTRIN_PARAM_NUM]; // 内参，数组顺序：fx, fy, cx, cy, s, k1, k2, k3, p1, p2	
			vhead_param_->camparam_list[idxC1].inparam.getParam(d);
			double fx = d[0], fy = d[1], cx = d[2], cy = d[3];
			double k1 = d[5], k2 = d[6], k3 = d[7];
			double p1 = 2.0 * d[9] / (fx + fy); // p1_cv = 2*p2/(fx+fy)
			double p2 = 2.0 * d[8] / (fx + fy); // p2_cv = 2*p1/(fx+fy)
			M1 = (cv::Mat_<double>(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1); //fx,0,cx / 0, fy,cy / 0,0,1
			D1 = (cv::Mat_<double>(1, 5) << k1, k2, p1, p2, k3); // k1,k2,p1,p2,k3
		}
		{
			double d[INTRIN_PARAM_NUM]; // 内参，数组顺序：fx, fy, cx, cy, s, k1, k2, k3, p1, p2	
			vhead_param_->camparam_list[idxC2].inparam.getParam(d);
			double fx = d[0], fy = d[1], cx = d[2], cy = d[3];
			double k1 = d[5], k2 = d[6], k3 = d[7];
			double p1 = 2.0 * d[9] / (fx + fy); // p1_cv = 2*p2/(fx+fy)
			double p2 = 2.0 * d[8] / (fx + fy); // p2_cv = 2*p1/(fx+fy)
			M2 = (cv::Mat_<double>(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1); //fx,0,cx / 0, fy,cy / 0,0,1
			D2 = (cv::Mat_<double>(1, 5) << k1, k2, p1, p2, k3); // k1,k2,p1,p2,k3
		}

		//
		cv::Mat R_fromWtoC1, T_fromWtoC1; // from world to c1
		cv::Mat R_fromWtoC2, T_fromWtoC2; // from world to c2
		cv::Mat R_fromC1toC2, T_fromC1toC2;
		{
			const double* tdata = &(vhead_param_->camparam_list[idxC1].exparam.getRotMat()(0));
			double ttdata[9];
			memcpy(ttdata, tdata, sizeof(double) * 9);
			cv::Mat Rmat(3, 3, CV_64FC1, ttdata);
			R_fromWtoC1 = Rmat.clone();
			CSV::Point3d m = vhead_param_->camparam_list[idxC1].exparam.getTrans();
			T_fromWtoC1 = (cv::Mat_<double>(3, 1) << m.x, m.y, m.z);
		}
		{
			const double* tdata = &(vhead_param_->camparam_list[idxC2].exparam.getRotMat()(0));
			double ttdata[9];
			memcpy(ttdata, tdata, sizeof(double) * 9);
			cv::Mat Rmat(3, 3, CV_64FC1, ttdata);
			R_fromWtoC2 = Rmat.clone();
			CSV::Point3d m = vhead_param_->camparam_list[idxC2].exparam.getTrans();
			T_fromWtoC2 = (cv::Mat_<double>(3, 1) << m.x, m.y, m.z);
		}

		//OpenCV立体标定中的的R表示右相机相对于左相机的旋转 R_fromC1_toC2
		R = R_fromWtoC2 * R_fromWtoC1.t();
		T = T_fromWtoC2 - R * T_fromWtoC1;
	}

	void CsvMeasure3D::csvMakeAtan2Map()
	{
		atanMap = cv::Mat(511, 511, CV_32FC1);
		for (int r = 0; r < 511; r++) {
			float* p = atanMap.ptr<float>(r);
			for (int c = 0; c < 511; c++) {
				p[c] = std::atan2(r - 255, c - 255);
			}
		}
	}

	unsigned int CsvMeasure3D::csvg2d(unsigned int x)
	{
		unsigned int y = x;
		while (x >>= 1)
			y ^= x;
		return y;
	}

	void CsvMeasure3D::csvMakeg2dMap()
	{
		g2dmap = cv::Mat(1, 128, CV_32S);
		cv::Mat_<int>& g2dmap_ = (cv::Mat_<int>&)g2dmap;

		for (size_t i = 0; i < 128; i++) {
			g2dmap_(0, i) = csvg2d(i);
		}
	}


	// 均值图像
	void CsvMeasure3D::getMeanImage(cv::Mat& src, cv::Mat& mean) {
		mean = cv::Mat(src.size(), CV_8UC1);
		for (int y = 0; y < src.rows; y++) {
			uchar* p = src.ptr<uchar>(y);
			uchar* q = mean.ptr<uchar>(y);
			for (int x = 0; x < src.cols; x++) {
				q[x] = p[x] >> 1;
			}
		}
	}

	void CsvMeasure3D::getBinaryImage(cv::Mat& src, cv::Mat& binary, cv::Mat& mean) {
		binary = cv::Mat(src.size(), CV_8UC1);
		for (int y = 0; y < src.rows; y++) {
			uchar* p = src.ptr<uchar>(y);
			uchar* q = binary.ptr<uchar>(y);
			uchar* m = mean.ptr<uchar>(y);
			for (int x = 0; x < src.cols; x++) {
				q[x] = (p[x] > m[x]) ? 1 : 0;
			}
		}
	}

	void CsvMeasure3D::csvWrapPhase(const std::vector<cv::Mat>& src_list, cv::Mat& wrap_phase, cv::Mat& atanMap)
	{
		//src_list.size() == PHASE_SHIFT_NUM;

		// 相对相位包	
		//int offset = 0;
		float phase_value = 0;
		//int mean_value = 0;
		int rows = src_list[0].rows, cols = src_list[0].cols;
		wrap_phase = cv::Mat(rows, cols, CV_32FC1);

		for (int r = 0; r < rows; r++) {
			const uchar* row_phase1_data = src_list[0].ptr<uchar>(r);
			const uchar* row_phase2_data = src_list[1].ptr<uchar>(r);
			const uchar* row_phase3_data = src_list[2].ptr<uchar>(r);
			const uchar* row_phase4_data = src_list[3].ptr<uchar>(r);
			float* row_wrap_phase_data = wrap_phase.ptr<float>(r);
			for (int c = 0; c < cols; c++) {
				int x = row_phase1_data[c] - row_phase3_data[c] + 255;// +255 为了查表
				int y = row_phase4_data[c] - row_phase2_data[c] + 255;
				float m = atanMap.at<float>(y, x);
				phase_value = m > 0 ? m : (m + CSV_PI_4HALF); // LUT查询actan表
				row_wrap_phase_data[c] = phase_value;
			}
		}
	}


	void CsvMeasure3D::csvUnwrapPhase(const std::vector<cv::Mat>& binary_list, cv::Mat& unwrap_phase, const cv::Mat& wrap_phase, cv::Mat& g2dmap, cv::Mat& grayCode)
	{
		grayCode = cv::Mat(wrap_phase.size(), CV_32SC1); //debug

		unwrap_phase = cv::Mat(wrap_phase.size(), CV_32FC1);
		// 计算绝对相位	
		cv::Mat_<int>& g2dmap_ = (cv::Mat_<int>&)g2dmap;
		int rows = binary_list[0].rows, cols = binary_list[0].cols;
		for (int r = 0; r < rows; r++) {
			const uchar* row_bindata0 = binary_list[0].ptr<uchar>(r);
			const uchar* row_bindata1 = binary_list[1].ptr<uchar>(r);
			const uchar* row_bindata2 = binary_list[2].ptr<uchar>(r);
			const uchar* row_bindata3 = binary_list[3].ptr<uchar>(r);
			const uchar* row_bindata4 = binary_list[4].ptr<uchar>(r);
			const uchar* row_bindata5 = binary_list[5].ptr<uchar>(r);
			const uchar* row_bindata6 = binary_list[6].ptr<uchar>(r);
			const float* row_wrap_phase = wrap_phase.ptr<float>(r);
			float* row_unwrap_phase = unwrap_phase.ptr<float>(r);

			int* row_gray_code = grayCode.ptr<int>(r);

			for (int c = 0; c < cols; c++) {
				//计算格雷码
				int graycode6 = (row_bindata0[c] << 5) +
					(row_bindata1[c] << 4) +
					(row_bindata2[c] << 3) +
					(row_bindata3[c] << 2) +
					(row_bindata4[c] << 1) +
					(row_bindata5[c]);
				int graycode7 = (graycode6 << 1) + row_bindata6[c];
				int k1 = g2dmap_(graycode6);// 格雷码查表
				int k2 = (g2dmap_(graycode7) + 1) >> 1;  // int((v2+1)/2)

				row_gray_code[c] = k2; //debug

				float offset_phase = row_wrap_phase[c] < CSV_PI_HALF ? CSV_PI_4HALF * k2 :
					(row_wrap_phase[c] < CSV_PI_3HALF ? CSV_PI_4HALF * k1 : (CSV_PI_4HALF * (k2 - 1)));
				row_unwrap_phase[c] = row_wrap_phase[c] + offset_phase;
			}
		}
	}


	void CsvMeasure3D::unwarpGrayCodePhaseShift() {
		unwrapPhaseList.clear(); //clear

		for (int group = 0; group < 2; group++) {
			std::vector<cv::Mat> imgGroup = imgGroupList[group];
			// 1 - 均值图像
			cv::Mat mean;
			getMeanImage(imgGroup[0], mean);

			//2 -格雷码图像二值化
			std::vector<cv::Mat> binaryList;
			for (int i = WHITE_BLACK_NUM; i < GRAY_CODE_NUM + WHITE_BLACK_NUM; i++) {
				cv::Mat grayImg = imgGroup[i];
				cv::Mat binary;
				getBinaryImage(grayImg, binary, mean);
				binaryList.emplace_back(binary);

#if (DEBUG_IMAGE_WRITE_FILE == 1)
				std::string filename1 = "..\\temp\\binary_C" + std::to_string(group + 1) + "_" + std::to_string(i + 1) + ".png";
				cv::Mat out1 = binary * 255;
				cv::imwrite(filename1, out1);
#endif
			}

			//3.相移图-相对相位（wrap）
			cv::Mat wrap_phase;
			std::vector<cv::Mat> phaseList;
			for (int i = 0; i < PHASE_SHIFT_NUM; i++) {
				phaseList.emplace_back(imgGroup[GRAY_CODE_NUM + 2 + i]);
			}
			csvWrapPhase(phaseList, wrap_phase, atanMap);
#if (DEBUG_IMAGE_WRITE_FILE == 1)
			{
				cv::Mat vdisp;
				cv::normalize(wrap_phase, vdisp, 0, 256, cv::NORM_MINMAX, CV_8U);
				std::string f0001 = "..\\temp\\wrapPhase_C" + std::to_string(group + 1) + ".png";
				cv::imwrite(f0001, vdisp);			
			}
#endif
			//4.相移图-解包（unwrap）
			cv::Mat unwrap_phase, grayCode;
			csvUnwrapPhase(binaryList, unwrap_phase, wrap_phase, g2dmap, grayCode);
#if (DEBUG_IMAGE_WRITE_FILE == 1)
			{
				cv::Mat vdisp;
				cv::normalize(unwrap_phase, vdisp, 0, 256, cv::NORM_MINMAX, CV_8U);
				std::string f0001 = "..\\temp\\unwrapPhase_C" + std::to_string(group + 1) + ".png";
				cv::imwrite(f0001, vdisp);			
			}
#endif
			// keep 
			unwrapPhaseList.emplace_back(unwrap_phase);
		}
	}

#if (DEBUG_IMAGE_WRITE_FILE == 1)

	void CsvMeasure3D::writeVerticalCanvas(cv::Mat& img1r, cv::Mat& img2r, cv::Size& imageSize) {
		cv::Mat canvas;
		int w = imageSize.width;
		int h = imageSize.height;
		double sf = 1280.0 / (h * 2);
		canvas.create(h * 2, w, CV_8UC3);
		cv::Mat canvasPart = canvas(cv::Rect(0, 0, w, h));
		cv::Mat img1r0, img2r0;

		cvtColor(img1r, img1r0, cv::COLOR_GRAY2BGR);
		cvtColor(img2r, img2r0, cv::COLOR_GRAY2BGR);

		img1r0.copyTo(canvasPart);
		//cv::rectangle(canvasPart, stereoParams.validROIL, cv::Scalar(0, 0, 255), 3, 8);
		canvasPart = canvas(cv::Rect(0, h, w, h));
		img2r0.copyTo(canvasPart);
		//cv::rectangle(canvasPart, stereoParams.validROIR, cv::Scalar(0, 255, 0), 3, 8);
		for (int i = 0; i < canvas.cols; i += 64)
			cv::line(canvas, cv::Point(i, 0), cv::Point(i, canvas.rows - 1), cv::Scalar(0, 255, 0), 1, 8);

		cv::line(canvas, cv::Point(0, canvas.rows / 2 - 1), cv::Point(canvas.cols - 1, canvas.rows / 2 - 1), cv::Scalar(0, 0, 255), 2, 8);
		cv::line(canvas, cv::Point(0, canvas.rows / 2), cv::Point(canvas.cols - 1, canvas.rows / 2), cv::Scalar(0, 0, 255), 2, 8);

		std::string remapImg = "..\\temp\\remap_stereo_vertical.png";
		//cv::imwrite(remapImg, canvas);
		cv::Mat rectified;
		cv::resize(canvas, rectified, cv::Size(0, 0), sf, sf, cv::INTER_LINEAR);
		cv::imwrite(remapImg, canvas);
	}

	void CsvMeasure3D::stereoRectifyGrayImage() {
		cv::Mat img1 = cv::imread("..\\cailibImage-g\\CSV_252C1.png", 0);
		cv::Mat img2 = cv::imread("..\\cailibImage-g\\CSV_252C2.png", 0);

		cv::Size imageSize(IM_WIDTH, IM_HEIGHT);
		cv::Mat R1, R2, P1, P2, map11, map12, map21, map22;
		cv::stereoRectify(M1, D1, M2, D2, imageSize, R, T, R1, R2, P1, P2, m_Q, 0);

		std::cout << "R1 : " << std::endl << R1 << std::endl << "P1 : " << std::endl << P1 << std::endl;
		std::cout << "R2 : " << std::endl << R2 << std::endl << "P2 : " << std::endl << P2 << std::endl;

		// Precompute maps for cvRemap()
		initUndistortRectifyMap(M1, D1, R1, P1, imageSize, CV_32FC1, map11, map12);
		initUndistortRectifyMap(M2, D2, R2, P2, imageSize, CV_32FC1, map21, map22);


		std::cout << map11.type() << "  " << map12.type() << std::endl;
		//std::cout << "pointsC1 :" << std::endl;
		//for (size_t i = 0; i < pointsC1.size(); i++) {
		//	//根据矫正平后的像素点，找到矫正前的像素
		//	cv::Point p((int)pointsC1[i].x, (int)pointsC1[i].y);
		//	float x = map11.at<float>(p.y, p.x);
		//	float y = map12.at<float>(p.y, p.x);
		//	std::cout << x << "  " << y << std::endl;
		//}
		//std::cout << "pointsC2 :" << std::endl;
		//for (size_t i = 0; i < pointsC2.size(); i++) {
		//	//根据矫正平后的像素点，找到矫正前的像素
		//	cv::Point p((int)pointsC2[i].x, (int)pointsC2[i].y);
		//	float x = map21.at<float>(p.y, p.x);
		//	float y = map22.at<float>(p.y, p.x);
		//	std::cout << x << "  " << y << std::endl;
		//}

		cv::Mat img1r, img2r, disp, vdisp;
		cv::remap(img1, img1r, map11, map12, cv::INTER_LINEAR);
		cv::remap(img2, img2r, map21, map22, cv::INTER_LINEAR);

		cv::imwrite("..\\temp\\C1_undistort.png", img1r);
		cv::imwrite("..\\temp\\C2_undistort.png", img2r);

		std::vector<cv::Mat> C1C2;

		C1C2.emplace_back(img1r);
		C1C2.emplace_back(img2r);


		//{
		//	cv::Point C10 = cv::Point(508, 547); //C1_undistort.png -- left big dot
		//	cv::Point C11 = cv::Point(556, 596);
		//	cv::Rect rect = cv::Rect(0, C10.y, img1r.cols, (C11.y - C10.y));
		//	cv::imwrite("..\\temp\\C1_undistort_roi.png", img1r(rect));
		//	cv::imwrite("..\\temp\\C2_undistort_roi.png", img2r(rect));
		//}


		cv::Mat canvas;
		int w = imageSize.width;
		int h = imageSize.height;
		double sf = 1280.0 / (w * 2);
		canvas.create(h, w * 2, CV_8UC3);
		cv::Mat canvasPart = canvas(cv::Rect(0, 0, w, h));
		cv::Mat img1r0, img2r0;

		cvtColor(C1C2[0], img1r0, cv::COLOR_GRAY2BGR);
		cvtColor(C1C2[1], img2r0, cv::COLOR_GRAY2BGR);

		img1r0.copyTo(canvasPart);
		//cv::rectangle(canvasPart, stereoParams.validROIL, cv::Scalar(0, 0, 255), 3, 8);
		canvasPart = canvas(cv::Rect(w, 0, w, h));
		img2r0.copyTo(canvasPart);
		//cv::rectangle(canvasPart, stereoParams.validROIR, cv::Scalar(0, 255, 0), 3, 8);
		for (int i = 0; i < canvas.rows; i += 64)
			cv::line(canvas, cv::Point(0, i), cv::Point(canvas.cols, i), cv::Scalar(0, 255, 0), 1, 8);

		cv::line(canvas, cv::Point(canvas.cols / 2 - 1, 0), cv::Point(canvas.cols / 2 - 1, canvas.rows - 1), cv::Scalar(0, 0, 255), 2, 8);
		cv::line(canvas, cv::Point(canvas.cols / 2, 0), cv::Point(canvas.cols / 2, canvas.rows - 1), cv::Scalar(0, 0, 255), 2, 8);

		std::string remapImg = "..\\temp\\remap_stereo.png";
		//cv::imwrite(remapImg, canvas);
		cv::Mat rectified;
		cv::resize(canvas, rectified, cv::Size(0, 0), sf, sf, cv::INTER_LINEAR);
		cv::imwrite(remapImg, canvas);

		writeVerticalCanvas(C1C2[0], C1C2[1], imageSize);
	}
#endif

	void CsvMeasure3D::stereoRectifyPhaseImage() {
#if (DEBUG_IMAGE_WRITE_FILE == 1)
		stereoRectifyGrayImage();
#endif
		cv::Mat img1 = unwrapPhaseList[0];
		cv::Mat img2 = unwrapPhaseList[1];

		cv::Size imageSize(IM_WIDTH, IM_HEIGHT);
		cv::Mat R1, R2, P1, P2, map11, map12, map21, map22;
#if 1
		cv::stereoRectify(M1, D1, M2, D2, imageSize, R, T, R1, R2, P1, P2, m_Q, 0);
#if (DEBUG_IMAGE_WRITE_FILE == 1)
		std::cout << "P1 : " << std::endl << P1 << std::endl;
		std::cout << "P2 : " << std::endl << P2 << std::endl;
		std::cout << "m_Q : " << std::endl << m_Q << std::endl;

#endif
		// Precompute maps for cvRemap()
		initUndistortRectifyMap(M1, D1, R1, P1, imageSize, CV_32FC1, map11, map12);
		initUndistortRectifyMap(M2, D2, R2, P2, imageSize, CV_32FC1, map21, map22);
#else
		csvStereoRectify(M1, D1, M2, D2, imageSize, R, T, R1, R2, P1, P2, m_Q);
#if (DEBUG_IMAGE_WRITE_FILE == 1)
		std::cout << "P1 : " << std::endl << P1 << std::endl;
		std::cout << "P2 : " << std::endl << P2 << std::endl;
		std::cout << "m_Q : " << std::endl << m_Q << std::endl;
#endif
		csvInitUndistortRectifyMap(M1, D1, R1, P1, imageSize, map11, map12);
		csvInitUndistortRectifyMap(M2, D2, R2, P2, imageSize, map21, map22);
#endif

		cv::Mat img1r, img2r;
		csvBilinearMap(img1, img1r, map11, map12);
		csvBilinearMap(img2, img2r, map21, map22);

		undistortPhaseList.emplace_back(img1r);
		undistortPhaseList.emplace_back(img2r);
	}


	// https://blog.csdn.net/yegeli/article/details/117354090
	// 立体校正源码分析(opencv)
	void CsvMeasure3D::csvStereoRectify(cv::Mat& M1, cv::Mat& D1, cv::Mat& M2, cv::Mat& D2, cv::Size imageSize,
		cv::Mat& R, cv::Mat& T, cv::Mat& R1, cv::Mat& R2, cv::Mat& P1, cv::Mat& P2, cv::Mat& Q)
	{
		// if float then convert to double
		if ((M1.type() != CV_64F) || (M2.type() != CV_64F) || (D1.type() != CV_64F) || (D2.type() != CV_64F) || (R.type() != CV_64F) || (T.type() != CV_64F)) {
			M1.convertTo(M1, CV_64F);
			M2.convertTo(M2, CV_64F);
			D1.convertTo(D1, CV_64F);
			D2.convertTo(D2, CV_64F);
			R.convertTo(R, CV_64F);
			T.convertTo(T, CV_64F);
		}

		// (1)共面：先把旋转矩阵变为旋转向量，对旋转向量的模长平分，这样使两个图像平面共面，此时行未对齐。
		cv::Mat om, r_r, t0, t;
		cv::Rodrigues(R, om);
		om *= -0.5;  // get average rotation

		// (2)行对准：建立行对准换行矩阵Rrect使极点转换到无穷远处。
		cv::Rodrigues(om, r_r); // 旋转向量转换为旋转矩阵
		//cout << r_r << endl;

		t0 = r_r * T;
		t = r_r * -T; // T 从左指向右，而从右向左是相机坐标系的正向，所以这里加负号
		//cout << t << endl;
		int idx = 0;
		if (fabs(T.at<double>(0, 0)) > fabs(T.at<double>(1, 0)))
			idx = 0; //Horizontal epipolar lines
		else
			idx = 1; //Vertical epipolar lines

		cv::Mat e1 = t / cv::norm(t);
		double Tx = t.at<double>(0, 0), Ty = t.at<double>(1, 0);
		cv::Mat e2 = (cv::Mat_<double>(3, 1) << -Ty, Tx, 0);
		e2 /= std::sqrt(Tx * Tx + Ty * Ty);
		cv::Mat e3 = e1.cross(e2);
		cv::Mat R_rect = (cv::Mat_<double>(3, 3) <<
			e1.at<double>(0, 0), e1.at<double>(1, 0), e1.at<double>(2, 0),
			e2.at<double>(0, 0), e2.at<double>(1, 0), e2.at<double>(2, 0),
			e3.at<double>(0, 0), e3.at<double>(1, 0), e3.at<double>(2, 0));
		//cout << R_rect << endl;

		R1 = R_rect * r_r.t();    //C1
		R2 = R_rect * r_r;        //C2

		//cout << R1 << endl;
		//cout << R2 << endl;

		// calculate projection/camera matrices
		// these contain the relevant rectified image internal params (fx, fy=fx, cx, cy)
		double nx = imageSize.width, ny = imageSize.height;
		double fc_new = std::numeric_limits<double>::max();
		cv::Point2d cc_new[2] = {};

		cv::Size newImgSize = {};
		newImgSize = newImgSize.width * newImgSize.height != 0 ? newImgSize : imageSize;
		const double ratio_x = (double)newImgSize.width / imageSize.width / 2;
		const double ratio_y = (double)newImgSize.height / imageSize.height / 2;
		const double ratio = idx == 1 ? ratio_x : ratio_y;
		fc_new = (M1.at<double>(idx ^ 1, idx ^ 1) + M2.at<double>(idx ^ 1, idx ^ 1)) * ratio;

		for (int k = 0; k < 2; k++) {
			const cv::Mat A = k == 0 ? M1 : M2;
			const cv::Mat Dk = k == 0 ? D1 : D2;
			std::vector<cv::Point2f> pts(4);
			std::vector<cv::Point3f> pts_3(4);
			for (int i = 0; i < 4; i++)
			{
				int j = (i < 2) ? 0 : 1;
				pts[i].x = (float)((i % 2)*(nx - 1));
				pts[i].y = (float)(j*(ny - 1));
			}
			//std::vector<cv::Point2f> corners_image_undistortPoints;
			cv::undistortPoints(pts, pts, A, Dk, cv::Mat(), cv::Mat());
			//cv::convertPointsFromHomogeneous(pts, pts_3);
			for (int i = 0; i < 4; i++) {
				pts_3[i].x = pts[i].x;
				pts_3[i].y = pts[i].y;
				pts_3[i].z = 1.0;
			}

			//Change camera matrix to have cc=[0,0] and fc = fc_new
			cv::Mat Z = cv::Mat::zeros(3, 1, CV_64F);
			cv::Mat A_tmp = cv::Mat(3, 3, CV_64F);
			A_tmp.at<double>(0, 0) = fc_new;
			A_tmp.at<double>(1, 1) = fc_new;
			A_tmp.at<double>(0, 2) = 0.0;
			A_tmp.at<double>(1, 2) = 0.0;
			//cvProjectPoints2(&pts_3, k == 0 ? R1 : R2, &Z, &A_tmp, 0, &pts);
			//std::vector<cv::Point2f> projectedPoints;
			cv::Mat distZeros = cv::Mat::zeros(5, 1, CV_64F); //distCoeffs
			cv::projectPoints(pts_3, k == 0 ? R1 : R2, Z, A_tmp, cv::Mat(), pts);
			cv::Scalar avg = cv::mean(pts);
			cc_new[k].x = (nx - 1) / 2 - avg.val[0];
			cc_new[k].y = (ny - 1) / 2 - avg.val[1];
		}
		if (idx == 0) // horizontal stereo
			cc_new[0].y = cc_new[1].y = (cc_new[0].y + cc_new[1].y)*0.5;
		else // vertical stereo
			cc_new[0].x = cc_new[1].x = (cc_new[0].x + cc_new[1].x)*0.5;

		cv::Mat pp = cv::Mat::zeros(3, 4, CV_64F);
		pp.at<double>(0, 0) = pp.at<double>(1, 1) = fc_new;
		pp.at<double>(0, 2) = cc_new[0].x;
		pp.at<double>(1, 2) = cc_new[0].y;
		pp.at<double>(2, 2) = 1;
		P1 = pp.clone();

		pp.at<double>(0, 2) = cc_new[1].x;
		pp.at<double>(1, 2) = cc_new[1].y;
		pp.at<double>(idx, 3) = T.at<double>(idx, 0) * fc_new; // baseline * focal length
		P2 = pp.clone();
		{
			double q[] =
			{
				1, 0, 0, -cc_new[0].x,
				0, 1, 0, -cc_new[0].y,
				0, 0, 0, fc_new,
				0, 0, -1. / t0.at<double>(idx, 0),
				(idx == 0 ? cc_new[0].x - cc_new[1].x : cc_new[0].y - cc_new[1].y) / t0.at<double>(idx, 0)
			};
			cv::Mat matQ = cv::Mat(4, 4, CV_64F, q);
			Q = matQ.clone();
		}
	}

	//参数说明：
	//cameraMatrix——输入的摄像头内参数矩阵（3X3矩阵）
	//distCoeffs——输入的摄像头畸变系数矩阵（5X1矩阵）
	//R——输入的第一和第二摄像头坐标系之间的旋转矩阵
	//newCameraMatrix——输入的校正后的3X3摄像机矩阵
	//size——摄像头采集的无失真图像尺寸
	//m1type——map1的数据类型，可以是CV_32FC1或CV_16SC2
	//map1——输出的X坐标重映射参数
	//map2——输出的Y坐标重映射参数

	// https://github.com/egonSchiele/OpenCV/blob/master/modules/imgproc/src/undistort.cpp
	void CsvMeasure3D::csvInitUndistortRectifyMap(cv::Mat& cameraMatrix, cv::Mat& distCoeffs,
		cv::Mat& R, cv::Mat& newCameraMatrix, cv::Size size, cv::Mat& map1, cv::Mat& map2) {

		map1.create(size, CV_32FC1);
		map2.create(size, CV_32FC1);

		cv::Mat A = cameraMatrix.clone();
		cv::Mat Ar = newCameraMatrix.clone();

		//CV_Assert(A.size() == Size(3, 3) && A.size() == R.size());
		//CV_Assert(Ar.size() == Size(3, 3) || Ar.size() == Size(4, 3));
		cv::Mat_<double> iR = (Ar.colRange(0, 3)*R).inv(cv::DECOMP_LU); //使用newCameraMatrix的逆，像素坐标系转相机坐标系
		const double* ir = &iR(0, 0);

		double u0 = A.at<double>(0, 2), v0 = A.at<double>(1, 2);
		double fx = A.at<double>(0, 0), fy = A.at<double>(1, 1);

		double k1 = ((double*)distCoeffs.data)[0];
		double k2 = ((double*)distCoeffs.data)[1];
		double p1 = ((double*)distCoeffs.data)[2];
		double p2 = ((double*)distCoeffs.data)[3];
		double k3 = distCoeffs.cols + distCoeffs.rows - 1 >= 5 ? ((double*)distCoeffs.data)[4] : 0.;
		double k4 = distCoeffs.cols + distCoeffs.rows - 1 >= 8 ? ((double*)distCoeffs.data)[5] : 0.;
		double k5 = distCoeffs.cols + distCoeffs.rows - 1 >= 8 ? ((double*)distCoeffs.data)[6] : 0.;
		double k6 = distCoeffs.cols + distCoeffs.rows - 1 >= 8 ? ((double*)distCoeffs.data)[7] : 0.;


		for (int i = 0; i < size.height; i++)
		{
			float* m1f = (float*)(map1.data + map1.step*i);
			float* m2f = (float*)(map2.data + map2.step*i);
			//short* m1 = (short*)m1f;
			//ushort* m2 = (ushort*)m2f;
			double _x = i * ir[1] + ir[2], _y = i * ir[4] + ir[5], _w = i * ir[7] + ir[8];

			for (int j = 0; j < size.width; j++, _x += ir[0], _y += ir[3], _w += ir[6])
			{
				double w = 1. / _w, x = _x * w, y = _y * w;
				double x2 = x * x, y2 = y * y;
				double r2 = x2 + y2, _2xy = 2 * x*y;
				double kr = (1 + ((k3*r2 + k2)*r2 + k1)*r2) / (1 + ((k6*r2 + k5)*r2 + k4)*r2);
				double u = fx * (x*kr + p1 * _2xy + p2 * (r2 + 2 * x2)) + u0;
				double v = fy * (y*kr + p1 * (r2 + 2 * y2) + p2 * _2xy) + v0;
				//if (m1type == CV_32FC1)
				m1f[j] = (float)u;
				m2f[j] = (float)v;
			}
		}
	}


	void CsvMeasure3D::csvBilinearMap(cv::Mat& src, cv::Mat& dst, cv::Mat& mapx, cv::Mat& mapy) {
		dst = cv::Mat(src.size(), CV_32FC1);
		int width = src.cols, height = src.rows;

		auto getx = [=](int k) { return (std::max)((std::min)(k, width - 1), 0); };
		auto gety = [=](int k) { return (std::max)((std::min)(k, height - 1), 0); };

		for (int r = 0; r < src.rows; r++) {
			const float* pX = mapx.ptr<float>(r);
			const float* pY = mapy.ptr<float>(r);
			float* pD = dst.ptr<float>(r);
			for (int c = 0; c < src.cols; c++) {
				float x = pX[c];
				float y = pY[c];
				int px = (int)x; // floor of x
				int py = (int)y; // floor of y
				// p1 p2
				// p3 p4
				float p1 = src.at<float>(gety(py), getx(px));
				float p2 = src.at<float>(gety(py), getx(px + 1));
				float p3 = src.at<float>(gety(py + 1), getx(px));
				float p4 = src.at<float>(gety(py + 1), getx(px + 1));
				// Calculate the weights for each pixel
				float fx = x - px;
				float fy = y - py;
				float fx1 = 1.0f - fx;
				float fy1 = 1.0f - fy;

				float w1 = fx1 * fy1; // (1-x)(1-y)
				float w2 = fx * fy1;  // x(1-y)
				float w3 = fx1 * fy;  // (1-x)y
				float w4 = fx * fy;   // xy
				float out = p1 * w1 + p2 * w2 + p3 * w3 + p4 * w4;
				pD[c] = out;
			}
		}
	}

	void CsvMeasure3D::calcPointCloud() {
		//cv::Size imageSize(IM_WIDTH, IM_HEIGHT);

		cv::Mat C1 = undistortPhaseList[0];
		cv::Mat C2 = undistortPhaseList[1];
		
		int rows = C1.rows, cols = C1.cols;
		cv::Mat disparityImage(cv::Size(cols, rows), CV_32FC1); //视差图

		
		CsvStereoMatching csvSM;
		csvSM.Initialize(m_Q);
		csvSM.Match(C1, C2, disparityImage);

#if (DEBUG_IMAGE_WRITE_FILE == 1)
		cv::Mat vdisp;
		cv::normalize(disparityImage, vdisp, 0, 256, cv::NORM_MINMAX, CV_8U);
		cv::imwrite("..\\temp\\disparity.png", vdisp);
#endif

		//! Reproject to compute xyz
		m_pointCloud = cv::Mat(disparityImage.size(), CV_32FC3);
		//cv::reprojectImageTo3D(disparityImage, m_pointCloud, m_Q);
		CsvReprojectImageTo3D(disparityImage, m_pointCloud, m_Q);
	}


	void CsvMeasure3D::CsvReprojectImageTo3D(const cv::Mat& disparity, cv::Mat& out3D, const cv::Mat& Q) {
		//CV_Assert(disparity.type() == CV_32F && !disparity.empty());
		//CV_Assert(Q.type() == CV_64F && Q.cols == 4 && Q.rows == 4);

		out3D = cv::Mat::zeros(disparity.size(), CV_32FC3);

		double Q03 = Q.at<double>(0, 3);
		double Q13 = Q.at<double>(1, 3);
		double Q23 = Q.at<double>(2, 3);
		double Q32 = Q.at<double>(3, 2);
		double Q33 = Q.at<double>(3, 3);

		for (int i = 0; i < disparity.rows; i++) {
			const float* disp_ptr = disparity.ptr<float>(i);
			cv::Vec3f* out3D_ptr = out3D.ptr<cv::Vec3f>(i);

			for (int j = 0; j < disparity.cols; j++)
			{
				const float pw = 1.0f / (disp_ptr[j] * Q32 + Q33);

				cv::Vec3f& point = out3D_ptr[j];
				point[0] = (static_cast<float>(j) + Q03) * pw;
				point[1] = (static_cast<float>(i) + Q13) * pw;
				point[2] = Q23 * pw;
			}
		}
	}


}