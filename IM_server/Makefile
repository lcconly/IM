IM_server:IM_server.o
	gcc -o IM_server IM_server.o -lpthread

IM_server.o:IM_server.c IM.h
	gcc -c IM_server.c

.PHONY: run clean

run:IM_server
	./IM_server

clean:
	rm IM_server.o IM_server
