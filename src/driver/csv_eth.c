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
* @brief		设置 IP 地址: 长整型方式
* @param[in]	ip_addr		ip地址网络字节序
* @return		0	: 设置成功\n
	-1	: 无法打开eth设备\n
	-2	: 设置地址失败\n
	-3	: 输入地址为零
*/
int csv_eth_ipaddr_set (char *ifrname, uint32_t ip_addr)
{
	char ip[20] = {0};
	int fd = -1, ret = 0;
	uint32_t local_ip_addr = 0;
	struct ifreq ifr;
	struct sockaddr_in sa_in;

	if (0 == ip_addr) {
		log_warn("ip_addr = 0.");
		return -3;
	}

	local_ip_addr = ntohl(ip_addr);
	snprintf(ip, 20, "%d.%d.%d.%d", (local_ip_addr & 0xff000000) >> 24, 
					(local_ip_addr & 0xff0000) >> 16, 
					(local_ip_addr & 0xff00) >> 8, 
					(local_ip_addr & 0xff));

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ret = -1;
		log_err("ERROR : socket 'SOCK_DGRAM'.");
		return ret;
	}

	memset(&sa_in, 0, sizeof(struct sockaddr_in));
	sa_in.sin_family = AF_INET;
	inet_aton(ip, &sa_in.sin_addr);

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, ifrname);
	memcpy((char *)&ifr.ifr_ifru.ifru_addr, (char *)&sa_in, sizeof(struct sockaddr_in));

	ret = ioctl(fd, SIOCSIFADDR, &ifr);
	if (ret < 0) {
		ret = -2;
		log_err("ERROR : ioctl 'SIOCSIFADDR'.");
		if (close(fd)<0) {
			log_err("ERROR : close.");
		}
		return ret;
	}

	if (close(fd)<0) {
		log_err("ERROR : close.");
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
int csv_eth_ipstraddr_set (char *ifrname, char *ipstr)
{
	int fd = -1, ret = 0;
	struct ifreq ifr;
	struct sockaddr_in sa_in;

	if (NULL == ipstr) {
		ret = -3;
		log_warn("ERROR : ipstr is NULL.");
		return ret;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ret = -1;
		log_err("ERROR : socket 'SOCK_DGRAM'.");
		return ret;
	}

	memset(&sa_in, 0, sizeof(struct sockaddr_in));
	sa_in.sin_family = AF_INET;
	inet_aton(ipstr, &sa_in.sin_addr);

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, ifrname);
	memcpy((char *)&ifr.ifr_ifru.ifru_addr, (char *)&sa_in, sizeof(struct sockaddr_in));

	ret = ioctl(fd, SIOCSIFADDR, &ifr);
	if (ret < 0) {
		ret = -2;
		log_err("ERROR : ioctl 'SIOCSIFADDR'.");
		if (close(fd)<0) {
			log_err("ERROR : close.");
		}
		return ret;
	}

	if (close(fd)<0) {
		log_err("ERROR : close.");
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
int csv_eth_mask_set (char *ifrname, uint32_t net_mask)
{
	int fd = -1, ret = 0;
	char nm[20] = {0};
	uint32_t local_net_mask;
	struct ifreq ifr;
	struct sockaddr_in sa_in;

	if (0 == net_mask) {
		log_warn("net_mask = 0.");
		return -3;
	}

	local_net_mask = ntohl(net_mask);
	snprintf(nm, 20, "%d.%d.%d.%d", (local_net_mask & 0xff000000) >> 24, 
					(local_net_mask & 0xff0000) >> 16, 
					(local_net_mask & 0xff00) >> 8, 
					(local_net_mask & 0xff));

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ret = -1;
		log_err("ERROR : socket 'SOCK_DGRAM'.");
		return ret;
	}

	// 初始化 struct sockaddr_in
	memset(&sa_in, 0, sizeof(struct sockaddr_in));
	sa_in.sin_family = AF_INET;
	inet_aton(nm, &sa_in.sin_addr);

	// 初始化 struct ifreq
	memset(&ifr, 0, sizeof(struct ifreq));	
	strcpy(ifr.ifr_name, ifrname);
	memcpy((char *)&ifr.ifr_ifru.ifru_addr, (char *)&sa_in, sizeof(struct sockaddr_in));

	// 设置子网掩码
	ret = ioctl(fd, SIOCSIFNETMASK, &ifr);
	if (ret < 0) {
		ret = -2;
		log_err("ERROR : ioctl 'SIOCSIFNETMASK'.");
		if (close(fd)<0) {
			log_err("ERROR : close.");
		}
		return ret;
	}

	if (close(fd)<0) {
		log_err("ERROR : close.");
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
int csv_eth_maskstr_set (char *ifrname, char *maskstr)
{
	int fd = -1, ret = 0;
	struct ifreq ifr;
	struct sockaddr_in sa_in;

	if (NULL == maskstr) {
		ret = -3;
		log_warn("maskstr is NULL.");
		return ret;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		ret = -1;
		log_err("ERROR : socket 'SOCK_DGRAM'.");
		return ret;
	}

	memset(&sa_in, 0, sizeof(struct sockaddr_in));
	sa_in.sin_family = AF_INET;
	inet_aton(maskstr, &sa_in.sin_addr);

	memset(&ifr, 0, sizeof(struct ifreq));	
	strcpy(ifr.ifr_name, ifrname);
	memcpy((char *)&ifr.ifr_ifru.ifru_addr, (char *)&sa_in, sizeof(struct sockaddr_in));

	ret = ioctl(fd, SIOCSIFNETMASK, &ifr);
	if (fd < 0) {
		ret = -2;
		log_err("ERROR : ioctl 'SIOCSIFNETMASK'.");
		if (close(fd)<0) {
			log_err("ERROR : close.");
		}
		return ret;
	}

	if (close(fd)<0) {
		log_err("ERROR : close.");
	}

	return ret;
}


/**
* @brief		获取 网关 地址
* @param[out]	*gate		存储网关地址的缓冲区
* @return		获取成功\n
	-1	: 获取掩码失败\n
*/
int csv_eth_gateway_get (char *ifrname, char *gate)
{
	char devname[32] = {0};
	uint32_t d, m, g = 0;
	int ret, flgs, ref, use, metric, mtu, win, ir, err = 0;
	FILE *fp = fopen("/proc/net/route", "r");

	// Iface Destination Gateway Flags RefCnt Use Metric Mask MTU Window IRTT
	// enp0s3  00000000  FE040012  0003  0  0  100  00000000  0  0  0
	err = fscanf(fp, "%*[^\n]\n");				// 跳过第一行
	if (err < 0) {
		log_err("ERROR : fscanf.");
		goto err_exit;
	}

	err =-1;
	for (;;) {
		ret = fscanf(fp, "%32s%x%x%X%d%d%d%x%d%d%d\n", devname, &d, &g, 
			&flgs, &ref, &use, &metric, &m, &mtu, &win, &ir);
		if (ret != 11) {
			log_warn("ERROR : fscanf not match.");
			err = -1;
			goto err_exit;
		}

		if (flgs == 0x0003) {		// UG
			struct in_addr addr;
			addr.s_addr = g;
			strcpy(ifrname, devname);
			strcpy(gate, inet_ntoa(addr));
			err = 0;
			break;
		}
	}

err_exit:

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
		log_warn("gate_way = 0.");
		return 0;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		log_err("ERROR : socket 'SOCK_DGRAM'.");
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
		log_err("ERROR : ioctl 'SIOCDELRT'.");
		if (close(fd)<0) {
			log_err("ERROR : close.");
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
		log_err("ERROR : ioctl 'SIOCADDRT'.");
		if (close(fd)<0) {
			log_err("ERROR : close.");
		}
		return ret;
	}

	if (close(fd)<0) {
		log_err("ERROR : close.");
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
* @param[in]	flag 0:down 1:up
* @return		0	: 获取成功\n
	-1	: 获取失败
*/
int csv_eth_mac_updown (char *ifname, uint8_t flag)
{
	int fd = 0, ret = 0;
	struct ifreq ifr;

	if (!ifname) {
		log_warn("ifname is NULL.");
		return -1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		log_err("ERROR : socket 'SOCK_DGRAM'.");
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
		log_err("ERROR : ioctl 'SIOCSIFFLAGS'.");
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
		log_err("ERROR : socket 'SOCK_DGRAM'.");
		return -1;
	}

	ifr.ifr_addr.sa_family = ARPHRD_ETHER;
	strncpy(ifr.ifr_name, (const char *)ifname, IFNAMSIZ-1);
	memcpy((uint8_t *)ifr.ifr_hwaddr.sa_data, mac, 6);

	if ((ret = ioctl(fd, SIOCSIFHWADDR, &ifr)) != 0) {
		log_err("ERROR : ioctl 'SIOCSIFHWADDR'.");
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

static int csv_ether_route_refresh (struct csv_ifcfg_t *pIFCFG)
{
	int ret = 0;

	ret = csv_eth_gateway_get(pIFCFG->ifrname, pIFCFG->gw);
	if (ret == 0) {
		pIFCFG->gateway = inet_addr(pIFCFG->gw);
	}

	//log_debug("route '%s' GW(%s)", pIFCFG->ifrname, pIFCFG->gw);

	return ret;
}

/*
	"man netdevice" - low-level access to Linux network devices

	struct ifreq {
	    char ifr_name[IFNAMSIZ]; // Interface name
	    union {
	        struct sockaddr ifr_addr;
	        struct sockaddr ifr_dstaddr;
	        struct sockaddr ifr_broadaddr;
	        struct sockaddr ifr_netmask;
	        struct sockaddr ifr_hwaddr;
	        short           ifr_flags;
	        int             ifr_ifindex;
	        int             ifr_metric;
	        int             ifr_mtu;
	        struct ifmap    ifr_map;
	        char            ifr_slave[IFNAMSIZ];
	        char            ifr_newname[IFNAMSIZ];
	        char           *ifr_data;
	    };
	};

	SIOCGIFFLAGS, SIOCSIFFLAGS :
		IFF_UP            Interface is running.
		IFF_BROADCAST     Valid broadcast address set.
		IFF_DEBUG         Internal debugging flag.
		IFF_LOOPBACK      Interface is a loopback interface.
		IFF_POINTOPOINT   Interface is a point-to-point link.
		IFF_RUNNING       Resources allocated.
		IFF_NOARP         No arp protocol, L2 destination address not set.
		IFF_PROMISC       Interface is in promiscuous mode.
		IFF_NOTRAILERS    Avoid use of trailers.
		IFF_ALLMULTI      Receive all multicast packets.
		IFF_MASTER        Master of a load balancing bundle.
		IFF_SLAVE         Slave of a load balancing bundle.
		IFF_MULTICAST     Supports multicast

		IFF_PORTSEL       Is able to select media type via ifmap.
		IFF_AUTOMEDIA     Auto media selection active.
		IFF_DYNAMIC       The addresses are lost when the interface goes down.
		IFF_LOWER_UP      Driver signals L1 up (since Linux 2.6.17)
		IFF_DORMANT       Driver signals dormant (since Linux 2.6.17)
		IFF_ECHO          Echo sent packets (since Linux 2.6.25)

	SIOCGIFNAME
	SIOCGIFINDEX
	SIOCGIFPFLAGS, SIOCSIFPFLAGS
		IFF_802_1Q_VLAN      Interface is 802.1Q VLAN device.
		IFF_EBRIDGE          Interface is Ethernet bridging device.
		IFF_SLAVE_INACTIVE   Interface is inactive bonding slave.
		IFF_MASTER_8023AD    Interface is 802.3ad bonding master.
		IFF_MASTER_ALB       Interface is balanced-alb bonding master.
		IFF_BONDING          Interface is a bonding master or slave.
		IFF_SLAVE_NEEDARP    Interface needs ARPs for validation.
		IFF_ISATAP           Interface is RFC4214 ISATAP interface.
	SIOCGIFADDR, SIOCSIFADDR
	SIOCGIFDSTADDR, SIOCSIFDSTADDR
	SIOCGIFBRDADDR, SIOCSIFBRDADDR
	SIOCGIFNETMASK, SIOCSIFNETMASK
	SIOCGIFMETRIC, SIOCSIFMETRIC
	SIOCGIFMTU, SIOCSIFMTU
	SIOCGIFHWADDR, SIOCSIFHWADDR
	SIOCSIFHWBROADCAST
	SIOCGIFMAP, SIOCSIFMAP
	SIOCADDMULTI, SIOCDELMULTI
	SIOCGIFTXQLEN, SIOCSIFTXQLEN
	SIOCSIFNAME
	SIOCGIFCONF
		struct ifconf {
		    int                 ifc_len; // size of buffer
		    union {
		        char           *ifc_buf; // buffer address
		        struct ifreq   *ifc_req; // array of structures
		    };
		};

*/

static int csv_ether_caddrs_refresh (struct csv_ifcfg_t *pIFCFG, uint8_t first)
{
	int fd = -1, ret = 0, i = 0;
	uint8_t nCaddr = 0;
	struct csv_eth_t *pETHER = NULL;

	// for more help : "man netdevice"

	pIFCFG->ifc.ifc_len = sizeof(pIFCFG->buf_ifc);
	pIFCFG->ifc.ifc_buf = (caddr_t)pIFCFG->buf_ifc;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		log_err("ERROR : socket 'SOCK_DGRAM'.");
		return -1;
	}

	ret = ioctl(fd, SIOCGIFCONF, (char *)&pIFCFG->ifc);
	if (ret < 0) {
		log_err("ERROR : ioctl 'SIOCGIFCONF'.");
		close(fd);
		return -1;
	}

	pIFCFG->cnt_ifc = pIFCFG->ifc.ifc_len / sizeof(struct ifreq);
	//log_debug("OK : found ifc %d caddrs.", pIFCFG->cnt_ifc);

	nCaddr = pIFCFG->cnt_ifc;
	if (pIFCFG->cnt_ifc > MAX_INTERFACES) {
		nCaddr = MAX_INTERFACES;
		log_warn("WARN : show first %d caddrs.", MAX_INTERFACES);
	}

	for (i = 0; i < nCaddr; i++) {
		pETHER = &pIFCFG->ether[i];

		strcpy(pETHER->ifrname, pIFCFG->buf_ifc[i].ifr_name);
		if (strcmp(pETHER->ifrname, pIFCFG->ifrname) == 0) {
			pETHER->ether_ug = true;
		} else {
			pETHER->ether_ug = false;
		}

		ret = ioctl(fd, SIOCGIFFLAGS, (char *)&pIFCFG->buf_ifc[i]);
		if (ret < 0) {
			log_err("ERROR : ioctl 'SIOCGIFFLAGS' : '%s'.", pETHER->ifrname);
			continue;
		}

		pETHER->flags = pIFCFG->buf_ifc[i].ifr_flags;

		if (pETHER->flags & IFF_RUNNING) {
			pETHER->running = 1;
		} else {
			pETHER->running = 0;
		}

		if (first) {
			pETHER->last_running = pETHER->running;
			log_info("ether '%s' plug on.", pETHER->ifrname);
		} else {
			if (pETHER->running != pETHER->last_running) {
				if (pETHER->running) {
					log_info("ether '%s' plug on.", pETHER->ifrname);
				} else {
					log_warn("ether '%s' plug off.", pETHER->ifrname);
				}
				pETHER->last_running = pETHER->running;
			}
		}

		// ip address
		ret = ioctl(fd, SIOCGIFADDR, (char *)&pIFCFG->buf_ifc[i]);
		if (ret < 0) {
			log_err("ERROR : ioctl 'SIOCGIFADDR' : '%s'.", pETHER->ifrname);
			continue;
		}
		pETHER->IPAddr = ((struct sockaddr_in *)(&pIFCFG->buf_ifc[i].ifr_addr))->sin_addr.s_addr;
		strcpy(pETHER->ip, inet_ntoa(((struct sockaddr_in *)(&pIFCFG->buf_ifc[i].ifr_addr))->sin_addr));

		// netmask
		ret = ioctl(fd, SIOCGIFNETMASK, (char *)&pIFCFG->buf_ifc[i]);
		if (ret < 0) {
			log_err("ERROR : ioctl 'SIOCGIFNETMASK' : '%s'.", pETHER->ifrname);
			continue;
		}
		pETHER->netmask = ((struct sockaddr_in *)(&pIFCFG->buf_ifc[i].ifr_addr))->sin_addr.s_addr;
		strcpy(pETHER->nm, inet_ntoa(((struct sockaddr_in *)(&pIFCFG->buf_ifc[i].ifr_addr))->sin_addr));

		// broadcast
		ret = ioctl(fd, SIOCGIFBRDADDR, (char *)&pIFCFG->buf_ifc[i]);
		if (ret < 0) {
			log_err("ERROR : ioctl 'SIOCGIFBRDADDR' : '%s'.", pETHER->ifrname);
			continue;
		}
		pETHER->broadcast = ((struct sockaddr_in *)(&pIFCFG->buf_ifc[i].ifr_addr))->sin_addr.s_addr;
		strcpy(pETHER->bc, inet_ntoa(((struct sockaddr_in *)(&pIFCFG->buf_ifc[i].ifr_addr))->sin_addr));

		// mac
		ret = ioctl(fd, SIOCGIFHWADDR, (char *)&pIFCFG->buf_ifc[i]);
		if (ret < 0) {
			log_err("ERROR : ioctl 'SIOCGIFHWADDR' : '%s'.", pETHER->ifrname);
			continue;
		}

		memcpy(pETHER->MACAddr, (uint8_t *)pIFCFG->buf_ifc[i].ifr_hwaddr.sa_data, 6);
		snprintf(pETHER->mac, 18, "%02X:%02X:%02X:%02X:%02X:%02X", pETHER->MACAddr[0],
			pETHER->MACAddr[1],pETHER->MACAddr[2],pETHER->MACAddr[3],
			pETHER->MACAddr[4],pETHER->MACAddr[5]);

		//log_debug("'%s' : IP(%s) NM(%s) BC(%s) MAC(%s)", pETHER->ifrname, 
		//	pETHER->ip, pETHER->nm, pETHER->bc, pETHER->mac);
	}

	close(fd);

	return 0;
}

int csv_ether_refresh (uint8_t first)
{
	int ret = 0;
	struct csv_ifcfg_t *pIFCFG = &gCSV->ifcfg;

	ret = csv_ether_route_refresh(pIFCFG);
	ret |= csv_ether_caddrs_refresh(pIFCFG, first);

	return ret;
}

int csv_eth_init (void)
{
	int ret = 0, i = 0;
	struct csv_ifcfg_t *pIFCFG = &gCSV->ifcfg;
	struct csv_eth_t *pETHER = NULL;
	struct gev_conf_t *pGC = &gCSV->cfg.gigecfg;

	pIFCFG->cnt_ifc = 0;

	csv_ether_refresh(true);

	for (i = 0 ; i < pIFCFG->cnt_ifc; i++) {
		pETHER = &pIFCFG->ether[i];
		if (pETHER->ether_ug) {
			pGC->MacHi = (uint32_t)u8v_to_u16(&pETHER->MACAddr[0]);
			pGC->MacLow = u8v_to_u32(&pETHER->MACAddr[2]);
			pGC->CurrentIPAddress0 = pETHER->IPAddr;
			pGC->CurrentSubnetMask0 = pETHER->netmask;
			break;
		}
	}

	pGC->CurrentDefaultGateway0 = pIFCFG->gateway;

	return ret;
}



#ifdef __cplusplus
}
#endif

