#ifndef __CSV_POINTCLOUD_H__
#define __CSV_POINTCLOUD_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
	DEPTH_TO_FILE			= (0),
	DEPTH_TO_STREAM			= (1)

};

extern int csv_3d_load_img (uint8_t rl, int rows, int cols, uint8_t *data);

extern int csv_3d_clear_img (uint8_t rl);

extern int csv_3d_calc (uint8_t what);

extern int csv_3d_init (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_POINTCLOUD_HPP__ */

