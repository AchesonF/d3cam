#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static struct reglist_t *csv_gev_reg_malloc (void)
{
	struct reglist_t *cur = NULL;

	cur = (struct reglist_t *)malloc(sizeof(struct reglist_t));
	if (cur == NULL) {
		log_err("ERROR : malloc reglist_t");
		return NULL;
	}
	memset(cur, 0, sizeof(struct reglist_t));

	return cur;
}

static void csv_gev_reg_add (uint16_t addr, uint8_t type, uint8_t mode,
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
				log_info("WARN : mem 0x%04X null", addr);
				return ;
			}

			len = strlen(info);
			if (len > 0) {
				if (len > length) {
					len = length;
				}
				pRI->info = (char *)malloc(len+1);
				if (NULL == pRI->info) {
					log_err("ERROR : malloc ri info");
					return ;
				}
				memcpy(pRI->info, info, len);
			}
		} else {
			log_info("ERROR : not support type");
		}

		if (NULL != desc) {
			len = strlen(desc);
			pRI->desc = (char *)malloc(len+1);
			if (NULL == pRI->info) {
				log_err("ERROR : malloc ri desc");
				return ;
			}
			memcpy(pRI->desc, desc, len);
		}

		list_add_tail(&cur->list, &gCSV->gev.head_reg.list);
	}
}

uint32_t csv_gev_reg_value_get (uint16_t addr)
{
	uint32_t value = 0;
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
			value = pRI->value;
			break;
		}
	}

	return value;
}

int csv_gev_reg_value_set (uint16_t addr, uint32_t value)
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
				// TODO  save to file
			}
			ret = 0;
			break;
		}
	}

	return ret;
}

char *csv_gev_mem_info_get (uint16_t addr)
{
	char *info = NULL;
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
			info = pRI->info;
			break;
		}
	}

	return info;
}

int csv_gev_mem_info_set (uint16_t addr, char *info)
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
				log_err("ERROR : malloc mem info");
				return -1;
			}
			memcpy(pRI->info, info, len);


			// TODO save to file

			ret = 0;
			break;
		}
	}

	return ret;
}

uint8_t csv_gev_reg_type_get (uint16_t addr)
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
			break;
		}
	}

	return type;
}





static void csv_gev_reg_enroll (void)
{
	struct gev_param_t *pGP = &gCSV->cfg.gp;

	csv_gev_reg_add(REG_Version, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGP->Version, NULL, NULL);
	csv_gev_reg_add(REG_DeviceMode, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, pGP->DeviceMode, NULL, NULL);
	csv_gev_reg_add(REG_DeviceMACAddressHigh0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_DeviceMACAddressLow0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_NetworkInterfaceCapability0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_NetworkInterfaceConfiguration0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_CurrentIPAddress0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_CurrentSubnetMask0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_CurrentDefaultGateway0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_ManufacturerName, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		32, 0, "CSV", NULL);
	csv_gev_reg_add(REG_ModelName, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		32, 0, "CSV01", NULL);
	csv_gev_reg_add(REG_DeviceVersion, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		32, 0, "V1.0.0", NULL);
	csv_gev_reg_add(REG_ManufacturerInfo, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		48, 0, "CSVCSV", NULL);
	csv_gev_reg_add(REG_SerialNumber, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		16, 0, "CCSSVV0001", NULL);
	csv_gev_reg_add(REG_UserdefinedName, GEV_REG_TYPE_MEM, GEV_REG_RDWR, 
		16, 0, "USERDEFINED", NULL);
	csv_gev_reg_add(REG_FirstURL, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		512, 0, "first_url_stringformat...", NULL);
	csv_gev_reg_add(REG_SecondURL, GEV_REG_TYPE_MEM, GEV_REG_READ, 
		512, 0, "second_url_stringformat...", NULL);
	csv_gev_reg_add(REG_NumberofNetworkInterfaces, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 1, NULL, NULL);
	csv_gev_reg_add(REG_PersistentIPAddress, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PersistentSubnetMask0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PersistentDefaultGateway0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_LinkSpeed0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_DeviceMACAddressHigh1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_DeviceMACAddressLow1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_NetworkInterfaceCapability1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_NetworkInterfaceConfiguration1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_CurrentIPAddress1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_CurrentSubnetMask1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_CurrentDefaultGateway1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PersistentIPAddress1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PersistentSubnetMask1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PersistentDefaultGateway1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_LinkSpeed1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_DeviceMACAddressHigh2, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_DeviceMACAddressLow2, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_NetworkInterfaceCapability2, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_NetworkInterfaceConfiguration2, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_CurrentIPAddress2, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_CurrentSubnetMask2, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_CurrentDefaultGateway2, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PersistentIPAddress2, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PersistentSubnetMask2, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PersistentDefaultGateway2, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_LinkSpeed2, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_DeviceMACAddressHigh3, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_DeviceMACAddressLow3, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_NetworkInterfaceCapabilityLow3, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_NetworkInterfaceConfiguration3, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_CurrentIPAddress3, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_CurrentSubnetMask3, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_CurrentDefaultGateway3, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PersistentIPAddress3, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PersistentSubnetMask3, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PersistentDefaultGateway3, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_LinkSpeed3, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_NumberofMessageChannels, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_NumberofStreamChannels, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_NumberofActionSignals, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_ActionDeviceKey, GEV_REG_TYPE_REG, GEV_REG_WRITE, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_NumberofActiveLinks, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_GVSPCapability, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_MessageChannelCapability, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_GVCPCapability, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_HeartbeatTimeout, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_TimestampTickFrequencyHigh, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_TimestampTickFrequencyLow, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_TimestampControl, GEV_REG_TYPE_REG, GEV_REG_WRITE, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_TimestampValueHigh, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_TimestampValueLow, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_DiscoveryACKDelay, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_GVCPConfiguration, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PendingTimeout, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_ControlSwitchoverKey, GEV_REG_TYPE_REG, GEV_REG_WRITE, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_GVSPConfiguration, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PhysicalLinkConfigurationCapability, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PhysicalLinkConfiguration, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_IEEE1588Status, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_ScheduledActionCommandQueueSize, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_ControlChannelPrivilege, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PrimaryApplicationPort, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_PrimaryApplicationIPAddress, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_MessageChannelPort, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_MessageChannelDestination, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_MessageChannelTransmissionTimeout, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_MessageChannelRetryCount, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_MessageChannelSourcePort, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_StreamChannelPort0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelPacketSize0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelPacketDelay0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelDestinationAddress0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelSourcePort0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelCapability0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelConfiguration0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelZone0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelZoneDirection0, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_StreamChannelPort1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelPacketSize1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelPacketDelay1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelDestinationAddress1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelSourcePort1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelCapability1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelConfiguration1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelZone1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelZoneDirection1, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_OtherStreamChannelsRegisters, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_StreamChannelPort511, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelPacketSize511, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelPacketDelay511, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelDestinationAddress511, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelSourcePort511, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelCapability511, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelConfiguration511, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelZone511, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_StreamChannelZoneDirection511, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_ManifestTable, GEV_REG_TYPE_REG, GEV_REG_READ, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_ActionGroupKey0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_ActionGroupMask0, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_ActionGroupKey1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_ActionGroupMask1, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_OtherActionSignalsRegisters, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_ActionGroupKey127, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
	csv_gev_reg_add(REG_ActionGroupMask127, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);

	csv_gev_reg_add(REG_StartofNanufacturerSpecificRegisterSpace, GEV_REG_TYPE_REG, GEV_REG_RDWR, 
		4, 0x00000000, NULL, NULL);
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
	pAckHdr->nAckMsgValue		= htons(pHDR->nCommand);
	pAckHdr->nLength			= htons(0x0000);
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	pGEV->txlen = sizeof(ACK_MSG_HEADER);

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
	struct gev_param_t *pGP = &gCSV->cfg.gp;

	pAckMsg->nMajorVer			= htons((pGP->Version>>16)&0xFFFF);
	pAckMsg->nMinorVer			= htons(pGP->Version&0xFFFF);
	pAckMsg->nDeviceMode		= htonl(pGP->DeviceMode);
	pAckMsg->nMacAddrHigh		= htons(pGP->MacHi)&0xFFFF;
	pAckMsg->nMacAddrLow		= htonl(pGP->MacLow);
	pAckMsg->nIpCfgOption		= htonl(0x80000007);
	pAckMsg->nIpCfgCurrent		= htonl(0x00000005);
	pAckMsg->nCurrentIp			= inet_addr(pETH->ip);
	pAckMsg->nCurrentSubNetMask	= inet_addr(pETH->nm);
	pAckMsg->nDefultGateWay		= inet_addr(pETH->gw);

	strncpy((char *)pAckMsg->chManufacturerName, "CSV", 32);
	strncpy((char *)pAckMsg->chModelName, "MV-CS001-50GM", 32);
	strncpy((char *)pAckMsg->chDeviceVersion, "V1.0.0", 32);
	strncpy((char *)pAckMsg->chManufacturerSpecificInfo, "GEV", 48);
	strncpy((char *)pAckMsg->chSerialNumber, "010A000B0DCC", 16);
	strncpy((char *)pAckMsg->chUserDefinedName, "user def here", 16);

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

	pETH->IPAddress = pFIP->nStaticIp;
	pETH->GateWayAddr = pFIP->nStaticDefaultGateWay;
	pETH->IPMask = pFIP->nStaticSubNetMask;

	struct in_addr addr;
	addr.s_addr = pETH->IPAddress;
	strcpy(pETH->ip, inet_ntoa(addr));

	addr.s_addr = pETH->GateWayAddr;
	strcpy(pETH->gw, inet_ntoa(addr));

	addr.s_addr = pETH->IPMask;
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
	int i = 0;
	int *pCurRegAddr = (int *)(pGEV->rxbuf + sizeof(CMD_MSG_HEADER));
	if (pHDR->nLength > GVCP_MAX_PAYLOAD_LEN) {
		log_info("ERROR : read too long reg addrs");
		return -1;
	}

	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGEV->txbuf;
	int *pCurRegData = (int *)(pGEV->txbuf + sizeof(ACK_MSG_HEADER));
	int nRegs = pHDR->nLength / sizeof(uint32_t);

	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_READREG_ACK);
	//pAckHdr->nLength			= htons(pHDR->nLength);
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	uint8_t type = GEV_REG_TYPE_NONE;
	uint32_t nTemp = 0; // GVCP_READ_REG_MAX_NUM
	for (i = 0; i < nRegs; i++) {
		type = csv_gev_reg_type_get((uint16_t)ntohl(*pCurRegAddr));
		if ((GEV_REG_TYPE_NONE == type)||(GEV_REG_TYPE_MEM == type)) {
			break;
		}
		nTemp = csv_gev_reg_value_get((uint16_t)ntohl(*pCurRegAddr));

		*pCurRegData++ = htonl(nTemp);
		pCurRegAddr++;
	}

	pAckHdr->nLength			= htons(i*4);

	pGEV->txlen = sizeof(ACK_MSG_HEADER) + pAckHdr->nLength;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_writereg_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{
	int i = 0, ret = 0;
	WRITEREG_CMD_MSG *pCurCmdMsg = (WRITEREG_CMD_MSG *)(pGEV->rxbuf + sizeof(CMD_MSG_HEADER));

	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGEV->txbuf;
	int *pCurRegData = (int *)(pGEV->txbuf + sizeof(ACK_MSG_HEADER));
	int nRegs = pHDR->nLength/sizeof(WRITEREG_CMD_MSG);

	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_WRITEREG_ACK);
	pAckHdr->nLength			= htons(pHDR->nLength);
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	uint8_t type = GEV_REG_TYPE_NONE;
	int nIndex = MAX_WRITEREG_INDEX;
	for (i = 0; i < nRegs; i++) {
		type = csv_gev_reg_type_get((uint16_t)ntohl(pCurCmdMsg->nRegAddress));
		if ((GEV_REG_TYPE_NONE == type)||(GEV_REG_TYPE_MEM == type)) {
			nIndex = i;
			break;
		}

		ret = csv_gev_reg_value_set((uint16_t)ntohl(pCurCmdMsg->nRegAddress), pCurCmdMsg->nRegData);
		if (ret != 0) {
			nIndex = i;
			break;
		}

		*pCurRegData++ = ret;
		pCurCmdMsg++;
	}

	WRITEREG_ACK_MSG *pAckMsg = (WRITEREG_ACK_MSG *)(pGEV->txbuf + sizeof(ACK_MSG_HEADER));
	pAckMsg->nReserved	= 0;
	pAckMsg->nIndex		= htons(nIndex);

	pGEV->txlen = sizeof(ACK_MSG_HEADER) + pAckHdr->nLength;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_readmem_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{



	pGEV->txlen = sizeof(ACK_MSG_HEADER) + 0;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_writemem_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{



	pGEV->txlen = sizeof(ACK_MSG_HEADER) + 0;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_event_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{



	pGEV->txlen = sizeof(ACK_MSG_HEADER) + 0;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_eventdata_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{



	pGEV->txlen = sizeof(ACK_MSG_HEADER) + 0;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_action_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{



	pGEV->txlen = sizeof(ACK_MSG_HEADER) + 0;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gev_gvcp_msg (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHdr)
{
	int ret = -1;

	memset(pGEV->txbuf, 0, GVCP_MAX_MSG_LEN);

	switch (pHdr->nCommand) {
	case GEV_DISCOVERY_CMD:
		if (pHdr->cFlg & 0x01) {
			ret = csv_gvcp_discover_ack(pGEV, pHdr);
		}
		break;

	case GEV_FORCEIP_CMD:
		ret = csv_gvcp_forceip_ack(pGEV, pHdr);
		break;

	case GEV_PACKETRESEND_CMD:
		ret = csv_gvcp_packetresend_ack(pGEV, pHdr);
		break;

	case GEV_READREG_CMD:
		ret = csv_gvcp_readreg_ack(pGEV, pHdr);
		break;

	case GEV_WRITEREG_CMD:
		ret = csv_gvcp_writereg_ack(pGEV, pHdr);
		break;

	case GEV_READMEM_CMD:
		ret = csv_gvcp_readmem_ack(pGEV, pHdr);
		break;

	case GEV_WRITEMEM_CMD:
		ret = csv_gvcp_writemem_ack(pGEV, pHdr);
		break;

	case GEV_EVENT_CMD:
		ret = csv_gvcp_event_ack(pGEV, pHdr);
		break;

	case GEV_EVENTDATA_CMD:
		ret = csv_gvcp_eventdata_ack(pGEV, pHdr);
		break;

	case GEV_ACTION_CMD:
		ret = csv_gvcp_action_ack(pGEV, pHdr);
		break;

	default:
		log_info("WARN : not support cmd 0x%04X", pHdr->nCommand);
		ret = csv_gvcp_error_ack(pGEV, pHdr, GEV_STATUS_NOT_IMPLEMENTED);
		break;
	}

	memset(pGEV->rxbuf, 0, GVCP_MAX_MSG_LEN);

	return ret;
}

int csv_gev_trigger (struct csv_gev_t *pGEV)
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
		return -1;
	}

	return csv_gev_gvcp_msg(pGEV, &Cmdheader);
}



int csv_gev_server_open (struct csv_gev_t *pGEV)
{
	int ret = -1;
	int fd = -1;
	struct sockaddr_in local_addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 ) {
		log_err("ERROR : socket %s", pGEV->name);

		return -1;
	}

	memset((void*)&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(pGEV->port);
	bzero(&(local_addr.sin_zero), 8);

	ret = bind(fd, (struct sockaddr*)&local_addr, sizeof(struct sockaddr));
	if(ret < 0) {
		log_err("ERROR : bind %s", pGEV->name);

		return -2;
	}

    ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (ret < 0) {
        log_err("ERROR : fcntl %s", pGEV->name);
        if (close(fd)<0) {
            log_err("ERROR : close %s", pGEV->name);
        }
        return ret;
    }

	pGEV->fd = fd;
	log_info("OK : bind %s : '%d/udp' as fd(%d).",  
		pGEV->name, pGEV->port, pGEV->fd);

	return 0;
}

static int csv_gev_server_close (struct csv_gev_t *pGEV)
{
	if (pGEV->fd > 0) {
		if (close(pGEV->fd) < 0) {
			log_err("ERROR : close %s", pGEV->name);
			return -1;
		}

		pGEV->fd = -1;
		log_info("OK : close %s.", pGEV->name);
	}

	return 0;
}

int csv_gev_init (void)
{
	int ret = 0;
	struct csv_gev_t *pGEV = &gCSV->gev;

	pGEV->fd = -1;
	pGEV->name = NAME_UDP_GVCP;
	pGEV->port = GVCP_PUBLIC_PORT;
	pGEV->ReqId = GVCP_REQ_ID_INIT;
	pGEV->rxlen = 0;
	pGEV->txlen = 0;

	csv_gev_reg_enroll();

	ret = csv_gev_server_open(pGEV);

	return ret;
}

int csv_gev_deinit (void)
{
	int ret = 0;

	csv_gev_reg_disroll();

	ret = csv_gev_server_close(&gCSV->gev);

	return ret;
}


#ifdef __cplusplus
}
#endif

