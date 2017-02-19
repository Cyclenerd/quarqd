#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include "quarqd.h"
#include "ant_channel.h"
#include "channel_manager.h"
#include "configuration.h"


#ifndef DEBUG    
#define ant_message_print_debug(message) do { ; } while (0)
#else
#include "generated-debug.c"
#endif

const ant_sensor_type_t ant_sensor_types[] = {
  { .type=CHANNEL_TYPE_UNUSED,
    .descriptive_name="Unused",
    .suffix='?'
  },
  { .type=CHANNEL_TYPE_HR,
    .period=ANT_SPORT_HR_PERIOD,
    .device_id=ANT_SPORT_HR_TYPE,
    .frequency=ANT_SPORT_FREQUENCY,
    .network=ANT_SPORT_NETWORK_NUMBER,
    .descriptive_name="Heartrate",
    .suffix='h'
  },
  { .type=CHANNEL_TYPE_POWER,
    .period=ANT_SPORT_POWER_PERIOD,
    .device_id=ANT_SPORT_POWER_TYPE,
    .frequency=ANT_SPORT_FREQUENCY,
    .network=ANT_SPORT_NETWORK_NUMBER,
    .descriptive_name="Power",
    .suffix='p'
  },
  { .type=CHANNEL_TYPE_SPEED,
    .period=ANT_SPORT_SPEED_PERIOD,
    .device_id=ANT_SPORT_SPEED_TYPE,
    .frequency=ANT_SPORT_FREQUENCY,
    .network=ANT_SPORT_NETWORK_NUMBER,
    .descriptive_name="Speed",
    .suffix='s'
  },
  { .type=CHANNEL_TYPE_CADENCE,
    .period=ANT_SPORT_CADENCE_PERIOD,
    .device_id=ANT_SPORT_CADENCE_TYPE,
    .frequency=ANT_SPORT_FREQUENCY,
    .network=ANT_SPORT_NETWORK_NUMBER,
    .descriptive_name="Cadence",
    .suffix='c'
  },
  { .type=CHANNEL_TYPE_SandC,
    .period=ANT_SPORT_SandC_PERIOD,
    .device_id=ANT_SPORT_SandC_TYPE,
    .frequency=ANT_SPORT_FREQUENCY,
    .network=ANT_SPORT_NETWORK_NUMBER,
    .descriptive_name="Speed + Cadence",
    .suffix='d'
  },
  { .type=CHANNEL_TYPE_QUARQ, 
    .period=ANT_QUARQ_PERIOD, 
    .device_id=ANT_QUARQ_TYPE, 
    .frequency=ANT_QUARQ_FREQUENCY, 
    .network=DEFAULT_NETWORK_NUMBER, 
    .descriptive_name="Quarq Channel", 
    .suffix='Q' 
  },
  { .type=CHANNEL_TYPE_FAST_QUARQ,
    .period=ANT_FAST_QUARQ_PERIOD,
    .device_id=ANT_FAST_QUARQ_TYPE,
    .frequency=ANT_FAST_QUARQ_FREQUENCY,
    .network=DEFAULT_NETWORK_NUMBER,
    .descriptive_name="Fast Quarq",
    .suffix='q'
  },
  { .type=CHANNEL_TYPE_FAST_QUARQ_NEW,
    .period=ANT_FAST_QUARQ_PERIOD,
    .device_id=ANT_FAST_QUARQ_TYPE_WAS,
    .frequency=ANT_FAST_QUARQ_FREQUENCY,
    .network=DEFAULT_NETWORK_NUMBER,
    .descriptive_name="Fast Quarq New",
    .suffix='n'
  },
  { .type=CHANNEL_TYPE_GUARD
  }
};

static float timeout_blanking=2.0;  // time before reporting stale data, seconds
static float timeout_drop=2.0; // time before reporting dropped message
static float timeout_scan=10.0; // time to do initial scan
static float timeout_lost=30.0; // time to do more thorough scan 

void ant_channel_set_id(ant_channel_t *self) {
  if ((self->channel_type)==CHANNEL_TYPE_UNUSED) {
    strcpy(self->id, "none");
  } else {
    snprintf(self->id, 10, "%d%c", self->device_number, ant_sensor_types[self->channel_type].suffix);
  }
}

void ant_channel_init(ant_channel_t *self, int number, channel_manager_t *parent) {
  
  self->parent=parent;
  
  self->channel_type=CHANNEL_TYPE_UNUSED;
  self->channel_type_flags=0;
  self->number=number;
  

  self->is_cinqo=0;
  self->is_old_cinqo=0;
  self->control_channel=NULL;  
  self->manufacturer_id=0;
  self->product_id=0;
  self->product_version=0;

  self->device_number=0;
  self->channel_assigned=0;

  ant_channel_set_id(self);

  ANT_AssignChannelEventFunction(self->number, 
				 (void (*)(void *, unsigned char*)) ant_channel_receive_message,
				 (void *)self);

  self->state=ANT_UNASSIGN_CHANNEL;
  self->blanked=1;

  self->messages_received=0;
  self->messages_dropped=0;

  ant_channel_burst_init(self);  

  INITIALIZE_MESSAGES_INITIALIZATION(self->mi);
}

char * ant_channel_get_description(ant_channel_t *self) {  
  return ant_sensor_types[self->channel_type].descriptive_name;
}

int ant_channel_interpret_description(char *description) {
  const ant_sensor_type_t *st=ant_sensor_types;

  do {
    if (0==strcmp(st->descriptive_name,description)) 
      return st->type;
  } while (++st, st->type != CHANNEL_TYPE_GUARD);
  
  return -1;
}

int ant_channel_interpret_suffix(char c) {
  const ant_sensor_type_t *st=ant_sensor_types;

  do {
    if (st->suffix==c)
      return st->type;
  } while (++st, st->type != CHANNEL_TYPE_GUARD);
  
  return -1;
}

void ant_channel_open(ant_channel_t *self, int device_number, int channel_type) {
  self->channel_type=channel_type;
  self->channel_type_flags = CHANNEL_TYPE_QUICK_SEARCH ; 
  self->device_number=device_number;

  ant_channel_set_id(self);

  if (self->channel_assigned)
    ANT_UnassignChannel(self->number);        
  else 
    ant_channel_attempt_transition(self,ANT_UNASSIGN_CHANNEL);      
}


void ant_channel_close(ant_channel_t *self) {  
  ant_channel_lost_info(self);

    ANT_Close(self->number); 

}

double timestamp;

static inline double get_timestamp( void ) {
  struct timeval tv;
  gettimeofday(&tv, NULL); 
  return tv.tv_sec * 1.0 + tv.tv_usec * 1.0e-6;
}

void ant_channel_receive_message(ant_channel_t *self, unsigned char *ant_message) {

  unsigned char *message=ant_message+2;

#ifdef DEBUG
  int i;
  if (quarqd_config.debug_level & DEBUG_LEVEL_ANT_MESSAGES) {
    fprintf(stderr, "Got a message\n");
    for (i=0; i<ant_message[ANT_OFFSET_LENGTH]; i++) {
      fprintf(stderr, "0x%02x ",message[i]);
    }
    fprintf(stderr, "\n");

    ant_message_print_debug(message);
  }
#endif

  switch (message[0]) {
  case ANT_CHANNEL_EVENT:
    ant_channel_channel_event(self, ant_message);
    break;
  case ANT_BROADCAST_DATA:
    ant_channel_broadcast_event(self, ant_message);   
    break;
  case ANT_ACK_DATA:
    ant_channel_ack_event(self, ant_message);   
    break;
  case ANT_CHANNEL_ID:
    ant_channel_channel_id(self, ant_message);
    break;
  case ANT_BURST_DATA:
    ant_channel_burst_data(self, ant_message);
    break;
  default:
    DEBUG_ERRORS("?");
  exit(-9);

  }  

  if (get_timestamp() > self->blanking_timestamp + timeout_blanking) {
    if (!self->blanked) {
      self->blanked=1;
      ant_channel_stale_info(self);
    }
  } else self->blanked=0;
}


void ant_channel_channel_event(ant_channel_t *self, unsigned char *ant_message) { 
  unsigned char *message=ant_message+2;

  if (MESSAGE_IS_RESPONSE_NO_ERROR(message)) {
    ant_channel_attempt_transition(self,RESPONSE_NO_ERROR_MESSAGE_ID(message));
  } else if (MESSAGE_IS_EVENT_CHANNEL_CLOSED(message)) {
    ANT_UnassignChannel(self->number);
  } else if (MESSAGE_IS_EVENT_RX_SEARCH_TIMEOUT(message)) {
    // timeouts are normal for search channel, so don't send xml for those
    if (self->channel_type_flags & CHANNEL_TYPE_QUICK_SEARCH) {
      DEBUG_ANT_CONNECTION("Got timeout on channel %d.  Turning off search.\n", self->number);
      self->channel_type_flags &= ~CHANNEL_TYPE_QUICK_SEARCH;		
      self->channel_type_flags |= CHANNEL_TYPE_WAITING;

    } else {
      DEBUG_ANT_CONNECTION("Got timeout on channel %d.  search is off.\n", self->number);

      ant_channel_lost_info(self);

      self->channel_type=CHANNEL_TYPE_UNUSED;
      self->channel_type_flags=0;
      self->device_number=0;
      ant_channel_set_id(self);

      ANT_UnassignChannel(self->number);      
    } 

    DEBUG_ANT_CONNECTION("Rx search timeout on %d\n",self->number);
    channel_manager_start_waiting_search(self->parent);
  
  } else if (MESSAGE_IS_EVENT_RX_FAIL(message)) {
    //ant_message_print_debug(message);        

    self->messages_dropped++;

    double t=get_timestamp();

    //fprintf(stderr, "timediff %f\n",self->last_message_timestamp-t);

    if (t > (self->last_message_timestamp + timeout_drop)) {
      if (self->channel_type != CHANNEL_TYPE_UNUSED)
	ant_channel_drop_info(self);      
      // this is a hacky way to prevent the drop message from sending multiple times
      self->last_message_timestamp+=2*timeout_drop;
    }

  } else if (MESSAGE_IS_EVENT_RX_ACKNOWLEDGED(message)) {
    exit(-10);
  } else if (MESSAGE_IS_EVENT_TRANSFER_TX_COMPLETED(message)) {
    if (self->tx_ack_disposition) {
      self->tx_ack_disposition(self);
    }
  } else {
      ant_message_print_debug(message);    
    ; // default
  }
}

void ant_channel_send_cinqo_error(ant_channel_t *self) {
  XmlPrintf("<CinQoError id='%s' />\n", self->id); 
}

void ant_channel_send_cinqo_success(ant_channel_t *self) {
  XmlPrintf("<CinQoConnected id='%s' />\n", self->id); 
}

void ant_channel_check_cinqo(ant_channel_t *self) {
  int version_hi, version_lo, swab_version;

  version_hi=(self->product_version >> 8) &0xff;
  version_lo=(self->product_version & 0xff);

  swab_version=version_lo | (version_hi<<8);

  fprintf(stderr, "check cinqo\n");
  fprintf(stderr, "Product id %x\n",self->product_id);
  fprintf(stderr, "Manu id %x\n",self->manufacturer_id);

  fprintf(stderr, "Product version %x\n",self->product_version);
  fprintf(stderr, "Product version cvs rev#%d\n",swab_version);
  fprintf(stderr, "Product version %d | %d\n",version_hi, version_lo);

  if (!(self->mi.first_time_manufacturer || self->mi.first_time_product)) {
    if ((self->product_id == 1) && (self->manufacturer_id==7)) { 
      // we are a cinqo, were we aware of this?
      self->is_cinqo=1;

      // are we an old-version or new-version cinqo?
      self->is_old_cinqo = ((version_hi <= 17) && (version_lo==10));

      fprintf(stderr, "I'm a cinqo %d!\n",self->is_old_cinqo);

      channel_manager_associate_control_channels(self->parent);
    } // cinqo check
  }
}

void ant_channel_broadcast_event(ant_channel_t *self, unsigned char *ant_message) { 

  unsigned char *message=ant_message+2;
  static char last_message[ANT_MAX_MESSAGE_SIZE];
  timestamp=get_timestamp();

  self->messages_received++;
  self->last_message_timestamp=timestamp;

  if (self->state != MESSAGE_RECEIVED) {
    // first message! who are we talking to?
    ANT_RequestMessage(self->number, ANT_CHANNEL_ID);
    self->blanking_timestamp=get_timestamp();
    self->blanked=0;
    return; // because we can't associate a channel id with the message yet
  } 

  if (0==memcmp(message, last_message, ANT_MAX_MESSAGE_SIZE)) {
    //fprintf(stderr, "No change\n");
    return; // no change
  }

  {
    // for automatically opening quarq channel on early cinqo
    if (MESSAGE_IS_PRODUCT(message)) {
      self->mi.first_time_product=0;
      self->product_version&=0x00ff; 
      self->product_version|=(PRODUCT_SW_REV(message))<<8;
      ant_channel_check_cinqo(self);
    } else if (MESSAGE_IS_MANUFACTURER(message)) {
      self->mi.first_time_manufacturer=0;
      self->product_version&=0xff00;
      self->product_version|=MANUFACTURER_HW_REV(message);
      self->manufacturer_id=MANUFACTURER_MANUFACTURER_ID(message);
      self->product_id=MANUFACTURER_MODEL_NUMBER_ID(message);
      ant_channel_check_cinqo(self);
    }            
  }

  {
    int matched=0;
    
    switch(self->channel_type) {
    case CHANNEL_TYPE_HR:
      ant_message_print_debug(message);
      matched=xml_message_interpret_heartrate_broadcast(self, message);
      break;
    case CHANNEL_TYPE_POWER:    
      matched=xml_message_interpret_power_broadcast(self, message);
      break;
    case CHANNEL_TYPE_SPEED:
      matched=xml_message_interpret_speed_broadcast(self, message);
      break;
    case CHANNEL_TYPE_CADENCE:
      matched=xml_message_interpret_cadence_broadcast(self, message);
      break;
    case CHANNEL_TYPE_SandC:
      matched=xml_message_interpret_speed_cadence_broadcast(self, message);
      break;
    case CHANNEL_TYPE_QUARQ:
    case CHANNEL_TYPE_FAST_QUARQ:
    case CHANNEL_TYPE_FAST_QUARQ_NEW:
#ifdef QUARQ_BUILD
      matched=xml_message_interpret_quarq_broadcast(self, message);
#else
      matched=xml_message_interpret_power_broadcast(self, message);
#endif
      break;
    default:      
      break;
    }
   
    if ((!matched) && (quarqd_config.debug_level & DEBUG_LEVEL_ERRORS)) {
      int i;
      fprintf(stderr, "Unknown Message!\n");
      for (i=0; i<ant_message[ANT_OFFSET_LENGTH]; i++) {
	fprintf(stderr, "0x%02x ",message[i]);
      }
      fprintf(stderr, "\n");

      ant_message_print_debug(message);
      
      //exit(-1); // for testing
    } 
  }
}

void ant_channel_ack_event(ant_channel_t *self, unsigned char *ant_message) { 

  unsigned char *message=ant_message+2;

  {
    int matched=0;
    
    switch(self->channel_type) {
    case CHANNEL_TYPE_POWER:    
      matched=xml_message_interpret_power_broadcast(self, message);
      break;  
    }

    if ((!matched) && (quarqd_config.debug_level & DEBUG_LEVEL_ERRORS)) {
      int i;
      fprintf(stderr, "Unknown Message!\n");
      for (i=0; i<ant_message[ANT_OFFSET_LENGTH]; i++) {
	fprintf(stderr, "0x%02x ",message[i]);
      }
      fprintf(stderr, "\n");

      ant_message_print_debug(message);
      
      exit(-1); // for testing
    } 
  }
}


void ant_channel_channel_id(ant_channel_t *self, unsigned char *ant_message) { 
  
  unsigned char *message=ant_message+2;
  
  self->device_number=CHANNEL_ID_DEVICE_NUMBER(message);
  self->device_id=CHANNEL_ID_DEVICE_TYPE_ID(message);
  
  ant_channel_set_id(self);

  self->state=MESSAGE_RECEIVED;

  DEBUG_ANT_CONNECTION("%d: Connected to %s (0x%x) device number %d\n",
		       self->number,
		       ant_channel_get_description(self),
		       self->device_id,
		       self->device_number);
  
  ant_channel_channel_info(self);

  // if we were searching, 
  if (self->channel_type_flags & CHANNEL_TYPE_QUICK_SEARCH) {
    ANT_SetChannelSearchTimeout(self->number, 
				(int)(timeout_lost/2.5));
  }  
  self->channel_type_flags &= ~CHANNEL_TYPE_QUICK_SEARCH;

  channel_manager_start_waiting_search(self->parent);

  // if we are quarq channel, hook up with the ant+ channel we are connected to
  channel_manager_associate_control_channels(self->parent);
  
}

void ant_channel_burst_init(ant_channel_t *self) {
  self->rx_burst_data_index=0;
  self->rx_burst_next_sequence=0;
  self->rx_burst_disposition=NULL;
}

int ant_channel_is_searching(ant_channel_t *self) {
  return ((self->channel_type_flags & (CHANNEL_TYPE_WAITING | CHANNEL_TYPE_QUICK_SEARCH)) || (self->state != MESSAGE_RECEIVED)); 
}


void ant_channel_burst_data(ant_channel_t *self, unsigned char *ant_message) {

  unsigned char *message=ant_message+2;
  char seq=(message[1]>>5)&0x3;
  char last=(message[1]>>7)&0x1;
  const unsigned char next_sequence[4]={1,2,3,1};

  if (seq!=self->rx_burst_next_sequence) {
    DEBUG_ERRORS("Bad sequence %d not %d\n",seq,self->rx_burst_next_sequence); 
   // burst problem!
  } else {
    int len=ant_message[ANT_OFFSET_LENGTH]-3;
    
    if ((self->rx_burst_data_index + len)>(RX_BURST_DATA_LEN)) {
      len = RX_BURST_DATA_LEN-self->rx_burst_data_index;
    }

    self->rx_burst_next_sequence=next_sequence[(int)seq];
    memcpy(self->rx_burst_data+self->rx_burst_data_index, message+2, len);
    self->rx_burst_data_index+=len; 
    
    //fprintf(stderr, "Copying %d bytes.\n",len);
  }

  if (last) {
    if (self->rx_burst_disposition) {
      self->rx_burst_disposition(self);
    }
    ant_channel_burst_init(self);
  }
}

void ant_channel_request_calibrate(ant_channel_t *self) {  
  ANT_RequestCalibrate(self->number);
}

void ant_channel_attempt_transition(ant_channel_t *self, int message_id) {
  
  const ant_sensor_type_t *st;

  int previous_state=self->state;

  st=ant_sensor_types+(self->channel_type);


  // update state
  self->state=message_id;

  // do transitions
  switch (self->state) {
  case ANT_CLOSE_CHANNEL:
    // next step is unassign and start over
    // but we must wait until event_channel_closed
    // which is its own channel event
    self->state=MESSAGE_RECEIVED; 
    break;
  case ANT_UNASSIGN_CHANNEL:
    self->channel_assigned=0;
    if (st->type==CHANNEL_TYPE_UNUSED) {
      // we're shutting the channel down

    } else {

      self->device_id=st->device_id;
	
      if (self->channel_type & CHANNEL_TYPE_PAIR) {
	self->device_id |= 0x80;
      }
      
      ant_channel_set_id(self);
    
      DEBUG_ANT_CONNECTION("Opening for %s\n",ant_channel_get_description(self));
      ANT_AssignChannel(self->number, 0, st->network); // recieve channel on network 1
    }
    break;
  case ANT_ASSIGN_CHANNEL:
    self->channel_assigned=1;
    ANT_SetChannelID(self->number, self->device_number, self->device_id, 0);
    break;
  case ANT_CHANNEL_ID:
    if (self->channel_type & CHANNEL_TYPE_QUICK_SEARCH) {
      DEBUG_ANT_CONNECTION("search\n");
      ANT_SetChannelSearchTimeout(self->number, 
				  (int)(timeout_scan/2.5));
    } else {
      DEBUG_ANT_CONNECTION("nosearch\n");
      ANT_SetChannelSearchTimeout(self->number, 
				  (int)(timeout_lost/2.5));
    }    
    break;
  case ANT_SEARCH_TIMEOUT:
    if (previous_state==ANT_CHANNEL_ID) {
      // continue down the intialization chain
      ANT_SetChannelPeriod(self->number, st->period); 
    } else {
      // we are setting the ant_search timeout after connected
      // we'll just pretend this never happened
      DEBUG_ANT_CONNECTION("resetting ant_search timeout.\n");
      self->state=previous_state; 
    }
    break;
  case ANT_CHANNEL_PERIOD:
    ANT_SetChannelFreq(self->number, st->frequency);
    break;
  case ANT_CHANNEL_FREQUENCY:
    ANT_Open(self->number);
    INITIALIZE_MESSAGES_INITIALIZATION(self->mi);
    break;
  case ANT_OPEN_CHANNEL:
    DEBUG_ANT_CONNECTION("Channel %d opened for %s\n",self->number,
			 ant_channel_get_description(self));
    break;
  default:
    DEBUG_ERRORS("unknown channel event 0x%x\n",message_id);
  }  
}

#define BXmlPrintf(format, args...) \
  self->blanking_timestamp=timestamp; \
  XmlPrintf(format, ##args)

#define RememberMessage(message_len, if_changed)		      \
  static unsigned char last_messages[ANT_CHANNEL_COUNT][message_len]; \
  unsigned char * last_message=last_messages[self->number]; \
  \
  if (!first_time) { \
    if (0!=memcmp(message,last_message,message_len)) {	\
      if_changed;					\
    } \
  } else {	  \
    first_time=0; \
  } \
  memcpy(last_message,message,message_len)

#define RememberXmlPrintf(message_len, format, args...)	\
  RememberMessage(message_len, XmlPrintf(format, ##args))

#define BlankingXmlPrintf(message_len, format, args...)	\
  RememberMessage(message_len, BXmlPrintf(format, ##args))

void ant_channel_channel_info(ant_channel_t *self) {
  XmlPrintf("<SensorFound id='%s' device_number='%d' type='%s%s'%s ant_channel='%d' messages_received='%d' messages_dropped='%d' drop_percent='%d'/>\n",
	    self->id,
	    self->device_number,
	    ant_channel_get_description(self),
	    ((self->state==MESSAGE_RECEIVED || 
	      (ant_sensor_types[self->channel_type]).type==CHANNEL_TYPE_UNUSED)?
	     "":" (searching)"),
	    (self->channel_type_flags&CHANNEL_TYPE_PAIR)?" paired":"",
	    self->number,
	    self->messages_received,
	    self->messages_dropped,
	    self->messages_received? \
	    100*self->messages_dropped/self->messages_received : 0);
}

void ant_channel_drop_info(ant_channel_t *self) {  
  XmlPrintf("<SensorDrop id='%s' device_number='%d' type='%s' timeout='%.1f' ant_channel='%d'/>\n",
	    self->id,
	    self->device_number,
	    ant_channel_get_description(self),
	    timeout_drop,
	    self->number);

}

void ant_channel_lost_info(ant_channel_t *self) {  
  XmlPrintf("<SensorLost id='%s' device_number='%d' type='%s' timeout='%.1f' ant_channel='%d'/>\n",
	    self->id,
	    self->device_number,
	    ant_channel_get_description(self),	    
	    timeout_lost,
	    self->number);
}

void ant_channel_stale_info(ant_channel_t *self) {  
  XmlPrintf("<SensorStale id='%s' device_number='%d' type='%s' timeout='%.1f' ant_channel='%d'/>\n",
	    self->id,
	    self->device_number,
	    ant_channel_get_description(self),	    
	    timeout_blanking,
	    self->number);
}

void ant_channel_report_timeouts( void ) {
  XmlPrintf("<Timeout type='blanking' value='%.2f'>\n"
	    "<Timeout type='drop' value='%.2f'>\n"
	    "<Timeout type='scan' value='%.2f'>\n"
	    "<Timeout type='lost' value='%.2f'>\n",
	    timeout_blanking,
	    timeout_drop,
	    timeout_scan,
	    timeout_lost);
}

int ant_channel_set_timeout( char *type, float value, int connection) {

  if (0==strcmp(type,"blanking")) timeout_blanking=value;
  else if (0==strcmp(type,"drop")) timeout_drop=value;
  else if (0==strcmp(type,"scan")) timeout_scan=value;
  else if (0==strcmp(type,"lost")) timeout_lost=value;
  else {
    ant_channel_report_timeouts();
    return 0;
  } 
  ant_channel_report_timeouts();
  return 1;
}

// this is in the wrong place here for the convenience of the 
// XmlPrintf macro
void ant_channel_search_complete( void ) {
  XmlPrintf("<SearchFinished />\n");  
}

float get_srm_offset(int device_id) {
  float ret=0.0;
  read_offset_log(device_id, &ret);
  return ret;
}

void set_srm_offset(int device_id, float value) {
  update_offset_log(device_id, value);
}

#include "generated-xml.c"
	    
