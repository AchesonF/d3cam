#include "inc_files.h"


#ifdef __cplusplus
extern "C" {
#endif






static int csv_gvcp_sendto (struct csv_gvcp_t *pGVCP)
{
	socklen_t from_len = sizeof(struct sockaddr_in);

	//log_data("udp send", pGVCP->Send.pTx, pGVCP->Send.nTx);



	return sendto(pGVCP->fd, pGVCP->Send.pTx, pGVCP->Send.nTx, 0, 
		(struct sockaddr *)&pGVCP->from_addr, from_len);
}


int csv_gvcp_trigger (struct csv_gvcp_t *pGVCP)
{
	uint8_t *pBuf = pGVCP->Recv.pRx;
	socklen_t from_len = sizeof(struct sockaddr_in);

	pGVCP->Recv.nRx = recvfrom(pGVCP->fd, pBuf, SIZE_UDP_BUFF, 0, 
		(struct sockaddr *)&pGVCP->from_addr, &from_len);

	if (pGVCP->Recv.nRx < sizeof(struct gvcp_cmd_header_t)) {
		log_hex(STREAM_UDP, pBuf, pGVCP->Recv.nRx, "wrong gvcp head length.");
		return -1;
	}

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

	return 0;
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

