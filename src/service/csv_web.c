
#include "webapp.h"
#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

static void *csv_web_loop (void *data)
{
	maRunWebServer("webapp.conf");

	log_info("WARN : exit pthread 'thr_web'");
	pthread_exit(NULL);

	return NULL;
}

static int csv_web_thread (void)
{
	int ret = 0;

	pthread_t tid;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&tid, &attr, csv_web_loop, NULL);
	if (ret < 0) {
		log_err("ERROR : create pthread");
		ret = -1;
	} else {
		log_info("OK : create pthread as (%p)",  tid);
	}

	return ret;
}

int csv_web_init (void)
{
	return csv_web_thread();
}

#ifdef __cplusplus
}
#endif


