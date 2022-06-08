#ifndef __CSV_TCP_LOCAL_H__
#define __CSV_TCP_LOCAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_TCP_LOCAL					"'tcp_local'"	// to tcp pc client
#define NAME_TMR_BEAT_TCPL				"'tmr_beat_tcpl'"

#define CHECK_NET_BREAK_TIME			10

#define MAX_TCP_CONNECT					(1)

struct tcp_local_t {
	char			*name;

	uint16_t		port;				///< 监听端口
	int				fd_listen;			///< 监听口句柄
	uint8_t			accepted;			///< 已建立连接

	struct csv_beat_t	beat;
};

extern int csv_tcp_local_accept (void);

extern int csv_tcp_local_init (void);

extern int csv_tcp_local_deinit (void);



#ifdef __cplusplus
}
#endif


#endif /* __CSV_TCP_LOCAL_H__ */