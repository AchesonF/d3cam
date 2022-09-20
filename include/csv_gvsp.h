#ifndef __CSV_GVSP_H__
#define __CSV_GVSP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define GVSP_PACKET_MAX_SIZE		(9000)

enum {
	GRAB_STATUS_NONE		= 0,
	GRAB_STATUS_RUNNING		= 1,
	GRAB_STATUS_STOP		= 2
};

struct image_info_t {
	uint64_t				timestamp;	// ms
	uint32_t				pixelformat;		// pixel
	uint32_t				length;
	uint32_t				width;
	uint32_t				height;
	uint32_t				offset_x;
	uint32_t				offset_y;
	uint16_t				padding_x;
	uint16_t				padding_y;
	uint8_t					*payload;
};

struct stream_list_t {
	struct image_info_t		img;
	struct list_head		list;
};

struct gvsp_stream_t {
	int						fd;
	char					*name;
	uint8_t					idx;
	uint8_t					grab_status;	///< 抓图状态 0:未知 1:抓取中 2:结束
	uint64_t				block_id64;		///< reset to 1 when the stream channel is opened
	uint32_t				packet_id32;	///< reset to 0 at the start of each data block
	uint32_t				re_packetid;	///< 请求重发id

	uint16_t				port;			///< 本地系统分配端口号
	struct sockaddr_in		peer_addr;		///< 目标地址/端口

	struct stream_list_t	head_stream;

	uint8_t					bufSend[GVSP_PACKET_MAX_SIZE];
	uint32_t				lenSend;

	const char				*name_stream;	///< 流
	pthread_t				thr_stream;
	pthread_mutex_t			mutex_stream;
	pthread_cond_t			cond_stream;

	const char				*name_gevgrab;	///< 抓图
	pthread_t				thr_gevgrab;
	pthread_mutex_t			mutex_gevgrab;
	pthread_cond_t			cond_gevgrab;
};

struct csv_gvsp_t {
	struct gvsp_stream_t	stream[TOTAL_CAMS];
};

extern int csv_gvsp_packet_test (struct gvsp_stream_t *pStream, 
	uint8_t *pData, uint32_t length);

extern int csv_gvsp_init (void);

extern int csv_gvsp_deinit (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_GVSP_H__ */


