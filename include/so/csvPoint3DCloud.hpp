#ifndef __CSV_POINT3D_CLOUD_HPP__
#define __CSV_POINT3D_CLOUD_HPP__

namespace CSV{
	enum  CSV_DataFormatType {
		FixPoint48bits = 0,
		FixPoint64bits = 1,
		FixZMap = 2,
		FixZMapSimple = 3
	};

	class CsvPoint3DCloud {
	public:
		CsvPoint3DCloud() {
			m_type = FixPoint48bits;
		}
	private:
		CSV_DataFormatType  m_type;

		//

	};
}

#endif
