#!/usr/bin/python

# These are ant messages from the public Ant docs

messages="""
* assign_channel    0x42,channel,channel_type,network_number
* unassign_channel  0x41,channel
* open_channel      0x4b,channel
* channel_id        0x51,channel,uint16_le:device_number,device_type_id,transmission_type
* burst_message     0x50,chan_seq,data0,data1,data2,data3,data4,data5,data6,data7
* burst_message7    0x50,chan_seq,data0,data1,data2,data3,data4,data5,data6
* burst_message6    0x50,chan_seq,data0,data1,data2,data3,data4,data5
* burst_message5    0x50,chan_seq,data0,data1,data2,data3,data4
* burst_message4    0x50,chan_seq,data0,data1,data2,data3
* burst_message3    0x50,chan_seq,data0,data1,data2
* burst_message2    0x50,chan_seq,data0,data1
* burst_message1    0x50,chan_seq,data0
* burst_message0    0x50,chan_seq
* channel_period    0x43,channel,uint16_le:period
* search_timeout    0x44,channel,search_timeout
* channel_frequency 0x45,channel,rf_frequency
* set_network       0x46,channel,key0,key1,key2,key3,key4,key5,key6,key7
* transmit_power    0x47,0,tx_power
* reset_system      0x4a,None
* request_message   0x4d,channel,message_requested
* close_channel     0x4c,channel
* response_no_error           0x40,channel,message_id,0x00 Returned on a successful operation
* response_assign_channel_ok  0x40,channel,0x42,0x00
* response_channel_unassign_ok  0x40,channel,0x41,0x00
* response_open_channel_ok  0x40,channel,0x4b,0x00
* response_channel_id_ok  0x40,channel,0x51,0x00
* response_channel_period_ok  0x40,channel,0x43,0x00
* response_channel_frequency_ok  0x40,channel,0x45,0x00
* response_set_network_ok  0x40,channel,0x46,0x00
* response_transmit_power_ok  0x40,channel,0x47,0x00
* response_close_channel_ok  0x40,channel,0x4c,0x00
* response_search_timeout_ok  0x40,channel,0x44,0x00
* event_rx_search_timeout     0x40,channel,message_id,0x01 A receive channel has timed out on searching. The search is terminated, and the channel has been automatically closed. In order to restart the search the Open Channel message must be sent again.
* event_rx_fail               0x40,channel,message_id,0x02 A receive channel missed a message which it was expecting. This would happen when a receiver is tracking a transmitter and is expecting a message at the set message rate.
* event_tx                    0x40,channel,message_id,0x03 A Broadcast message has been transmitted successfully. This event should be used to send the next message for transmission to the ANT device if the node is setup as a transmitter.
* event_transfer_rx_failed    0x40,channel,message_id,0x04 A receive transfer has failed. This occurs when a Burst Transfer Message was incorrectly received.
* event_transfer_tx_completed 0x40,channel,message_id,0x05 An Acknowledged Data message or a Burst Transfer sequence has been completed successfully. When transmitting Acknowledged Data or Burst Transfer, there is no EVENT_TX message.
* event_transfer_tx_failed    0x40,channel,message_id,0x06 An Acknowledged Data message or a Burst Transfer Message has been initiated and the transmission has failed to complete successfully
* event_channel_closed        0x40,channel,message_id,0x07 The channel has been successfully closed. When the Host sends a message to close a channel, it first receives a RESPONSE_NO_ERROR to indicate that the message was successfully received by ANT. This event is the actual indication of the closure of the channel. So, the Host must use this event message instead of the RESPONSE_NO_ERROR message to let a channel state machine continue.
* event_rx_fail_go_to_search  0x40,channel,message_id,0x08 The channel has dropped to search after missing too many messages.
* event_channel_collision     0x40,channel,message_id,0x09 Two channels have drifted into each other and overlapped in time on the device causing one channel to be blocked. 
* event_transfer_tx_start   0x40,channel,message_id,0x0A A burst transfer has begun
# event_rx_broadcast          0x40,channel,message_id,0x0A ANT Library special event (Not in serial interface). This event is sent to denote a valid broadcast data message has been received by the ANT library, and the data is waiting in the appropriate channel receive buffer.
* event_rx_acknowledged       0x40,channel,message_id,0x0B ANT Library special event (Not in serial interface). This event is sent to denote that a valid acknowledged data message has been received by the ANT library, and the data is waiting in the appropriate channel receive buffer.
* event_rx_burst_packet       0x40,channel,message_id,0x0C ANT Library special event (Not in serial interface). It indicates the successful reception of a burst packet in a Burst Transfer sequence.
* channel_in_wrong_state      0x40,channel,message_id,0x15 Returned on attempt to perform an action on a channel that is not valid for the channel's state
* channel_not_opened          0x40,channel,message_id,0x16 Attempt to transmit data on an unopened channel
* channel_id_not_set          0x40,channel,message_id,0x18 Returned on attempt to open a channel before setting a valid ID
* transfer_in_progress        0x40,channel,message_id,31 Returned on an attempt to communicate on a channel with a transmit transfer in progress.
* channel_status              0x52,channel,status
* cw_init                     0x53,None Initialize CW test mode
* cw_test                     0x48,None,power,freq Begin CW test at specified power and frequency
* capabilities                0x54,max_channels,max_networks,standard_options,advanced_options
* capabilities_extended       0x54,max_channels,max_networks,standard_options,advanced_options,advanced_options_2,max_data_channels
* ant_version                 0x3e,data0,data1,data2,data3,data4,data5,data6,data7,data8   9-byte null terminated version string
* ant_version_long                 0x3e,data0,data1,data2,data3,data4,data5,data6,data7,data8,data9,data10,data11,data12   13-byte null terminated version string
* transfer_seq_number_error   0x40,channel,message_id,0x20 Returned when sequence number is out of order on a Burst Transfer
* invalid_message             0x40,channel,message_id,0x28 Returned when message has invalid parameters
* invalid_network_number      0x40,channel,message_id,0x29 Returned when an invalid network number is provided. As mentioned earlier, valid network numbers are between 0 and MAX_NET-1.
* channel_response            0x40,channel,message_id,message_code Generic requested data response function
* extended_broadcast_data     0x5d,channel,uint16le:device_number,device_type,transmission_type,data0,data1,data2,data3,data4,data5,data6,data7
* extended_ack_data           0x5e,channel,uint16le:device_number,device_type,transmission_type,data0,data1,data2,data3,data4,data5,data6,data7
* extended_burst_data         0x5f,channel,uint16le:device_number,device_type,transmission_type,data0,data1,data2,data3,data4,data5,data6,data7
* startup_message             0x6f,start_message
"""

message_calculations="""
burst_message   uint8  channel=chan_seq&0x1f
burst_message   list   burst_data=[data0,data1,data2,data3,data4,data5,data6,data7]
burst_message   uint8  seq=chan_seq>>5

"""

import message_set
messages=message_set.MessageSet(messages,message_calculations)
