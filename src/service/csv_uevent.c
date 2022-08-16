#include "inc_files.h"

#ifdef __cplusplus
extern "C" {
#endif


static void parse_event (const char *kmsg)
{
	struct kmsg_uevent_t kuevent;
	kuevent.action = "";
	kuevent.devpath = "";
	kuevent.subsystem = "";
	kuevent.devname = "";
	kuevent.devtype = "";
	kuevent.product = "";
	kuevent.major = -1;
	kuevent.minor = -1;

	while (*kmsg) {
//		log_debug("%s", kmsg);
		if (!strncmp(kmsg, "ACTION=", 7)) {
			kmsg += 7;
			kuevent.action = kmsg;
		} else if (!strncmp(kmsg, "DEVPATH=", 8)) {
			kmsg += 8;
			kuevent.devpath = kmsg;
		} else if (!strncmp(kmsg, "SUBSYSTEM=", 10)) {
			kmsg += 10;
			kuevent.subsystem = kmsg;
		} else if (!strncmp(kmsg, "DEVNAME=", 8)) {
			kmsg += 8;
			kuevent.subsystem = kmsg;
		} else if (!strncmp(kmsg, "DEVTYPE=", 8)) {
			kmsg += 8;
			kuevent.devtype = kmsg;
		} else if (!strncmp(kmsg, "PRODUCT=", 8)) {
			kmsg += 8;
			kuevent.product = kmsg;
		} else if (!strncmp(kmsg, "MAJOR=", 6)) {
			kmsg += 6;
			kuevent.major = atoi(kmsg);
		} else if (!strncmp(kmsg, "MINOR=", 6)) {
			kmsg += 6;
			kuevent.minor = atoi(kmsg);
		}

		while (*kmsg++) ; // 以 \n 分割字段
	}

	log_debug("%s : %s %s %s", kuevent.action, kuevent.subsystem, 
		kuevent.devname, kuevent.devtype, kuevent.product);
	if ((strncasecmp(kuevent.devtype, "usb_device", 10) == 0)
		&&(strncasecmp(kuevent.product, "2bdf", 4) == 0)) { // hik
		if ((strncasecmp(kuevent.action, "add", 3) == 0)
		  ||(strncasecmp(kuevent.action, "remove", 6) == 0)) {

// hik plug in/out event

			pthread_cond_broadcast(&gCSV->mvs.cond_mvs);
		}
	}

}

int csv_uevent_trigger (struct csv_uevent_t *pUE)
{
	int rcvlen = 0;
	char kmsg[LEN_UEVENT_MSG+2] = {0};
	int timeo = 0;

	while ((rcvlen = recv(pUE->fd, kmsg, LEN_UEVENT_MSG, 0)) > 0) {
		if (++timeo >= 200) {
			log_info("WARN : too many event flush. Something go wrong.");
			csv_uevent_deinit();
			csv_uevent_init();
			break;
		}

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
		log_err("ERROR : socket %s.", pUE->name);
		return -1;
	}

	ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz));
	if (ret < 0) {
		log_err("ERROR : setsockopt %s.", pUE->name);
		close(fd);
		return -1;
	}

	ret = bind(fd, (struct sockaddr *)&nl_addr, sizeof(struct sockaddr_nl));
	if (ret < 0) {
		log_err("ERROR : bind %s.", pUE->name);
		close(fd);
		return -1;
	}

	int err = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (err < 0) {
		log_err("ERROR : fcntl %s.", pUE->name);
		return err;
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
		log_info("OK : close %s.", pUE->name);
		close(pUE->fd);
		pUE->fd = -1;
	}

	return 0;
}


#ifdef __cplusplus
}
#endif



