override CFLAGS=-Wall -Wextra -Wshadow -fanalyzer -g -O0 -fsanitize=address,undefined

ifdef CI
override CFLAGS=-Wall -Wextra -Wshadow -Werror
endif

.PHONY: clean all

all: server client

server: server.c
	gcc $(CFLAGS) -o server server.c
	
client: client.c
	gcc $(CFLAGS) -o client client.c

clean:
	rm -f sop-chat
