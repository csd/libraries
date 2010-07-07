srcdir = .
AR = ar
AS = as
CC = gcc
CFLAGS = -g -O2
CPP = gcc -E
CPPFLAGS = 
DEFS = -DHAVE_CONFIG_H
DEPDIR = .deps

LIBS = ../x264API/libx264api.a ../../libtidx264.a -lm -lpthread
 
SRC = x264client.c

OBJ = $(SRC:.c=.o)

DEFAULT_INCLUDES =  -I. -I$(srcdir) -I../x264API -I../ -I../../
INCLUDES = $(DEFAULT_INCLUDES)

COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CFLAGS)

all: default

.SUFFIXES:
.SUFFIXES: .c .o

default: x264client

.c.o:
	$(COMPILE) -c $< -o $@

x264client: $(OBJ) 
	$(CC) -o $@ $+ $(LDFLAGS) $(LIBS)

clean:
	rm -f $(OBJ) x264client
