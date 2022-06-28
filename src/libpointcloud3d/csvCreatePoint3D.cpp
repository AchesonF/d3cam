//#include "CsvVersion.h"
#include "csvCreatePoint3D.hpp"

namespace CSV {
/*
	std::string csvGetCreatePoint3DALgVersion() {
		CsvVersion version;
		return version.getVersion();
	}
*/
	//setting only one time after power on
	bool csvSetCreatePoint3DParam(const CsvCreatePoint3DParam &param) {
		return true;
	}

	bool csvGetCreatePoint3DParam(CsvCreatePoint3DParam &param) {
		return true;
	}

	bool csvCreatePoint3D(const std::vector<std::vector<CsvImageSimple>>& inImages, CsvPoint3DCloud &pointCloud) {
		return true;
	}
}
