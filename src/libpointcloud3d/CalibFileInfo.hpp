#ifndef _CSV_CALIBRATION_FILE_INFO_HPP__
#define _CSV_CALIBRATION_FILE_INFO_HPP__

#include <string.h>
#include <vector>
#define CALIBRATION_FILE_FLAG	"CSV_HANDEYE_CALIB"
#define CALIBRATION_FILE_VER	4.0
#define SAVE_OPENCV_YML_FILE

enum  UPDATE_FLAG{
	UpdateFlagNotUpdate = 0,
	UpdateFlagWithUnknownWS = 1,
	UpdateFlagWithTeachPos = 2,
	UpdateFlagWithNewHandEye = 3,
	UpdateFlagNum
};


#define CALIB_FILE_INTRIN_PARAM_NUM		10
#define CALIB_FILE_EXTRIN_PARAM_NUM		12

enum CALIB_INTRINSIC_VERSION {
	eIntrisicVersionMin = 0,
	eIntrinsicVersionCsv = 1,
	eIntrisicVersionOpencv = 2,
	eIntrisicVersionCsv2Opencv = 3,
	eIntrisicVersionOpencv2Csv = 4,
	eIntrisicVersionMax
};


//#pragma warning(push)
//#pragma warning (disable : 4251)


namespace CSV {
	class Mat33;
	class Point3d;
}

namespace CSV{
	class CalibFileInfo{
	public:
		class CameraCalibParam{
		public:
			int ID;
			CALIB_INTRINSIC_VERSION intrinsicVersion;
			double inparam[CALIB_FILE_INTRIN_PARAM_NUM];
			double exparam[12];
			CameraCalibParam(){
				ID = -1;
				intrinsicVersion = eIntrinsicVersionCsv;
				memset(inparam, 0, sizeof(inparam));
				memset(exparam, 0, sizeof(exparam));
				exparam[0] = 1.0;  exparam[1] = 0.0;  exparam[2] = 0.0;
				exparam[3] = 0.0;  exparam[4] = 1.0;  exparam[5] = 0.0;
				exparam[6] = 0.0;  exparam[7] = 0.0;  exparam[8] = 1.0;
				exparam[9] = exparam[10] = exparam[11] = 0.0;
			}
		};
		
		class LaserCalibParam{
		public:
			int laserid;
			int camid;
			double plane_param[4];//ax + by + cz + t = 0;
			double exparam[12];//from camera to laser coordinate system
			double moveMMPF[3];//move distance mm per frame
			LaserCalibParam(){
				laserid = 0;
				camid = 0;
				plane_param[0] = plane_param[1] = plane_param[2] = plane_param[3] = 0.0;
				
				exparam[0] = 1.0;  exparam[1] = 0.0;  exparam[2] = 0.0;
				exparam[3] = 0.0;  exparam[4] = 1.0;  exparam[5] = 0.0;
				exparam[6] = 0.0;  exparam[7] = 0.0;  exparam[8] = 1.0;
				exparam[9] = exparam[10] = exparam[11] = 0.0;

				moveMMPF[0] = 0;
				moveMMPF[1] = 0;
				moveMMPF[2] = 0;
			}
		};

		class CameraInfo{
		public:
			int number;
			double focus;
			int width;
			int height;
			double rms;
			std::vector<CameraCalibParam> calib_param;

			CameraInfo(){init();}
			void init(){
				calib_param.clear();
				number = 0;
				focus = 0;
				width = 0;
				height = 0;
				rms = 0.0;
			}
		};

		class ProjectorInfo{
		public:
			int number;
			int width;
			int height;
			double rms;
			std::vector<CameraCalibParam> calib_param;

			ProjectorInfo(){init();}
			void init(){
				calib_param.clear();
				number = 0;
				width = 0;
				height = 0;
				rms = 0.0;
			}
		};

		class LaserInfo{
		public:
			int number;
			double rms;
			std::vector<LaserCalibParam>  laserparam;
			LaserInfo(){init();}
			void init(){
				laserparam.clear();
				number = 0;
				rms = 0.0;
			}
		};

		class RobotInfo{
		public:
			std::string type;
			double rms;
			RobotInfo(){ init(); }
			void init(){
				type = "UnKnown";
				rms = 0.0;
			}
		};

	public:
		std::string file_flag;
		std::string stamp;
		double version;
		int calib_type;
		UPDATE_FLAG  update_flag;
		double board_length[4];

		double move_center[3];//x,y,z
		double move_range[3];//dx,dy,dz

		CameraInfo cam_info;
		ProjectorInfo prj_info;
		LaserInfo laser_info;
		RobotInfo robot_info;

		CalibFileInfo() {
			init();
		};

		void init(){
			file_flag = CALIBRATION_FILE_FLAG;
			calib_type = 0;
			update_flag = UpdateFlagNotUpdate;
			version = CALIBRATION_FILE_VER;
			memset(board_length, 0, sizeof(board_length));
			move_center[0] = move_center[1] = move_center[2] = 0;
			move_range[0] = move_range[1] = move_range[2] = 0;

			cam_info.init();
			prj_info.init();
			laser_info.init();
			robot_info.init();
		}

		bool save(const char *filename)const;
		bool load(const char *filename);
		bool isLegal()const;
	private:
		bool loadV4_0(const char *filename);//version 4.0
	};
}

#pragma warning(pop)
#endif