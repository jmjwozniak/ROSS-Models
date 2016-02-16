
# USER: Edit these locations
ROSS = $(HOME)/sfw/ROSS
MPI     := $(HOME)/sfw/mpich-master

ROSS_CONFIG := $(ROSS)/bin/ross-config
CC	:= $(shell $(ROSS_CONFIG) --cc)
CFLAGS  := -g -Wall -I. -I/include -I$(MPI)/include
          # -I$(HOME)/proj/ROSS/code/core
OPTIONS := $(shell $(ROSS_CONFIG) --cflags)
LDFLAGS := $(shell $(ROSS_CONFIG) --ldflags)
ROSS_LIB := $(ROSS)/lib/libROSS.a

all: airport
clean:
	rm -f airport
	rm -f *.o *~ core*



airport: airport.c $(ROSS_LIB) $(ROSS_CONFIG)
	$(CC) $(CFLAGS) $(OPTIONS) airport.c -o airport $(ROSS_LIB) $(LDFLAGS) -L$(MPI)/lib -lmpi -lm -Wl,-rpath -Wl,$(MPI)/lib
