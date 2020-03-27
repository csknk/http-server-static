CFLAGS = -std=gnu99 -W -Wall -pedantic -g
CC = gcc ${CFLAGS}
BIN = bin
NAME = http-server

$(NAME): ${BIN}/main.o ${BIN}/server.o ${BIN}/errors.o
	$(CC) -o ${BIN}/$@ $^

${BIN}/%.o: %.c
	$(CC) -c -o $@ $<

server.o: server.h
errors.o: errors.h

.PHONY: clean
clean:
	-rm ${BIN}/$(NAME) ${BIN}/*.o

