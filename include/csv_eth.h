#ifndef __CSV_ETH_H__
#define __CSV_ETH_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LEN_IP					(16)				///< IP地址最大长度

#define MAX_INTERFACES				(16)				/// 最多支持16个网卡

struct csv_eth_t {
	char				ifrname[IFNAMSIZ];		///< 网口设备名称

	char				ip[MAX_LEN_IP];			///< 设备 IP 地址字符串
	char				nm[MAX_LEN_IP];			///< 子网掩码字符串
	char				bc[MAX_LEN_IP];			///< 广播地址字符串
	char				mac[MAX_LEN_IP+2];		///< 物理地址字符串

	uint32_t			ipaddress;				///< IP地址
	uint32_t			netmask;				///< 子网掩码
	uint32_t			broadcast;				///< 广播地址	
	uint8_t				macaddrress[8];			///< mac地址

	uint16_t			flags;					///< 当前状态标识
	uint8_t				running;				///< 网线接入状态 1:on 0:off
	uint8_t				last_running;			///< 上周期网线接入状态

	uint8_t				ether_ug;				///< 主网卡(路由表UG)
	uint8_t				update;					///< 地址更新
};

// 本地多个网卡
struct csv_ifcfg_t {
	uint8_t					cnt_ifc;			///< 实际网卡数
	struct ifconf			ifc;
	struct ifreq			buf_ifc[MAX_INTERFACES];

	struct csv_eth_t		ether[MAX_INTERFACES];

	char					ifrname[IFNAMSIZ];	///< 主网卡
	char					gw[MAX_LEN_IP];		///< 默认网关
	uint32_t				gateway;
	uint8_t					update_gw;			///< 更新网关
};


extern int check_user_ip(const char* ip);

/* 设置 IP 地址: 长整型方式 */
extern int csv_eth_ipaddr_set (char *ifrname, uint32_t ip_addr);

/* 设置 IP 地址 : 字符串方式*/
extern int csv_eth_ipstraddr_set (char *ifrname, char *ipstr);

/* 设置子网掩码 地址: 长整型方式 */
extern int csv_eth_mask_set (char *ifrname, uint32_t net_mask);

/* 设置子网掩码 地址: 字符串方式 */
extern int csv_eth_maskstr_set (char *ifrname, char *maskstr);

/* 获取 网关 地址 */
extern int csv_eth_gateway_get (char *ifrname, char *gate);

/* 设置网关 地址: 长整型方式 */
extern int csv_eth_gateway_set (uint32_t gate_way);

extern int csv_eth_mac_updown (char *ifname, uint8_t flag);

/* 设置 MAC 地址 */
extern int csv_eth_mac_set (char *ifname, uint8_t *mac);


extern int csv_ether_refresh (uint8_t first);

extern int csv_ether_init (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_ETH_H__ */



