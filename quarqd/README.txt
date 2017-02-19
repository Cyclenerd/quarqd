quarqd

  Purpose: 
  
    quarqd is a daemon for communicating with an Ant device and
    reading Ant+Sport data.  Data is communicated over a network socket.

    Source code is available to ANT+ members at the forums over at
    thisisant.com
    
  Usage:
    
    quarqd may be run by itself.  It does not detach from the terminal.

    Control files:     

       quarqd can take some information from the file
       $HOME/.quarqd_config, e.g.:

          QUARQD_PORT=8168    # the port quarqd should listen for 
	                        connections on
          ANT_TTY=/dev/ttyS0  # the device name of the Ant serial port 
          ANT_BAUDRATE=115200 # the baudrate of the Ant device
	  QUARQD_DEBUG=1      # debug is a bitmask.  
	  		      # 1 -> print runtime error information
			      # 2 -> print Ant connection status changes
			      # 4 -> (quarq builds only) print all Ant messages 
			      # (default is 1)

       Environment variables of the same names may be used, overriding the
       settings in $HOME/.quarqd_config

    Getting data:

       1) Connect to $QUARQD_PORT.  Data is streamed out as XML.

  Implementation:

     Single thread.  All data input comes from a select loop.
     
     Data output is buffered, and the messages are (hopefully) short
     enough that I/O doesn't block.

     The part of the code that generates messages comes from the
     Python script quarqd_messages.py, which generates the messages
     with the help of ant_messages.py.

   About XML messages:
 
        <$xml_message id=$ID time=$TIME $xml_message_dataname=$value 
             [$xml_message_dataname=$value [...]] />

     $ID is the Ant device_number for the sensor followed by:
         'h' (heart rate),
	 'p' (power sensor),
	 's' (speed sensor),
	 'c' (cadence sensor),
	 'd' (dual speed+cadence sensor)

     $TIME is seconds past the epoch

     For values of $xml_message and $xml_message_dataname, see 
     message_format.txt     

     Out-of-band values:

        <SensorFound id=$ID type=$SENSORTYPE [paired]>
          A new sensor has been found.  if paired, then the device was
          found through the pairing process. 

        <SensorStale id=$ID type=$SENSORTYPE>
          The data from the sensor hasn't been updated for the stale
	  timeout.
          (time to blank the display)

        <SensorDropped id=$ID type=$SENSORTYPE>
	  The sensor has not been heard from for the drop timeout.

	<SensorLost  id=$ID type=$SENSORTYPE>
          The sensor is missing.  To restart scan, resend
          X-set-channel for the channel.

	<Timeout type=$TIMEOUT_TYPE value=$V>
  	  Reports timeout.  

	  $TIMEOUT_TYPE is one of:
           
 	  - blanking: timeout before a "stale data" message is reported.

          - drop: timeout before a drop is reported.
 
	  - scan: initial scan timeout.  Lower values for quicker
                initial connection with sensors.

	  - lost: timeout in seconds before giving up on a sensor.
 	        Set to "0" to reserve a channel for a specific sensor
 	        always.
	 
	<Error> message </Error>

   Commands:
     X-set-channel: $ID[,channel_number]

        - where $ID is the Ant device_number of the sensor, followed
          by the one-character sensor type.  "0" for any device_number
          of the specified type.

	  Examples: X-set-channel: 17p  (power meter #17)
	            X-set-channel: 0h,1   (scan for heart rate sensor)

	- channel_number is the Ant channel to use for the
	  device. Four channels, numbered 0-3. The previous occupant
	  of the channel will be booted if necessary.  Leave blank for
	  next-available-channel.
	
     X-unset-channel: $ID
        channel is closed and removed.

     X-set-timeout: timeout_type=$TIMEOUT_TYPE
        value is in seconds, floating-point
	<Timeout> message is returned

     X-list-channels: 
        Will cause quarqd to output SensorFound messages for all the
        channels it is connected to.

     X-pair-device: $ID[,channel_number] Looks for a device with the
        pairing bit set.  device_number in $ID should be 0.

     X-calibrate: $ID
        Sends the calibration message to the power channel $ID

     X-set-test-mode: $ID
        Sets the test mode for $ID (must be a CinQo meter)

     X-unset-test-mode: $ID
        Unsets the test mode for $ID 
     
     X-get-slope: $ID
        Retrieves the slope value for $ID

     X-set-slope: $ID $SLOPE
        Sets the slope value for $ID to $SLOPE

  Setup notes:

     OS X:

       Garmin ANT stick:

         With Garmin driver:

	   Use the following .quarqd_config:
             ANT_TTY=/dev/cu.ANTUSBStick.slabvcp
             ANT_BAUDRATE=115200 

	 With generic SiLabs driver:
  
           Uses cp210x driver.  Download from SiLabs:

           http://www.silabs.com/products/mcu/Pages/USBtoUARTBridgeVCPDrivers.aspx

           Then edit
           /System/Library/Extensions/SiLabsUSBDriver.kext/Contents/Info.plist
           and change the idProduct to 4100 and the idVendor to 4047.

           Then reboot.  Look for the device at /dev/cu.SLAB_USBtoUART

           set .quarqd_config:
             ANT_TTY=/dev/cu.SLAB_USBtoUART
             ANT_BAUDRATE=115200

           Thanks to Mark Liversedge for access to his computer for
           testing this.

       Sparkfun ANT stick:

         Uses FTDI driver. http://www.ftdichip.com/Drivers/VCP.htm

         set .quarqd_config:
           ANT_BAUDRATE=4800 # SparkFun baudrate
           ANT_TTY=/dev/cu.usbserial-A7005G7c # SparkFun FTDI driver

         Thanks to Justin Knotzke for access to his computer for
         testing this.
 
     Linux:  
         
       The required drivers are already in the kernel.  ANT_TTY will
       be /dev/ttyUSB0 or similar and ANT_BAUDRATE will be the same as
       on OS X.

     iPhone: 

       Quarqd can talk to the hardware serial port.  This was tested
       with an Ant module connected to the +3.3v, ground, and serial
       RX/TX lines of the dock connector.  Tested with a jailbroken
       iPod touch.  (When Apple-sanctioned ANT devices hit the market,
       it is likely that they will have an intervening microprocessor
       and quarqd won't be compatible.)
