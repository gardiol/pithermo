CFLAGS=-fPIC -g

FRAMEWORK_SRCS := ./framework/basecomminterface.cpp ./framework/basemutex.cpp ./framework/basesemaphor.cpp ./framework/basethread.cpp ./framework/circularbuffer.cpp ./framework/cmdlineparser.cpp ./framework/configdata.cpp ./framework/configfile.cpp ./framework/debugprint.cpp ./framework/udpsocket.cpp ./framework/variant.cpp ./framework/variantlive.cpp ./framework/variantstatic.cpp ./framework/frameworksighandler.cpp ./framework/frameworktimer.cpp ./framework/frameworkutils.cpp ./framework/genericcommdevice.cpp ./framework/memorychecker.cpp ./framework/profiler.cpp ./framework/profilerinternal.cpp ./framework/scheduledthread.cpp ./framework/socketcommdevice.cpp ./framework/tcpclient.cpp ./framework/tcpserver.cpp ./framework/transmissionstats.cpp ./framework/typeinfo.cpp
FRAMEWORK_OBJS := $(foreach src,$(FRAMEWORK_SRCS),obj/$(basename $(notdir $(src))).o)
$(foreach o,$(FRAMEWORK_OBJS),$(eval FRAMEWORK_MAPS_$(basename $(notdir $(o))) := framework/$(basename $(notdir $(o))).cpp))

all: obj pithermo_daemon

obj:
	mkdir obj

$(FRAMEWORK_OBJS): ${FRAMEWORK_SRCS}
	g++ $(CFLAGS) -o $@ -c $(FRAMEWORK_MAPS_$(basename $(notdir $@))) -Iframework

obj/main.o: src/main.cpp
	g++ $(CFLAGS) -o $@ -c $^ -Iframework

obj/runnerthread.o: src/runnerthread.cpp
	g++ $(CFLAGS) -o $@ -c $^ -Iframework

obj/command.o: src/command.cpp
	g++ $(CFLAGS) -o $@ -c $^ -Iframework

obj/program.o: src/program.cpp
	g++ $(CFLAGS) -o $@ -c $^ -Iframework

pithermo_daemon:  ${FRAMEWORK_OBJS} obj/main.o obj/runnerthread.o obj/command.o obj/program.o
	g++ -o $@ $^ -lpthread -lrt 

.PHONY: clean

clean:
	rm -rf obj
	rm -f pithermo_daemon
