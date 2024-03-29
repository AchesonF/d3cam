#ifndef __INC_FILES_H__
#define __INC_FILES_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <malloc.h>
#include <math.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <linux/netlink.h>

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <sys/wait.h>

#include "project_version.h"
#include "config.h"

// for xml2
#include "libxml/parser.h"
#include "libxml/tree.h"


// for MVS
#include "MvCameraControl.h"

// for GigE Vision
#include "GigEVisionGVCPDefine.h"
#include "GigEVisionGVCPRegisterDefine.h"
#include "GigEVisionGVSPDefine.h"

#include "list.h"
#include "queue.h"
#include "crc.h"

#include "csv_utility.h"
#include "csv_type.h"
#include "csv_udp.h"
#include "csv_hb.h"
#include "csv_file.h"
#include "csv_xml.h"
#include "csv_eth.h"
#include "csv_gpi.h"
#include "csv_life.h"
#include "csv_stat.h"
#include "csv_beat.h"
#include "csv_tcp.h"
#include "csv_tick.h"
#include "csv_tty.h"
#include "csv_uevent.h"
#include "csv_msg.h"
#include "csv_dlp.h"
#include "csv_web.h"
#include "csv_png.h"
#include "csv_mat.h"
#include "csv_mvs.h"
#include "csv_gev.h"
#include "csv_cfg.h"
#include "csv_main.h"

#include "log.h"

#ifdef __cplusplus
}
#endif


#endif /* __INC_FILES_H__ */




