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


// genicam custom registers

// R 4 Category Inquiry Register
#define REG_Category_Inq									(0x00010004)
// [0]CounterAndTimerControl
#define CounterAndTimerControl				(1u<<31)		
// [1-31]reserved

// R 4 DeviceControl Inquiry Register
#define REG_DeviceControlInq								(0x00010104)
// [0]DeviceTemperatureSelector
// [1]DeviceTemperatureSelector_Sensor
// [2]DeviceTemperatureSelector_Mainboard
// [3]DeviceLinkThroughputLimit
// [4]DeviceMaxThroughput
// [5]DeviceLinkHeartbeatTimeout
// [6-31]

// R 4 GevSCPSMax Register : REG_StreamChannelPacketSize0
#define REG_GevSCPSMaxInq									(0x00010120)

// R 4 GevSCPSMin Register
#define REG_GevSCPSMinInq									(0x00010124)

// R 4 GevSCPDMax Register : REG_StreamChannelPacketDelay0
#define REG_GevSCPDMaxInq									(0x00010130)

// R 4 GevSCPDMin Register
#define REG_GevSCPDMinInq									(0x00010134)

// R 4 Reverse Inq Register
#define REG_ReverseInq										(0x00010180)
// [0]ReverseXInq_Reg
// [1]ReverseYInq_Reg
// [2]Binning_Horizontal_Inq
// [3]Binning_Vertical_Inq
// [4]Deinterlacing_Off_Inq
// [5]Deinterlacing_lineDup_Inq
// [6]Deinterlacing_Weave_Inq
// [7]Sensor_Taps_Inq
// [8]Sensor_Dig_Taps_Inq
// [9]RegionMode_Inq
// [10]LinePitch_Inq
// [11]PixelCoding_Inq
// [12]PixelColorFilter_Inq
// [13]PixelDynamicRangeMin_Inq
// [14]PixelDynamicRangeMax_Inq
// [15-31]reserved

// R 4 WidthMin Register
#define REG_WidthMin										(0x00010184)

// R 4 HeightMin Register
#define REG_HeightMin										(0x00010188)

// R 4 Binning Max Register
#define REG_BinningMax										(0x0001018C)
// [0]BinningHorizontal_1
// [1]BinningHorizontal_2
// [2]BinningHorizontal_3
// [3]BinningHorizontal_4
// [4-15]reserved
// [16]BinningVertical_1
// [17]BinningVertical_2
// [18]BinningVertical_3
// [19]BinningVertical_4
// [20-31]reserved

// R 4 Register
#define REG_Hunit											(0x00010190)

// R 4 Register
#define REG_Vunit											(0x00010194)

// R 4 RegionSelector Inquiry Register
#define REG_RegionSelectorInq								(0x00010198)
// [0] Region_0
// [1] Region_1
// [2] Region_2
// [3-29] reserved
// [30] Region_Sensor
// [31] Region_All

// R 4 Video Mode Inquiry Register
#define REG_VideoModeInq									(0x0001019C)
// [0] VideoMode0
// [1] VideoMode1
// [2] VideoMode2
// [3] VideoMode3
// [4] VideoMode4
// [5] VideoMode5
// [6] VideoMode6
// [7] VideoMode7
// [8] VideoMode8
// [9] VideoMode9
// [10] VideoMode10
// [11] VideoMode11
// [12] VideoMode12
// [13] VideoMode13
// [14] VideoMode14
// [15] VideoMode15
// [16-31] reserved

// R 4 PixelFormat Inquiry 0 Register
#define REG_PixelFormatInq0									(0x000101A0)
// [0] Mono8
#define PFI_Mono8							(1<<31)
// [1] YUV411Packed
// [2] YUV422Packed
// [3] YUV444Packed
// [4] RGB8
// [5] Mono16
// [6] Mono12Packed
// [7] YUV422YUYVPacked
// [8] Mono10
// [9] Mono12
// [10] Mono10Packed
// [11-31] reserved

// R 4 PixelFormat Inquiry Register
#define REG_PixelFormatInq									(0x000101A4)
// [0] BayerGR8
// [1] BayerGB8
// [2] BayerBG8
// [3] BayerRG8
// [4] BayerGR16
// [5] BayerRG16
// [6] BayerBG16
// [7] BayerGB16
// [8] BayerGR12Packed
// [9] BayerGB12Packed
// [10] BayerBG12Packed
// [11] BayerRG12Packed
// [12] BayerGR12
// [13] BayerRG12
// [14] BayerBG12
// [15] BayerGB12
// [16] BayerGR10
// [17] BayerRG10
// [18] BayerGB10
// [19] BayerBG10
// [20] BayerGR10Packed
// [21] BayerRG10Packed
// [22] BayerGB10Packed
// [23] BayerBG10Packed
// [24-31] reserved

// R 4 TestPattern Inquiry Register
#define REG_TestPatternInq									(0x000101A8)
// [0] Black
// [1] White
// [2] GreyHRamp
// [3] GreyVRamp
// [4] GreyHMoving
// [5] GreyVMoving
// [6] HLineMoving
// [7] VLineMoving
// [8] ColorBar
// [9] FrameCounter
// [10] MonoBar
// [11] TestImage12
// [12] TestImage13
// [13-31] reserved

// R 4 embedded image info Inquiry Register
#define REG_FrameSpecInfoInq								(0x000101AC)
// [0] FrameSpecInfo
// [1] Timestamp
// [2] Gain
// [3] Exposure
// [4] BrightnessInfo
// [5] WhiteBalance
// [6] Framecounter
// [7] ExtTriggerCount
// [8] ROIPosition
// [9-31] reserved

// R 4 Frame Rate Control Inquiry Register
#define REG_FrameRateControlInq								(0x00010200)
// [0] AcqFrameRatePres
// [1] AcqFrameRateEn
// [2] AcqModeContinuous
// [3] AcqModeSingleFrame
// [4] AcqModeMultiFrame
// [5-31] reserved

// R 4 Register
#define REG_AutoExposureInq									(0x00010204)
// [0] AutoExposurePres
// [1] AutoExposureOnePush
// [2] AutoExposureOnOff
// [3] AutoExposureAutoMode
// [4] ExposureMode
// [5] ExposureModeTimed
// [6] ExposureModeTriggerWidth
// [7] GainShutPrior
// [8] GainPrior
// [9] ShutPrior
// [10] HDR
// [11] HDRGain
// [12-31] reserved

// R 4 Register
#define REG_ExposureTimeMaxInq								(0x00010208)

// R 4 Register
#define REG_ExposureTimeMinInq								(0x0001020C)

// R 4 Trigger Inquiry Register
#define REG_TriggerInq										(0x00010210)
// [0] OnOff
// [1] Activation
// [2] Delay
// [3] SourceSw
// [4] SourceCounter0
// [5-31] reserved

// R 4 Trigger Select Inquiry Register
#define REG_TriggerSelectInq								(0x00010214)
// [0] AcqStart
// [1] AcqEnd
// [2] AcqActive
// [3] FrameStart
// [4] FrameEnd
// [5] FrameActive
// [6] FrameBStart
// [7] FrameBEnd
// [8] FrameBActive
// [9] LineStart
// [10] ExpStart
// [11] ExpEnd
// [12] ExpActive
// [13-31] reserved

// R 4 Trigger Delay Max Microsecs Register
#define REG_TriggerDelayAbsMax								(0x00010220)

// R 4 Trigger Delay Min Microsecs Register
#define REG_TriggerDelayAbsMin								(0x00010224)

// R 4 Acquistion Frame Rate Max Register
#define REG_AcqFrameRateMaxInq								(0x00010230)

// R 4 Acquistion Frame Rate Min Register
#define REG_AcqFrameRateMinInq								(0x00010234)

// R 4 Acquisition Frame Count Max Register
#define REG_AcqFrameCountMaxInq								(0x00010240)

// R 4 Acquisition Frame Count Min Register
#define REG_AcqFrameCountMinInq								(0x00010244)

// R 4 Acquisition Burst Frame Count Max Register
#define REG_AcqBurstFrameCountMaxInq						(0x00010250)

// R 4 Acquisition Burst Frame Count Min Register
#define REG_AcqBurstFrameCountMinInq						(0x00010254)

// R 4 DigitalIOControl Inq Register
#define REG_DigitalIOControlInq								(0x00010280)
// [0] GPIOPin0Pres
// [1] GPIOPin1Pres
// [2] GPIOPin2Pres
// [3] GPIOPin3Pres
// [4-31] reserved

// R 4 LineMode Input Inquiry Register
#define REG_LineModeInputInq								(0x00010284)
// [0] TriggerSource0
// [1] TriggerSource1
// [2] TriggerSource2
// [3] TriggerSource3
// [4-31] reserved

// R 4 LineMode Output Inquiry Register
#define REG_LineModeOutputInq								(0x00010288)
// [0] Strobe0
// [1] Strobe1
// [2] Strobe2
// [3] Strobe3
// [4-31] reserved

// R 4 Strobe Line Inquiry Register
#define REG_LineStrobeInq									(0x0001028C)
// [0] StrobeLine0OnOff
// [1] StrobeLine1OnOff
// [2] StrobeLine2OnOff
// [3] StrobeLine3OnOff
// [4-31] reserved

// R 4 Line Debouncer Time Min Register
#define REG_LineDebouncerTimeMinInq							(0x00010290)

// R 4 Line Debouncer Time Max Register
#define REG_LineDebouncerTimeMaxInq							(0x00010294)

// R 4 Line Debouncer Time Unit Register
#define REG_LineDebouncerTimeUnitInq						(0x00010298)

// R 4 Strobe Line Duration Min Register
#define REG_StrobeLineDurationMinInq						(0x0001029C)

// R 4 Strobe Line Duration Max Register
#define REG_StrobeLineDurationMaxInq						(0x000102A0)

// R 4 Strobe Line Duration Unit Register
#define REG_StrobeLineDurationUnitInq						(0x000102A4)

// R 4 Strobe Line Delay Min Register
#define REG_StrobeLineDelayMinInq							(0x000102A8)

// R 4 Strobe Line Delay Max Register
#define REG_StrobeLineDelayMaxInq							(0x000102AC)

// R 4 Strobe Line Delay Unit Register
#define REG_StrobeLineDelayUnitInq							(0x000102B0)

// R 4 Strobe Line Pre Delay Min Register
#define REG_StrobeLinePreDelayMinInq						(0x000102B4)

// R 4 Strobe Line Pre Delay Max Register
#define REG_StrobeLinePreDelayMaxInq						(0x000102B8)

// R 4 Strobe Line Pre Delay Unit Register
#define REG_StrobeLinePreDelayUnitInq						(0x000102BC)

// R 4 Hue Inquiry Register
#define REG_HueInq											(0x00010400)
// [0] Pres
// [1] OnOff
// [2] Auto
// [3] Manual
// [4] Once
// [5-31] reserved

// R 4 Hue Ab Min Register
#define REG_HueAbsMinInq									(0x00010404)

// R 4 Hue Ab Max Register
#define REG_HueAbsMaxInq									(0x00010408)

// R 4 Gain Inquiry Register
#define REG_GainInq											(0x00010410)
// [0] GainPres
// [1] GainOnOff
// [2] GainAuto
// [3] GainManual
// [4] GainOnce
// [5-31] reserved

// R 4 Gain Abs Min Register
#define REG_GainAbsMinInq									(0x00010414)

// R 4 Gain Abs Max Register
#define REG_GainAbsMaxInq									(0x00010418)

// R 4 BlackLevel Inquiry Register
#define REG_BlackLevelInq									(0x00010420)
// [0] Pres
// [1] OnOff
// [2] Auto
// [3] Manual
// [4] Once
// [5-31] reserved

// R 4 Black Level Min Register
#define REG_BlackLevelMinInq								(0x00010424)

// R 4 Black Level Max Register
#define REG_BlackLevelMaxInq								(0x00010428)

// R 4 BalanceWhiteAuto enable Inq Register
#define REG_BalanceWhiteAutoInq								(0x00010430)
// [0] Off
// [1] Once
// [2] Continuous
// [3-31] reserved

// R 4 BalanceRatioSelector Inq Register
#define REG_BalanceRatioSelectorInq							(0x00010434)
// [0] BalanceRatioSelector
// [1] Red
// [2] Blue
// [3] Green
// [4-31] reserved

// R 4 Balance Ratio Min Register
#define REG_BalanceRatioMinInq								(0x00010440)

// R 4 Balance Ratio Max Register
#define REG_BalanceRatioMaxInq								(0x00010460)

// R 4 Sharpness Inquiry Register
#define REG_SharpnessInq									(0x000104A0)
// [0] Pres
// [1] OnOff
// [2] Auto
// [3] Manual
// [4] Once
// [5-31] reserved

// R 4 Sharpness Min Register
#define REG_SharpnessMinInq									(0x000104A4)

// R 4 Sharpness Max Register
#define REG_SharpnessMaxInq									(0x000104A8)

// R 4 Saturation Inquiry Register
#define REG_SaturationInq									(0x000104B0)
// [0] Pres
// [1] OnOff
// [2] Auto
// [3] Manual
// [4] Once
// [5-31] reserved

// R 4 Saturation Abs Min Register
#define REG_SaturationAbsMinInq								(0x000104B4)

// R 4 Saturation Abs Max Register
#define REG_SaturationAbsMaxInq								(0x000104B8)

// R 4 Gamma Inquiry Register
#define REG_GammaInq										(0x000104C0)
// [0] Pres
// [1] OnOff
// [2] Selector_User
// [3] Selector_sRGB
// [4-31] reserved

// R 4 Gamma Abs Min Register
#define REG_GammaAbsMinInq									(0x000104C4)

// R 4 Gamma Abs Max Register
#define REG_GammaAbsMaxInq									(0x000104C8)

// R 4 Digital shift Inquiry Register
#define REG_DigitalShiftInq									(0x000104D0)


// [0] 
// [1] 
// [2] 
// [3] 
// [4] 
// [5] 
// [6] 
// [7] 
// [8] 
// [9] 

#define REG_DigitalShiftMinInq								(0x000104D4)
#define REG_DigitalShiftMaxInq								(0x000104D8)
#define REG_DigitalShiftUintInq								(0x000104DC)

#define REG_BrightnessInq									(0x000104E0)

#define REG_BrightnessMinInq								(0x000104E4)
#define REG_BrightnessMaxInq								(0x000104E8)
#define REG_BrightnessUintInq								(0x000104EC)


#define REG_AutoAOIInq										(0x000104F0)

#define REG_AutoLimitInq									(0x000104F4)

#define REG_AutoExposureTimeLowerLimitMinInq				(0x00010500)
#define REG_AutoExposureTimeLowerLimitMaxInq				(0x00010504)
#define REG_AutoExposureTimeLowerLimitIncInq				(0x00010508)
#define REG_AutoExposureTimeUpperLimitMinInq				(0x00010510)
#define REG_AutoExposureTimeUpperLimitMaxInq				(0x00010514)
#define REG_AutoExposureTimeUpperLimitIncInq				(0x00010518)
#define REG_AutoGainLowerLimitMinInq						(0x00010520)
#define REG_AutoGainLowerLimitMaxInq						(0x00010524)
#define REG_AutoGainLowerLimitIncInq						(0x00010528)
#define REG_AutoGainUpperLimitMinInq						(0x00010530)
#define REG_AutoGainUpperLimitMaxInq						(0x00010534)
#define REG_AutoGainUpperLimitIncInq						(0x00010538)
#define REG_DNRInq											(0x0001053C)
#define REG_TemporalNoiseReductionMinInq					(0x00010540)
#define REG_TemporalNoiseReductionMaxInq					(0x00010544)
#define REG_TemporalNoiseReductionIncInq					(0x00010548)
#define REG_AirspaceNoiseReductionMinInq					(0x0001054C)
#define REG_AirspaceNoiseReductionMaxInq					(0x00010550)
#define REG_AirspaceNoiseReductionIncInq					(0x00010554)
#define REG_NoiseReductionMinInq							(0x00010558)
#define REG_NoiseReductionMaxInq							(0x0001055C)
#define REG_NoiseReductionIncInq							(0x00010560)
#define REG_FanOpenThresholdMinInq							(0x00010564)
#define REG_FanOpenThresholdMaxInq							(0x00010568)
#define REG_FanOpenThresholdIncInq							(0x0001056C)
#define REG_GainValueInq0									(0x00010570)

#define REG_LUTControlInq									(0x00010600)

#define REG_LUTIndexMinInq									(0x00010620)
#define REG_LUTIndexMaxInq									(0x00010640)
#define REG_LUTIndexIncInq									(0x00010660)
#define REG_LUTValueMinInq									(0x00010680)
#define REG_LUTValueMaxInq									(0x000106A0)
#define REG_LUTValueIncInq									(0x000106C0)
#define REG_NUCInq											(0x00010720)
#define REG_FPNCIndexMinInq									(0x00010730)
#define REG_FPNCIndexMaxInq									(0x00010734)
#define REG_FPNCIndexIncInq									(0x00010738)
#define REG_FPNCValueMinInq									(0x0001073C)
#define REG_FPNCValueMaxInq									(0x00010740)
#define REG_FPNCValueIncInq									(0x00010744)
#define REG_PRNUCIndexMinInq								(0x00010748)
#define REG_PRNUCIndexMaxInq								(0x0001074C)
#define REG_PRNUCIndexIncInq								(0x00010750)
#define REG_PRNUCValueMinInq								(0x00010754)
#define REG_PRNUCValueMaxInq								(0x00010758)
#define REG_PRNUCValueIncInq								(0x0001075C)
#define REG_DPCIndexMinInq									(0x00010760)
#define REG_DPCIndexMaxInq									(0x00010764)
#define REG_DPCIndexIncInq									(0x00010768)
#define REG_UserDataIndexMinInq								(0x0001076C)
#define REG_UserDataIndexMaxInq								(0x00010770)
#define REG_UserDataIndexIncInq								(0x00010774)
#define REG_DCCIndexMinInq									(0x00010778)
#define REG_DCCIndexMaxInq									(0x0001077C)
#define REG_DCCIndexIncInq									(0x00010780)
#define REG_DLCIndexMinInq									(0x00010784)
#define REG_DLCIndexMaxInq									(0x00010788)
#define REG_DLCIndexIncInq									(0x0001078C)

#define REG_UserSetInq										(0x00010800)

#define REG_AutoCorrectionInq								(0x00010804)

#define REG_CounterAndTimerInq								(0x00010840)

#define REG_CounterSelectorInq								(0x00010844)

#define REG_CounterValueMinInq								(0x00010864)

#define REG_CounterValueMaxInq								(0x00010868)

#define REG_CounterValueIncInq								(0x0001086C)

#define REG_CounterEventSourceInq							(0x00010870)
#define REG_CounterEventActivationInq						(0x00010874)
#define REG_CounterResetSourceInq							(0x00010878)
#define REG_CounterResetActivationInq						(0x0001087C)
#define REG_EncoderControlInq								(0x00010980)

// [0] 
// [1] 
// [2] 
// [3] 
// [4] 
// [5] 
// [6] 
// [7] 
// [8] 
// [9] 

#define REG_EncoderSelectorInq								(0x00010984)
#define REG_EncoderOutputModeInq							(0x00010990)
#define REG_EncoderCounterModeInq							(0x00010994)
#define REG_EncoderCounterMaxMinInq							(0x00010998)
#define REG_EncoderCounterMaxMaxInq							(0x0001099C)
#define REG_EncoderCounterMaxIncInq							(0x000109A0)
#define REG_EncoderMaxReverseCounterMinInq					(0x000109A4)
#define REG_EncoderMaxReverseCounterMaxInq					(0x000109A8)
#define REG_EncoderMaxReverseCounterIncInq					(0x000109AC)
#define REG_FrequencyConverterInq							(0x00010A00)
#define REG_SignalAlignmentInq								(0x00010A04)
#define REG_PreDividerMinInq								(0x00010A08)
#define REG_PreDividerMaxInq								(0x00010A0C)
#define REG_PreDividerIncInq								(0x00010A10)
#define REG_MultiplierMinInq								(0x00010A14)
#define REG_MultiplierMaxInq								(0x00010A18)
#define REG_MultiplierIncInq								(0x00010A1C)
#define REG_PostDividerMinInq								(0x00010A20)
#define REG_PostDividerMaxInq								(0x00010A24)
#define REG_PostDividerIncInq								(0x00010A28)
#define REG_ShadingSelectorInq								(0x00010B00)
#define REG_ShadingCorrectionInq							(0x00010B04)
#define REG_DefaultShadingSetSelectorInq					(0x00010B08)
#define REG_ShadingSetSelectorInq							(0x00010B0C)
#define REG_CreateShadingSetInq								(0x00010B10)
#define REG_ShadingStatusInq								(0x00010B14)
#define REG_FileAccessControlInq							(0x00010B18)
#define REG_FileSelectorInq									(0x00010B1C)
#define REG_FileOperationSelectorInq						(0x00010B20)
#define REG_FileOpenModeInq									(0x00010B24)
#define REG_FileOperationStatusInq							(0x00010B28)
#define REG_FileAccessOffsetMinInq							(0x00010B2C)
#define REG_FileAccessOffsetMaxInq							(0x00010B30)
#define REG_FileAccessOffsetIncInq							(0x00010B34)
#define REG_FileAccessLengthMinInq							(0x00010B38)
#define REG_FileAccessLengthMaxInq							(0x00010B3C)
#define REG_FileAccessLengthIncInq							(0x00010B40)
#define REG_EventSelectorInq								(0x00010C00)
#define REG_CtrlTransCtrlInq								(0x00010E00)
#define REG_ColorTransformationValueMaxInq					(0x00010E04)
#define REG_ColorTransformationValueMinInq					(0x00010E08)
#define REG_ColorTransformationValueUnitInq					(0x00010E0C)
#define REG_UCCIndexMinInq									(0x00012004)
#define REG_UCCIndexMaxInq									(0x00012008)
#define REG_UCCIndexIncInq									(0x0001200C)
#define REG_UCCValueIndexMinInq								(0x00012010)
#define REG_UCCValueIndexMaxInq								(0x00012014)
#define REG_UCCValueIndexIncInq								(0x00012018)


#define REG_DeviceScanType									(0x00030000)
#define REG_DeviceFirmwareVersion							(0x00030024)
#define REG_DeviceUptime									(0x00030048)
#define REG_BoardDeviceType									(0x0003004C)
#define REG_DeviceMaxThroughput								(0x0003006C)
#define REG_DeviceConnectionSpeed							(0x0003007C)
#define REG_DeviceConnectionStatus							(0x000300BC)
#define REG_DeviceLinkThroughputLimitMode					(0x00030100)
#define REG_DeviceLinkThroughputLimit						(0x00030110)
#define REG_DeviceLinkConnectionCount						(0x00030120)

// RW 4 Device Link Heartbeat Mode Register
#define REG_DeviceLinkHeartbeatMode							(0x00030130)
// [0-30]reserved
// [31]DeviceLinkHeartbeatMode

#define REG_DeviceLinkHeartbeatTimeout						(0x00030140)
#define REG_DeviceCommandTimeout							(0x00030150)

// W 4 Device Reset Register
#define REG_DeviceReset										(0x00030180)


#define REG_DeviceTemperature								(0x000301B0)

// W 4 Find Me Register
#define REG_FindMe											(0x000302B0)

// R 4 Sensor Channel Version Register
#define REG_SensorChannelVersion							(0x000302B4)

#define REG_DevicePJNumber									(0x000302CC)
#define REG_SensorWidth										(0x00030300)
#define REG_SensorHeight									(0x00030304)
#define REG_SensorTaps										(0x00030308)
#define REG_SensorDigitizationTaps							(0x0003030C)

// R 4 WidthMax Register
#define REG_WidthMax										(0x00030310)

// R 4 HeightMax Register
#define REG_HeightMax										(0x00030314)

#define REG_RegionMode										(0x00030318)
#define REG_RegionDestination								(0x00030320)
#define REG_Width											(0x00030360)
#define REG_Height											(0x000303A0)
#define REG_OffsetX											(0x000303E0)
#define REG_OffsetY											(0x00030420)
#define REG_LinePitch										(0x00030460)
#define REG_BinningHorizontal								(0x000304A0)
#define REG_BinningVertical									(0x000304E0)
#define REG_Decimation										(0x00030600)
#define REG_ReverseX										(0x00030608)
#define REG_ReverseY										(0x0003060C)
#define REG_PixelFormat										(0x00030610)
#define REG_PixelCoding										(0x00030660)
#define REG_PixelColorFilter								(0x00030668)
#define REG_TestPattern										(0x00030680)
#define REG_Deinterlacing									(0x000306C4)
#define REG_ImageCompressionMode							(0x000306C8)
#define REG_ImageCompressionQuality							(0x000306D0)
#define REG_FrameSpecInfo									(0x000306DC)
#define REG_ReverseScanDirection							(0x000306F4)
#define REG_BitRegionSelector								(0x00030724)

// RW 4 Acquisition Mode
#define REG_AcquisitionMode									(0x00030800)
// 0:SingleFrame  1:MultiFrame  2:Continuous

// RW 4 Acquisition Start Register. Starts the Acquisition of the device
#define REG_AcquisitionStart								(0x00030804)
// [0]		command value -> 1
// [1-31]reserved

// RW 4 Acquisition Stop Register. Stops the acquisition of the device at the end of the current frame.
#define REG_AcquisitionStop									(0x00030808)
// [0]		command value -> 0
// [1-31]reserved

// RW 4 Acquisition Frame Count. Number of frames to acquire in multi-frame acquisition mode.
#define REG_AcquisitionFrameCount							(0x00030814)

// RW 4 Number of frames to acquire for each FrameBurstStart trigger.
#define REG_AcquisitionBurstFrameCount						(0x00030818)

// RW 4 Controls the acquisition rate at which the frames are captured.
#define REG_AcquisitionFrameRate							(0x0003081C)
// [0-15] Enable 0:off 1:on  Acquisition Frame Rate Control Enable.
// [16-31] rate 

// R 4 Indicates the 'absolute' value of the maximum allowed acquisition frame rate. 
// The 'absolute' value is a float value that indicates the maximum allowed 
// acquisition frame rate in frames per second given the current settings for the 
// area of interest, exposure time, and bandwidth.
#define REG_ResultingFrameRate								(0x00030824)

//#define REG_AcquisitionLineRate								(0x0003082C)

// RW 4 Controls whether or not the selected trigger is active.
#define REG_TriggerMode										(0x00030840)

// RW 4 Selects the type of trigger to configure.
#define REG_TriggerSelector									(0x00030844)
// 0-12 : REG_TriggerSelectInq

// RW 4 Generates an internal trigger if Trigger Source is set to Software.
#define REG_TriggerSoftware									(0x00030880)

// RW 4 Specifies the internal signal or physical input line to use as the trigger source.
#define REG_TriggerSource									(0x000308C0)
// 0:Source0 1:Source1 2:Source2 3:Source3 4:SourceCounter0 7:SourceSw

// RW 4 Specifies the activation mode of the trigger.
#define REG_TriggerActivation								(0x00030900)
// 0: Rising Edge  1:Falling Edge

// RW 4 Specifies the delay (in us) to apply after the trigger reception before activating it.
#define REG_TriggerDelayAbsVal								(0x00030980)

// RW 4 Acquisition calibrate images.
#define REG_Calibrate										(0x00030A00)

// RW 4 Exposure Time for Calibrate brightness 1 image.
#define REG_CalibrateExpoTime0								(0x00030A04)

// RW 4 Exposure Time for Calibrate 22 serial images.
#define REG_CalibrateExpoTime1								(0x00030A08)

// RW 4 Acquisition Point Cloud file
#define REG_PointCloud										(0x00030A10)
// [0]		command value -> 1
// [1-31]reserved

// RW 4 Exposure Time for Point Cloud.
#define REG_PointCloudExpoTime								(0x00030A14)


// RW 4 Acquisition Depth Image file
#define REG_DepthImage										(0x00030A20)
// [0]		command value -> 1
// [1-31]reserved

// RW 4 Exposure Time for Depth Image.
#define REG_DepthImageExpoTime								(0x00030A24)

// RW 4 Acquisition HDR image.
#define REG_HDRImage										(0x00030A30)

// RW 4 Exposure Time for HDRI.
#define REG_HDRIExposureTime0								(0x00030A34)

#define REG_HDRIExposureTime1								(0x00030A38)



// RW 4 Sets the operation mode of the Exposure (or shutter).
#define REG_ExposureMode									(0x00030B00)
// 0:Timed  1:Trigger Width

// RW 4 Exposure time in us when Exposure Mode is Timed.
#define REG_ExposureTime									(0x00030B04)

// RW 4 Sets the automatic exposure mode when Exposure Mode is Timed.
#define REG_ExposureAuto									(0x00030B08)
// 0:off 1:Once 2:Continuous

#define REG_ResultingLineRate								(0x00030B0C)
#define REG_AcquisitionStatus								(0x00030B10)
#define REG_FrameTimeoutEnable								(0x00030B14)
#define REG_FrameTimeoutTime								(0x00030B18)
#define REG_TriggerCacheEnable								(0x00030B1C)
#define REG_ExposureTimeMode								(0x00030B20)
#define REG_SensorShutterMode								(0x00030B28)
#define REG_TriggerDelayRisingAbsVal						(0x00030B38)
#define REG_TGCRC											(0x00030B38)
#define REG_TriggerDelayFallingAbsVal						(0x00030B3C)
#define REG_TGDeviceType									(0x00030B3C)
#define REG_TriggerPartialClose								(0x00030B5C)
#define REG_PartialFrameDiscard								(0x00030B60)

#define REG_LineMode										(0x00030D00)

#define REG_LineInverter									(0x00030D80)
#define REG_LineStrobe										(0x00030D84)
#define REG_LineStatusAll									(0x00030D88)
#define REG_LineSource										(0x00030DA0)
#define REG_UserOutputValue									(0x00030EA0)

#define REG_LineDebouncerTime								(0x00030F20)

#define REG_StrobeLineDuration								(0x00030FA0)
#define REG_StrobeLineDelay									(0x00031020)
#define REG_StrobeLinePreDelay								(0x000310A0)
#define REG_SerialPortBaudrate								(0x000310A4)
#define REG_LineTermination									(0x00031120)
#define REG_HardwareTriggerSource							(0x00031124)
#define REG_GainAbsVal										(0x00031200)
#define REG_GainCtrl										(0x00031220)
#define REG_DigitalShift									(0x00031244)
#define REG_BlackLevelCtrl									(0x0003124C)
#define REG_BlackLevel										(0x00031250)
#define REG_BlackLevelAuto									(0x00031270)
#define REG_BalanceRatio									(0x000312C0)
#define REG_BalanceWhiteAuto								(0x000312E0)
#define REG_BalanceColorTempMode							(0x000312E4)

#define REG_GammaAbsVal										(0x00031300)

#define REG_GammaCtrl										(0x00031308)
#define REG_SharpnessVal									(0x00031310)
#define REG_SharpnessCtrl									(0x00031314)
#define REG_HueAbsVal										(0x00031320)
#define REG_HueCtrl											(0x00031324)
#define REG_SaturationAbsVal								(0x00031330)
#define REG_SaturationCtrl									(0x00031334)
#define REG_Brightness										(0x0003133C)
#define REG_AutoAOIWidth									(0x00031360)
#define REG_AutoAOIHeight									(0x00031380)
#define REG_AutoAOIOffsetX									(0x000313A0)
#define REG_AutoAOIOffsetY									(0x000313C0)
#define REG_AutoAOIUsage									(0x000313F0)
#define REG_AutoExposureTimeLowerLimit						(0x00031400)
#define REG_AutoExposureTimeUpperLimit						(0x00031404)
#define REG_AutoGainLowerLimit								(0x00031408)
#define REG_AutoGainUpperLimit								(0x0003140C)

// RW 4 Sets the priority of the Exposure and Gain.
#define REG_GainShutPrior									(0x00031430)
// 0: Shut  1:Gain

#define REG_HDREnable										(0x00031450)
#define REG_HDRShuter										(0x00031454)
#define REG_HDRGain											(0x00031474)
#define REG_NUCEnable										(0x000314A0)
#define REG_ADCGainEnable									(0x000314A4)
#define REG_DigitalNoiseReductionMode						(0x000314A8)
#define REG_TemporalNoiseReduction							(0x000314AC)
#define REG_AirspaceNoiseReduction							(0x000314B0)
#define REG_NoiseReduction									(0x000314B4)
#define REG_ChannelCorrectMode								(0x000314B8)
#define REG_FanOpenThreshold								(0x000314BC)
#define REG_TZCoef0											(0x00031500)
#define REG_TZCoef1											(0x00031504)
#define REG_TZCoef2											(0x00031508)
#define REG_TZIndex											(0x0003150C)
#define REG_TZPureOpen										(0x00031510)
#define REG_TZDenoiseOpen									(0x00031514)
#define REG_TZDenoiseCoef									(0x00031518)
#define REG_PreampGain										(0x00031520)
#define REG_LUTEnable										(0x00031700)

#define REG_UserSetCurrent									(0x00031800)

#define REG_UserSetSelector									(0x00031804)
#define REG_UserSetLoad										(0x00031808)
#define REG_UserSetSave										(0x0003180C)
#define REG_UserSetDefaultSelector							(0x00031810)

#define REG_PayloadSize										(0x00031900)

#define REG_PacketUnorderSupport							(0x00031904)
#define REG_CounterEventSource								(0x00032000)

#define REG_CounterResetSource								(0x00032100)

#define REG_CounterReset									(0x00032200)
#define REG_CounterValue									(0x00032300)
#define REG_CounterCurrentValue								(0x00032380)
#define REG_CounterValueAtReset								(0x00032400)
#define REG_EncoderSourceA									(0x00033100)
#define REG_EncoderSourceB									(0x00033110)
#define REG_EncoderOutputMode								(0x00033120)
#define REG_EncoderCounterMode								(0x00033130)
#define REG_EncoderCounter									(0x00033140)
#define REG_EncoderCounterReset								(0x00033150)
#define REG_EncoderReverseCounterReset						(0x00033160)
#define REG_EncoderMaxReverseCounter						(0x00033170)
#define REG_EncoderCounterMax								(0x00033180)
#define REG_InputSource										(0x00033300)
#define REG_SignalAlignment									(0x00033304)
#define REG_PreDivider										(0x00033308)
#define REG_Multiplier										(0x0003330C)
#define REG_PostDivider										(0x00033310)
#define REG_PreventOvertrigger								(0x00033314)
#define REG_ShadingEnable									(0x00033500)
#define REG_DefaultShadingSetSelector						(0x00033510)
#define REG_ShadingSetSelector								(0x00033520)
#define REG_ActivateShading									(0x00033530)
#define REG_CreateShadingSet								(0x00033540)
#define REG_ShadingStatus									(0x00033550)
#define REG_FileOperationExecute							(0x00033700)
#define REG_FileOpenMode									(0x000337A0)
#define REG_FileAccessOffset								(0x000337C0)
#define REG_FileAccessLength								(0x00033860)
#define REG_FileOperationStatus								(0x00033900)
#define REG_FileOperationResult								(0x000339A0)
#define REG_FileSize										(0x000339C0)
#define REG_EventNotification								(0x00033B00)
#define REG_EventSelector									(0x00033B04)
#define REG_ChunkModeActive									(0x00033C00)
#define REG_ChunkEnable										(0x00033C04)
#define REG_HardwareTriggerActivation						(0x00033E00)
#define REG_LineTriggerSoftware								(0x00033E80)
#define REG_ColorTransformationEnable						(0x00036004)
#define REG_ColorTransformationValue						(0x00036014)
#define REG_DPCValue										(0x00070000)
#define REG_FPNCValue										(0x00080000)
#define REG_PRNUCValue										(0x00088000)
#define REG_ClampLevelValueALL								(0x000A0000)
#define REG_DCCValue										(0x000A1000)
#define REG_DLCValue										(0x000A3000)
#define REG_LUTValue										(0x000B0000)
#define REG_DPCSave											(0x000E1000)
#define REG_DPCEnable										(0x000E1004)
#define REG_FPNCSave										(0x000E1008)
#define REG_FPNCEnable										(0x000E100C)
#define REG_PRNUCSave										(0x000E1010)
#define REG_PRNUCEnable										(0x000E1014)
#define REG_FPNCXSave										(0x000E1018)
#define REG_UserDataSave									(0x000E1040)
#define REG_SHD												(0x000E104C)
#define REG_SHP												(0x000E106C)
#define REG_ClampLevel										(0x000E108C)
#define REG_VESValue										(0x000E10AC)
#define REG_ClampLevelSave									(0x000E10B0)
#define REG_UCCStartState									(0x000E10B4)
#define REG_UCCValueSave									(0x000E10B8)
#define REG_EGreyEnable										(0x000E10BC)
#define REG_EBlackEnable									(0x000E10C0)
#define REG_CorrectVersionSelector							(0x000E10C8)
#define REG_FPNCV2TableSelector								(0x000E10CC)
#define REG_DCCSave											(0x000E10D0)
#define REG_DCCEnable										(0x000E10D4)
#define REG_PRNUCV2TableSelector							(0x000E10D8)
#define REG_DLCSave											(0x000E10E8)
#define REG_DLCEnable										(0x000E10EC)
#define REG_FPNCXValue										(0x00300000)
#define REG_UserDataValue									(0x00400000)
#define REG_FileAccessBuffer								(0x00500000)
#define REG_UCCValue										(0x00505000)



//O R 536(max) Start of XML file ... size 1M bytes  [0x100000-0x1FFFFF]
#define REG_StartOfXmlFile									(0x00100000)



#ifdef __cplusplus
}
#endif

#endif  /* __GVCP_REGISTER_H__ */


