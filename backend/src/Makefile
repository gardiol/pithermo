
CC := g++
FLAGS := -fPIC -g -Wall -std=c++11 -I.

ifndef DEMO
$(info Build per TARGET - DEMO not specified)
DEFINES=
DEMO_TGT=
WPI_LIBS="-lwiringPi"
else
$(info Build per PC - DEMO specified)
DEFINES := -DDEMO
DEMO_TGT=test_temp
WPI_LIBS=
endif

CPPFLAGS=$(FLAGS) $(DEFINES)
CFLAGS=$(FLAGS) $(DEFINES)
SRCS := $(shell find ./ -name "*.cpp")
HEADERS := $(shell find ./ -name "*.h")
OBJS := $(SRCS:.cpp=.o) 

BINARIES := ../../bin/pithermo_daemon

all: ../../bin $(BINARIES)

../../bin: 
	mkdir ../../bin

-include Makefile.deps
 
Makefile.deps: $(SRCS) $(HEADERS)
	makedepend -- $(CPPFLAGS) -- $(SRCS) -f- > Makefile.deps 2> /dev/null

../../bin/pithermo_daemon: Makefile.deps $(OBJS)
	g++ -o $@ $(OBJS) -lpthread -lrt -lm -lmosquitto -L/usr/local/lib ${WPI_LIBS}


.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f $(BINARIES) 
	rm -f Makefile.deps

