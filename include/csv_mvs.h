#ifndef __CSV_MVS_H__
#define __CSV_MVS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_THREAD_MVS			("'thr_mvs'")
#define NAME_THREAD_GRAB		("'thr_grab'")

enum {
	CAM_LEFT				= (0),
	CAM_RIGHT				= (1),
//	CAM_FRONT				= (2),
//	CAM_BACK				= (3),

	TOTAL_CAMS
};

enum {
	GRAB_DEMARCATE			= (0),
	GRAB_HIGHSPEED			= (1),

	GRAB_END
};

// (#define INFO_MAX_BUFFER_SIZE 64) in CameraParams.h
struct cam_spec_t {
	uint8_t					opened;
	uint8_t					grabbing;
	void					*pHandle;
	char					serialNum[64];
	char					modelName[64];
	MVCC_FLOATVALUE			exposureTime;
	MVCC_FLOATVALUE			gain;
	MVCC_INTVALUE			stParam;		// PayloadSize
	MV_FRAME_OUT_INFO_EX	imageInfo;
	uint8_t					*imgData;
};

struct csv_mvs_t {
	uint8_t					cnt_mvs;		///< used cams

	MV_CC_DEVICE_INFO_LIST	stDeviceList;
	struct cam_spec_t		Cam[TOTAL_CAMS];
	// todo bind dev
	uint8_t					grabing;		///< 正在抓图

	uint64_t				firstTimestamp;	///< ms
	uint64_t				lastTimestamp;

	const char				*name_mvs;		///< 消息
	pthread_t				thr_mvs;		///< ID
	pthread_mutex_t			mutex_mvs;		///< 锁
	pthread_cond_t			cond_mvs;		///< 条件

	uint8_t					grab_type;		///< 获取图像组类型 0:calib 1:pointcloud ...
	const char				*name_grab;		///< 消息
	pthread_t				thr_grab;		///< ID
	pthread_mutex_t			mutex_grab;		///< 锁
	pthread_cond_t			cond_grab;		///< 条件
};


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


