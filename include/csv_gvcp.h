#ifndef __CSV_GVCP_H__
#define __CSV_GVCP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_SOCKET_GVCP			"'udp_gvcp'"
#define NAME_THREAD_GVCP_ASK		"'thr_gvcpask'"


struct gvcp_ask_t {
	uint32_t				len;
	uint8_t					buf[GVCP_MAX_MSG_LEN];
	struct sockaddr_in		from_addr;		///< 来源地址参数
};

struct gvcp_ask_list_t {
	struct gvcp_ask_t		ask;
	struct list_head		list;
};

struct gvcp_ack_t {
	uint32_t				len;
	uint8_t					buf[GVCP_MAX_MSG_LEN];
};

struct csv_gvcp_t {
	int						fd;			///< 文件描述符
	char					*name;		///< 链接名称
	uint16_t				port;		///< udp 监听端口
	uint16_t				ReqId;		///< 发起的消息序号

	uint32_t				rxlen;
	uint8_t					rxbuf[GVCP_MAX_MSG_LEN];

	struct sockaddr_in		from_addr;		///< 来源地址参数

	struct gvcp_ask_list_t	head_gvcpask;	///< gvcp消息链
	struct gvcp_ack_t		gvcp_ack;

	const char				*name_gvcpask;	///< 消息处理
	pthread_t				thr_gvcpask;
	pthread_mutex_t			mutex_gvcpask;
	pthread_cond_t			cond_gvcpask;
};

extern int csv_gvcp_recv_trigger (struct csv_gvcp_t *pGVCP);

extern int csv_gvcp_init (void);

extern int csv_gvcp_deinit (void);

#ifdef __cplusplus
}
#endif

#endif /* __CSV_GVCP_H__ */


