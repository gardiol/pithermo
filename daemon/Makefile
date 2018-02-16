ifndef NOPI
$(info Build per TARGET - NOPI not specified)
CFLAGS=-fPIC -g
else
$(info Build per PC - NOPI specified)
CFLAGS=-fPIC -g -DNOPI
endif

FRAMEWORK_SRCS := ./framework/basecomminterface.cpp ./framework/basemutex.cpp ./framework/basesemaphor.cpp ./framework/basethread.cpp ./framework/circularbuffer.cpp ./framework/cmdlineparser.cpp ./framework/configdata.cpp ./framework/configfile.cpp ./framework/debugprint.cpp ./framework/udpsocket.cpp ./framework/variant.cpp ./framework/variantlive.cpp ./framework/variantstatic.cpp ./framework/frameworksighandler.cpp ./framework/frameworktimer.cpp ./framework/frameworkutils.cpp ./framework/genericcommdevice.cpp ./framework/memorychecker.cpp ./framework/profiler.cpp ./framework/profilerinternal.cpp ./framework/scheduledthread.cpp ./framework/socketcommdevice.cpp ./framework/tcpclient.cpp ./framework/tcpserver.cpp ./framework/transmissionstats.cpp ./framework/typeinfo.cpp
FRAMEWORK_OBJS := $(foreach src,$(FRAMEWORK_SRCS),obj/$(basename $(notdir $(src))).o)
$(foreach o,$(FRAMEWORK_OBJS),$(eval FRAMEWORK_MAPS_$(basename $(notdir $(o))) := framework/$(basename $(notdir $(o))).cpp))

all: obj pithermo_daemon

obj:
	mkdir obj

$(FRAMEWORK_OBJS): ${FRAMEWORK_SRCS}
	g++ $(CFLAGS) -o $@ -c $(FRAMEWORK_MAPS_$(basename $(notdir $@))) -Iframework

obj/main.o: src/main.cpp src/runnerthread.h src/command.h src/logitem.h src/logger.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework -I/usr/local/include

obj/runnerthread.o: src/runnerthread.cpp src/command.h src/program.h src/runnerthread.h src/logitem.h src/tempsensor.h src/gpiodevice.h src/logger.h src/generator.h
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

obj/gpiodevice.o: src/gpiodevice.cpp src/gpiodevice.h -I/usr/local/include src/logger.h src/logitem.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework

obj/tempsensor.o: src/tempsensor.cpp src/tempsensor.h src/gpiodevice.h src/logger.h src/logitem.h
	g++ $(CFLAGS) -o $@ -c $< -Iframework


pithermo_daemon:  ${FRAMEWORK_OBJS} obj/main.o obj/runnerthread.o obj/command.o obj/program.o obj/historyitem.o obj/logger.o obj/history.o obj/logitem.o obj/generator.o obj/gpiodevice.o obj/tempsensor.o
	g++ -o $@ $^ -lpthread -lrt -lm -L/usr/local/lib -lwiringPi

.PHONY: clean

clean:
	rm -rf obj
	rm -f pithermo_daemon
