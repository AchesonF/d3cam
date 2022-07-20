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

	pGP->VersionMajor = GEV_VERSION_MAJOR;
	pGP->VersionMinor = GEV_VERSION_MINOR;
	pGP->DeviceMode = (DM_ENDIANESS|DM_CLASS_TRANSMITTER|DM_CLC_SingleLC|DM_CHARACTER_UTF8);
	pGP->IfCapability0 = (NICap0_LLA|NICap0_DHCP|NICap0_PIP);
	pGP->IfConfiguration0 = (NICap0_LLA|NICap0_DHCP);

	strcpy(pGP->ManufacturerName, "CSVision");
	strcpy(pGP->ModelName, GEV_DEVICE_MODEL_NAME);
	strcpy(pGP->DeviceVersion, SOFTWARE_VERSION);
	strcpy(pGP->ManufacturerInfo, "CS vision 3d infomation");
	strcpy(pGP->SerialNumber, "CS300128001"); // todo update

	uint32_t file_size = 0;
	pGP->strXmlfile = GEV_XML_FILENAME;
	csv_file_get_size(pGP->strXmlfile, &file_size);
	snprintf(pGP->FirstURL, GVCP_URL_MAX_LEN, "local:%s;%x;%x", 
		pGP->strXmlfile, REG_StartOfXmlFile, file_size);

	pGP->GVSPCapability = GVSPCap_SP|GVSPCap_LB;
	pGP->MessageCapability = MSGCap_MCSP;
	pGP->GVCPCapability = GVCPCap_UN|GVCPCap_SN|GVCPCap_HD|GVCPCap_CAP
		|GVCPCap_DD|GVCPCap_PR|GVCPCap_W|GVCPCap_C;

	pGP->ControlChannelPrivilege = (CCP_CSE|CCP_CA|CCP_EA);


	return 0;
}



#ifdef __cplusplus
}
#endif



