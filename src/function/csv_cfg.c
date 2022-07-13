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

	pCFG->depthimg_param.numDisparities = 816;
	pCFG->depthimg_param.blockSize = 21;
	pCFG->depthimg_param.uniqRatio = 9;


	pGP->Version = (GEV_VERSION_MAJOR<<16) | GEV_VERSION_MINOR;
	pGP->DeviceMode = GEV_DEVICE_MODE;
	pGP->MacHi = (uint32_t)u8v_to_u16(&gCSV->eth.MACAddr[0]);
	pGP->MacLow = u8v_to_u32(&gCSV->eth.MACAddr[2]);




	return 0;
}



#ifdef __cplusplus
}
#endif



