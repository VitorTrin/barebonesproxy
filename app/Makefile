# October 2006
# Prof.: Silvana Rossetto
#------------------------------------------------------------

VERSION = 1.0

#SERVER = server_thread.c
SERVER = proxy_server.c

SRV = proxy

#-------------------------------------------------------------
# the rest of the file should not change
INC = ../include
SRC = ../src
OBJ = ../obj
BIN = ../bin

CC = gcc -Wall
CFLAGS = -I$(INC) -DDEBUG_BUILD
LFLAGS = -lpthread
RM = rm

# make all
all:  $(OBJ)/mysocket.o $(BIN)/$(SRV) $(BIN)/$(CLI)

$(OBJ)/utils.o: $(SRC)/utils.c $(INC)/utils.h
	$(CC) -c $(CFLAGS) -o $@ $(SRC)/utils.c


$(OBJ)/mysocket.o: $(SRC)/mysocket.c $(INC)/mysocket.h
	$(CC) -c $(CFLAGS) -o $@ $(SRC)/mysocket.c

$(BIN)/$(SRV): $(OBJ)/mysocket.o $(OBJ)/utils.o $(SERVER)
	$(CC) $(CFLAGS) -o $@ $(OBJ)/mysocket.o $(OBJ)/utils.o $(SERVER) $(LFLAGS)

# version
version:
	@echo $(VERSION) "(Draft Version!)"
# clean
clean:
	$(RM) -rf $(OBJ)/*.o  $(BIN)/*
