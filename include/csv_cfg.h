#ifndef __CSV_CFG_H__
#define __CSV_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_GVSP_PACKETSIZE			(1500)//(8164)

#define TOTAL_CAMS				(2)		///< 实际相机总数
#define CAM_LEFT				(0)		///< left cam  -> 流通道0
#define CAM_RIGHT				(1)		///< right cam	-> 流通道1
#define CAM_DEPTH				(2)		///< 虚拟相机 传输深度图 流通道2

#define TOTAL_CHANNELS			(3)		///< 总流通道数

// 存储图片后缀
enum {
	SUFFIX_BMP		= 0,
	SUFFIX_PNG		= 1,
	SUFFIX_JPEG		= 2,

	END_SUFFIX
};

struct dlp_conf_t {
	char				name[64];
	float				rate;
	float				brightness;
	float				expoTime;
};

struct device_conf_t {
	uint8_t				SwitchCams;				///< 左右相机互换 1:互换
	uint8_t				SaveImageFile;			///< 保存图像
	uint8_t				SaveImageFormat;		///< 图像存储格式 SUFFIX_BMP
	uint8_t				ftpupload;				///< 通过ftp送出
	uint8_t				exportCamsCfg;			///< 导出相机配置
	char				*strSuffix;

	struct dlp_conf_t	dlpcfg[TOTAL_DLP_CMDS];
};

struct channel_conf_t {
	uint32_t				Address;
	uint16_t				idx;
	uint16_t				Port;
	uint32_t				Cfg_PacketSize;
	uint32_t				PacketDelay;		// ns
	uint32_t				Configuration;
	uint32_t				SourcePort;
	uint32_t				Capability;
	uint32_t				Zone;
	uint32_t				ZoneDirection;
};

/* GigE Version regiseter's parameters */
struct gev_conf_t {
	uint16_t				VersionMajor;
	uint16_t				VersionMinor;
	uint32_t				DeviceMode;	// 
	uint32_t				MacHi;		// mac[0-1]
	uint32_t				MacLow;		// mac[2-5]

	uint32_t				IfCapability0; // PR[0]PG[1]...L[29]D[30]P[31]
	uint32_t				IfConfiguration0; // PR[0]PG[1]...L[29]D[30]P[31]
	uint32_t				CurrentIPAddress0; // ipv4
	uint32_t				CurrentSubnetMask0;
	uint32_t				CurrentDefaultGateway0;

	char					ManufacturerName[32];
	char					ModelName[32];
	char					DeviceVersion[32];
	char					ManufacturerInfo[48];
	char					SerialNumber[16];
	char					UserdefinedName[16];
	char					FirstURL[GVCP_URL_MAX_LEN];
	char					SecondURL[GVCP_URL_MAX_LEN];
	char					*strXmlfile;
	uint8_t					*xmlData;
	uint32_t				xmlLength;

	uint32_t				NumberofNetworkInterfaces;
	uint32_t				LinkSpeed0;

	uint32_t				NumberofMessageChannels;
	uint32_t				NumberofStreamChannels;
	uint32_t				NumberofActionSignals;
	uint32_t				ActionDeviceKey;
	uint32_t				NumberofActiveLinks; // [28-31]
	uint32_t				GVSPCapability; // SP[0]LB[1]...
	uint32_t				MessageCapability; // SP[0]...
	uint32_t				GVCPCapability;
	uint32_t				TimestampTickFrequencyHigh;
	uint32_t				TimestampTickFrequencyLow;
	uint32_t				HeartbeatTimeout;
	uint32_t				TimestampControl;
	uint32_t				TimestampValueHigh;
	uint32_t				TimestampValueLow;
	uint32_t				DiscoveryACKDelay;
	uint32_t				GVCPConfiguration;
	uint32_t				PendingTimeout;
	uint32_t				ControlSwitchoverKey;
	uint32_t				GVSPConfiguration;
	uint32_t				PhysicalLinkConfigurationCapability;
	uint32_t				PhysicalLinkConfiguration;
	uint32_t				IEEE1588Status;
	uint32_t				ScheduledActionCommandQueueSize;
	uint32_t				ControlChannelPrivilege;

	uint32_t				PrimaryAddress;
	uint16_t				PrimaryPort;

	uint32_t				MessageAddress;
	uint32_t				MessagePort;
	uint32_t				MessageTimeout;			// ms
	uint32_t				MessageRetryCount;
	uint32_t				MessageSourcePort;

	struct channel_conf_t	Channel[TOTAL_CHANNELS];
};

struct pointcloud_conf_t {
	char					ModelRoot[128];		///< 模型存放路径
	char					PCImageRoot[128];	///< 图片存放路径
	char					calibFile[128];		///< 标定参数文件名
	char					outFileXYZ[256];	///< 生成文件名
	char					outDepthImage[256];	///< 生成深度图png
	uint8_t					saveXYZ;			///< 保存点云文件
	uint8_t					groupPointCloud;	///< 点云次数 1..255
	uint8_t					initialized;		///< 已初始化。执行一次即可
};

// 标定
struct calib_conf_t {
	char					CalibImageRoot[128];///< 标定图片路径
	uint8_t					groupCalibrate;	///< 标定次数, 1..255 need to save for next boot

};

struct hdri_conf_t {
	char					HdrImageRoot[128];		///< hdri路径
	uint8_t					groupHdri;
};

struct csv_cfg_t {
	struct device_conf_t		devicecfg;
	struct pointcloud_conf_t	pointcloudcfg;

	struct hdri_conf_t			hdricfg;
	struct calib_conf_t			calibcfg;
	struct gev_conf_t			gigecfg;			///< gev参数
};


extern int csv_cfg_init (void);

extern int csv_cfg_deinit (void);

#ifdef __cplusplus
}
#endif

#endif /* __CSV_CFG_H__ */

