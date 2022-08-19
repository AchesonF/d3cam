/* GiGEVision v2.0.3 register defines */


#ifndef __GVCP_REGISTER_H__
#define __GVCP_REGISTER_H__

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************************************
*********************************************************************************************
*********************************************************************************************
 寄存器地址  参见白皮书2.0 318页 Table 28-1: Bootstrap Registers
 *********************************************************************************************
 *********************************************************************************************
 ********************************************************************************************/

//M R 4 Version of the GigE Standard with which the device is compliant
//The two most-significant bytes are allocated to the major
//number of the version, the two least-significant bytes for the minor number.
#define REG_Version											(0x0000)
// [0-15]major 
#define GEV_VERSION_MAJOR					(2u)
// [16-31]minor
#define GEV_VERSION_MINOR					(0u)

// M R 4 Information about device mode of operation.
#define REG_DeviceMode										(0x0004)
// [0]endianess 
#define DM_E								(1u<<31)
// [1-3]device_class 
#define	DM_DC_TRANSMITTER					(0u<<28)
#define	DM_DC_RECEIVER						(1u<<28)
#define	DM_DC_TRANSCEIVER					(2u<<28)
#define	DM_DC_PERIPHERAL					(3u<<28)
// [4-7]current link configuration 
#define DM_CLC_SingleLC						(0u<<24)
#define DM_CLC_MultipleLC					(1u<<24)
#define DM_CLC_StaticLC						(2u<<24)
#define DM_CLC_DynamicLC					(3u<<24)
// [8-23]reserved 
// [24-31]character set index
#define DM_CHARACTER_RESERVED				(0u<<0)
#define DM_CHARACTER_UTF8					(1u<<0)
#define DM_CHARACTER_ASCII					(2u<<0)

//(Network interface #0) M R 4 The two most-significant bytes of this area are reserved
//and will return 0. Upper 2 bytes of the MAC address are
//stored in the lower 2 significant bytes of this register.
#define REG_DeviceMACAddressHigh0							(0x0008)
// [0-15]reserved
// [16-31]mac address upper two bytes of MAC address

//(Network interface #0) M R 4 Lower 4 bytes of the MAC address
#define REG_DeviceMACAddressLow0							(0x000C)
// [0-31]mac address lower four bytes of MAC address

//(Network interface #0)M R 4 Supported IP Configuration and PAUSE schemes.
//Bits can be OR-ed. All other bits are reserved and set to0. DHCP and LLA bits must be on.
#define REG_NetworkInterfaceCapability0						(0x0010)
// [0]pause reception (PR)
#define NIC_PR								(1u<<31)
// [1]pause generation (PG)
#define NIC_PG								(1u<<30)
// [2-28]reserved
// [29]link local address is supported. always 1.
#define NIC_LLA								(1u<<2)
// [30]dhcp is supported. always 1.
#define NIC_DHCP							(1u<<1)
// [31]persistent ip is support. 0 otherwise. 1 supported.
#define NIC_PIP								(1u<<0)

//(Network interface #0)M R/W 4 Activated IP Configuration and PAUSE schemes.
//Bits can be OR-ed. LLA is always activated and is readonly.PAUSE settings might be hard-coded.
#define REG_NetworkInterfaceConfiguration0					(0x0014)

//0x0018 Reserved - - -

//(Networkinterface #0)M R 4 Current IP address of this device on its first interface.
#define REG_CurrentIPAddress0								(0x0024)
// [0-31]IPv4_address

//0x0028 Reserved - - -

//(Networkinterface #0)M R 4 Current subnet mask used by this device on its first interface.
#define REG_CurrentSubnetMask0								(0x0034)
// [0-31]IPv4_subnet_mask

//0x0038 Reserved - - -

//(Network interface #0)M R 4 Current default gateway used by this device on its first interface.
#define REG_CurrentDefaultGateway0							(0x0044)
// [0-31]IPv4_default_gateway

//M R 32 Provides the name of the manufacturer of the device.This string is provided in the manufacturer_name field of
//the DISCOVERY_ACK message.String using the format in character_set_index of Device Mode register.
#define REG_ManufacturerName								(0x0048)

//M R 32 Provides the model name of the device.This string is provided in the model_name field of the
//DISCOVERY_ACK message.String using the format in character_set_index of Device Mode register.
#define REG_ModelName										(0x0068)

//M R 32 Provides the version of the device.This string is provided in the device_version field of the
//DISCOVERY_ACK message.String using the format in character_set_index of DeviceMode register.
#define REG_DeviceVersion									(0x0088)

//M R 48 Provides extended manufacturer information about the device.This string is provided in the manufacturer_specific_info
//field of the DISCOVERY_ACK message.String using the format in character_set_index of Device Mode register.
#define REG_ManufacturerInfo								(0x00A8)

//O R 16 When supported, this string contains the serial number of the device. It can be used to identify the device. This
//optional register is supported when bit 1 of the GVCP Capability register is set.
//This string is provided in the serial_number field of the DISCOVERY_ACK message (set to all NULL when not
//supported).String using the format in character_set_index of Device Mode register.
#define REG_SerialNumber									(0x00D8)

// O R/W 16 When supported, this string contains a user-programmable name to assign to the device. It can be used to identify the
//device. This optional register is supported when bit 0 of the GVCP Capability register is set.
//This string is provided in the user_defined_name field of the DISCOVERY_ACK message (set to all NULL when
//not supported).String using the format in character_set_index of Device Mode register.
#define REG_UserdefinedName									(0x00E8)

//0x00F8 Reserved - - -

//M R 512 String using the format in character_set_index of Device Mode register.
#define REG_FirstURL										(0x0200)

//M R 512 String using the format in character_set_index of Device Mode register.
#define REG_SecondURL										(0x0400)

//M R 4 Indicates the number of physical network interfaces on this device. A device must have at least one network interface.
#define REG_NumberofNetworkInterfaces						(0x0600)
// [0-28]reserved
// [29-31]number_of_interfaces: only 1-4

//0x0604 Reserved - - -

//(Networkinterface #0)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentIPAddress								(0x064C)
// [0-31]persistent_IP_address

//0x0650 Reserved - - -

//(Network interface #0)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentSubnetMask0							(0x065C)
// [0-31]persistent_subnet_mask

//0x0660 Reserved - - -

//(Network interface #0)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentDefaultGateway0						(0x066C)
// [0-31]persistent_default_gateway

//(Network interface#0)O R 4 32-bit value indicating current Ethernet link speed in Mbits per second on the first interface.
#define REG_LinkSpeed0										(0x0670)
// [0-31]link_speed. Ethernet link speed value in Mbps. 0 if Link is down.

//0x0674 Reserved - - -

// (Network interface #1)O R 4 The two most-significant bytes of this area are reserved and will return 0. Upper 2 bytes of the MAC address are
//stored in the lower 2 significant bytes of this register.
#define REG_DeviceMACAddressHigh1							(0x0680)

// (Network interface #1)O R 4 Lower 4 bytes of the MAC address
#define REG_DeviceMACAddressLow1							(0x0684)

//(Network interface #1)O R 4 Supported IP Configuration and PAUSE schemes.Bits can be OR-ed. All other bits are reserved and set to
//0. DHCP and LLA bits must be on.
#define REG_NetworkInterfaceCapability1						(0x0688)

//(Network interface #1)O R/W 4 Activated IP Configuration and PAUSE schemes.Bits can be OR-ed. LLA is always activated and is readonly.
//PAUSE settings might be hard-coded.
#define REG_NetworkInterfaceConfiguration1					(0x068C)

//0x0690 Reserved - - -

//(Networkinterface #1)O R 4 Current IP address of this device on its second interface.
#define REG_CurrentIPAddress1								(0x069C)

//0x06A0 Reserved - - -

//(Networkinterface #1)O R 4 Current subnet mask used by this device on its second interface.
#define REG_CurrentSubnetMask1								(0x06AC)

//0x06B0 Reserved - - -

//(Network interface #1)O R 4 Current default gateway used by this device on its second interface.
#define REG_CurrentDefaultGateway1							(0x06BC)


//0x06C0 Reserved - - -

//(Network interface #1)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentIPAddress1							(0x06CC)

//0x06D0 Reserved - - -

//(Network interface #1)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentSubnetMask1							(0x06DC)


//0x06E0 Reserved - - -

//(Network interface #1)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentDefaultGateway1						(0x06EC)

//(Network interface#1)O R 4 32-bit value indicating current Ethernet link speed in Mbits per second on the second interface.
#define REG_LinkSpeed1										(0x06F0)

//0x06F4 Reserved - - -

// - High(Network interface #2)O R 4 The two most-significant bytes of this area are reserved
// and will return 0. Upper 2 bytes of the MAC address are stored in the lower 2 significant bytes of this register.
#define REG_DeviceMACAddressHigh2							(0x0700)

// - Low(Network interface #2)O R 4 Lower 4 bytes of the MAC address
#define REG_DeviceMACAddressLow2							(0x0704)

//(Network interface #2)O R 4 Supported IP Configuration and PAUSE schemes.
//Bits can be OR-ed. All other bits are reserved and set to 0. DHCP and LLA bits must be on.
#define REG_NetworkInterfaceCapability2						(0x0708)

//(Network interface #2)O R/W 4 Activated IP Configuration and PAUSE schemes.
//Bits can be OR-ed. LLA is always activated and is readonly.PAUSE settings might be hard-coded.
#define REG_NetworkInterfaceConfiguration2					(0x070C)

//0x0710 Reserved - - -

//(Network interface #2)O R 4 Current IP address of this device on its third interface.
#define REG_CurrentIPAddress2								(0x071C)

//0x0720 Reserved - - -

//(Network interface #2)O R 4 Current subnet mask used by this device on its third interface.
#define REG_CurrentSubnetMask2								(0x072C)

//0x0730 Reserved - - -

//(Network interface #2)O R 4 Current default gateway used by this device on its third interface.
#define REG_CurrentDefaultGateway2							(0x073C)

//0x0740 Reserved - - -

//(Network interface #2)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentIPAddress2							(0x074C)

//0x0750 Reserved - - -

//(Network interface #2)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentSubnetMask2							(0x075C)


//0x0760 Reserved - - -

//(Network interface #2)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentDefaultGateway2						(0x076C)

//(Network interface#2)O R 4 32-bit value indicating current Ethernet link speed in Mbits per second on the third interface.
#define REG_LinkSpeed2										(0x0770)

//0x0774 Reserved - - -

// - High Network interface #3)O R 4 The two most-significant bytes of this area are reserved and will return 0. Upper 2 bytes of the MAC address are
//stored in the lower 2 significant bytes of this register.
#define REG_DeviceMACAddressHigh3							(0x0780)

// - Low(Network interface #3)O R 4 Lower 4 bytes of the MAC address
#define REG_DeviceMACAddressLow3							(0x0784)

//(Network interface #3)O R 4 Supported IP Configuration and PAUSE schemes.
//Bits can be OR-ed. All other bits are reserved and set to 0. DHCP and LLA bits must be on.
#define REG_NetworkInterfaceCapabilityLow3					(0x0788)

//(Network interface #3)O R/W 4 Activated IP Configuration and PAUSE schemes.
//Bits can be OR-ed. LLA is always activated and is readonly.PAUSE settings might be hard-coded.
#define REG_NetworkInterfaceConfiguration3					(0x078C)

//0x0790 Reserved - - -

//(Network interface #3)O R 4 Current IP address of this device on its fourth interface.
#define REG_CurrentIPAddress3								(0x079C)

//0x07A0 Reserved - - -

//(Network interface #3)O R 4 Current subnet mask used by this device on its fourth interface.
#define REG_CurrentSubnetMask3								(0x07AC)

//0x07B0 Reserved - - -

//(Network interface #3)O R 4 Current default gateway used by this device on its fourth interface.
#define REG_CurrentDefaultGateway3							(0x07BC)

//0x07C0 Reserved - - -

//(Network interface #3)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentIPAddress3							(0x07CC)

//0x07D0 Reserved - - -

//(Network interface #3)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentSubnetMask3							(0x07DC)

//0x07E0 Reserved - - -

//(Network interface #3)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentDefaultGateway3						(0x07EC)

//(Network interface#3)O R 4 32-bit value indicating current Ethernet link speed in Mbits per second on the fourth interface.
#define REG_LinkSpeed3										(0x07F0)

//0x07F4 Reserved - - -

//M R 4 Indicates the number of message channels supported by this device. It can take two values: 0 or 1.
#define REG_NumberofMessageChannels							(0x0900)
// [0-31]number_of_message_channels

//M R 4 Indicates the number of stream channels supported by this device. It can take any value from 0 to 512.
#define REG_NumberofStreamChannels							(0x0904)
// [0-31]number_of_stream_channels

//O R 4 Indicates the number of separate action signals supported by this device. It can take any value ranging from 0 to 128.
#define REG_NumberofActionSignals							(0x0908)
// [0-31]number_of_action_signals

//O W 4 Device key to check the validity of action commands.
#define REG_ActionDeviceKey									(0x090C)
// [0-31]action_device_key
// This optional register provides the device key to check the validity of action commands. The action device
// key is write-only to hide the key if CCP is not in exclusive access mode. The intention is for the Primary
// Application to have absolute control. The Primary Application can send the key to a Secondary Application.
// This mechanism prevents a rogue application to have access to the ACTION_CMD mechanism.

//M R 4 Indicates the number of physical links that are currently active.
#define REG_NumberofActiveLinks								(0x0910)
// [0-27]reserved
// [28-31]number_of_active_links  <= REG_NumberofNetworkInterfaces

//0x0914 Reserved - - -

//M R 4 Indicates the capabilities of the stream channels. It lists which of the
//non-mandatory stream channel features are supported.
#define REG_GVSPCapability									(0x092C)
// [0]SCSPx (Stream Channel Source Port) registers supported.
#define GVSPCap_SP							(1<<31)
// [1]Indicates this GVSP transmitter or receiver can support 16-bit block_id.
// Note that GigE Vision 2.0 transmitters and receivers MUST support 64-bit block_id64.
// When 16-bit block_id are used, then 24-bit packet_id must be used. 
// When 64-bit block_id64 are used, then 32-bit packet_id32 must be used.
#define GVSPCap_LB							(1<<30)
// [2-31]reserved

//M R 4 Indicates the capabilities of the message channel. It lists
// which of the non-mandatory message channel features are supported.
#define REG_MessageChannelCapability						(0x0930)
// [0]MCSP (Message Channel Source Port) register is available for message channel.
#define MSGCap_SP							(1<<31)
// [1-31]reserved

//M R 4 This is a capability register indicating which one of the
//non-mandatory GVCP features are supported by this device.
#define REG_GVCPCapability									(0x0934)
// [0]User-defined Name register
#define GVCPCap_UN							(1u<<31)
// [1]Serial Number register
#define GVCPCap_SN							(1u<<30)
// [2]Heartbeat can be Disable
#define GVCPCap_HD							(1u<<29)
// [3]link Speed registers
#define GVCPCap_LS							(1u<<28)
// [4]CCP Application Port and IP address register
#define GVCPCap_CAP							(1u<<27)
// [5]Mainifest Table. when supported application must usr MT to retrieve the XML desc file.
#define GVCPCap_MT							(1u<<26)
// [6]Test packet is filled with Data from the LFSR generator.
#define GVCPCap_TD							(1u<<25)
// [7]Discovery ACK Delay register.
#define GVCPCap_DD							(1u<<24)
// [8]When Discovery ACK Delay reigister id supported, 
//   this bit indicates that the application can write it. If this bit is 0, the register is Read-Only.
#define GVCPCap_WD							(1u<<23)
// [9]Support generation of Extended Status codes introduced in spec 1.1
//  PACKET_(RESEND / NOT_YET_AVAILABLE / AND_PREV_REMOVED_FROM_MEMORY / REMOVED_FROM_MEMORY)
#define GVCPCap_ES							(1u<<22)
// [10]Primary application switchover capability.
#define GVCPCap_PAS							(1u<<21)
// [11]Uncoditional ACTION_CMD.
#define GVCPCap_UA							(1u<<20)
// [12]Support for IEEE1588 PTP
#define GVCPCap_PTP							(1u<<19)
// [13]Support generation of Extended Status codes introduces in spec 2.0
//  PACKET_TEMPORARILY_UNAVAILABLE / OVERFLOW / NO_REF_TIME
#define GVCPCap_ES2							(1u<<18)
// [14]Scheduled Action Commands.
#define GVCPCap_SAC							(1u<<17)
// [15-24]reserved
// [25]ACTION_CMD and ACTION_ACK are supported
#define GVCPCap_A							(1u<<6)
// [26]PENDING_ACK is supported.
#define GVCPCap_PA							(1u<<5)
// [27]EVENTDATA_CMD and EVENTDATA_ACK are supported
#define GVCPCap_ED							(1u<<4)
// [28]EVENT_CMD and EVENT_ACK are supported
#define GVCPCap_E							(1u<<3)
// [29]PACKETRESEND_CMD is supported
#define GVCPCap_PR							(1u<<2)
// [30]WRITEMEM_CMD and WRITEMEM_ACK are supported
#define GVCPCap_W							(1u<<1)
// [31]Multiple operations in a single message are supported.
#define GVCPCap_C							(1u<<0)

//M R/W 4 In msec, default is 3000 msec. Internally, the heartbeat is rounded according to the clock used for heartbeat. The
//heartbeat timeout shall have a minimum precision of 100ms. The minimal value is 500 ms.
#define REG_HeartbeatTimeout								(0x0938)

//High O R 4 64-bit value indicating the number of timestamp clock tick in 1 second. This register holds the most significant
//bytes. Timestamp tick frequency is 0 if timestamp is not supported.
#define REG_TimestampTickFrequencyHigh						(0x093C)
// [0-31]timestamp_frequency, upper 32-bit

//Low O R 4 64-bit value indicating the number of timestamp clock
//tick in 1 second. This register holds the least significant bytes.
//Timestamp tick frequency is 0 if timestamp is not supported. (1s = 1 000 000 000ns : 0x3B9ACA00)
#define REG_TimestampTickFrequencyLow						(0x0940)
// [0-31]timestamp_frequency, lower 32-bit

//O W 4 Used to latch the current timestamp value. No need to clear to 0.
#define REG_TimestampControl								(0x0944)
// [0-29]reserved
// [30]latch
#define TCtrl_L								(1u<<1)
// [31]reset
#define TCtrl_R								(1u<<0)

// - High O R 4 Latched value of the timestamp (most significant bytes)
#define REG_TimestampValueHigh								(0x0948)
// [0-31]latched_timestamp_value, upper 32-bit

// - Low O R 4 Latched value of the timestamp (least significant bytes)
#define REG_TimestampValueLow								(0x094C)
// [0-31]latched_timestamp_value, lower 32-bit

//O R/(W) 4 Maximum randomized delay in ms to acknowledge a DISCOVERY_CMD.
#define REG_DiscoveryACKDelay								(0x0950)
// [0-15]reserved
// [16-31]delay in ms


// O R/W 4 Configuration of GVCP optional features
#define REG_GVCPConfiguration								(0x0954)
// [0-11]reserved
// [12]IEEE1588_PTP_enable
#define GVCPCfg_PTP							(1u<<19)
// [13]extended_status_code_for_2_0
#define GVCPCfg_ES2							(1u<<18)
// [14-27]reserved
// [28]unconditional_ACTION_enable
#define GVCPCfg_UA							(1u<<3)
// [28]extended_status_code_for_1_1
#define GVCPCfg_ES							(1u<<2)
// [28]PENDING_ACK_enable
#define GVCPCfg_PE							(1u<<1)
// [28]heartbeat_disable
#define GVCPCfg_HD							(1u<<0)

//M R 4 Pending Timeout to report the longest GVCP command execution time before issuing a PENDING_ACK. If
//PENDING_ACK is not supported, then this is the worstcase execution time before command completion.
#define REG_PendingTimeout									(0x0958)
// [0-31]max_execution_time in ms

//O W 4 Key to authenticate primary application switchover requests.
#define REG_ControlSwitchoverKey							(0x095C)
// [0-15]reserved
// [16-31]control_switchover_key
// This optional register provides the key to authenticate primary application switchover requests. The register
// is write-only to hide the key from secondary applications. The primary intent is to have a mechanism to
// control who can take control over a device. The primary application or a higher level system management
// entity can send the key to an application that would request control over a device.

//O R/W 4 Configuration of the optional GVSP features.
#define REG_GVSPConfiguration								(0x0960)
//[0]reserved
//[1]64bit_block_id_enable
#define GVSPCcfg_BL							(1<<30)
//[2-31]reserved

// M R 4 Indicates the physical link configuration supported by thisdevice.
#define REG_PhysicalLinkConfigurationCapability				(0x0964)
// [0-27]reserved
// [28]dynamic_link_aggregation_group_configuration
#define PLCC_dLAG							(1u<<3)
// [29]static_link_aggregation_group_configuration
#define PLCC_sLAG							(1u<<2)
// [30]multiple_links_configuration
#define PLCC_ML								(1u<<1)
// [31]single_link_configuration
#define PLCC_SL								(1u<<0)

//M R/(W) 4 Indicates the currently active physical link configuration
#define REG_PhysicalLinkConfiguration						(0x0968)
// [0-29]reserved
// [30-31]link_configuration
#define PLC_SL								(0u<<0)
#define PLC_ML								(1u<<0)
#define PLC_sLAG							(2u<<0)
#define PLC_dLAG							(3u<<0)

//O R 4 Reports the state of the IEEE 1588 clock
#define REG_IEEE1588Status									(0x096C)
// [0-27]reserved
// [28-31]clock_status: Values of this field must match the IEEE 1588 PTP port state enumeration.
// INITIALIZING, FAULTY, DISABLED,LISTENING, PRE_MASTER, MASTER, PASSIVE, UNCALIBRATED, SLAVE.

//O R 4 Indicates the number of Scheduled Action Commands that
//can be queued (size of the queue).
#define REG_ScheduledActionCommandQueueSize					(0x0970)
// [0-31]queue_size

//0x0974 Reserved - - -

//(CCP)M R/W 4 Control Channel Privilege register.
#define REG_ControlChannelPrivilege							(0x0A00)
// [0-15]control switchover key
// [16-28]reserved
// [29]control switchover_enable
#define CCP_CSE								(1u<<2)
// [30]control access
#define CCP_CA								(1u<<1)
// [31]exclusive access
#define CCP_EA								(1u<<0)

//O R 4 UDP source port of the control channel of the primary application.
#define REG_PrimaryApplicationPort							(0x0A04)
// [0-15]reserved
// [16-31]primary_application_port
// This value must be 0 when no primary application is bound to the device.(CCP=0)

//0x0A08 Reserved - - -

//O R 4 Source IP address of the control channel of the primary application.
#define REG_PrimaryApplicationIPAddress						(0x0A14)
// [0-31]primary_application_IP_address

//0x0A18 Reserved - - -

//(MCP) O R/W 4 Message Channel Port register.
#define REG_MessageChannelPort								(0x0B00)
// [0-11]reserved
// [12-15]network_interface_index: Always 0 in this version. Only the primary interface (#0) supports GVCP.
// [16-31]host_port

//0x0B04 Reserved - - -

//Address (MCDA)O R/W 4 Message Channel Destination Address register.
#define REG_MessageChannelDestination						(0x0B10)
// [0-31]channel_destination_IP

//(MCTT)O R/W 4 Message Channel Transmission Timeout in milliseconds.
#define REG_MessageChannelTransmissionTimeout				(0x0B14)
// [0-31]timeout in milliseconds.

//(MCRC)O R/W 4 Message Channel Retry Count.
#define REG_MessageChannelRetryCount						(0x0B18)
// [0-31]retry_count

//(MCSP)O R 4 Message Channel Source Port.
#define REG_MessageChannelSourcePort						(0x0B1C)
// [0-15]reserved
// [16-31]source_port

//0x0B20 Reserved - - -

//(SCP0) M1 R/W 4 First Stream Channel Port register.
#define REG_StreamChannelPort0								(0x0D00)
// [0] direction 0:Transmitter 1:Receiver
#define SCP_D								(1u<<31)
// [1-11]reserved
// [12-15]network_interface_index
#define SCP_IF0								(0u<<16)
#define SCP_IF1								(1u<<16)
#define SCP_IF2								(2u<<16)
#define SCP_IF3								(3u<<16)
// [16-31]host_port

//(SCPS0)M1 R/W 4 First Stream Channel Packet Size register.
#define REG_StreamChannelPacketSize0						(0x0D04)
// [0]fire_test_packet
#define SCPS_F								(1u<<31)
// [1]do_not_fragment
#define SCPS_D								(1u<<30)
// [2]pixel_endianess
#define SCPS_P								(1u<<29)
// [3-15]reserved
// [16-31]packet_size

//(SCPD0)M2 R/W 4 First Stream Channel Packet Delay register. ticks: ns
#define REG_StreamChannelPacketDelay0						(0x0D08)
// [0-31]delays  Inter-packet delay in timestamp ticks

//0x0D0C Reserved - - -

//(SCDA0)M1 R/W 4 First Stream Channel Destination Address register.
#define REG_StreamChannelDestinationAddress0				(0x0D18)
// [0-31]channel_destination_IP

//(SCSP0)O R 4 First Stream Channel Source Port register.
#define REG_StreamChannelSourcePort0						(0x0D1C)
// [0-15]reserved
// [16-31]source_port. 
// Indicates the UDP source port GVSP traffic will be generated from while the streaming channel is open.

//(SCC0)O R 4 First Stream Channel Capability register.
#define REG_StreamChannelCapability0						(0x0D20)
// [0]big_and_little_endian_supported
#define SCC_BE								(1u<<31)
// [1]IP_reassembly_supported 
#define SCC_R								(1u<<30)
// [2-26]reserved
// [27]multi_zone_supported
#define SCC_MZ								(1u<<4)
// [28]packet_resend_destination_option
#define SCC_PRD								(1u<<3)
// [29]all_in_transmission_supported
#define SCC_AIT								(1u<<2)
// [30]unconditional_streaming_supported
#define SCC_US								(1u<<1)
// [31]extended_chunk_data_supported
#define SCC_EC								(1u<<0)

//(SCCFG0)O R/W 4 First Stream Channel Configuration register.
#define REG_StreamChannelConfiguration0						(0x0D24)
// [0-27]reserved
// [28]packet_resend_destination
#define SCCFG_PRD							(1u<<3)
// [29]all_in_transmission_enabled
#define SCCFG_AIT							(1u<<2)
// [30]unconditional_streaming_enabled
#define SCCFG_US							(1u<<1)
// [31]extended_chunk_data_enable
#define SCCFG_EC							(1u<<0)

//(SCZ0) O R 4 First Stream Channel Zone register (multi-zone image payload type only).
#define REG_StreamChannelZone0								(0x0D28)
// [0-26]reserved
// [27-31]additional_zones

//(SCZD0)O R 4 First Stream Channel Zone Direction register (multi-zone image payload type only).
#define REG_StreamChannelZoneDirection0						(0x0D2C)
// [0-31]
#define SCZD_Z0								(1u<<31)
#define SCZD_Z1								(1u<<30)
#define SCZD_Z2								(1u<<29)
#define SCZD_Z3								(1u<<28)
#define SCZD_Z4								(1u<<27)
#define SCZD_Z5								(1u<<26)
#define SCZD_Z6								(1u<<25)
#define SCZD_Z7								(1u<<24)
#define SCZD_Z8								(1u<<23)
#define SCZD_Z9								(1u<<22)
#define SCZD_Z10							(1u<<21)
#define SCZD_Z11							(1u<<20)
#define SCZD_Z12							(1u<<19)
#define SCZD_Z13							(1u<<18)
#define SCZD_Z14							(1u<<17)
#define SCZD_Z15							(1u<<16)
#define SCZD_Z16							(1u<<15)
#define SCZD_Z17							(1u<<14)
#define SCZD_Z18							(1u<<13)
#define SCZD_Z19							(1u<<12)
#define SCZD_Z20							(1u<<11)
#define SCZD_Z21							(1u<<10)
#define SCZD_Z22							(1u<<9)
#define SCZD_Z23							(1u<<8)
#define SCZD_Z24							(1u<<7)
#define SCZD_Z25							(1u<<6)
#define SCZD_Z26							(1u<<5)
#define SCZD_Z27							(1u<<4)
#define SCZD_Z28							(1u<<3)
#define SCZD_Z29							(1u<<2)
#define SCZD_Z30							(1u<<1)
#define SCZD_Z31							(1u<<0)

//0x0D30 Reserved - - -

//(SCP1) O R/W 4 Second stream channel, if supported.
#define REG_StreamChannelPort1								(0x0D40)

//(SCPS1)O R/W 4 Second stream channel, if supported.
#define REG_StreamChannelPacketSize1						(0x0D44)

//(SCPD1)O R/W 4 Second stream channel, if supported.
#define REG_StreamChannelPacketDelay1						(0x0D48)

//0x0D4C Reserved - - -

//(SCDA1)O R/W 4 Second stream channel, if supported.
#define REG_StreamChannelDestinationAddress1				(0x0D58)

//(SCSP1)O R 4 Second stream channel, if supported.
#define REG_StreamChannelSourcePort1						(0x0D5C)

//(SCC1)O R 4 Second stream channel, if supported.
#define REG_StreamChannelCapability1						(0x0D60)

//(SCCFG1)O R/W 4 Second stream channel, if supported.
#define REG_StreamChannelConfiguration1						(0x0D64)

//(SCZ1) O R 4 Second stream channel, if supported.
#define REG_StreamChannelZone1								(0x0D68)

//(SCZD1) O R 4 Second stream channel, if supported.
#define REG_StreamChannelZoneDirection1						(0x0D6C)

//0x0D70 Reserved - - -

//O R/W - Each stream channel is allocated a section of 64 bytes(0x40). Only supported channels are available.
#define REG_OtherStreamChannelsRegisters					(0x0D80)

//(SCP511)O R/W 4 512th stream channel, if supported.
#define REG_StreamChannelPort511							(0x8CC0)

//  (SCPS511)O R/W 4 512th stream channel, if supported.

#define REG_StreamChannelPacketSize511						(0x8CC4)

//(SCPD511)O R/W 4 512th stream channel, if supported.
#define REG_StreamChannelPacketDelay511						(0x8CC8)

//0x8CCC Reserved - - -

//(SCDA511)O R/W 4 512th stream channel, if supported.
#define REG_StreamChannelDestinationAddress511				(0x8CD8)

//(SCSP511)O R 4 512th stream channel, if supported.
#define REG_StreamChannelSourcePort511						(0x8CDC)

//(SCC511)O R 4 512th stream channel, if supported.
#define REG_StreamChannelCapability511						(0x8CE0)

//(SCCFG511)O R/W 4 512th stream channel, if supported.
#define REG_StreamChannelConfiguration511					(0x8CE4)

//(SCZ511)O R 4 512th stream channel, if supported.
#define REG_StreamChannelZone511							(0x8CE8)

//(SCZD511)O R 4 512th stream channel, if supported.
#define REG_StreamChannelZoneDirection511					(0x8CEC)

//0x8CF0 Reserved - - -

//O R 512 Manifest Table providing information to retrieve XML documents stored in the device.
#define REG_ManifestTable									(0x9000)

//0x9200 Reserved - - -

//O R/W 4 First action signal group key
#define REG_ActionGroupKey0									(0x9800)
// [0-31]action_group_key

//O R/W 4 First action signal group mask
#define REG_ActionGroupMask0								(0x9804)
// [0-31]action_group_mask

//0x9808 Reserved - - -

//O R/W 4 Second action signal group key
#define REG_ActionGroupKey1									(0x9810)

//O R/W 4 Second action signal group mask
#define REG_ActionGroupMask1								(0x9814)

//0x9818 Reserved - - -

//O R/W - Each action signal is allocated a section of 16 bytes(0x10). Only supported action signals are available.
#define REG_OtherActionSignalsRegisters						(0x9820)

//O R/W 4 128th action signal group key
#define REG_ActionGroupKey127								(0x9FF0)

//O R/W 4 128th action signal group mask
#define REG_ActionGroupMask127								(0x9FF4)

//0x9FF8 Reserved - - -

//- - - Start of manufacturer-specific registers. These are not
//covered by the specification and should be reported through the XML device description file.
#define REG_StartofManufacturerSpecificRegisterSpace		(0xA000)

#define REG_XML_AcquisitionStart							(0x30804)
#define REG_XML_AcquisitionStop								(0x30808)
#define REG_XML_Width										(0x30360)
#define REG_XML_Height										(0x303A0)
#define REG_XML_OffsetX										(0x303E0)
#define REG_XML_OffsetY										(0x30420)

//O R 536(max) Start of XML file ... size 1M bytes  [0x100000-0x1FFFFF]
#define REG_StartOfXmlFile									(0x100000)



#ifdef __cplusplus
}
#endif

#endif  /* __GVCP_REGISTER_H__ */


