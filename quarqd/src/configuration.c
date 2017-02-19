#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include "configuration.h"

configuration_t quarqd_config;

FILE *fopen_dotfile(char *filename, char *mode) {
#define PATH_LENGTH 4096
  char path [PATH_LENGTH];

  strncpy(path,getenv("HOME"), PATH_LENGTH-(2+strlen(filename)));
  strcat(path,"/");
  strcat(path,filename);

  return fopen(path,mode);

}

#define SET_VAL(st) \
    int val; \
    errno=0; \
    /* return if we can't parse value to int */		\
    if ((val=strtol(st, NULL, 0)) && errno) return 0 

int parse_config(char *name, char *value) {

  DEBUG_CONFIG_PARSE("Looking at name %s\n value %s\n",name,value);

  if (!strcmp(name,"QUARQD_PORT")) {
    SET_VAL(value);
    quarqd_config.quarqd_port=val;
    
  } else if (!strcmp(name,"ANT_TTY")) {
    quarqd_config.ant_tty=strdup(value);
    
  } else if (!strcmp(name,"ANT_BAUDRATE")) {
    SET_VAL(value);
    quarqd_config.ant_baudrate=val;

  } else if (!strcmp(name,"QUARQD_DEBUG")) {
    SET_VAL(value);
    quarqd_config.debug_level=val;

  } else return 0;
  return 1;
}
#undef SET_VAL

int read_config_dotfile(void) {
  FILE *fd=fopen_dotfile(".quarqd_config","r");
  
  char buf[4096];

  if (!fd) {
    DEBUG_ERRORS(".quarqd_config open failed\n");
    return 0;
  }

  int l=fread(buf, 1,4096,fd);

  char *line;
  char *last_line;

  // sorry about this grotty parser.
  
  // first, split lines using strtok
  // then look for the name=val pair on each line
  
  line = strtok_r(buf, "\n\r", &last_line); 
  if (line==NULL) return 0;
  
  do {
    char *name;
    char *val;
    char *c=line;
    // first skip nonalpha characters
    while (!isalpha(*c) && (*c) && (c<(buf+l))) c++;
    name=c;
    // look for '=' separator
    while ((*c!='=') && (*c) && (c<(buf+l))) c++; 
    if (*c==0) continue; // no '=' found

    // make the '=' into '\0'
    *c=0;
    val=++c;
    // now go till nonalphanum
    while (((!isspace(*c)) || (*c == '/') || (*c == '.')) && 
	   (*c) && (c<(buf+l)) && (*c != '#')) c++;
    // make the offending char into '\0'
    *c=0;

#ifdef DEBUG
    fprintf(stderr, "name is |%s|\n",name);
    fprintf(stderr, "val is |%s|\n",val);
#endif        

    parse_config(name,val);

  } while ((line = strtok_r(NULL, "\n\r", &last_line)));

  return 1;
}

#define CHECK_ENV(n) c=getenv(n); \
  if (c) { \
    parse_config(n,c); \
    ret = 1; \
    } 
  
int read_config_env(void) {
  char *c;
  int ret=0;

  CHECK_ENV("QUARQD_PORT");
  CHECK_ENV("ANT_TTY");
  CHECK_ENV("ANT_BAUDRATE");
  CHECK_ENV("QUARQD_DEBUG");

  return ret;
}

#undef CHECK_ENV

void init_config(void) {
  
  // first, the hardcoded defaults
  quarqd_config.quarqd_port=8168;
  quarqd_config.ant_tty="/dev/ttyANT";
  quarqd_config.ant_baudrate=19200;
  quarqd_config.debug_level=1;

  // next, the config file
  read_config_dotfile();

  // next, the environment variables
  read_config_env(); 
}

int read_offset_log(int device_id, float *offset) {

  FILE *rfd=fopen_dotfile(".quarqd_offset_log","r");
  char buf[4096];
  char srm_id[128];

  snprintf(srm_id,128,"srm_%d",device_id);

  if (!rfd) {
    DEBUG_ERRORS(".quard_offset_log open failed\n");
    return 0;
  }

  int l=fread(buf, 1,4096,rfd);

  char *line;
  char *last_line;

  // first, split lines using strtok
  // then look for the name=val pair on each line
  
  line = strtok_r(buf, "\n\r", &last_line); 
  if (line==NULL) return 0;
  
  do {
    char *name;
    char *val;
    char *c=line;
    // first skip nonalpha characters
    while (!isalpha(*c) && (*c) && (c<(buf+l))) c++;
    name=c;
    // look for '=' separator
    while ((*c!='=') && (*c) && (c<(buf+l))) c++; 
    if (*c==0) continue; // no '=' found

    // make the '=' into '\0'
    *c=0;
    val=++c;
    // now go till nonalphanum
    while (((!isspace(*c)) || (*c == '/') || (*c == '.')) && 
	   (*c) && (c<(buf+l)) && (*c != '#')) c++;
    // make the offending char into '\0'
    *c=0;
    if (strcmp(name,srm_id)) {
      *offset=strtod(val, NULL);
      fclose(rfd);
      return 1;
    }

  } while ((line = strtok_r(NULL, "\n\r", &last_line)));
  
  fclose(rfd);  
  return 0;
}

int update_offset_log(int device_id, float offset) {
  FILE *rfd=fopen_dotfile(".quarqd_offset_log","r");
  FILE *wfd=fopen_dotfile(".quarqd_offset_log.new","w");

  char buf[4096];
  char srm_id[128];

  snprintf(srm_id,128,"srm_%d",device_id);

  if ((!rfd) || (!wfd)) {
    DEBUG_ERRORS(".quard_offset_log open failed\n");
    return 0;
  }

  int l=fread(buf, 1,4096,rfd);

  char *line;
  char *last_line;

  // first, split lines using strtok
  // then look for the name=val pair on each line
  
  line = strtok_r(buf, "\n\r", &last_line); 
  if (line==NULL) return 0;
  
  do {
    char *name;
    char *val;
    char *c=line;
    // first skip nonalpha characters
    while (!isalpha(*c) && (*c) && (c<(buf+l))) c++;
    name=c;
    // look for '=' separator
    while ((*c!='=') && (*c) && (c<(buf+l))) c++; 
    if (*c==0) continue; // no '=' found

    // make the '=' into '\0'
    *c=0;
    val=++c;
    // now go till nonalphanum
    while (((!isspace(*c)) || (*c == '/') || (*c == '.')) && 
	   (*c) && (c<(buf+l)) && (*c != '#')) c++;
    // make the offending char into '\0'
    *c=0;

#ifdef DEBUG
    fprintf(stderr, "name is |%s|\n",name);
    fprintf(stderr, "val is |%s|\n",val);
#endif        

    if (strcmp(name,srm_id)) {
      fprintf(wfd,"%s=%s\n\r",name,val);
    }

  } while ((line = strtok_r(NULL, "\n\r", &last_line)));

  fprintf(wfd,"%s=%f\n\r",srm_id,offset);
  
  fclose(wfd);
  fclose(rfd);
  
  rename(".quarqd_offset_log.new",".quarqd_offset_log");
  return 1;
}
