#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


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

	pAckMsg->nMajorVer			= htons(0x0001);
	pAckMsg->nMinorVer			= htons(0x0002);
	pAckMsg->nDeviceMode		= htonl(0x00000001);
	pAckMsg->nMacAddrHigh		= htons((pETH->MACAddr[0]<<8)+pETH->MACAddr[1]) & 0xFFFF;
	pAckMsg->nMacAddrLow		= htonl((pETH->MACAddr[2]<<24)+(pETH->MACAddr[3]<<16)+(pETH->MACAddr[4]<<8)+pETH->MACAddr[5]);
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

static uint32_t pick_reg_value (int reg_addr)
{
	switch (reg_addr) {
	case REG_Version:
		break;
	case REG_DeviceMode:
		break;





	}

	return 0;
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

	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_READREG_ACK);
	pAckHdr->nLength			= htons(pHDR->nLength);
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	uint32_t nTemp = 0; // GVCP_READ_REG_MAX_NUM
	for (i = 0; i < pHDR->nLength/sizeof(int); i++) {
		nTemp = pick_reg_value(ntohl(*(pCurRegAddr+i)));
		// TODO
		*(pCurRegData+i) = htonl(nTemp);
	}


	pGEV->txlen = sizeof(ACK_MSG_HEADER) + pAckHdr->nLength;

	return csv_gvcp_sendto(pGEV);
}

static int csv_gvcp_writereg_ack (struct csv_gev_t *pGEV, CMD_MSG_HEADER *pHDR)
{
	int i = 0, ret = MV_OK;
	//WRITEREG_CMD_MSG *pCurCmdMsg = (WRITEREG_CMD_MSG *)(pGEV->rxbuf + sizeof(CMD_MSG_HEADER));

	ACK_MSG_HEADER *pAckHdr = (ACK_MSG_HEADER *)pGEV->txbuf;
	//int *pCurRegData = (int *)(pGEV->txbuf + sizeof(ACK_MSG_HEADER));

	pAckHdr->nStatus			= htons(GEV_STATUS_SUCCESS);
	pAckHdr->nAckMsgValue		= htons(GEV_WRITEREG_ACK);
	pAckHdr->nLength			= htons(pHDR->nLength);
	pAckHdr->nAckId				= htons(pHDR->nReqId);

	int nIndex = MAX_WRITEREG_INDEX;
	for (i = 0; i < pHDR->nLength/sizeof(WRITEREG_CMD_MSG); i++) {
		//pCurCmdMsg = (WRITEREG_CMD_MSG *)pCurCmdMsg + i;

		// TODO set reg

		if (ret != MV_OK) {
			nIndex = i;
			break;
		}
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

	ret = csv_gev_server_open(pGEV);

	return ret;
}

int csv_gev_deinit (void)
{
	int ret = 0;

	ret = csv_gev_server_close(&gCSV->gev);

	return ret;
}


#ifdef __cplusplus
}
#endif

