#ifndef __CSV_MAIN_H__
#define __CSV_MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_PID_LOCK			"/var/lock/" CONFIG_APP_NAME ".pid"

enum {
	STREAM_TCP			= (1),		///< 显示 TCP 数据流
	STREAM_TTY			= (2),		///< 显示 TTY 数据流
	STREAM_UDP			= (3),		///< 显示 UDP 数据流
	STREAM_SQL			= (4),		///< 显示 SQL 数据流

	STREAM_DATA			= (9),		// just for data show

	STREAM_ALL			= (255)		///< 显示全部数据流
};

struct csv_product_t {
	uint8_t				tlog;					///< terminal 显示log日志0:不显示
	uint8_t				tdata;					///< terminal 显示data数据
												///< 1:tcp 2:tty 3:udp 4:sql ...9:data 255:all
	uint8_t				dis_daemon;				///< 禁用守护进程
	char				WebCfg[256];			///< web 配置文件


	char				*app_name;				///< 本软件名称
	char				*app_version;			///< 本软件版本
	char				*app_buildtime;			///< 本软件编译时间
	char				*compiler_version;		///< 编译链版本
	uint32_t			build_timestamp;		///< 编译时间戳
	char				app_info[128];			///< 版本字节流

	char				kernel_version[132];	///< linux kernel version, such as "Linux 5.10.14 #12"
	char				kernel_buildtime[32];	///< yyyy-mm-dd HH:MM:SS
	char				filesystem_version[32];	///< 文件系统版本

	uint32_t			uptime;					///< 操作系统运行时间，单位s
	uint32_t			app_runtime;			///< 本软件运行时间，单位s

	int					fd_lock;

	struct csv_lifetime_t	lifetime;			///< 工时信息
	struct csv_hb_t			hb;					///< 守护进程及交互
};

struct csv_info_t {
	struct csv_file_t	file;					///< 外部配置文件
	struct csv_xml_t	xml;					///< config file using xml
	struct csv_cfg_t	cfg;
	struct csv_ifcfg_t	ifcfg;
	struct csv_stat_t	stat;					///< 系统状态信息
	struct csv_gpi_t	gpi;					///< 输入按键事件

	struct csv_uevent_t	uevent;					///< 探测内核事件
#if defined(USE_HK_CAMS)
	struct csv_mvs_t	mvs;					///< 海康相机
#elif defined(USE_GX_CAMS)
	struct csv_gx_t		gx;						///< 大恒相机
#endif

	struct csv_gvcp_t	gvcp;					///< GVCP 控制通道
	struct csv_gvsp_t	gvsp;					///< GVSP 流通道
//	struct csv_gevmsg_t	gvmsg;					///< GVMessage 消息通道
	struct csv_gev_t	gev;					///< GigE Vision

	struct csv_dlp_t	dlp;					///< 光机控制通道

	struct csv_tcp_t	tcpl;					///< 本地tcp服务
	struct csv_web_t	web;					///< WEB 服务

	struct csv_tick_t	tick;					///< 轮询时钟

	struct csv_msg_t	msg;					///< 接口消息处理

	struct csv_img_t	img;					///< 延后存图
};


extern struct csv_product_t gPdct;

extern struct csv_info_t *gCSV;

extern int csv_lock_close (void);

#ifdef __cplusplus
}
#endif

#endif /* __CSV_MAIN_H__ */

