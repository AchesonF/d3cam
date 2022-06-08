#ifndef __CSV_BEAT_H__
#define __CSV_BEAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define BEAT_TIMEO				3		// 3 次
#define BEAT_PERIOD				(5000)	// 5s 周期

struct csv_beat_t {
	char				*name;
	int					timerfd;

	uint8_t				iftype;			///< 通道号
	uint8_t				responsed;		///< 有应答
	uint8_t				cnt_timeo;		///< 超时计数

	uint32_t			millis;			///< 发送周期

	struct itimerspec	its;
};


extern int csv_beat_timer_open (struct csv_beat_t *pBeat);

extern int csv_beat_timer_close (struct csv_beat_t *pBeat);

extern int csv_beat_timer_trigger (struct csv_beat_t *pBeat);



#ifdef __cplusplus
}
#endif


#endif /* __CSV_BEAT_H__ */