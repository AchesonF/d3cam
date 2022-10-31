#ifndef __CSV_POINTCLOUD_H__
#define __CSV_POINTCLOUD_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int csv_3d_load_im (uint8_t rl, int rows, int cols, uint8_t *data);

extern int csv_3d_clear_im (uint8_t rl);

extern int csv_3d_calc (void);

extern int csv_3d_init (void);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_POINTCLOUD_HPP__ */

