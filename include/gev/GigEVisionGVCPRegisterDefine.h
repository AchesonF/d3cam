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

// M R 4 Information about device mode of operation.
#define REG_DeviceMode										(0x0004)

//(Network interface #0) M R 4 The two most-significant bytes of this area are reserved
//and will return 0. Upper 2 bytes of the MAC address are
//stored in the lower 2 significant bytes of this register.
#define REG_DeviceMACAddressHigh0							(0x0008)

//(Network interface #0) M R 4 Lower 4 bytes of the MAC address
#define REG_DeviceMACAddressLow0							(0x000C)

//(Network interface #0)M R 4 Supported IP Configuration and PAUSE schemes.
//Bits can be OR-ed. All other bits are reserved and set to0. DHCP and LLA bits must be on.
#define REG_NetworkInterfaceCapability0						(0x0010)

//(Network interface #0)M R/W 4 Activated IP Configuration and PAUSE schemes.
//Bits can be OR-ed. LLA is always activated and is readonly.PAUSE settings might be hard-coded.
#define REG_NetworkInterfaceConfiguration0					(0x0014)


//0x0018 Reserved - - -

//(Networkinterface #0)M R 4 Current IP address of this device on its first interface.
#define REG_CurrentIPAddress0								(0x0024)

//0x0028 Reserved - - -

//(Networkinterface #0)M R 4 Current subnet mask used by this device on its first interface.
#define REG_CurrentSubnetMask0								(0x0034)

//0x0038 Reserved - - -

//(Network interface #0)M R 4 Current default gateway used by this device on its first interface.
#define REG_CurrentDefaultGateway0							(0x0044)

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

//0x0604 Reserved - - -

//(Networkinterface #0)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentIPAddress								(0x064C)

//0x0650 Reserved - - -

//(Network interface #0)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentSubnetMask0							(0x065C)

//0x0660 Reserved - - -

//(Network interface #0)O R/W 4 Only available if Persistent IP is supported by the device.
#define REG_PersistentDefaultGateway0						(0x066C)

//(Network interface#0)O R 4 32-bit value indicating current Ethernet link speed in Mbits per second on the first interface.
#define REG_LinkSpeed0										(0x0670)

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

//M R 4 Indicates the number of stream channels supported by this device. It can take any value from 0 to 512.
#define REG_NumberofStreamChannels							(0x0904)

//O R 4 Indicates the number of separate action signals supported by this device. It can take any value ranging from 0 to 128.
#define REG_NumberofActionSignals							(0x0908)

//O W 4 Device key to check the validity of action commands.
#define REG_ActionDeviceKey									(0x090C)

//M R 4 Indicates the number of physical links that are currently active.
#define REG_NumberofActiveLinks								(0x0910)

//0x0914 Reserved - - -

//M R 4 Indicates the capabilities of the stream channels. It lists which of the
//non-mandatory stream channel features are supported.
#define REG_GVSPCapability									(0x092C)

//M R 4 Indicates the capabilities of the message channel. It lists
// which of the non-mandatory message channel features are supported.
#define REG_MessageChannelCapability						(0x0930)

//M R 4 This is a capability register indicating which one of the
//non-mandatory GVCP features are supported by this device.
#define REG_GVCPCapability									(0x0934)

//M R/W 4 In msec, default is 3000 msec. Internally, the heartbeat is rounded according to the clock used for heartbeat. The
//heartbeat timeout shall have a minimum precision of 100ms. The minimal value is 500 ms.
#define REG_HeartbeatTimeout								(0x0938)

//High O R 4 64-bit value indicating the number of timestamp clock tick in 1 second. This register holds the most significant
//bytes. Timestamp tick frequency is 0 if timestamp is not supported.
#define REG_TimestampTickFrequencyHigh						(0x093C)

//Low O R 4 64-bit value indicating the number of timestamp clock
//tick in 1 second. This register holds the least significant bytes.
//Timestamp tick frequency is 0 if timestamp is not supported.
#define REG_TimestampTickFrequencyLow						(0x0940)

//O W 4 Used to latch the current timestamp value. No need to clear to 0.
#define REG_TimestampControl								(0x0944)

// - High O R 4 Latched value of the timestamp (most significant bytes)
#define REG_TimestampValueHigh								(0x0948)

// - Low O R 4 Latched value of the timestamp (least significant bytes)
#define REG_TimestampValueLow								(0x094C)

//O R/(W) 4 Maximum randomized delay in ms to acknowledge a DISCOVERY_CMD.
#define REG_DiscoveryACKDelay								(0x0950)

// O R/W 4 Configuration of GVCP optional features
#define REG_GVCPConfiguration								(0x0954)

//M R 4 Pending Timeout to report the longest GVCP command execution time before issuing a PENDING_ACK. If
//PENDING_ACK is not supported, then this is the worstcase execution time before command completion.
#define REG_PendingTimeout									(0x0958)
//O W 4 Key to authenticate primary application switchover requests.
#define REG_ControlSwitchoverKey							(0x095C)

//O R/W 4 Configuration of the optional GVSP features.
#define REG_GVSPConfiguration								(0x0960)

// M R 4 Indicates the physical link configuration supported by thisdevice.
#define REG_PhysicalLinkConfigurationCapability				(0x0964)

//M R/(W) 4 Indicates the currently active physical link configuration
#define REG_PhysicalLinkConfiguration						(0x0968)

//O R 4 Reports the state of the IEEE 1588 clock
#define REG_IEEE1588Status									(0x096C)

//O R 4 Indicates the number of Scheduled Action Commands that
//can be queued (size of the queue).
#define REG_ScheduledActionCommandQueueSize					(0x0970)

//0x0974 Reserved - - -

//(CCP)M R/W 4 Control Channel Privilege register.
#define REG_ControlChannelPrivilege							(0x0A00)

//O R 4 UDP source port of the control channel of the primary application.
#define REG_PrimaryApplicationPort							(0x0A04)

//0x0A08 Reserved - - -

//O R 4 Source IP address of the control channel of the primary application.
#define REG_PrimaryApplicationIPAddress						(0x0A14)

//0x0A18 Reserved - - -

//(MCP) O R/W 4 Message Channel Port register.
#define REG_MessageChannelPort								(0x0B00)

//0x0B04 Reserved - - -

//Address (MCDA)O R/W 4 Message Channel Destination Address register.
#define REG_MessageChannelDestination						(0x0B10)

//(MCTT)O R/W 4 Message Channel Transmission Timeout in ms.
#define REG_MessageChannelTransmissionTimeout				(0x0B14)

//(MCRC)O R/W 4 Message Channel Retry Count.
#define REG_MessageChannelRetryCount						(0x0B18)

//(MCSP)O R 4 Message Channel Source Port.
#define REG_MessageChannelSourcePort						(0x0B1C)

//0x0B20 Reserved - - -

//(SCP0) M1 R/W 4 First Stream Channel Port register.
#define REG_StreamChannelPort0								(0x0D00)

//(SCPS0)M1 R/W 4 First Stream Channel Packet Size register.
#define REG_StreamChannelPacketSize0						(0x0D04)

//(SCPD0)M2 R/W 4 First Stream Channel Packet Delay register.
#define REG_StreamChannelPacketDelay0						(0x0D08)

//0x0D0C Reserved - - -

//(SCDA0)M1 R/W 4 First Stream Channel Destination Address register.
#define REG_StreamChannelDestinationAddress0				(0x0D18)

//(SCSP0)O R 4 First Stream Channel Source Port register.
#define REG_StreamChannelSourcePort0						(0x0D1C)

//(SCC0)O R 4 First Stream Channel Capability register.
#define REG_StreamChannelCapability0						(0x0D20)

//(SCCFG0)O R/W 4 First Stream Channel Configuration register.
#define REG_StreamChannelConfiguration0						(0x0D24)

//(SCZ0) O R 4 First Stream Channel Zone register (multi-zone image payload type only).
#define REG_StreamChannelZone0								(0x0D28)

//(SCZD0)O R 4 First Stream Channel Zone Direction register (multi-zone image payload type only).
#define REG_StreamChannelZoneDirection0						(0x0D2C)

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

//O R/W 4 First action signal group mask
#define REG_ActionGroupMask0								(0x9804)

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

#ifdef __cplusplus
}
#endif

#endif  /* __GVCP_REGISTER_H__ */


