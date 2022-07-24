#include "CsvVersion.h"
#include "csvCreatePoint3D.hpp"
#include "CsvMeasure3D.hpp"

namespace CSV {

	typedef std::shared_ptr<CsvMeasure3D>  measure3DPtr;

	class SingleObject {
	public:
		static measure3DPtr getInstance() {
			if (nullptr == m_instance) {
				//std::lock_guard<std::mutex> lk(m_mutex);
				if (nullptr == m_instance) {
					m_instance = measure3DPtr(new CsvMeasure3D());
				}
			}
			return m_instance;
		}

	private:
		static measure3DPtr m_instance;
		//static std::mutex m_mutex;
		SingleObject() {}
	};
	measure3DPtr SingleObject::m_instance = nullptr;

	static measure3DPtr  g_classifyEngine = nullptr;
	static CsvCreatePoint3DParam g_param;

	std::string csvGetCreatePoint3DALgVersion() {
		CsvVersion version;
		return version.getVersion();
	}

	//setting only one time after power on
	bool CsvSetCreatePoint3DParam(const CsvCreatePoint3DParam &param) {
		std::string version = csvGetCreatePoint3DALgVersion();
		std::cout << version << std::endl;

		g_classifyEngine = SingleObject::getInstance();
		g_classifyEngine->loadModel(param.calibXml);
		g_param = param;
		return true;
	}

	bool CsvGetCreatePoint3DParam(CsvCreatePoint3DParam &param) {
		param = g_param;
		return true;
	}

	bool csvCreatePoint3D(const std::vector<std::vector<CsvImageSimple>>& inImages, CsvPoint3DCloud &pointCloud) {
		std::vector<std::vector<cv::Mat>> imageGroup;

		int rows = inImages[0][0].m_height, cols = inImages[0][0].m_width;
		for (int g = 0; g < inImages.size(); g++) {
			std::vector<cv::Mat> imgs;
			for (int i = 0; i < inImages[g].size(); i++) {
				const CsvImageSimple* m = &(inImages[g][i]);
				void* data = static_cast<void*>(m->m_data);
				cv::Mat img(rows, cols, CV_8UC1, data, m->m_widthStep);
				imgs.emplace_back(img);
			}
			imageGroup.emplace_back(imgs);
		}
		int rst = g_classifyEngine->pullStripeImages(imageGroup); //fringe images : (2+7+4)*2=26
		if (rst != 0) {
			return false;
		}

		cv::Mat cvPointCloud;
		g_classifyEngine->pushPointCloud(cvPointCloud); //push out point cloud

		//size_t Mat::total() - return number of elements
		cv::Mat flat = cvPointCloud.reshape(1, cvPointCloud.total() * cvPointCloud.channels());
		pointCloud.m_point3DData = cvPointCloud.isContinuous() ? flat : flat.clone(); 
		pointCloud.m_type = g_param.type;

		// change type of point cloud
		if (g_param.type == CSV_DataFormatType::FixPoint48bits) {

		}
		else if (g_param.type == CSV_DataFormatType::FixPoint64bits) {

		}
		else if (g_param.type == CSV_DataFormatType::FixZMap) {

		}
		else if (g_param.type == CSV_DataFormatType::FixZMapSimple) {

		}
		else {
			return false;
		}

		return true;
	}
}
