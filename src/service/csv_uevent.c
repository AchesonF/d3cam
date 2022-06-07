#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
remove@/devices/3610000.xhci/usb2/2-3/2-3.1/2-3.1:1.0
ACTION=remove
DEVPATH=/devices/3610000.xhci/usb2/2-3/2-3.1/2-3.1:1.0
SUBSYSTEM=usb
DEVTYPE=usb_interface
PRODUCT=2bdf/1/100
TYPE=239/2/1
INTERFACE=239/5/0
MODALIAS=usb:v2BDFp0001d0100dcEFdsc02dp01icEFisc05ip00in00
SEQNUM=6369

remove@/devices/3610000.xhci/usb2/2-3/2-3.1/2-3.1:1.1
ACTION=remove
DEVPATH=/devices/3610000.xhci/usb2/2-3/2-3.1/2-3.1:1.1
SUBSYSTEM=usb
DEVTYPE=usb_interface
PRODUCT=2bdf/1/100
TYPE=239/2/1
INTERFACE=239/5/1
MODALIAS=usb:v2BDFp0001d0100dcEFdsc02dp01icEFisc05ip01in01
SEQNUM=6370

remove@/devices/3610000.xhci/usb2/2-3/2-3.1/2-3.1:1.2
ACTION=remove
DEVPATH=/devices/3610000.xhci/usb2/2-3/2-3.1/2-3.1:1.2
SUBSYSTEM=usb
DEVTYPE=usb_interface
PRODUCT=2bdf/1/100
TYPE=239/2/1
INTERFACE=239/5/2
MODALIAS=usb:v2BDFp0001d0100dcEFdsc02dp01icEFisc05ip02in02
SEQNUM=6371

remove@/devices/3610000.xhci/usb2/2-3/2-3.1
ACTION=remove
DEVPATH=/devices/3610000.xhci/usb2/2-3/2-3.1
SUBSYSTEM=usb
MAJOR=189
MINOR=135
DEVNAME=bus/usb/002/008
DEVTYPE=usb_device
PRODUCT=2bdf/1/100
TYPE=239/2/1
BUSNUM=002
DEVNUM=008
SEQNUM=6372



add@/devices/3610000.xhci/usb2/2-3/2-3.1
ACTION=add
DEVPATH=/devices/3610000.xhci/usb2/2-3/2-3.1
SUBSYSTEM=usb
MAJOR=189
MINOR=136
DEVNAME=bus/usb/002/009
DEVTYPE=usb_device
PRODUCT=2bdf/1/100
TYPE=239/2/1
BUSNUM=002
DEVNUM=009
SEQNUM=6373

add@/devices/3610000.xhci/usb2/2-3/2-3.1/2-3.1:1.0
ACTION=add
DEVPATH=/devices/3610000.xhci/usb2/2-3/2-3.1/2-3.1:1.0
SUBSYSTEM=usb
DEVTYPE=usb_interface
PRODUCT=2bdf/1/100
TYPE=239/2/1
INTERFACE=239/5/0
MODALIAS=usb:v2BDFp0001d0100dcEFdsc02dp01icEFisc05ip00in00
SEQNUM=6374

add@/devices/3610000.xhci/usb2/2-3/2-3.1/2-3.1:1.1
ACTION=add
DEVPATH=/devices/3610000.xhci/usb2/2-3/2-3.1/2-3.1:1.1
SUBSYSTEM=usb
DEVTYPE=usb_interface
PRODUCT=2bdf/1/100
TYPE=239/2/1
INTERFACE=239/5/1
MODALIAS=usb:v2BDFp0001d0100dcEFdsc02dp01icEFisc05ip01in01
SEQNUM=6375

add@/devices/3610000.xhci/usb2/2-3/2-3.1/2-3.1:1.2
ACTION=add
DEVPATH=/devices/3610000.xhci/usb2/2-3/2-3.1/2-3.1:1.2
SUBSYSTEM=usb
DEVTYPE=usb_interface
PRODUCT=2bdf/1/100
TYPE=239/2/1
INTERFACE=239/5/2
MODALIAS=usb:v2BDFp0001d0100dcEFdsc02dp01icEFisc05ip02in02
SEQNUM=6376

*/

static void parse_event (const char *kmsg)
{
	struct kmsg_uevent_t kuevent;
	kuevent.action = "";
	kuevent.devpath = "";
	kuevent.subsystem = "";
	kuevent.devtype = "";
	kuevent.major = -1;
	kuevent.minor = -1;

	while (*kmsg) {
		log_debug("%s", kmsg);
		if (!strncmp(kmsg, "ACTION=", 7)) {
			kmsg += 7;
			kuevent.action = kmsg;
		} else if (!strncmp(kmsg, "DEVPATH=", 8)) {
			kmsg += 8;
			kuevent.devpath = kmsg;
		} else if (!strncmp(kmsg, "SUBSYSTEM=", 10)) {
			kmsg += 10;
			kuevent.subsystem = kmsg;
		} else if (!strncmp(kmsg, "DEVTYPE=", 8)) {
			kmsg += 8;
			kuevent.devtype = kmsg;
		} else if (!strncmp(kmsg, "MAJOR=", 6)) {
			kmsg += 6;
			kuevent.major = atoi(kmsg);
		} else if (!strncmp(kmsg, "MINOR=", 6)) {
			kmsg += 6;
			kuevent.minor = atoi(kmsg);
		}

		while (*kmsg++) ; // 以 \n 分割字段
	}

	if (strncasecmp(kuevent.devtype, "usb_device", 10) == 0) {
		if ((strncasecmp(kuevent.action, "add", 3) == 0)
		  ||(strncasecmp(kuevent.action, "remove", 6) == 0)) {
			// todo MV_CC_EnumDevices
			pthread_cond_broadcast(&gCSV->mvs.cond_mvs);
		}
	}

}

int csv_uevent_trigger (struct csv_uevent_t *pUE)
{
	int rcvlen = 0;
	char kmsg[LEN_UEVENT_MSG+2] = {0};

	while ((rcvlen = recv(pUE->fd, kmsg, LEN_UEVENT_MSG, 0)) > 0) {
		if (rcvlen == LEN_UEVENT_MSG) { // overflow. omit
			continue;
		}

		kmsg[rcvlen] = '\0';
		kmsg[rcvlen+1] = '\0';

		parse_event(kmsg);
	}

	return 0;
}

static int csv_uevent_open (struct csv_uevent_t *pUE)
{
	struct sockaddr_nl nl_addr;
	int ret = -1;
	int sz = 64*1024;
	int fd = -1;

	memset(&nl_addr, 0, sizeof(nl_addr));
	nl_addr.nl_family = AF_NETLINK;
	nl_addr.nl_pid = getpid();
	nl_addr.nl_groups = 1;

	fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (fd < 0) {
		log_err("ERROR : socket uevent");
		return -1;
	}

	ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz));
	if (ret < 0) {
		log_err("ERROR : setsockopt");
		close(fd);
		return -1;
	}

	ret = bind(fd, (struct sockaddr *)&nl_addr, sizeof(struct sockaddr_nl));
	if (ret < 0) {
		log_err("ERROR : bind");
		close(fd);
		return -1;
	}

	pUE->fd = fd;

	log_info("OK : bind %s as fd(%d).",  pUE->name, pUE->fd);

	return 0;
}

int csv_uevent_init (void)
{
	struct csv_uevent_t *pUE = &gCSV->uevent;

	pUE->name = NAME_UEVENT;
	pUE->fd = -1;

	return csv_uevent_open(pUE);
}

int csv_uevent_deinit (void)
{
	struct csv_uevent_t *pUE = &gCSV->uevent;

	if (pUE->fd > 0) {
		close(pUE->fd);
		pUE->fd = -1;
	}

	return 0;
}


#ifdef __cplusplus
}
#endif



