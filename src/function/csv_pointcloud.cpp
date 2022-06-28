#include <iostream>

#if (USING_POINTCLOUD3D==1)

#include "csvCreatePoint3D.hpp"

#ifdef __cplusplus
extern "C" {
#endif

int testPoint3DCloud (void)
{
	CSV::CsvCreatePoint3DParam param;
	param.calibXml = std::string("./CalibInv.xml");
	param.type = CSV::CSV_DataFormatType::FixPoint64bits;

	//CSV::CsvCreatePoint3DParam param0;
	csvGetCreatePoint3DParam(param);
	std::cout << param.calibXml << std::endl;
	std::cout << param.type << std::endl;

	std::vector<std::vector<CSV::CsvImageSimple>> imageGroups;
	CSV::CsvPoint3DCloud pointCloud;

	bool result = csvCreatePoint3D(imageGroups, pointCloud);
	std::cout << result << std::endl;

	return 0;
}



#ifdef __cplusplus
}
#endif

#endif