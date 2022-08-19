#ifndef __CSV_PNG_H__
#define __CSV_PNG_H__


#ifdef __cplusplus
extern "C" {
#endif

#define NAME_THREAD_PNG		"'thr_png'"

#define MAX_LEN_FILE_NAME		(256)

struct png_package_t {
	char					filename[MAX_LEN_FILE_NAME];
	uint32_t				width;
	uint32_t				height;
	uint32_t				length;
	uint32_t				depth;
	uint8_t					end_pc;
	uint8_t					*payload;
};

struct pnglist_t {
	struct png_package_t	pp;
	struct list_head		list;
};


struct csv_png_t {
	struct pnglist_t		head_png;

	const char				*name_png;		///< 消息
	pthread_t				thr_png;		///< ID
	pthread_mutex_t			mutex_png;		///< 锁
	pthread_cond_t			cond_png;		///< 条件
};






extern int csv_png_push (char *filename, uint8_t *buf, 
	uint32_t length, uint32_t width, uint32_t height, uint8_t end_pc);


extern int csv_png_init (void);

extern int csv_png_deinit (void);

#ifdef __cplusplus
}
#endif

#endif /* __CSV_PNG_H__ */
