FILES=README cli.c tcping.h tcping.c Makefile LICENSE
VERNUM=$$(grep TCPING_VERSION tcping.h | cut -d" " -f3 | sed -e 's/"//g')
VER=tcping-$(VERNUM)

CCFLAGS=-Wall
CC=gcc

tcping.linux: tcping.c
	$(CC) -o tcping $(CCFLAGS) cli.c tcping.c

tcping.macos: tcping.linux

tcping.openbsd: tcping.linux

readme: man/tcping.1
	groff -man -Tascii man/tcping.1 | col -bx > README

.PHONY: clean dist
clean:
	rm -f tcping core *.o

dist:
	mkdir $(VER)
	mkdir $(VER)/man
	cp $(FILES) $(VER)/
	cp man/tcping.1 $(VER)/man/
	tar cvzf $(VER).tar.gz $(VER)
	rm -rf $(VER)
