#ifndef __CSV_DLP_H__
#define __CSV_DLP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_DEV_DLP			"'tty_dlp'"
#define DEV_TTY_DLP				"/dev/ttyTHS0"

#define SIZE_DLP_BUFF			(1024)
#define DEFAULT_DLP_BAUDRATE	(9600)

typedef enum {
	OPEN_LOGO_LED			= (0),
	CLOSE_LOGO_LED			= (1),
	OPEN_LASER_LED			= (2),
	CLOSE_LASER_LED			= (3),
	OPEN_SYNC				= (4),
	CLOSE_SYNC				= (5),
	OPEN_FAN				= (6),
	CLOSE_FAN				= (7),

	RL_CMD_NUM
}rl_cmd_index;




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

