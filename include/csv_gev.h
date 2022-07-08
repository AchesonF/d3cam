#ifndef __CSV_GEV_H__
#define __CSV_GEV_H__

/* GigE Vision server */

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_UDP_GVCP		"'udp_gev'"


struct csv_gev_t {
	int					fd;			///< 文件描述符
	char				*name;		///< 链接名称
	uint16_t			port;		///< udp 监听端口
	uint16_t			ReqId;		///< 发起的消息序号

	uint32_t			rxlen;
	uint8_t				rxbuf[GVCP_MAX_MSG_LEN];

	uint32_t			txlen;
	uint8_t				txbuf[GVCP_MAX_MSG_LEN];

	struct sockaddr_in	from_addr;		///< 来源地址参数
	struct sockaddr_in	target_addr;	///< 媒体流流向参数
};


extern int csv_gev_trigger (struct csv_gev_t *pGEV);

extern int csv_gev_init (void);

extern int csv_gev_deinit (void);



#ifdef __cplusplus
}
#endif


#endif /* __CSV_GEV_H__ */


