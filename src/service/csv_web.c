
#ifdef __cplusplus
extern "C" {
#endif

// USING_WEB_THREAD define in cmake

#ifdef USING_WEB_THREAD

#include "webapp.h"
#include "inc_files.h"

static void *csv_web_loop (void *data)
{
	struct csv_web_t *pWEB = (struct csv_web_t *)data;

	maRunWebServer(pWEB->ConfigFile);

	log_alert("ALERT : exit pthread 'thr_web'.");

	pWEB->thr_web = 0;
	pthread_exit(NULL);

	return NULL;
}

static int csv_web_thread (struct csv_web_t *pWEB)
{
	int ret = 0;

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_mutex_init(&pWEB->mutex_web, NULL) != 0) {
		log_err("ERROR : mutex %s.", pWEB->name_web);
        return -1;
    }

	ret = pthread_create(&pWEB->thr_web, &attr, csv_web_loop, (void *)pWEB);
	if (ret < 0) {
		log_err("ERROR : create pthread %s.", pWEB->name_web);
		return -1;
	} else {
		log_info("OK : create pthread %s @ (%p).", pWEB->name_web, pWEB->thr_web);
	}

	//pthread_attr_destory(&attr);

	return ret;
}

static int csv_web_thread_cancel (struct csv_web_t *pWEB)
{
	int ret = 0;
	void *retval = NULL;

	if (pWEB->thr_web <= 0) {
		return 0;
	}

	ret = pthread_cancel(pWEB->thr_web);
	if (ret != 0) {
		log_err("ERROR : pthread_cancel %s.", pWEB->name_web);
	} else {
		log_info("OK : cancel pthread %s (%p).", pWEB->name_web, pWEB->thr_web);
	}

	ret = pthread_join(pWEB->thr_web, &retval);

	pWEB->thr_web = 0;

	return ret;
}

int csv_web_init (void)
{
	struct csv_web_t *pWEB = &gCSV->web;

	pWEB->name_web = NAME_THREAD_WEB;

	if ((isprint(gPdct.WebCfg[0]))&&(isprint(gPdct.WebCfg[1]))
	 &&(strstr(gPdct.WebCfg, ".conf") != NULL)) {
		pWEB->ConfigFile = gPdct.WebCfg;
		log_info("OK : Web config file set as : '%s'.", pWEB->ConfigFile);
	} else {
		log_info("OK : Use default web config file.");
		pWEB->ConfigFile = DEFAULT_WEB_CONFIG;
	}

	return csv_web_thread(pWEB);
}

int csv_web_deinit (void)
{
	return csv_web_thread_cancel(&gCSV->web);
}

#else

int csv_web_init (void)
{
	return 0;
}

int csv_web_deinit (void)
{

	return 0;
}

#endif

#ifdef __cplusplus
}
#endif


