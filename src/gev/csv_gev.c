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





static void csv_gev_reg_enroll (void)
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

	csv_gev_reg_add(REG_StreamChannelPort0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel.Port, NULL, toSTR(REG_StreamChannelPort0));
	csv_gev_reg_add(REG_StreamChannelPacketSize0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel.Cfg_PacketSize, NULL, toSTR(REG_StreamChannelPacketSize0));
	csv_gev_reg_add(REG_StreamChannelPacketDelay0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel.PacketDelay, NULL, toSTR(REG_StreamChannelPacketDelay0));
	csv_gev_reg_add(REG_StreamChannelDestinationAddress0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel.Address, NULL, toSTR(REG_StreamChannelDestinationAddress0));
	csv_gev_reg_add(REG_StreamChannelSourcePort0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel.SourcePort, NULL, toSTR(REG_StreamChannelSourcePort0));
	csv_gev_reg_add(REG_StreamChannelCapability0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel.Capability, NULL, toSTR(REG_StreamChannelCapability0));
	csv_gev_reg_add(REG_StreamChannelConfiguration0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, pGC->Channel.Configuration, NULL, toSTR(REG_StreamChannelConfiguration0));
	csv_gev_reg_add(REG_StreamChannelZone0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel.Zone, NULL, toSTR(REG_StreamChannelZone0));
	csv_gev_reg_add(REG_StreamChannelZoneDirection0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGC->Channel.ZoneDirection, NULL, toSTR(REG_StreamChannelZoneDirection0));

	//  add more stream

	csv_gev_reg_add(REG_ManifestTable, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, toSTR(REG_ManifestTable));

	csv_gev_reg_add(REG_ActionGroupKey0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, toSTR(REG_ActionGroupKey0));
	csv_gev_reg_add(REG_ActionGroupMask0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, toSTR(REG_ActionGroupMask0));

	csv_gev_reg_add(REG_StartofManufacturerSpecificRegisterSpace, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, toSTR(REG_StartofManufacturerSpecificRegisterSpace));

	// add more from genicam xml.

	csv_gev_reg_add(REG_DeviceControlInq, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0x80000000, NULL, toSTR(REG_DeviceControlInq));

	csv_gev_reg_add(REG_GevSCPSMaxInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, GVSP_PACKET_MAX_SIZE, NULL, toSTR(REG_GevSCPSMaxInq));

	csv_gev_reg_add(REG_GevSCPSMinInq, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 1500, NULL, toSTR(REG_GevSCPSMinInq));



	csv_gev_reg_add(REG_DeviceFirmwareVersion, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, pGC->DeviceVersion, toSTR(REG_DeviceFirmwareVersion));

	csv_gev_reg_add(REG_DeviceUptime, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0, NULL, toSTR(REG_DeviceUptime));

	csv_gev_reg_add(REG_BoardDeviceType, GEV_REG_TYPE_REG, GEV_REG_READ,
		4, 0x00000000, NULL, toSTR(REG_BoardDeviceType));

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


	csv_gev_reg_add(REG_AcquisitionStart, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0x00000000, NULL, toSTR(REG_AcquisitionStart));

	csv_gev_reg_add(REG_AcquisitionStop, GEV_REG_TYPE_REG, GEV_REG_RDWR,
		4, 0x00000000, NULL, toSTR(REG_AcquisitionStop));


































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

