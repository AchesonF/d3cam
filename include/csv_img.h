#ifndef __CSV_BMP_H__
#define __CSV_BMP_H__


#ifdef __cplusplus
extern "C" {
#endif

#define NAME_THREAD_IMG			"'thr_img'"

#define MAX_LEN_FILE_NAME		(256)

struct img_package_t {
	char					filename[MAX_LEN_FILE_NAME];
	uint32_t				width;
	uint32_t				height;
	uint32_t				length;
	uint8_t					position;	///< 左右位
	uint8_t					grabtype;	///< 抓取动作组合类型
	uint8_t					lastPic;

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

extern int csv_img_generate_filename (char *path, uint16_t group, 
	int idx, int lr, char *img_file);

extern int csv_img_generate_depth_filename (char *path, uint16_t group, char *img_file);

extern int csv_img_clear (char *path);

extern int csv_img_push (char *filename, uint8_t *pRawData, uint32_t length, 
	uint32_t nWidth, uint32_t nHeight, uint8_t pos, uint8_t grabtype, uint8_t lastpic);

extern void csv_img_list_release (struct csv_img_t *pIMG);

extern int csv_img_init (void);

extern int csv_img_deinit (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_BMP_H__ */


