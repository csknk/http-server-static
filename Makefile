CFLAGS = -std=gnu99 -fPIE -W -Wall -pedantic -g
CC = gcc ${CFLAGS}
BIN = bin
NAME = http-server

$(NAME): ${BIN}/main.o ${BIN}/server.o ${BIN}/errors.o ${BIN}/string-utilities.o
	$(CC) -o ${BIN}/$@ $^

${BIN}/%.o: %.c
	$(CC) -c -o $@ $<

server.o: server.h
errors.o: errors.h
string-utilities.o: string-utilities.h

.PHONY: clean
clean:
	-rm ${BIN}/$(NAME) ${BIN}/*.o

