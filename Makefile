all:histclient histserver histclient_th histserver_th

histclient: histclient.c
	gcc -Wall -o histclient histclient.c -lrt

histserver: histserver.c
	gcc -Wall -o histserver histserver.c -lrt

histclient_th: histclient_th.c
	gcc -Wall -o histclient_th histclient_th.c -lrt

histserver_th: histserver_th.c
	gcc -Wall -o histserver_th histserver_th.c -lpthread -lrt

clean:
	rm -fr *~ histclient histserver histclient_th histserver_th


