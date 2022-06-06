#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define CONFIG_SHOW_STAT

static int csv_stat_get_system_uptime (uint32_t *uptime)
{
	int ret = 0;
	char buffer[64] = {0};

	/* We need the uptime in 1/100 seconds, so we can't use sysinfo() */
	ret = csv_file_read_string("/proc/uptime", buffer, sizeof(buffer));
	if (ret < 0) {
		return ret;
	}

	*uptime = (uint32_t)(atof(buffer) * 100);
#ifdef CONFIG_SHOW_STAT
	log_debug("system uptime : %u*0.01 s", *uptime);
#endif

	return 0;
}

static int csv_stat_get_process_uptime (uint32_t *uptime)
{
	int ret = 0;
	static uint32_t uptime_start = 0;

	ret = csv_stat_get_system_uptime(&gCSV->stat.uptime_sys);
	if (ret < 0) {
		return -1;
	}

	if (uptime_start == 0) {
		uptime_start = gCSV->stat.uptime_sys;
	}

	*uptime = gCSV->stat.uptime_sys - uptime_start;
#ifdef CONFIG_SHOW_STAT
	log_debug("process uptime : %u s", *uptime/100);
#endif

	return 0;
}

static int csv_stat_get_loadinfo (struct loadinfo_t *loadinfo)
{
	int ret = 0;
	char buffer[128] = {0};
	char *ptr = NULL;
	int i = 0;

	ret = csv_file_read_string("/proc/loadavg", buffer, sizeof(buffer));

	if (ret == 0) {
		ptr = buffer;
		for (i = 0; i < 3; i++) {
			while (isspace(*ptr)) {
				ptr++;
			}
			if (*ptr != '\0') {
				loadinfo->avg[i] = strtod(ptr, &ptr) * 100;
			}
		}
#ifdef CONFIG_SHOW_STAT
		log_debug("loadavg %(1m 5m 15m) : %d %d %d",  
			loadinfo->avg[0], loadinfo->avg[1], loadinfo->avg[2]);
#endif
	} else {
		memset(loadinfo, 0, sizeof(struct loadinfo_t));
		return -1;
	}

	return 0;
}

static int csv_stat_get_meminfo (struct meminfo_t *meminfo)
{
	int ret = 0;
	char buffer[BUFSIZ] = {0};

	ret = csv_file_read_string("/proc/meminfo", buffer, sizeof(buffer));
	if (ret == 0) {
		meminfo->total = utility_read_value(buffer, "MemTotal:");
		meminfo->free = utility_read_value(buffer, "MemFree:");
		meminfo->shared = utility_read_value(buffer, "Shmem:");
		meminfo->buffers = utility_read_value(buffer, "Buffers:");
		meminfo->cached = utility_read_value(buffer, "Cached:");

		meminfo->used = 1000-(meminfo->free*1000)/meminfo->total;
#ifdef CONFIG_SHOW_STAT
		log_debug("memory KiB: %u(Total) %u(Free) %u(Shared) %u(Buffers) %u(Cached)",  
			meminfo->total, meminfo->free, meminfo->shared, 
			meminfo->buffers, meminfo->cached);
#endif
	} else {
		memset(meminfo, 0, sizeof(struct meminfo_t));
		return -1;
	}

	return 0;
}

static int csv_stat_get_cpuinfo (struct csv_stat_t *pSTAT)
{
	int ret = 0;
	char buffer[BUFSIZ] = {0};
	uint32_t values[4] = {0};

	struct cpuinfo_t *pCPU = &pSTAT->cpu[0];	// now
	struct cpuinfo_t *pCPU2 = &pSTAT->cpu[1];	// last

	ret = csv_file_read_string("/proc/stat", buffer, sizeof(buffer));
	if (ret == 0) {
		utility_read_values(buffer, "cpu", values, 4);
		pCPU->user = values[0];
		pCPU->nice = values[1];
		pCPU->system = values[2];
		pCPU->idle = values[3];
		pCPU->irqs = utility_read_value(buffer, "intr");
		pCPU->cntxts = utility_read_value(buffer, "ctxt");
		pCPU->softirq = utility_read_value(buffer, "softirq");

		if (pCPU->user != 0) {
			uint32_t od, nd;
			uint32_t id, sd;
			// (用户+ 优先级 + 系统 + 空闲)时间
			nd = pCPU->user + pCPU->nice + pCPU->system + pCPU->idle;
			od = pCPU2->user + pCPU2->nice + pCPU2->system + pCPU2->idle;

			id = (uint32_t)(pCPU->idle - pCPU2->idle);
			sd = (uint32_t)(nd - od);

			if (sd != 0) {
				// (用户+系统)*100 / 时间差
				pCPU->used = 1000 - (int)(id*1000)/sd;
			} else {
				pCPU->used = 1000;
			}
		}

		pCPU2->user = pCPU->user;
		pCPU2->nice = pCPU->nice ;
		pCPU2->system = pCPU->system;
		pCPU2->idle = pCPU->idle;
		pCPU2->irqs = pCPU->irqs;
		pCPU2->cntxts = pCPU->cntxts;
#ifdef CONFIG_SHOW_STAT
		log_debug("cpu : %u(user) %u(nice) %u(system) %u(idle) %u(irqs) %u(cntxts) %u(softirq) %u(used)",  
			pCPU->user, pCPU->nice, pCPU->system, 
			pCPU->idle, pCPU->irqs, pCPU->cntxts, 
			pCPU2->softirq, pCPU->used);
#endif
	} else {
		memset(pCPU, 0, sizeof(struct cpuinfo_t));
		return -1;
	}

	return 0;
}

static int csv_stat_get_diskinfo (struct diskinfo_t *diskinfo)
{
	int ret = 0;
	struct statfs fs;

	if (diskinfo->path == NULL) {
		return -1;
	}

	ret = statfs(diskinfo->path, &fs);
	if (ret == 0) {
		diskinfo->total = ((float)fs.f_blocks * fs.f_bsize) / 1024;
		diskinfo->free = ((float)fs.f_bfree * fs.f_bsize) / 1024;
		diskinfo->used = ((float)(fs.f_blocks - fs.f_bfree) * fs.f_bsize) / 1024;
		diskinfo->blocks_used_percent = ((float)(fs.f_blocks - fs.f_bfree)
			* 100 + fs.f_blocks - 1) / fs.f_blocks;
		if (fs.f_files > 0) {
			diskinfo->inodes_used_percent = ((float)(fs.f_files - fs.f_ffree)
				* 100 + fs.f_files - 1) / fs.f_files;
		} else {
			diskinfo->inodes_used_percent = 0;
		}
#ifdef CONFIG_SHOW_STAT
		log_debug("disk KiB: %u(total) %u(free) %u(used) %u(blocks%%) %u(inodes%%)",  
			diskinfo->total, diskinfo->free, diskinfo->used, 
			diskinfo->blocks_used_percent, diskinfo->inodes_used_percent);
#endif
	} else {
		log_err("ERROR : statfs");
		diskinfo->total = 0;
		diskinfo->free = 0;
		diskinfo->used = 0;
		diskinfo->blocks_used_percent = 0;
		diskinfo->inodes_used_percent = 0;

		return -1;
	}

	return 0;
}

static int csv_stat_get_netinfo (struct netinfo_t *netinfo)
{
	if (netinfo->dev == NULL) {
		return -1;
	}

	int ret = 0;
	struct ifreq ifreq;
	char buffer[BUFSIZ] = {0};
	char name[16] = {0};
	uint32_t values[16] = {0};
	int fd = -1;

	ret = csv_file_read_string("/proc/net/dev", buffer, sizeof(buffer));
	if (ret == -1) {
		buffer[0] = '\0';
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		log_err("ERROR : socket");
		return -1;
	}

	snprintf(ifreq.ifr_name, sizeof(ifreq.ifr_name), "%s", netinfo->dev);
	ret = ioctl(fd, SIOCGIFFLAGS, &ifreq);
	// status for snmp ifOperStatus .1.3.6.1.2.1.2.2.1.8.1 : 
	// 1:up 2:down 3:testing 4:unknown 5:dormant 6:notPresent 7:lowerLayerDown
	if (ret < 0) {
		if (ifreq.ifr_flags & IFF_UP) {
			netinfo->status = (ifreq.ifr_flags & IFF_RUNNING) ? 1 : 7;
		} else {
			netinfo->status = 2;
		}
	} else {
		netinfo->status = 4;
	}

	if (buffer[0] != '\0') {
		snprintf(name, sizeof(name), "%s:", netinfo->dev);
		utility_read_values(buffer, name, values, 16);
	} else {
		memset(values, 0, sizeof(values));
	}

	netinfo->rx_bytes = values[0];
	netinfo->rx_packets = values[1];
	netinfo->rx_errors = values[2];
	netinfo->rx_drops = values[3];
	netinfo->tx_bytes = values[8];
	netinfo->tx_packets = values[9];
	netinfo->tx_errors = values[10];
	netinfo->tx_drops = values[11];

	close(fd);
#ifdef CONFIG_SHOW_STAT
	log_debug("net : %u %u(rx) %u(rxpkt) %u %u %u(tx) %u(txpkt) %u %u",  
		netinfo->status, netinfo->rx_bytes, netinfo->rx_packets, netinfo->rx_errors,
		netinfo->rx_drops, netinfo->tx_bytes, netinfo->tx_packets, netinfo->tx_errors,
		netinfo->tx_drops);
#endif

	return 0;
}


int csv_stat_update (void)
{
	int ret = 0;
	struct csv_stat_t *pSTAT = &gCSV->stat;

	ret = csv_stat_get_process_uptime(&pSTAT->uptime_proc);
	ret |= csv_stat_get_loadinfo(&pSTAT->load);
	ret |= csv_stat_get_meminfo(&pSTAT->mem);
	ret |= csv_stat_get_cpuinfo(pSTAT);
	ret |= csv_stat_get_diskinfo(&pSTAT->disk[0]);
	ret |= csv_stat_get_netinfo(&pSTAT->net[0]);

	return ret;
}

int csv_stat_init (void)
{
	struct csv_stat_t *pSTAT = &gCSV->stat;

	pSTAT->uptime_sys = 0;
	pSTAT->uptime_proc = 0;

	pSTAT->cnt_disk = 1;
	pSTAT->disk[0].path = "/";

	pSTAT->cnt_net = 1;
	pSTAT->net[0].dev = DEV_ETH; // eth0

	return csv_stat_update();
}





#ifdef __cplusplus
}
#endif

