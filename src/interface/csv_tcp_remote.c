#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


static int csv_tcp_retmote_connect_try (struct tcp_remote_t *pTCPR)
{
	int fd = -1, ret = 0;
	struct csv_tcp_t *pTCP = &gCSV->tcp[TCP_REMOTE];

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		log_err("ERROR : socket %s", pTCPR->name);
		close(fd);
		return -1;
	}

	struct sockaddr_in dest;
	bzero(&dest, sizeof (dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(pTCPR->port);
	dest.sin_addr.s_addr = inet_addr(pTCPR->ip);

	//log_debug("Connecting to '%s:%d'", pTCPR->ip, pTCPR->port);

	// 设置成非阻塞模式创建连接
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		log_err("ERROR : fcntl %s", pTCPR->name);
		close(fd);
		return -1;
	}

	/* you can see "man connect" sub EINPROGRESS, why do this below. */
	int n = connect(fd, (struct sockaddr *) &dest, sizeof (dest));
	if (n < 0) {
		if ((errno != EINPROGRESS)&&(errno != EWOULDBLOCK)) {
			close(fd);
			log_err("ERROR : connect %s", pTCPR->name);
			return -2;
		}

		struct timeval tv;
		tv.tv_sec = 5;	// can't be too short
		tv.tv_usec = 0;
		fd_set wset;
		FD_ZERO(&wset);

		if (fd > 0) {
			FD_SET(fd, &wset);
		}

		ret = select(fd + 1, NULL, &wset, NULL, &tv);
		if (1 == ret) {
			socklen_t optlen;
			int optval = 0;
			ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
			if (ret < 0) {
				log_err("ERROR : getsockopt %s", pTCPR->name);
				close(fd);
				return -3;
			}

			//log_debug("optval %d", optval);

			if (0 == optval) {
				pTCP->fd = fd;
				pTCPR->connected = true;
			} else {
				close(fd);
				pTCP->fd = -1;
				pTCPR->connected = false;
				//log_err("getsockopt");

				return -4;
			}
		} else {
			close(fd);
			return -1;
		}
	}else {
		close(fd);
		return -1;
	}

	int keepalive = 1; // 开启 keepalive 属性
	int keepidle = 5; // 如果5s内无任何数据往来，进行探测
	int keepinterval = 3; // 探测时发包的时间间隔3s
	int keepcount = 2; // 探测尝试的次数。如果第1次探测包有响应，则后1次不再发。
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*) &keepalive, sizeof (keepalive));
	setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*) &keepidle, sizeof (keepidle));
	setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void*) &keepinterval, sizeof (keepinterval));
	setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void*) &keepcount, sizeof (keepcount));

	pTCP->beat_fd = pTCP->beat_open();
	log_info("OK : Connected '%s:%d' as fd(%d).",  
		pTCPR->ip, pTCPR->port, pTCP->fd);

	pTCP->cnnt_time = 0;

	return 0;
}

static void *csv_tcp_remote_cnnt_loop (void *data)
{
	if (NULL == data) {
		return NULL;
	}

	struct tcp_remote_t *pTCPR = (struct tcp_remote_t *)data;

	while (1) {
    	sleep(1);
		if (!pTCPR->connected) {
			csv_tcp_retmote_connect_try(pTCPR);
		}
	}

	log_info("WARN : exit pthread %s", pTCPR->name_tcpr_cnnt);
	pthread_exit(NULL);

	return NULL;
}

static int csv_tcp_remote_cnnt_thread (struct tcp_remote_t *pTCPR)
{
	int ret = 0;

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&pTCPR->thr_tcpr_cnnt, &attr, 
		csv_tcp_remote_cnnt_loop, (void *)pTCPR);
	if (ret < 0) {
		log_info("ERROR : create pthread %s", pTCPR->name_tcpr_cnnt);
		ret = -1;
	} else {
		log_info("OK : create pthread %s %d @ (%p)", 
			pTCPR->name_tcpr_cnnt, getpid(), pTCPR->thr_tcpr_cnnt);
	}

	return ret;
}

static int csv_tcp_remote_cnnt_thread_cancel (struct tcp_remote_t *pTCPR)
{
	int ret = 0;
	void *retval = NULL;

	ret = pthread_cancel(pTCPR->thr_tcpr_cnnt);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s", pTCPR->name_tcpr_cnnt);
	} else {
		log_info("OK : cancel pthread %s", pTCPR->name_tcpr_cnnt);
	}

	ret = pthread_join(pTCPR->thr_tcpr_cnnt, &retval);

	return ret;
}

static int csv_tcp_remote_open (void)
{
	return csv_tcp_remote_cnnt_thread(&gCSV->tcp_r);
}

static int csv_tcp_remote_close (void)
{
	struct csv_tcp_t *pTCP = &gCSV->tcp[TCP_REMOTE];
	struct tcp_remote_t *pTCPR = &gCSV->tcp_r;

	if (pTCP->fd > 0) {
		close(pTCP->fd);
	}

	pTCP->fd = -1;
	pTCP->beat_close();
	pTCP->beat_fd = -1;
	pTCPR->connected = false;

	log_info("OK : close %s", pTCP->name);

	return 0;
}

static int csv_tcp_remote_recv (uint8_t *buf, int nbytes)
{
	int ret = 0;
	uint32_t n_read = 0;
	struct csv_tcp_t *pTCP = &gCSV->tcp[TCP_REMOTE];

	if (pTCP->fd <= 0) {
		return -1;
	}

	while (n_read < nbytes) {
		ret = recv(pTCP->fd, buf+n_read, nbytes-n_read, 0);
		if (ret <= 0)
			break;
		n_read += ret;
	}

	if (ret == 0) {	/* EOF */
		return 0;
	} else if (ret < 0) {
		if (errno == EAGAIN) {
			log_hex(buf, n_read,"tcpr recv");

			return n_read;
		}
		return -1;
	}

	log_hex(buf, n_read, "tcpr recv");

	return n_read;
}

static int csv_tcp_remote_send (uint8_t *buf, int nbytes)
{
	int ret;
	uint32_t n_written = 0;
	struct csv_tcp_t *pTCP = &gCSV->tcp[TCP_REMOTE];
	struct tcp_remote_t *pTCPR = &gCSV->tcp_r;

	if (pTCP->fd <= 0) {
		return -1;
	}

	static int snd_err = 0;
	while (n_written < nbytes) {
		ret = send(pTCP->fd, buf+n_written, nbytes-n_written, 0);
		if (ret < 0) {
			if (++snd_err <= 3) {
				log_err("ERROR : <%d> send", snd_err);
			} else {
				pTCPR->connected = false;
			}

			if (errno == EAGAIN) {
				log_hex(buf, n_written, "tcpr send");
				return n_written;
			}
			return ret;
		}

		snd_err = 0;
		n_written += ret;
	}

	log_hex(buf, n_written, "tcpr send");

	return n_written;
}

static int csv_tcp_remote_beat_open (void)
{
	return csv_beat_timer_open(&gCSV->tcp_r.beat);
}

static int csv_tcp_remote_beat_close (void)
{
	return csv_beat_timer_close(&gCSV->tcp_r.beat);
}

int csv_tcp_remote_init (void)
{
	struct csv_tcp_t *pTCP = &gCSV->tcp[TCP_REMOTE];
	struct tcp_remote_t *pTCPR = &gCSV->tcp_r;

	pTCPR->name = NAME_TCP_REMOTE;
	pTCPR->connected = false;
	pTCPR->name_tcpr_cnnt = NAME_THREAD_TCPR_CNNT;
	pTCPR->port = 9091;	// must from config file
	memcpy(pTCPR->ip, "18.0.4.230", MAX_LEN_IP);

	pTCPR->beat.name = NAME_TMR_BEAT_TCPR;
	pTCPR->beat.timerfd = -1;
	pTCPR->beat.iftype = TCP_REMOTE;
	pTCPR->beat.responsed = false;
	pTCPR->beat.cnt_timeo = 0;
	pTCPR->beat.millis = BEAT_PERIOD;

	pTCP->fd = -1;
	pTCP->name = NAME_TCP_REMOTE;
	pTCP->index = TCP_REMOTE;
	pTCP->cnnt_time = 0;
	pTCP->open = csv_tcp_remote_open;
	pTCP->close = csv_tcp_remote_close;
	pTCP->recv = csv_tcp_remote_recv;
	pTCP->send = csv_tcp_remote_send;
	pTCP->beat_fd = -1;
	pTCP->pBeat = &pTCPR->beat;
	pTCP->beat_open = csv_tcp_remote_beat_open;
	pTCP->beat_close = csv_tcp_remote_beat_close;

	return pTCP->open();
}

int csv_tcp_remote_deinit (void)
{
	csv_tcp_remote_beat_close();

	return csv_tcp_remote_cnnt_thread_cancel(&gCSV->tcp_r);
}


#ifdef __cplusplus
}
#endif


