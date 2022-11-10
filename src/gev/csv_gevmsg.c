#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static int csv_gevmsg_sendto (struct csv_gevmsg_t *pGVMSG)
{
	if ((pGVMSG->fd <= 0)||(pGVMSG->lenSend == 0)
	  ||(0x00000000 == pGVMSG->peer_addr.sin_addr.s_addr)
	  ||(0x0000 == pGVMSG->peer_addr.sin_port)) {
		return 0;
	}

	log_hex(STREAM_UDP, pGVMSG->bufSend, pGVMSG->lenSend, "gvmsg send[%d] '%s:%d'", pGVMSG->lenSend, 
		inet_ntoa(pGVMSG->peer_addr.sin_addr), htons(pGVMSG->peer_addr.sin_port));

	return sendto(pGVMSG->fd, pGVMSG->bufSend, pGVMSG->lenSend, 0, 
		(struct sockaddr *)&pGVMSG->peer_addr, sizeof(struct sockaddr_in));
}

int csv_gevmsg_package (struct csv_gevmsg_t *pGVMSG)
{
	pGVMSG->lenSend = 0;

	return csv_gevmsg_sendto(pGVMSG);
}


static int csv_gevmsg_client_open (struct csv_gevmsg_t *pGVMSG)
{
	int ret = 0;
	int fd = -1;

	if (pGVMSG->fd > 0) {
		ret = close(pGVMSG->fd);
		if (ret < 0) {
			log_err("ERROR : close %s.", pGVMSG->name);
		}
		pGVMSG->fd = -1;
	}

	struct sockaddr_in local_addr;
	socklen_t sin_size = sizeof(struct sockaddr);

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 ) {
		log_err("ERROR : socket %s.", pGVMSG->name);

		return -1;
	}

	memset((void*)&local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(0);
	bzero(&(local_addr.sin_zero), 8);

	ret = bind(fd, (struct sockaddr*)&local_addr, sizeof(struct sockaddr));
	if(ret < 0) {
		log_err("ERROR : bind %s.", pGVMSG->name);

		return -1;
	}

	int val = IP_PMTUDISC_DO; /* don't fragment */
	ret = setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));
	if (ret < 0) {
		log_err("ERROR : setsockopt 'IP_MTU_DISCOVER'.");

		return -1;
	}

	getsockname(fd, (struct sockaddr *)&local_addr, &sin_size);

	pGVMSG->fd = fd;
	pGVMSG->port = ntohs(local_addr.sin_port);

	log_info("OK : bind %s : '%d/udp' as fd(%d).", pGVMSG->name, 
		pGVMSG->port, pGVMSG->fd);

	csv_gev_reg_value_update(REG_MessageChannelSourcePort, pGVMSG->port);
	gCSV->cfg.gigecfg.MessageSourcePort = pGVMSG->port;

	return 0;
}

static int csv_gevmsg_client_close (struct csv_gevmsg_t *pGVMSG)
{
	if (NULL == pGVMSG) {
		return -1;
	}

	if (pGVMSG->fd > 0) {
		if (close(pGVMSG->fd) < 0) {
			log_err("ERROR : close %s.", pGVMSG->name);
			return -1;
		}
		log_info("OK : close %s fd(%d).", pGVMSG->name, pGVMSG->fd);
		pGVMSG->fd = -1;
	}

	return 0;
}

int csv_gevmsg_init (void)
{
	int ret = 0;
	struct csv_gevmsg_t *pGVMSG = &gCSV->gvmsg;

	pGVMSG->fd = -1;
	pGVMSG->name = NAME_SOCKET_GEV_MESSAGE;
	pGVMSG->port = 0;

	ret = csv_gevmsg_client_open(pGVMSG);


	return ret;
}

int csv_gevmsg_deinit (void)
{
	return csv_gevmsg_client_close(&gCSV->gvmsg);
}


#ifdef __cplusplus
}
#endif



