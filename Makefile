# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile

CFLAGS = -Wall -g

# Portul pe care asculta serverul (de completat)
PORT = 

# Adresa IP a serverului (de completat)
IP_SERVER = 

all: server subscriber

helpers: helpers.c

# Compileaza server.c
server: server.c 
	gcc $(CFLAGS) $^ -o server -lm

# Compileaza client.c
subscriber: subscriber.c
	gcc $(CFLAGS) $^ -o subscriber -lm
# Compileaza clientudp.c

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_client:
	./client ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber clientudp
