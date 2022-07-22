#ifndef __CSV_GEV_H__
#define __CSV_GEV_H__

/* GigE Vision server */

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_UDP_GVCP				"'udp_gev'"

#define NAME_GEV_MSG				"'gev_msg'"

#define GEV_DEVICE_MODEL_NAME		"CS-3D001-28HS"
#define GEV_XML_FILENAME			GEV_DEVICE_MODEL_NAME ".zip" //GEV_DEVICE_MODEL_NAME ".xml" // GEV_DEVICE_MODEL_NAME ".zip" 


enum {
	GEV_REG_TYPE_NONE		= 0,
	GEV_REG_TYPE_REG		= 1,		///< 寄存器
	GEV_REG_TYPE_MEM		= 2,		///< memory
};

#define GEV_REG_READ		(0x01)							///< 可读
#define GEV_REG_WRITE		(0x02)							///< 可写
#define GEV_REG_RDWR		(GEV_REG_READ|GEV_REG_WRITE)	///< 可读可写

struct reg_info_t {
	uint32_t				addr;
	uint32_t				value;		// reg 值
	uint8_t					type;		// GEV_REG_TYPE_REG/GEV_REG_TYPE_MEM
	uint8_t					mode;		// GEV_REG_RDWR
	uint16_t				length;		// 长度
	char					*info;		// mem内容
	char					*desc;		// 寄存器描述
};

struct reglist_t {
	struct reg_info_t		ri;
	struct list_head		list;
};






struct image_info_t {
	uint64_t				timestamp;	// ms
	uint32_t				format;		// pixel
	uint32_t				width;
	uint32_t				height;
	uint32_t				offset_x;
	uint32_t				offset_y;
	uint16_t				padding_x;
	uint16_t				padding_y;
	uint8_t					*payload;
};

struct stream_list_t {
	struct image_info_t		img;
	struct list_head		list;
};

#define GRAB_STATUS_NONE			0
#define GRAB_STATUS_RUNNING			1
#define GRAB_STATUS_STOP			2

struct gvsp_stream_t {
	int						fd;
	char					*name;
	uint8_t					idx;
	uint8_t					grab_status;	///< 抓图状态 0:未知 1:抓取中 2:结束
	uint32_t				blockid;
	uint32_t				packetid;
	uint32_t				re_packetid;	///< 请求重发id
	uint16_t				port;		///< 本地系统分配端口号
	struct sockaddr_in		peer_addr;

	struct stream_list_t	head_stream;

	const char				*name_stream;	///< 流
	pthread_t				thr_stream;		///< ID
	pthread_mutex_t			mutex_stream;	///< 锁
	pthread_cond_t			cond_stream;	///< 条件

	const char				*name_grab;		///< 抓图
	pthread_t				thr_grab;
};

struct gev_message_t {
	int						fd;
	char					*name;
	uint16_t				port;		///< 本地系统分配端口号
	struct sockaddr_in		peer_addr;

};

struct csv_gev_t {
	int						fd;			///< 文件描述符
	char					*name;		///< 链接名称
	uint16_t				port;		///< udp 监听端口
	uint16_t				ReqId;		///< 发起的消息序号

	uint32_t				rxlen;
	uint8_t					rxbuf[GVCP_MAX_MSG_LEN];

	uint32_t				txlen;
	uint8_t					txbuf[GVCP_MAX_MSG_LEN];

	struct reglist_t		head_reg;		///< 寄存器链表

	struct gvsp_stream_t	stream[TOTAL_CAMS];
	struct gev_message_t	message;

	struct sockaddr_in		from_addr;		///< 来源地址参数
};

extern int csv_gev_reg_value_update (uint32_t addr, uint32_t value);

extern int csv_gvcp_trigger (struct csv_gev_t *pGEV);


extern int csv_mvs_cam_grab_thread (uint8_t idx);

extern int csv_gev_init (void);

extern int csv_gev_deinit (void);



#ifdef __cplusplus
}
#endif


#endif /* __CSV_GEV_H__ */


