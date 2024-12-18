
CC := gcc
FLAGS := -fPIC -g -Wall

CFLAGS=$(FLAGS) $(DEFINES)
SRCS := $(shell find ./ -name "*.c")
HEADERS := $(shell find ./ -name "*.h")
OBJS := $(SRCS:.c=.o) 

BINARIES := end_conv ext_add gen_hist history_extract history_purge read_hist

all: Makefile.deps $(BINARIES)

-include Makefile.deps
 
Makefile.deps: $(SRCS) $(HEADERS)
	makedepend -- $(CFLAGS) -- $(SRCS) -f- > Makefile.deps 2> /dev/null

end_conv: end_conv.o
	gcc -o $@ $^ 

ext_add: ext_add.o
	gcc -o $@ $^ 

gen_hist: gen_hist.o
	gcc -o $@ $^ 

history_extract: history_extract.o
	gcc -o $@ $^ 

history_purge: history_purge.o
	gcc -o $@ $^ 

read_hist: read_hist.o
	gcc -o $@ $^ 


.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f $(BINARIES) 
	rm -f Makefile.deps

