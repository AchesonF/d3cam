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
//		log_err(LOG_FMT"ERROR : create %s", LOG_ARGS, pTICK->name);

		return -1;
	}

	if (timerfd_settime(fd, 0, &its, NULL) < 0) {
//		log_err(LOG_FMT"ERROR : settime %s", LOG_ARGS, pTICK->name);
		if (close(fd)<0) {
//			log_err(LOG_FMT"ERROR : close %s", LOG_ARGS, pTICK->name);

			return -3;
		}

//		log_info(LOG_FMT"OK : close %s", LOG_ARGS, pTICK->name);

		return -2;
	}

	pTICK->fd = fd;
//	log_info(LOG_FMT"OK : create timerfd %s as fd(%d).", LOG_ARGS, pTICK->name, pTICK->fd);

	return 0;
}

static int csv_tick_timer_close (struct csv_tick_t *pTICK)
{
	if (pTICK->fd > 0) {
		if (close(pTICK->fd) < 0) {
//			log_err(LOG_FMT"ERROR : close %s", LOG_ARGS, pTICK->name);
			return -1;
		}

		pTICK->fd = -1;
//		log_info(LOG_FMT"OK : close timerfd %s.", LOG_ARGS, pTICK->name);
	}

	return 0;
}

int csv_tick_timer_trigger (struct csv_tick_t *pTICK)
{
	struct csv_product_t *pPdct = &gPdct;

	uint64_t num_exp = 0;
	if (read(pTICK->fd, &num_exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
//		log_err(LOG_FMT"ERROR : read %s", LOG_ARGS, pTICK->name);
		return -1;
	}

	if (pTICK->cnt%(60*TICKS_PER_SECOND) == 0) {
//		log_info(LOG_FMT"R : %03dD %02dH:%02dM", LOG_ARGS, pDEV->app_runtime/86400,
//			pDEV->app_runtime%86400/3600, pDEV->app_runtime%86400%3600/60);

		if (pTICK->cnt > 0) {
			csv_life_update();
		}
	}


	pTICK->cnt++;

	if (pTICK->cnt%TICKS_PER_SECOND == 0) {
		pPdct->app_runtime++;
printf("%d\n", pPdct->app_runtime);
	}

	if (pTICK->cnt%(5*TICKS_PER_SECOND) == 0) {
//		csv_stat_update();
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



