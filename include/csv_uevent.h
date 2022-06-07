#ifndef __CSV_UEVENT_H__
#define __CSV_UEVENT_H__

#ifdef __cplusplus
extern "C" {
#endif


#define NAME_UEVENT			"'uevent'"

#define LEN_UEVENT_MSG		(4096)

struct kmsg_uevent_t {
	const char		*action;		// ACTION=	add/remove/change/move/online/offline
	const char		*devpath;		// DEVPATH=  /devices/...
	const char		*subsystem;		// SUBSYSTEM= usb/...
	const char		*devtype;		// DEVTYPE=   usb_device/usb_interface/...
	int				major;			// MAJOR=
	int				minor;			// MINOR=
};


struct csv_uevent_t {
	char				*name;
	int					fd;


};

extern int csv_uevent_trigger (struct csv_uevent_t *pUE);

extern int csv_uevent_init (void);

extern int csv_uevent_deinit (void);

#ifdef __cplusplus
}
#endif

#endif /* __CSV_UEVENT_H__ */

