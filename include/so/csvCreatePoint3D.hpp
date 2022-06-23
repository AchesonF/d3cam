#ifndef __CSV_CREATE_POINT3D_HPP__
#define __CSV_CREATE_POINT3D_HPP__
#include "csvPoint3DCloud.hpp"

namespace CSV {
	//Jetson 框架与算法so交互接口
	class CsvCreatePoint3DParam
	{
	public:
		std::string calibXml; //相机标定文件
		CSV_DataFormatType type; //出点云类型

		CsvCreatePoint3DParam() {
			calibXml = "";
			type = FixPoint64bits;
		}
	};


	class CsvImageSimple {
	public:
		CsvImageSimple() {
			m_data = NULL;
			m_height = m_width = m_widthStep = 0;
			m_channel = 0;
		}

		inline unsigned char *ptr(int x, int y) { return m_data + y * m_widthStep + x * m_channel; }
		inline unsigned char &at(int x, int y) { return m_data[y * m_widthStep + x * m_channel]; }
	public:
		unsigned char *m_data;
		unsigned int  m_height;
		unsigned int  m_width;
		unsigned int  m_channel;
		unsigned int  m_widthStep;
	};



	bool csvSetCreatePoint3DParam(const CsvCreatePoint3DParam &param); //设置标定文件（开机设置一次）
	bool csvGetCreatePoint3DParam(CsvCreatePoint3DParam &param);
	bool csvCreatePoint3D(
		const std::vector<std::vector<CsvImageSimple>>& inImages,
		CsvPoint3DCloud &pointCloud
		);

};
#endif
