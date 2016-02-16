CCC=gcc
CPP=g++

COMMON_INC= -I. -I./common-utils -I./apache-utils -I./shm_core -I./log-utils \
-I./stomp-utils -I./xml-core -I./http-utils

C_SRC=./log-utils/logging.c ./log-utils/log_core.c ./common-utils/common_utils.c  \
./shm_core/shm_dup.c ./shm_core/shm_data.c  ./shm_core/shm_mutex.c \
./shm_core/shm_datatypes.c ./shm_core/shm_apr.c ./http-utils/http_client.c \
./stomp-utils/stomp_utils.c ./stomp-utils/stomp.c \
./xml-core/xml_core.c ./xml-core/token_utils.c \
./config_core2.c ./config_messaging_parsing2.c ./config_messaging2.c ./config_bindings2.c ./config_error2.c \
./switch_config_core.c ./switch_config_xml.c ./switch_config.c \
./db_core2.c ./libswitch_conf.c ./switch_types.c ./sqlite3.c

C_SCSRC=./send_command.c
 
COMMON_CFLAGS=-fPIC -DLINUX=2 -DSHARED_MODULE -D_REENTRANT -DEAPI -DTHREADSAFE=1 -DMSG_PROC_RESTART

OSTYPE:= $(shell uname)
ifeq ($(OSTYPE),Darwin)
		INC=-I/usr/include/apr-1 $(COMMON_INC)
		CFLAGS=$(COMMON_CFLAGS) -DMACOSX
		LDFLAGS=-L/usr/lib -lpthread -lm -ldl -lapr-1 -laprutil-1 -lcrypto -lz -lcurl -lexpat -lpcreposix -lpcre
else

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
		INC=-I/usr/include/apr-1 $(COMMON_INC)
		CFLAGS=$(COMMON_CFLAGS) -D_GNU_SOURCE
		LDFLAGS=-lexpat -lcurl -lpcreposix -lpcre -lapr-1 -laprutil-1 -luuid
else
		INC=-I/usr/include/apr-0 $(COMMON_INC)
		CFLAGS=$(COMMON_CFLAGS) -D_GNU_SOURCE
		LDFLAGS=-lexpat -lcurl -lpcreposix -lpcre -lidn -lapr-0 -laprutil-0 -luuid
endif

    
endif

C_OBJ = ${C_SRC:.c=.o}
C_SCOBJ = ${C_SCSRC:.c=.o}

# clear out all suffixes
.SUFFIXES:

# list only those we use
.SUFFIXES: .o .c .cpp

# define a suffix rule for .c -> .o
.c.o:
	$(CCC) $(CFLAGS) $(INC) -c $< -o $*.o
	
all: clean switch switch_command

clean:
	rm -f core $(C_OBJ) $(C_SCOBJ) libswitch.so sc

switch: $(C_OBJ)
	ar rcs libswitch.a $(C_OBJ)
		
switch_command: $(C_SCOBJ) $(C_OBJ)
	$(CCC) -o sc $(C_SCOBJ) $(C_OBJ) $(LDFLAGS)

test.o:
	$(CCC) -g -I/usr/include -c ./test.c
	
test: test.o
	$(CCC) -o test test.o -L/usr/lib -lpthread -Wl,-rpath ./  -L./ -lswitch $(LDFLAGS)
	
