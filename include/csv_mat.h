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


extern int hk_msg_cameras_grab_gray (struct msg_package_t *pMP, struct msg_ack_t *pACK);

extern int hk_msg_cameras_grab_rgb (struct msg_package_t *pMP, struct msg_ack_t *pACK);

extern int msg_cameras_grab_urandom (struct msg_package_t *pMP, struct msg_ack_t *pACK);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_MAT_H__ */



