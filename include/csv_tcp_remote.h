#ifndef __CSV_TCP_REMOTE_H__
#define __CSV_TCP_REMOTE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_TCP_REMOTE					"'tcp_remote'"	//  tcp connect to remote pc host 
#define NAME_TMR_BEAT_TCPR				"'tmr_beat_tcpr'"
#define NAME_THREAD_TCPR_CNNT			"'thr_cnnt_tcpr'"

struct tcp_remote_t {
	char				*name;
	uint8_t				connected;

	uint16_t			port;					///< 远程接口端口号
	char				ip[MAX_LEN_IP];			///< 远程IP 地址

	const char			*name_tcpr_cnnt;		///< 主动建链
	pthread_t			thr_tcpr_cnnt;

	struct csv_beat_t	beat;
};


extern int csv_tcp_remote_init (void);

extern int csv_tcp_remote_deinit (void);



#ifdef __cplusplus
}
#endif


#endif /* __CSV_TCP_REMOTE_H__ */