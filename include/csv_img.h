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


#define NAME_THREAD_IMG			"'thr_img'"

#define MAX_LEN_FILE_NAME		(256)

struct img_package_t {
	char					filename[MAX_LEN_FILE_NAME];
	uint32_t				width;
	uint32_t				height;
	uint32_t				length;
	uint8_t					position;	///< 左右位
	uint8_t					flip;		///< 上下翻转

	uint8_t					*payload;
};

struct img_list_t {
	struct img_package_t	ipk;
	struct list_head		list;
};

struct csv_img_t {
	struct img_list_t		head_img;

	const char				*name_img;		///< 消息
	pthread_t				thr_img;		///< ID
	pthread_mutex_t			mutex_img;		///< 锁
	pthread_cond_t			cond_img;		///< 条件
};


extern int csv_img_push (char *filename, uint8_t *pRawData, 
	uint32_t length, uint32_t nWidth, uint32_t nHeight, uint8_t pos);


extern int gray_raw2bmp (uint8_t *pRawData, uint32_t nWidth, 
	uint32_t nHeight, char *pBmpName);

extern int gray_raw2png(void *image, size_t length, uint32_t width, 
	uint32_t height, int bit_depth, char *out_file);


extern int csv_img_init (void);

extern int csv_img_deinit (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_BMP_H__ */


