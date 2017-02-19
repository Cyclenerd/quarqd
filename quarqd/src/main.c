#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <ctype.h>

#include "quarqd.h"
#include "ant.h"
#include "channel_manager.h"
#include "configuration.h"


#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif

#define MAX_CLIENTS 64

#error Fix NetworkKey to ANT+ key
static const unsigned char NetworkKey[8] = { 0x00, 0x00, 0x00, 0x00,
					     0x00, 0x00, 0x00, 0x00 };

static const char NAME[] = "Quarqd";
char * heartRateMsgFormat;
char * wheelMsgFormat;
char * crankMsgFormat;
char * powerMsgFormat;
char * cadenceMsgFormat;
char * speedMsgFormat;

extern int ant_fd;
static int server_fd;
static int quit = 0;
static int clients[MAX_CLIENTS];
static int num_clients=0;

static int max_fd;

void compute_max_fd(void) {
  int i;

  max_fd=max(server_fd, ant_fd);
  for (i=0; i<num_clients; i++) {
    max_fd=max(clients[i],max_fd);    
  }
}

void removeClient(int index) {
  int j;

  num_clients--;
  
  for (j=index; j<num_clients; j++) {
    clients[j]=clients[j+1];
  }

  compute_max_fd();

}

char xml_buffer[XML_BUFFER_LEN];

// returns 1 on success, 0 if the channel is closed
int writeDataSingleChannel(char *str, int channel_index) {

  int result=write(clients[channel_index], str, strlen(str));
  
  if(result < strlen(str)) {
        
    DEBUG_ERRORS("Error writing to server socket %d\n",result);    
    perror(NAME);
    
    // if we get an error, close that client and continue on our way.
    close(clients[channel_index]);
    removeClient(channel_index);

    return 0;
  }	   
  return 1;
}

void writeData(char *str) {
        int i;
	for(i = 0; i < num_clients; i++) {
	  if (0==writeDataSingleChannel(str, i)) i--;
	}
}

void init_tcp(void) {
	struct sockaddr_in addr;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(server_fd < 0) {
	  fprintf(stderr,"Error opening tcp socket\n");
	  perror(NAME);
	  exit(server_fd);
	}
	
	int option_value = 1;
	if(setsockopt(server_fd, 
		      SOL_SOCKET, 
		      SO_REUSEADDR, 
		      (char *)&option_value, 
		      sizeof(option_value)) < 0)
    		perror("setsockopt");
	
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(quarqd_config.quarqd_port);
	
	fcntl(server_fd, F_SETFL, O_NONBLOCK);
	
	if(bind(server_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		fprintf(stderr,"Error binding tcp socket\n");
		perror(NAME);
		exit(-1);
	}
	
	if(listen(server_fd, 5) < 0) {
		fprintf(stderr,"Error listen to tcp socket\n");
		perror(NAME);
		exit(-1);
	}
}

int setdtr (int on)
{
  int controlbits = TIOCM_DTR;
  if(on)
    return(ioctl(ant_fd, TIOCMBIC, &controlbits));
  else
    return(ioctl(ant_fd, TIOCMBIS, &controlbits));
} 



void init_tty(char *dev) {
	struct termios tty_conf;
	
	ant_fd = open(dev, O_RDWR | O_NOCTTY );

	if(ant_fd < 0) {
		fprintf(stderr,"Error opening %s: are you root?\n", dev);
		perror(dev);
		exit(ant_fd);
	}

	tcgetattr(ant_fd, &tty_conf);

#define TRY(sp)						\
	if (quarqd_config.ant_baudrate==sp) {		\
	  cfsetispeed(&tty_conf, B ## sp);		\
	  cfsetospeed(&tty_conf, B ## sp);		\
	}
	TRY(4800)
	else TRY(9600)
	else TRY(19200)
	else TRY(38400)
	else TRY(57600)
	else TRY(115200)
	else {
	  fprintf(stderr, "Invalid baudrate: %d\n",quarqd_config.ant_baudrate);
	  exit(-1);
	}
#undef TRY

	cfmakeraw(&tty_conf);

	tty_conf.c_cc[VMIN]=1;
   	tty_conf.c_cc[VTIME]=0;

	// Set the new options for the port...
	tcflush(ant_fd, TCIFLUSH);
	tcsetattr(ant_fd, TCSANOW, &tty_conf);

	setdtr(0);
	setdtr(1);
}

/* 
  The default sigpipe handler doesn't display an error message
  ignoring the sigpipe causes our error messages to be displayed.
 */
static void HandleSigPipe(int sig) {
  // the man page says the signal handler gets reset after use
  // so let's re-enable it.
  fprintf(stderr, "whoa! got a sigpipe!\n");
  //signal(SIGPIPE, HandleSigPipe);
}

static void Quit(int signal) {
	quit = 1;
}

channel_manager_t cm;

/* parses a device id specifier (integer followed by a single character)

 preconditions: in points to a pointer to a string possibly containing
                the device id specifier
		num points to an integer to hold the device_num
		type points to an integer to hold the channel_type 
		     (enumerated in ant_channel.h)

 returns:  0 for success
           -1 for integer parse fail
	   -2 for character specification fail
	   
 postconditions: *in points to the next character after the device id

*/ 
int parse_device_id(char **in, int *num, int *type) {
  char *c = *in;
  char *d;

  errno=0;
  *num = strtol(c,&d,0);
  if ((errno) || 
      (*num<0) || (*num>0xffff)) return -1; 
 
  *type = ant_channel_interpret_suffix(*d);
  
  if (*type<0) return -2;
						
  *in=d+1;

  return 0;
}

int act_on_incoming_message(char *key, char *value, int connection) {

  DEBUG_CONFIG_PARSE("Key is \"%s\" val is \"%s\"\n",key,value);

  if ((0==strcmp(key,"X-set-channel")) ||
      (0==strcmp(key,"X-pair-device"))) {

    char *c=value;
    int device_number, channel_type;
    int channel_number=-1;

    if (0!=parse_device_id(&c, &device_number, &channel_type)) {
      SendErrorMessage(connection,"Could not parse the device id");
      return 0;
    }
    
    if (*c==',') {
      c++;
      errno=0;
      switch (*c) {
      case '0':
      case '1':
      case '2':
      case '3':
	channel_number=*c-'0';
      }      
    }

    if (0==strcmp(key,"X-pair-device")) channel_type |= CHANNEL_TYPE_PAIR;

    return channel_manager_add_device(&cm, device_number, channel_type, channel_number);

  } else if (0==strcmp(key,"X-unset-channel")) { 
    int device_number, channel_type;
    int ret=0;
 
    if (0!=parse_device_id(&value, &device_number, &channel_type)) {
      SendErrorMessage(connection,"Could not parse the device id");
      return 0; 
    }

    if (channel_type==CHANNEL_TYPE_POWER) { // special case for quarq
      //  |, not || because we don't want short-circuit evaluation...
      ret=(channel_manager_remove_device(&cm, device_number, CHANNEL_TYPE_POWER) |
	   channel_manager_remove_device(&cm, device_number, CHANNEL_TYPE_QUARQ) |
	   channel_manager_remove_device(&cm, device_number, CHANNEL_TYPE_FAST_QUARQ) |
	   channel_manager_remove_device(&cm, device_number, CHANNEL_TYPE_FAST_QUARQ_NEW));
    } else {
      ret=channel_manager_remove_device(&cm, device_number, channel_type);
    }
     
    if (!ret) {
      SendErrorMessage(connection,"%s requested on an unset channel",key);
    }
    return ret;

  } else if (0==strcmp(key,"X-set-timeout")) {
    char *c;
    float timeout_value=strtod("NAN",0);

    for (c=value; *c; c++)
      if (*c=='=') {
	*c='\0'; 
	errno=0;
	timeout_value=strtod(c+1,NULL);
	if (errno) value="unknown"; // force value setting to fail
	break;
      }

    if ((0!=strcmp(value, "blanking")) &&
	(0!=strcmp(value, "drop")) &&
	(0!=strcmp(value, "scan")) &&
	(0!=strcmp(value, "lost"))) {
      SendErrorMessage(connection,
		       "<Error message=\"Unknown timeout type '%s'\" /r>\n", value); }
    
    return ant_channel_set_timeout(value, timeout_value,connection);
  } else if (0==strcmp(key,"X-list-channels")) {
    channel_manager_report(&cm);
    return 1;

    /* Handle CinQo-specific messages. */
  } else if ((0==strcmp(key,"X-calibrate")) || 
	     (0==strcmp(key,"X-get-slope")) ||
	     (0==strcmp(key,"X-get-offset")) ||
	     (0==strcmp(key,"X-set-slope")) ||
	     (0==strcmp(key,"X-set-test-mode")) ||
	     (0==strcmp(key,"X-unset-test-mode"))) {

    int device_number, channel_type;
    
    if (0!=parse_device_id(&value, &device_number, &channel_type)) {
      SendErrorMessage(connection,"Could not parse the device id");
      return 0;
    }


    if ((channel_type != CHANNEL_TYPE_POWER) &&
	(channel_type != CHANNEL_TYPE_FAST_QUARQ) &&
	(channel_type != CHANNEL_TYPE_FAST_QUARQ_NEW)) {
      SendErrorMessage(connection,"%s requested on a non-power sensor",key);
      return 0;
    }
      
    ant_channel_t *ac = channel_manager_find_device(&cm, device_number, channel_type);

    if (NULL==ac) { 
      ac = channel_manager_find_device(&cm, device_number, CHANNEL_TYPE_FAST_QUARQ);
    }

    if (NULL==ac) { 
      ac = channel_manager_find_device(&cm, device_number, CHANNEL_TYPE_FAST_QUARQ_NEW);
    }

    if (NULL==ac) {
      SendErrorMessage(connection,"%s requested on an unset channel",key);
      return 0;
    }

    if (ac->state != MESSAGE_RECEIVED) {
      SendErrorMessage(connection,"%s requested on a not-connected device",key);
      return 0;
    }

    // this one isn't cinqo-specific
    if (0==strcmp(key,"X-calibrate")) {
      ant_channel_request_calibrate(ac);
      return 1;
    }
    
    // but all the others are
#ifdef QUARQ_BUILD
    if (!(ac->is_cinqo)) {
      SendErrorMessage(connection,"%s requested on a non-CinQo device",key);
      return 0;
    }

    if (NULL==ac->control_channel) {
      SendErrorMessage(connection,"%s requested on a CinQo before the control channel is opened",key);
      return 0;
    }

    if (0==strcmp(key,"X-get-offset")) {
      ant_channel_request_offset(ac->control_channel);
    }    
    if (0==strcmp(key,"X-get-slope")) {
      ant_channel_request_slope(ac->control_channel);
    }    
    if (0==strcmp(key,"X-set-slope")) {
      float slope=strtod(value,0);

      if (slope==0.0) {
	SendErrorMessage(connection,"Bad slope value");
	return 0;    
      }

      fprintf(stderr,"setting slope to %f\n",slope);      
      ant_channel_set_slope(ac->control_channel, slope);

    } else if (0==strcmp(key,"X-set-test-mode")) {
      ant_channel_set_test_mode(ac->control_channel);
    } else if (0==strcmp(key,"X-unset-test-mode")) {
      ant_channel_unset_test_mode(ac->control_channel);
    }
    
    return 1;
#endif // QUARQ_BUILD

  } else {
    SendErrorMessage(connection,"Unknown request");
    return 0;
  }
  return 0;
}

int check_incoming_line(char *line, int connection) {
  char *v = line;
  
  for (v=line; *v; v++) {
    if (*v==':') {
      *v++=0;	
      // skip whitespace
      for (; isspace(*v); v++); 
      return act_on_incoming_message(line, v, connection);
    }
  }
  SendErrorMessage(connection,"Unknown request");
  return 0;
}

 int check_incoming_message(char * incoming, int connection) {
  char *line;
  char *last_line;

  line = strtok_r(incoming, "\n\r", &last_line); 
  if (line==NULL) return 0;
  
  do {    
    DEBUG_CONFIG_PARSE("Request line: %s\n",line);
    // dump out if the line doesn't parse
    if (!check_incoming_line(line, connection)) {
      return 0; 
    }
  } while ((line = strtok_r(NULL, "\n\r", &last_line)));  

  return 1;  
}

#include "svn_version.h"

int main(void) {
  
        printf("\nquarqd v%d\n\nCopyright (c) 2007-2010, Quarq Technology Inc\nAll rights reserved.\n\n",QUARQD_VERSION);

        signal(SIGPIPE, SIG_IGN);
	//signal(SIGPIPE, HandleSigPipe);

	init_config();

	init_tty(quarqd_config.ant_tty);
	init_tcp();

	ANT_ResetSystem();
	ANT_SetNetworkKey(1, NetworkKey);	

	channel_manager_init(&cm);
	
	compute_max_fd();

	signal(SIGINT, Quit);
	signal(SIGTERM, Quit);
	while(!quit) {
	  int i;
	  fd_set fdset;
	  struct timeval tv;
	  int ret;
	  
	  FD_ZERO(&fdset);
	  FD_SET(server_fd, &fdset);
	  FD_SET(ant_fd, &fdset);
	  
	  for (i=0; i<num_clients; i++) {
	    FD_SET(clients[i], &fdset);
	  }
	  
	  tv.tv_sec = 1;
	  tv.tv_usec = 0;

	  ret = select(1+max_fd,   // num 
		       &fdset, // readfds
		       NULL,   // writefds
		       NULL,   // exceptions
		       &tv);   // timeout

 	  //fprintf(stderr,"."); 
	  fflush(stderr);

	  if(ret < 0) {
	    if (errno == EINTR)
	      continue;
	    else
	      perror("select");
	  }
	  if(ret == 0)
	    continue;
	  
	  for (i=0; i<num_clients; i++) {
	    if FD_ISSET(clients[i], &fdset) {

#define READBUF_SIZE 1024
		char data[READBUF_SIZE];
		
		int request_len;

		request_len=recv(clients[i],data,READBUF_SIZE,0);
		
		data[request_len]=0;

		check_incoming_message(data, i);

		if (request_len==0) {
		  close(clients[i]);
		  removeClient(i--);
		}
	      }
	  }

	  if FD_ISSET(ant_fd, &fdset) {
	      char c;
	      int r=read(ant_fd,&c,1);
	      if (r)
		ANT_ReceiveByte(c);	            	      
	    }

	  if FD_ISSET(server_fd, &fdset) {
	      int newsock = accept(server_fd, NULL, NULL);
	      if(newsock < 0) 
		perror("accept");
		
	      fcntl(newsock, F_SETFL, O_NONBLOCK);

	      if (num_clients < MAX_CLIENTS) {
		clients[num_clients++] = newsock;		
		compute_max_fd();
	      } else {
		DEBUG_ERRORS("too many clients\n");
		close(newsock);
	      }
	    }
	}	  
		
	ANT_Close(0);	
	ANT_ResetSystem();

	close(ant_fd);
	close(server_fd);
	fprintf(stderr,"Goodbye\n");
	return 0;
}
