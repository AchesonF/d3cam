#ifndef __CSV_GEV_H__
#define __CSV_GEV_H__

/* GigE Vision server */

#ifdef __cplusplus
extern "C" {
#endif



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

struct csv_gev_t {
	struct reglist_t		head_reg;		///< 寄存器链表

};

extern uint32_t csv_gev_reg_value_get (uint32_t addr, uint32_t *value);
extern int csv_gev_reg_value_set (uint32_t addr, uint32_t value);
extern uint16_t csv_gev_mem_info_get_length (uint32_t addr);
extern int csv_gev_mem_info_get (uint32_t addr, char **info);
extern int csv_gev_mem_info_set (uint32_t addr, char *info);
extern uint8_t csv_gev_reg_type_get (uint32_t addr, char **desc);
extern int csv_gev_reg_value_update (uint32_t addr, uint32_t value);

extern int csv_gev_init (void);

extern int csv_gev_deinit (void);



#ifdef __cplusplus
}
#endif


#endif /* __CSV_GEV_H__ */


