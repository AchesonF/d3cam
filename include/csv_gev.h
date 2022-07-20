#ifndef __CSV_GEV_H__
#define __CSV_GEV_H__

/* GigE Vision server */

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_UDP_GVCP				"'udp_gev'"

#define NAME_GEV_MSG				"'gev_msg'"

#define GEV_VERSION_MAJOR			(0x0001)
#define GEV_VERSION_MINOR			(0x0002)
#define GEV_DEVICE_MODE				(1)


enum {
	GEV_REG_TYPE_NONE		= 0,
	GEV_REG_TYPE_REG		= 1,		///< 寄存器
	GEV_REG_TYPE_MEM		= 2,		///< memory
};

#define GEV_REG_READ		(0x01)							///< 可读
#define GEV_REG_WRITE		(0x02)							///< 可写
#define GEV_REG_RDWR		(GEV_REG_READ|GEV_REG_WRITE)	///< 可读可写

struct reg_info_t {
	uint16_t				addr;
	uint8_t					type;		// GEV_REG_TYPE_REG/GEV_REG_TYPE_MEM
	uint8_t					mode;		// GEV_REG_RDWR
	uint32_t				value;		// reg 值
	uint16_t				length;		// 长度
	char					*info;		// mem内容
	char					*desc;		// 寄存器描述
};

struct reglist_t {
	struct reg_info_t		ri;
	struct list_head		list;
};


struct gvsp_param_t {
	int						fd;
	char					*name;
	uint16_t				port;		///< 本地系统分配端口号
	struct sockaddr_in		peer_addr;

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

	struct gvsp_param_t		stream[TOTAL_CAMS];
	struct gev_message_t	message;

	struct sockaddr_in		from_addr;		///< 来源地址参数
};

extern int csv_gev_reg_value_set (uint16_t addr, uint32_t value);

extern int csv_gvcp_trigger (struct csv_gev_t *pGEV);

extern int csv_gev_init (void);

extern int csv_gev_deinit (void);



#ifdef __cplusplus
}
#endif


#endif /* __CSV_GEV_H__ */


