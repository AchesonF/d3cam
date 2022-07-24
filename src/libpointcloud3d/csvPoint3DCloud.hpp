#ifndef __CSV_POINT3D_CLOUD_HPP__
#define __CSV_POINT3D_CLOUD_HPP__

#include <vector>
#include <limits>
#include <cstring>

namespace CSV{
	enum  CSV_DataFormatType {
		FixPoint48bits = 0,
		FixPoint64bits = 1,
		FixZMap = 2,
		FixZMapSimple = 3
	};
	enum Point3DFrame {
		Point3DFrameCamera1 = 0,
		Point3DFrameCamera2 = 1,
		Point3DFrameProjector1 = 2,
		Point3DFrameProjector2 = 3
	};

	class CsvPoint3DCloud {
	public:
		CsvPoint3DCloud() {
			m_type = FixPoint48bits;
			m_pointFrame = Point3DFrameCamera1;
			m_point3DData.clear();
		}

		void transformToBytes(std::vector<unsigned char> &data)const {
			int allBytes = 4;//sizeof(allBytes)
			allBytes += 16;//m_type,m_pointFrame,

			allBytes += 3 * 4;//sizeof(m_minPoint3D)
			allBytes += 4;//sizeof(m_point3DNum)

			const int point3DNum = m_point3DData.size() / 3;
			if (FixPoint48bits == m_type)  allBytes += point3DNum * 6;
			else if (FixPoint64bits == m_type)  allBytes += point3DNum * 8;
			else { data.clear(); return; }

			data.resize(allBytes);

			float minPoint3D[3] = { 0,0,0 };
			const float *pntPtr = &m_point3DData[0];
			minPoint3D[0] = minPoint3D[1] = minPoint3D[2] = std::numeric_limits<double>::max();
			for (int n = 0; n < point3DNum; ++n, pntPtr += 3) {
				if (pntPtr[0] < minPoint3D[0]) minPoint3D[0] = pntPtr[0];
				if (pntPtr[1] < minPoint3D[1]) minPoint3D[1] = pntPtr[1];
				if (pntPtr[2] < minPoint3D[2]) minPoint3D[2] = pntPtr[2];
			}


			int index = 0, length = 0;

			length = 4;  std::memcpy(&data[index], &m_type, length); index += length;

			unsigned char tempBufer[16] = { m_type,m_pointFrame,0 };
			length = 16;  std::memcpy(&data[index], &tempBufer[0], length); index += length;
			length = 12; std::memcpy(&data[index], &minPoint3D[0], length); index += length;
			length = 4;  std::memcpy(&data[index], &point3DNum, length); index += length;


			if (FixPoint48bits == m_type) {
				const float *pntPtr = &m_point3DData[0];
				unsigned short  pnt3D[3];
				for (int n = 0; n < point3DNum; ++n, pntPtr += 3) {
					pnt3D[0] = (unsigned short)((pntPtr[0] - minPoint3D[0]) * 10 + 0.5);
					pnt3D[1] = (unsigned short)((pntPtr[1] - minPoint3D[1]) * 10 + 0.5);
					pnt3D[2] = (unsigned short)((pntPtr[2] - minPoint3D[2]) * 10 + 0.5);

					length = 6;
					std::memcpy(&data[index], &pnt3D[0], length);
					index += length;
				}
			}
			else if (FixPoint64bits == m_type) {
				const float *pntPtr = &m_point3DData[0];
				for (int n = 0; n < point3DNum; ++n, pntPtr += 3) {
					unsigned long long  data64bits =
						((0XFFFFF & (unsigned int)((pntPtr[0] - minPoint3D[0]) * 100 + 0.5)) << 0) |
						((0XFFFFF & (unsigned int)((pntPtr[1] - minPoint3D[1]) * 100 + 0.5)) << 20) |
						((0XFFFFF & (unsigned long long)((pntPtr[2] - minPoint3D[2]) * 100 + 0.5)) << 40);

					length = sizeof(data64bits);
					std::memcpy(&data[index], &data64bits, length);
					index += length;
				}

			}
			else {


			}
		}

		bool transformFromBytes(const std::vector<unsigned char> &data) {
			int allBytes = 4;//sizeof(allBytes)
			allBytes += 16;//m_fixType,m_pointFrame
			allBytes += 3 * 4;//sizeof(m_minPoint3D)
			allBytes += 4;//sizeof(point3DNum)


			if (data.size() < allBytes) return false;
			int index = 0, length = 0;

			length = 4;  std::memcpy(&allBytes, &data[index], length); index += length;
			if (allBytes != data.size()) return false;

			unsigned char tempBufer[16];
			length = 16;  std::memcpy(&tempBufer[0], &data[index], length); index += length;

			m_type = static_cast<CSV_DataFormatType>(tempBufer[0]);
			m_pointFrame = static_cast<Point3DFrame>(tempBufer[1]);
			if (FixPoint48bits != m_type && FixPoint64bits != m_type) return false;

			int point3DNum = 0;
			float minPoint3D[3] = { 0,0,0 };

			length = 12; std::memcpy(&minPoint3D[0], &data[index], length); index += length;
			length = 4;  std::memcpy(&point3DNum, &data[index], length); index += length;
			if (point3DNum < 0 || point3DNum > 10000000) return false;
			m_point3DData.resize(3 * point3DNum);

			if ((data.size() - index) % 6 != 0 && (data.size() - index) % 8 != 0) return false;


			if (FixPoint48bits == m_type) {
				float *pntPtr = &m_point3DData[0];
				unsigned short  pnt3D[3];
				for (int n = 0; n < point3DNum; ++n, pntPtr += 3) {
					length = 6;	std::memcpy(&pnt3D[0], &data[index], length); index += length;

					pntPtr[0] = pnt3D[0] * 0.1 + minPoint3D[0];
					pntPtr[1] = pnt3D[1] * 0.1 + minPoint3D[1];
					pntPtr[2] = pnt3D[2] * 0.1 + minPoint3D[2];
				}
			}
			else if (FixPoint64bits == m_type) {
				float *pntPtr = &m_point3DData[0];
				for (int n = 0; n < point3DNum; ++n, pntPtr += 3) {
					unsigned long long  data64bits = 0;

					length = sizeof(data64bits); std::memcpy(&data64bits, &data[index], length); index += length;
					pntPtr[0] = ((data64bits >> 0) & 0XFFFFF)*0.01 + minPoint3D[0];
					pntPtr[1] = ((data64bits >> 20) & 0XFFFFF)*0.01 + minPoint3D[1];
					pntPtr[2] = ((data64bits >> 40) & 0XFFFFF)*0.01 + minPoint3D[2];
				}

			}
			else {

			}

		}

	//private:
	public:
		CSV_DataFormatType  m_type;
		Point3DFrame m_pointFrame;
		std::vector<float> m_point3DData;//point3DNum*3

	};
}

#endif
