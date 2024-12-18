ifndef DEMO
$(info Build per TARGET - DEMO not specified)
else
$(info Build per PC - DEMO specified)
endif

MODULES := pithermo_daemon CGI test

all: ../bin ../cgi-bin $(MODULES)

../bin:
	mkdir ../bin

../cgi-bin:
	mkdir ../cgi-bin

pithermo_daemon:
	cd src && $(MAKE)

CGI:
	cd CGI && $(MAKE)

test:
	cd test && $(MAKE)

.PHONY: clean $(MODULES)

clean:
	cd src && $(MAKE) clean
	cd CGI && $(MAKE) clean
	cd test && $(MAKE) clean
	rm -rf ../bin
	rm -rf ../cgi-bin
