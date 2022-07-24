#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif




int csv_cfg_init (void)
{
	struct csv_cfg_t *pCFG = &gCSV->cfg;
	struct gev_conf_t *pGC = &pCFG->gigecfg;
	struct device_cfg_t *pDevC = &pCFG->devicecfg;
	struct depthimg_cfg_t *pDepC = &pCFG->depthimgcfg;

	pDevC->device_type = 0;
	pDevC->camera_leftright_type = false;
	pDevC->camera_img_type = true;
	pDevC->exposure_time = 40000.0f;
	pDevC->exposure_time_for_rgb = 10000.0f;
	pDevC->dlp_rate = 0x002D;
	pDevC->dlp_brightness = 700;
	pDevC->img_type = SUFFIX_BMP;

	pDepC->numDisparities = 816;
	pDepC->blockSize = 21;
	pDepC->uniqRatio = 9;

	strcpy(pCFG->pointcloudcfg.imgRoot, "data/PointCloudImage");
	strcpy(pCFG->pointcloudcfg.imgPrefixNameL, "CSV_001C1S00P");
	strcpy(pCFG->pointcloudcfg.imgPrefixNameR, "CSV_001C2S00P");
	strcpy(pCFG->pointcloudcfg.outFileXYZ, "pointcloud.xyz");
	pCFG->pointcloudcfg.enable = 0;

	pCFG->calibcfg.groupDemarcate = 1;
	strcpy(pCFG->calibcfg.path, "data/calibImage");
	strcpy(pCFG->calibcfg.calibFile, "CSV_Cali_HST_L21D_C0280.xml");

	pGC->VersionMajor = GEV_VERSION_MAJOR;
	pGC->VersionMinor = GEV_VERSION_MINOR;
	pGC->DeviceMode = (DM_ENDIANESS|DM_CLASS_TRANSMITTER|DM_CLC_SingleLC|DM_CHARACTER_UTF8);
	pGC->IfCapability0 = (NICap0_LLA|NICap0_DHCP|NICap0_PIP);
	pGC->IfConfiguration0 = (NICap0_LLA|NICap0_DHCP);

	strcpy(pGC->ManufacturerName, "CSVision");
	strcpy(pGC->ModelName, GEV_DEVICE_MODEL_NAME);
	strcpy(pGC->DeviceVersion, SOFTWARE_VERSION);
	strcpy(pGC->ManufacturerInfo, "CS vision 3d infomation");
	strcpy(pGC->SerialNumber, "CS300128001"); // todo update

	uint32_t file_size = 0;
	pGC->strXmlfile = GEV_XML_FILENAME;
	pGC->xmlData = NULL;
	csv_file_get_size(pGC->strXmlfile, &file_size);
	snprintf(pGC->FirstURL, GVCP_URL_MAX_LEN, "local:%s;%x;%x", 
		pGC->strXmlfile, REG_StartOfXmlFile, file_size);
	if (file_size > 0) {
		pGC->xmlData = (uint8_t *)malloc(file_size);
		if (NULL != pGC->xmlData) {
			csv_file_read_data(pGC->strXmlfile, pGC->xmlData, file_size);
		}
	}

	pGC->GVSPCapability = GVSPCap_SP|GVSPCap_LB;
	pGC->MessageCapability = MSGCap_MCSP;
	pGC->GVCPCapability = GVCPCap_UN|GVCPCap_SN|GVCPCap_HD|GVCPCap_CAP
		|GVCPCap_DD|GVCPCap_PR|GVCPCap_W|GVCPCap_C;

	pGC->ControlChannelPrivilege = (CCP_CSE|CCP_CA|CCP_EA);


	return 0;
}

int csv_cfg_deinit (void)
{
	struct csv_cfg_t *pCFG = &gCSV->cfg;
	struct gev_conf_t *pGC = &pCFG->gigecfg;

	if (NULL != pGC->xmlData) {
		free(pGC->xmlData);
		pGC->xmlData = NULL;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif



