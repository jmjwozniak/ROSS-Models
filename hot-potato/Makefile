
# USER: Edit these locations
ROSS = $(HOME)/sfw/ROSS
MPI     := $(HOME)/sfw/mpich-master

ROSS_CONFIG := $(ROSS)/bin/ross-config
CC	:= $(shell $(ROSS_CONFIG) --cc)
OPTIONS := $(shell $(ROSS_CONFIG) --cflags)
INCLUDES := -I. -I/include -I$(MPI)/include
# -I $(HOME)/proj/codes-base/codes
CPPFLAGS := $(INCLUDES)
CFLAGS  := -g -Wall -std=gnu99 $(OPTIONS)
          # -I$(HOME)/proj/ROSS/code/core
LDFLAGS := $(shell $(ROSS_CONFIG) --ldflags)
ROSS_LIB := $(ROSS)/lib/libROSS.a

all:

clean:
	rm -fv hot-potato* *.o

# ~/proj/codes-base/src/util/rc-stack.o

hot-potato% : hp%.o $(ROSS_LIB) $(ROSS_CONFIG)
	$(CC) -o $(@)                      \
		$(CFLAGS)                  \
		$(OPTIONS)                 \
		$(<) $(ROSS_LIB)           \
		$(LDFLAGS)                 \
		-L$(MPI)/lib -lmpi -lm     \
		-Wl,-rpath -Wl,$(MPI)/lib
