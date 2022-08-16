#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static struct reglist_t *csv_gev_reg_malloc (void)
{
	struct reglist_t *cur = NULL;

	cur = (struct reglist_t *)malloc(sizeof(struct reglist_t));
	if (cur == NULL) {
		log_err("ERROR : malloc reglist_t.");
		return NULL;
	}
	memset(cur, 0, sizeof(struct reglist_t));

	return cur;
}

static void csv_gev_reg_add (uint32_t addr, uint8_t type, uint8_t mode,
	uint16_t length, uint32_t value, char *info, char *desc)
{
	int len = 0;
	struct reglist_t *cur = NULL;
	struct reg_info_t *pRI = NULL;

	cur = csv_gev_reg_malloc();
	if (cur != NULL) {
		pRI = &cur->ri;
		pRI->addr = addr;
		pRI->type = type;
		pRI->mode = mode;
		pRI->length = length;
		if (GEV_REG_TYPE_REG == type) {
			pRI->value = value;
		} else if (GEV_REG_TYPE_MEM == type) {
			if (NULL == info) {
				log_warn("WARN : mem 0x%04X null.", addr);
				return ;
			}

			len = strlen(info);
			if (len > 0) {
				if (len > length) {
					len = length;
				}
				pRI->info = (char *)malloc(len+1);
				if (NULL == pRI->info) {
					log_err("ERROR : malloc reg info.");
					return ;
				}
				memset(pRI->info, 0, len+1);
				memcpy(pRI->info, info, len);
			}
		} else {
			log_warn("ERROR : not support type.");
		}

		if (NULL != desc) {
			len = strlen(desc);
			pRI->desc = (char *)malloc(len+1);
			if (NULL == pRI->desc) {
				log_err("ERROR : malloc reg desc.");
				return ;
			}
			memset(pRI->desc, 0, len+1);
			memcpy(pRI->desc, desc, len);
		}

		list_add_tail(&cur->list, &gCSV->gev.head_reg.list);
	}
}

static uint32_t csv_gev_reg_value_get (uint32_t addr, uint32_t *value)
{
	int ret = -1;
	struct list_head *pos = NULL;
	struct reglist_t *task = NULL;
	struct reg_info_t *pRI = NULL;

	list_for_each(pos, &gCSV->gev.head_reg.list) {
		task = list_entry(pos, struct reglist_t, list);
		if (task == NULL) {
			break;
		}

		pRI = &task->ri;

		if (addr == pRI->addr) {
			if (pRI->mode & GEV_REG_READ) {
				*value = pRI->value;
				ret = 0;
			} else {
				ret = -2;
			}

			break;
		}
	}

	return ret;
}

static int csv_gev_reg_value_set (uint32_t addr, uint32_t value)
{
	int ret = -1;	// !0 : not save
	struct list_head *pos = NULL;
	struct reglist_t *task = NULL;
	struct reg_info_t *pRI = NULL;

	list_for_each(pos, &gCSV->gev.head_reg.list) {
		task = list_entry(pos, struct reglist_t, list);
		if (task == NULL) {
			break;
		}

		pRI = &task->ri;

		if (addr == pRI->addr) {
			if (pRI->mode & GEV_REG_WRITE) {
				if (value != pRI->value) {
					pRI->value = value;
				}
				ret = 0;	// 0: wait to save
				break;
			} else {
				ret = -2;	// can't be write
				break;
			}
		}
	}

	return ret;
}

int csv_gev_reg_value_update (uint32_t addr, uint32_t value)
{
	int ret = -1;
	struct list_head *pos = NULL;
	struct reglist_t *task = NULL;
	struct reg_info_t *pRI = NULL;

	list_for_each(pos, &gCSV->gev.head_reg.list) {
		task = list_entry(pos, struct reglist_t, list);
		if (task == NULL) {
			break;
		}

		pRI = &task->ri;

		if (addr == pRI->addr) {
			if (value != pRI->value) {
				pRI->value = value;
			}
			ret = 0;
			break;
		}
	}

	return ret;
}

static uint16_t csv_gev_mem_info_get_length (uint32_t addr)
{
	uint16_t length = 0;
	struct list_head *pos = NULL;
	struct reglist_t *task = NULL;
	struct reg_info_t *pRI = NULL;

	list_for_each(pos, &gCSV->gev.head_reg.list) {
		task = list_entry(pos, struct reglist_t, list);
		if (task == NULL) {
			break;
		}

		pRI = &task->ri;

		if (addr == pRI->addr) {
			length = pRI->length;
			break;
		}
	}

	return length;
}

static int csv_gev_mem_info_get (uint32_t addr, char **info)
{
	int ret = -1;
	struct list_head *pos = NULL;
	struct reglist_t *task = NULL;
	struct reg_info_t *pRI = NULL;

	list_for_each(pos, &gCSV->gev.head_reg.list) {
		task = list_entry(pos, struct reglist_t, list);
		if (task == NULL) {
			break;
		}

		pRI = &task->ri;

		if (addr == pRI->addr) {
			if (pRI->mode & GEV_REG_READ) {
				*info = pRI->info;
				ret = 0;
			} else {
				ret = -2;
			}

			break;
		}
	}

	return ret;
}

int csv_gev_mem_info_set (uint32_t addr, char *info)
{
	int ret = -1;
	struct list_head *pos = NULL;
	struct reglist_t *task = NULL;
	struct reg_info_t *pRI = NULL;

	list_for_each(pos, &gCSV->gev.head_reg.list) {
		task = list_entry(pos, struct reglist_t, list);
		if (task == NULL) {
			break;
		}

		pRI = &task->ri;

		if (addr == pRI->addr) {
			if (pRI->mode & GEV_REG_WRITE) {
				if (NULL != pRI->info) {
					if (strcmp(info, pRI->info) == 0) {
						ret = 0;
						break;
					}

					free(pRI->info);
				}

				int len = strlen(info);
				if (len > pRI->length) {
					len = pRI->length;
				}

				pRI->info = (char *)malloc(len+1);
				if (NULL == pRI->info) {
					log_err("ERROR : malloc mem info.");
					return -1;
				}
				memset(pRI->info, 0, len+1);
				memcpy(pRI->info, info, len);

				ret = len;
			}

			break;
		}
	}

	return ret;
}

static uint8_t csv_gev_reg_type_get (uint32_t addr, char **desc)
{
	uint8_t type = GEV_REG_TYPE_NONE;
	struct list_head *pos = NULL;
	struct reglist_t *task = NULL;
	struct reg_info_t *pRI = NULL;

	list_for_each(pos, &gCSV->gev.head_reg.list) {
		task = list_entry(pos, struct reglist_t, list);
		if (task == NULL) {
			break;
		}

		pRI = &task->ri;

		if (addr == pRI->addr) {
			type = pRI->type;
			*desc = pRI->desc;
			break;
		}
	}

	return type;
}





static void csv_gev_reg_enroll (void)
{
	struct gev_conf_t *pGC = &gCSV->cfg.gigecfg;
	struct csv_eth_t *pETH = &gCSV->eth;
	//struct csv_mvs_t *pMVS = &gCSV->mvs;

	csv_gev_reg_add(REG_Version, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, (pGC->VersionMajor<<16)|pGC->VersionMinor, NULL, toSTR(REG_Version));
	csv_gev_reg_add(REG_DeviceMode, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->DeviceMode, NULL, toSTR(REG_DeviceMode));
	csv_gev_reg_add(REG_DeviceMACAddressHigh0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->MacHi, NULL, toSTR(REG_DeviceMACAddressHigh0));
	csv_gev_reg_add(REG_DeviceMACAddressLow0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->MacLow, NULL, toSTR(REG_DeviceMACAddressLow0));
	csv_gev_reg_add(REG_NetworkInterfaceCapability0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->IfCapability0, NULL, toSTR(REG_NetworkInterfaceCapability0));
	csv_gev_reg_add(REG_NetworkInterfaceConfiguration0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->IfConfiguration0, NULL, toSTR(REG_NetworkInterfaceConfiguration0));
	csv_gev_reg_add(REG_CurrentIPAddress0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->CurrentIPAddress0, NULL, toSTR(REG_CurrentIPAddress0));
	csv_gev_reg_add(REG_CurrentSubnetMask0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->CurrentSubnetMask0, NULL, toSTR(REG_CurrentSubnetMask0));
	csv_gev_reg_add(REG_CurrentDefaultGateway0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->CurrentDefaultGateway0, NULL, toSTR(REG_CurrentDefaultGateway0));

	csv_gev_reg_add(REG_ManufacturerName, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		32, 0, pGC->ManufacturerName, toSTR(REG_ManufacturerName));
	csv_gev_reg_add(REG_ModelName, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		32, 0, pGC->ModelName, toSTR(REG_ModelName));
	csv_gev_reg_add(REG_DeviceVersion, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		32, 0, pGC->DeviceVersion, toSTR(REG_DeviceVersion));
	csv_gev_reg_add(REG_ManufacturerInfo, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		48, 0, pGC->ManufacturerInfo, toSTR(REG_ManufacturerInfo));
	csv_gev_reg_add(REG_SerialNumber, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		16, 0, pGC->SerialNumber, toSTR(REG_SerialNumber));
	csv_gev_reg_add(REG_UserdefinedName, GEV_REG_TYPE_MEM, GEV_REG_RDWR, 
		16, 0, pGC->UserdefinedName, toSTR(REG_UserdefinedName));
	csv_gev_reg_add(REG_FirstURL, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		512, 0, pGC->FirstURL, toSTR(REG_FirstURL));
	csv_gev_reg_add(REG_SecondURL, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		512, 0, pGC->SecondURL, toSTR(REG_SecondURL));

	csv_gev_reg_add(REG_NumberofNetworkInterfaces, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->NumberofNetworkInterfaces, NULL, toSTR(REG_NumberofNetworkInterfaces));

	csv_gev_reg_add(REG_PersistentIPAddress, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pETH->IPAddr, NULL, toSTR(REG_PersistentIPAddress));
	csv_gev_reg_add(REG_PersistentSubnetMask0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pETH->NetmaskAddr, NULL, toSTR(REG_PersistentSubnetMask0));
	csv_gev_reg_add(REG_PersistentDefaultGateway0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pETH->GatewayAddr, NULL, toSTR(REG_PersistentDefaultGateway0));
	csv_gev_reg_add(REG_LinkSpeed0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->LinkSpeed0, NULL, toSTR(REG_LinkSpeed0));

	csv_gev_reg_add(REG_NumberofMessageChannels, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->NumberofMessageChannels, NULL, toSTR(REG_NumberofMessageChannels));
	csv_gev_reg_add(REG_NumberofStreamChannels, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->NumberofStreamChannels, NULL, toSTR(REG_NumberofStreamChannels));
	csv_gev_reg_add(REG_NumberofActionSignals, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->NumberofActionSignals, NULL, toSTR(REG_NumberofActionSignals));
	csv_gev_reg_add(REG_ActionDeviceKey, GEV_REG_TYPE_REG, GEV_REG_WRITE, 
		4, pGC->ActionDeviceKey, NULL, toSTR(REG_ActionDeviceKey));
	csv_gev_reg_add(REG_NumberofActiveLinks, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->NumberofActiveLinks, NULL, toSTR(REG_NumberofActiveLinks));

	csv_gev_reg_add(REG_GVSPCapability, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->GVSPCapability, NULL, toSTR(REG_GVSPCapability));
	csv_gev_reg_add(REG_MessageChannelCapability, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->MessageCapability, NULL, toSTR(REG_MessageChannelCapability));
	csv_gev_reg_add(REG_GVCPCapability, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->GVCPCapability, NULL, toSTR(REG_GVCPCapability));
	csv_gev_reg_add(REG_HeartbeatTimeout, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->HeartbeatTimeout, NULL, toSTR(REG_HeartbeatTimeout));
	csv_gev_reg_add(REG_TimestampTickFrequencyHigh, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->TimestampTickFrequencyHigh, NULL, toSTR(REG_TimestampTickFrequencyHigh));
	csv_gev_reg_add(REG_TimestampTickFrequencyLow, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->TimestampTickFrequencyLow, NULL, toSTR(REG_TimestampTickFrequencyLow));
	csv_gev_reg_add(REG_TimestampControl, GEV_REG_TYPE_REG, GEV_REG_WRITE, 
		4, pGC->TimestampControl, NULL, toSTR(REG_TimestampControl));
	csv_gev_reg_add(REG_TimestampValueHigh, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->TimestampValueHigh, NULL, toSTR(REG_TimestampValueHigh));
	csv_gev_reg_add(REG_TimestampValueLow, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->TimestampValueLow, NULL, toSTR(REG_TimestampValueLow));
	csv_gev_reg_add(REG_DiscoveryACKDelay, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->DiscoveryACKDelay, NULL, toSTR(REG_DiscoveryACKDelay));
	csv_gev_reg_add(REG_GVCPConfiguration, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->GVCPConfiguration, NULL, toSTR(REG_GVCPConfiguration));
	csv_gev_reg_add(REG_PendingTimeout, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->PendingTimeout, NULL, toSTR(REG_PendingTimeout));
	csv_gev_reg_add(REG_ControlSwitchoverKey, GEV_REG_TYPE_REG, GEV_REG_WRITE, 
		4, pGC->ControlSwitchoverKey, NULL, toSTR(REG_ControlSwitchoverKey));
	csv_gev_reg_add(REG_GVSPConfiguration, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->GVSPConfiguration, NULL, toSTR(REG_GVSPConfiguration));
	csv_gev_reg_add(REG_PhysicalLinkConfigurationCapability, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->PhysicalLinkConfigurationCapability, NULL, toSTR(REG_PhysicalLinkConfigurationCapability));
	csv_gev_reg_add(REG_PhysicalLinkConfiguration, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->PhysicalLinkConfiguration, NULL, toSTR(REG_PhysicalLinkConfiguration));
	csv_gev_reg_add(REG_IEEE1588Status, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->IEEE1588Status, NULL, toSTR(REG_IEEE1588Status));
	csv_gev_reg_add(REG_ScheduledActionCommandQueueSize, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->ScheduledActionCommandQueueSize, NULL, toSTR(REG_ScheduledActionCommandQueueSize));

	csv_gev_reg_add(REG_ControlChannelPrivilege, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->ControlChannelPrivilege, NULL, toSTR(REG_ControlChannelPrivilege));
	csv_gev_reg_add(REG_PrimaryApplicationPort, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->PrimaryPort, NULL, toSTR(REG_PrimaryApplicationPort));
	csv_gev_reg_add(REG_PrimaryApplicationIPAddress, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->PrimaryAddress, NULL, toSTR(REG_PrimaryApplicationIPAddress));

	csv_gev_reg_add(REG_MessageChannelPort, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->MessagePort, NULL, toSTR(REG_MessageChannelPort));
	csv_gev_reg_add(REG_MessageChannelDestination, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->MessageAddress, NULL, toSTR(REG_MessageChannelDestination));
	csv_gev_reg_add(REG_MessageChannelTransmissionTimeout, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->MessageTimeout, NULL, toSTR(REG_MessageChannelTransmissionTimeout));
	csv_gev_reg_add(REG_MessageChannelRetryCount, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->MessageRetryCount, NULL, toSTR(REG_MessageChannelRetryCount));
	csv_gev_reg_add(REG_MessageChannelSourcePort, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->MessageSourcePort, NULL, toSTR(REG_MessageChannelSourcePort));

	csv_gev_reg_add(REG_StreamChannelPort0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_LEFT].Port, NULL, toSTR(REG_StreamChannelPort0));
	csv_gev_reg_add(REG_StreamChannelPacketSize0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_LEFT].Cfg_PacketSize, NULL, toSTR(REG_StreamChannelPacketSize0));
	csv_gev_reg_add(REG_StreamChannelPacketDelay0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_LEFT].PacketDelay, NULL, toSTR(REG_StreamChannelPacketDelay0));
	csv_gev_reg_add(REG_StreamChannelDestinationAddress0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_LEFT].Address, NULL, toSTR(REG_StreamChannelDestinationAddress0));
	csv_gev_reg_add(REG_StreamChannelSourcePort0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel[CAM_LEFT].SourcePort, NULL, toSTR(REG_StreamChannelSourcePort0));
	csv_gev_reg_add(REG_StreamChannelCapability0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel[CAM_LEFT].Capability, NULL, toSTR(REG_StreamChannelCapability0));
	csv_gev_reg_add(REG_StreamChannelConfiguration0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_LEFT].Configuration, NULL, toSTR(REG_StreamChannelConfiguration0));
	csv_gev_reg_add(REG_StreamChannelZone0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel[CAM_LEFT].Zone, NULL, toSTR(REG_StreamChannelZone0));
	csv_gev_reg_add(REG_StreamChannelZoneDirection0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel[CAM_LEFT].ZoneDirection, NULL, toSTR(REG_StreamChannelZoneDirection0));

	csv_gev_reg_add(REG_StreamChannelPort1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_RIGHT].Port, NULL, toSTR(REG_StreamChannelPort1));
	csv_gev_reg_add(REG_StreamChannelPacketSize1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_RIGHT].Cfg_PacketSize, NULL, toSTR(REG_StreamChannelPacketSize1));
	csv_gev_reg_add(REG_StreamChannelPacketDelay1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_RIGHT].PacketDelay, NULL, toSTR(REG_StreamChannelPacketDelay1));
	csv_gev_reg_add(REG_StreamChannelDestinationAddress1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_RIGHT].Address, NULL, toSTR(REG_StreamChannelDestinationAddress1));
	csv_gev_reg_add(REG_StreamChannelSourcePort1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel[CAM_RIGHT].SourcePort, NULL, toSTR(REG_StreamChannelSourcePort1));
	csv_gev_reg_add(REG_StreamChannelCapability1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel[CAM_RIGHT].Capability, NULL, toSTR(REG_StreamChannelCapability1));
	csv_gev_reg_add(REG_StreamChannelConfiguration1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_RIGHT].Configuration, NULL, toSTR(REG_StreamChannelConfiguration1));
	csv_gev_reg_add(REG_StreamChannelZone1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel[CAM_RIGHT].Zone, NULL, toSTR(REG_StreamChannelZone1));
	csv_gev_reg_add(REG_StreamChannelZoneDirection1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel[CAM_RIGHT].ZoneDirection, NULL, toSTR(REG_StreamChannelZoneDirection1));

	csv_gev_reg_add(REG_OtherStreamChannelsRegisters, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, toSTR(REG_OtherStreamChannelsRegisters));

	// stream 2-3 to add more

	csv_gev_reg_add(REG_ManifestTable, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, toSTR(REG_ManifestTable));

	csv_gev_reg_add(REG_ActionGroupKey0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, toSTR(REG_ActionGroupKey0));
	csv_gev_reg_add(REG_ActionGroupMask0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, toSTR(REG_ActionGroupMask0));

	csv_gev_reg_add(REG_ActionGroupKey1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, toSTR(REG_ActionGroupKey1));
	csv_gev_reg_add(REG_ActionGroupMask1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, toSTR(REG_ActionGroupMask1));

	csv_gev_reg_add(REG_OtherActionSignalsRegisters, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, toSTR(REG_OtherActionSignalsRegisters));

	csv_gev_reg_add(REG_StartofManufacturerSpecificRegisterSpace, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, toSTR(REG_StartofManufacturerSpecificRegisterSpace));

	// add more from genicam xml.




	log_info("OK : enroll gev reg.");
}

static void csv_gev_reg_disroll (void)
{
	struct list_head *pos = NULL, *n = NULL;
	struct reglist_t *task = NULL;
	struct reg_info_t *pRI = NULL;

	list_for_each_safe(pos, n, &gCSV->gev.head_reg.list) {
		task = list_entry(pos, struct reglist_t, list);
		if (task == NULL) {
			break;
		}

		pRI = &task->ri;

		if (NULL != pRI->info) {
			free(pRI->info);
			pRI->info = NULL;
		}

		if (NULL != pRI->desc) {
			free(pRI->desc);
			pRI->desc = NULL;
		}

		list_del(&task->list);
		free(task);
		task = NULL;
	}

	log_info("OK : disroll gev reg.");
}


int csv_gvcp_sendto (struct csv_gev_t *pGEV)
{
	socklen_t from_len = sizeof(struct sockaddr_in);

	log_hex(STREAM_UDP, pGEV->txbuf, pGEV->txlen, "gev sendto '%s:%d'",
		inet_ntoa(pGEV->from_addr.sin_addr), htons(pGEV->from_addr.sin_port));

	return sendto(pGEV->fd, pGEV->txbuf, pGEV->txlen, 0, 
		(struct sockaddr *)&pGEV->from_addr, from_len);
}

static int csv_gvcp_error_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR, uint16_t errcode)
{
	memset(pGEV->txbuf, 0, GVCP_MAX_MSG_LEN);

	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGEV->txbuf;

	pAckHdr->nStatus			= htons(errcode);
	pAckHdr->nAckMsgValue		= htons(pHDR->nCommand+1);
	pAckHdr->nLength			= htons(0x0000);
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	pGEV->txlen = sizeof(ACK_MSG_HEADER);

	return csv_gvcp_sendto(pGEV);
}

int csv_gvcp_event_send (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{
	CMD_MSG_HEADER *pHeader = (CMD_MSG_HEADER *)pGEV->txbuf;

	pHeader->cKeyValue = GVCP_CMD_KEY_VALUE;
	pHeader->cFlg = GVCP_CMD_FLAG_NEED_ACK; // TODO
	pHeader->nCommand = GEV_EVENT_CMD;
	pHeader->nReqId = pHDR->nReqId;


	pGEV->txlen = sizeof(ACK_MSG_HEADER) + 0;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_discover_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{
	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGEV->txbuf;

	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_DISCOVERY_ACK);
	pAckHdr->nLength			= htons(sizeof(DISCOVERY_ACK_MSG));
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	DISCOVERY_ACK_MSG *pAckMsg = (DISCOVERY_ACK_MSG *)(pGEV->txbuf + sizeof(ACK_MSG_HEADER));
	struct csv_eth_t *pETH = &gCSV->eth;
	struct gev_conf_t *pGC = &gCSV->cfg.gigecfg;

	pAckMsg->nMajorVer			= htons(pGC->VersionMajor);
	pAckMsg->nMinorVer			= htons(pGC->VersionMinor);
	pAckMsg->nDeviceMode		= htonl(pGC->DeviceMode);
	pAckMsg->nMacAddrHigh		= htons(pGC->MacHi)&0xFFFF;
	pAckMsg->nMacAddrLow		= htonl(pGC->MacLow);
	pAckMsg->nIpCfgOption		= htonl(pGC->IfCapability0);
	pAckMsg->nIpCfgCurrent		= htonl(pGC->IfConfiguration0);
	pAckMsg->nCurrentIp			= inet_addr(pETH->ip);
	pAckMsg->nCurrentSubNetMask	= inet_addr(pETH->nm);
	pAckMsg->nDefultGateWay		= inet_addr(pETH->gw);

	strncpy((char *)pAckMsg->chManufacturerName, pGC->ManufacturerName, 32);
	strncpy((char *)pAckMsg->chModelName, pGC->ModelName, 32);
	strncpy((char *)pAckMsg->chDeviceVersion, pGC->DeviceVersion, 32);
	strncpy((char *)pAckMsg->chManufacturerSpecificInfo, pGC->ManufacturerInfo, 48);
	strncpy((char *)pAckMsg->chSerialNumber, pGC->SerialNumber, 16);
	strncpy((char *)pAckMsg->chUserDefinedName, pGC->UserdefinedName, 16);

	pGEV->txlen = sizeof(ACK_MSG_HEADER) + sizeof(DISCOVERY_ACK_MSG);

	// TODO ack delay "Discovery ACK Delay"

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_forceip_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{
	FORCEIP_CMD_MSG *pFIP = (FORCEIP_CMD_MSG *)(pGEV->rxbuf + sizeof(CMD_MSG_HEADER));

	if (pFIP->nStaticIp == 0x00000000) {
		// TODO special ip
		return 0;
	}

	struct csv_eth_t *pETH = &gCSV->eth;

	// TODO compare mac

	pETH->IPAddr = pFIP->nStaticIp;
	pETH->GatewayAddr = pFIP->nStaticDefaultGateWay;
	pETH->NetmaskAddr = pFIP->nStaticSubNetMask;

	struct in_addr addr;
	addr.s_addr = pETH->IPAddr;
	strcpy(pETH->ip, inet_ntoa(addr));

	addr.s_addr = pETH->GatewayAddr;
	strcpy(pETH->gw, inet_ntoa(addr));

	addr.s_addr = pETH->NetmaskAddr;
	strcpy(pETH->nm, inet_ntoa(addr));

	// TODO effective ip

	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGEV->txbuf;

	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_FORCEIP_ACK);
	pAckHdr->nLength			= htons(0x0000);
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	pGEV->txlen = sizeof(ACK_MSG_HEADER) + pAckHdr->nLength;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_packetresend_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{

	//ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGEV->txbuf;

	//pAckHdr->nStatus			= htons(MV_GEV_STATUS_SUCCESS);
	//pAckHdr->nAckMsgValue		= htons(MV_GEV_PACKETRESEND_INVALID);
	//pAckHdr->nLength			= htons(0x0000);
	//pAckHdr->nAckId			= htons(pHDR->nReqId);

	pGEV->txlen = sizeof(ACK_MSG_HEADER) + 0;

	return csv_gvcp_sendto(pGEV);
}



static int csv_gvcp_readreg_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{
	if (pHDR->nLength > GVCP_MAX_PAYLOAD_LEN) {
		log_warn("ERROR : read too long reg addrs.");
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_HEADER);
		return -1;
	}

	if ((pHDR->nLength % sizeof(uint32_t) != 0)||(pGEV->rxlen != 8+pHDR->nLength)) {
		log_warn("ERROR : length not match.");
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_HEADER);
		return -1;
	}

	int i = 0, ret = -1;
	uint32_t *pRegAddr = (uint32_t *)(pGEV->rxbuf + sizeof(CMD_MSG_HEADER));
	uint32_t *pRegData = (uint32_t *)(pGEV->txbuf + sizeof(ACK_MSG_HEADER));
	int nRegs = pHDR->nLength / sizeof(uint32_t); // GVCP_READ_REG_MAX_NUM
	uint32_t reg_addr = 0;
	uint8_t type = GEV_REG_TYPE_NONE;
	uint32_t nTemp = 0;
	char *desc = NULL;

	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGEV->txbuf;
	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_READREG_ACK);
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	for (i = 0; i < nRegs; i++) {
		reg_addr = ntohl(*pRegAddr);

		if (reg_addr % 4) {
			csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_ADDRESS);
			return -1;
		}

		type = csv_gev_reg_type_get(reg_addr, &desc);
		if ((GEV_REG_TYPE_NONE == type)||(GEV_REG_TYPE_MEM == type)) {
			csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_ADDRESS);
			return -1;
		}

		ret = csv_gev_reg_value_get(reg_addr, &nTemp);
		if (ret < 0) {
			csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_ADDRESS);
			return -1;
		}

		if (NULL != desc) {
			log_debug("RD : %s", desc);
		}

		*pRegData++ = htonl(nTemp);
		pRegAddr++;
	}

	if (i != nRegs) {
		pAckHdr->nStatus		= htons(GEV_STATUS_INVALID_ADDRESS);
	}
	pAckHdr->nLength			= htons(i*4);

	pGEV->txlen = sizeof(ACK_MSG_HEADER) + i*4;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_writereg_effective (uint32_t regAddr, uint32_t regData)
{
	int ret = 0;

	struct csv_gev_t *pGEV = &gCSV->gev;
	struct gev_conf_t *pGC = &gCSV->cfg.gigecfg;

	switch (regAddr) {
	case REG_NetworkInterfaceConfiguration0:
		pGC->IfConfiguration0 = regData;
		break;

	case REG_PersistentIPAddress:
		break;

	case REG_PersistentSubnetMask0:
		break;

	case REG_PersistentDefaultGateway0:
		break;

	case REG_ActionDeviceKey:
		pGC->ActionDeviceKey = regData;
		break;

	case REG_HeartbeatTimeout:
		if (regData < GVCP_HEARTBEAT_INTERVAL) {
			pGC->HeartbeatTimeout = GVCP_HEARTBEAT_INTERVAL;
		} else {
			pGC->HeartbeatTimeout = regData;
		}
		break;

	case REG_TimestampControl:
		pGC->TimestampControl = regData;
		break;

	case REG_DiscoveryACKDelay:
		pGC->DiscoveryACKDelay = regData;
		break;

	case REG_GVCPConfiguration:
		pGC->GVCPConfiguration = regData;
		break;

	case REG_ControlSwitchoverKey:
		pGC->ControlSwitchoverKey = regData;
		break;

	case REG_GVSPConfiguration:
		pGC->GVSPConfiguration = regData;
		break;

	case REG_PhysicalLinkConfiguration:
		pGC->PhysicalLinkConfiguration = regData;
		break;

	case REG_ControlChannelPrivilege:
		pGC->ControlChannelPrivilege = regData;
		
		break;

	case REG_PrimaryApplicationPort:
		pGC->PrimaryPort = (uint16_t)regData;
		break;

	case REG_PrimaryApplicationIPAddress:
		if (0 != regData) {
			pGC->PrimaryAddress = regData;
		} else {
			ret = -1;
		}
		break;

	case REG_MessageChannelPort:
		pGC->MessagePort = (uint16_t)regData;
		pGEV->message.peer_addr.sin_port = htons((uint16_t)regData);
		break;

	case REG_MessageChannelDestination:
		if (0 != regData) {
			pGC->MessageAddress = regData;
			pGEV->message.peer_addr.sin_addr.s_addr = htonl(regData);
		} else {
			ret = -1;
		}
		break;

	case REG_MessageChannelTransmissionTimeout:
		pGC->MessageTimeout = regData;
		break;

	case REG_MessageChannelRetryCount:
		pGC->MessageRetryCount = regData;
		break;

	case REG_MessageChannelSourcePort:
		pGC->MessageSourcePort = regData;
		break;

	case REG_StreamChannelPort0:
		pGC->Channel[CAM_LEFT].Port = (uint16_t)regData;
		pGEV->stream[CAM_LEFT].peer_addr.sin_port = htons((uint16_t)regData);
		if (0 == pGC->Channel[CAM_LEFT].Port) {
			pGEV->stream[CAM_LEFT].grab_status = GRAB_STATUS_STOP;
		} else {
			csv_gvsp_cam_grab_thread(CAM_LEFT);
		}
		break;

	case REG_StreamChannelPacketSize0:
		if ((0 < (regData&0xFFFF))&&((regData&0xFFFF) < GVSP_PACKET_MAX_SIZE)) {
			pGC->Channel[CAM_LEFT].Cfg_PacketSize = regData;
			if (regData & SCPS_F) {
				// todo test
			}

		} else {
			ret = -1;
		}
		break;

	case REG_StreamChannelPacketDelay0:
		pGC->Channel[CAM_LEFT].PacketDelay = regData;
		break;

	case REG_StreamChannelDestinationAddress0:
		if (0 != regData) {
			pGC->Channel[CAM_LEFT].Address = regData;
			pGEV->stream[CAM_LEFT].peer_addr.sin_addr.s_addr = regData;
		} else {
			ret = -1;
		}
		break;

	case REG_StreamChannelConfiguration0:
		pGC->Channel[CAM_LEFT].Configuration = regData;
		break;

	case REG_StreamChannelPort1:
		pGC->Channel[CAM_RIGHT].Port = (uint16_t)regData;
		pGEV->stream[CAM_RIGHT].peer_addr.sin_port = htons((uint16_t)regData);
		if (0 == pGC->Channel[CAM_RIGHT].Port) {
			pGEV->stream[CAM_RIGHT].grab_status = GRAB_STATUS_STOP;
		} else {
			csv_gvsp_cam_grab_thread(CAM_RIGHT);
		}
		break;

	case REG_StreamChannelPacketSize1:
		if ((0 < (regData&0xFFFF))&&((regData&0xFFFF) < GVSP_PACKET_MAX_SIZE)) {
			pGC->Channel[CAM_RIGHT].Cfg_PacketSize = regData;
			if (regData & SCPS_F) {
				// todo test
			}

		} else {
			ret = -1;
		}
		break;

	case REG_StreamChannelPacketDelay1:
		pGC->Channel[CAM_RIGHT].PacketDelay = regData;
		break;

	case REG_StreamChannelDestinationAddress1:
		if (0 != regData) {
			pGC->Channel[CAM_RIGHT].Address = regData;
			pGEV->stream[CAM_RIGHT].peer_addr.sin_addr.s_addr = regData;
		} else {
			ret = -1;
		}
		break;

	case REG_StreamChannelConfiguration1:
		pGC->Channel[CAM_RIGHT].Configuration = regData;
		break;

	case REG_OtherStreamChannelsRegisters:
		break;

	// stream 2-3 to add more

	case REG_ActionGroupKey0:
		break;

	case REG_ActionGroupMask0:
		break;

	case REG_ActionGroupKey1:
		break;

	case REG_ActionGroupMask1:
		break;

	case REG_OtherActionSignalsRegisters:
		break;

	case REG_StartofManufacturerSpecificRegisterSpace:
		break;

	default:
		break;
	}

	return ret;
}

static int csv_gvcp_writereg_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{
	int i = 0, ret = -1;


	if (pHDR->nLength > GVCP_MAX_PAYLOAD_LEN) {
		log_warn("ERROR : too long regs.");
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_HEADER);
		return -1;
	}

	if ((pHDR->nLength % sizeof(WRITEREG_CMD_MSG) != 0)||(pGEV->rxlen != 8+pHDR->nLength)) {
		log_warn("ERROR : length not match.");
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_HEADER);
		return -1;
	}

	int nRegs = pHDR->nLength/sizeof(WRITEREG_CMD_MSG);

	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGEV->txbuf;
	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_WRITEREG_ACK);
	pAckHdr->nLength			= htons(sizeof(WRITEREG_ACK_MSG));
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	uint32_t reg_addr = 0;
	uint8_t type = GEV_REG_TYPE_NONE;
	int nIndex = nRegs;
	uint32_t reg_data = 0;
	char *desc = NULL;

	WRITEREG_CMD_MSG *pRegMsg = (WRITEREG_CMD_MSG *)(pGEV->rxbuf + sizeof(CMD_MSG_HEADER));
	for (i = 0; i < nRegs; i++) {
		reg_addr = ntohl(pRegMsg->nRegAddress);
		reg_data = ntohl(pRegMsg->nRegData);

		if (reg_addr % 4) {
			csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_ADDRESS);
			return -1;
		}

		type = csv_gev_reg_type_get(reg_addr, &desc);
		if ((GEV_REG_TYPE_NONE == type)||(GEV_REG_TYPE_MEM == type)) {
			csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_ADDRESS);
			return -1;
		}

		if (NULL != desc) {
			log_debug("WR : %s", desc);
		}

		ret = csv_gvcp_writereg_effective(reg_addr, reg_data);
		if (ret == -1) {
			nIndex = i;
			break;
		}

		ret = csv_gev_reg_value_set(reg_addr, reg_data);
		if (ret == -1) {
			nIndex = i;
			break;
		} else if (ret == -2) {
			csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_WRITE_PROTECT);
			return -1;
		}

		pRegMsg++;
	}

	WRITEREG_ACK_MSG *pAckMsg = (WRITEREG_ACK_MSG *)(pGEV->txbuf + sizeof(ACK_MSG_HEADER));
	pAckMsg->nReserved			= htons(0x0000);
	pAckMsg->nIndex				= htons(nIndex);

	if (ret == 0) {
		// TODO 1 save to xml file
	}

	pGEV->txlen = sizeof(ACK_MSG_HEADER) + sizeof(WRITEREG_ACK_MSG);

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_readmem_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{
	if (pHDR->nLength != sizeof(READMEM_CMD_MSG)) {
		log_warn("ERROR : readmem param length.");
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_PARAMETER);
		return -1;
	}

	if (pGEV->rxlen != 8+pHDR->nLength) {
		log_warn("ERROR : length not match.");
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_HEADER);
		return -1;
	}

	int ret = -1;
	char *info = NULL;
	uint16_t len_info = 0;
	uint8_t type = GEV_REG_TYPE_NONE;
	READMEM_CMD_MSG *pReadMem = (READMEM_CMD_MSG *)(pGEV->rxbuf + sizeof(CMD_MSG_HEADER));
	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGEV->txbuf;
	READMEM_ACK_MSG *pMemAck = (READMEM_ACK_MSG *)(pGEV->txbuf + sizeof(ACK_MSG_HEADER));
	uint32_t mem_addr = ntohl(pReadMem->nMemAddress);
	uint16_t mem_len = ntohs(pReadMem->nMemLen);
	struct gev_conf_t *pGC = &gCSV->cfg.gigecfg;
	char *desc = NULL;

	if (mem_addr % 4) {
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_ADDRESS);
		return -1;
	}

	if ((mem_len % 4)||(mem_len > GVCP_READ_MEM_MAX_LEN)) {
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_PARAMETER);
		return -1;
	}

	// branch 1 读取 xml 文件
	if ((mem_addr >= REG_StartOfXmlFile)&&(mem_addr < REG_StartOfXmlFile+GEV_XML_FILE_MAX_SIZE)) {
		if (NULL == pGC->xmlData) {
			csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_ACCESS_DENIED);
			return -1;
		}

		uint32_t offset_addr = mem_addr-REG_StartOfXmlFile;
		pMemAck->nMemAddress = pReadMem->nMemAddress;
		memset(pMemAck->chReadMemData, 0, GVCP_MAX_PAYLOAD_LEN);
		memcpy(pMemAck->chReadMemData, pGC->xmlData+offset_addr, mem_len);

		pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
		pAckHdr->nAckMsgValue		= htons(GEV_READMEM_ACK);
		pAckHdr->nLength			= htons(sizeof(uint32_t) + mem_len);
		pAckHdr->nAckId				= htons(pHDR->nReqId);

		pGEV->txlen = sizeof(ACK_MSG_HEADER) + sizeof(uint32_t) + mem_len;

		return csv_gvcp_sendto(pGEV);
	}

	// branch 2 读取常规 mem
	type = csv_gev_reg_type_get(mem_addr, &desc);
	if ((GEV_REG_TYPE_NONE == type)||(GEV_REG_TYPE_REG == type)) {
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_ADDRESS);
		return -1;
	}

	if (NULL != desc) {
		log_debug("MemR : %s", desc);
	}

	ret = csv_gev_mem_info_get(mem_addr, &info);
	if (ret == -1) {
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_PACKET_REMOVED_FROM_MEMORY);
		return -1;
	} else if (ret == -2) {
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_ADDRESS);
		return -1;
	}

	len_info = csv_gev_mem_info_get_length(mem_addr);
	if (mem_len > len_info) {
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_BAD_ALIGNMENT);
		return -1;
	}

	pMemAck->nMemAddress = pReadMem->nMemAddress;
	memset(pMemAck->chReadMemData, 0, GVCP_MAX_PAYLOAD_LEN);
	if (NULL != info) {
		if (strlen(info) < mem_len) {
			memcpy(pMemAck->chReadMemData, info, strlen(info));
		} else {
			memcpy(pMemAck->chReadMemData, info, mem_len);
		}
	}

	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_READMEM_ACK);
	pAckHdr->nLength			= htons(sizeof(uint32_t) + mem_len);
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	pGEV->txlen = sizeof(ACK_MSG_HEADER) + sizeof(uint32_t) + mem_len;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_writemem_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{
	if ((pHDR->nLength == 0)||(pHDR->nLength % 4)
	  ||(pHDR->nLength <= 4)||(pHDR->nLength > GVCP_MAX_PAYLOAD_LEN+4)) {
		log_warn("ERROR : writemem param length.");
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_PARAMETER);
		return -1;
	}

	if (pGEV->rxlen != 8+pHDR->nLength) {
		log_warn("ERROR : length not match.");
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_HEADER);
		return -1;
	}

	int ret = -1;
	uint8_t type = GEV_REG_TYPE_NONE;
	WRITEMEM_CMD_MSG *pWriteMem = (WRITEMEM_CMD_MSG *)(pGEV->rxbuf + sizeof(CMD_MSG_HEADER));
	uint32_t mem_addr = ntohl(pWriteMem->nMemAddress);
	uint16_t len_info = 0;
	char *info = (char *)pWriteMem->chWriteMemData;
	char *desc = NULL;

	if (mem_addr % 4) {
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_ADDRESS);
		return -1;
	}

	if (strlen(info) > GVCP_WRITE_MEM_MAX_LEN) {
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_PARAMETER);
		return -1;
	}

	type = csv_gev_reg_type_get(mem_addr, &desc);
	if ((GEV_REG_TYPE_NONE == type)||(GEV_REG_TYPE_REG == type)) {
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_INVALID_ADDRESS);
		return -1;
	}

	if (NULL != desc) {
		log_debug("MemW : %s", desc);
	}

	ret = csv_gev_mem_info_set(mem_addr, info);
	if (ret < 0) {
		csv_gvcp_error_ack(pGEV, pHDR, GEV_STATUS_WRITE_PROTECT);
		return -1;
	} else if (ret == 0) {
		len_info = pHDR->nLength - 4;
	} else {
		len_info = ret;
	}

	WRITEMEM_ACK_MSG *pMemAck = (WRITEMEM_ACK_MSG *)(pGEV->txbuf + sizeof(ACK_MSG_HEADER));
	pMemAck->nReserved = htons(0);
	pMemAck->nIndex = htons(len_info);

	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGEV->txbuf;
	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_WRITEMEM_ACK);
	pAckHdr->nLength			= htons(sizeof(WRITEMEM_ACK_MSG));
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	pGEV->txlen = sizeof(ACK_MSG_HEADER) + sizeof(WRITEMEM_ACK_MSG);

	return csv_gvcp_sendto(pGEV);
}

int csv_gvcp_eventdata_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{



	pGEV->txlen = sizeof(ACK_MSG_HEADER) + 0;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_action_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{



	pGEV->txlen = sizeof(ACK_MSG_HEADER) + 0;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_msg_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHdr)
{
	int ret = -1;

	memset(pGEV->txbuf, 0, GVCP_MAX_MSG_LEN);

	// TODO check where data from "from_addr.s_addr"

	switch (pHdr->nCommand) {
	case GEV_DISCOVERY_CMD:
		log_debug("%s", toSTR(GEV_DISCOVERY_CMD));
		ret = csv_gvcp_discover_ack(pGEV, pHdr);
		break;

	case GEV_FORCEIP_CMD:
		log_debug("%s", toSTR(GEV_FORCEIP_CMD));
		ret = csv_gvcp_forceip_ack(pGEV, pHdr);
		break;

	case GEV_PACKETRESEND_CMD:
		log_debug("%s", toSTR(GEV_PACKETRESEND_CMD));
		ret = csv_gvcp_packetresend_ack(pGEV, pHdr);
		break;

	case GEV_READREG_CMD:
		log_debug("%s", toSTR(GEV_READREG_CMD));
		ret = csv_gvcp_readreg_ack(pGEV, pHdr);
		break;

	case GEV_WRITEREG_CMD:
		log_debug("%s", toSTR(GEV_WRITEREG_CMD));
		ret = csv_gvcp_writereg_ack(pGEV, pHdr);
		break;

	case GEV_READMEM_CMD:
		log_debug("%s", toSTR(GEV_READMEM_CMD));
		ret = csv_gvcp_readmem_ack(pGEV, pHdr);
		break;

	case GEV_WRITEMEM_CMD:
		log_debug("%s", toSTR(GEV_WRITEMEM_CMD));
		ret = csv_gvcp_writemem_ack(pGEV, pHdr);
		break;

	case GEV_EVENT_ACK:
		//ret = csv_gvcp_event_ack(pGEV, pHdr);
		break;

	case GEV_EVENTDATA_ACK:
		//ret = csv_gvcp_eventdata_ack(pGEV, pHdr);
		break;

	case GEV_ACTION_CMD:
		log_debug("%s", toSTR(GEV_ACTION_CMD));
		ret = csv_gvcp_action_ack(pGEV, pHdr);
		break;

	default:
		log_debug("WARN : not support cmd 0x%04X.", pHdr->nCommand);
		ret = csv_gvcp_error_ack(pGEV, pHdr, GEV_STATUS_NOT_IMPLEMENTED);
		break;
	}

	memset(pGEV->rxbuf, 0, GVCP_MAX_MSG_LEN);

	return ret;
}

int csv_gvcp_trigger (struct csv_gev_t *pGEV)
{
	socklen_t from_len = sizeof(struct sockaddr_in);

	pGEV->rxlen = recvfrom(pGEV->fd, pGEV->rxbuf, GVCP_MAX_MSG_LEN, 0, 
		(struct sockaddr *)&pGEV->from_addr, &from_len);

	if (pGEV->rxlen < sizeof(CMD_MSG_HEADER)) {
		log_hex(STREAM_UDP, pGEV->rxbuf, pGEV->rxlen, "Wrong gev head length. '%s:%d'", 
			inet_ntoa(pGEV->from_addr.sin_addr), htons(pGEV->from_addr.sin_port));
		return -1;
	}

	log_hex(STREAM_UDP, pGEV->rxbuf, pGEV->rxlen, "gev recvfrom '%s:%d'", 
		inet_ntoa(pGEV->from_addr.sin_addr), htons(pGEV->from_addr.sin_port));

	CMD_MSG_HEADER *pHeader = (CMD_MSG_HEADER *)pGEV->rxbuf;
	CMD_MSG_HEADER Cmdheader;

	Cmdheader.cKeyValue = pHeader->cKeyValue;
	Cmdheader.cFlg = pHeader->cFlg;
	Cmdheader.nCommand = ntohs(pHeader->nCommand);
	Cmdheader.nLength = ntohs(pHeader->nLength);
	Cmdheader.nReqId = ntohs(pHeader->nReqId);

	if (GVCP_CMD_KEY_VALUE != pHeader->cKeyValue) {
		if ((GEV_STATUS_SUCCESS == (pHeader->cKeyValue<<8)+pHeader->cFlg)
		  &&(GEV_EVENT_ACK == Cmdheader.nCommand)) {
			log_info("OK : event ack id[%d].", Cmdheader.nReqId);
		}

		return -1;
	}

	return csv_gvcp_msg_ack(pGEV, &Cmdheader);
}



static int csv_gvcp_server_open (struct csv_gev_t *pGEV)
{
	int ret = -1;
	int fd = -1;
	struct sockaddr_in local_addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 ) {
		log_err("ERROR : socket %s.", pGEV->name);

		return -1;
	}

	memset((void*)&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(pGEV->port);
	bzero(&(local_addr.sin_zero), 8);

	ret = bind(fd, (struct sockaddr*)&local_addr, sizeof(struct sockaddr));
	if(ret < 0) {
		log_err("ERROR : bind %s.", pGEV->name);

		return -2;
	}

    ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (ret < 0) {
        log_err("ERROR : fcntl %s.", pGEV->name);
        if (close(fd)<0) {
            log_err("ERROR : close %s.", pGEV->name);
        }
        return ret;
    }

	pGEV->fd = fd;
	log_info("OK : bind %s : '%d/udp' as fd(%d).",  
		pGEV->name, pGEV->port, pGEV->fd);

	return 0;
}

static int csv_gvcp_server_close (struct csv_gev_t *pGEV)
{
	if (NULL == pGEV) {
		return -1;
	}

	if (pGEV->fd > 0) {
		if (close(pGEV->fd) < 0) {
			log_err("ERROR : close %s.", pGEV->name);
			return -1;
		}

		log_info("OK : close %s fd(%d).", pGEV->name, pGEV->fd);
		pGEV->fd = -1;
	}

	return 0;
}

static int csv_gvcp_message_open (struct gev_message_t *pMsg)
{
	int ret = 0;
	int fd = -1;

	if (pMsg->fd > 0) {
		ret = close(pMsg->fd);
		if (ret < 0) {
			log_err("ERROR : close %s.", pMsg->name);
		}
		pMsg->fd = -1;
	}

	struct sockaddr_in local_addr;
	socklen_t sin_size = sizeof(struct sockaddr);

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 ) {
		log_err("ERROR : socket %s.", pMsg->name);

		return -1;
	}

	memset((void*)&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(0);
	bzero(&(local_addr.sin_zero), 8);

	ret = bind(fd, (struct sockaddr*)&local_addr, sizeof(struct sockaddr));
	if(ret < 0) {
		log_err("ERROR : bind %s.", pMsg->name);

		return -1;
	}

	getsockname(fd, (struct sockaddr *)&local_addr, &sin_size);

	pMsg->fd = fd;
	pMsg->port = ntohs(local_addr.sin_port);

	log_info("OK : bind %s : '%d/udp' as fd(%d).", pMsg->name, 
		pMsg->port, pMsg->fd);

	return 0;
}

static int csv_gvcp_message_close (struct gev_message_t *pMsg)
{
	if (NULL == pMsg) {
		return -1;
	}

	if (pMsg->fd > 0) {
		if (close(pMsg->fd) < 0) {
			log_err("ERROR : close %s.", pMsg->name);
			return -1;
		}

		log_info("OK : close %s fd(%d).", pMsg->name, pMsg->fd);
		pMsg->fd = -1;
	}

	return 0;
}

static int csv_gvsp_cam_grab_fetch (struct gvsp_stream_t *pStream, 
	uint8_t *imgData, uint32_t imgLength, MV_FRAME_OUT_INFO_EX *imgInfo)
{
	struct stream_list_t *cur = NULL;
	struct image_info_t *pIMG = NULL;

	cur = (struct stream_list_t *)malloc(sizeof(struct stream_list_t));
	if (cur == NULL) {
		log_err("ERROR : malloc stream_list_t.");
		return -1;
	}
	memset(cur, 0, sizeof(struct stream_list_t));
	pIMG = &cur->img;

	pIMG->pixelformat = (uint32_t)imgInfo->enPixelType;
	pIMG->length = imgLength;
	pIMG->width = imgInfo->nWidth;
	pIMG->height = imgInfo->nHeight;
	pIMG->offset_x = imgInfo->nOffsetX;
	pIMG->offset_y = imgInfo->nOffsetY;
	pIMG->padding_x = 0;
	pIMG->padding_y = 0;

	if (imgLength > 0) {
		pIMG->payload = (uint8_t *)malloc(imgLength);
		if (NULL == pIMG->payload) {
			log_err("ERROR : malloc payload. [%d].", imgLength);
			return -1;
		}
	}

	pthread_mutex_lock(&pStream->mutex_stream);
	list_add_tail(&cur->list, &pStream->head_stream.list);
	pthread_mutex_unlock(&pStream->mutex_stream);

	pthread_cond_broadcast(&pStream->cond_stream);

	return 0;
}

static void *csv_gvsp_cam_grab_loop (void *pData)
{
	if (NULL == pData) {
		goto exit_thr;
	}

	struct gvsp_stream_t *pStream = (struct gvsp_stream_t *)pData;

	if (pStream->idx >= TOTAL_CAMS) {
		goto exit_thr;
	}

	int nRet = MV_OK;
	struct csv_mvs_t *pMVS = &gCSV->mvs;
	struct cam_spec_t *pCAM = &pMVS->Cam[pStream->idx];

	if (pCAM->grabbing) {
		goto exit_thr;
	}

	if ((!pCAM->opened)||(NULL == pCAM->pHandle)) {
		goto exit_thr;
	}

	nRet = MV_CC_SetEnumValue(pCAM->pHandle, "TriggerMode", MV_TRIGGER_MODE_OFF);
	if (MV_OK != nRet) {
		log_warn("ERROR : SetEnumValue 'TriggerMode' errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
	}

	nRet = MV_CC_StartGrabbing(pCAM->pHandle);
	if (MV_OK != nRet) {
		log_warn("ERROR : StartGrabbing errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
		goto exit_thr;
	}

	pCAM->grabbing = true;
	pStream->grab_status = GRAB_STATUS_RUNNING;
	pStream->block_id64 = 1;

	while (1) {
		if (pStream->grab_status != GRAB_STATUS_RUNNING) {
			break;
		}

		memset(&pCAM->imgInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
		nRet = MV_CC_GetOneFrameTimeout(pCAM->pHandle, pCAM->imgData, 
			pCAM->sizePayload.nCurValue, &pCAM->imgInfo, 3000);
		if (nRet != MV_OK) {
			log_warn("ERROR : CAM '%s' : GetOneFrameTimeout, errcode[0x%08X]:'%s'.", 
				pCAM->sn, nRet, strMsg(nRet));
			continue;
		}
		log_debug("OK : CAM '%s' : GetOneFrame[%d] %d x %d", pCAM->sn, 
			pCAM->imgInfo.nFrameNum, pCAM->imgInfo.nWidth, pCAM->imgInfo.nHeight);

		csv_gvsp_cam_grab_fetch(pStream, pCAM->imgData, pCAM->sizePayload.nCurValue, &pCAM->imgInfo);
	}

	nRet = MV_CC_StopGrabbing(pCAM->pHandle);
	if (MV_OK != nRet) {
		log_warn("ERROR : CAM '%s' StopGrabbing errcode[0x%08X]:'%s'.", 
				pCAM->sn, nRet, strMsg(nRet));
	}

	nRet = MV_CC_SetEnumValue(pCAM->pHandle, "TriggerMode", MV_TRIGGER_MODE_ON);
	if (MV_OK != nRet) {
		log_warn("ERROR : SetEnumValue 'TriggerMode' errcode[0x%08X]:'%s'.", nRet, strMsg(nRet));
	}

exit_thr:

	pCAM->grabbing = false;
	pStream->thr_grab = 0;
	pStream->grab_status = GRAB_STATUS_STOP;

	log_info("OK : exit pthread '%s'", pStream->name_egvgrab);

	pthread_exit(NULL);

	return NULL;
}

int csv_gvsp_cam_grab_thread (uint8_t idx)
{
	int ret = -1;

	if (idx >= TOTAL_CAMS) {
		return -1;
	}

	struct gvsp_stream_t *pStream = &gCSV->gev.stream[idx];

	if (GRAB_STATUS_RUNNING == pStream->grab_status) {
		return 0;
	}

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&pStream->thr_grab, &attr, csv_gvsp_cam_grab_loop, (void *)pStream);
	if (ret < 0) {
		log_err("ERROR : create pthread '%s'.", pStream->name_egvgrab);
		pStream->grab_status = GRAB_STATUS_NONE;
		return -1;
	} else {
		log_info("OK : create pthread '%s' @ (%p).", pStream->name_egvgrab, pStream->thr_grab);
	}

	return ret;
}


int csv_gvsp_sendto (int fd, struct sockaddr_in *peer, uint8_t *txbuf, uint32_t txlen)
{
	socklen_t size_len = sizeof(struct sockaddr_in);

	return sendto(fd, txbuf, txlen, 0, (struct sockaddr *)&peer, size_len);
}

static int csv_gvsp_packet_leader (struct gvsp_stream_t *pStream, 
	struct image_info_t *pIMG)
{
    GVSP_PACKET_HEADER *pHdr = (GVSP_PACKET_HEADER *)pStream->bufSend;
    pHdr->status			= htons(GEV_STATUS_SUCCESS);
	pHdr->blockid_flag		= htons(0);	// resend flag ~bit15
    pHdr->packet_format		= htonl((1<<31)|(GVSP_PACKET_FMT_LEADER<<24)); // EI=1 & Leader
	pHdr->block_id_high		= htonl((pStream->block_id64>>32)&0xFFFFFFFF);
	pHdr->block_id_low		= htonl(pStream->block_id64&0xFFFFFFFF);
	pHdr->packet_id			= htonl(pStream->packet_id32);

    GVSP_IMAGE_DATA_LEADER* pDataLeader = (GVSP_IMAGE_DATA_LEADER*)(pStream->bufSend + sizeof(GVSP_PACKET_HEADER));
    pDataLeader->reserved       = 0;
    pDataLeader->payload_type   = htons(GVSP_PT_UNCOMPRESSED_IMAGE);
    pDataLeader->timestamp_high = htonl(0);  // TODO
    pDataLeader->timestamp_low  = htonl(0);
    pDataLeader->pixel_format   = htonl(PixelType_Gvsp_Mono8);
    pDataLeader->size_x         = htonl(pIMG->width);
    pDataLeader->size_y         = htonl(pIMG->height);
    pDataLeader->offset_x       = htonl(0);
    pDataLeader->offset_y       = htonl(0);
    pDataLeader->padding_x      = htons(0);
    pDataLeader->padding_y      = htons(0);

    pStream->lenSend = sizeof(GVSP_PACKET_HEADER) + sizeof(GVSP_IMAGE_DATA_LEADER);

	return csv_gvsp_sendto(pStream->fd, &pStream->peer_addr, pStream->bufSend, pStream->lenSend);
}

static int csv_gvsp_packet_payload (struct gvsp_stream_t *pStream, 
	uint8_t *pData, uint32_t length)
{
    GVSP_PACKET_HEADER *pHdr = (GVSP_PACKET_HEADER *)pStream->bufSend;
    pHdr->status			= htons(GEV_STATUS_SUCCESS);
	pHdr->blockid_flag		= htons(0);	// resend flag ~bit15
    pHdr->packet_format		= htonl((1<<31)|(GVSP_PACKET_FMT_LEADER<<24)); // EI=1 & Leader
	pHdr->block_id_high		= htonl((pStream->block_id64>>32)&0xFFFFFFFF);
	pHdr->block_id_low		= htonl(pStream->block_id64&0xFFFFFFFF);
	pHdr->packet_id			= htonl(pStream->packet_id32);

	memcpy(pStream->bufSend+sizeof(GVSP_PACKET_HEADER), pData, length);

    pStream->lenSend = sizeof(GVSP_PACKET_HEADER) + length;

	return csv_gvsp_sendto(pStream->fd, &pStream->peer_addr, pStream->bufSend, pStream->lenSend);
}

int csv_gvsp_packet_trailer (struct gvsp_stream_t *pStream, 
	struct image_info_t *pIMG)
{
    GVSP_PACKET_HEADER *pHdr = (GVSP_PACKET_HEADER *)pStream->bufSend;
    pHdr->status			= htons(GEV_STATUS_SUCCESS);
	pHdr->blockid_flag		= htons(0);	// resend flag ~bit15
    pHdr->packet_format		= htonl((1<<31)|(GVSP_PACKET_FMT_LEADER<<24)); // EI=1 & Leader
	pHdr->block_id_high		= htonl((pStream->block_id64>>32)&0xFFFFFFFF);
	pHdr->block_id_low		= htonl(pStream->block_id64&0xFFFFFFFF);
	pHdr->packet_id			= htonl(pStream->packet_id32);

    GVSPIMAGEDATATRAILER *pDataTrailer = (GVSPIMAGEDATATRAILER*)(pStream->bufSend + sizeof(GVSP_PACKET_HEADER));
    pDataTrailer->reserved     = htons(0);
    pDataTrailer->payload_type = htons(GVSP_PT_UNCOMPRESSED_IMAGE);
    pDataTrailer->size_y       = htonl(pIMG->height);

    pStream->lenSend = sizeof(GVSP_PACKET_HEADER) + sizeof(GVSPIMAGEDATATRAILER);

	return csv_gvsp_sendto(pStream->fd, &pStream->peer_addr, pStream->bufSend, pStream->lenSend);
}

static int csv_gvsp_image_dispatch (struct gvsp_stream_t *pStream, 
	struct image_info_t *pIMG)
{
	if ((NULL == pStream)||(NULL == pIMG)) {
		return -1;
	}

	int ret = 0;
	struct channel_cfg_t *pCH = &gCSV->cfg.gigecfg.Channel[pStream->idx];
	uint32_t packsize = (pCH->Cfg_PacketSize&0xFFFF) - 28 - sizeof(GVSP_PACKET_HEADER);
	uint8_t *pData = pIMG->payload;

	pStream->packet_id32 = 0;
	ret = csv_gvsp_packet_leader(pStream, pIMG);

	for (pData = pIMG->payload; pData < pIMG->payload+pIMG->length; ) {
		pStream->packet_id32++;
		ret = csv_gvsp_packet_payload(pStream, pData, 
			(pData+packsize < pIMG->payload+pIMG->length) ? packsize : (pIMG->length%packsize));
		pData += packsize;

		if (pCH->PacketDelay/1000) {
			usleep(pCH->PacketDelay/1000);
		}

		if (pStream->grab_status != GRAB_STATUS_RUNNING) {
			return -1;
		}
	}

	pStream->packet_id32++;
	ret = csv_gvsp_packet_trailer(pStream, pIMG);

	return ret;
}

static int csv_gvsp_client_open (struct gvsp_stream_t *pStream)
{
	int ret = 0;
	int fd = -1;

	if (pStream->fd > 0) {
		ret = close(pStream->fd);
		if (ret < 0) {
			log_err("ERROR : close '%s'.", pStream->name);
		}
		pStream->fd = -1;
	}

	struct sockaddr_in local_addr;
	socklen_t sin_size = sizeof(struct sockaddr);

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 ) {
		log_err("ERROR : socket %s.", pStream->name);

		return -1;
	}

	memset((void*)&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(0);
	bzero(&(local_addr.sin_zero), 8);

	ret = bind(fd, (struct sockaddr*)&local_addr, sizeof(struct sockaddr));
	if(ret < 0) {
		log_err("ERROR : bind %s.", pStream->name);

		return -1;
	}

	int val = IP_PMTUDISC_DO; /* don't fragment */
	ret = setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));
	if (ret < 0) {
		log_err("ERROR : setsockopt 'IP_MTU_DISCOVER'.");

		return -1;
	}

	int send_len = (64<<10);	// 64K
	ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &send_len, sizeof(send_len));
	if (ret < 0) {
		log_err("ERROR : setsockopt 'SO_SNDBUF'.");

		return -1;
	}

	getsockname(fd, (struct sockaddr *)&local_addr, &sin_size);

	pStream->fd = fd;
	pStream->port = ntohs(local_addr.sin_port);

	log_info("OK : bind '%s' : '%d/udp' as fd(%d).", pStream->name, 
		pStream->port, pStream->fd);

	return 0;
}

static int csv_gvsp_client_close (struct gvsp_stream_t *pStream)
{
	if (NULL == pStream) {
		return -1;
	}

	if (pStream->fd > 0) {
		if (close(pStream->fd) < 0) {
			log_err("ERROR : close '%s'.", pStream->name);
			return -1;
		}
		pStream->fd = -1;
	}

	return 0;
}

static void *csv_gvsp_client_loop (void *data)
{
	if (NULL == data) {
		goto exit_thr;
	}

	struct gvsp_stream_t *pStream = (struct gvsp_stream_t *)data;

	int ret = 0;

	ret = csv_gvsp_client_open(pStream);
	if (ret < 0) {
		goto exit_thr;
	}

	csv_gev_reg_value_update(REG_StreamChannelSourcePort0+0x40*pStream->idx, pStream->port);
	gCSV->cfg.gigecfg.Channel[pStream->idx].SourcePort = pStream->port;

	struct timeval now;
	struct timespec timeo;
	struct list_head *pos = NULL, *n = NULL;
	struct stream_list_t *task = NULL;
	struct image_info_t *pIMG = NULL;

	while (1) {
		list_for_each_safe(pos, n, &pStream->head_stream.list) {
			task = list_entry(pos, struct stream_list_t, list);
			if (task == NULL) {
				break;
			}

			pIMG = &task->img;

			if (pStream->grab_status == GRAB_STATUS_RUNNING) {
				csv_gvsp_image_dispatch(pStream, pIMG);
				pStream->block_id64++;
			}

			if (NULL != pIMG->payload) {
				free(pIMG->payload);
				pIMG->payload = NULL;
			}

			list_del(&task->list);
			free(task);
			task = NULL;
		}


		gettimeofday(&now, NULL);
		timeo.tv_sec = now.tv_sec + 5;
		timeo.tv_nsec = now.tv_usec * 1000;

		ret = pthread_cond_timedwait(&pStream->cond_stream, &pStream->mutex_stream, &timeo);
		if (ret == ETIMEDOUT) {
			// use timeo as a block and than retry.
		}
	}

exit_thr:

	log_warn("WARN : exit pthread %s.", pStream->name_stream);

	pStream->thr_stream = 0;
	csv_gvsp_client_close(pStream);

	pthread_exit(NULL);

	return NULL;
}

int csv_gvsp_client_thread (struct gvsp_stream_t *pStream)
{
	int ret = -1;

	if (NULL == pStream) {
		return -1;
	}

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pStream->mutex_stream, NULL) != 0) {
		log_err("ERROR : mutex '%s'.", pStream->name_stream);
        return -1;
    }

    if (pthread_cond_init(&pStream->cond_stream, NULL) != 0) {
		log_err("ERROR : cond '%s'.", pStream->name_stream);
        return -1;
    }

	ret = pthread_create(&pStream->thr_stream, &attr, csv_gvsp_client_loop, (void *)pStream);
	if (ret < 0) {
		log_err("ERROR : create pthread '%s'.", pStream->name_stream);
		return -1;
	} else {
		log_info("OK : create pthread '%s' @ (%p).", pStream->name_stream, pStream->thr_stream);
	}

	return ret;
}

static int csv_gvsp_client_thread_cancel (struct gvsp_stream_t *pStream)
{
	int ret = 0;
	void *retval = NULL;

	if (pStream->thr_stream <= 0) {
		return 0;
	}

	csv_gvsp_client_close(pStream);

	ret = pthread_cancel(pStream->thr_stream);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel '%s'.", pStream->name_stream);
	} else {
		log_info("OK : cancel pthread '%s' (%p).", pStream->name_stream, pStream->thr_stream);
	}

	ret = pthread_join(pStream->thr_stream, &retval);

	pStream->thr_stream = 0;

	return ret;
}


int csv_gev_init (void)
{
	int ret = 0, i = 0;
	struct csv_gev_t *pGEV = &gCSV->gev;
	struct gvsp_stream_t *pStream = NULL;
	struct gev_message_t *pMsg = &pGEV->message;

	pGEV->fd = -1;
	pGEV->name = NAME_UDP_GVCP;
	pGEV->port = GVCP_PUBLIC_PORT;
	pGEV->ReqId = GVCP_REQ_ID_INIT;
	pGEV->rxlen = 0;
	pGEV->txlen = 0;

	INIT_LIST_HEAD(&pGEV->head_reg.list);

	csv_gev_reg_enroll();

	pGEV->stream[CAM_LEFT].name = "stream_"toSTR(CAM_LEFT);
	pGEV->stream[CAM_LEFT].name_stream = "thr_"toSTR(CAM_LEFT);
	pGEV->stream[CAM_LEFT].name_egvgrab = "grab_"toSTR(CAM_LEFT);
	pGEV->stream[CAM_RIGHT].name = "stream_"toSTR(CAM_RIGHT);
	pGEV->stream[CAM_RIGHT].name_stream = "thr_"toSTR(CAM_RIGHT);
	pGEV->stream[CAM_RIGHT].name_egvgrab = "grab_"toSTR(CAM_RIGHT);
	for (i = 0; i < TOTAL_CAMS; i++) {
		pStream = &pGEV->stream[i];
		pStream->idx = i;
		pStream->fd = -1;
		pStream->grab_status = GRAB_STATUS_NONE;
		pStream->block_id64 = 1;
		pStream->packet_id32 = 0;
		INIT_LIST_HEAD(&pStream->head_stream.list);
		ret |= csv_gvsp_client_thread(pStream);
	}

	pMsg->fd = -1;
	pMsg->name = NAME_GEV_MSG;
	ret |= csv_gvcp_message_open(pMsg);
	csv_gev_reg_value_update(REG_MessageChannelSourcePort, pMsg->port);

	ret |= csv_gvcp_server_open(pGEV);

	return ret;
}

int csv_gev_deinit (void)
{
	int ret = 0, i = 0;
	struct csv_gev_t *pGEV = &gCSV->gev;
	struct gvsp_stream_t *pStream = NULL;

	csv_gev_reg_disroll();

	for (i = 0; i < TOTAL_CAMS; i++) {
		pStream = &pGEV->stream[i];
		ret |= csv_gvsp_client_thread_cancel(pStream);
	}

	ret |= csv_gvcp_message_close(&pGEV->message);
	ret |= csv_gvcp_server_close(pGEV);

	return ret;
}


#ifdef __cplusplus
}
#endif

