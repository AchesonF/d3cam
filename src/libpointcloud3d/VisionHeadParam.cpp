#include "VisionHeadParam.h"
#include "CalibFileInfo.hpp"
#include "CameraParam.h"

namespace CSV
{
	VisionHeadParam::VisionHeadParam()
	{

	}

	int VisionHeadParam::load(std::string calib_file_path)
	{
		// 从标定文件导入标定参数
		CalibFileInfo calbinfo;
		std::string calib_path = calib_file_path;
		calbinfo.load(calib_path.c_str());

		// 常规参数设置
		memcpy(board_length, calbinfo.board_length, sizeof(double) * 4);
		file_flag = calbinfo.file_flag;
		stamp = calbinfo.stamp;
		version = calbinfo.version;
		calib_type = calbinfo.calib_type;
		update_flag = calbinfo.update_flag;
		memcpy(move_center, calbinfo.move_center, sizeof(double) * 3);
		memcpy(move_range, calbinfo.move_range, sizeof(double) * 3);

		// 设置相机参数
		int came_num = calbinfo.cam_info.number;
		for (int i = 0; i < came_num; i++)
		{
			CameraParam camparam;
			IntrinsicParam inparam;
			ExtrinsicParam exparam;
			camparam.inparam.setParam(calbinfo.cam_info.calib_param[i].inparam);
			camparam.inparam.setInfo(7.308, 5.580,
				calbinfo.cam_info.width, calbinfo.cam_info.height,
				calbinfo.cam_info.focus, calbinfo.cam_info.rms);
			camparam.exparam.set(calbinfo.cam_info.calib_param[i].exparam);

			camparam_list.push_back(camparam);
		}
		// 设置投影仪参数
		int prj_num = calbinfo.prj_info.number;
		for (int i = 0; i < prj_num; i++)
		{
			CameraParam prjparam;
			IntrinsicParam inparam;
			ExtrinsicParam exparam;
			prjparam.inparam.setParam(calbinfo.prj_info.calib_param[i].inparam);
			prjparam.inparam.setInfo(7.308, 5.580,
				calbinfo.prj_info.width, calbinfo.prj_info.height,
				0, calbinfo.prj_info.rms);
			prjparam.exparam.set(calbinfo.prj_info.calib_param[i].exparam);

			prjparam_list.push_back(prjparam);
		}

		return 0;
	}

}