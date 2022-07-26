#include<time.h> 
#include <string>
#include <vector>
#include <iostream>
#include "tinyxml.h"
#include "CalibFileInfo.hpp"
#include "CsvType.hpp"


#define CALIB_FILE_MAX_CAMERA_NUM		16
#define CALIB_FILE_MAX_PROJECTOR_NUM	4
#define CALIB_FILE_MIN_CAMERA_IMG_SIZE	32
#define CALIB_FILE_MAX_CAMERA_IMG_SIZE	10000

#ifdef __linux__
#include <cstdio>
#include <cstdarg>
void sprintf_s(char* buf, int buf_size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, buf_size, fmt, args);
    va_end(args);
}
#endif

static std::vector<double> splitStringToDouble(const std::string &str){
	std::vector<double> data;
	int startIndex = 0;
	while (1)
	{
		const int index = (int)str.find_first_of(',',startIndex);
		if (index >= 0 && index <= startIndex){ startIndex = index + 1; continue; }

		if (index < 0){//no ',' found since startIndex
			std::string subStr = str.substr(startIndex);
			if (subStr.empty()) break;
			
			data.push_back(atof(subStr.data()));
			break;
		}

		std::string subStr = str.substr(startIndex, index - startIndex);
		startIndex = index + 1;
		data.push_back(atof(subStr.data()));
	}

	return data;
}


namespace CSV {
	bool CalibFileInfo::loadV4_0(const char *filename)
	{
		TiXmlDocument xmlDocument;
		if (nullptr == filename) return false;

		if (!xmlDocument.LoadFile(filename, TIXML_ENCODING_UTF8)) {
			std::cout << "Can't load the xml file " << filename;
			return false;
		}
		std::string readString;
		std::vector<double> data;

		init();

		TiXmlElement* xmlnodeCalibration = xmlDocument.FirstChildElement("calibration");
		if (NULL == xmlnodeCalibration) return false;

		file_flag = xmlnodeCalibration->Attribute("fileFlag");
		const char *value = xmlnodeCalibration->Attribute("version");
		stamp = xmlnodeCalibration->Attribute("time");
		version = atof(value);

		bool loadResult = true;
		do {
			TiXmlElement* xmlCalibSubNode = NULL;

			if ((xmlCalibSubNode = xmlnodeCalibration->FirstChildElement("calibrationBoardSize")) == NULL) { loadResult = false; break; }
			board_length[0] = board_length[1] = board_length[2] = board_length[3] = atof(xmlCalibSubNode->GetText());
			if ((xmlCalibSubNode = xmlnodeCalibration->FirstChildElement("calibType")) == NULL) { loadResult = false; break; }
			calib_type = atoi(xmlCalibSubNode->GetText());

			if ((xmlCalibSubNode = xmlnodeCalibration->FirstChildElement("updateFlag")) != NULL) {
				int flag = atoi(xmlCalibSubNode->GetText());
				if (flag < 0 || flag >= UpdateFlagNum) flag = UpdateFlagNotUpdate;
				update_flag = (UPDATE_FLAG)flag;
			}

			if ((xmlCalibSubNode = xmlnodeCalibration->FirstChildElement("moveRange")) == NULL) { loadResult = false; break; }
			readString = xmlCalibSubNode->GetText();
			if ((data = splitStringToDouble(readString)).size() != 6) { loadResult = false; break; }
			move_center[0] = data[0]; move_center[1] = data[1]; move_center[2] = data[2];
			move_range[0] = data[3]; move_range[1] = data[4]; move_range[2] = data[5];

			TiXmlElement * xmlCamera = xmlnodeCalibration->FirstChildElement("camera");
			if (NULL == xmlCamera) { loadResult = false; break; }
			cam_info.number = atoi(xmlCamera->Attribute("cameraNumber"));
			cam_info.rms = atof(xmlCamera->Attribute("cameraRMS"));

			TiXmlElement *xmlFocalItem = xmlCamera->FirstChildElement("focalLength");
			if (NULL == xmlFocalItem) { loadResult = false; break; }
			cam_info.focus = atof(xmlFocalItem->GetText());

			TiXmlElement *xmlCameraRes = xmlCamera->FirstChildElement("cameraResolution");
			if (NULL == xmlCameraRes) { loadResult = false; break; }
			if ((data = splitStringToDouble(xmlCameraRes->GetText())).size() != 2) { loadResult = false; break; }
			cam_info.width = (int)(data[0] + 0.1);
			cam_info.height = (int)(data[1] + 0.1);


			for (int c = 0; c < cam_info.number; ++c)
			{
				char cameraName[128];
				sprintf_s(cameraName, sizeof(cameraName), "camera-%d", c + 1);
				TiXmlElement * xmlCameraItem = xmlCamera->FirstChildElement(cameraName);
				if (NULL == xmlCameraItem) { loadResult = false; break; }

				CameraCalibParam cameraCalibParam;
				TiXmlElement * xmlCameraItemI = NULL;

				if ((xmlCameraItemI = xmlCameraItem->FirstChildElement("cameraID")) == NULL) { loadResult = false; break; }
				cameraCalibParam.ID = atoi(xmlCameraItemI->GetText());

				if ((xmlCameraItemI = xmlCameraItem->FirstChildElement("intrinVersion")) != NULL) {
					int intrinsicVersion = atoi(xmlCameraItemI->GetText());
					if (intrinsicVersion <= eIntrisicVersionMin || intrinsicVersion >= eIntrisicVersionMax)
					{
						std::cout << "Illegal input calib intrinsic version " << intrinsicVersion << ",force it to 1" << std::endl;
						intrinsicVersion = eIntrinsicVersionCsv;
					}
					cameraCalibParam.intrinsicVersion = (CALIB_INTRINSIC_VERSION)intrinsicVersion;
				}

				if ((xmlCameraItemI = xmlCameraItem->FirstChildElement("intrinsicParam")) == NULL) { loadResult = false; break; }
				readString = xmlCameraItemI->GetText();
				if ((data = splitStringToDouble(readString)).size() != CALIB_FILE_INTRIN_PARAM_NUM) { loadResult = false; break; }
				for (int i = 0; i < CALIB_FILE_INTRIN_PARAM_NUM; ++i) cameraCalibParam.inparam[i] = data[i];
				for (int i = 0; i < 4; ++i) {//check the fx,fy,u0,v0
					if (cameraCalibParam.inparam[i] < 0) { loadResult = false; break; }
				}

				if ((xmlCameraItemI = xmlCameraItem->FirstChildElement("extrinsicParam")) == NULL) { loadResult = false; break; }
				for (int i = 0; i < 3; ++i) {
					char rowName[128];
					sprintf_s(rowName, sizeof(rowName), "row%d", i);
					if ((data = splitStringToDouble(xmlCameraItemI->FirstChildElement(rowName)->GetText())).size() != CALIB_FILE_EXTRIN_PARAM_NUM / 3) {
						loadResult = false; break;
					}

					cameraCalibParam.exparam[i * 3 + 0] = data[0];
					cameraCalibParam.exparam[i * 3 + 1] = data[1];
					cameraCalibParam.exparam[i * 3 + 2] = data[2];
					cameraCalibParam.exparam[9 + i] = data[3];
				}
				if (loadResult == false) {
					std::cout << "Illegal camera extrin param." << std::endl;
					break;
				}

				cam_info.calib_param.push_back(cameraCalibParam);
			}
			if (loadResult == false) break;

			TiXmlElement * xmlProjector = xmlnodeCalibration->FirstChildElement("projector");
			if (NULL == xmlProjector) { loadResult = false; break; }
			prj_info.number = atoi(xmlProjector->Attribute("projectorNumber"));
			prj_info.rms = atof(xmlProjector->Attribute("projectorRMS"));

			if (prj_info.number > 0) {
				TiXmlElement *xmlProjectorRes = xmlProjector->FirstChildElement("projectorResolution");
				if (NULL == xmlProjectorRes) { loadResult = false; break; }
				if ((data = splitStringToDouble(xmlProjectorRes->GetText())).size() != 2) { loadResult = false; break; }
				prj_info.width = (int)(data[0] + 0.1);
				prj_info.height = (int)(data[1] + 0.1);
			}

			for (int p = 0; p < prj_info.number; ++p)
			{
				char projectorName[128];
				sprintf_s(projectorName, sizeof(projectorName), "projector-%d", p + 1);
				TiXmlElement * xmlProjectorItem = xmlProjector->FirstChildElement(projectorName);
				if (NULL == xmlProjectorItem) { loadResult = false; break; }

				CameraCalibParam projectorCalibParam;

				TiXmlElement * xmlProjectorItemI = NULL;
				if ((xmlProjectorItemI = xmlProjectorItem->FirstChildElement("projectorID")) == NULL) { loadResult = false; break; }
				projectorCalibParam.ID = atoi(xmlProjectorItemI->GetText());

				if ((xmlProjectorItemI = xmlProjectorItem->FirstChildElement("intrinVersion")) != NULL) {
					int intrinsicVersion = atoi(xmlProjectorItemI->GetText());
					if (intrinsicVersion <= eIntrisicVersionMin || intrinsicVersion >= eIntrisicVersionMax)
					{
						std::cout << "Illegal input calib intrinsic version " << intrinsicVersion << ",force it to 1" << std::endl;
						intrinsicVersion = eIntrinsicVersionCsv;
					}
					projectorCalibParam.intrinsicVersion = (CALIB_INTRINSIC_VERSION)intrinsicVersion;
				}

				if ((xmlProjectorItemI = xmlProjectorItem->FirstChildElement("intrinsicParam")) == NULL) { loadResult = false; break; }
				readString = xmlProjectorItemI->GetText();
				if ((data = splitStringToDouble(readString)).size() != CALIB_FILE_INTRIN_PARAM_NUM) { loadResult = false; break; }
				for (int i = 0; i < CALIB_FILE_INTRIN_PARAM_NUM; ++i) projectorCalibParam.inparam[i] = data[i];
				for (int i = 0; i < 4; ++i) {//check fx,fy,u0,v0
					if (projectorCalibParam.inparam[i] < 0) { loadResult = false; break; }
				}
				if (false == loadResult) break;

				if ((xmlProjectorItemI = xmlProjectorItem->FirstChildElement("extrinsicParam")) == NULL) { loadResult = false; break; }
				for (int i = 0; i < 3; ++i) {
					char rowName[128];
					sprintf_s(rowName, sizeof(rowName), "row%d", i);
					readString = xmlProjectorItemI->FirstChildElement(rowName)->GetText();
					if ((data = splitStringToDouble(readString)).size() != CALIB_FILE_EXTRIN_PARAM_NUM / 3) { loadResult = false; break; }

					projectorCalibParam.exparam[i * 3 + 0] = data[0];
					projectorCalibParam.exparam[i * 3 + 1] = data[1];
					projectorCalibParam.exparam[i * 3 + 2] = data[2];
					projectorCalibParam.exparam[9 + i] = data[3];
				}
				if (false == loadResult) {
					std::cout << "Illegal projector extrin param" << std::endl;
					break;
				}
				prj_info.calib_param.push_back(projectorCalibParam);
			}

			if (false == loadResult) break;

			//Robot information
			if ((xmlCalibSubNode = xmlnodeCalibration->FirstChildElement("Robot")) == NULL) { loadResult = false; break; }
			robot_info.rms = atof(xmlCalibSubNode->Attribute("robotRMS"));
			if (robot_info.rms < 0) { loadResult = false; break; }

			TiXmlElement* xmlnodeTemp = xmlCalibSubNode->FirstChildElement("RobotType");
			if (NULL != xmlnodeTemp) {
				robot_info.type = xmlnodeTemp->GetText();
			}


			TiXmlElement* xmlnodeLaser = xmlnodeCalibration->FirstChildElement("Laser");
			if (NULL == xmlnodeLaser) { loadResult = false; break; }
			laser_info.number = atoi(xmlnodeLaser->Attribute("laserNumber"));
			laser_info.rms = atof(xmlnodeLaser->Attribute("laserRMS"));
			if (laser_info.number < 0 || laser_info.rms < 0) { loadResult = false; break; }


			for (int n = 0; n < laser_info.number; ++n) {
				char laserName[128];
				sprintf_s(laserName, sizeof(laserName), "laser-%d", n + 1);
				TiXmlElement * xmlLaserItem = xmlnodeLaser->FirstChildElement(laserName);
				if (NULL == xmlLaserItem) { loadResult = false; break; }

				LaserCalibParam laserCalibParam;

				TiXmlElement *xmlLaserItemI;
				if ((xmlLaserItemI = xmlLaserItem->FirstChildElement("laserID")) == NULL) { loadResult = false; break; }
				laserCalibParam.laserid = atoi(xmlLaserItemI->GetText());

				if ((xmlLaserItemI = xmlLaserItem->FirstChildElement("cameraID")) == NULL) { loadResult = false; break; }
				laserCalibParam.camid = atoi(xmlLaserItemI->GetText());

				if ((xmlLaserItemI = xmlLaserItem->FirstChildElement("laserPlane")) == NULL) { loadResult = false; break; }
				if ((data = splitStringToDouble(xmlLaserItemI->GetText())).size() != 4) { loadResult = false; break; }
				for (int i = 0; i < 4; ++i) {
					laserCalibParam.plane_param[i] = data[i];
				}
				if (false == loadResult) break;


				if ((xmlLaserItemI = xmlLaserItem->FirstChildElement("ExtrinFromCameraToLaser")) == NULL) { loadResult = false; break; }
				for (int i = 0; i < 3; ++i) {
					char rowName[128];
					sprintf_s(rowName, sizeof(rowName), "row%d", i);
					if ((data = splitStringToDouble(xmlLaserItemI->FirstChildElement(rowName)->GetText())).size() != CALIB_FILE_EXTRIN_PARAM_NUM / 3) {
						loadResult = false; break;
					}

					laserCalibParam.exparam[i * 3 + 0] = data[0];
					laserCalibParam.exparam[i * 3 + 1] = data[1];
					laserCalibParam.exparam[i * 3 + 2] = data[2];
					laserCalibParam.exparam[9 + i] = data[3];
				}

				//move speed: mm per frame
				if ((xmlLaserItemI = xmlLaserItem->FirstChildElement("MoveMMPF")) != NULL) {
					double mx, my, mz;
					if (3 == sscanf(xmlLaserItemI->GetText(), "%lf,%lf,%lf", &mx, &my, &mz)) {
						laserCalibParam.moveMMPF[0] = mx;
						laserCalibParam.moveMMPF[1] = my;
						laserCalibParam.moveMMPF[2] = mz;
					}

				}

				laser_info.laserparam.push_back(laserCalibParam);
			}
			if (false == loadResult) break;

		} while (false);

		if (loadResult) return true;
		else {
			init();
			return false;
		}
	}

	bool CalibFileInfo::load(const char *filename) {
		TiXmlDocument xmlDocument;
		if (nullptr == filename) return false;

		if (!xmlDocument.LoadFile(filename, TIXML_ENCODING_UTF8)) {
			std::cout << "Can't load the xml file " << filename << std::endl;
			return false;
		}
		std::string readString;
		std::vector<double> data;

		init();

		TiXmlElement* xmlnodeCalibration = xmlDocument.FirstChildElement("calibration");
		if (NULL == xmlnodeCalibration) return false;

		file_flag = xmlnodeCalibration->Attribute("fileFlag");
		if (CALIBRATION_FILE_FLAG != file_flag) {
			std::cout << "calibration flag should be " << CALIBRATION_FILE_FLAG << " instead of " << file_flag << std::endl;
			return false;
		}

		const char *value = xmlnodeCalibration->Attribute("version");
		version = atof(value);

		if (std::abs(version - 4.0) < 0.0001) return loadV4_0(filename);//version 4.0

		return false;
	}

	bool CalibFileInfo::isLegal()const {

		for (int i = 0; i < 4; ++i) {
			if (board_length[i] < 10.0 || board_length[i] > 100000) {//mm
				std::cout << "Illegal board_length[" << i << "] = " << board_length[i] << std::endl;
				return false;
			}
		}

		for (int i = 0; i < 3; ++i) {
			if (move_range[i] < 10.0) {//mm
				std::cout << "Illegal move_range[" << i << "] = " << move_range[i] << std::endl;
				return false;
			}
		}

		const int cameraNum = this->cam_info.number;
		if (cameraNum<0 || cameraNum > CALIB_FILE_MAX_CAMERA_NUM) {
			std::cout << "Camera number should be between 0 and " << CALIB_FILE_MAX_CAMERA_NUM << std::endl;
			return false;
		}

		if (cam_info.width <CALIB_FILE_MIN_CAMERA_IMG_SIZE || cam_info.width > CALIB_FILE_MAX_CAMERA_IMG_SIZE ||
			cam_info.height <CALIB_FILE_MIN_CAMERA_IMG_SIZE || cam_info.height > CALIB_FILE_MAX_CAMERA_IMG_SIZE) {
			std::cout << "Illegal camera image size:width = " << cam_info.width << ",height = " << cam_info.height << std::endl;
			return false;
		}

		if (cam_info.rms<0 || cam_info.rms>1000.0) {
			std::cout << "Illegal camera rms = " << cam_info.rms;
			return false;
		}


		for (int c = 0; c < cameraNum; ++c) {
			const double fx = cam_info.calib_param[c].inparam[0];
			const double fy = cam_info.calib_param[c].inparam[1];
			const double u0 = cam_info.calib_param[c].inparam[2];
			const double v0 = cam_info.calib_param[c].inparam[3];
			if (fx < 0 || fy < 0 || u0<0 || u0 > cam_info.width || v0 < 0 || v0 > cam_info.height) {
				std::cout << "Illegal camera intrinsic param [fx,fy,u0,v0] = [" << fx << "," << fy << "," << u0 << "," << v0 << "]" << std::endl;
				return false;
			}

			double x[3], y[3], z[3];//, trans[3];
			x[0] = cam_info.calib_param[c].exparam[0];
			y[0] = cam_info.calib_param[c].exparam[1];
			z[0] = cam_info.calib_param[c].exparam[2];
			x[1] = cam_info.calib_param[c].exparam[3];
			y[1] = cam_info.calib_param[c].exparam[4];
			z[1] = cam_info.calib_param[c].exparam[5];
			x[2] = cam_info.calib_param[c].exparam[6];
			y[2] = cam_info.calib_param[c].exparam[7];
			z[2] = cam_info.calib_param[c].exparam[8];
			//trans[0] = cam_info.calib_param[c].exparam[9];
			//trans[1] = cam_info.calib_param[c].exparam[10];
			//trans[2] = cam_info.calib_param[c].exparam[11];
			//unit vector
			if (std::abs(x[0] * x[0] + x[1] * x[1] + x[2] * x[2] - 1.0) > 0.01 ||
				std::abs(y[0] * y[0] + y[1] * y[1] + y[2] * y[2] - 1.0) > 0.01 ||
				std::abs(z[0] * z[0] + z[1] * z[1] + z[2] * z[2] - 1.0) > 0.01) {
				std::cout << "Illegal camera extrinsic param,it's should be unit vector" << std::endl;
				return false;
			}
			//coordinate vector is orthogonal to each other
			if (std::abs(x[0] * y[0] + x[1] * y[1] + x[2] * y[2]) > 0.01 ||
				std::abs(y[0] * z[0] + y[1] * z[1] + y[2] * z[2]) > 0.01 ||
				std::abs(x[0] * z[0] + x[1] * z[1] + x[2] * z[2]) > 0.01) {
				std::cout << "Illegal camera extrinsic param,coordinate vector is orthogonal to each other" << std::endl;
				return false;
			}

			double xyCrossProd[3];
			xyCrossProd[0] = x[1] * y[2] - x[2] * y[1];
			xyCrossProd[1] = x[2] * y[0] - x[0] * y[2];
			xyCrossProd[2] = x[0] * y[1] - x[1] * y[0];
			const double diff[3] = { xyCrossProd[0] - z[0], xyCrossProd[1] - z[1], xyCrossProd[2] - z[2] };
			if (std::abs(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]) > 0.01) {
				std::cout << "Illegal camera extrinsic param,coordinate vector is illegal" << std::endl;
				return false;
			}
		}

		return true;
	}

}

