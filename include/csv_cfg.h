#ifndef __CSV_CFG_H__
#define __CSV_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_GVSP_PACKETSIZE			(1500)//(8164)

enum {
	CAM_LEFT				= (0),
	CAM_RIGHT				= (1),
//	CAM_FRONT				= (2),
//	CAM_BACK				= (3),

	TOTAL_CAMS
};

// 存储图片后缀
enum {
	SUFFIX_BMP		= 0,
	SUFFIX_PNG		= 1,

	END_SUFFIX
};

struct dlp_cfg_t {
	char				name[64];
	float				rate;
	float				brightness;
	float				expoTime;
};

struct device_cfg_t {
	int					DeviceType;				///< 设备类型 0:2cams
	uint8_t				SwitchCams;				///< 左右相机互换 1:互换
	uint8_t				CamImageType;			///< 图像类型 0:灰度图 1:RGB图
	uint8_t				SaveImageFile;			///< 保存图像
	uint8_t				SaveImageFormat;		///< 图像存储格式 SUFFIX_BMP
	uint8_t				flip_left;				///< 左机 上下翻转
	uint8_t				flip_right;				///< 右机 上下翻转
	char				*strSuffix;

	struct dlp_cfg_t	dlpcfg[TOTAL_DLP_CMDS];
};

struct depthimg_cfg_t {
	int					NumDisparities;
	int					BlockSize;
	int					UniqRatio;
};

struct channel_cfg_t {
	uint32_t				Address;
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
	uint32_t				MessageTimeout;
	uint32_t				MessageRetryCount;
	uint32_t				MessageSourcePort;

	struct channel_cfg_t	Channel;
};

struct pointcloud_cfg_t {
	char					ImageSaveRoot[128];	///< 图片存放路径
	char					calibFile[128];		///< 标定参数文件名
	char					outFileXYZ[256];	///< 生成文件名
	char					outDepthImage[256];	///< 生成深度图png
	uint8_t					saveXYZ;			///< 保存点云文件
	uint8_t					saveDepthImage;		///< 保存深度图
	uint8_t					groupPointCloud;	///< 点云次数
	uint8_t					enable;				///< 使能计算
	uint8_t					initialized;		///< 已初始化。执行一次即可

	uint8_t					test_bmp;
};

// 标定
struct calib_conf_t {
	char					path[128];		///< 图片相对路径
	uint8_t					groupDemarcate;	///< 标定次数, need to save for next boot

};

struct csv_cfg_t {
	struct device_cfg_t		devicecfg;
	struct depthimg_cfg_t	depthimgcfg;
	struct pointcloud_cfg_t	pointcloudcfg;

	struct calib_conf_t		calibcfg;
	struct gev_conf_t		gigecfg;			///< gev参数
};


extern int csv_cfg_init (void);

extern int csv_cfg_deinit (void);

#ifdef __cplusplus
}
#endif

#endif /* __CSV_CFG_H__ */

