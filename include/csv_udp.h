#ifndef __CSV_UDP_H__
#define __CSV_UDP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_UDP_LOG		"'udp_log'"

#define DEFAULT_LOG_SERV	("127.0.0.1")
#define DEFAULT_LOG_PORT	(36666)

struct csv_udp_t {
	int				fd;
	char			*name;
	uint8_t			enable;		// 使能
	uint8_t			level;		// 等级
	uint8_t			initialled;	// 已初始化
	uint8_t			reinit;		// 需重置目标

	char			hostid[32];
	char			ip[32];
	uint16_t		port;

	struct sockaddr_in peer;
};

extern struct csv_udp_t gUDP;

extern int csv_udp_set_level (uint8_t enable, uint8_t level);

extern int csv_udp_set_server (uint32_t ipaddr, uint16_t port);

extern int csv_udp_sendto (const char *content);

extern int csv_udp_init (void);

extern int csv_udp_deinit (void);


#ifdef __cplusplus
}
#endif

#endif	/* __CSV_UDP_H__ */


