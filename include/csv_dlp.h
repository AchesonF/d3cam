#ifndef __CSV_DLP_H__
#define __CSV_DLP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_DEV_DLP			"'tty_dlp'"
#define NAME_THREAD_DLP			"'thr_dlp'"
#define DEV_TTY_DLP				"/dev/ttyTHS0"

#define SIZE_DLP_BUFF			(256)
#define DEFAULT_DLP_BAUDRATE	(9600)

#define LEN_DLP_CTRL			(12)


// 起始位(0-1)[2B]+类别(2)[1B]+帧率(3)[1B]+亮度(4-5)[2B]+曝光(6-9)[4B]+校验(10-11)[2B]
// 校验位 = 0xAAAA 不做校验
#define DLP_HEAD_A			(0x36)
#define DLP_HEAD_B			(0xAA)

// 类别
enum {
	CMD_PINSTRIPE				= (0x01),	// 12 细条纹
	CMD_DEMARCATE				= (0x02),	// 22 标定
	CMD_WIDISTRIPE				= (0x03),	// 12 宽条纹
	CMD_FOCUS					= (0x04),	// 1 调焦
	CMD_BRIGHT					= (0x05),	// 1 亮光
	CMD_SINGLE_SINE				= (0x06),	// 1 单张正弦
	CMD_SINGLE_WIDESTRIPE_SINE	= (0x07),	// 1 单张宽条纹正弦
	CMD_HIGH_SPEED				= (0x0A),	// 13 高速光

	TOTAL_CMDS
};




struct csv_dlp_t {
	char					*dev;
	char					*name;

	int						fd;
	uint8_t					rbuf[SIZE_DLP_BUFF];
	uint16_t				rlen;

	uint8_t					tbuf[SIZE_DLP_BUFF];
	uint16_t				tlen;

	struct csv_tty_param_t	param;

	const char				*name_dlp;		///< 消息
	pthread_t				thr_dlp;		///< ID
	pthread_mutex_t			mutex_dlp;		///< 锁
	pthread_cond_t			cond_dlp;		///< 条件
};

extern int csv_dlp_just_write (uint8_t cmd);

extern int csv_dlp_write_and_read (uint8_t cmd);

extern int csv_dlp_init (void);

extern int csv_dlp_deinit (void);

#ifdef __cplusplus
}
#endif


#endif /* __CSV_DLP_H__ */

