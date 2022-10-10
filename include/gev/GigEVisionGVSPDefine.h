#ifndef __GIGEVISION_GVSP_H__
#define __GIGEVISION_GVSP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define GIGEVISION_MAJOR_VERSION		GEV_VERSION_MAJOR	//(2)

#pragma pack(push, 1)

#if 0//(GIGEVISION_MAJOR_VERSION == 1)

/////////////////////////////////////////////////////GVSP Define ////////////////////////////////////////////
/////////////////////////////////////// GVSP common packet header define ////////////////////////////////////

/*                                All GVSP packets share the same basic header.
 *  0 bit                    7|8                      15|16                     23|24                     31|
 *  +------------+------------+------------+------------+------------+------------+------------+------------+
 *  |                      status                       |                      block_id                     |
 *  ---------------------------------------------------------------------------------------------------------
 *  |        packet_fmt       |                                    packet_id                                |
 *  ---------------------------------------------------------------------------------------------------------
 *  |                                           ......                                                      |
 *  +------------+------------+------------+------------+------------+------------+------------+------------+
 */
typedef struct _GVSP_PACKET_HEADER_ {
    uint16_t			status;						// 16bits，状态码
    uint16_t			block_id;					// 16bits

    union {
        struct   {
    		uint32_t		packet_fmt:8;			// 8bits, 参考 GVSP的包类型定义
   			uint32_t		packet_id:24;			// 24bits
        };
    	uint32_t		packet_fmt_id;
    };

} GVSP_PACKET_HEADER;


#define GVSP_PACKET_ID_MAX					(0x00FFFFFF)

// GVSP的包类型，目前定义6个值，其他保留
// For Standard Transmission Mode:
#define GVSP_PACKET_FMT_LEADER				(0x01)
#define GVSP_PACKET_FMT_TRAILER				(0x02)
#define GVSP_PACKET_FMT_PAYLOAD_GENERIC		(0x03)
// For All-in Transmission Mode:
#define GVSP_PACKET_FMT_ALL_IN				(0x04)


// GVSP 包的负载类型
typedef enum _GVSP_PACKET_PAYLOAD_TYPE_ {
    GVSP_PT_UNCOMPRESSED_IMAGE          = (0x0001),
    GVSP_PT_RAW_DATA                    = (0x0002),
    GVSP_PT_FILE                        = (0x0003),
    GVSP_PT_CHUNK_DATA                  = (0x0004),
    GVSP_PT_CHUNK_DATA_EX               = (0x0005),

    // [0x8000, 0xffff]自定义

} GVSP_PACKET_PAYLOAD_TYPE;


//////////////////////////// Data Leader define ////////////////////////////

// additional info for Image Data Leader Packet
typedef struct _GVSP_IMAGE_DATA_LEADER_ {
    uint16_t			reserved;           // 置0
    uint16_t			payload_type;       // 0x0001
    uint32_t			timestamp_high;
    uint32_t			timestamp_low;
    uint32_t			pixel_format;
    uint32_t			size_x;
    uint32_t			size_y;
    uint32_t			offset_x;
    uint32_t			offset_y;
    uint16_t			padding_x;
    uint16_t			padding_y;
} GVSP_IMAGE_DATA_LEADER;

//////////////////////////// Data Payload define ////////////////////////////


//////////////////////////// Data Trailer define ////////////////////////////

// additional info for Image Data Trailer Packet
typedef struct _GVSP_IMAGE_DATA_TRAILER_ {
    uint16_t			reserved;           // 置0
    uint16_t			payload_type;       // 0x0001
    uint32_t			size_y;             // 32bit
} GVSP_IMAGE_DATA_TRAILER;

#endif  // (GIGEVISION_MAJOR_VERSION == 1)



#if (GIGEVISION_MAJOR_VERSION == 2)
/*                                All GVSP packets share the same basic header.
 *  0 bit                    7|8                      15|16                     23|24                     31|
 *  +------------+------------+------------+------------+------------+------------+------------+------------+
 *  |                      status                       |                   block_id / flag                 |
 *  ---------------------------------------------------------------------------------------------------------
 *  |EI|reserved | packet_fmt |                               packet_id / reserved                          |
 *  ---------------------------------------------------------------------------------------------------------
 *  |                                           block_id64 (high part)                                      |
 *  ---------------------------------------------------------------------------------------------------------
 *  |                                           block_id64 (low part)                                       |
 *  ---------------------------------------------------------------------------------------------------------
 *  |                                           packet_id32                                                 |
 *  ---------------------------------------------------------------------------------------------------------
 *  |                                           ......                                                      |
 *  +------------+------------+------------+------------+------------+------------+------------+------------+
 */

// typedef struct gvsp_packet_header_t {
//     unsigned short  status;                     // 16bits，状态码

//     /*
//      * 字段意义由EI位决定。
//      * 1. 当表示block_id时，block_id = 0保留作为测试包；
//      * 2. 当表示flag时，高8bit(0~7) device-specific;
//      * bit (8~12) 保留，置0;
//      * bit 13, 置1表示 GEV_FLAG_RESEND_RANGE_ERROR
//      * bit 14, 置1表示 GEV_FLAG_PREVIOUS_BLOCK_DROPPED
//      * bit 15, 置1表示 GEV_FLAG_PACKET_RESEND
//      */
//     unsigned short  block_id_or_flag;           // 16bits

//     /*
//      * EI flag = 0 → block_id，表示 block_id 使用16bits，packet_id 使用24bits；无扩展字段
//      * EI flag = 1 → flag，表示 block_id 使用64bits， packet_id 使用32bits。包含扩展字段
//      */
//     unsigned char   ei;                         // 1bit
//     unsigned char   reserved;                   // 3bits，置0
//     unsigned char   packet_format;              // 4bits, 参考 GVSP的包类型定义

//     /*
//      * EI flag = 0 → packet_id，该block中的数据包packet_id。每个block开始时重置里面的packet_id号
//      * EI flag = 1 → reserved，置0
//      */
//     unsigned int    packet_id_or_reserved;      // 24bits

//     // 以下字段仅当 EI flag = 1 时有效
//     unsigned int    block_id_high;              // 32bits
//     unsigned int    block_id_low;               // 32bits
//     unsigned int    packet_id32;                // 32bits

// } GVSP_PACKET_HEADER;

typedef struct gvsp_packet_header_t {
    uint16_t			status;						// 16bits，状态码
    uint16_t			blockid_flag;				// 16bits
	uint32_t			packet_format;				// [0]EI [1-3]reserved[4-7]format[8-30]reserved
    uint32_t			block_id_high;              // 32bits
    uint32_t			block_id_low;               // 32bits
    uint32_t			packet_id;                  // 32bits
} GVSP_PACKET_HEADER;

#define GVSP_EI0_PACKET_SIZE				(8)		// GVSP基本包头，当EI位置0时的长度
#define GVSP_EI1_PACKET_SIZE				(20)	// GVSP基本包头，当EI位置1时的长度


// GVSP的包类型，目前定义6个值，其他保留
// For Standard Transmission Mode:
#define GVSP_PACKET_FMT_LEADER				(0x01)
#define GVSP_PACKET_FMT_TRAILER				(0x02)
#define GVSP_PACKET_FMT_PAYLOAD_GENERIC		(0x03)
#define GVSP_PACKET_FMT_PAYLOAD_H264		(0x05)
#define GVSP_PACKET_FMT_PAYLOAD_IMAGE		(0x06)
// For All-in Transmission Mode:
#define GVSP_PACKET_FMT_ALL_IN				(0x04)


// GVSP 包的负载类型
typedef enum _GVSP_PACKET_PAYLOAD_TYPE_ {
    GVSP_PT_UNCOMPRESSED_IMAGE          = (0x0001),
    GVSP_PT_RAW_DATA                    = (0x0002),
    GVSP_PT_FILE                        = (0x0003),
    GVSP_PT_CHUNK_DATA                  = (0x0004),
    GVSP_PT_CHUNK_DATA_EX               = (0x0005),
    GVSP_PT_JPEG                        = (0x0006),   // ITU-T Rec. T.81 定义的jpeg格式
    GVSP_PT_JPEG2000                    = (0x0007),   // ITU-T Rec. T.800 定义的jpeg格式
    GVSP_PT_H264                        = (0x0008),   // ITU-T Rec. H.264
    GVSP_PT_MULTI_IMAGE                 = (0x0009),

    // bit1 为1， 表示 扩展定义
    GVSP_PT_UNCOMPRESSED_IMAGE_EX       = (0x4001),
    GVSP_PT_RAW_DATA_EX                 = (0x4002),
    GVSP_PT_FILE_EX                     = (0x4003),
    GVSP_PT_JPEG_EX                     = (0x4006),
    GVSP_PT_JPEG2000_EX                 = (0x4007),
    GVSP_PT_H264_EX                     = (0x4008),
    GVSP_PT_MULTI_IMAGE_EX              = (0x4009),

    // [0x8000, 0xffff]自定义
} GVSP_PACKET_PAYLOAD_TYPE;

// additional info for Image Data Leader Packet
typedef struct ImageDataLeader_t {
    struct {
    	uint8_t			id:4;
    	uint8_t			count:4;
    } field;     // 场标识。如果两场，显示或存储时，需要按照 0-0,1-0;0-1,1-1...的顺序

    uint8_t				reserved;           // 置0
    uint16_t			payload_type;       // 0x0001 or 0x4001
    uint32_t			timestamp_high;
    uint32_t			timestamp_low;
    uint32_t			pixel_format;
    uint32_t			size_x;
    uint32_t			size_y;
    uint32_t			offset_x;
    uint32_t			offset_y;
    uint16_t			padding_x;
    uint16_t			padding_y;
} GVSP_IMAGE_DATA_LEADER;

typedef struct ImageDataTrailer_t {
    uint16_t			reserved;
    uint16_t			payload_type; // 0x0001 or 0x4001
    uint32_t			size_y;
} GVSP_IMAGE_DATA_TRAILER;


// additional info for raw Data Leader Packet
typedef struct RawDataLeader_t {
    uint16_t			reserved;           // 置0
    uint16_t			payload_type;       // 0x0002 or 0x4002
    uint32_t			timestamp_high;
    uint32_t			timestamp_low;
    uint32_t			payload_size_high;
    uint32_t			payload_size_low;
} GVSP_RAW_DATA_LEADER;

typedef struct RawDataTrailer_t {
    uint16_t			reserved;
    uint16_t			payload_type; // 0x0002 or 0x4002
} GVSP_RAW_DATA_TRAILER;

#define MAX_SIZE_FILENAME			(64)

// additional info for file Data Leader Packet
typedef struct FileDataLeader_t {
    uint16_t			reserved;           // 置0
    uint16_t			payload_type;       // 0x0003 or 0x4003
    uint32_t			timestamp_high;
    uint32_t			timestamp_low;
    uint32_t			payload_size_high;
    uint32_t			payload_size_low;
	char				filename[MAX_SIZE_FILENAME];
} GVSP_FILE_DATA_LEADER;

typedef struct FileDataTrailer_t {
    uint16_t			reserved;
    uint16_t			payload_type; // 0x0003 or 0x4003
} GVSP_FILE_DATA_TRAILER;



#endif  // (GIGEVISION_MAJOR_VERSION == 2)


typedef struct _GVSP_TEST_PACKET_ {
    uint16_t			dontcare16;					// 16bits
    uint16_t			packetid16;					// = 0
	uint32_t			dontcare32;					// 32
} GVSP_TEST_PACKET;


// GVSP packet format
#define GVSP_DATA_LEADER_FORMAT			(0x1)
#define GVSP_DATA_TRAILER_FORMAT		(0x2)
#define GVSP_DATA_PAYLOAD_FORMAT_GD		(0x3)
#define GVSP_ALL_IN_FORMAT				(0x4)
#define GVSP_DATA_PAYLOAD_FORMAT_264	(0x5)
#define GVSP_DATA_PAYLOAD_FORMAT_MI		(0x6)



// payload type
#define GVSP_PAYLOAD_IMAGE				(0x0001)
#define GVSP_PAYLOAD_IMAGE_EX			(0x4001)
#define GVSP_PAYLOAD_RAWDATA			(0x0002)
#define GVSP_PAYLOAD_RAWDATA_EX			(0x4002)
#define GVSP_PAYLOAD_FILE				(0x0003)
#define GVSP_PAYLOAD_FILE_EX			(0x4003)
#define GVSP_PAYLOAD_CHUNKDATA			(0x0004)
#define GVSP_PAYLOAD_CHUNKDATA_EX		(0x0005)
#define GVSP_PAYLOAD_JPEG				(0x0006)
#define GVSP_PAYLOAD_JPEG_EX			(0x4006)
#define GVSP_PAYLOAD_JPEG2000			(0x0007)
#define GVSP_PAYLOAD_JPEG2000_EX		(0x4007)
#define GVSP_PAYLOAD_H264				(0x0008)
#define GVSP_PAYLOAD_H264_EX			(0x4008)
#define GVSP_PAYLOAD_MULTIZONEIMAGE		(0x0009)
#define GVSP_PAYLOAD_MULTIZONEIMAGE_EX	(0x0009)
#define GVSP_PAYLOAD_DEVICESPECIFIC		(0x8000)	// >= 0x8000都是这个宏

#pragma pack(pop)


#ifdef __cplusplus
}
#endif


#endif /* __GIGEVISION_GVSP_H__ */


