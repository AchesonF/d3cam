#ifndef __CSV_TTY_H__
#define __CSV_TTY_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
	BR_4800							= (0),		// 4800 bps
	BR_9600							= (1),		// 9600 bps
	BR_19200						= (2),		// 19200 bps
	BR_38400						= (3),		// 38400 bps
	BR_57600						= (4),		// 57600 bps
	BR_115200						= (5),		// 115200 bps
	BR_230400						= (6),		// 230400 bps
	BR_468000						= (7),		// 460800 bps
	BR_921600						= (8),		// 921600 bps

	TOTAL_BAUDRATE
};

enum {
	PARITY_N						= (0),
	PARITY_O						= (1),
	PARITY_E						= (2),

	TOTAL_PARITY
};


/** @name    串口配置参数
* @{ */
struct csv_tty_param_t {
	uint32_t			baudrate;					///< 波特率
	uint8_t				databits;					///< 数据位
	uint8_t				stopbits;					///< 停止位
	char				parity;						///< 校验位
	uint8_t				flowcontrol;				///< 流控制

	uint8_t				delay;						///< 接收延时 单位 100ms
};
/** @} */


extern int csv_tty_flush (int fd);

extern int csv_tty_drain (int fd);

extern int csv_tty_read (int fd, uint8_t *buf, int nbytes);

extern int csv_tty_write (int fd, uint8_t *buf, int nbytes);

extern int csv_tty_init (char *tty_name, struct csv_tty_param_t *pPARAM);

extern int csv_tty_deinit (int fd);



#ifdef __cplusplus
}
#endif

#endif /* __CSV_TTY_H__ */
