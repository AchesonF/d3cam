#ifndef __CSV_CFG_H__
#define __CSV_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif


struct device_param_t {
	int					device_type;
	uint8_t				camera_leftright_type;
	uint8_t				camera_img_type;
	float				exposure_time;
	float				exposure_time_for_rgb;
};

struct depthimg_param_t {
	int					numDisparities;
	int					blockSize;
	int					uniqRatio;
};


struct gev_param_t {
	uint32_t				Version;	// hi16 major, low16 minor
	uint32_t				DeviceMode;
	uint32_t				MacHi;		// mac[0-1]
	uint32_t				MacLow;		// mac[2-5]

};


struct csv_cfg_t {
	struct device_param_t	device_param;
	struct depthimg_param_t depthimg_param;

	struct gev_param_t		gp;				///< gev参数
};


extern int csv_cfg_init (void);



#ifdef __cplusplus
}
#endif

#endif /* __CSV_CFG_H__ */

