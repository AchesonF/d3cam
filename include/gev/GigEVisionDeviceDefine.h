#ifndef __GIGEVISION_DEVICE_H__
#define __GIGEVISION_DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

//#include "GiGEVisionPixelFormatDefine.h" // == "../mvs/PixelType.h"


// 设备类型定义
#define DEVICE_UNKNOW			(0x00000000)	// 未知设备类型，保留意义
#define DEVICE_GIGE				(0x00000001)	// GigE设备
#define DEVICE_1394				(0x00000002)	// 1394-a/b 设备
#define DEVICE_USB3				(0x00000004)	// USB3.0 设备
#define DEVICE_CAMERALINK		(0x00000008)	// CameraLink设备

typedef struct _GIGE_DEVICE_INFO_ {
    uint32_t			nIpCfgOption;
    uint32_t			nIpCfgCurrent;                   // IP configuration:bit31-static bit30-dhcp bit29-lla
    uint32_t			nCurrentIp;                      // curtent ip
    uint32_t			nCurrentSubNetMask;              // curtent subnet mask
    uint32_t			nDefultGateWay;                  // current gateway
    uint8_t				chManufacturerName[32];
    uint8_t				chModelName[32];
    uint8_t				chDeviceVersion[32];
    uint8_t				chManufacturerSpecificInfo[48];
    uint8_t				chSerialNumber[16];
    uint8_t				chUserDefinedName[16];

    uint32_t			nNetExport;                      // 网口IP地址
} GIGE_DEVICE_INFO;

// 设备信息
typedef struct _CC_DEVICE_INFO_ {
    // common info
    uint16_t			nMajorVer;
    uint16_t			nMinorVer;
    uint32_t			nDeviceMode;
    uint32_t			nMacAddrHigh;               // MAC 地址
    uint32_t			nMacAddrLow;

    uint32_t			nTLayerType;                // 设备传输层协议类型，e.g. MV_GIGE_DEVICE

    union
    {
        GIGE_DEVICE_INFO stGigEInfo;
        // more ...
    };

} CC_DEVICE_INFO;

// 网络传输的相关信息
typedef struct _NETTRANS_INFO_ {
    long long       nReviceDataSize;    // 已接收数据大小  [统计StartGrabbing和StopGrabbing之间的数据量]
    int				nThrowFrameCount;   // 丢帧数量
    int				nReserved;

} NETTRANS_INFO;


// 最多支持的传输层实例个数
#define MAX_TLS_NUM          8
// 最大支持的设备个数
#define MAX_DEVICE_NUM      256

// 设备信息列表
typedef struct _CC_DEVICE_INFO_LIST_ {
    uint32_t			nDeviceNum;                         // 在线设备数量
    CC_DEVICE_INFO*      pDeviceInfo[MAX_DEVICE_NUM];     // 支持最多256个设备

} CC_DEVICE_INFO_LIST;


// 输出帧的信息
typedef struct _FRAME_OUT_INFO_ {
    uint16_t			nWidth;             // 图像宽
    uint16_t			nHeight;            // 图像高
    uint32_t			enPixelType;        // 像素格式

    /*以下字段暂不支持*/
    uint32_t			nFrameNum;          // 帧号
    uint32_t			nDevTimeStampHigh;  // 时间戳高32位
    uint32_t			nDevTimeStampLow;   // 时间戳低32位
    long long           nHostTimeStamp;     // 主机生成的时间戳

    uint32_t			nReserved[4];       // 保留
} FRAME_OUT_INFO;


// 采集模式
typedef enum _CAM_ACQUISITION_MODE_ {
	ACQ_MODE_SINGLE			= (0),			// 单帧模式
	ACQ_MODE_MUTLI			= (1),			// 多帧模式
	ACQ_MODE_CONTINUOUS		= (2),			// 持续采集模式

} CAM_ACQUISITION_MODE;

// GigEVision IP Configuration
#define IP_CFG_STATIC				(0x01000000)
#define IP_CFG_DHCP					(0x02000000)
#define IP_CFG_LLA					(0x04000000)


// 信息类型
#define MATCH_TYPE_NET_DETECT		(0x00000001)	// 网络流量和丢包信息

// 全匹配的一种信息结构体
typedef struct _ALL_MATCH_INFO_ {
    uint32_t			nType;          // 需要输出的信息类型，e.g. MATCH_TYPE_NET_DETECT
    void*           	pInfo;          // 输出的信息缓存，由调用者分配
    uint32_t			nInfoSize;      // 信息缓存的大小

} ALL_MATCH_INFO;

//  网络流量和丢包信息反馈结构体，对应类型为 MATCH_TYPE_NET_DETECT
typedef struct _MATCH_INFO_NET_DETECT_ {
    long long			nReviceDataSize;    // 已接收数据大小  [统计StartGrabbing和StopGrabbing之间的数据量]
    long long       	nLostPacketCount;   // 丢失的包数量
    uint32_t			nLostFrameCount;    // 丢帧数量
    uint32_t			nReserved;          // 保留
} MATCH_INFO_NET_DETECT;

//  异常消息类型
#define EXCEPTION_DEV_DISCONNECT	(0x00008001)	// 设备断开连接


// 打开设备的权限模式
#define ExclusivePrivilege									(1)
#define ExclusivePrivilegeWithSwitchoverKey					(2)
#define ControlPrivilege									(3)
#define ControlPrivilegeWithSwitchoverKey					(4)
#define ControlSwitchoverEnablePrivilege					(5)
#define ControlSwitchoverEnablePrivilegeWithSwitchoverKey	(6)
#define MonitorPrivilege									(7)

#ifdef __cplusplus
}
#endif

#endif /* __GIGEVISION_DEVICE_H__ */


