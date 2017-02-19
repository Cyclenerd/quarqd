#CC=arm-apple-darwin-gcc # uncomment for iphone build

OUTPUT_DIR ?= .
ARCH ?= 
STRIP ?= /usr/bin/strip

BINDIR = /usr/local/bin
CFLAGS ?= -O2 -Wall -Wundef -Wstrict-prototypes -g $(ARCH) #--std=c99 
OBJS = 	$(OUTPUT_DIR)/main.o $(OUTPUT_DIR)/ant.o \
	$(OUTPUT_DIR)/main.o \
	$(OUTPUT_DIR)/ant_channel.o \
	$(OUTPUT_DIR)/channel_manager.o \
	$(OUTPUT_DIR)/configuration.o 

PROGRAM = $(OUTPUT_DIR)/quarqd$(EXT)

ifeq "$(VERSION)" "DEBUG"
CFLAGS += -O0 -g -DDEBUG
endif

ifdef QUARQ_BUILD	
OBJS += $(OUTPUT_DIR)/ant_quarq.o
endif

all : $(PROGRAM) generated-schema.sql

$(PROGRAM) : $(OBJS)
	$(CC) $(ARCH) -static-libgcc -o $@ $^ $(LIBS)
	$(STRIP) $(PROGRAM)

install:
	install -s $(PROGRAM) $(BINDIR)

$(OUTPUT_DIR)/%.o : %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(PROGRAM) *~ TAGS
	rm -f generated-debug.c generated-messages.h generated-xml.c generated-schema.sql generated-header.h

TAGS: *.c *.h
	etags *.c *.h

PY_SCRIPTS=../../qapy
ANT_DEBUG_CREATOR = $(PY_SCRIPTS)/quarqd_messages.py
MESSAGE_FORMAT=$(PY_SCRIPTS)/message_format.txt
ANT_MESSAGES = $(PY_SCRIPTS)/ant_messages.py

GENERATED_PREREQUISITES=$(ANT_DEBUG_CREATOR) $(ANT_MESSAGES) $(MESSAGE_FORMAT)

generated-debug.c: $(GENERATED_PREREQUISITES)
	python $(ANT_DEBUG_CREATOR) --debug.c < $(MESSAGE_FORMAT) > generated-debug.c

generated-header.h: $(GENERATED_PREREQUISITES)
	python $(ANT_DEBUG_CREATOR) --header1 < $(MESSAGE_FORMAT) > generated-header.h

generated-messages.h: $(GENERATED_PREREQUISITES)
	python $(ANT_DEBUG_CREATOR) --header < $(MESSAGE_FORMAT) > generated-messages.h

generated-xml.c: $(GENERATED_PREREQUISITES)
	python $(ANT_DEBUG_CREATOR) --xml_messages < $(MESSAGE_FORMAT) > generated-xml.c

generated-schema.sql: $(GENERATED_PREREQUISITES)
	python $(ANT_DEBUG_CREATOR) --sql < $(MESSAGE_FORMAT) > generated-schema.sql

ant_quarq.h: 
	touch ant_quarq.h

ant_channel.h: generated-messages.h generated-header.h ant_quarq.h

svn_version.h:
	( echo "#define QUARQD_VERSION `svn info | grep Revision | awk '{print $$2}'`"  ) > svn_version.h  

$(OUTPUT_DIR)/ant_channel.o: ant_channel.c ant_channel.h ant.h generated-header.h generated-messages.h generated-debug.c generated-xml.c quarqd.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_DIR)/main.o: main.c ant_channel.h ant.h generated-xml.c quarqd.h svn_version.h
	$(CC) $(CFLAGS) -c $< -o $@

test: quarqd
	./quarqd < test.dat | head -100
