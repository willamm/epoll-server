SOURCES := main.c server.c
OBJECTS := $(SOURCES:.c=.o) 
ASSEMBLY := $(SOURCES:.c=.s) # assembly files
POSTPROC := $(SOURCES:.c=.i) # files created by postprocessor

CFLAGS += -ggdb -Wall -Werror -save-temps

.PHONY: all clean

all: server

server: $(OBJECTS)
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $^

clean:
	rm $(OBJECTS) $(ASSEMBLY) $(POSTPROC) server
