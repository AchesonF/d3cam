#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static int csv_cfg_device (struct device_cfg_t *pDevC)
{
	struct dlp_cfg_t *pDlpcfg = NULL;

	pDevC->DeviceType = 0;
	pDevC->SwitchCams = 0;
	pDevC->CamImageType = 0;
	pDevC->SaveBmpFile = 0;

	pDlpcfg = &pDevC->dlpcfg[DLP_CMD_NORMAL];
	strcpy(pDlpcfg->name, "normal");
	pDlpcfg->rate = 80.0f;
	pDlpcfg->brightness = 700.0f;
	pDlpcfg->expoTime = 10000.0f;

	pDlpcfg = &pDevC->dlpcfg[DLP_CMD_DEMARCATE];
	strcpy(pDlpcfg->name, "demarcate");
	pDlpcfg->rate = 80.0f;
	pDlpcfg->brightness = 700.0f;
	pDlpcfg->expoTime = 10000.0f;

	pDlpcfg = &pDevC->dlpcfg[DLP_CMD_BRIGHT];
	strcpy(pDlpcfg->name, "bright");
	pDlpcfg->rate = 80.0f;
	pDlpcfg->brightness = 700.0f;
	pDlpcfg->expoTime = 10000.0f;

	pDlpcfg = &pDevC->dlpcfg[DLP_CMD_HIGHSPEED];
	strcpy(pDlpcfg->name, "highspeed");
	pDlpcfg->rate = 80.0f;
	pDlpcfg->brightness = 700.0f;
	pDlpcfg->expoTime = 40000.0f;

	return 0;
}

static int csv_cfg_depth (struct depthimg_cfg_t *pDepC)
{
	pDepC->NumDisparities = 816;
	pDepC->BlockSize = 21;
	pDepC->UniqRatio = 9;

	return 0;
}

static int csv_cfg_pointcloud (struct pointcloud_cfg_t *pPC)
{

	strcpy(pPC->ImageSaveRoot, "data/PointCloudImage");
	strcpy(pPC->calibFile, "CSV_Cali_HST_L21D_C0280.xml");
	strcpy(pPC->outFileXYZ, "pointcloud.xyz");
	strcpy(pPC->outDepthImage, "depthImage.png");
	pPC->saveXYZ = 0;
	pPC->saveDepthImage = 0;
	pPC->groupPointCloud = 1;
	pPC->enable = 0;
	pPC->initialized = 0;

	return 0;
}

static int csv_cfg_calib (struct calib_conf_t *pCALIB)
{
	strcpy(pCALIB->path, "data/calibImage");
	pCALIB->groupDemarcate = 1;

	return 0;
}

static int csv_cfg_gev (struct gev_conf_t *pGC)
{
	int i = 0;
	uint32_t file_size = 0;
	struct channel_cfg_t *pCH = NULL;

	pGC->VersionMajor = GEV_VERSION_MAJOR;
	pGC->VersionMinor = GEV_VERSION_MINOR;
	pGC->DeviceMode = (DM_E|DM_DC_TRANSMITTER|DM_CLC_SingleLC|DM_CHARACTER_UTF8);
	pGC->IfCapability0 = (NIC_LLA|NIC_DHCP|NIC_PIP);
	pGC->IfConfiguration0 = (NIC_LLA|NIC_DHCP);

	strcpy(pGC->ManufacturerName, "CSVision");
	strcpy(pGC->ModelName, GEV_DEVICE_MODEL_NAME);
	strcpy(pGC->DeviceVersion, SOFTWARE_VERSION);
	strcpy(pGC->ManufacturerInfo, "CS vision 3d infomation");
	strcpy(pGC->SerialNumber, "CS300128001"); // todo update

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
	strcpy(pGC->SecondURL, pGC->FirstURL);

	pGC->NumberofNetworkInterfaces = 1;
	pGC->LinkSpeed0 = 100;

	pGC->NumberofMessageChannels = 1;
	pGC->NumberofStreamChannels = TOTAL_CAMS;
	pGC->NumberofActionSignals = 1;
	pGC->ActionDeviceKey = 0;
	pGC->NumberofActiveLinks = 1;
	pGC->GVSPCapability = GVSPCap_SP|GVSPCap_LB;
	pGC->MessageCapability = MSGCap_SP;
	pGC->GVCPCapability = GVCPCap_UN|GVCPCap_SN|GVCPCap_HD|GVCPCap_LS //|GVCPCap_TD
		|GVCPCap_ES|GVCPCap_PTP|GVCPCap_ES2|GVCPCap_SAC
		|GVCPCap_A|GVCPCap_E|GVCPCap_PR|GVCPCap_W|GVCPCap_C;
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

	for (i = 0; i < TOTAL_CAMS; i++) {
		pCH = &pGC->Channel[i];

		pCH->Address = 0x00000000;
		pCH->Port = 0x0000;
		pCH->Cfg_PacketSize = SCPS_D|DEFAULT_GVSP_PACKETSIZE;
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
	csv_cfg_depth(&pCFG->depthimgcfg);
	csv_cfg_pointcloud(&pCFG->pointcloudcfg);
	csv_cfg_calib(&pCFG->calibcfg);
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



