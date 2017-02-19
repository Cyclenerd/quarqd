#ifndef ANT_CHANNEL_H
#define ANT_CHANNEL_H

#include <sys/types.h>
#include "ant.h"

#define ANT_CHANNEL_COUNT 4

//#include "channel_manager.h"

/* I'll re-use the ANT constants as much as possible.
   For this to be useful state information, we must initialize Ant in
   a certain sequence.

   Fortunately, this is not hard to do. */

enum { 
  CHANNEL_TYPE_UNUSED,
  CHANNEL_TYPE_HR,
  CHANNEL_TYPE_POWER, 
  CHANNEL_TYPE_SPEED,
  CHANNEL_TYPE_CADENCE,
  CHANNEL_TYPE_SandC,
  CHANNEL_TYPE_QUARQ,
  CHANNEL_TYPE_FAST_QUARQ,
  CHANNEL_TYPE_FAST_QUARQ_NEW,
  CHANNEL_TYPE_GUARD
};

#define CHANNEL_TYPE_QUICK_SEARCH 0x10 // or'ed with current channel type
/* after fast search, wait for slow search.  Otherwise, starting slow
   search might postpone the fast search on another channel. */
#define CHANNEL_TYPE_WAITING 0x20 
#define CHANNEL_TYPE_PAIR   0x40 // to do an Ant pair
#define MESSAGE_RECEIVED -1

typedef struct ant_sensor_type {
  int type;
  int period;
  int device_id;
  int frequency;
  int network;
  char *descriptive_name;
  char suffix;
  
} ant_sensor_type_t;

#define DEFAULT_NETWORK_NUMBER 0
#define ANT_SPORT_NETWORK_NUMBER 1

extern const ant_sensor_type_t ant_sensor_types[];

#include "generated-header.h"

typedef struct ant_channel {
  int number; // Channel number within Ant chip  
  int state;  
  int channel_type;
  int channel_type_flags;
  int device_number;
  int device_id;

  int search_type;

  struct channel_manager *parent;

  double last_message_timestamp;
  double blanking_timestamp;
  int blanked;

  char id[10]; // short identifier

  int channel_assigned;
 
  messages_initialization_t mi;

  int messages_received; // for signal strength metric
  int messages_dropped;  

#define RX_BURST_DATA_LEN 128

  unsigned char rx_burst_data[RX_BURST_DATA_LEN];
  int           rx_burst_data_index;
  unsigned char rx_burst_next_sequence;

  void (*rx_burst_disposition)(struct ant_channel *);
  void (*tx_ack_disposition)(struct ant_channel *);

  int is_cinqo; // bool
  int is_old_cinqo; // bool, set for cinqo needing separate control channel
  struct ant_channel *control_channel;
  /* cinqo-specific, which channel should control
     messages be sent to?   Open a channel for older
     cinqos */
  int manufacturer_id;
  int product_id;
  int product_version;

} ant_channel_t;

#include "ant_quarq.h"
#include "generated-messages.h"

void ant_channel_init(ant_channel_t *self, int number, struct channel_manager *parent);
char * ant_channel_get_description(ant_channel_t *self);
int ant_channel_interpret_description(char *description);
int ant_channel_interpret_suffix(char c);
void ant_channel_open(ant_channel_t *self, int device_number, int channel_type);
void ant_channel_close(ant_channel_t *self);

void ant_channel_receive_message(ant_channel_t *self, unsigned char *message);
void ant_channel_channel_event(ant_channel_t *self, unsigned char *message);
void ant_channel_burst_data(ant_channel_t *self, unsigned char *message);
void ant_channel_broadcast_event(ant_channel_t *self, unsigned char *message);
void ant_channel_ack_event(ant_channel_t *self, unsigned char *message);
void ant_channel_channel_id(ant_channel_t *self, unsigned char *message);
void ant_channel_request_calibrate(ant_channel_t *self);
void ant_channel_attempt_transition(ant_channel_t *self, int message_code);
void ant_channel_channel_info(ant_channel_t *self);
void ant_channel_drop_info(ant_channel_t *self);
void ant_channel_lost_info(ant_channel_t *self);
void ant_channel_stale_info(ant_channel_t *self);
void ant_channel_report_timeouts( void );
int ant_channel_set_timeout( char *type, float value, int connection );
void ant_channel_search_complete(void);
void ant_channel_send_error( char * message );
void ant_channel_set_id(ant_channel_t *self);
int ant_channel_is_searching(ant_channel_t *self);
void ant_channel_burst_init(ant_channel_t *self);
void ant_channel_send_cinqo_error(ant_channel_t *self);
void ant_channel_send_cinqo_success(ant_channel_t *self);

#endif
