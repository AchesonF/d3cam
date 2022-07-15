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
	pCFG->device_param.exposure_time = 10000.0f;
	pCFG->device_param.exposure_time_for_rgb = 10000.0f;
	pCFG->device_param.dlp_rate = 0x002D;
	pCFG->device_param.dlp_brightness = 0x03FF;
	pCFG->device_param.img_type = SUFFIX_BMP;

	pCFG->depthimg_param.numDisparities = 816;
	pCFG->depthimg_param.blockSize = 21;
	pCFG->depthimg_param.uniqRatio = 9;

	strcpy(pCFG->pointcloud_param.imgRoot, "data/PointCloudImage/");
	strcpy(pCFG->pointcloud_param.imgPrefixNameL, "CSV_001C1S00P");
	strcpy(pCFG->pointcloud_param.imgPrefixNameR, "CSV_001C2S00P");
	strcpy(pCFG->pointcloud_param.outFileXYZ, "pointcloud.xyz");

	pCFG->calib_param.groupDemarcate = 1;
	strcpy(pCFG->calib_param.path, "data/calibImage/");
	strcpy(pCFG->calib_param.calibFile, "grayCodeParams.xml");

	pGP->Version = (GEV_VERSION_MAJOR<<16) | GEV_VERSION_MINOR;
	pGP->DeviceMode = GEV_DEVICE_MODE;


	return 0;
}



#ifdef __cplusplus
}
#endif



