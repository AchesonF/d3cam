#ifndef __CSV_GEVMSG_H__
#define __CSV_GEVMSG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_SOCKET_GEV_MESSAGE				"'gev_msg'"
#define NAME_THREAD_GEV_MESSAGE				"'thr_gevmsg'"


struct message_info_t {
	uint32_t				retries;		///< 发送次数
	uint16_t				ReqId;			///< 消息编号
	uint16_t				acked;			///< 确认状态

	uint32_t				lendata;		///< 数据长度
	uint8_t					data[GVCP_PACKET_MAX_LEN];
};

struct gevmsg_list_t {
	struct message_info_t	mi;
	struct list_head		list;
};


struct csv_gevmsg_t {
	int						fd;
	char					*name;

	uint16_t				ReqId;		///< 发起的消息序号
	uint16_t				port;			///< 本地系统分配端口号

	struct sockaddr_in		peer_addr;		///< 目标地址/端口
	struct sockaddr_in		from_addr;		///< 来自地址/端口

	uint8_t					bufRecv[GVCP_PACKET_MAX_LEN];
	uint32_t				lenRecv;
	uint8_t					bufSend[GVCP_PACKET_MAX_LEN];
	uint32_t				lenSend;

	struct gevmsg_list_t	head_gevmsg;	///< 发送(及等待确认)消息队列

	const char				*name_gevmsg;
	pthread_t				thr_gevmsg;
	pthread_mutex_t			mutex_gevmsg;
	pthread_cond_t			cond_gevmsg;
};


extern int csv_gevmsg_package (uint16_t identifier);

extern int csv_gevmsg_udp_reading_trigger (struct csv_gevmsg_t *pGVMSG);

extern int csv_gevmsg_init (void);

extern int csv_gevmsg_deinit (void);



#ifdef __cplusplus
}
#endif

#endif	/* __CSV_GEVMSG_H__ */

