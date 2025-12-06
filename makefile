CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lm
TARGET = scm3
SRC = main.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c scm3.h
	$(CC) $(CFLAGS) -c $<

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: check
check:
	cppcheck --enable=warning,performance,portability --std=c17 --library=posix -j4 .