#ifndef __CSV_STAT_H__
#define __CSV_STAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NR_DISKS				(2)		///< 系统存储分区  		"/" & "/tmp"
#define MAX_NR_INTERFACES			(2)		///< "eth0"

struct diskinfo_t {
	const char				*path;
	uint32_t				total;
	uint32_t				free;
	uint32_t				used;
	uint32_t				blocks_used_percent;
	uint32_t				inodes_used_percent;
};

struct loadinfo_t {
	uint32_t				avg[3];
};

struct meminfo_t {
	uint32_t				total;
	uint32_t				free;
	uint32_t				shared;
	uint32_t				buffers;
	uint32_t				cached;
	uint32_t				used;			///< 使用率 0.1%
};

struct cpuinfo_t {
	uint32_t				user;
	uint32_t				nice;
	uint32_t				system;
	uint32_t				idle;
	uint32_t				irqs;
	uint32_t				cntxts;
	uint32_t				softirq;
	int						used;			///< 使用率 0.1%
};

struct netinfo_t {
	char					dev[IFNAMSIZ];
	uint32_t				status;
	uint32_t				rx_bytes;
	uint32_t				rx_packets;
	uint32_t				rx_errors;
	uint32_t				rx_drops;
	uint32_t				tx_bytes;
	uint32_t				tx_packets;
	uint32_t				tx_errors;
	uint32_t				tx_drops;
};

struct csv_stat_t {
	uint32_t			uptime_sys;				///< 1/100s
	uint32_t			uptime_proc;			///< 1/100s
	uint8_t				cnt_disk;
	struct diskinfo_t	disk[MAX_NR_DISKS];		///< 存储分区信息
	struct loadinfo_t	load;					///< 负载信息
	struct meminfo_t	mem;					///< 内存信息
	struct cpuinfo_t	cpu[2];					///< cpu信息
	uint8_t				cnt_net;
	struct netinfo_t	net[MAX_NR_INTERFACES];	///< 网络信息
};


extern int csv_stat_update (void);

extern int csv_stat_init (void);





#ifdef __cplusplus
}
#endif

#endif /* __CSV_STAT_H__ */



