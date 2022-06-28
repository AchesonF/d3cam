#ifndef __CSV_TICK_H__
#define __CSV_TICK_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @name    主定时器接口宏定义
* @{ */
#define NAME_TICK_TIMER			("'tmr_tick'")
/** @} */

/** @name    Tick 定时器参数宏定义
* @{ */
#define NANOS_PER_SECOND		(1000000000L)	///< 每秒含纳秒数
#define NANOS_PER_TICK			(100000000L)	///< 每个tick 含纳秒数
#define TICKS_PER_SECOND		(NANOS_PER_SECOND/NANOS_PER_TICK)	///< 每秒含tick 数
/** @} */

/** @name    主定时器的结构体参数
* @{ */
struct csv_tick_t {
	int				fd;						///< 定时器文件描述符
	char			*name;					///< 名称
	uint32_t		nanos;					///< 纳秒数。强制认证小于1s(NANOS_PER_SECOND)
	uint32_t		cnt;					///< tick 计数
};
/** @} */


/* 定时器触发功能函数 */
extern int csv_tick_timer_trigger (struct csv_tick_t *pTICK);

extern int csv_tick_init (void);

extern int csv_tick_deinit (void);


#ifdef __cplusplus
}
#endif

#endif	/* __CSV_TICK_H__ */


