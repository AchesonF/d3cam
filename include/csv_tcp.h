#ifndef __CSV_TCP_H__
#define __CSV_TCP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_TCP_LISTEN					"'tcp_listen'"
#define NAME_TCP_LOCAL					"'tcp_local'"	// to tcp pc client
#define NAME_TMR_BEAT_TCPL				"'tmr_beat_tcpl'"

#define CHECK_NET_BREAK_TIME			10

#define PORT_TCP_LISTEN					(26666)
#define MAX_TCP_CONNECT					(1)

#define MAX_LEN_TCP_FRAME				(20*1024*1024)


struct csv_tcp_t {
	int					fd;					///< 描述符
	char				*name;				///< 名称
	char				*name_listen;		///< 监听名称
	uint16_t			port;				///< 监听端口
	int					fd_listen;			///< 监听口句柄
	uint8_t				accepted;			///< 已建立连接
	double				time_start;			///< 建链时刻 s

	struct csv_beat_t	beat;

	uint8_t				buf_recv[MAX_LEN_TCP_FRAME];
	uint32_t			len_recv;
};


extern int csv_tcp_reading_trigger (struct csv_tcp_t *pTCP);

extern int csv_tcp_local_close (void);

extern int csv_tcp_local_accept (void);

extern int csv_tcp_init (void);

extern int csv_tcp_deinit (void);



#ifdef __cplusplus
}
#endif


#endif /* __CSV_TCP_H__ */



