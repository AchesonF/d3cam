#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static int csv_cfg_device (struct device_conf_t *pDevC)
{
	struct dlp_conf_t *pDlpcfg = NULL;

	pDevC->SwitchCams = 0;
	pDevC->SaveImageFile = 1;
	pDevC->SaveImageFormat = SUFFIX_BMP;
	pDevC->ftpupload = 1;
	pDevC->exportCamsCfg = 0;
	pDevC->strSuffix = ".bmp";

	pDlpcfg = &pDevC->dlpcfg[DLP_CMD_POINTCLOUD];
	strcpy(pDlpcfg->name, "深度采图13对");
	pDlpcfg->rate = 60.0f;
	pDlpcfg->brightness = 1000.0f;
	pDlpcfg->expoTime = 20000.0f;

	pDlpcfg = &pDevC->dlpcfg[DLP_CMD_CALIB];
	strcpy(pDlpcfg->name, "标定采图22对");
	pDlpcfg->rate = 60.0f;
	pDlpcfg->brightness = 1000.0f;
	pDlpcfg->expoTime = 20000.0f;

	pDlpcfg = &pDevC->dlpcfg[DLP_CMD_BRIGHT];
	strcpy(pDlpcfg->name, "亮光采图1对");
	pDlpcfg->rate = 60.0f;
	pDlpcfg->brightness = 1000.0f;
	pDlpcfg->expoTime = 20000.0f;

	pDlpcfg = &pDevC->dlpcfg[DLP_CMD_HDRI];
	strcpy(pDlpcfg->name, "HDRI采多图");
	pDlpcfg->rate = 60.0f;
	pDlpcfg->brightness = 1000.0f;
	pDlpcfg->expoTime = 1000000.0f;

	return 0;
}

static int csv_cfg_pointcloud (struct pointcloud_conf_t *pPC)
{
	strcpy(pPC->ModelRoot, PATH_MODEL_FILES);
	snprintf(pPC->PCImageRoot, 128, "%s/PointCloudImage", PATH_DATA_FILES);
	strcpy(pPC->calibFile, FILE_CALIB_XML);
	snprintf(pPC->outFileXYZ, 256, "%s/pointcloud.xyz", PATH_DATA_FILES);
	pPC->saveXYZ = 0;
	pPC->groupPointCloud = 1;
	pPC->initialized = 0;

	return 0;
}

static int csv_cfg_calib (struct calib_conf_t *pCALIB)
{
	snprintf(pCALIB->CalibImageRoot, 128, "%s/calibImage", PATH_DATA_FILES);
	pCALIB->groupCalibrate = 1;

	return 0;
}

static int csv_cfg_hdri (struct hdri_conf_t *pHDRI)
{
	snprintf(pHDRI->HdrImageRoot, 128, "%s/HdrImage", PATH_DATA_FILES);
	pHDRI->groupHdri = 1;

	return 0;
}

static int csv_cfg_gev (struct gev_conf_t *pGC)
{
	uint32_t file_size = 0;
	int i = 0;
	struct channel_conf_t *pCH = NULL;

	pGC->VersionMajor = GEV_VERSION_MAJOR;
	pGC->VersionMinor = GEV_VERSION_MINOR;
	pGC->DeviceMode = (DM_E|DM_DC_TRANSMITTER|DM_CLC_SingleLC|DM_CHARACTER_UTF8);
	pGC->IfCapability0 = (NIC_LLA|NIC_DHCP|NIC_PIP);
	pGC->IfConfiguration0 = (NIC_LLA|NIC_DHCP|NIC_PIP);

	strcpy(pGC->ManufacturerName, "CSVision");
	strcpy(pGC->ModelName, GEV_DEVICE_MODEL_NAME);
	strcpy(pGC->DeviceVersion, SOFTWARE_VERSION);
	strcpy(pGC->ManufacturerInfo, "CS Vision 3d highspeed structure.");
	strcpy(pGC->SerialNumber, "CS300131001"); // todo update

	pGC->strXmlfile = FILE_GEV_XML;
	pGC->xmlData = NULL;
	if (!csv_file_isPath(pGC->strXmlfile, S_IFREG)) {
		log_warn("ERROR : lost gev xml file.");
	} else {
		csv_file_get_size(pGC->strXmlfile, &file_size);
		pGC->xmlLength = file_size;
		snprintf(pGC->FirstURL, GVCP_URL_MAX_LEN, "local:%s;%x;%x", 
			pGC->strXmlfile, REG_StartOfXmlFile, file_size);
		if (file_size > 0) {
			pGC->xmlData = (uint8_t *)malloc(file_size);
			if (NULL != pGC->xmlData) {
				csv_file_read_data(pGC->strXmlfile, pGC->xmlData, file_size);
			} else {
				log_err("ERROR : malloc xmlData");
			}
		}
		strcpy(pGC->SecondURL, pGC->FirstURL);
	}

	pGC->NumberofNetworkInterfaces = 1;
	pGC->LinkSpeed0 = 1000;

	pGC->NumberofMessageChannels = 1;
	pGC->NumberofStreamChannels = TOTAL_CHANNELS;
	pGC->NumberofActionSignals = 1;
	pGC->ActionDeviceKey = 0;
	pGC->NumberofActiveLinks = 1;
	pGC->GVSPCapability = GVSPCap_SP|GVSPCap_LB;
	pGC->MessageCapability = MSGCap_SP;
	pGC->GVCPCapability = GVCPCap_UN|GVCPCap_SN|GVCPCap_HD|GVCPCap_LS //|GVCPCap_TD
		|GVCPCap_ES|GVCPCap_PTP|GVCPCap_ES2|GVCPCap_SAC
		|GVCPCap_A|GVCPCap_E|/*GVCPCap_PR|*/GVCPCap_W|GVCPCap_C;
	pGC->HeartbeatTimeout = GVCP_HEARTBEAT_TIMEOUT;
	pGC->TimestampTickFrequencyHigh = 0;
	pGC->TimestampTickFrequencyLow = 0x3B9ACA00;
	pGC->TimestampControl = 0;
	pGC->TimestampValueHigh = 0;
	pGC->TimestampValueLow= 0;
	pGC->DiscoveryACKDelay = 0;
	pGC->GVCPConfiguration = GVCPCfg_PTP|GVCPCfg_ES2|GVCPCfg_UA|GVCPCfg_ES|GVCPCfg_PE;
	pGC->PendingTimeout = 0;
	pGC->ControlSwitchoverKey = 0;
	pGC->GVSPConfiguration = GVSPCcfg_BL;
	pGC->PhysicalLinkConfigurationCapability = PLCC_SL;
	pGC->PhysicalLinkConfiguration = PLC_SL;
	pGC->IEEE1588Status = 0;
	pGC->ScheduledActionCommandQueueSize = 0;
	pGC->ControlChannelPrivilege = 0;//(CCP_CSE|CCP_CA|CCP_EA);

	pGC->PrimaryAddress = 0x00000000;
	pGC->PrimaryPort = 0x0000;

	pGC->MessageAddress = 0x00000000;
	pGC->MessagePort = 0x0000;
	pGC->MessageTimeout = 2000;
	pGC->MessageRetryCount = 0;
	pGC->MessageSourcePort = 0x0000;

	for (i = CAM_LEFT; i < TOTAL_CHANNELS; i++) {
		pCH = &pGC->Channel[i];
		pCH->idx = i;
		pCH->Address = 0x00000000;
		pCH->Port = 0x0000;
		pCH->Cfg_PacketSize = SCPS_D|SCPS_P|DEFAULT_GVSP_PACKETSIZE;
		pCH->PacketDelay = 0;
		pCH->Configuration = 0;
		pCH->SourcePort = 0;
		pCH->Capability = 0;
		pCH->Zone = 0;
		pCH->ZoneDirection = 0;
	}

	return 0;
}

int csv_cfg_init (void)
{
	struct csv_cfg_t *pCFG = &gCSV->cfg;

	csv_cfg_device(&pCFG->devicecfg);
	csv_cfg_pointcloud(&pCFG->pointcloudcfg);
	csv_cfg_calib(&pCFG->calibcfg);
	csv_cfg_hdri(&pCFG->hdricfg);
	csv_cfg_gev(&pCFG->gigecfg);

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



