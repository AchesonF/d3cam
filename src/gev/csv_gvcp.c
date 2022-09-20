#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static int csv_gvcp_sendto (struct csv_gvcp_t *pGVCP)
{
	if ((pGVCP->fd <= 0)||(pGVCP->txlen == 0)
	  ||(0x00000000 == pGVCP->from_addr.sin_addr.s_addr)
	  ||(0x0000 == pGVCP->from_addr.sin_port)) {
		return 0;
	}

	log_hex(STREAM_UDP, pGVCP->txbuf, pGVCP->txlen, "gvcp send[%d] '%s:%d'", pGVCP->txlen, 
		inet_ntoa(pGVCP->from_addr.sin_addr), htons(pGVCP->from_addr.sin_port));

	return sendto(pGVCP->fd, pGVCP->txbuf, pGVCP->txlen, 0, 
		(struct sockaddr *)&pGVCP->from_addr, sizeof(struct sockaddr_in));
}

static int csv_gvcp_error_ack (struct csv_gvcp_t *pGVCP, CMD_MSG_HEADER *pHDR, uint16_t errcode)
{
	memset(pGVCP->txbuf, 0, GVCP_MAX_MSG_LEN);

	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGVCP->txbuf;

	pAckHdr->nStatus			= htons(errcode);
	pAckHdr->nAckMsgValue		= htons(pHDR->nCommand+1);
	pAckHdr->nLength			= htons(0x0000);
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	pGVCP->txlen = sizeof(ACK_MSG_HEADER);

	return csv_gvcp_sendto(pGVCP);
}

int csv_gvcp_event_send (struct csv_gvcp_t *pGVCP, CMD_MSG_HEADER *pHDR)
{
	CMD_MSG_HEADER *pHeader = (CMD_MSG_HEADER *)pGVCP->txbuf;

	pHeader->cKeyValue = GVCP_CMD_KEY_VALUE;
	pHeader->cFlg = GVCP_CMD_FLAG_NEED_ACK; // TODO
	pHeader->nCommand = GEV_EVENT_CMD;
	pHeader->nReqId = pHDR->nReqId;


	pGVCP->txlen = sizeof(ACK_MSG_HEADER) + 0;

	return csv_gvcp_sendto(pGVCP);
}

static int csv_gvcp_discover_ack (struct csv_gvcp_t *pGVCP, CMD_MSG_HEADER *pHDR)
{
	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGVCP->txbuf;

	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_DISCOVERY_ACK);
	pAckHdr->nLength			= htons(sizeof(DISCOVERY_ACK_MSG));
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	DISCOVERY_ACK_MSG *pAckMsg = (DISCOVERY_ACK_MSG *)(pGVCP->txbuf + sizeof(ACK_MSG_HEADER));
	struct gev_conf_t *pGC = &gCSV->cfg.gigecfg;

	pAckMsg->nMajorVer			= htons(pGC->VersionMajor);
	pAckMsg->nMinorVer			= htons(pGC->VersionMinor);
	pAckMsg->nDeviceMode		= htonl(pGC->DeviceMode);
	pAckMsg->nMacAddrHigh		= htons(pGC->MacHi)&0xFFFF;
	pAckMsg->nMacAddrLow		= htonl(pGC->MacLow);
	pAckMsg->nIpCfgOption		= htonl(pGC->IfCapability0);
	pAckMsg->nIpCfgCurrent		= htonl(pGC->IfConfiguration0);
	pAckMsg->nCurrentIp			= pGC->CurrentIPAddress0;
	pAckMsg->nCurrentSubNetMask	= pGC->CurrentSubnetMask0;
	pAckMsg->nDefultGateWay		= pGC->CurrentDefaultGateway0;

	strncpy((char *)pAckMsg->chManufacturerName, pGC->ManufacturerName, 32);
	strncpy((char *)pAckMsg->chModelName, pGC->ModelName, 32);
	strncpy((char *)pAckMsg->chDeviceVersion, pGC->DeviceVersion, 32);
	strncpy((char *)pAckMsg->chManufacturerSpecificInfo, pGC->ManufacturerInfo, 48);
	strncpy((char *)pAckMsg->chSerialNumber, pGC->SerialNumber, 16);
	strncpy((char *)pAckMsg->chUserDefinedName, pGC->UserdefinedName, 16);

	pGVCP->txlen = sizeof(ACK_MSG_HEADER) + sizeof(DISCOVERY_ACK_MSG);

	// TODO ack delay "Discovery ACK Delay"

	return csv_gvcp_sendto(pGVCP);
}

static int csv_gvcp_forceip_ack (struct csv_gvcp_t *pGVCP, CMD_MSG_HEADER *pHDR)
{
	int ret = 0, i = 0;
	uint8_t match_mac = false;
	FORCEIP_CMD_MSG *pFIP = (FORCEIP_CMD_MSG *)(pGVCP->rxbuf + sizeof(CMD_MSG_HEADER));

	if (pFIP->nStaticIp == 0x00000000) {
		// TODO special ip
		return 0;
	}

	struct csv_ifcfg_t *pIFCFG = &gCSV->ifcfg;
	struct csv_eth_t *pETHER = NULL;

	for (i = 0; i < pIFCFG->cnt_ifc; i++) {
		pETHER = &pIFCFG->ether[i];
		if ((pFIP->nMacAddrHigh == u8v_to_u16(pETHER->macaddrress))
		  &&(pFIP->nMacAddrLow == u8v_to_u16(&pETHER->macaddrress[2]))) {
			match_mac = true;
			break;
		}
	}

	if (!match_mac) {
		return -1;
	}

	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGVCP->txbuf;

	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_FORCEIP_ACK);
	pAckHdr->nLength			= htons(0x0000);
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	pGVCP->txlen = sizeof(ACK_MSG_HEADER) + pAckHdr->nLength;
	ret = csv_gvcp_sendto(pGVCP);

	if (NULL != pETHER) {
		usleep(100000);
		csv_eth_ipaddr_set(pETHER->ifrname, pFIP->nStaticIp);
		csv_eth_mask_set(pETHER->ifrname, pFIP->nStaticSubNetMask);
		csv_eth_gateway_set(pFIP->nStaticDefaultGateWay);
		log_info("OK : forceip to IP(0x%08X), NM(0x%08X), GW(0x%08X)", 
			pFIP->nStaticIp, pFIP->nStaticSubNetMask, pFIP->nStaticDefaultGateWay);
	}

	return ret;
}

static int csv_gvcp_packetresend_ack (struct csv_gvcp_t *pGVCP, CMD_MSG_HEADER *pHDR)
{

	//ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGVCP->txbuf;

	//pAckHdr->nStatus			= htons(MV_GEV_STATUS_SUCCESS);
	//pAckHdr->nAckMsgValue		= htons(MV_GEV_PACKETRESEND_INVALID);
	//pAckHdr->nLength			= htons(0x0000);
	//pAckHdr->nAckId			= htons(pHDR->nReqId);

	pGVCP->txlen = sizeof(ACK_MSG_HEADER) + 0;

	return csv_gvcp_sendto(pGVCP);
}



static int csv_gvcp_readreg_ack (struct csv_gvcp_t *pGVCP, CMD_MSG_HEADER *pHDR)
{
	if (pHDR->nLength > GVCP_MAX_PAYLOAD_LEN) {
		log_warn("ERROR : read too long reg addrs.");
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_HEADER);
		return -1;
	}

	if ((pHDR->nLength % sizeof(uint32_t) != 0)||(pGVCP->rxlen != 8+pHDR->nLength)) {
		log_warn("ERROR : length not match.");
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_HEADER);
		return -1;
	}

	int i = 0, ret = -1;
	uint32_t *pRegAddr = (uint32_t *)(pGVCP->rxbuf + sizeof(CMD_MSG_HEADER));
	uint32_t *pRegData = (uint32_t *)(pGVCP->txbuf + sizeof(ACK_MSG_HEADER));
	int nRegs = pHDR->nLength / sizeof(uint32_t); // GVCP_READ_REG_MAX_NUM
	uint32_t reg_addr = 0;
	uint8_t type = GEV_REG_TYPE_NONE;
	uint32_t nTemp = 0;
	char *desc = NULL;

	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGVCP->txbuf;
	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_READREG_ACK);
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	for (i = 0; i < nRegs; i++) {
		reg_addr = ntohl(*pRegAddr);

		if (reg_addr % 4) {
			csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_ADDRESS);
			return -1;
		}

		type = csv_gev_reg_type_get(reg_addr, &desc);
		if ((GEV_REG_TYPE_NONE == type)||(GEV_REG_TYPE_MEM == type)) {
			csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_ADDRESS);
			return -1;
		}

		ret = csv_gev_reg_value_get(reg_addr, &nTemp);
		if (ret < 0) {
			csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_ADDRESS);
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

	pGVCP->txlen = sizeof(ACK_MSG_HEADER) + i*4;

	return csv_gvcp_sendto(pGVCP);
}

static void csv_gvsp_lfsr_generator (uint8_t *data, uint32_t len)
{
	//uint16_t POLYNOMIAL = 0x8016;
	//uint16_t INIT_VALUE = 0xFFFF;
	uint16_t lfsr = 0xFFFF;
	uint8_t *p = data;
	uint32_t i = 0;

	for (i = 0; i < len; i++) {
		*p++ = lfsr & 0xFF;
		lfsr = (lfsr >> 1) ^ (-(lfsr & 1)&0x8016);
	}
}


static int csv_gvcp_writereg_effective (uint32_t regAddr, uint32_t regData)
{
	int ret = 0;

	//struct csv_gvcp_t *pGVCP = &gCSV->gvcp;
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
		// regData & 0xFFFF0000; // key
		pGC->ControlChannelPrivilege = pGC->ControlChannelPrivilege | (regData&(CCP_CSE|CCP_CA|CCP_EA));
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
		break;

	case REG_MessageChannelDestination:
		if (0 != regData) {
			pGC->MessageAddress = swap32(regData);
			//pGVCP->message.peer_addr.sin_addr.s_addr = swap32(regData);
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
		//pGVCP->stream[CAM_LEFT].peer_addr.sin_port = (uint16_t)regData;
		break;

	case REG_StreamChannelPacketSize0:
		if ((0 < (regData&0xFFFF))&&((regData&0xFFFF) < GVSP_PACKET_MAX_SIZE)) {
			if (regData & SCPS_F) {
				// GVCPCap_TD
				uint8_t *data = NULL;
				uint16_t length = regData&0xFFFF;
				data = malloc(length);
				if (NULL == data) {
					log_err("ERROR : malloc test packet");
					return -1;
				}
				csv_gvsp_lfsr_generator(data, length);
				//csv_gvsp_packet_test(&pGVCP->stream[CAM_LEFT], data, length);
				free(data);

				regData &= ~SCPS_F;
			}

			pGC->Channel[CAM_LEFT].Cfg_PacketSize = regData;
		} else {
			ret = -1;
		}
		break;

	case REG_StreamChannelPacketDelay0:
		pGC->Channel[CAM_LEFT].PacketDelay = regData;
		break;

	case REG_StreamChannelDestinationAddress0:
		if (0 != regData) {
			pGC->Channel[CAM_LEFT].Address = swap32(regData);
			//pGVCP->stream[CAM_LEFT].peer_addr.sin_addr.s_addr = swap32(regData);
		} else {
			ret = -1;
		}
		break;

	case REG_StreamChannelConfiguration0:
		pGC->Channel[CAM_LEFT].Configuration = regData;
		break;

	case REG_StreamChannelPort1:
		pGC->Channel[CAM_RIGHT].Port = (uint16_t)regData;
		//pGVCP->stream[CAM_RIGHT].peer_addr.sin_port = (uint16_t)regData;
		break;

	case REG_StreamChannelPacketSize1:
		if ((0 < (regData&0xFFFF))&&((regData&0xFFFF) < GVSP_PACKET_MAX_SIZE)) {
			if (regData & SCPS_F) {
				// GVCPCap_TD
				uint8_t *data = NULL;
				uint16_t length = regData&0xFFFF;
				data = malloc(length);
				if (NULL == data) {
					log_err("ERROR : malloc test packet");
					return -1;
				}
				csv_gvsp_lfsr_generator(data, length);
				//csv_gvsp_packet_test(&pGVCP->stream[CAM_RIGHT], data, length);
				free(data);

				regData &= ~SCPS_F;
			}

			pGC->Channel[CAM_RIGHT].Cfg_PacketSize = regData;
		} else {
			ret = -1;
		}
		break;

	case REG_StreamChannelPacketDelay1:
		pGC->Channel[CAM_RIGHT].PacketDelay = regData;
		break;

	case REG_StreamChannelDestinationAddress1:
		if (0 != regData) {
			pGC->Channel[CAM_RIGHT].Address = swap32(regData);
			//pGVCP->stream[CAM_RIGHT].peer_addr.sin_addr.s_addr = swap32(regData);
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

	case REG_AcquisitionStart:
		/*if (0 != pGC->Channel[CAM_RIGHT].Port) {
			pGVCP->stream[CAM_LEFT].grab_status = GRAB_STATUS_RUNNING;
			pthread_cond_broadcast(&pGVCP->stream[CAM_LEFT].cond_gevgrab);
		}

		if (0 != pGC->Channel[CAM_RIGHT].Port) {
			pGVCP->stream[CAM_RIGHT].grab_status = GRAB_STATUS_RUNNING;
			pthread_cond_broadcast(&pGVCP->stream[CAM_RIGHT].cond_gevgrab);
		}*/
		break;

	case REG_AcquisitionStop:
		/*pGVCP->stream[CAM_LEFT].grab_status = GRAB_STATUS_STOP;
		pthread_cond_broadcast(&pGVCP->stream[CAM_LEFT].cond_gevgrab);
		pGVCP->stream[CAM_RIGHT].grab_status = GRAB_STATUS_STOP;
		pthread_cond_broadcast(&pGVCP->stream[CAM_RIGHT].cond_gevgrab);*/
		break;

	default:
		break;
	}

	return ret;
}

static int csv_gvcp_writereg_ack (struct csv_gvcp_t *pGVCP, CMD_MSG_HEADER *pHDR)
{
	int i = 0, ret = -1;


	if (pHDR->nLength > GVCP_MAX_PAYLOAD_LEN) {
		log_warn("ERROR : too long regs.");
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_HEADER);
		return -1;
	}

	if ((pHDR->nLength % sizeof(WRITEREG_CMD_MSG) != 0)||(pGVCP->rxlen != 8+pHDR->nLength)) {
		log_warn("ERROR : length not match.");
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_HEADER);
		return -1;
	}

	int nRegs = pHDR->nLength/sizeof(WRITEREG_CMD_MSG);

	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGVCP->txbuf;
	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_WRITEREG_ACK);
	pAckHdr->nLength			= htons(sizeof(WRITEREG_ACK_MSG));
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	uint32_t reg_addr = 0;
	uint8_t type = GEV_REG_TYPE_NONE;
	int nIndex = nRegs;
	uint32_t reg_data = 0;
	char *desc = NULL;

	WRITEREG_CMD_MSG *pRegMsg = (WRITEREG_CMD_MSG *)(pGVCP->rxbuf + sizeof(CMD_MSG_HEADER));
	for (i = 0; i < nRegs; i++) {
		reg_addr = ntohl(pRegMsg->nRegAddress);
		reg_data = ntohl(pRegMsg->nRegData);

		if (reg_addr % 4) {
			csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_ADDRESS);
			return -1;
		}

		type = csv_gev_reg_type_get(reg_addr, &desc);
		if ((GEV_REG_TYPE_NONE == type)||(GEV_REG_TYPE_MEM == type)) {
			csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_ADDRESS);
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
			csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_WRITE_PROTECT);
			return -1;
		}

		pRegMsg++;
	}

	WRITEREG_ACK_MSG *pAckMsg = (WRITEREG_ACK_MSG *)(pGVCP->txbuf + sizeof(ACK_MSG_HEADER));
	pAckMsg->nReserved			= htons(0x0000);
	pAckMsg->nIndex				= htons

	if (ret == 0) {
		// TODO 1 save to xml file
	}

	pGVCP->txlen = sizeof(ACK_MSG_HEADER) + sizeof(WRITEREG_ACK_MSG);

	return csv_gvcp_sendto(pGVCP);
}

static int csv_gvcp_readmem_ack (struct csv_gvcp_t *pGVCP, CMD_MSG_HEADER *pHDR)
{
	if (pHDR->nLength != sizeof(READMEM_CMD_MSG)) {
		log_warn("ERROR : readmem param length.");
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_PARAMETER);
		return -1;
	}

	if (pGVCP->rxlen != 8+pHDR->nLength) {
		log_warn("ERROR : length not match.");
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_HEADER);
		return -1;
	}

	int ret = -1;
	char *info = NULL;
	uint16_t len_info = 0;
	uint8_t type = GEV_REG_TYPE_NONE;
	READMEM_CMD_MSG *pReadMem = (READMEM_CMD_MSG *)(pGVCP->rxbuf + sizeof(CMD_MSG_HEADER));
	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGVCP->txbuf;
	READMEM_ACK_MSG *pMemAck = (READMEM_ACK_MSG *)(pGVCP->txbuf + sizeof(ACK_MSG_HEADER));
	uint32_t mem_addr = ntohl(pReadMem->nMemAddress);
	uint16_t mem_len = ntohs(pReadMem->nMemLen);
	struct gev_conf_t *pGC = &gCSV->cfg.gigecfg;
	char *desc = NULL;

	if (mem_addr % 4) {
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_ADDRESS);
		return -1;
	}

	if ((mem_len % 4)||(mem_len > GVCP_READ_MEM_MAX_LEN)) {
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_PARAMETER);
		return -1;
	}

	// branch 1 读取 xml 文件
	if ((mem_addr >= REG_StartOfXmlFile)&&(mem_addr < REG_StartOfXmlFile+GEV_XML_FILE_MAX_SIZE)) {
		if (NULL == pGC->xmlData) {
			csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_ACCESS_DENIED);
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

		pGVCP->txlen = sizeof(ACK_MSG_HEADER) + sizeof(uint32_t) + mem_len;

		return csv_gvcp_sendto(pGVCP);
	}

	// branch 2 读取常规 mem
	type = csv_gev_reg_type_get(mem_addr, &desc);
	if ((GEV_REG_TYPE_NONE == type)||(GEV_REG_TYPE_REG == type)) {
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_ADDRESS);
		return -1;
	}

	if (NULL != desc) {
		log_debug("MR : %s", desc);
	}

	ret = csv_gev_mem_info_get(mem_addr, &info);
	if (ret == -1) {
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_PACKET_REMOVED_FROM_MEMORY);
		return -1;
	} else if (ret == -2) {
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_ADDRESS);
		return -1;
	}

	len_info = csv_gev_mem_info_get_length(mem_addr);
	if (mem_len > len_info) {
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_BAD_ALIGNMENT);
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

	pGVCP->txlen = sizeof(ACK_MSG_HEADER) + sizeof(uint32_t) + mem_len;

	return csv_gvcp_sendto(pGVCP);
}

static int csv_gvcp_writemem_ack (struct csv_gvcp_t *pGVCP, CMD_MSG_HEADER *pHDR)
{
	if ((pHDR->nLength == 0)||(pHDR->nLength % 4)
	  ||(pHDR->nLength <= 4)||(pHDR->nLength > GVCP_MAX_PAYLOAD_LEN+4)) {
		log_warn("ERROR : writemem param length.");
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_PARAMETER);
		return -1;
	}

	if (pGVCP->rxlen != 8+pHDR->nLength) {
		log_warn("ERROR : length not match.");
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_HEADER);
		return -1;
	}

	int ret = -1;
	uint8_t type = GEV_REG_TYPE_NONE;
	WRITEMEM_CMD_MSG *pWriteMem = (WRITEMEM_CMD_MSG *)(pGVCP->rxbuf + sizeof(CMD_MSG_HEADER));
	uint32_t mem_addr = ntohl(pWriteMem->nMemAddress);
	uint16_t len_info = 0;
	char *info = (char *)pWriteMem->chWriteMemData;
	char *desc = NULL;

	if (mem_addr % 4) {
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_ADDRESS);
		return -1;
	}

	if (strlen(info) > GVCP_WRITE_MEM_MAX_LEN) {
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_PARAMETER);
		return -1;
	}

	type = csv_gev_reg_type_get(mem_addr, &desc);
	if ((GEV_REG_TYPE_NONE == type)||(GEV_REG_TYPE_REG == type)) {
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_INVALID_ADDRESS);
		return -1;
	}

	if (NULL != desc) {
		log_debug("MW : %s", desc);
	}

	ret = csv_gev_mem_info_set(mem_addr, info);
	if (ret < 0) {
		csv_gvcp_error_ack(pGVCP, pHDR, GEV_STATUS_WRITE_PROTECT);
		return -1;
	} else if (ret == 0) {
		len_info = pHDR->nLength - 4;
	} else {
		len_info = ret;
	}

	WRITEMEM_ACK_MSG *pMemAck = (WRITEMEM_ACK_MSG *)(pGVCP->txbuf + sizeof(ACK_MSG_HEADER));
	pMemAck->nReserved = htons(0);
	pMemAck->nIndex = htons(len_info);

	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGVCP->txbuf;
	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_WRITEMEM_ACK);
	pAckHdr->nLength			= htons(sizeof(WRITEMEM_ACK_MSG));
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	pGVCP->txlen = sizeof(ACK_MSG_HEADER) + sizeof(WRITEMEM_ACK_MSG);

	return csv_gvcp_sendto(pGVCP);
}

int csv_gvcp_eventdata_ack (struct csv_gvcp_t *pGVCP, CMD_MSG_HEADER *pHDR)
{



	pGVCP->txlen = sizeof(ACK_MSG_HEADER) + 0;

	return csv_gvcp_sendto(pGVCP);
}

static int csv_gvcp_action_ack (struct csv_gvcp_t *pGVCP, CMD_MSG_HEADER *pHDR)
{



	pGVCP->txlen = sizeof(ACK_MSG_HEADER) + 0;

	return csv_gvcp_sendto(pGVCP);
}

static int csv_gvcp_msg_ack (struct csv_gvcp_t *pGVCP, CMD_MSG_HEADER *pHdr)
{
	int ret = -1;

	memset(pGVCP->txbuf, 0, GVCP_MAX_MSG_LEN);

	// TODO check where data from "from_addr.s_addr"

	switch (pHdr->nCommand) {
	case GEV_DISCOVERY_CMD:
		log_debug("%s", toSTR(GEV_DISCOVERY_CMD));
		ret = csv_gvcp_discover_ack(pGVCP, pHdr);
		break;

	case GEV_FORCEIP_CMD:
		log_debug("%s", toSTR(GEV_FORCEIP_CMD));
		ret = csv_gvcp_forceip_ack(pGVCP, pHdr);
		break;

	case GEV_PACKETRESEND_CMD:
		log_debug("%s", toSTR(GEV_PACKETRESEND_CMD));
		ret = csv_gvcp_packetresend_ack(pGVCP, pHdr);
		break;

	case GEV_READREG_CMD:
		log_debug("%s", toSTR(GEV_READREG_CMD));
		ret = csv_gvcp_readreg_ack(pGVCP, pHdr);
		break;

	case GEV_WRITEREG_CMD:
		log_debug("%s", toSTR(GEV_WRITEREG_CMD));
		ret = csv_gvcp_writereg_ack(pGVCP, pHdr);
		break;

	case GEV_READMEM_CMD:
		log_debug("%s", toSTR(GEV_READMEM_CMD));
		ret = csv_gvcp_readmem_ack(pGVCP, pHdr);
		break;

	case GEV_WRITEMEM_CMD:
		log_debug("%s", toSTR(GEV_WRITEMEM_CMD));
		ret = csv_gvcp_writemem_ack(pGVCP, pHdr);
		break;

	case GEV_EVENT_ACK:
		//ret = csv_gvcp_event_ack(pGVCP, pHdr);
		break;

	case GEV_EVENTDATA_ACK:
		//ret = csv_gvcp_eventdata_ack(pGVCP, pHdr);
		break;

	case GEV_ACTION_CMD:
		log_debug("%s", toSTR(GEV_ACTION_CMD));
		ret = csv_gvcp_action_ack(pGVCP, pHdr);
		break;

	default:
		log_debug("WARN : not support cmd 0x%04X.", pHdr->nCommand);
		ret = csv_gvcp_error_ack(pGVCP, pHdr, GEV_STATUS_NOT_IMPLEMENTED);
		break;
	}

	memset(pGVCP->rxbuf, 0, GVCP_MAX_MSG_LEN);

	return ret;
}

int csv_gvcp_trigger (struct csv_gvcp_t *pGVCP)
{
	int i = 0;
	struct csv_ifcfg_t *pIFCFG = &gCSV->ifcfg;
	struct csv_eth_t *pETHER = NULL;
	socklen_t from_len = sizeof(struct sockaddr_in);

	pGVCP->rxlen = recvfrom(pGVCP->fd, pGVCP->rxbuf, GVCP_MAX_MSG_LEN, 0, 
		(struct sockaddr *)&pGVCP->from_addr, &from_len);

	for (i = 0; i < pIFCFG->cnt_ifc; i++) {
		pETHER = &pIFCFG->ether[i];
		if (pGVCP->from_addr.sin_addr.s_addr == pETHER->ipaddress) {
			//log_debug("gvcp msg from myself.");
			return 0;
		}
	}

	if (pGVCP->rxlen < sizeof(CMD_MSG_HEADER)) {
		log_hex(STREAM_UDP, pGVCP->rxbuf, pGVCP->rxlen, "Wrong gev head length. '%s:%d'", 
			inet_ntoa(pGVCP->from_addr.sin_addr), htons(pGVCP->from_addr.sin_port));
		return -1;
	}

	log_hex(STREAM_UDP, pGVCP->rxbuf, pGVCP->rxlen, "gvcp recv[%d] '%s:%d'", pGVCP->rxlen, 
		inet_ntoa(pGVCP->from_addr.sin_addr), htons(pGVCP->from_addr.sin_port));

	CMD_MSG_HEADER *pHeader = (CMD_MSG_HEADER *)pGVCP->rxbuf;
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

	return csv_gvcp_msg_ack(pGVCP, &Cmdheader);
}

static int csv_gvcp_server_open (struct csv_gvcp_t *pGVCP)
{
	int ret = -1;
	int fd = -1;
	struct sockaddr_in local_addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 ) {
		log_err("ERROR : socket %s.", pGVCP->name);

		return -1;
	}

	memset((void*)&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(pGVCP->port);
	bzero(&(local_addr.sin_zero), 8);

	ret = bind(fd, (struct sockaddr*)&local_addr, sizeof(struct sockaddr));
	if(ret < 0) {
		log_err("ERROR : bind %s.", pGVCP->name);

		return -2;
	}

    ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (ret < 0) {
        log_err("ERROR : fcntl %s.", pGVCP->name);
        if (close(fd)<0) {
            log_err("ERROR : close %s.", pGVCP->name);
        }
        return ret;
    }

	pGVCP->fd = fd;
	log_info("OK : bind %s : '%d/udp' as fd(%d).",  
		pGVCP->name, pGVCP->port, pGVCP->fd);

	return 0;
}

static int csv_gvcp_server_close (struct csv_gvcp_t *pGVCP)
{
	if (NULL == pGVCP) {
		return -1;
	}

	if (pGVCP->fd > 0) {
		if (close(pGVCP->fd) < 0) {
			log_err("ERROR : close %s.", pGVCP->name);
			return -1;
		}

		log_info("OK : close %s fd(%d).", pGVCP->name, pGVCP->fd);
		pGVCP->fd = -1;
	}

	return 0;
}

int csv_gvcp_init (void)
{
	int ret = 0;
	struct csv_gvcp_t *pGVCP = &gCSV->gvcp;

	pGVCP->fd = -1;
	pGVCP->name = NAME_UDP_GVCP;
	pGVCP->port = GVCP_PUBLIC_PORT;
	pGVCP->ReqId = GVCP_REQ_ID_INIT;
	pGVCP->rxlen = 0;
	pGVCP->txlen = 0;

	ret = csv_gvcp_server_open(pGVCP);

	return ret;
}

int csv_gvcp_deinit (void)
{
	int ret = 0;
	struct csv_gvcp_t *pGVCP = &gCSV->gvcp;

	ret = csv_gvcp_server_close(pGVCP);

	return ret;
}


#ifdef __cplusplus
}
#endif

