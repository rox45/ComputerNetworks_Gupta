server:
	gcc -o serverBinaries/server server.c
client:
	gcc -o clientBinaries/client client.c
serverUDP:
	gcc -o serverBinaries/serverUDP serverUDP.c
clientUDP:
	gcc -o clientBinaries/clientUDP clientUDP.c
