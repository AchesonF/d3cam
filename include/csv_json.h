#ifndef __CSV_JSON_H__
#define __CSV_JSON_H__

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_NAME_CJSON			"csvcfg.json"

struct device_param_t {
	int					device_type;
	uint8_t				camera_leftright_type;
	uint8_t				camera_img_type;
	double				exposure_time;
	double				exposure_time_for_rgb;
};

struct depthimg_param_t {
	int					numDisparities;
	int					blockSize;
	int					uniqRatio;
};

struct csv_json_t {
	char				*name;

	struct device_param_t device_param;
	struct depthimg_param_t depthimg_param;



};


extern int csv_json_init (void);

extern int csv_json_deinit (void);


#ifdef __cplusplus
}
#endif


#endif /* __CSV_JSON_H__ */

