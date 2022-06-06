#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


static int csv_tick_timer_open (struct csv_tick_t *pTICK)
{
	int fd = -1;

	struct itimerspec its;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = pTICK->nanos;
	its.it_value.tv_sec =0;
	its.it_value.tv_nsec = pTICK->nanos;

	fd = timerfd_create(CLOCK_REALTIME, 0);
	if (fd < 0) {
		log_err("ERROR : create %s", pTICK->name);

		return -1;
	}

	if (timerfd_settime(fd, 0, &its, NULL) < 0) {
		log_err("ERROR : settime %s", pTICK->name);
		if (close(fd)<0) {
			log_err("ERROR : close %s", pTICK->name);

			return -3;
		}

		log_info("OK : close %s", pTICK->name);

		return -2;
	}

	pTICK->fd = fd;
	log_info("OK : create timerfd %s as fd(%d).", pTICK->name, pTICK->fd);

	return 0;
}

static int csv_tick_timer_close (struct csv_tick_t *pTICK)
{
	if (pTICK->fd > 0) {
		if (close(pTICK->fd) < 0) {
			log_err("ERROR : close %s", pTICK->name);
			return -1;
		}

		pTICK->fd = -1;
		log_info("OK : close timerfd %s.", pTICK->name);
	}

	return 0;
}

int csv_tick_timer_trigger (struct csv_tick_t *pTICK)
{
	struct csv_product_t *pPdct = &gPdct;

	uint64_t num_exp = 0;
	if (read(pTICK->fd, &num_exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
		log_err("ERROR : read %s", pTICK->name);
		return -1;
	}

	if (pTICK->cnt%(60*TICKS_PER_SECOND) == 0) {
		log_info("R : %03dD %02dH:%02dM", pPdct->app_runtime/86400,
			pPdct->app_runtime%86400/3600, pPdct->app_runtime%86400%3600/60);

		if (pTICK->cnt > 0) {
			csv_life_update();
		}
	}


	pTICK->cnt++;

	if (pTICK->cnt%TICKS_PER_SECOND == 0) {
		pPdct->app_runtime++;

		csv_hb_write();
	}

	if (pTICK->cnt%(5*TICKS_PER_SECOND) == 0) {
		csv_stat_update();
//		log_info("%d", pPdct->app_runtime);
	}

	return 0;
}


int csv_tick_init (void)
{
	struct csv_tick_t *pTICK = &gCSV->tick;

	pTICK->fd = -1;
	pTICK->name = NAME_TICK_TIMER;
	pTICK->nanos = NANOS_PER_TICK;
	pTICK->cnt = 0;

	return csv_tick_timer_open(pTICK);
}

int csv_tick_deinit (void)
{
	return csv_tick_timer_close(&gCSV->tick);
}


#ifdef __cplusplus
}
#endif



