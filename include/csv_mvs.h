#ifndef __CSV_MVS_H__
#define __CSV_MVS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_THREAD_MVS			("'thr_mvs'")

#define MAX_CAMERA_NUM			(2) // 4


// (#define INFO_MAX_BUFFER_SIZE 64) in CameraParams.h
struct cam_spec_t {
	uint8_t					opened;
	void					*cameraHandle;
	char					serialNum[64];
	char					modelName[64];
	MVCC_FLOATVALUE			exposureTime;
	MVCC_FLOATVALUE			camGain;

	MV_FRAME_OUT_INFO_EX	imageInfo;
	uint8_t					*imgData;
};

struct csv_mvs_t {
	uint8_t					cnt_mvs;
	uint8_t					bExit;

	MV_CC_DEVICE_INFO_LIST	stDeviceList;
	struct cam_spec_t		cam[MAX_CAMERA_NUM];

	// todo bind dev

	const char				*name_mvs;		///< 消息
	pthread_t				thr_mvs;		///< ID
	pthread_mutex_t			mutex_mvs;		///< 锁
	pthread_cond_t			cond_mvs;		///< 条件
};


extern int csv_mvs_cams_enum (void);

extern int csv_mvs_cams_open (void);

extern int csv_mvs_cams_close (void);

extern int csv_mvs_cams_exposure_get (void);

extern int csv_mvs_cams_exposure_set (float fExposureTime);

extern int csv_mvs_cams_gain_get (void);

extern int csv_mvs_cams_gain_set (float fGain);

extern int csv_mvs_cams_name_set (char *camSNum, char *strValue);

extern int csv_mvs_cams_grab_both (void);

extern int csv_mvs_init (void);

extern int csv_mvs_deinit (void);



#ifdef __cplusplus
}
#endif

#endif /* __CSV_MVS_H__ */


