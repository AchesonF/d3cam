#ifndef __CSV_MAIN_H__
#define __CSV_MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif


struct csv_product_t {
	uint8_t				tlog;					///< terminal 显示log日志0:不显示
	uint8_t				tdata;					///< terminal 显示data数据
												///< 1:tcp 2:tty 3:udp 4:sql ...255:all

	char				*app_name;				///< 本软件名称
	char				*app_version;			///< 本软件版本
	char				*app_buildtime;			///< 本软件编译时间
	uint32_t			build_timestamp;		///< 编译时间戳
	char				app_info[64];			///< 版本字节流

	char				kernel_version[32];		///< linux kernel version, such as "Linux 5.10.14 #12"
	char				kernel_buildtime[32];	///< yyyy-mm-dd HH:MM:SS
	char				filesystem_version[32];	///< 文件系统版本

	uint32_t			uptime;					///< 操作系统运行时间，单位s
	uint32_t			app_runtime;			///< 本软件运行时间，单位s

	struct csv_lifetime_t	lifetime;			///< 工时信息


};

struct csv_info_t {

	struct csv_tick_t	tick;					///< 轮询时钟

	struct csv_file_t	file;					///< 外部配置文件

};


extern struct csv_product_t gPdct;

extern struct csv_info_t *gCSV;


#ifdef __cplusplus
}
#endif

#endif /* __CSV_MAIN_H__ */

