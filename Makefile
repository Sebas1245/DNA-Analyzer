CFLAGS = -O
CC = gcc

SRC = server.c
OBJ = $(SRC:.c = .o)

miniDBMS: $(OBJ)
	$(CC) $(CFLAGS) -o server.o $(OBJ) 