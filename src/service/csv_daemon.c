#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

// 启动时，创建一个子进程。
// 该子进程启动 udp 服务，监听父进程特定数据作为父进程存活依据.
// 子进程在接收数据超时后，认为父进程已挂起或异常退出。
// 此时，子进程再次启动父进程并退出自己。
// 如此循环。


static int csv_daemon_client_check_localloop (void)
{
	int fd = -1;
	struct ifreq ifr;
	char *lp = "lo";

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		log_err("ERROR : socket interface '%s'.", lp);
		return -1;
	}

	strcpy(ifr.ifr_name, lp);
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
		log_err("ERROR : ioctl SIOCGIFFLAGS '%s'", lp);
		close(fd);

		return -1;
	}

	// up 本地回环
	if (!(ifr.ifr_flags & IFF_RUNNING)) {
		return system("ifconfig lo up");
	}

	return 0;
}


static int csv_daemon_client_init (struct csv_daemon_t *pDAEMON)
{
	int ret = 0;
	int fd = -1;

	ret = csv_daemon_client_check_localloop();
	if (ret < 0) {
		return -1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 ) {
		log_err("ERROR : socket");

		return -1;
	}

	pDAEMON->fd_client = fd;
	log_info("OK : open %s udp as fd(%d).", pDAEMON->name_client, fd);

	bzero(&pDAEMON->target_addr, sizeof(struct sockaddr_in));
	pDAEMON->target_addr.sin_family = AF_INET; 
	pDAEMON->target_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	pDAEMON->target_addr.sin_port = htons(pDAEMON->port); 

	return 0;
}

int csv_daemon_client_feed (struct csv_daemon_t *pDAEMON)
{
	if (pDAEMON->fd_client > 0) {
		sendto(pDAEMON->fd_client, &pDAEMON->beacon, sizeof(uint32_t), 0, 
			(struct sockaddr *)&pDAEMON->target_addr, sizeof(struct sockaddr_in));

		return 0;
	}

	return -1;
}

static int csv_daemon_server_create (struct csv_daemon_t *pDAEMON)
{
	int ret = 0;
	int fd = -1;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( fd < 0 ) {
		log_err("ERROR : socket");

		return -1;
	}

	memset((void*)&pDAEMON->local_addr, 0, sizeof(struct sockaddr_in));
	pDAEMON->local_addr.sin_family = AF_INET;
	pDAEMON->local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	pDAEMON->local_addr.sin_port = htons(pDAEMON->port);
	bzero(&pDAEMON->local_addr.sin_zero, 8);

	ret = bind(fd, (struct sockaddr*)&pDAEMON->local_addr, sizeof(struct sockaddr));
	if(ret < 0) {
		log_err("ERROR : bind");
		exit(-2);
		return -2;
	}

    ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (ret < 0) {
        log_err("ERROR : fcntl");
        close(fd);
        return ret;
    }

	pDAEMON->fd_server = fd;
	log_info("OK : bind %s %d/udp as fd(%d).", 
		pDAEMON->name_server, pDAEMON->port, fd);

	return 0;
}

static int csv_daemon_server_recvfrom (struct csv_daemon_t *pDAEMON)
{
	int r_len = 0;
	uint8_t r_buf[256] = {0};
	struct sockaddr_in from_addr;
	socklen_t from_len = sizeof(struct sockaddr_in);

	r_len = recvfrom(pDAEMON->fd_server, r_buf, 256, 0, 
		(struct sockaddr *)&from_addr, &from_len);

	if (r_len > 0) {
		if (pDAEMON->beacon == swap32(u8v_to_u32(r_buf))) {
			return 0;
		}
		//log_hex("R", r_buf, r_len);
	}

	return -1;
}

static int csv_daemon_server_cmdline_get (struct csv_daemon_t *pDAEMON)
{
	int ret = 0;
	char file[128] = {0};
	char cmd_buf[MAX_LEN_CMDLINE] = {0};
	char argv0[32] = {0};
	char argv1[32] = {0};
	char argv2[32] = {0};

	memset(pDAEMON->cmdline, 0, MAX_LEN_CMDLINE);
	memset(cmd_buf, 0, MAX_LEN_CMDLINE);
	memset(file, 0, 128);
	snprintf(file, 128, "/proc/%d/cmdline", pDAEMON->pid);

	int fd = -1;

	fd = open(file, O_RDONLY|O_SYNC);
	if (fd < 0) {
		log_err("ERROR : open '%s'", file);
		return -1;
	}


	ret = read(fd, cmd_buf, MAX_LEN_CMDLINE);
	if (ret < 0) {
		log_err("ERROR : read '%s'", file);
		close(fd);
		return -1;
	}

	int i = 0;
	for (i = 0; i < ret-1; i++) {
		if (0x00 == cmd_buf[i]) {
			cmd_buf[i] = ' ';
		}
	}
	ret = sscanf(cmd_buf, "%s %s %s", argv0, argv1, argv2);

	switch (ret) {
	case 1:
		snprintf(pDAEMON->cmdline, MAX_LEN_CMDLINE, "%s &", argv0);
	break;
	case 2:
		snprintf(pDAEMON->cmdline, MAX_LEN_CMDLINE, "%s %s &", argv0, argv1);
	break;
	case 3:
		snprintf(pDAEMON->cmdline, MAX_LEN_CMDLINE, "%s %s %s &", argv0, argv1, argv2);
	break;
	default:
		log_err("ERROR : scanf");
		return -1;
	break;
	}

	log_info("OK : get cmdline : '%s'", pDAEMON->cmdline);

	return 0;
}

static int csv_daemon_server_init (struct csv_daemon_t *pDAEMON)
{
	struct timeval tv;
	fd_set readset;
	pid_t pid = pDAEMON->pid;
	int timeo = 0;
	int ret = 0;

	pDAEMON->pid = getpid();
	log_info("OK : child pid(%d) from parent pid(%d).", pDAEMON->pid, pid);

	ret = csv_daemon_server_create(pDAEMON);
	if (ret < 0) {
		return -1;
	}

	csv_daemon_server_cmdline_get(pDAEMON);

	while (1) {
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		FD_ZERO(&readset);

		if (pDAEMON->fd_server > 0) {
			FD_SET(pDAEMON->fd_server, &readset);
		}

		ret = select(pDAEMON->fd_server + 1, &readset, NULL, NULL, &tv);
		if (ret < 0) {
			log_err("ERROR : select", ret);
			continue;
		} else if (ret == 0) {
			if (++timeo >= DAEMON_TIMEO) {
				close(pDAEMON->fd_server);
				if (isprint(pDAEMON->cmdline[0])) {
					ret = system(pDAEMON->cmdline);
					log_info("OK : re-launch '%s'", pDAEMON->cmdline);
				}

				log_info("OK : pid(%d) says 'byebye'.", pDAEMON->pid);
				exit(ret);
			}

			continue;
		}

		if (FD_ISSET(pDAEMON->fd_server, &readset)){
			ret = csv_daemon_server_recvfrom(pDAEMON);
			if (ret == 0) {
				timeo = 0;
			}
		}
	}

	// never go here
	return 0;
}

int csv_daemon_init (int argc, char **argv)
{
	pid_t pid = 0;

	struct csv_daemon_t *pDAEMON = &gPdct.daemon;

	pDAEMON->name_server = NAME_DAEMON_SERVER;
	pDAEMON->name_client = NAME_DAEMON_CLIENT;
	pDAEMON->fd_server = -1;
	pDAEMON->fd_client = -1;
	pDAEMON->port = PORT_DAEMON_UDP;
	pDAEMON->beacon = DAEMON_BEACON;
	pDAEMON->pid = getpid();

	pid = fork();

	switch (pid) {
	case 0:		// child process
		csv_daemon_server_init(pDAEMON);
	break;
	case -1:	// failure
		log_err("ERROR : fork");
		
	break;
	default:	// parent process
	{
		log_info("OK : parent pid(%d) fork child pid(%d)",  
			pDAEMON->pid, pid);
		csv_daemon_client_init(pDAEMON);
#if 0
		int cnt = 0;
		while (1) {
			sleep(1);
			if (++cnt < 5) {	// test
				csv_daemon_client_feed(pDAEMON);
			} else {
				log_info("OK : pid(%d) says 'byebye'.", pDAEMON->pid);
				exit(1);
			}
		}
#endif
	}
	break;
	}

	return 0;
}


#ifdef __cplusplus
}
#endif



