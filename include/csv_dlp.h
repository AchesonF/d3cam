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
#define CMD_POINTCLOUD		(0x01)	// NUM_PICS_POINTCLOUD 深度采图
#define CMD_CALIB			(0x02)	// NUM_PICS_CALIB 标定条纹采图
#define CMD_BRIGHT			(0x05)	// NUM_PICS_BRIGHT 常亮采图
#define CMD_HDRI			(0x05)	// 常亮采多图

// 图数
#define NUM_PICS_POINTCLOUD		(13)	// 深度采图
#define NUM_PICS_CALIB			(22)	// 标定条纹采图
#define NUM_PICS_BRIGHT			(1)		// 常亮采图

typedef enum {
	DLP_CMD_POINTCLOUD			= (0),
	DLP_CMD_CALIB				= (1),
	DLP_CMD_BRIGHT				= (2),
	DLP_CMD_HDRI				= (3),

	TOTAL_DLP_CMDS
} eDLP_CMD_t;



struct csv_dlp_t {
	char					*dev;
	char					*name;

	int						fd;
	uint8_t					rbuf[SIZE_DLP_BUFF];
	uint16_t				rlen;

	uint8_t					tbuf[SIZE_DLP_BUFF];
	uint16_t				tlen;

	struct csv_tty_param_t	param;

	float					rate;			///< 帧率
	float					brightness;		///< 亮度
	float					expoTime;		///< 曝光时间

	const char				*name_dlp;		///< 消息
	pthread_t				thr_dlp;		///< ID
	pthread_mutex_t			mutex_dlp;		///< 锁
	pthread_cond_t			cond_dlp;		///< 条件
};

extern int csv_dlp_write_only (eDLP_CMD_t eCmd);

extern int csv_dlp_just_write (eDLP_CMD_t eCmd);

extern int csv_dlp_write_and_read (eDLP_CMD_t eCmd);

extern int csv_dlp_init (void);

extern int csv_dlp_deinit (void);

#ifdef __cplusplus
}
#endif


#endif /* __CSV_DLP_H__ */

