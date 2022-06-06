#ifndef __CSV_GVCP_H__
#define __CSV_GVCP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define NAME_UDP_GVCP		"'udp_gvcp'"

#define PORT_UDP_GVCP		(3956)

#define SIZE_UDP_BUFF		(4096)


#define GVCP_DISCOVERY_CMD	0x02
#define GVCP_DISCOVERY_ACK	0x03
#define GVCP_FORCEIP_CMD	0x04
#define GVCP_FORCEIP_ACK	0x05
#define GVCP_READREG_CMD	0x80
#define GVCP_READREG_ACK	0x81
#define GVCP_WRITEREG_CMD	0x82
#define GVCP_WRITEREG_ACK	0x83
#define GVCP_READMEM_CMD	0x84
#define GVCP_READMEM_ACK	0x85

struct gvcp_cmd_header_t {
	uint8_t			cMsgKeyCode;		// 0x42
	uint8_t			cFlag;				// 0x11 allow broadcast ack;ack required
	uint16_t		wCmd;				// DISCOVER=0x02;FORCEIP=0x04;READREG=0x80
	uint16_t		wLen;				// payload length
	uint16_t		wReqID;				// request id = 1;READREG id=12345
};

struct gvcp_forceip_payload_t {
	uint8_t			Mac[8];				// last 6 byte
	uint8_t			CurIP[16];			// last 4 byte
	uint8_t			SubMask[16];		// last 4 byte
	uint8_t			Gateway[16];		// last 4 byte
};

struct gvcp_ack_header_t {
	uint16_t		wStatus;			// success=0;
	uint16_t		wAck;				// discover_ack=3;forceip_ack=5;READREG_ACK=0x81
	uint16_t		wLen;
	uint16_t		wReqID;
};

struct gvcp_ack_payload_t {
	uint32_t		dwSpecVer;
	uint32_t		dwDevMode;
	uint8_t			Mac[8];				// last 6 byte
	uint32_t		dwSupIpSet;
	uint32_t		dwCurIpSet;
	//uint8_t		unused1[12];
	uint8_t			CurIP[16];			// last 4 byte
	uint8_t			SubMask[16];		// last 4 byte
	uint8_t			Gateway[16];		// last 4 byte
	char 			szFacName[32];		// first
	char 			szModelName[32];	// first
	char 			szDevVer[32];
	char 			szFacInfo[48];
	char 			szSerial[16];
	char 			szUserName[16];
};

struct gvcp_discover_cmd_t {
	struct gvcp_cmd_header_t	header;
};
struct gvcp_discover_ack{
	struct gvcp_ack_header_t	header;
	struct gvcp_ack_payload_t	payload;
};
struct gvcp_forceip_cmd{
	struct gvcp_cmd_header_t	header;
	struct gvcp_forceip_payload_t payload;
};
struct gvcp_forceip_ack{
	struct gvcp_ack_header_t	header;
};
struct gvcp_readreg_cmd{
	struct gvcp_cmd_header_t	header;
	uint32_t					dwRegAddr;
};
struct gvcp_readreg_ack{
	struct gvcp_ack_header_t header;
	uint32_t					dwRegValue;
};
struct gvcp_writereg_cmd {
	struct gvcp_cmd_header_t header;
	uint32_t					dwRegAddr;
	uint32_t					dwRegValue;
};

struct gvcp_writereg_ack {
	struct gvcp_ack_header_t header;
};

struct gvcp_readmem_cmd {
	struct gvcp_cmd_header_t header;
	uint32_t				dwMemAddr;
	uint32_t				dwMemCount;	// last 2 byte
};
struct gvcp_readmem_ack {
	struct gvcp_ack_header_t header;
	uint32_t				dwMemAddr;
	char					*pMemBuf;
};


struct csv_gvcp_t {
	int				fd;			///< 文件描述符
	char			*name;		///< 链接名称
	uint16_t		port;		///< udp 监听端口

	struct {
		uint32_t	nRx;
		uint8_t		pRx[SIZE_UDP_BUFF];
	} Recv;

	struct {
		uint32_t	nTx;
		uint8_t		pTx[SIZE_UDP_BUFF];
	} Send;

	struct sockaddr_in	from_addr;		///< 来源地址参数
};


extern int csv_gvcp_trigger (struct csv_gvcp_t *pGVCP);

extern int csv_gvcp_init (void);

extern int csv_gvcp_deinit (void);



#ifdef __cplusplus
}
#endif


#endif /* __CSV_GVCP_H__ */


