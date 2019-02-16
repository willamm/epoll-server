SOURCES := main.c server.c
OBJECTS := $(SOURCES:.c=.o)

CFLAGS += -ggdb

.PHONY: all clean

all: server

server: $(OBJECTS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	rm $(OBJECTS) server
