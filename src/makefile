CC = gcc
CFLAGS = -g -O3 -I/usr/bin/include/
LDFLAGS = -L/usr/bin/lib/
LDLIBS = -lflint -lgmp -lmpfr -lsodium

TARGET = main
SRCS = matrix.c utils.c params.c main.c 
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)