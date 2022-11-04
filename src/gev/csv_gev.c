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

uint32_t csv_gev_reg_value_get (uint32_t addr, uint32_t *value)
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
				*value = csv_gvcp_readreg_realtime(addr, pRI->value);
				ret = 0;
			} else {
				ret = -2;
			}

			break;
		}
	}

	return ret;
}

int csv_gev_reg_value_set (uint32_t addr, uint32_t value)
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

uint16_t csv_gev_mem_info_get_length (uint32_t addr)
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

int csv_gev_mem_info_get (uint32_t addr, char **info)
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

uint8_t csv_gev_reg_type_get (uint32_t addr, char **desc)
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

static void csv_gev_reg_general_enroll (void)
{
	struct gev_conf_t *pGC = &gCSV->cfg.gigecfg;

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
		4, swap32(pGC->CurrentIPAddress0), NULL, toSTR(REG_CurrentIPAddress0));
	csv_gev_reg_add(REG_CurrentSubnetMask0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, swap32(pGC->CurrentSubnetMask0), NULL, toSTR(REG_CurrentSubnetMask0));
	csv_gev_reg_add(REG_CurrentDefaultGateway0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, swap32(pGC->CurrentDefaultGateway0), NULL, toSTR(REG_CurrentDefaultGateway0));

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
		4, swap32(pGC->CurrentIPAddress0), NULL, toSTR(REG_PersistentIPAddress));
	csv_gev_reg_add(REG_PersistentSubnetMask0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, swap32(pGC->CurrentSubnetMask0), NULL, toSTR(REG_PersistentSubnetMask0));
	csv_gev_reg_add(REG_PersistentDefaultGateway0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, swap32(pGC->CurrentDefaultGateway0), NULL, toSTR(REG_PersistentDefaultGateway0));
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

	// left channel stream
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

	// right channel stream
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

	// depth channel stream
	csv_gev_reg_add(REG_StreamChannelPort2, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_DEPTH].Port, NULL, toSTR(REG_StreamChannelPort2));
	csv_gev_reg_add(REG_StreamChannelPacketSize2, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_DEPTH].Cfg_PacketSize, NULL, toSTR(REG_StreamChannelPacketSize2));
	csv_gev_reg_add(REG_StreamChannelPacketDelay2, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_DEPTH].PacketDelay, NULL, toSTR(REG_StreamChannelPacketDelay2));
	csv_gev_reg_add(REG_StreamChannelDestinationAddress2, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_DEPTH].Address, NULL, toSTR(REG_StreamChannelDestinationAddress2));
	csv_gev_reg_add(REG_StreamChannelSourcePort2, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel[CAM_DEPTH].SourcePort, NULL, toSTR(REG_StreamChannelSourcePort2));
	csv_gev_reg_add(REG_StreamChannelCapability2, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel[CAM_DEPTH].Capability, NULL, toSTR(REG_StreamChannelCapability2));
	csv_gev_reg_add(REG_StreamChannelConfiguration2, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel[CAM_DEPTH].Configuration, NULL, toSTR(REG_StreamChannelConfiguration2));
	csv_gev_reg_add(REG_StreamChannelZone2, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel[CAM_DEPTH].Zone, NULL, toSTR(REG_StreamChannelZone2));
	csv_gev_reg_add(REG_StreamChannelZoneDirection2, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel[CAM_DEPTH].ZoneDirection, NULL, toSTR(REG_StreamChannelZoneDirection2));

	//  add more stream

	csv_gev_reg_add(REG_ManifestTable, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, toSTR(REG_ManifestTable));

	csv_gev_reg_add(REG_ActionGroupKey0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, toSTR(REG_ActionGroupKey0));
	csv_gev_reg_add(REG_ActionGroupMask0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, toSTR(REG_ActionGroupMask0));

	csv_gev_reg_add(REG_StartofManufacturerSpecificRegisterSpace, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, toSTR(REG_StartofManufacturerSpecificRegisterSpace));

}

// from genicam xml
static void csv_gev_reg_custom_inq_enroll (void)
{
	struct gev_conf_t *pGC = &gCSV->cfg.gigecfg;

	csv_gev_reg_add(REG_Category_Inq, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, CounterAndTimerControl, NULL, toSTR(REG_Category_Inq));
	csv_gev_reg_add(REG_DeviceControlInq, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_DeviceControlInq));
	csv_gev_reg_add(REG_GevSCPSMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, GVSP_PACKET_MAX_SIZE, NULL, toSTR(REG_GevSCPSMaxInq));
	csv_gev_reg_add(REG_GevSCPSMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, GVSP_PACKET_MIN_SIZE, NULL, toSTR(REG_GevSCPSMinInq));
	csv_gev_reg_add(REG_GevSCPDMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 10000, NULL, toSTR(REG_GevSCPDMaxInq));
	csv_gev_reg_add(REG_GevSCPDMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_GevSCPDMinInq));
	csv_gev_reg_add(REG_ReverseInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_ReverseInq));
	csv_gev_reg_add(REG_WidthMin, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, DEFAULT_WIDTH, NULL, toSTR(REG_WidthMin));
	csv_gev_reg_add(REG_HeightMin, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, DEFAULT_HEIGHT, NULL, toSTR(REG_HeightMin));
	csv_gev_reg_add(REG_BinningMax, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_BinningMax));
	csv_gev_reg_add(REG_Hunit, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 1, NULL, toSTR(REG_Hunit));
	csv_gev_reg_add(REG_Vunit, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 1, NULL, toSTR(REG_Vunit));
	csv_gev_reg_add(REG_RegionSelectorInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_RegionSelectorInq));
	csv_gev_reg_add(REG_VideoModeInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_VideoModeInq));
	csv_gev_reg_add(REG_PixelFormatInq0, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, PFI_Mono8, NULL, toSTR(REG_PixelFormatInq0));
	csv_gev_reg_add(REG_PixelFormatInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_PixelFormatInq));
	csv_gev_reg_add(REG_TestPatternInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_TestPatternInq));
	csv_gev_reg_add(REG_FrameSpecInfoInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_FrameSpecInfoInq));
	csv_gev_reg_add(REG_FrameRateControlInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0xF8000000, NULL, toSTR(REG_FrameRateControlInq));
	csv_gev_reg_add(REG_AutoExposureInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_AutoExposureInq));
	csv_gev_reg_add(REG_ExposureTimeMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 1000000, NULL, toSTR(REG_ExposureTimeMaxInq));
	csv_gev_reg_add(REG_ExposureTimeMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 20, NULL, toSTR(REG_ExposureTimeMinInq));
	csv_gev_reg_add(REG_TriggerInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_TriggerInq));
	csv_gev_reg_add(REG_TriggerSelectInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_TriggerSelectInq));
	csv_gev_reg_add(REG_TriggerDelayAbsMax, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 10000, NULL, toSTR(REG_TriggerDelayAbsMax));
	csv_gev_reg_add(REG_TriggerDelayAbsMin, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_TriggerDelayAbsMin));
	csv_gev_reg_add(REG_AcqFrameRateMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 125, NULL, toSTR(REG_AcqFrameRateMaxInq));
	csv_gev_reg_add(REG_AcqFrameRateMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_AcqFrameRateMinInq));
	csv_gev_reg_add(REG_AcqFrameCountMaxInq, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 125, NULL, toSTR(REG_AcqFrameCountMaxInq));
	csv_gev_reg_add(REG_AcqFrameCountMinInq, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 1, NULL, toSTR(REG_AcqFrameCountMinInq));
	csv_gev_reg_add(REG_AcqBurstFrameCountMaxInq, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 125, NULL, toSTR(REG_AcqBurstFrameCountMaxInq));
	csv_gev_reg_add(REG_AcqBurstFrameCountMinInq, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 1, NULL, toSTR(REG_AcqBurstFrameCountMinInq));
	csv_gev_reg_add(REG_DigitalIOControlInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_DigitalIOControlInq));
	csv_gev_reg_add(REG_LineModeInputInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_LineModeInputInq));
	csv_gev_reg_add(REG_LineModeOutputInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_LineModeOutputInq));
	csv_gev_reg_add(REG_LineStrobeInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_LineStrobeInq));
	csv_gev_reg_add(REG_LineDebouncerTimeMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_LineDebouncerTimeMinInq));
	csv_gev_reg_add(REG_LineDebouncerTimeMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_LineDebouncerTimeMaxInq));
	csv_gev_reg_add(REG_LineDebouncerTimeUnitInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_LineDebouncerTimeUnitInq));
	csv_gev_reg_add(REG_StrobeLineDurationMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_StrobeLineDurationMinInq));
	csv_gev_reg_add(REG_StrobeLineDurationMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_StrobeLineDurationMaxInq));
	csv_gev_reg_add(REG_StrobeLineDurationUnitInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_StrobeLineDurationUnitInq));
	csv_gev_reg_add(REG_StrobeLineDelayMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_StrobeLineDelayMinInq));
	csv_gev_reg_add(REG_StrobeLineDelayMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_StrobeLineDelayMaxInq));
	csv_gev_reg_add(REG_StrobeLineDelayUnitInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_StrobeLineDelayUnitInq));
	csv_gev_reg_add(REG_StrobeLinePreDelayMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_StrobeLinePreDelayMinInq));
	csv_gev_reg_add(REG_StrobeLinePreDelayMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_StrobeLinePreDelayMaxInq));
	csv_gev_reg_add(REG_StrobeLinePreDelayUnitInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_StrobeLinePreDelayUnitInq));
	csv_gev_reg_add(REG_HueInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_HueInq));
	csv_gev_reg_add(REG_HueAbsMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_HueAbsMinInq));
	csv_gev_reg_add(REG_HueAbsMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_HueAbsMaxInq));
	csv_gev_reg_add(REG_GainInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_GainInq));

	csv_gev_reg_add(REG_GainAbsMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_GainAbsMinInq));
	csv_gev_reg_add(REG_GainAbsMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 10, NULL, toSTR(REG_GainAbsMaxInq));
	csv_gev_reg_add(REG_BlackLevelInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_BlackLevelInq));
	csv_gev_reg_add(REG_BlackLevelMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_BlackLevelMinInq));
	csv_gev_reg_add(REG_BlackLevelMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_BlackLevelMaxInq));
	csv_gev_reg_add(REG_BalanceWhiteAutoInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_BalanceWhiteAutoInq));
	csv_gev_reg_add(REG_BalanceRatioSelectorInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_BalanceRatioSelectorInq));
	csv_gev_reg_add(REG_BalanceRatioMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_BalanceRatioMinInq));
	csv_gev_reg_add(REG_BalanceRatioMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_BalanceRatioMaxInq));
	csv_gev_reg_add(REG_SharpnessInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_SharpnessInq));
	csv_gev_reg_add(REG_SharpnessMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_SharpnessMinInq));
	csv_gev_reg_add(REG_SharpnessMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_SharpnessMaxInq));
	csv_gev_reg_add(REG_SaturationInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_SaturationInq));
	csv_gev_reg_add(REG_SaturationAbsMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_SaturationAbsMinInq));
	csv_gev_reg_add(REG_SaturationAbsMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_SaturationAbsMaxInq));
	csv_gev_reg_add(REG_GammaInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_GammaInq));
	csv_gev_reg_add(REG_GammaAbsMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_GammaAbsMinInq));
	csv_gev_reg_add(REG_GammaAbsMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_GammaAbsMaxInq));
	csv_gev_reg_add(REG_DigitalShiftInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_DigitalShiftInq));


	csv_gev_reg_add(REG_BrightnessInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_BrightnessInq));
	csv_gev_reg_add(REG_BrightnessMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_BrightnessMinInq));
	csv_gev_reg_add(REG_BrightnessMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_BrightnessMaxInq));
	csv_gev_reg_add(REG_BrightnessUintInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_BrightnessUintInq));

	csv_gev_reg_add(REG_AutoAOIInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_AutoAOIInq));
	csv_gev_reg_add(REG_AutoLimitInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_AutoLimitInq));

	csv_gev_reg_add(REG_AutoExposureTimeLowerLimitMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 20, NULL, toSTR(REG_AutoExposureTimeLowerLimitMinInq));
	csv_gev_reg_add(REG_AutoExposureTimeLowerLimitMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 1000000, NULL, toSTR(REG_AutoExposureTimeLowerLimitMaxInq));
	csv_gev_reg_add(REG_AutoExposureTimeLowerLimitIncInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 1, NULL, toSTR(REG_AutoExposureTimeLowerLimitIncInq));
	csv_gev_reg_add(REG_AutoExposureTimeUpperLimitMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 20, NULL, toSTR(REG_AutoExposureTimeUpperLimitMinInq));
	csv_gev_reg_add(REG_AutoExposureTimeUpperLimitMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 1000000, NULL, toSTR(REG_AutoExposureTimeUpperLimitMaxInq));
	csv_gev_reg_add(REG_AutoExposureTimeUpperLimitIncInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 1, NULL, toSTR(REG_AutoExposureTimeUpperLimitIncInq));

	csv_gev_reg_add(REG_LUTControlInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_LUTControlInq));
	csv_gev_reg_add(REG_LUTIndexMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_LUTIndexMinInq));
	csv_gev_reg_add(REG_LUTIndexMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_LUTIndexMaxInq));
	csv_gev_reg_add(REG_LUTIndexIncInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_LUTIndexIncInq));
	csv_gev_reg_add(REG_LUTValueMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_LUTValueMinInq));
	csv_gev_reg_add(REG_LUTValueMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_LUTValueMaxInq));
	csv_gev_reg_add(REG_LUTValueIncInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_LUTValueIncInq));

	csv_gev_reg_add(REG_UserSetInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, pGC->DeviceVersion, toSTR(REG_UserSetInq));
	csv_gev_reg_add(REG_AutoCorrectionInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, pGC->DeviceVersion, toSTR(REG_AutoCorrectionInq));
	csv_gev_reg_add(REG_CounterAndTimerInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, pGC->DeviceVersion, toSTR(REG_CounterAndTimerInq));
	csv_gev_reg_add(REG_CounterSelectorInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, pGC->DeviceVersion, toSTR(REG_CounterSelectorInq));
	csv_gev_reg_add(REG_CounterValueMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, pGC->DeviceVersion, toSTR(REG_CounterValueMinInq));
	csv_gev_reg_add(REG_CounterValueMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 100000, pGC->DeviceVersion, toSTR(REG_CounterValueMaxInq));
	csv_gev_reg_add(REG_CounterValueIncInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 1, pGC->DeviceVersion, toSTR(REG_CounterValueIncInq));

}

static void csv_gev_reg_genicam_enroll (void)
{
	struct gev_conf_t *pGC = &gCSV->cfg.gigecfg;

	csv_gev_reg_add(REG_DeviceScanType, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, 0, toSTR(REG_DeviceScanType));
	csv_gev_reg_add(REG_DeviceFirmwareVersion, GEV_REG_TYPE_MEM, GEV_REG_READ,
		4, 0, pGC->DeviceVersion, toSTR(REG_DeviceFirmwareVersion));
	csv_gev_reg_add(REG_DeviceUptime, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_DeviceUptime));
	csv_gev_reg_add(REG_BoardDeviceType, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_BoardDeviceType));
	csv_gev_reg_add(REG_DeviceMaxThroughput, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 1000000000, NULL, toSTR(REG_DeviceMaxThroughput));
	csv_gev_reg_add(REG_DeviceConnectionSpeed, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 1000, NULL, toSTR(REG_DeviceConnectionSpeed));
	csv_gev_reg_add(REG_DeviceConnectionStatus, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_DeviceConnectionStatus));
	csv_gev_reg_add(REG_DeviceLinkThroughputLimitMode, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_DeviceLinkThroughputLimitMode));
	csv_gev_reg_add(REG_DeviceLinkThroughputLimit, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 1000, NULL, toSTR(REG_DeviceLinkThroughputLimit));
	csv_gev_reg_add(REG_DeviceLinkConnectionCount, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 1, NULL, toSTR(REG_DeviceLinkConnectionCount));
	csv_gev_reg_add(REG_DeviceLinkHeartbeatMode, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_DeviceLinkHeartbeatMode));
	csv_gev_reg_add(REG_DeviceLinkHeartbeatTimeout, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 5000, NULL, toSTR(REG_DeviceLinkHeartbeatTimeout));
	csv_gev_reg_add(REG_DeviceCommandTimeout, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 1000, NULL, toSTR(REG_DeviceCommandTimeout));
	csv_gev_reg_add(REG_DeviceReset, GEV_REG_TYPE_REG, GEV_REG_WRITE,
		4, 0, NULL, toSTR(REG_DeviceReset));
	csv_gev_reg_add(REG_DeviceTemperature, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_DeviceTemperature));
	csv_gev_reg_add(REG_FindMe, GEV_REG_TYPE_REG, GEV_REG_WRITE,
		4, 0, NULL, toSTR(REG_FindMe));

	csv_gev_reg_add(REG_SensorTaps, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_SensorTaps));
	csv_gev_reg_add(REG_SensorDigitizationTaps, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_SensorDigitizationTaps));
	csv_gev_reg_add(REG_WidthMax, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, DEFAULT_WIDTH, NULL, toSTR(REG_WidthMax));
	csv_gev_reg_add(REG_HeightMax, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, DEFAULT_HEIGHT, NULL, toSTR(REG_HeightMax));
	csv_gev_reg_add(REG_RegionMode, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_RegionMode));
	csv_gev_reg_add(REG_RegionDestination, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_RegionDestination));
	csv_gev_reg_add(REG_Width, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, DEFAULT_WIDTH, NULL, toSTR(REG_Width));
	csv_gev_reg_add(REG_Height, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, DEFAULT_HEIGHT, NULL, toSTR(REG_Height));
	csv_gev_reg_add(REG_OffsetX, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_OffsetX));
	csv_gev_reg_add(REG_OffsetY, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_OffsetY));
	csv_gev_reg_add(REG_LinePitch, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_LinePitch));
	csv_gev_reg_add(REG_BinningHorizontal, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_BinningHorizontal));
	csv_gev_reg_add(REG_BinningVertical, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_BinningVertical));
	csv_gev_reg_add(REG_Decimation, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_Decimation));
	csv_gev_reg_add(REG_ReverseX, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_ReverseX));
	csv_gev_reg_add(REG_ReverseY, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_ReverseY));
	csv_gev_reg_add(REG_PixelFormat, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, GX_PIXEL_FORMAT_MONO8, NULL, toSTR(REG_PixelFormat));
	csv_gev_reg_add(REG_PixelCoding, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 8, NULL, toSTR(REG_PixelCoding));
	csv_gev_reg_add(REG_PixelColorFilter, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_PixelColorFilter));
	csv_gev_reg_add(REG_TestPattern, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_TestPattern));
	csv_gev_reg_add(REG_Deinterlacing, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_Deinterlacing));

	csv_gev_reg_add(REG_FrameSpecInfo, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_FrameSpecInfo));


	csv_gev_reg_add(REG_AcquisitionMode, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_AcquisitionMode));
	csv_gev_reg_add(REG_AcquisitionStart, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_AcquisitionStart));
	csv_gev_reg_add(REG_AcquisitionStop, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_AcquisitionStop));
	csv_gev_reg_add(REG_AcquisitionFrameCount, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 1, NULL, toSTR(REG_AcquisitionFrameCount));
	csv_gev_reg_add(REG_AcquisitionBurstFrameCount, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 1, NULL, toSTR(REG_AcquisitionBurstFrameCount));
	csv_gev_reg_add(REG_AcquisitionFrameRate, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 1, NULL, toSTR(REG_AcquisitionFrameRate));
	csv_gev_reg_add(REG_ResultingFrameRate, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_ResultingFrameRate));
	csv_gev_reg_add(REG_TriggerMode, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_TriggerMode));
	csv_gev_reg_add(REG_TriggerSelector, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_TriggerSelector));
	csv_gev_reg_add(REG_TriggerSoftware, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_TriggerSoftware));
	csv_gev_reg_add(REG_TriggerSource, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_TriggerSource));
	csv_gev_reg_add(REG_TriggerActivation, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_TriggerActivation));
	csv_gev_reg_add(REG_TriggerDelayAbsVal, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_TriggerDelayAbsVal));

	csv_gev_reg_add(REG_Calibrate, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_Calibrate));
	csv_gev_reg_add(REG_CalibrateExpoTime0, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 30000, NULL, toSTR(REG_CalibrateExpoTime0));
	csv_gev_reg_add(REG_CalibrateExpoTime1, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 10000, NULL, toSTR(REG_CalibrateExpoTime0));

	csv_gev_reg_add(REG_PointCloud, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_PointCloud));
	csv_gev_reg_add(REG_PointCloudExpoTime, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 10000, NULL, toSTR(REG_PointCloudExpoTime));

	csv_gev_reg_add(REG_DepthImage, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_DepthImage));
	csv_gev_reg_add(REG_DepthImageExpoTime, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 10000, NULL, toSTR(REG_DepthImageExpoTime));

	csv_gev_reg_add(REG_ExposureMode, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_ExposureMode));
	csv_gev_reg_add(REG_ExposureTime, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 10000, NULL, toSTR(REG_ExposureTime));
	csv_gev_reg_add(REG_ExposureAuto, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_ExposureAuto));

	csv_gev_reg_add(REG_HDRImage, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_HDRImage));

	csv_gev_reg_add(REG_LineMode, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_LineMode));

	csv_gev_reg_add(REG_LineDebouncerTime, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_LineDebouncerTime));

	csv_gev_reg_add(REG_GammaAbsVal, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_GammaAbsVal));







	csv_gev_reg_add(REG_AutoExposureTimeLowerLimit, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 100, NULL, toSTR(REG_AutoExposureTimeLowerLimit));
	csv_gev_reg_add(REG_AutoExposureTimeUpperLimit, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 100000, NULL, toSTR(REG_AutoExposureTimeUpperLimit));

	csv_gev_reg_add(REG_GainShutPrior, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_GainShutPrior));



	csv_gev_reg_add(REG_UserSetCurrent, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_UserSetCurrent));
	csv_gev_reg_add(REG_UserSetSelector, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_UserSetSelector));
	csv_gev_reg_add(REG_UserSetLoad, GEV_REG_TYPE_REG, GEV_REG_WRITE,
		4, 0, NULL, toSTR(REG_UserSetLoad));
	csv_gev_reg_add(REG_UserSetSave, GEV_REG_TYPE_REG, GEV_REG_WRITE,
		4, 0, NULL, toSTR(REG_UserSetSave));
	csv_gev_reg_add(REG_UserSetDefaultSelector, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0, NULL, toSTR(REG_UserSetDefaultSelector));

	csv_gev_reg_add(REG_PayloadSize, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, DEFAULT_WIDTH*DEFAULT_HEIGHT, NULL, toSTR(REG_PayloadSize));



	csv_gev_reg_add(REG_CounterResetSource, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, DEFAULT_WIDTH*DEFAULT_HEIGHT, NULL, toSTR(REG_CounterResetSource));




}

static void csv_gev_reg_enroll (void)
{
	csv_gev_reg_general_enroll();
	csv_gev_reg_custom_inq_enroll();
	csv_gev_reg_genicam_enroll();

	log_info("OK : 'enroll' gev reg.");
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

	log_info("OK : 'disroll' gev reg.");
}


int csv_gev_init (void)
{
	int ret = 0;
	struct csv_gev_t *pGEV = &gCSV->gev;

	INIT_LIST_HEAD(&pGEV->head_reg.list);

	csv_gev_reg_enroll();

	ret = csv_gvsp_init();
	ret |= csv_gvcp_init();

	return ret;
}

int csv_gev_deinit (void)
{
	int ret = 0;

	csv_gev_reg_disroll();

	ret = csv_gvsp_deinit();
	ret |= csv_gvcp_deinit();

	return ret;
}


#ifdef __cplusplus
}
#endif

