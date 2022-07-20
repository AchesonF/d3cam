#ifndef __CSV_ETH_H__
#define __CSV_ETH_H__

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_ETH						("eth0")//("ens33")//("enp0s3")	///< 网卡设备节点

#define THREAD_NAME_DHCP			("'thr_dhcp'")

#define MAX_LEN_IP					(16)				///< IP地址最大长度

struct csv_eth_t {
	const char			*name;					///< 网口设备名称
	int					fd;						///< 网口socket
	uint8_t				now_sta;				///< 网口连线运行状态
	uint8_t				last_sta;				///< 网口上周期运行状态

	char				ip[MAX_LEN_IP];			///< 设备 IP 地址字符串
	char				gw[MAX_LEN_IP];			///< 网关地址字符串
	char				nm[MAX_LEN_IP];			///< 子网掩码字符串
	char				bc[MAX_LEN_IP];			///< 广播地址字符串
	char				dns[MAX_LEN_IP];		///< DNS 地址字符串
	char				mac[MAX_LEN_IP+2];		///< 物理地址字符串

	uint8_t				MACAddr[8];				///< mac地址
	uint32_t			IPAddr;					///< IP地址
	uint32_t			GatewayAddr;			///< 网关地址
	uint32_t			NetmaskAddr;			///< 子网掩码
	uint32_t			BroadcastAddr;			///< 广播地址
	uint32_t			DNSAddr;				///< DNS地址

	const char			*name_dhcp;				///< 动态获取IP线程
	pthread_t			thr_dhcp;
};



extern int check_user_ip(const char* ip);

/* 获取 IP 地址 */
extern int csv_eth_ipaddr_get (char *ipstr);

/* 设置 IP 地址: 长整型方式 */
extern int csv_eth_ipaddr_set (uint32_t ip_addr);

/* 设置 IP 地址 : 字符串方式*/
extern int csv_eth_ipstraddr_set (char *ipstr);

/* 获取 子网掩码 地址 */
extern int csv_eth_mask_get (char *mask);

/* 设置子网掩码 地址: 长整型方式 */
extern int csv_eth_mask_set (uint32_t net_mask);

/* 设置子网掩码 地址: 字符串方式 */
extern int csv_eth_maskstr_set (char *maskstr);

/* 获取 网关 地址 */
extern int csv_eth_gateway_get (char *gate);

/* 设置网关 地址: 长整型方式 */
extern int csv_eth_gateway_set (uint32_t gate_way);

/* 获取 MAC 地址 */
extern int csv_eth_mac_get (const char *ifname, uint8_t *mac);

extern int csv_eth_mac_updown (char *ifname, uint8_t flag);

/* 设置 MAC 地址 */
extern int csv_eth_mac_set (char *ifname, uint8_t *mac);

extern int csv_eth_get (struct csv_eth_t *pETH);


extern int csv_eth_init (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_ETH_H__ */



