#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief		检测ip字符串是否合法
* @param[in]	*ipstr		待检测ip字符串
* @return		0	: 合法\n
	<0	: 不合法
*/
int check_user_ip (const char* ip)
{
	const char *str = ip;
	int i = 0, p, j = 0;

	while(ip[i] != '\0') {
		if((ip[i] != '.')&&(ip[i]<'0' || ip[i]>'9')) {
			return -1;
		}

		if(ip[i] == '.') { 
			if(str[0] == '.') return -1;

			p = atoi(str);
			if(p < 0 || p > 255 || str[0] == '\0') {
				return -1;
			}
			str = ip + i + 1;
			j++;
		}

		i++;
	}

	if (j == 3) {
		p = atoi(str);
		if(p < 0 || p > 255) {
			return -1;
		}
	}

	if(j != 3) return -1;

	return 0;
}

/**
* @brief		获取 IP 地址
* @param[out]	*ipstr		获取到的ip字符串
* @return		0	: 获取成功\n
	-1	: 无法打开eth设备\n
	-2	: 获取地址失败
*/
int csv_eth_ipaddr_get (char *ipstr)
{
	int fd = -1, ret = 0;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		log_err("ERROR : socket");
		ret = -1;
		return ret;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, DEV_ETH);
	
	ret = ioctl(fd, SIOCGIFADDR, &ifr);
	if (ret < 0) {
		log_err("ERROR : ioctl");
		if (close(fd)<0) {
			log_err("ERROR : close");
		}
		ret = -2;
		return ret;
	}

	strcpy(ipstr, inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr));

	if (close(fd)<0) {
		log_err("ERROR : close");
	}

	return ret;
}

/**
* @brief		设置 IP 地址: 长整型方式
* @param[in]	ip_addr		ip地址网络字节序
* @return		0	: 设置成功\n
	-1	: 无法打开eth设备\n
	-2	: 设置地址失败\n
	-3	: 输入地址为零
*/
int csv_eth_ipaddr_set (uint32_t ip_addr)
{
	char ip[20] = {0};
	int fd = -1, ret = 0;
	uint32_t local_ip_addr = 0;
	struct ifreq ifr;
	struct sockaddr_in sa_in;

	if (0 == ip_addr) {
		log_info("ip_addr = 0");
		return -3;
	}

	local_ip_addr = ntohl(ip_addr);
	sprintf(ip, "%d.%d.%d.%d", (local_ip_addr & 0xff000000) >> 24, 
					(local_ip_addr & 0xff0000) >> 16, 
					(local_ip_addr & 0xff00) >> 8, 
					(local_ip_addr & 0xff));

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ret = -1;
		log_err("ERROR : socket");
		return ret;
	}

	memset(&sa_in, 0, sizeof(struct sockaddr_in));
	sa_in.sin_family = AF_INET;
	inet_aton(ip, &sa_in.sin_addr);

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, DEV_ETH, IFNAMSIZ);
	memcpy((char *)&ifr.ifr_ifru.ifru_addr, (char *)&sa_in, sizeof(struct sockaddr_in));

	ret = ioctl(fd, SIOCSIFADDR, &ifr);
	if (ret < 0) {
		ret = -2;
		log_err("ERROR : ioctl");
		if (close(fd)<0) {
			log_err("ERROR : close");
		}
		return ret;
	}

	if (close(fd)<0) {
		log_err("ERROR : close");
	}

	return ret;
}

/**
* @brief		设置 IP 地址: 字符串方式
* @param[in]	*ip_str		ip地址字符串
* @return		0	: 设置成功\n
	-1	: 无法打开eth设备\n
	-2	: 设置地址失败\n
	-3	: 输入字符串为空
*/
int csv_eth_ipstraddr_set (char *ipstr)
{
	int fd = -1, ret = 0;
	struct ifreq ifr;
	struct sockaddr_in sa_in;

	if (NULL == ipstr) {
		ret = -3;
		log_info("ERROR : ipstr is NULL.");
		return ret;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ret = -1;
		log_err("ERROR : socket");
		return ret;
	}

	memset(&sa_in, 0, sizeof(struct sockaddr_in));
	sa_in.sin_family = AF_INET;
	inet_aton(ipstr, &sa_in.sin_addr);

	memset(&ifr, 0, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, DEV_ETH, IFNAMSIZ);
	memcpy((char *)&ifr.ifr_ifru.ifru_addr, (char *)&sa_in, sizeof(struct sockaddr_in));

	ret = ioctl(fd, SIOCSIFADDR, &ifr);
	if (ret < 0) {
		ret = -2;
		log_err("ERROR : ioctl");
		if (close(fd)<0) {
			log_err("ERROR : close");
		}
		return ret;
	}

	if (close(fd)<0) {
		log_err("ERROR : close");
	}
	
	return ret;
}

/**
* @brief		获取 子网掩码 地址
* @param[out]	*mask		存储掩码地址的缓冲区
* @return		0	: 获取成功\n
	-1	: 无法打开eth设备\n
	-2	: 获取掩码失败
*/
int csv_eth_mask_get (char *mask)
{
	int fd = -1, ret = 0;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ret = -1;
		log_err("ERROR : socket");
		return ret;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, DEV_ETH);
	
	ret = ioctl(fd, SIOCGIFNETMASK, &ifr);
	if (ret < 0) {
		ret = -2;
		log_err("ERROR : ioctl");
		if (close(fd)<0) {
			log_err("ERROR : close");
		}
		return ret;
	}

	strcpy(mask, inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_netmask))->sin_addr));

	if (close(fd)<0) {
		log_err("ERROR : close");
	}

	return ret;
}

/**
* @brief		设置子网掩码 地址: 长整型方式
* @param[in]	net_mask		子网掩码地址
* @return		0	: 设置成功\n
	-1	: 无法打开eth设备\n
	-2	: 设置掩码失败\n
	-3	: 输入子网掩码为零
*/
int csv_eth_mask_set (uint32_t net_mask)
{
	int fd = -1, ret = 0;
	char nm[20] = {0};
	uint32_t local_net_mask;
	struct ifreq ifr;
	struct sockaddr_in sa_in;

	if (0 == net_mask) {
		log_info("net_mask = 0");
		return -3;
	}

	local_net_mask = ntohl(net_mask);
	sprintf(nm, "%d.%d.%d.%d", (local_net_mask & 0xff000000) >> 24, 
					(local_net_mask & 0xff0000) >> 16, 
					(local_net_mask & 0xff00) >> 8, 
					(local_net_mask & 0xff));

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ret = -1;
		log_err("ERROR : socket");
		return ret;
	}

	// 初始化 struct sockaddr_in
	memset(&sa_in, 0, sizeof(struct sockaddr_in));
	sa_in.sin_family = AF_INET;
	inet_aton(nm, &sa_in.sin_addr);

	// 初始化 struct ifreq
	memset(&ifr, 0, sizeof(struct ifreq));	
	strncpy(ifr.ifr_name, DEV_ETH, IFNAMSIZ);
	memcpy((char *)&ifr.ifr_ifru.ifru_addr, (char *)&sa_in, sizeof(struct sockaddr_in));

	// 设置子网掩码
	ret = ioctl(fd, SIOCSIFNETMASK, &ifr);
	if (ret < 0) {
		ret = -2;
		log_err("ERROR : ioctl");
		if (close(fd)<0) {
			log_err("ERROR : close");
		}
		return ret;
	}

	if (close(fd)<0) {
		log_err("ERROR : close");
	}

	return ret;
}

/**
* @brief		设置子网掩码 地址: 字符串方式
* @param[in]	*maskstr		子网掩码地址字符串
* @return		0	: 设置成功\n
	-1	: 无法打开eth设备\n
	-2	: 设置掩码失败\n
	-3	: 输入子网掩码为空
*/
int csv_eth_maskstr_set (char *maskstr)
{
	int fd = -1, ret = 0;
	struct ifreq ifr;
	struct sockaddr_in sa_in;

	if (NULL == maskstr) {
		ret = -3;
		log_info("maskstr is NULL");
		return ret;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ret = -1;
		log_err("ERROR : socket");
		return ret;
	}

	memset(&sa_in, 0, sizeof(struct sockaddr_in));
	sa_in.sin_family = AF_INET;
	inet_aton(maskstr, &sa_in.sin_addr);

	memset(&ifr, 0, sizeof(struct ifreq));	
	strncpy(ifr.ifr_name, DEV_ETH, IFNAMSIZ);
	memcpy((char *)&ifr.ifr_ifru.ifru_addr, (char *)&sa_in, sizeof(struct sockaddr_in));

	ret = ioctl(fd, SIOCSIFNETMASK, &ifr);
	if (fd < 0) {
		ret = -2;
		log_err("ERROR : ioctl");
		if (close(fd)<0) {
			log_err("ERROR : close");
		}
		return ret;
	}

	if (close(fd)<0) {
		log_err("ERROR : close");
	}

	return ret;
}

int csv_eth_broadcast_get (char *bc)
{
	int fd = -1, ret = 0;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ret = -1;
		log_err("ERROR : socket");
		return ret;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, DEV_ETH);
	
	ret = ioctl(fd, SIOCGIFBRDADDR, &ifr);
	if (ret < 0) {
		ret = -2;
		log_err("ERROR : ioctl");
		if (close(fd)<0) {
			log_err("ERROR : close");
		}
		return ret;
	}

	strcpy(bc, inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_netmask))->sin_addr));

	if (close(fd)<0) {
		log_err("ERROR : close");
	}

	return ret;
}


/**
* @brief		获取 网关 地址
* @param[out]	*gate		存储网关地址的缓冲区
* @return		获取成功\n
	-1	: 获取掩码失败\n
*/
int csv_eth_gateway_get (char *gate)
{
	char devname[64] = {0};
	uint32_t d, m, g = 0;
	int flgs, ref, use, metric, mtu, win, ir, err = 0;
	FILE *fp = fopen("/proc/net/route", "r");

	err = fscanf(fp, "%*[^\n]\n");				// 跳过第一行
	if (err < 0) {
		fclose(fp);
		return err;
	}

	for (;;) {
		int ret = fscanf(fp, "%63s%x%x%X%d%d%d%x%d%d%d\n",
					devname, &d, &g, &flgs, &ref, &use, &metric, &m,
					&mtu, &win, &ir);
		if (ret != 11) {
			err = -1;
			fclose(fp);
			return err;
		}
		if (flgs == 0x3) {
			struct in_addr addr;
			addr.s_addr = g;
			strcpy(gate, inet_ntoa(addr));
			break;
		}
	}

	fclose(fp);
	return err;
}

/**
* @brief		设置网关 地址: 长整型方式
* @param[in]	gate_way		网关地址
* @return		0	: 设置成功\n
	-1	: 无法打开eth设备\n
	-2	: 无法删除现有默认网关\n
	-3	: 无法设置网关
*/
int csv_eth_gateway_set (uint32_t gate_way)
{
	int fd = -1, ret = 0;
	struct rtentry rt;

	if (0 == gate_way) {
		log_info(" gate_way = 0");
		return 0;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		log_err("ERROR : socket");
		return -1;
	}

	// 删除现有的默认网关
	memset(&rt, 0, sizeof(struct rtentry));
	rt.rt_dst.sa_family = AF_INET;
	((struct sockaddr_in *)&rt.rt_dst)->sin_addr.s_addr = 0;
	
	rt.rt_genmask.sa_family = AF_INET;
	((struct sockaddr_in *)&rt.rt_genmask)->sin_addr.s_addr = 0;

	rt.rt_flags = RTF_UP;

	ret = ioctl(fd, SIOCDELRT, &rt);
	if ((0 != ret) && (ESRCH != errno)) {
		ret = -2;
		log_err("ERROR : ioctl");
		if (close(fd)<0) {
			log_err("ERROR : close");
		}
		return ret;
	}

	// 设置默认网关
	memset(&rt, 0, sizeof(struct rtentry));

	rt.rt_dst.sa_family = AF_INET;
	((struct sockaddr_in *)&rt.rt_dst)->sin_addr.s_addr = 0;

	rt.rt_gateway.sa_family = AF_INET;
	((struct sockaddr_in *)&rt.rt_gateway)->sin_addr.s_addr = gate_way;

	rt.rt_genmask.sa_family = AF_INET;
	((struct sockaddr_in *)&rt.rt_genmask)->sin_addr.s_addr = 0;

	rt.rt_flags = RTF_UP | RTF_GATEWAY;

	ret = ioctl(fd, SIOCADDRT, &rt);
	if ((ret < 0) && (ESRCH != errno)) {
		ret = -3;
		log_err("ERROR : ioctl");
		if (close(fd)<0) {
			log_err("ERROR : close");
		}
		return ret;
	}

	if (close(fd)<0) {
		log_err("ERROR : close");
	}

	// set as default gateway
	char str_gw[128] = {0};
	memset(str_gw, 0, 128);
	struct in_addr addr;
	addr.s_addr = gate_way;
	snprintf(str_gw, 128, "route add default gw %s", inet_ntoa(addr));
	system_redef(str_gw);

	return ret;
}

/**
* @brief		获取 MAC 地址
* @param[in]	*ifname	网卡接口
* @param[in]	*mac	MAC 地址
* @return		0	: 获取成功\n
	-1	: 获取失败
*/
int csv_eth_mac_get (const char *ifname, uint8_t *mac)
{
	int fd = 0, ret = 0;
	struct ifreq ifr;

	if (!ifname || !mac) {
		return -1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		log_err("ERROR : socket");
		return -1;
	}

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, (const char *)ifname, IFNAMSIZ-1);

	if ((ret = ioctl(fd, SIOCGIFHWADDR, &ifr)) == 0) {
		memcpy(mac, (uint8_t *)ifr.ifr_hwaddr.sa_data, 6);
	}

	close(fd);

	return ret;
}

/**
* @brief		获取 MAC 地址
* @param[in]	*ifname	网卡接口
* @param[in]	flag 0:down 1:up
* @return		0	: 获取成功\n
	-1	: 获取失败
*/
int csv_eth_mac_updown (char *ifname, uint8_t flag)
{
	int fd = 0, ret = 0;
	struct ifreq ifr;

	if (!ifname) {
		return -1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		log_err("ERROR : socket");
		return -1;
	}

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, (const char *)ifname, IFNAMSIZ-1);

	if ((ret = ioctl(fd, SIOCGIFFLAGS, &ifr)) == 0) {
		if (flag == 0) {
			ifr.ifr_flags &= ~IFF_UP;
		} else if (flag ==1) {
			ifr.ifr_flags |= IFF_UP;
		}
	}

	if ((ret = ioctl(fd, SIOCSIFFLAGS, &ifr)) != 0) {
		log_err("ERROR : ioctl SIOCSIFFLAGS");
	}

	close(fd);

	return ret;
}

/**
* @brief		设置 MAC 地址
* @param[in]	*ifname	网卡接口
* @param[in]	*mac	MAC 地址
* @return		0	: 设置成功\n
	<0	: 设置失败
*/
int csv_eth_mac_set (char *ifname, uint8_t *mac)
{
	int fd = 0, ret = 0;
	struct ifreq ifr;

	if (!ifname || !mac) {
		return -1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		log_err("ERROR : socket");
		return -1;
	}

	ifr.ifr_addr.sa_family = ARPHRD_ETHER;
	strncpy(ifr.ifr_name, (const char *)ifname, IFNAMSIZ-1);
	memcpy((uint8_t *)ifr.ifr_hwaddr.sa_data, mac, 6);

	if ((ret = ioctl(fd, SIOCSIFHWADDR, &ifr)) != 0) {
		log_err("ERROR : ioctl SIOCSIFHWADDR");
	}

	close(fd);

	return ret;
}

int csv_eth_mac_update_online (char *ifname, uint8_t *mac)
{
	int ret = 0;

	ret = csv_eth_mac_updown(ifname, 0);
	ret |= csv_eth_mac_set(ifname, mac);
	ret |= csv_eth_mac_updown(ifname, 1);

	return ret;
}

/**
* @brief		获取网络信息
* @return		0	: 成功\n
	<0	: 失败
*/
int csv_eth_get (struct csv_eth_t *pETH)
{
	int ret = -1;

	ret = csv_eth_ipaddr_get(pETH->ip);
	if (ret == 0) {
		pETH->IPAddress = inet_addr(pETH->ip);
	}

	ret |= csv_eth_mask_get(pETH->nm);
	if (ret == 0) {
		pETH->IPMask = inet_addr(pETH->nm);
	}

	ret |= csv_eth_gateway_get(pETH->gw);
	if (ret == 0) {
		pETH->GateWayAddr = inet_addr(pETH->gw);
	}

	ret |= csv_eth_broadcast_get(pETH->bc);
	if (ret == 0) {
		pETH->BroadcastAddr = inet_addr(pETH->bc);
	}

	ret = csv_eth_mac_get(pETH->name, pETH->MACAddr);

	sprintf(pETH->mac, "%02X:%02X:%02X:%02X:%02X:%02X", pETH->MACAddr[0],
		pETH->MACAddr[1],pETH->MACAddr[2],pETH->MACAddr[3],
		pETH->MACAddr[4],pETH->MACAddr[5]);

	log_debug("%s %s %s %s %s", pETH->ip, pETH->nm, pETH->gw, pETH->bc, pETH->mac);


	return ret;
}

int csv_eth_init (void)
{
	struct csv_eth_t *pETH = &gCSV->eth;

	pETH->name = DEV_ETH;
	pETH->fd = -1;
	pETH->now_sta = false;
	pETH->last_sta = false;
	pETH->name_dhcp = THREAD_NAME_DHCP;

	csv_eth_get(pETH);

	// 备份 后台
	return system("ifconfig eth0:1 18.0.4.101");
}


#ifdef __cplusplus
}
#endif

