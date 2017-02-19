#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H

#include "ant_channel.h"

typedef struct channel_manager {
  struct ant_channel channels[4];
} channel_manager_t;

/* The channel manager: keep track of the channels.

   There are four channels in the Ant chip.  Let's use them wisely.
   The fourth channel can be used for searching. 

   Three levels of allocation:

   1) Not allocated.  Channel can remain closed.

   2) Auto-allocated.  The search channel has found a device and
      allocated it to the channel to allow it to search for other
      devices.  If the device goes out of range, the auto-allocated
      channel should be de-allocated.

   3) User-allocated.  The user wants this channel open for the
      specific device. Leave it alone.

   So the "use device" message should allocate a new channel and push
   the oldest one off the queue.

   Messages:

    add_device:  allocate a channel for the specified device
    remove_device: de-allocate a channel for the specified device
    report:      channels each report what they're doing
    save:        save channel configurations in $HOME/.quarqd
    load:        load channel configurations from $HOME/.quarqd
       
  .quarqd format: 

  [device_type] [device_number]

  where channel type is a string and device_number is a 16-bit integer.

 */


void channel_manager_init(channel_manager_t *self);

int channel_manager_initiate_search(channel_manager_t *self);
void channel_manager_search(channel_manager_t *self, int channel_type, int channel_num);
int channel_manager_search_timed_out(channel_manager_t *self, int channel_type, int channel_num);
void channel_manager_search_connected(channel_manager_t *self, int channel_type, int device_number, int channel_num);
    
int channel_manager_next_search_type(channel_manager_t *self);

int channel_manager_add_device(channel_manager_t *self, int device_number, int device_type, int channel_number);
int channel_manager_remember_device(channel_manager_t *self, int device_number, int device_type);
int channel_manager_remove_device(channel_manager_t *self, int device_number, int device_type);
ant_channel_t * channel_manager_find_device(channel_manager_t *self, int device_number, int device_type);
int channel_manager_start_waiting_search(channel_manager_t *self);
void channel_manager_report(channel_manager_t *self);
int channel_manager_save(channel_manager_t *self);
int channel_manager_load(channel_manager_t *self);
void channel_manager_associate_control_channels(channel_manager_t *self);

#endif
