#ifndef ANT_H
#define ANT_H

#define POWER_MSG_ID         	0x10
#define TORQUE_AT_WHEEL_MSG_ID	0x11
#define TORQUE_AT_CRANK_MSG_ID	0x12

void ANT_ResetSystem(void);
void ANT_AssignChannelEventFunction(int channel, void func(void *data, unsigned char *message), void *data);
void ANT_UnassignChannel(const unsigned char channel);
void ANT_AssignChannel(const unsigned char channel, const unsigned char type, const unsigned char network);
void ANT_SetChannelSearchTimeout(const unsigned char channel, const unsigned char search_timeout);
void ANT_RequestMessage(const unsigned char channel, const unsigned char request);
void ANT_SetChannelID(const unsigned char channel, const unsigned short device, const unsigned char deviceType, const unsigned char txType);
void ANT_SetChannelPeriod(const unsigned char channel, const unsigned short period);
void ANT_SetChannelFreq(const unsigned char channel, const unsigned char frequency);
void ANT_SetNetworkKey(const unsigned char net, const unsigned char *key);
void ANT_SetAutoCalibrate(const unsigned char channel, const int autozero);
void ANT_RequestCalibrate(const unsigned char channel);

void ANT_RequestData(const unsigned char channel, 
		     const unsigned int address,
		     const unsigned char length);

void ANT_SetConfigData(const unsigned char channel, 
		       const unsigned int address,
		       const unsigned char length,
		       char *config_data);

void ANT_Unlock(const unsigned char channel);
void ANT_Open(const unsigned char channel);
void ANT_Close(const unsigned char channel);
void ANT_ReceiveByte(unsigned char byte);

#define ANT_MAX_DATA_MESSAGE_SIZE	8

/*
 * ANT Messages
 */
#define ANT_UNASSIGN_CHANNEL		0x41
#define ANT_ASSIGN_CHANNEL		0x42
#define ANT_CHANNEL_ID			0x51
#define ANT_CHANNEL_PERIOD		0x43
#define ANT_SEARCH_TIMEOUT		0x44
#define ANT_CHANNEL_FREQUENCY		0x45
#define ANT_SET_NETWORK			0x46
#define ANT_TX_POWER			0x47
#define ANT_ID_LIST_ADD			0x59
#define ANT_ID_LIST_CONFIG		0x5A
#define ANT_CHANNEL_TX_POWER		0x60
#define ANT_LP_SEARCH_TIMEOUT		0x63
#define ANT_SET_SERIAL_NUMBER		0x65
#define ANT_ENABLE_EXT_MSGS		0x66
#define ANT_ENABLE_LED			0x68
#define ANT_SYSTEM_RESET		0x4A
#define ANT_OPEN_CHANNEL		0x4B
#define ANT_CLOSE_CHANNEL		0x4C
#define ANT_OPEN_RX_SCAN_CH		0x5B
#define ANT_REQ_MESSAGE			0x4D
#define ANT_BROADCAST_DATA		0x4E
#define ANT_ACK_DATA			0x4F
#define ANT_BURST_DATA			0x50
#define ANT_CHANNEL_EVENT		0x40
#define ANT_CHANNEL_STATUS		0x52
#define ANT_CHANNEL_ID			0x51
#define ANT_VERSION			0x3E
#define ANT_CAPABILITIES		0x54
#define ANT_SERIAL_NUMBER		0x61
#define ANT_CW_INIT			0x53
#define ANT_CW_TEST			0x48

/*
 * ANT message structure.
 */
#define ANT_OFFSET_SYNC			0
#define ANT_OFFSET_LENGTH		1
#define ANT_OFFSET_ID			2
#define ANT_OFFSET_DATA			3
#define ANT_OFFSET_CHANNEL_NUMBER       3
#define ANT_OFFSET_MESSAGE_ID           4
#define ANT_OFFSET_MESSAGE_CODE         5

/*
 * Other ANT defines
 */
#define ANT_SYNC_BYTE		0xA4
#define ANT_MAX_LENGTH		9
#define ANT_KEY_LENGTH		8
#define ANT_MAX_BURST_DATA	8
#define ANT_MAX_MESSAGE_SIZE    12
#define ANT_MAX_CHANNELS	4

/*
 * Channel messages
 */ 

#define RESPONSE_NO_ERROR               0
#define EVENT_RX_SEARCH_TIMEOUT         1
#define EVENT_RX_FAIL                   2
#define EVENT_TX                        3
#define EVENT_TRANSFER_RX_FAILED        4
#define EVENT_TRANSFER_TX_COMPLETED     5
#define EVENT_TRANSFER_TX_FAILED        6
#define EVENT_CHANNEL_CLOSED            7
#define EVENT_RX_BROADCAST             10
#define EVENT_RX_ACKNOWLEDGED          11
#define EVENT_RX_BURST_PACKET          12
#define CHANNEL_IN_WRONG_STATE         21
#define CHANNEL_NOT_OPENED             22
#define CHANNEL_ID_NOT_SET             24
#define TRANSFER_IN_PROGRESS           31
#define TRANSFER_SEQUENCE_NUMBER_ERROR 32
#define INVALID_MESSAGE                40
#define INVALID_NETWORK_NUMBER         41

// ANT+sport
#define ANT_SPORT_HR_PERIOD 8070
#define ANT_SPORT_POWER_PERIOD 8182
#define ANT_SPORT_SPEED_PERIOD 8118
#define ANT_SPORT_CADENCE_PERIOD 8102
#define ANT_SPORT_SandC_PERIOD 8086
#define ANT_FAST_QUARQ_PERIOD (8182/16)
#define ANT_QUARQ_PERIOD (8182*4)

#define ANT_SPORT_HR_TYPE 0x78
#define ANT_SPORT_POWER_TYPE 11
#define ANT_SPORT_SPEED_TYPE 0x7B
#define ANT_SPORT_CADENCE_TYPE 0x7A
#define ANT_SPORT_SandC_TYPE 0x79
#define ANT_FAST_QUARQ_TYPE_WAS 11 // before release 1.8
#define ANT_FAST_QUARQ_TYPE 0x60 
#define ANT_QUARQ_TYPE 0x60 

#define ANT_SPORT_FREQUENCY 57
#define ANT_FAST_QUARQ_FREQUENCY 61
#define ANT_QUARQ_FREQUENCY 61 

#ifdef DEBUG 
#define ANT_ERROR(format, args...)  fprintf(stderr,format, ##args)

#else
#define ANT_ERROR(format, args...) ;
#endif 


#define ANT_SPORT_CALIBRATION_MESSAGE                 0x01
#define ANT_SPORT_AUTOZERO_OFF                        0x00
#define ANT_SPORT_AUTOZERO_ON                         0x01
#define ANT_SPORT_CALIBRATION_REQUEST_MANUALZERO      0xAA
#define ANT_SPORT_CALIBRATION_REQUEST_AUTOZERO_CONFIG 0xAB


#endif // ANT_H


