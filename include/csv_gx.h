#ifndef __CSV_GX_H__
#define __CSV_GX_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_THREAD_GX			("'thr_gx'")
#define NAME_THREAD_GRAB		("'thr_grab'")
#define NAME_THREAD_CAM_LEFT	("'cam_left'")
#define NAME_THREAD_CAM_RIGHT	("'cam_right'")

#define SIZE_CAM_STR			(64)

#define ACQ_BUFFER_NUM			(5)			///< Acquisition Buffer Qty.
#define ACQ_TRANSFER_SIZE		(64 * 1024)	///< Size of data transfer block
#define ACQ_TRANSFER_NUMBER_URB	(64)		///< Qty. of data transfer block

#define DEFAULT_WIDTH			(2048)
#define DEFAULT_HEIGHT			(1536)

#define MAX_CAM_RAW_PICS		(50)		///< 50图/相机

enum {
	GX_LIB_OPEN,				///< 打开gx库
	GX_LIB_CLOSE				///< 关闭gx库
};

enum {
	GX_START_ACQ,				///< 开采
	GX_STOP_ACQ					///< 停采
};

enum {
	GX_TRI_USE_HW_C,			///< 硬触发2上升沿 连续
	GX_TRI_USE_SW_C,			///< 软触发 连续
	GX_TRI_nUSE_C				///< 无触发 连续
};

enum {
	GX_EXPOTIME_USE,			///< 关闭自动曝光->定时
	GX_EXPOTIME_nUSE_S,			///< 一次自动曝光
	GX_EXPOTIME_nUSE_C			///< 连续自动曝光
};

enum {
	GX_GAIN_USE,				///< 关闭自动增益->指定增益
	GX_GAIN_nUSE_S,				///< 一次自动增益
	GX_GAIN_nUSE_C,				///< 连续自动增益
};

enum {
	GX_THR_PUT_LIMIT,			///< 限制带宽
	GX_THR_PUT_nLIMIT			///< 无限制带宽
};

enum {
	GX_FRAME_LIMIT,				///< 限制帧率
	GX_FRAME_nLIMIT				///< 无限制帧率
};

enum {
	GRAB_NONE					= (0),
	GRAB_CALIB_PICS				= (1),
	GRAB_DEPTHIMAGE_PICS		= (2),
	GRAB_HDRIMAGE_PICS			= (3)
};

enum {
	CAM_STATUS_IDLE				= (0),		///< 空闲
	CAM_STATUS_GRAB_BRIGHT		= (1),		///< 采普通常亮原图 1
	CAM_STATUS_CALIB_BRIGHT		= (2),		///< 采标定常亮原图 1
	CAM_STATUS_CALIB_STRIPE		= (3),		///< 采标定条纹原图 22
	CAM_STATUS_DEPTH_BRIGHT		= (4),		///< 采深度常亮原图 1
	CAM_STATUS_DEPTH_STRIPE		= (5),		///< 采深度条纹原图 13
	CAM_STATUS_HDR				= (6),		///< 采高动态原图 25+
	CAM_STATUS_STREAM			= (7),		///< 采原图做流

	CAM_STATUS_END
};

struct img_payload_t {
	uint8_t						data[DEFAULT_WIDTH*DEFAULT_HEIGHT];
};

struct cam_gx_spec_t {
	uint8_t					opened;			///< 已打开
	uint8_t					grabDone;		///< 当前组捕获完成
	uint8_t					index;			///< 左右编号

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

	int64_t					nWidth;
	int64_t					nHeight;
	GX_INT_RANGE			widthRange;
	GX_INT_RANGE			heightRange;

	double					expoTime;		// GX_FLOAT_EXPOSURE_TIME
	GX_FLOAT_RANGE			expoTimeRange;

	double					gain;			// GX_FLOAT_GAIN
	GX_FLOAT_RANGE			gainRange;

	PGX_FRAME_BUFFER		pFrameBuffer;
	uint8_t					*pMonoImageBuf;

	uint8_t					*pImgRawData;		///< 图像原始数据总缓冲区(50pics=150MB)
	struct img_payload_t	*pImgPayload;		///< 缓冲区以此分区域管理 (索引nPos)

	const char				*name_cam;		///< 相机
	pthread_t				thr_cam;		///< ID
	pthread_mutex_t			mutex_cam;		///< 锁
	pthread_cond_t			cond_cam;		///< 条件
};

struct csv_gx_t {
	uint8_t					cnt_gx;

	struct cam_gx_spec_t	Cam[TOTAL_CAMS];

	uint8_t					*pImgPCRawData;		///< 深度图计算素材总缓冲区(限定只支持26*2048*1536的图)
	struct img_payload_t	*pImgPayload;		///< 总缓冲区以此分区域管理

	uint8_t					nPos;				///< 当前在缓冲区的分区号 0~(MAX_CAM_RAW_PICS-1)

	const char				*name_gx;		///< 
	pthread_t				thr_gx;			///< ID
	pthread_mutex_t			mutex_gx;		///< 锁
	pthread_cond_t			cond_gx;		///< 条件

	uint8_t					grab_type;		///< 获取图像组类型 0: none 1:calib 2:pointcloud/depth ...
	uint8_t					busying;		///< 忙于处理图像
	uint8_t					cams_status;	///< 当前相机工作状态 CAM_STATUS_IDLE

	const char				*name_grab;		///< 取图
	pthread_t				thr_grab;		///< ID
	pthread_mutex_t			mutex_grab;		///< 锁
	pthread_cond_t			cond_grab;		///< 条件
};

extern int csv_gx_cams_acquisition (uint8_t mode);

extern int csv_gx_cams_exposure_time_selector (float expoT);

extern int csv_gx_cams_grab_both (struct csv_gx_t *pGX);

extern int csv_gx_calibrate_prepare (struct csv_gx_t *pGX);

extern int csv_gx_calibrate_bright (struct csv_gx_t *pGX);

extern int csv_gx_calibrate_stripe (struct csv_gx_t *pGX);

extern int csv_gx_cams_calibrate (struct csv_gx_t *pGX);

extern int csv_gx_cams_pointcloud (struct csv_gx_t *pGX, uint8_t towhere);

extern int csv_gx_cams_hdrimage (struct csv_gx_t *pGX);

extern int csv_gx_init (void);

extern int csv_gx_deinit (void);




#ifdef __cplusplus
}
#endif

#endif /* __CSV_GX_H__ */



