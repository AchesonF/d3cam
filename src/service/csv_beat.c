#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

int csv_beat_timer_open (struct csv_beat_t *pBeat)
{
	int fd = -1;

	if (pBeat == NULL) {
		return -1;
	}

	if (!pBeat->enable) {
		return 0;
	}

	fd = timerfd_create(CLOCK_REALTIME, 0);
	if (fd < 0) {
		log_err("ERROR : create hb %s.", pBeat->name);
		return -1;
	}

	pBeat->its.it_interval.tv_sec = pBeat->millis / 1000;
	pBeat->its.it_interval.tv_nsec = (pBeat->millis % 1000) * 1000000;
	pBeat->its.it_value.tv_sec = pBeat->millis / 1000;
	pBeat->its.it_value.tv_nsec = (pBeat->millis % 1000) * 1000000;

	if (timerfd_settime(fd, 0, &pBeat->its, NULL) < 0) {
		log_err("ERROR : settime timerfd %s.", pBeat->name);

		close(fd);
		return -1;
	}

	pBeat->timerfd = fd;
	pBeat->responsed = false;
	pBeat->cnt_timeo = 0;

	log_info("OK : create timerfd %s as fd(%d).", pBeat->name, fd);

	return fd;
}

int csv_beat_timer_close (struct csv_beat_t *pBeat)
{
	if (pBeat == NULL) {
		return -1;
	}

	if (pBeat->timerfd > 0) {
		if (close(pBeat->timerfd) < 0) {
			log_err("ERROR : close hb timer %s.", pBeat->name);
			return -1;
		}

		pBeat->timerfd = -1;

		log_info("OK : close hb timer %s.", pBeat->name);
	}

	return 0;
}

int csv_beat_timer_trigger (struct csv_beat_t *pBeat)
{
	if (pBeat == NULL) {
		return -1;
	}

	uint64_t num_exp = 0;
	if (read(pBeat->timerfd, &num_exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
		log_err("ERROR : read timerfd %s.", pBeat->name);
		return -2;
	}

	if (pBeat->responsed) {
		pBeat->cnt_timeo = 0;
	}

	if (++pBeat->cnt_timeo >= HEARTBEAT_TIMEO) {
		log_info("WARN : %s beat timeout.", pBeat->name);

		return csv_tcp_local_close();
	}

//	uhf_msg_heartbeat_package(&gCSV->msg.up_data);
//	uhf_msg_up_data(&gCSV->msg.up_data, pBeat->iftype);

	pBeat->responsed = false;

	return 0;
}



#ifdef __cplusplus
}
#endif

