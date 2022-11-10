#ifndef __CSV_POINTCLOUD_H__
#define __CSV_POINTCLOUD_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
	DEPTH_TO_FILE			= (0),		// 存为本地
	DEPTH_TO_STREAM			= (1),		// 从 gev 流送出
	DEPTH_TO_INTERFACE		= (2)		// 从消息接口送出

};

extern int csv_3d_load_img (uint8_t rl, int rows, int cols, uint8_t *data);

extern int csv_3d_clear_img (uint8_t rl);

extern int csv_3d_calc (uint8_t towhere);

extern int csv_3d_init (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_POINTCLOUD_HPP__ */

