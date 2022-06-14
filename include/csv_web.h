#ifndef __CSV_WEB_H__
#define __CSV_WEB_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_THREAD_WEB			"'thr_web'"

#define DEFAULT_WEB_CONFIG		"webapp.conf"

struct csv_web_t {
	char					*ConfigFile;

	const char				*name_web;		///< 消息
	pthread_t				thr_web;		///< ID
	pthread_mutex_t			mutex_web;		///< 锁
};

extern int csv_web_init (void);

extern int csv_web_deinit (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_WEB_H__ */


