#ifndef __CSV_MAT_H__
#define __CSV_MAT_H__

#ifdef __cplusplus
extern "C" {
#endif

struct img_hdr_t {
	int			type;
	int			cols;
	int			rows;
	int			channel;
};


#if defined(USE_HK_CAMS)
extern int hk_msg_cameras_grab_gray (struct msg_package_t *pMP, struct msg_ack_t *pACK);

extern int hk_msg_cameras_grab_rgb (struct msg_package_t *pMP, struct msg_ack_t *pACK);

#elif defined(USE_GX_CAMS)

extern bool csv_mat_img_save (uint32_t height, uint32_t width, void *imgData, char *out_file);

extern int gx_msg_cameras_grab_gray (struct msg_package_t *pMP, struct msg_ack_t *pACK);

extern int gx_msg_cameras_grab_rgb (struct msg_package_t *pMP, struct msg_ack_t *pACK);

extern int gx_msg_cameras_grab_img_depth (struct msg_package_t *pMP, struct msg_ack_t *pACK);

#endif

extern int msg_cameras_grab_urandom (struct msg_package_t *pMP, struct msg_ack_t *pACK);



#ifdef __cplusplus
}
#endif

#endif /* __CSV_MAT_H__ */



