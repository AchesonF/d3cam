#ifndef __CSV_SO_H__
#define __CSV_SO_H__

#ifdef __cplusplus
extern "C" {
#endif

struct something_data_t {
	uint8_t			data1;
	uint32_t		data2;
	uint8_t			buf[1024];
};

typedef int (*SORUTINE)(int, uint32_t, struct something_data_t);

#define PATH_SO_FILE		"./some.so"

struct csv_so_t {
	char			*path;
	void 			*handle;

	void			*func1;
	void			*func2;

};


#ifdef __cplusplus
}
#endif

#endif /* __CSV_SO_H__ */

