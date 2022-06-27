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


extern int msg_cameras_grab_image (struct msg_package_t *pMP, struct msg_ack_t *pACK);


#ifdef __cplusplus
}
#endif

#endif /* __CSV_MAT_H__ */



