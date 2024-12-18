
CC := g++
FLAGS := -fPIC -g -Wall -std=c++11 -I.

CPPFLAGS=$(FLAGS) $(DEFINES)
CFLAGS=$(FLAGS) $(DEFINES)
SRCS := $(shell find ./ -name "*.cpp")
HEADERS := $(shell find ./ -name "*.h")
OBJS := $(SRCS:.cpp=.o) 

BINARIES := ../../cgi-bin/history-cgi ../../cgi-bin/events-cgi ../../cgi-bin/stats-cgi ../../cgi-bin/status-cgi ../../cgi-bin/command-cgi ../../cgi-bin/program-cgi

all: Makefile.deps ../../cgi-bin $(BINARIES)

../../cgi-bin: 
	mkdir ../../cgi-bin

-include Makefile.deps
 
Makefile.deps: $(SRCS) $(HEADERS)
	makedepend -- $(CPPFLAGS) -- $(SRCS) -f- > Makefile.deps 2> /dev/null

../../cgi-bin/history-cgi: history_cgi.o ../src/history.o ../src/pithermoutils.o ../src/logger.o ../src/logitem.o ../src/historyitem.o ../src/pithermotimer.o
	g++ -o $@ $^ -lpthread -lrt -lm

../../cgi-bin/events-cgi: events_cgi.o ../src/logger.o ../src/logitem.o ../src/pithermotimer.o ../src/pithermoutils.o
	g++ -o $@ $^ -lpthread -lrt -lm

../../cgi-bin/stats-cgi: stats_cgi.o ../src/logger.o ../src/history.o ../src/pithermoutils.o ../src/pithermotimer.o ../src/logitem.o ../src/historyitem.o
	g++ -o $@ $^ -lpthread -lrt -lm

../../cgi-bin/status-cgi: status_cgi.o ../src/sharedmemory.o ../src/pithermotimer.o
	g++ -o $@ $^ -lpthread -lrt -lm

../../cgi-bin/program-cgi: program_cgi.o ../src/sharedmemory.o ../src/pithermotimer.o
	g++ -o $@ $^ -lpthread -lrt -lm

../../cgi-bin/command-cgi: command_cgi.o ../src/sharedmemory.o ../src/pithermotimer.o ../src/pithermoutils.o
	g++ -o $@ $^ -lpthread -lrt -lm


.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f $(BINARIES) 
	rm -f Makefile.deps

