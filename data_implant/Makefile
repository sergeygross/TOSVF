CC = gcc

pkg_packages = libavformat libavcodec libswscale libavutil

PKG_CFLAGS = $(shell pkg-config --cflags $(pkg_packages)) -D__STDC_CONSTANT_MACROS
PKG_LDFLAGS = $(shell pkg-config --libs $(pkg_packages))

CFLAGS = -W -Wall -g -Wextra -fPIC $(PKG_CFLAGS) -I/usr/include/ -I. 
LDFLAGS += $(PKG_LDFLAGS) -lz -lpthread -L/usr/lib/

SRC =  main.c network_nal2rtp.c filter_add_data.c other/list.c

TARGET = encapsulate_sei_stream

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)
	rm -f *.o
