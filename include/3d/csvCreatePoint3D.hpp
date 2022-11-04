#ifndef __CSV_CREATE_POINT3D_HPP__
#define __CSV_CREATE_POINT3D_HPP__

#include <string>
#include <vector>
#include "csvPoint3DCloud.hpp"
#include "CsvJetsonErrorType.hpp"

namespace CSV {
	//interface between Jetson and 3D measure
	class CsvCreatePoint3DParam
	{
	public:
		std::string calibXml; // calibration file (xml) of cameras
		std::string modelPathFolder; // path folder of model
		CSV_DataFormatType type; //type of output point cloud

		CsvCreatePoint3DParam() {
			calibXml = "";
			type = FixPoint64bits;
		}
	};

	enum DepthType {
		ImageDepthType8bits = 8,
		ImageDepthType16bits = 16
	};

	class CsvImageSimple {
	public:
		CsvImageSimple() {}

		CsvImageSimple(unsigned int rows, unsigned int cols, unsigned int channel, unsigned char* data) {
			m_height = rows;
			m_width = cols;
			m_channel = channel;
			m_widthStep = m_width * channel * sizeof(unsigned char);
			m_data = (void*)data; // std::vector<unsigned char>(data, data + m_height * m_widthStep); //deep copy into MYSELF space !!
		}

		~CsvImageSimple() {	}

		//inline unsigned char *ptr(int x, int y) { return m_data.data() + y * m_widthStep + x * m_channel; }
		//inline unsigned char &at(int x, int y) { return m_data.data()[y * m_widthStep + x * m_channel]; }

	public:
		void *m_data;
		unsigned int  m_height;
		unsigned int  m_width;
		unsigned int  m_channel;
		unsigned int  m_widthStep; // number of bytes in a line
		DepthType m_depth;
	};
	
	std::string csvGetCreatePoint3DALgVersion();
	int CsvSetCreatePoint3DParam(const CsvCreatePoint3DParam &param); //setting only one time after power on
	int CsvGetCreatePoint3DParam(CsvCreatePoint3DParam &param);
	int CsvCreateLUT(const CsvCreatePoint3DParam &param);
	int CsvCreatePoint3D(const std::vector<std::vector<CsvImageSimple>>& inImages, CsvImageSimple &depthImage,	std::vector<float> *point3D = nullptr);

	bool ParseDepthImage2PointCloud(CsvImageSimple &depthImage, std::vector<float> *point3D);
};
#endif
