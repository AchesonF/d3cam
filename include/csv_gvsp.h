#ifndef __CSV_GVSP_H__
#define __CSV_GVSP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_SOCKET_STREAM					"'udp_stream'"
#define NAME_THREAD_STREAM					"'thr_stream'"

#define GVSP_PACKET_MAX_SIZE		(9000)
#define GVSP_PACKET_MIN_SIZE		(1500)

struct payload_data_t {
	uint64_t				timestamp;	// ms
	uint32_t				pixelformat;// pixel
	uint32_t				width;		// 宽
	uint32_t				height;		// 高
	uint32_t				offset_x;
	uint32_t				offset_y;
	uint16_t				padding_x;
	uint16_t				padding_y;
	uint16_t				payload_type;	// 负载类型
	uint64_t				payload_size;	// 负载数据长度
	char					filename[MAX_SIZE_FILENAME];
	uint8_t					*payload;
};

struct stream_list_t {
	struct payload_data_t	pd;
	struct list_head		list;
};

struct gvsp_stream_t {
	int						fd;
	char					*name;
	uint8_t					idx;

	// for ver 1
	uint16_t				block_id;		///< reset to 1 when the stream channel is opened
	uint32_t				packet_id;		///< reset to 0 at the start of each data block

	// for ver 2
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


	uint8_t					enable_test;
	pthread_t				thr_test;
	pthread_mutex_t			mutex_test;
	pthread_cond_t			cond_test;
};

struct csv_gvsp_t {
	struct gvsp_stream_t	stream;
};

extern int csv_gvsp_data_fetch (struct gvsp_stream_t *pStream, uint16_t type,
	uint8_t *pData, uint64_t Length, PGX_FRAME_BUFFER imgInfo, char *filename);

extern int csv_gvsp_packet_test (struct gvsp_stream_t *pStream, 
	uint8_t *pData, uint32_t length);

extern int csv_gvsp_init (void);

extern int csv_gvsp_deinit (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_GVSP_H__ */


