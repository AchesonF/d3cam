#ifndef __CSV_GEV_H__
#define __CSV_GEV_H__

/* GigE Vision server */

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_UDP_GVCP		"'udp_gev'"

enum {
	GEV_REG_TYPE_REG		= 0,		///< 寄存器
	GEV_REG_TYPE_MEM		= 1			///< memory
};

#define GEV_REG_READ		(0x01)							///< 可读
#define GEV_REG_WRITE		(0x02)							///< 可写
#define GEV_REG_RDWR		(GEV_REG_READ|GEV_REG_WRITE)	///< 可读可写

struct reg_info_t {
	uint16_t				addr;
	uint8_t					type;		// GEV_REG_TYPE_REG/GEV_REG_TYPE_MEM
	uint8_t					mode;		// GEV_REG_RDWR
	uint16_t				length;		// 长度
	uint32_t				value;		// reg 值
	char					*info;		// mem内容
	char					*desc;		// 寄存器描述
};

struct reglist_t {
	struct reg_info_t		ri;
	struct list_head		list;
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

	struct sockaddr_in		from_addr;		///< 来源地址参数
	struct sockaddr_in		target_addr;	///< 媒体流流向参数
};


extern int csv_gev_trigger (struct csv_gev_t *pGEV);

extern int csv_gev_init (void);

extern int csv_gev_deinit (void);



#ifdef __cplusplus
}
#endif


#endif /* __CSV_GEV_H__ */


