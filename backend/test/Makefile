
CC := g++
FLAGS := -fPIC -g -Wall -std=c++11 -I.

ifndef DEMO
$(info Build per TARGET - DEMO not specified)
DEFINES=
WPI_LIBS="-lwiringPi"
WPI_OBJ=
else
$(info Build per PC - DEMO specified)
DEFINES := -DDEMO
WPI_LIBS=
WPI_OBJ=../src/demowiringpi.o
endif

CPPFLAGS=$(FLAGS) $(DEFINES)
CFLAGS=$(FLAGS) $(DEFINES)
SRCS := $(shell find ./ -name "*.cpp")
HEADERS := $(shell find ./ -name "*.h")
OBJS := $(SRCS:.cpp=.o) 

BINARIES := ../../bin/read_dht22 ../../bin/test_temp

all: Makefile.deps ../../bin $(BINARIES) 

../../bin:
	mkdir ../../bin

-include Makefile.deps
 
Makefile.deps: $(SRCS) $(HEADERS)
	makedepend -- $(CPPFLAGS) -- $(SRCS) -f- > Makefile.deps 2> /dev/null

../../bin/read_dht22: read_dht22.o ../src/pithermotimer.o ../src/tempsensor.o ../src/pithermoutils.o ../src/logger.o ../src/gpiodevice.o ../src/logitem.o ${WPI_OBJ}
	g++ -o $@ $^ -lpthread -lrt -lm ${WPI_LIBS}

../../bin/test_temp: test_temp.o ../src/pithermotimer.o ../src/tempsensor.o ../src/pithermoutils.o ../src/logger.o ../src/gpiodevice.o ../src/logitem.o ${WPI_OBJ}
	g++ -o $@ $^ -lpthread -lrt -lm


.PHONY: clean

clean:
	rm -f $(OBJS)
	rm -f $(BINARIES) 
	rm -f Makefile.deps

