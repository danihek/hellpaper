# Makefile for Hellpaper

CC = gcc
SRC = hellpaper.c

TARGET = hellpaper
CFLAGS = -Wall -O2

#- mac os need some different flags for raylib 
#- see https://github.com/raysan5/raylib/wiki/Working-on-macOS#:~:text=If-,the,-build%20fails%2C%20you
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S), Darwin)
    CFLAGS += -I$(shell brew --prefix raylib)/include
    LIBS = -lraylib -lm -L$(shell brew --prefix raylib)/lib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
else
    LIBS = -lraylib -lm #-lGL -lpthread -ldl -lrt
endif


PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	@echo "Cleaning up..."
	rm -f $(TARGET)

install: $(TARGET)
	mkdir -p $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)

uninstall:
	rm -f $(BINDIR)/$(TARGET)

.PHONY: all clean install uninstall
