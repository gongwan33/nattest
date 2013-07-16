all: 	
	gcc -O3 -Wall RDTServer.c -I./ -I../Include -L../Lib/Linux/x86 -lpthread -lIOTCAPIs -lRDTAPIs -o RDTServer
	gcc -O3 -Wall RDTClient.c -I./ -I../Include -L../Lib/Linux/x86 -lpthread -lIOTCAPIs -lRDTAPIs -o RDTClient

clean:
	rm -rf *.o *~ *.bak RDTServer RDTClient
