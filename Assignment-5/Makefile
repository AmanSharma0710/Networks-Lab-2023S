SHELL = /bin/sh
CC = gcc
CFLAGS = -Wall
LIBNAME = libmsocket.a
LIBRARY_PATH = /usr/include/lib
OBJS = mysocket.o
HEADERS = mysocket.h
FILE = mysocket.c
HEADER_PATH = /usr/include
RM = rm -f

$(LIBNAME): $(OBJS)
	$(AR) rcs $@ $^

OBJS: $(FILE)
	$(CC) $(CFLAGS) -c $^

install: $(LIBNAME)
	mkdir -p $(LIBRARY_PATH)
	cp $(LIBNAME) $(LIBRARY_PATH)/

headers: $(HEADERS)
	cp $(HEADERS) $(HEADER_PATH)

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) $(LIBNAME)