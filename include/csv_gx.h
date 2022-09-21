#ifndef __CSV_GX_H__
#define __CSV_GX_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_THREAD_GX			("'thr_gx'")
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

struct cam_gx_spec_t {
	uint8_t					opened;			///< 已打开

	GX_DEV_HANDLE			hDevice;		///< handle device

	char					vendor[SIZE_CAM_STR];
	char					model[SIZE_CAM_STR];
	char					serial[SIZE_CAM_STR];
	char					version[SIZE_CAM_STR];
	char					userid[SIZE_CAM_STR];
	int64_t					PayloadSize;
	bool					ColorFilter;

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

	const char				*name_gx;		///< 消息
	pthread_t				thr_gx;			///< ID
	pthread_mutex_t			mutex_gx;		///< 锁
	pthread_cond_t			cond_gx;		///< 条件
};

extern int csv_gx_acquisition (struct csv_gx_t *pGX, uint8_t state);

extern int csv_gx_init (void);

extern int csv_gx_deinit (void);




#ifdef __cplusplus
}
#endif

#endif /* __CSV_GX_H__ */



