IM_client:IM_client.o
	gcc -o IM_client IM_client.o -lpthread -lncurses

IM_client.o:IM_client.c IM.h
	gcc -c IM_client.c

.PHONY: clean

clean:
	rm IM_client.o IM_client
