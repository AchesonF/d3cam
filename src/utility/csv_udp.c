
#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

struct csv_udp_t gUDP;

static int csv_udp_target_update (struct csv_udp_t *pUDP)
{
    memset(&pUDP->peer, 0, sizeof(struct sockaddr_in));
    pUDP->peer.sin_family = AF_INET;
    pUDP->peer.sin_port = htons(pUDP->port);
    pUDP->peer.sin_addr.s_addr = inet_addr(pUDP->ip);

	return 0;
}

static int csv_udp_server_init (struct csv_udp_t *pUDP)
{
    int fd = 0;

	if (pUDP->initialled) {
		close(pUDP->fd);
		pUDP->fd = -1;
		pUDP->initialled = 0;
	}

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        log_err("ERROR : socket 'SOCK_DGRAM'.");
        return -1;
    }

	int ret = 0;
    int broadcast = 1;
    ret = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast,sizeof(int));
    if(ret < 0) {
		log_err("ERROR : setsockopt 'SO_BROADCAST'.");
		return -1;
	}

	csv_udp_target_update(pUDP);

	pUDP->fd = fd;
	pUDP->initialled = 1;

	log_info("OK : init %s as fd(%d).", pUDP->name, pUDP->fd);

    return fd;
}

int csv_udp_set_level (uint8_t enable, uint8_t level)
{
	struct csv_udp_t *pUDP = &gUDP;

	if (enable <= 1) {
		pUDP->enable = enable;
	} else {
		return -1;
	}

	if (level <= LOG_DEBUG) {
		pUDP->level = level;
	} else {
		return -1;
	}

	return 0;
}

int csv_udp_set_server (uint32_t ipaddr, uint16_t port)
{
    int ret = -1, wlen = 0;
	struct csv_udp_t *pUDP = &gUDP;
	char old_ip[32] = {0};
	uint16_t old_port = 0;
	struct in_addr addr;
	addr.s_addr = swap32(ipaddr);
	char str_serv[64] = {0};
	memset(str_serv, 0, 64);
	wlen = snprintf(str_serv, 64, "%s:%d", inet_ntoa(addr), port);

	strcpy(old_ip, pUDP->ip);
	old_port = pUDP->port;
	if (strcmp(old_ip, inet_ntoa(addr))||(old_port != port)) {
		ret = csv_file_write_data(gFILE.udpserv, (uint8_t *)str_serv, wlen+1);
    	strcpy(pUDP->ip, inet_ntoa(addr));
		pUDP->port = port;
		pUDP->reinit = 1;
		log_info("OK : new log serv is : '%s:%d'", inet_ntoa(addr), port);	// sendto old
		log_info("OK : old log serv is : '%s:%d'", old_ip, old_port);		// sendto new
	}

	return ret;
}

int csv_udp_sendto (const char *content)
{
	int ret = 0;
	static int err_cnt = 0;
	struct csv_udp_t *pUDP = &gUDP;

	if (!pUDP->initialled) {
		csv_udp_server_init(pUDP);
	}

	if (pUDP->fd < 0) {
		return -1;
	}

	char str_send[MAX_LINE+32] = {0};
	memset(str_send, 0, MAX_LINE+32);
	sprintf(str_send, "<%s>", pUDP->hostid);
	strcat(str_send, content);

    ret = sendto(pUDP->fd, str_send, strlen(str_send), 0, 
    	(struct sockaddr *)&pUDP->peer, sizeof(struct sockaddr_in));
    if (ret < 0) {
		if (err_cnt++ <= 5) {
			log_err("ERROR : sendto %s:%d.", pUDP->ip, pUDP->port);
		}
	} else {
		err_cnt = 0;
	}

	if (pUDP->reinit) {
		pUDP->reinit = 0;
		csv_udp_target_update(pUDP);
	}

    return ret;
}


int csv_udp_init (void)
{
	struct csv_udp_t *pUDP = &gUDP;

	memset(&gUDP, 0, sizeof(struct csv_udp_t));

	pUDP->fd = -1;
	pUDP->name = NAME_UDP_LOG;
	pUDP->enable = 0;
	pUDP->level = LOG_INFO;
	pUDP->initialled = 0;
	pUDP->reinit = 0;

	strcpy(pUDP->ip, DEFAULT_LOG_SERV);
	pUDP->port = DEFAULT_LOG_PORT;
	memset(pUDP->hostid, 0, 32);
	snprintf(pUDP->hostid, 32, "%ld", gethostid());

	csv_file_init();

	log_info("===========================================================\n");

	return 0;
}

int csv_udp_deinit (void)
{
	struct csv_udp_t *pUDP = &gUDP;

	if (pUDP->fd > 0) {
		close(pUDP->fd);

		log_info("OK : close %s fd(%d).", pUDP->name, pUDP->fd);
		pUDP->fd = -1;
	}

	return 0;
}


#ifdef __cplusplus
}
#endif



