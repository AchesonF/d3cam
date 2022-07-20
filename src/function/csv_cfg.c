#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif




int csv_cfg_init (void)
{
	struct csv_cfg_t *pCFG = &gCSV->cfg;
	struct gev_param_t *pGP = &pCFG->gp;

	pCFG->device_param.device_type = 0;
	pCFG->device_param.camera_leftright_type = false;
	pCFG->device_param.camera_img_type = true;
	pCFG->device_param.exposure_time = 40000.0f;
	pCFG->device_param.exposure_time_for_rgb = 10000.0f;
	pCFG->device_param.dlp_rate = 0x002D;
	pCFG->device_param.dlp_brightness = 700;
	pCFG->device_param.img_type = SUFFIX_BMP;

	pCFG->depthimg_param.numDisparities = 816;
	pCFG->depthimg_param.blockSize = 21;
	pCFG->depthimg_param.uniqRatio = 9;

	strcpy(pCFG->pointcloud_param.imgRoot, "data/PointCloudImage");
	strcpy(pCFG->pointcloud_param.imgPrefixNameL, "CSV_001C1S00P");
	strcpy(pCFG->pointcloud_param.imgPrefixNameR, "CSV_001C2S00P");
	strcpy(pCFG->pointcloud_param.outFileXYZ, "pointcloud.xyz");
	pCFG->pointcloud_param.enable = 0;

	pCFG->calib_param.groupDemarcate = 1;
	strcpy(pCFG->calib_param.path, "data/calibImage");
	strcpy(pCFG->calib_param.calibFile, "grayCodeParams.xml");

	pGP->Version = (GEV_VERSION_MAJOR<<16) | GEV_VERSION_MINOR;
	pGP->DeviceMode = GEV_DEVICE_MODE;
	pGP->IfCapability0 = 7;
	pGP->IfConfiguration0 = 6;
	strcpy(pGP->ManufacturerName, "CSVision");
	strcpy(pGP->ModelName, "CS-3D001-28HS");
	strcpy(pGP->DeviceVersion, SOFTWARE_VERSION);
	strcpy(pGP->ManufacturerInfo, "CS vision 3d infomation");
	strcpy(pGP->SerialNumber, "CS300128001"); // todo update

	return 0;
}



#ifdef __cplusplus
}
#endif



