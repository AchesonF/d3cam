#ifndef __CSV_MVS_H__
#define __CSV_MVS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_THREAD_MVS			("'thr_mvs'")

#define MAX_SUPPORT_CAMS		(4)		/* 左右上下四个位置 */

struct mvd_param_t {

};

struct csv_mvs_t {
	uint8_t					cnt_mvs;
	uint8_t					bExit;

	struct mvd_param_t		cam[MAX_SUPPORT_CAMS];

	const char				*name_mvs;		///< 消息
	pthread_t				thr_mvs;		///< ID
	pthread_mutex_t			mutex_mvs;		///< 锁
	pthread_cond_t			cond_mvs;		///< 条件
};


extern int csv_mvs_init (void);

extern int csv_mvs_deinit (void);



#ifdef __cplusplus
}
#endif

#endif /* __CSV_MVS_H__ */


