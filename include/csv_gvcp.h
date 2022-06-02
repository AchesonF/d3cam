#ifndef __CSV_GVCP_H__
#define __CSV_GVCP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_UDP_GVCP		"'udp_gvcp'"

#define PORT_UDP_GVCP		(3956)

#define SIZE_UDP_BUFF		(4096)


struct csv_gvcp_t {
	int				fd;			///< 文件描述符
	char			*name;		///< 链接名称
	uint16_t		port;		///< udp 监听端口

	struct {
		uint32_t	nRx;
		uint8_t		pRx[SIZE_UDP_BUFF];
	} Recv;

	struct {
		uint32_t	nTx;
		uint8_t		pTx[SIZE_UDP_BUFF];
	} Send;

	struct sockaddr_in	from_addr;		///< 来源地址参数
};


extern int csv_gvcp_trigger (struct csv_gvcp_t *pGVCP);

extern int csv_gvcp_init (void);

extern int csv_gvcp_deinit (void);



#ifdef __cplusplus
}
#endif


#endif /* __CSV_GVCP_H__ */


