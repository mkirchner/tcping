FILES=README cli.c tcping.h tcping.c Makefile LICENSE
VERNUM=`grep VERSION tcping.c | cut -d" " -f3`
VER=tcping-$(VERNUM)

CCFLAGS=-Wall
CC=gcc

tcping.linux: tcping.c
	$(CC) -o tcping $(CCFLAGS) cli.c tcping.c

tcping.macos: tcping.linux

tcping.openbsd: tcping.linux

readme: man/tcping.1
	groff -man -Tascii man/tcping.1 | col -bx > README

clean:
	rm -f tcping core *.o
	rm -rf debian/

dist:
	mkdir $(VER)
	mkdir $(VER)/man
	cp $(FILES) $(VER)/
	cp man/tcping.1 $(VER)/man/
	tar cvzf $(VER).tar.gz $(VER)
