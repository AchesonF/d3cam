#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


static int csv_tcp_local_open (void)
{
	int err = 0, fd = -1, ret = 0;

	struct tcp_local_t *pTCPL = &gCSV->tcp_l;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		log_err("ERROR : socket %s", pTCPL->name);
		return -1;
	}

	err = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (err < 0) {
		log_err("ERROR : fnctl %s", pTCPL->name);
		close(fd);
		return err;
	}

    int optval = 0;
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (ret < 0) {
        log_err("ERROR : setsockopt %s", pTCPL->name);
        close(fd);
        return ret;
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));    
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(pTCPL->port);

	err = bind(fd, (struct sockaddr *)&sin, sizeof(sin));
	if (err < 0) {
		log_err("ERROR : bind %s", pTCPL->name);
		close(fd);
		return err;
	}

	err = listen(fd, MAX_TCP_CONNECT);
	if (err < 0) {
		log_err("ERROR : listen %s", pTCPL->name);
		close(fd);
		return err;
	}

	pTCPL->fd_listen = fd;

	log_info("OK : bind %s : '%d/tcp' as fd(%d).", 
		pTCPL->name, pTCPL->port, pTCPL->fd_listen);

	return 0;
}

static int csv_tcp_local_release (struct tcp_local_t *pTCPL)
{
	if (pTCPL->fd_listen > 0) {
		if (close(pTCPL->fd_listen) < 0) {
			log_err("ERROR : close %s", pTCPL->name);
			return -1;
		}

		pTCPL->fd_listen = -1;
		log_info("OK : close %s.", pTCPL->name);
	}

	return 0;
}

static int csv_tcp_local_close (void)
{
	struct csv_tcp_t *pTCP = &gCSV->tcp[TCP_LOCAL];
	struct tcp_local_t *pTCPL = &gCSV->tcp_l;

	if (pTCP->fd > 0) {
		close(pTCP->fd);
	}

	pTCP->fd = -1;
	pTCP->beat_close();
	pTCP->beat_fd = -1;
	pTCPL->accepted = false;

	log_info("OK : close %s", pTCP->name);

	return 0;
}

int csv_tcp_local_accept (void)
{
	int fd = -1, ret = 0;
	struct csv_tcp_t *pTCP = &gCSV->tcp[TCP_LOCAL];
	struct tcp_local_t *pTCPL = &gCSV->tcp_l;

	struct sockaddr_in peer;
	socklen_t sock_len = sizeof(peer);

	fd = accept(pTCPL->fd_listen, (struct sockaddr *)&peer, &sock_len);
	if (fd <= 0) {
		log_err("ERROR : accept %s", pTCPL->name);
		return fd;
	}

    struct linger so_linger;
    so_linger.l_onoff = true;
    so_linger.l_linger = 0;
    ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
    if (ret < 0) {
        log_err("ERROR : setsockopt %s", pTCP->name);
        close(fd);

        return ret;
    }

    // 断线探测
    int keepalive = 1;	// 开启 keepalive 属性
    int keepidle = 5;		// 如果5s内无任何数据往来，进行探测
    int keepinterval = 3;	// 探测时发包的时间间隔3s
    int keepcount = 2;	// 探测尝试的次数。如果第1次探测包有响应，则后续不再发。
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepalive, sizeof(keepalive));
    setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle, sizeof(keepidle));
    setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void*)&keepinterval, sizeof(keepinterval));
    setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void*)&keepcount, sizeof(keepcount));

	int err = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (err < 0) {
		log_err("ERROR : fcntl %s", pTCPL->name);
		return err;
	}

	pTCPL->accepted = true;

	pTCP->fd = fd;

	pTCP->beat_fd = pTCP->beat_open();

	log_info("OK : %s accept '%s:%d' as fd(%d).", pTCP->name,
            inet_ntoa(peer.sin_addr), htons(peer.sin_port), fd);

	pTCP->cnnt_time = 0;

	return 0;
}

static int csv_tcp_local_recv (uint8_t *buf, int nbytes)
{
	int ret = 0;
	uint32_t n_read = 0;
	struct csv_tcp_t *pTCP = &gCSV->tcp[TCP_LOCAL];

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
			log_hex(buf, n_read, "tcpl recv");

			return n_read;
		}
		return -1;
	}

	log_hex(buf, n_read, "tcpl recv");

	return n_read;
}

static int csv_tcp_local_send (uint8_t *buf, int nbytes)
{
	int ret;
	uint32_t n_written = 0;
	struct csv_tcp_t *pTCP = &gCSV->tcp[TCP_LOCAL];
	struct tcp_local_t *pTCPL = &gCSV->tcp_l;
	static int snd_err = 0;

	if (pTCP->fd <= 0) {
		return -1;
	}

	while (n_written < nbytes) {
		ret = send(pTCP->fd, buf+n_written, nbytes-n_written, 0);
		if (ret < 0) {
			if (++snd_err <= 3) {
				log_err("ERROR : <%d> send", snd_err);
			} else {
				pTCPL->accepted = false;
			}

			if (errno == EAGAIN) {
				log_hex(buf, n_written, "tcpl send");
				return n_written;
			}
			return ret;
		}

		snd_err = 0;
		n_written += ret;
	}

	log_hex(buf, n_written, "tcpl send");

	return n_written;
}

static int csv_tcp_local_beat_open (void)
{
	return csv_beat_timer_open(&gCSV->tcp_l.beat);
}

static int csv_tcp_local_beat_close (void)
{
	return csv_beat_timer_close(&gCSV->tcp_l.beat);
}

int csv_tcp_local_init (void)
{
	struct csv_tcp_t *pTCP = &gCSV->tcp[TCP_LOCAL];
	struct tcp_local_t *pTCPL = &gCSV->tcp_l;

	pTCPL->name = NAME_TCP_LOCAL;
	pTCPL->port = 9090;
	pTCPL->fd_listen = -1;
	pTCPL->accepted = false;

	pTCPL->beat.name = NAME_TMR_BEAT_TCPL;
	pTCPL->beat.timerfd = -1;
	pTCPL->beat.iftype = TCP_LOCAL;
	pTCPL->beat.responsed = false;
	pTCPL->beat.cnt_timeo = 0;
	pTCPL->beat.millis = BEAT_PERIOD;

	pTCP->fd = -1;
	pTCP->name = NAME_TCP_LOCAL;
	pTCP->index = TCP_LOCAL;
	pTCP->cnnt_time = 0;
	pTCP->open = csv_tcp_local_open;
	pTCP->close = csv_tcp_local_close;
	pTCP->recv = csv_tcp_local_recv;
	pTCP->send = csv_tcp_local_send;
	pTCP->beat_fd = -1;
	pTCP->pBeat = &pTCPL->beat;
	pTCP->beat_open = csv_tcp_local_beat_open;
	pTCP->beat_close = csv_tcp_local_beat_close;

	return pTCP->open();
}

int csv_tcp_local_deinit (void)
{
	csv_tcp_local_beat_close();

	return csv_tcp_local_release(&gCSV->tcp_l);
}


#ifdef __cplusplus
}
#endif


