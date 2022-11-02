#ifndef __CSV_MVS_H__
#define __CSV_MVS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_THREAD_MVS			("'thr_mvs'")
#define NAME_THREAD_GRAB		("'thr_grab'")

enum {
	MVS_CALIBRATE			= (0),
	MVS_POINTCLOUD			= (1),

	GRAB_END
};

// (#define INFO_MAX_BUFFER_SIZE 64) in CameraParams.h
struct cam_hk_spec_t {
	uint8_t					opened;			///< 已打开
	uint8_t					grabbing;		///< 正在抓图。两个来源互斥：控制消息抓图/gev抓图
	void					*pHandle;
	char					sn[64];			///< 序列号
	char					model[64];		///< 型号
	MVCC_FLOATVALUE			expoTime;		///< 曝光时间 us
	MVCC_FLOATVALUE			gain;			///< 增益
	MVCC_INTVALUE			sizePayload;	///< 图像大小
	MV_FRAME_OUT_INFO_EX	imgInfo;		///< 图像参数
	uint8_t					*imgData;		///< 图像数据
};

struct csv_mvs_t {
	uint8_t					cnt_mvs;		///< used cams

	MV_CC_DEVICE_INFO_LIST	stDeviceList;
	struct cam_hk_spec_t	Cam[TOTAL_CAMS];

	const char				*name_mvs;		///< 消息
	pthread_t				thr_mvs;		///< ID
	pthread_mutex_t			mutex_mvs;		///< 锁
	pthread_cond_t			cond_mvs;		///< 条件

	uint8_t					mvs_grab_type;		///< 获取图像组类型 0:calib 1:pointcloud ...
	const char				*name_grab;		///< 消息
	pthread_t				thr_grab;		///< ID
	pthread_mutex_t			mutex_grab;		///< 锁
	pthread_cond_t			cond_grab;		///< 条件
};

extern char *strMsg (int errcode);

extern int csv_mvs_cams_enum (struct csv_mvs_t *pMVS);

extern int csv_mvs_cams_reset (struct csv_mvs_t *pMVS);

extern int csv_mvs_cams_open (struct csv_mvs_t *pMVS);

extern int csv_mvs_cams_close (struct csv_mvs_t *pMVS);

extern int csv_mvs_cams_exposure_get (struct csv_mvs_t *pMVS);

extern int csv_mvs_cams_exposure_set (struct csv_mvs_t *pMVS, float fExposureTime);

extern int csv_mvs_cams_gain_get (struct csv_mvs_t *pMVS);

extern int csv_mvs_cams_gain_set (struct csv_mvs_t *pMVS, float fGain);

extern int csv_mvs_cams_name_set (struct csv_mvs_t *pMVS, char *camSNum, char *strValue);

extern int csv_mvs_cams_grab_both (struct csv_mvs_t *pMVS);


extern int csv_mvs_init (void);

extern int csv_mvs_deinit (void);



#ifdef __cplusplus
}
#endif

#endif /* __CSV_MVS_H__ */


