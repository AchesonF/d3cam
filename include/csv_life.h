#ifndef __CSV_LIFE_H__
#define __CSV_LIFE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_PATH_LIFETIME			(PATH_DATA_FILES "/.lifetime") ///< 总工时文件


struct csv_lifetime_t {
	const char			*file;					///< 工时记录文件
	uint32_t			total;					///< 总工时 mins
	uint32_t			current;				///< 本次工时 mins
	uint32_t			last;					///< 上次工时 mins
	uint32_t			count;					///< 工时统计次数
};


extern int csv_life_update (void);

extern int csv_life_start (void);

extern int csv_life_init (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_LIFE_H__ */



