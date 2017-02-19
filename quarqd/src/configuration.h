#ifndef CONFIGURATION_H
#define CONFIGURATION_H

typedef struct {
  int quarqd_port;
  char *ant_tty;
  int ant_baudrate;
  int debug_level;

} configuration_t;

enum { DEBUG_LEVEL_ERRORS=1,
       DEBUG_LEVEL_ANT_CONNECTION=2,
       DEBUG_LEVEL_ANT_MESSAGES=4,
       DEBUG_LEVEL_CONFIG_PARSE=8
};

extern configuration_t quarqd_config;

#define DEBUG_PRINTF(mask, format, args...) \
  if (quarqd_config.debug_level & mask) \
    fprintf(stderr, format, ##args)

#define DEBUG_ERRORS(format, args...) \
  DEBUG_PRINTF(DEBUG_LEVEL_ERRORS, format, ##args)

#define DEBUG_ANT_CONNECTION(format, args...)  \
  DEBUG_PRINTF(DEBUG_LEVEL_ANT_CONNECTION, format, ##args)

#define DEBUG_ANT_MESSAGES(format, args...)  \
  DEBUG_PRINTF(DEBUG_LEVEL_ANT_MESSAGES, format, ##args)

#define DEBUG_CONFIG_PARSE(format, args...)  \
  DEBUG_PRINTF(DEBUG_LEVEL_CONFIG_PARSE, format, ##args)

#include <stdio.h>

FILE *fopen_dotfile(char *filename, char *mode);

int read_offset_log(int device_id, float *offset);
int update_offset_log(int device_id, float offset);

void init_config(void);

#endif

