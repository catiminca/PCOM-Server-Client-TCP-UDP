CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

# Portul pe care asculta serverul
PORT = 12345

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

all: server subscriber

common.o: common.cpp

# Compileaza server.c
server: server.cpp common.o

# Compileaza client.c
subscriber: subscriber.cpp common.o

.PHONY: clean run_server run-subscriber

# Ruleaza serverul
run_server:
	./server ${IP_SERVER} ${PORT}

# Ruleaza clientul 	
run-subscriber:
	./subscriber ${IP_SERVER} ${PORT}

clean:
	rm -rf server subscriber *.o *.dSYM