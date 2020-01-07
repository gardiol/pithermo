ifndef DEMO
$(info Build per TARGET - DEMO not specified)
CFLAGS=-fPIC -g -Wall -std=c++11
WPI_LIBS="-lwiringPi"
else
$(info Build per PC - DEMO specified)
CFLAGS=-fPIC -g -Wall -std=c++11 -DDEMO
DEMO_OBJ=obj/demowiringpi.o
DEMO_TGT=test_temp
WPI_LIBS=""
endif

FRAMEWORK_SRCS := ./framework/basecomminterface.cpp ./framework/basemutex.cpp ./framework/basethread.cpp ./framework/cmdlineparser.cpp ./framework/configdata.cpp ./framework/configfile.cpp ./framework/debugprint.cpp ./framework/udpsocket.cpp ./framework/frameworksighandler.cpp ./framework/frameworktimer.cpp ./framework/frameworkutils.cpp ./framework/genericcommdevice.cpp ./framework/memorychecker.cpp ./framework/profiler.cpp ./framework/profilerinternal.cpp ./framework/scheduledthread.cpp ./framework/socketcommdevice.cpp ./framework/tcpclient.cpp ./framework/tcpserver.cpp ./framework/transmissionstats.cpp ./framework/sharedmemory.cpp
FRAMEWORK_OBJS := $(foreach src,$(FRAMEWORK_SRCS),obj/$(basename $(notdir $(src))).o)
$(foreach o,$(FRAMEWORK_OBJS),$(eval FRAMEWORK_MAPS_$(basename $(notdir $(o))) := framework/$(basename $(notdir $(o))).cpp))

all: obj export_history pithermo_daemon ../cgi-bin/history ../cgi-bin/events ../cgi-bin/stats ../cgi-bin/status ../cgi-bin/command ../cgi-bin/program $(DEMO_TGT) read_dht22

obj:
	mkdir obj

$(FRAMEWORK_OBJS): ${FRAMEWORK_SRCS}
	g++ $(CFLAGS) -o $@ -c $(FRAMEWORK_MAPS_$(basename $(notdir $@))) -Iframework

obj/main.o: src/main.cpp src/runnerthread.h src/command.h src/logitem.h src/logger.h src/demowiringpi.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework -I/usr/local/include -Isrc

obj/read_dht22.o: test/read_dht22.cpp
	g++ $(CFLAGS) -o $@ -c $< -Iframework -Isrc 

obj/runnerthread.o: src/runnerthread.cpp src/command.h src/program.h src/runnerthread.h src/logitem.h src/tempsensor.h src/gpiodevice.h src/logger.h src/generator.h src/sharedstatus.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework 

obj/command.o: src/command.cpp src/command.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework

obj/program.o: src/program.cpp src/program.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework

obj/historyitem.o: src/historyitem.cpp src/historyitem.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework

obj/history.o: src/history.cpp src/history.h src/logger.h src/logitem.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework

obj/logger.o: src/logger.cpp src/logitem.h src/logger.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework

obj/logitem.o: src/logitem.cpp src/logitem.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework

obj/generator.o: src/generator.cpp src/generator.h src/logger.h src/logitem.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework

obj/gpiodevice.o: src/gpiodevice.cpp src/gpiodevice.h src/logger.h src/logitem.h src/demowiringpi.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework -I/usr/local/include -Isrc

obj/tempsensor.o: src/tempsensor.cpp src/tempsensor.h src/gpiodevice.h src/logger.h src/logitem.h src/demowiringpi.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework -Isrc

obj/demowiringpi.o: src/demowiringpi.cpp src/demowiringpi.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework -Isrc
	
obj/history_cgi.o: CGI/history_cgi.cpp
	g++ $(CFLAGS) -o $@ -c $< -Iframework -Isrc

obj/events_cgi.o: CGI/events_cgi.cpp
	g++ $(CFLAGS) -o $@ -c $< -Iframework -Isrc

obj/stats_cgi.o: CGI/stats_cgi.cpp
	g++ $(CFLAGS) -o $@ -c $< -Iframework -Isrc

obj/status_cgi.o: CGI/status_cgi.cpp src/sharedstatus.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework -Isrc

obj/command_cgi.o: CGI/command_cgi.cpp
	g++ $(CFLAGS) -o $@ -c $< -Iframework -Isrc

obj/program_cgi.o: CGI/program_cgi.cpp src/sharedstatus.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework -Isrc

obj/test_temp.o: test/test_temp.cpp src/logitem.h src/tempsensor.cpp src/tempsensor.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework -Isrc

obj/export_history.o: test/export_history.cpp
	g++ $(CFLAGS) -o $@ -c $< -Isrc

pithermo_daemon:  ${FRAMEWORK_OBJS} obj/main.o obj/runnerthread.o obj/command.o obj/program.o obj/historyitem.o obj/logger.o obj/history.o obj/logitem.o obj/generator.o obj/gpiodevice.o obj/tempsensor.o ${DEMO_OBJ}
	g++ -o $@ $^ -lpthread -lrt -lm -L/usr/local/lib ${WPI_LIBS}

../cgi-bin/history: obj/frameworktimer.o obj/frameworkutils.o obj/history_cgi.o obj/historyitem.o obj/history.o obj/debugprint.o obj/logitem.o obj/logger.o
	g++ -o $@ $^ -lpthread -lrt -lm

../cgi-bin/events: obj/frameworktimer.o obj/frameworkutils.o obj/events_cgi.o obj/logitem.o obj/logger.o obj/debugprint.o
	g++ -o $@ $^ -lpthread -lrt -lm

../cgi-bin/stats: obj/frameworktimer.o obj/frameworkutils.o obj/stats_cgi.o obj/logitem.o obj/logger.o obj/debugprint.o obj/historyitem.o obj/history.o
	g++ -o $@ $^ -lpthread -lrt -lm

../cgi-bin/status: obj/frameworktimer.o obj/frameworkutils.o obj/status_cgi.o obj/debugprint.o obj/sharedmemory.o
	g++ -o $@ $^ -lpthread -lrt -lm

../cgi-bin/program: obj/frameworktimer.o obj/frameworkutils.o obj/program_cgi.o obj/debugprint.o obj/udpsocket.o obj/basecomminterface.o obj/socketcommdevice.o obj/genericcommdevice.o obj/memorychecker.o obj/basemutex.o obj/transmissionstats.o obj/sharedmemory.o
	g++ -o $@ $^ -lpthread -lrt -lm

../cgi-bin/command: obj/frameworktimer.o obj/frameworkutils.o obj/command_cgi.o obj/debugprint.o obj/udpsocket.o obj/basecomminterface.o obj/socketcommdevice.o obj/genericcommdevice.o obj/memorychecker.o obj/basemutex.o obj/transmissionstats.o
	g++ -o $@ $^ -lpthread -lrt -lm

read_dht22: obj/read_dht22.o obj/frameworkutils.o obj/frameworktimer.o obj/debugprint.o obj/tempsensor.o obj/gpiodevice.o ${DEMO_OBJ} obj/logger.o obj/logitem.o
	g++ -o $@ $^ -lpthread -lrt -lm ${WPI_LIBS}

test_temp: obj/frameworktimer.o obj/frameworkutils.o obj/test_temp.o obj/tempsensor.o obj/gpiodevice.o obj/demowiringpi.o obj/debugprint.o obj/logger.o obj/logitem.o
	g++ -o $@ $^ -lpthread -lrt -lm

export_history: obj/historyitem.o obj/export_history.o
	g++ -o $@ $^ 

.PHONY: clean

clean:
	rm -rf obj
	rm -f pithermo_daemon
	rm -f ../cgi-bin/history
	rm -f ../cgi-bin/events
	rm -f ../cgi-bin/stats
