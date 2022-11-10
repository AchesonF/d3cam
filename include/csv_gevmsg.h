#ifndef __CSV_GEVMSG_H__
#define __CSV_GEVMSG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_SOCKET_GEV_MESSAGE				"'gev_msg'"
#define NAME_THREAD_GEV_MESSAGE				"'thr_gevmsg'"



struct csv_gevmsg_t {
	int						fd;
	char					*name;

	uint16_t				port;			///< 本地系统分配端口号
	struct sockaddr_in		peer_addr;		///< 目标地址/端口

	uint8_t					bufSend[GVSP_PACKET_MAX_SIZE];
	uint32_t				lenSend;

	const char				*name_gevmsg;
	pthread_t				thr_gevmsg;
	pthread_mutex_t			mutex_gevmsg;
	pthread_cond_t			cond_gevmsg;
};


extern int csv_gevmsg_init (void);

extern int csv_gevmsg_deinit (void);



#ifdef __cplusplus
}
#endif

#endif	/* __CSV_GEVMSG_H__ */

