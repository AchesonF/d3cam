#ifndef __CSV_TCP_H__
#define __CSV_TCP_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
	TCP_LOCAL		= (0),			///< 本地服务
	TCP_REMOTE		= (1),			///< 远程服务

	TOTAL_TCP
};


typedef int (*tcp_open)(void);
typedef int (*tcp_close)(void);
typedef int (*tcp_recv)(uint8_t *buf, int nbytes);
typedef int (*tcp_send)(uint8_t *buf, int nbytes);
typedef int (*tcp_beat_open)(void);
typedef int (*tcp_beat_close)(void);


struct csv_tcp_t {
	int					fd;				///< 描述符
	char				*name;			///< 名称
	int					index;			///< 通道索引
	uint32_t			cnnt_time;		///< 建链至今时间 s

	tcp_open			open;
	tcp_close			close;
	tcp_recv			recv;
	tcp_send			send;

	int					beat_fd;		///< 心跳描述符
	struct csv_beat_t	*pBeat;
	tcp_beat_open		beat_open;
	tcp_beat_close		beat_close;
};


extern int csv_tcp_reading_trigger (struct csv_tcp_t *pTCP);

extern int csv_tcp_init (void);

extern int csv_tcp_deinit (void);



#ifdef __cplusplus
}
#endif


#endif /* __CSV_TCP_H__ */



