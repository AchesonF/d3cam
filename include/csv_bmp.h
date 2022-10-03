#ifndef __CSV_BMP_H__
#define __CSV_BMP_H__


#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)


struct bitmap_file_header_t {
	uint16_t		bfType;
	uint32_t		bfSize;
	uint16_t		bfReserved1;
	uint16_t		bfReserved2;
	uint32_t		bfOffBits;
};

struct bitmap_info_header_t {
	uint32_t		biSize;
	uint32_t		biWidth;
	uint32_t 		biHeight;
	uint16_t		biPlanes;
	uint16_t		biBitCount;
	uint32_t		biCompression;
	uint32_t		biSizeImage;
	uint32_t		biXPelsPerMeter;
	uint32_t		biYPelsPerMeter;
	uint32_t		biClrUsed;
	uint32_t		biClrImportant;
};

struct rgbquad_t {
	uint8_t			rgbBlue;
	uint8_t			rgbGreen;
	uint8_t			rgbRed;
	uint8_t			rgbReserved;
};

#pragma pack(pop)


#define NAME_THREAD_BMP			"'thr_bmp'"

#define MAX_LEN_FILE_NAME		(256)

struct bmp_package_t {
	char					filename[MAX_LEN_FILE_NAME];
	uint32_t				width;
	uint32_t				height;
	uint32_t				length;

	uint8_t					*payload;
};

struct bmp_list_t {
	struct bmp_package_t	bp;
	struct list_head		list;
};

struct csv_bmp_t {
	struct bmp_list_t		head_bmp;

	const char				*name_bmp;		///< 消息
	pthread_t				thr_bmp;		///< ID
	pthread_mutex_t			mutex_bmp;		///< 锁
	pthread_cond_t			cond_bmp;		///< 条件
};


extern int csv_bmp_push (char *filename, uint8_t *pRawData, 
	uint32_t length, uint32_t nWidth, uint32_t nHeight);


extern int gray_raw2bmp (uint8_t *pRawData, uint32_t nWidth, 
	uint32_t nHeight, char *pBmpName);


extern int csv_bmp_init (void);

extern int csv_bmp_deinit (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_BMP_H__ */


