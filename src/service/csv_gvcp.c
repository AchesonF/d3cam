#include "inc_files.h"


#ifdef __cplusplus
extern "C" {
#endif


int csv_gvcp_sendto (struct csv_gvcp_t *pGVCP)
{
	socklen_t from_len = sizeof(struct sockaddr_in);

	log_hex(STREAM_UDP, pGVCP->Send.pTx, pGVCP->Send.nTx, "gvcp sendto '%s:%d'",
		inet_ntoa(pGVCP->from_addr.sin_addr), htons(pGVCP->from_addr.sin_port));

	return sendto(pGVCP->fd, pGVCP->Send.pTx, pGVCP->Send.nTx, 0, 
		(struct sockaddr *)&pGVCP->from_addr, from_len);
}


int csv_gvcp_discover_ack (struct csv_gvcp_t *pGVCP, struct gvcp_cmd_header_t *pHDR)
{
	struct csv_eth_t *pETH = &gCSV->eth;


	struct gvcp_discover_ack ack_msg;
	memset(&ack_msg, 0, sizeof(struct gvcp_discover_ack));
	ack_msg.header.wStatus=htons(0);
	ack_msg.header.wAck=htons(GVCP_DISCOVERY_ACK);
	ack_msg.header.wLen=htons(sizeof(struct gvcp_ack_payload_t));
	ack_msg.header.wReqID=htons(pHDR->wReqID);
	ack_msg.payload.dwSpecVer=htonl(0x010002);;
	ack_msg.payload.dwDevMode=htonl(1);
	//uint8 MyMac[6]={0xc4,0x2f,0x90,0xf1,0x71,0x3e};
	memcpy(&ack_msg.payload.Mac[2], pETH->MACAddr,6);
	ack_msg.payload.dwSupIpSet=htonl(0x80000007);
	ack_msg.payload.dwCurIpSet=htonl(0x00005);
	//uint8 unused1[12];
	*((uint32_t *)&ack_msg.payload.CurIP[12])=inet_addr(pETH->ip);//last 4 byte
	*((uint32_t *)&ack_msg.payload.SubMask[12])=inet_addr(pETH->nm);//last 4 byte
	*((uint32_t *)&ack_msg.payload.Gateway[12])=inet_addr(pETH->gw);//last 4 byte
	strcpy(ack_msg.payload.szFacName,"GEV");//first
	strcpy(ack_msg.payload.szModelName,"MV-CS001-50GM");//first
	strcpy(ack_msg.payload.szDevVer,"V2.8.6 180210 143913");
	strcpy(ack_msg.payload.szFacInfo,"GEV");
	strcpy(ack_msg.payload.szSerial,"00C31976084");
	strcpy(ack_msg.payload.szUserName,"");

	pGVCP->Send.nTx = sizeof(struct gvcp_discover_ack);
	memcpy(pGVCP->Send.pTx, (uint8_t *)&ack_msg, pGVCP->Send.nTx);


	return csv_gvcp_sendto(pGVCP);
}


int csv_gvcp_trigger (struct csv_gvcp_t *pGVCP)
{
	int ret = 0;
	uint8_t *pBuf = pGVCP->Recv.pRx;
	socklen_t from_len = sizeof(struct sockaddr_in);

	pGVCP->Recv.nRx = recvfrom(pGVCP->fd, pBuf, SIZE_UDP_BUFF, 0, 
		(struct sockaddr *)&pGVCP->from_addr, &from_len);

	if (pGVCP->Recv.nRx < sizeof(struct gvcp_cmd_header_t)) {
		log_hex(STREAM_UDP, pBuf, pGVCP->Recv.nRx, "Wrong gvcp head length. '%s:%d'", 
			inet_ntoa(pGVCP->from_addr.sin_addr), htons(pGVCP->from_addr.sin_port));
		return -1;
	}

	log_hex(STREAM_UDP, pGVCP->Recv.pRx, pGVCP->Recv.nRx, "gvcp recvfrom '%s:%d'", 
		inet_ntoa(pGVCP->from_addr.sin_addr), htons(pGVCP->from_addr.sin_port));

	struct gvcp_cmd_header_t *pHeader = (struct gvcp_cmd_header_t *)pBuf;
	struct gvcp_cmd_header_t Cmdheader;

	Cmdheader.cMsgKeyCode = pHeader->cMsgKeyCode;
	Cmdheader.cFlag = pHeader->cFlag;
	Cmdheader.wCmd = ntohs(pHeader->wCmd);
	Cmdheader.wLen = ntohs(pHeader->wLen);
	Cmdheader.wReqID = ntohs(pHeader->wReqID);

	if (Cmdheader.cMsgKeyCode != 0x42) {
		return 0;
	}

	switch (Cmdheader.wCmd) {
	case GVCP_DISCOVERY_CMD:
		ret = csv_gvcp_discover_ack(pGVCP, &Cmdheader);
		break;
	case GVCP_FORCEIP_CMD:
		break;
	case GVCP_READMEM_CMD:
		break;
	case GVCP_WRITEREG_CMD:
		break;
	default:
		break;
	}

	return ret;
}



int csv_gvcp_server_open (struct csv_gvcp_t *pGVCP)
{
	int ret = -1;
	int fd = -1;
	struct sockaddr_in local_addr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 ) {
		log_err("ERROR : socket %s", pGVCP->name);

		return -1;
	}

	memset((void*)&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(pGVCP->port);
	bzero(&(local_addr.sin_zero), 8);

	ret = bind(fd, (struct sockaddr*)&local_addr, sizeof(struct sockaddr));
	if(ret < 0) {
		log_err("ERROR : bind %s", pGVCP->name);

		return -2;
	}

    ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (ret < 0) {
        log_err("ERROR : fcntl %s", pGVCP->name);
        if (close(fd)<0) {
            log_err("ERROR : close %s", pGVCP->name);
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
	if (pGVCP->fd > 0) {
		if (close(pGVCP->fd) < 0) {
			log_err("ERROR : close %s", pGVCP->name);
			return -1;
		}

		pGVCP->fd = -1;
		log_info("OK : close %s.", pGVCP->name);
	}

	return 0;
}

int csv_gvcp_init (void)
{
	int ret = 0;
	struct csv_gvcp_t *pGVCP = &gCSV->gvcp;

	pGVCP->fd = -1;
	pGVCP->name = NAME_UDP_GVCP;
	pGVCP->port = PORT_UDP_GVCP;
	pGVCP->Recv.nRx = 0;
	pGVCP->Send.nTx = 0;

	ret = csv_gvcp_server_open(pGVCP);

	return ret;
}

int csv_gvcp_deinit (void)
{
	int ret = 0;

	ret = csv_gvcp_server_close(&gCSV->gvcp);

	return ret;
}


#ifdef __cplusplus
}
#endif

