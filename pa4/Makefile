BIN = client server
GEN = who_clnt.c who_svc.c who_xdr.c who.h
RPCCOM = rpcgen
CC = gcc
CFLAGS = -g -I/usr/include/tirpc
LDFLAGS = -ltirpc

all: $(GEN) $(BIN)

client: client.o who_clnt.o who_xdr.o
	$(CC) -o $@ client.o who_clnt.o who_xdr.o $(LDFLAGS)

server: server.o who_svc.o who_xdr.o
	$(CC) -o $@ server.o who_svc.o who_xdr.o $(LDFLAGS)

%.o: %.c who.h
	$(CC) $(CFLAGS) -c $< -o $@

$(GEN): who.x
	$(RPCCOM) who.x

clean:
	rm -f $(GEN) *.o $(BIN)

