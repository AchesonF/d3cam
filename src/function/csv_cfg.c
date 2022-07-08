#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif





int csv_cfg_init (void)
{
	struct csv_cfg_t *pCFG = &gCSV->cfg;


	pCFG->device_param.device_type = 0;
	pCFG->device_param.camera_leftright_type = false;
	pCFG->device_param.camera_img_type = true;
	pCFG->device_param.exposure_time = 10000.0f;
	pCFG->device_param.exposure_time_for_rgb = 10000.0f;

	pCFG->depthimg_param.numDisparities = 816;
	pCFG->depthimg_param.blockSize = 21;
	pCFG->depthimg_param.uniqRatio = 9;



	return 0;
}



#ifdef __cplusplus
}
#endif



