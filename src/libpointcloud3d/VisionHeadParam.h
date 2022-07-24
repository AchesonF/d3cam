#ifndef _CSV_VISION_HEAD_PARAM_H
#define _CSV_VISION_HEAD_PARAM_H


#include <iostream>
#include <vector>
#include "CameraParam.h"

namespace CSV
{
	class CameraParam;

	class VisionHeadParam
	{
	public:
		VisionHeadParam();

		int load(std::string calib_file_path = "calib.xml");

	public:
		std::vector<CameraParam> camparam_list;
		std::vector<CameraParam> prjparam_list;

		std::string file_flag;
		std::string stamp;
		double version;
		int calib_type;
		int  update_flag;
		double board_length[4];

		double move_center[3];//x,y,z
		double move_range[3];//dx,dy,dz
	};
}

#endif // !_CSV_VISION_HEAD_PARAM_H

