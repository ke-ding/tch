SRCS    := $(wildcard ../lib/*.c)
SRCS    := $(SRCS) $(wildcard *.c)
OBJS    := $(patsubst %.c, %.o, $(SRCS))

HDRS	:= $(wildcard ../include/*.h)
HDRS	:= $(HDRS) $(wildcard *.h)

TARGET	:= ../srv
WARN    := -Wall
INCLUDE := -I. -I../include
LIBRARY := -L../lib
DEBUG   :=  -D__DEBUG__
#CFLAGS  := -O2 ${WARN} ${INCLUDE}
CFLAGS  := -O0 -g ${WARN} ${INCLUDE}
LDFLAGS := ${LIBRARY} -rdynamic -lpthread -lrt
#LDFLAGS := ${LIBRARY} --static -rdynamic -lpthread -lrt
#CC      := arm-linux-gnueabihf-gcc
CC      := gcc

bin: ${TARGET}
all: tags ${TARGET}

${TARGET}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}

%.o: %.c
	${CC} ${CFLAGS} ${DEBUG} -c -o $@ $<

tags: ${SRCS} ${HDRS}
	ctags ${SRCS} ${HDRS}
	find -name "*.[chsS]" > cscope.files
	cscope -qRbki cscope.files

rebuild: clean all

.PHONY: clean
clean:
	rm -rf ${TARGET} ${OBJS} tags cscope.*
