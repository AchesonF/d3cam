#ifndef __CSV_DLP_H__
#define __CSV_DLP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_DEV_DLP			"'tty_dlp'"
#define DEV_TTY_DLP				"/dev/ttyTHS0"

#define SIZE_DLP_BUFF			(1024)
#define DEFAULT_DLP_BAUDRATE	(9600)

#define LEN_DLP_CTRL			(12)

typedef enum {
	PINSTRIPE				= (0),		// 细条纹
	DEMARCATE				= (1),		// 标定
	WIDISTRIPE				= (2),		// 宽条纹
	FOCUS					= (3),		// 调焦
	BRIGHT					= (4),		// 亮光
	SINGLE_SINE				= (5),		// 单张正弦
	SINGLE_WIDESTRIPE_SINE	= (6),		// 单张宽条纹正弦
	FOCUS_BRIGHT			= (7),		// 调焦常亮
	LIGHT_BRIGHT			= (8),		// 亮光常亮
	PINSTRIPE_SINE_BRIGHT	= (9),		// 细纹正弦常亮
	WIDESTRIPE_SINE_BRIGHT	= (10),		// 宽纹正弦常亮

	TOTAL_DLP_CMD
} dlp_ctrl_idx;


struct csv_dlp_t {
	char					*dev;
	char					*name;

	int						fd;
	uint8_t					rbuf[SIZE_DLP_BUFF];
	uint16_t				rlen;

	struct csv_tty_param_t	param;
};

extern int csv_dlp_write (uint8_t idx);

extern int csv_dlp_read (void);

extern int csv_dlp_init (void);

extern int csv_dlp_deinit (void);

#ifdef __cplusplus
}
#endif


#endif /* __CSV_DLP_H__ */

