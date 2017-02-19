#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ant_channel.h"
#include "channel_manager.h"
#include "configuration.h"

void channel_manager_init(channel_manager_t *self) {
  int i;
  
  for (i=0; i<ANT_CHANNEL_COUNT; i++)
    ant_channel_init(self->channels+i, i, self);  

  if (0) {
    channel_manager_remember_device(self, 17, CHANNEL_TYPE_POWER);
    channel_manager_remember_device(self, 17, CHANNEL_TYPE_HR);
    channel_manager_remember_device(self, 17, CHANNEL_TYPE_SandC);
    channel_manager_remember_device(self, 17, CHANNEL_TYPE_SPEED);
  
    return;
  }
  
}

// returns 1 for success, 0 for fail.
int channel_manager_add_device(channel_manager_t *self, int device_number, int device_type, int channel_number) {
  int i;  

  // if we're given a device number, then use that one
  if (channel_number>-1) {
    ant_channel_close(self->channels+channel_number);
    ant_channel_open(self->channels+channel_number, device_number, device_type);
    return 1;
  }

  // if we already have the device, then return.
  for (i=0; i<ANT_CHANNEL_COUNT; i++)
    if (((self->channels[i].channel_type & 0xf ) == device_type) &&
	(self->channels[i].device_number == device_number)) {
      // send the channel found...
      ant_channel_channel_info(self->channels+i);
      return 1; 
    }
  
  // look for an unused channel and use on that one
  for (i=0; i<ANT_CHANNEL_COUNT; i++)
    if (self->channels[i].channel_type == CHANNEL_TYPE_UNUSED) {
      
      fprintf(stderr, "opening channel #%d\n",i);

      ant_channel_open(self->channels+i, device_number, device_type);
      return 1;
    }

  // there are no unused channels.  fail.
  return 0;  
}

// returns 1 for successfully removed, 0 for none found.
int channel_manager_remove_device(channel_manager_t *self, int device_number, int channel_type) {

  int i;

  for (i=0; i<ANT_CHANNEL_COUNT; i++) {
    ant_channel_t *ac = self->channels+i;

    if ((ac->channel_type == channel_type) && 
	(ac->device_number == device_number)) {
      
      if ((ac->control_channel!=ac) && ac->control_channel) {
      
	channel_manager_remove_device(self, device_number, ac->control_channel->channel_type);
      }
      
      ant_channel_close(ac);

      ac->channel_type=CHANNEL_TYPE_UNUSED;
      ac->device_number=0;
      ant_channel_set_id(ac);
      return 1;
    }
  }

  // device not found.
  return 0;
}

ant_channel_t * channel_manager_find_device(channel_manager_t *self, int device_number, int channel_type) {

  int i;

  for (i=0; i<ANT_CHANNEL_COUNT; i++)
    if (((self->channels[i].channel_type) == channel_type) && 
	(self->channels[i].device_number==device_number)) {      
      return self->channels+i;
    }

  // device not found.
  return NULL;

}

int channel_manager_start_waiting_search(channel_manager_t *self) {
  
  int i;
  // are any fast searches in progress?  if so, then bail
  for (i=0; i<ANT_CHANNEL_COUNT; i++) {
    if (self->channels[i].channel_type_flags & CHANNEL_TYPE_QUICK_SEARCH) {
      return 0;
    }
  }

  // start the first slow search
  for (i=0; i<ANT_CHANNEL_COUNT; i++) {
    if (self->channels[i].channel_type_flags & CHANNEL_TYPE_WAITING) {
      self->channels[i].channel_type_flags &= ~CHANNEL_TYPE_WAITING;
      ANT_UnassignChannel(i);
      return 1;
    }
  }
  
  return 0;
}

void channel_manager_report(channel_manager_t *self) {
  int i;

  for (i=0; i<ANT_CHANNEL_COUNT; i++) 
    ant_channel_channel_info(self->channels+i);

}

void channel_manager_associate_control_channels(channel_manager_t *self) {
  int i;

  // first, unassociate all control channels
  for (i=0; i<ANT_CHANNEL_COUNT; i++) 
    self->channels[i].control_channel=NULL;

  // then, associate cinqos:
  //    new cinqos get their own selves for control
  //    old cinqos, look for an open control channel
  //       if found and open, associate
  //       elif found and not open yet, nop
  //       elif not found, open one

  for (i=0; i<ANT_CHANNEL_COUNT; i++) {
    ant_channel_t *ac=self->channels+i;

    switch (ac->channel_type) {
    case CHANNEL_TYPE_POWER:
      if (ac->is_cinqo) {
	if (ac->is_old_cinqo) {
	  ant_channel_t * my_ant_channel;

	  my_ant_channel=channel_manager_find_device(self,
						     ac->device_number,
						     CHANNEL_TYPE_QUARQ);
	  if (!my_ant_channel) {
	    my_ant_channel=channel_manager_find_device(self,
						       ac->device_number,
						       CHANNEL_TYPE_FAST_QUARQ);
	  }
	  if (!my_ant_channel) {
	    my_ant_channel=channel_manager_find_device(self,
						       ac->device_number,
						       CHANNEL_TYPE_FAST_QUARQ_NEW);
	  }

	  if (my_ant_channel) {
	    if (ant_channel_is_searching(my_ant_channel)) {
	      ; // searching, just wait around 
	    } else {
	      ac->control_channel=my_ant_channel;
	      ant_channel_send_cinqo_success(ac);
	    }
	  } else { // no ant channel, let's start one
	    channel_manager_add_device(self, ac->device_number,
				       CHANNEL_TYPE_QUARQ, -1);
	  }
    
	} else { // new cinqo
	  ac->control_channel=ac;
	  ant_channel_send_cinqo_success(ac);
	}
      } // is_cinqo
      break;

    case CHANNEL_TYPE_FAST_QUARQ:
    case CHANNEL_TYPE_FAST_QUARQ_NEW:
    case CHANNEL_TYPE_QUARQ:
      ac->is_cinqo=1;
      ac->control_channel=ac;
    default:
      ;
    } // channel_type case
  } // for-loop

}


