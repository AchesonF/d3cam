#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif




int csv_tcp_local_beat_open (void)
{
	return csv_beat_timer_open(&gCSV->tcpl.beat);
}

static int csv_tcp_local_beat_close (void)
{
	return csv_beat_timer_close(&gCSV->tcpl.beat);
}

static int csv_tcp_local_open (void)
{
	int fd = -1, ret = 0;

	struct csv_tcp_t *pTCPL = &gCSV->tcpl;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		log_err("ERROR : socket %s.", pTCPL->name_listen);
		return -1;
	}

	ret = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (ret < 0) {
		log_err("ERROR : fnctl %s.", pTCPL->name_listen);
		close(fd);
		return ret;
	}

    int optval = 0;
    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (ret < 0) {
        log_err("ERROR : setsockopt %s.", pTCPL->name_listen);
        close(fd);
        return ret;
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));    
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(pTCPL->port);

	ret = bind(fd, (struct sockaddr *)&sin, sizeof(sin));
	if (ret < 0) {
		log_err("ERROR : bind %s.", pTCPL->name_listen);
		close(fd);
		sleep(5);
		exit(EXIT_FAILURE);
		return ret;
	}

	ret = listen(fd, MAX_TCP_CONNECT);
	if (ret < 0) {
		log_err("ERROR : listen %s.", pTCPL->name_listen);
		close(fd);
		return ret;
	}

	pTCPL->fd_listen = fd;

	log_info("OK : bind %s : '%d/tcp' as fd(%d).", 
		pTCPL->name_listen, pTCPL->port, pTCPL->fd_listen);

	return 0;
}

static int csv_tcp_local_release (struct csv_tcp_t *pTCPL)
{
	if (pTCPL->fd_listen > 0) {
		if (close(pTCPL->fd_listen) < 0) {
			log_err("ERROR : close %s.", pTCPL->name_listen);
			return -1;
		}

		log_info("OK : close %s fd(%d).", pTCPL->name_listen, pTCPL->fd_listen);
		pTCPL->fd_listen = -1;
	}

	return 0;
}

int csv_tcp_local_close (void)
{
	struct csv_tcp_t *pTCPL = &gCSV->tcpl;

	csv_tcp_local_beat_close();

	if (pTCPL->fd > 0) {
		close(pTCPL->fd);
		log_info("OK : close %s fd(%d). cnnt: %f s.", pTCPL->name, pTCPL->fd,
			(utility_get_sec_since_boot() - pTCPL->time_start));
	}

	pTCPL->fd = -1;
	pTCPL->accepted = false;

	return 0;
}

int csv_tcp_local_accept (void)
{
	int fd = -1, ret = 0;
	struct csv_tcp_t *pTCPL = &gCSV->tcpl;

	socklen_t sock_len = sizeof(pTCPL->peer);

	fd = accept(pTCPL->fd_listen, (struct sockaddr *)&pTCPL->peer, &sock_len);
	if (fd <= 0) {
		log_err("ERROR : accept %s.", pTCPL->name_listen);
		return fd;
	}

    struct linger so_linger;
    so_linger.l_onoff = true;
    so_linger.l_linger = 0;
    ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
    if (ret < 0) {
        log_err("ERROR : setsockopt 'SO_LINGER'.");
        close(fd);

        return ret;
    }

    // 断线探测
/*    int keepalive = 1;	// 开启 keepalive 属性
    int keepidle = 5;		// 如果5s内无任何数据往来，进行探测
    int keepinterval = 3;	// 探测时发包的时间间隔3s
    int keepcount = 2;	// 探测尝试的次数。如果第1次探测包有响应，则后续不再发。
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepalive, sizeof(keepalive));
    setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle, sizeof(keepidle));
    setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, (void*)&keepinterval, sizeof(keepinterval));
    setsockopt(fd, SOL_TCP, TCP_KEEPCNT, (void*)&keepcount, sizeof(keepcount));
*/
	int opt = 60*1024*1024;	// for send big data
	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(int));


	int err = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (err < 0) {
		log_err("ERROR : fcntl %s.", pTCPL->name);
		return err;
	}

	pTCPL->accepted = true;

	pTCPL->fd = fd;

	log_info("OK : %s accept '%s:%d' as fd(%d).", pTCPL->name,
            inet_ntoa(pTCPL->peer.sin_addr), htons(pTCPL->peer.sin_port), fd);

	pTCPL->time_start = utility_get_sec_since_boot();

	csv_beat_timer_open(&pTCPL->beat);

	return 0;
}

int csv_tcp_local_recv (uint8_t *buf, int nbytes)
{
	int ret = 0;
	uint32_t n_read = 0;
	uint32_t timeo = 0;
	struct csv_tcp_t *pTCPL = &gCSV->tcpl;

	if (pTCPL->fd <= 0) {
		return -3;
	}

	while (n_read < nbytes) {
		ret = recv(pTCPL->fd, buf+n_read, nbytes-n_read, 0);
		if (ret <= 0)
			break;
		n_read += ret;

		if (++timeo >= 100) { // timeout, maybe more
			return -3;
		}
	}

	if (ret == 0) {	/* EOF */
		return 0;
	} else if (ret < 0) {
		if (errno == EAGAIN) {
			log_hex(STREAM_TCP, buf, n_read, "tcpl recv [%d]", n_read);

			return n_read;
		}
		return -1;
	}

	log_hex(STREAM_TCP, buf, n_read, "tcpl recv [%d]", n_read);

	return n_read;
}

int csv_tcp_local_send (uint8_t *buf, int nbytes)
{
	int ret;
	uint32_t n_written = 0;
	struct csv_tcp_t *pTCPL = &gCSV->tcpl;
	static int snd_err = 0;

	if (pTCPL->fd <= 0) {
		return -1;
	}

	while (n_written < nbytes) {
		ret = send(pTCPL->fd, buf+n_written, nbytes-n_written, 0);
		if (ret < 0) {
			if (++snd_err <= 10) {
				log_err("ERROR : <%d> send.", snd_err);
			} else {
				pTCPL->accepted = false;
			}

			if (errno == EAGAIN) {
				log_hex(STREAM_TCP, buf, n_written, "tcpl send [%d]", n_written);
				return n_written;
			}
			return ret;
		}

		snd_err = 0;
		n_written += ret;
	}

	log_hex(STREAM_TCP, buf, n_written, "tcpl send [%d]", n_written);

	return n_written;
}


int csv_tcp_reading_trigger (struct csv_tcp_t *pTCPL)
{
	int nRead = 0;

	nRead = csv_tcp_local_recv(pTCPL->buf_recv, MAX_LEN_TCP_RCV);
	if (nRead == -1) {
		log_err("ERROR : %s recv (%d)", pTCPL->name, nRead);
		csv_tcp_local_close();
		pTCPL->len_recv = 0;
		return -1;
	} else if (nRead == 0) {
		log_warn("WARN : %s recv end-of-file.", pTCPL->name);
		csv_tcp_local_close();
		pTCPL->len_recv = 0;
		return 0;
	} else if (nRead < 0) {
		return -1;
	}

	pTCPL->len_recv = nRead;

	return csv_msg_check(pTCPL->buf_recv, pTCPL->len_recv);
}

int csv_tcp_init (void)
{
	int ret = 0;

	struct csv_tcp_t *pTCPL = &gCSV->tcpl;

	pTCPL->fd = -1;
	pTCPL->name = NAME_TCP_LOCAL;
	pTCPL->name_listen = NAME_TCP_LISTEN;
	pTCPL->port = PORT_TCP_LISTEN;
	pTCPL->fd_listen = -1;
	pTCPL->accepted = false;
	pTCPL->time_start = 0.0;

	pTCPL->beat.name = NAME_TMR_BEAT_TCPL;
	pTCPL->beat.timerfd = -1;
	pTCPL->beat.responsed = false;
	pTCPL->beat.cnt_timeo = 0;
	pTCPL->beat.millis = BEAT_PERIOD;

	ret = csv_tcp_local_open();

	return ret;
}

int csv_tcp_deinit (void)
{
	int ret = 0;

	ret = csv_tcp_local_close();

	ret |= csv_tcp_local_release(&gCSV->tcpl);

	return ret;
}

#ifdef __cplusplus
}
#endif


