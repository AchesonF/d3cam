#ifndef __CSV_GX_H__
#define __CSV_GX_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_THREAD_GX			("'thr_gx'")
#define NAME_THREAD_GRAB		("'thr_grab'")
#define SIZE_CAM_STR			(64)

#define ACQ_BUFFER_NUM			(5)			///< Acquisition Buffer Qty.
#define ACQ_TRANSFER_SIZE		(64 * 1024)	///< Size of data transfer block
#define ACQ_TRANSFER_NUMBER_URB	(64)		///< Qty. of data transfer block

enum {
	GX_LIB_OPEN,
	GX_LIB_CLOSE
};

enum {
	GX_ACQUISITION_START,
	GX_ACQUISITION_STOP
};

enum {
	GRAB_NONE					= (0),
	GRAB_CALIB_PICS				= (1),
	GRAB_DEPTHIMAGE_PICS		= (2),
	GRAB_HDRIMAGE_PICS			= (3),

	GRAB_PICS_END
};

enum {
	ACTION_NONE					= (0),
	ACTION_CALIBRATION			= (1),
	ACTION_POINTCLOUD			= (2),

	ACTION_END
};

struct cam_gx_spec_t {
	uint8_t					opened;			///< 已打开

	GX_DEV_HANDLE			hDevice;		///< handle device

	char					vendor[SIZE_CAM_STR];
	char					model[SIZE_CAM_STR];
	char					serial[SIZE_CAM_STR];
	char					version[SIZE_CAM_STR];
	char					userid[SIZE_CAM_STR];
	int64_t					PayloadSize;
	int64_t					PixelColorFilter;
	int64_t					LinkThroughputLimit;
	int64_t					PixelFormat;
	double					FrameRate;

	double					expoTime;		// GX_FLOAT_EXPOSURE_TIME
	GX_FLOAT_RANGE			expoTimeRange;

	double					gain;			// GX_FLOAT_GAIN
	GX_FLOAT_RANGE			gainRange;

	PGX_FRAME_BUFFER		pFrameBuffer;
	uint8_t					*pMonoImageBuf;
};

struct csv_gx_t {
	uint8_t					cnt_gx;

	struct cam_gx_spec_t	Cam[TOTAL_CAMS];

	const char				*name_gx;		///< 
	pthread_t				thr_gx;			///< ID
	pthread_mutex_t			mutex_gx;		///< 锁
	pthread_cond_t			cond_gx;		///< 条件

	uint8_t					action_type;		///< 获取图像组类型 0: none 1:calib 2:pointcloud ...
	uint8_t					busying;		///< 忙于处理图像

	pthread_mutex_t			mutex_wait_depth;
	pthread_cond_t			cond_wait_depth;

	const char				*name_grab;		///< 取图
	pthread_t				thr_grab;		///< ID
	pthread_mutex_t			mutex_grab;		///< 锁
	pthread_cond_t			cond_grab;		///< 条件
};

extern int csv_gx_acquisition (uint8_t state);

extern int csv_gx_cams_exposure_set (float fExposureTime);

extern int csv_gx_cams_grab_both (struct csv_gx_t *pGX);

extern int csv_gx_cams_calibrate (struct csv_gx_t *pGX);

extern int csv_gx_cams_pointcloud (struct csv_gx_t *pGX);

extern int csv_gx_init (void);

extern int csv_gx_deinit (void);




#ifdef __cplusplus
}
#endif

#endif /* __CSV_GX_H__ */



