#include <fstream> //debug
#include <iostream>
#include "CsvMeasure3D.hpp"
#include "CsvErrorType.hpp"

namespace CSV {

	void CsvMeasure3D::initialize() {
		root = "../../";
	}

	// only load param file now !!!
	int CsvMeasure3D::loadModel(const std::string &filename) {
		initialize();
		m_modelfileName = filename;

		//std::string xmlFile = root + "target/grayCodeParams.xml";
		std::string xmlFile = m_modelfileName;
		//cv::Mat M1, D1, M2, D2, R, T;
		cv::Size imageSize;
		cv::FileStorage fs(xmlFile, cv::FileStorage::READ);
		if (!fs.isOpened()){
			std::cout << "No file XML 11" << std::endl;
			return -1;
		}
		
		fs["M1"] >> M1;
		fs["D1"] >> D1;
		fs["M2"] >> M2;
		fs["D2"] >> D2;
		fs["imageSize"] >> imageSize;
		fs["R"] >> R;
		fs["T"] >> T;
		fs.release();

		return CSV_OK;
	}

	//fringe images : (2+7+4)*2=26
	int CsvMeasure3D::pullStripeImages(std::vector<std::vector<cv::Mat>>& inImageGroup) {
		imgGroupList = inImageGroup; //input images group

		csvMakeAtan2Map();
		csvMakeg2dMap();
		unwarpGrayCodePhaseShift(); //���������λ
		stereoRectifyPhaseImage(); //stereoRectify
		calcPointCloud();
		return CSV_OK;
	}

	//push out point cloud
	int CsvMeasure3D::pushPointCloud(cv::Mat& outPointCloud) {
		outPointCloud = m_pointCloud;

#if 1
		std::string outfilename = "./pointcloud.xyz";
		std::ofstream outfile(outfilename);
		for (int i = 0; i < m_pointCloud.rows; i++) {
			cv::Vec3f *p0 = m_pointCloud.ptr<cv::Vec3f>(i);
			for (int j = 0; j < m_pointCloud.cols; j++) {
				cv::Vec3f p = p0[j];
				outfile << p[0] << " " << p[1] << " " << p[2] << " " << "\n";
			}
		}
		outfile.close();
#endif
		return CSV_OK;
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


	// ��ֵͼ��
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

		// �����λ��	
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
				int x = row_phase1_data[c] - row_phase3_data[c] + 255;// +255 Ϊ�˲��
				int y = row_phase4_data[c] - row_phase2_data[c] + 255;
				float m = atanMap.at<float>(y, x);
				phase_value = m > 0 ? m : (m + CSV_PI_4HALF); // LUT��ѯactan��
				row_wrap_phase_data[c] = phase_value;
			}
		}
	}


	void CsvMeasure3D::csvUnwrapPhase(const std::vector<cv::Mat>& binary_list, cv::Mat& unwrap_phase, const cv::Mat& wrap_phase, cv::Mat& g2dmap, cv::Mat& grayCode)
	{
		grayCode = cv::Mat(wrap_phase.size(), CV_32SC1); //debug

		unwrap_phase = cv::Mat(wrap_phase.size(), CV_32FC1);
		// ���������λ	
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
				//���������
				int graycode6 = (row_bindata0[c] << 5) +
					(row_bindata1[c] << 4) +
					(row_bindata2[c] << 3) +
					(row_bindata3[c] << 2) +
					(row_bindata4[c] << 1) +
					(row_bindata5[c]);
				int graycode7 = (graycode6 << 1) + row_bindata6[c];
				int k1 = g2dmap_(graycode6);// ��������
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
			// 1 - ��ֵͼ��
			cv::Mat mean;
			getMeanImage(imgGroup[0], mean);

			//2 -������ͼ���ֵ��
			std::vector<cv::Mat> binaryList;
			for (int i = WHITE_BLACK_NUM; i < GRAY_CODE_NUM + WHITE_BLACK_NUM; i++) {
				cv::Mat grayImg = imgGroup[i];
				cv::Mat binary;
				getBinaryImage(grayImg, binary, mean);
				binaryList.emplace_back(binary);
			}

			//3.����ͼ-�����λ��wrap��
			cv::Mat wrap_phase;
			std::vector<cv::Mat> phaseList;
			for (int i = 0; i < PHASE_SHIFT_NUM; i++) {
				phaseList.emplace_back(imgGroup[GRAY_CODE_NUM + 2 + i]);
			}
			csvWrapPhase(phaseList, wrap_phase, atanMap);

			//4.����ͼ-�����unwrap��
			cv::Mat unwrap_phase, grayCode;
			csvUnwrapPhase(binaryList, unwrap_phase, wrap_phase, g2dmap, grayCode);

			// keep 
			unwrapPhaseList.emplace_back(unwrap_phase);
		}
	}


	void CsvMeasure3D::stereoRectifyPhaseImage() {
		cv::Mat img1 = unwrapPhaseList[0];
		cv::Mat img2 = unwrapPhaseList[1];

		cv::Size imageSize(IM_WIDTH, IM_HEIGHT);
		cv::Mat R1, R2, P1, P2, map11, map12, map21, map22;
		cv::stereoRectify(M1, D1, M2, D2, imageSize, R, T, R1, R2, P1, P2, m_Q, 0);

		// Precompute maps for cvRemap()
		initUndistortRectifyMap(M1, D1, R1, P1, imageSize, CV_32FC1, map11, map12);
		initUndistortRectifyMap(M2, D2, R2, P2, imageSize, CV_32FC1, map21, map22);


		cv::Mat img1r, img2r;
		csvBilinearMap(img1, img1r, map11, map12);
		csvBilinearMap(img2, img2r, map21, map22);

		undistortPhaseList.emplace_back(img1r);
		undistortPhaseList.emplace_back(img2r);
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
		cv::Mat disparityImage(cv::Size(cols, rows), CV_32FC1, cv::Scalar(0)); //�Ӳ�ͼ

		for (int y = 0; y < rows; y++) {
			if (y == 21) {
				//int k = 9;
			}
			float* disparityPtr = disparityImage.ptr<float>(y);
			float* lLine = C1.ptr<float>(y);
			float* rLine = C2.ptr<float>(y);

			//����Լ����һ����λֵ��������
			//������Լ����һ����Լ����Ψһ��Լ��
			// ��right�ҵ�leftƥ������λ��
			int histStart = 0; //��һ�����Ҳ��е��������
			for (int lx = 0; lx < cols; lx++) {//������ͼ���ص�
				//�ҵ���С����λ�õ�
				float left = lLine[lx]; //��ǰ��ͼ������ֵ
				int rStart = (std::max)(histStart - 2, 0); //����ͼ����λ����ǰƫ�Ƽ�������

				float minDist = std::abs(left - rLine[rStart]); //�����һ����λ��С����
				float historyDist = minDist; //Ϊ������Сֵ������ʷ
				int minRx = rStart; //��ͼ��С�����ƫ������
				int incCnt = 0; //������������
				for (int rx = rStart + 1; rx < cols; rx++) { //�����Ҳ�ͼ����
					float right = rLine[rx];
					float dist = std::abs(left - right);
					if (minDist > dist) {
						minDist = dist;
						minRx = rx;
					}
					if (historyDist < dist) { //������������������˳�
						incCnt++;
						if (incCnt >= 3) { //���������Ϳ��Կ���������
							disparityPtr[lx] = (float)(lx - minRx); //���������Ӳ� d
							histStart = minRx; // ������Լ�����ӵ�ǰ��С�������ؿ�ʼ��һ������
							break;
						}
					}
					else {
						incCnt = 0;
					}
					historyDist = dist;
				}
				//��ͼ��������Ե��������ͼ������
				if (cols - histStart < 10) { //��һ�������İ�ȫ����
					break;
				}
			}
		}

		//! Reproject to compute xyz
		m_pointCloud = cv::Mat(disparityImage.size(), CV_32FC3);
		cv::reprojectImageTo3D(disparityImage, m_pointCloud, m_Q);
	}

}