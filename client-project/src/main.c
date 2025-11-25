/*
 * main.c
 *
 * TCP Client - Template for Computer Networks assignment
 *
 * This file contains the boilerplate code for a TCP client
 * portable across Windows, Linux and macOS.
 */

#if defined WIN32
#include <winsock.h>
#else
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "protocol.h"
#define NO_ERROR 0

void clearwinsock() {
#if defined WIN32
    WSACleanup();
#endif
}

void errorhandler(char *msg) {
    printf("%s\n", msg);
}

int main(int argc, char *argv[]) {
	//controllo parametri di input
	char server_ip[16]="127.0.0.1";
	    int server_port = SERVER_PORT;
	    char *request_str = NULL;
	for (int i = 1; i < argc; i++) {
	        if (strcmp(argv[i], "-s") == 0) {
	            if (i + 1 < argc) {
	                strncpy(server_ip, argv[i+1], sizeof(server_ip) - 1);
	                server_ip[sizeof(server_ip) - 1] = '\0';
	                i++;
	            }
	        } else if (strcmp(argv[i], "-p") == 0) {
	            if (i + 1 < argc) {
	                server_port = atoi(argv[i+1]);
	                i++;
	            }
	        } else if (strcmp(argv[i], "-r") == 0) {
	            if (i + 1 < argc) {
	                request_str = argv[i+1];
	                i++;
	            }
	        }
	    }

	    if (request_str == NULL) {
	        printf("Errore: Il parametro '-r request' è obbligatorio.\n");
	        printf("Utilizzo: ./client-project [-s server] [-p port] -r \"tipo città\"\n");
	        return 1;
	    }
	    if (strlen(request_str) < 2) {
	        printf("Errore: La richiesta deve contenere almeno tipo e città.\n");
	        return 1;
	    }

	    //creazione richiesta
    weather_request_t req;
    memset(&req, 0, sizeof(req));

    req.type = request_str[0];

        int start_index = 1;
        while (request_str[start_index] == ' ') {
            start_index++;
        }
        snprintf(req.city, sizeof(req.city), "%s", request_str + start_index);

	#if defined WIN32
		WSADATA wsa_data;
		int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
		if (result != NO_ERROR) {
			printf("Error at WSAStartup()\n");
			return 0;
		}
	#endif

		//creazione socket
    int my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (my_socket < 0) {
        errorhandler("creazione socket fallita");
        clearwinsock();
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);

    //connessione
    if (connect(my_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        errorhandler("connect() fallito");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    //invio dati al server
    if (send(my_socket, &req, sizeof(req), 0) != sizeof(req)){
        errorhandler("send() fallito");
        closesocket(my_socket);
        clearwinsock();
        return -1;
    }

    //ricezione dati dal server
    weather_response_t resp;
    memset(&resp, 0, sizeof(resp));

    size_t total_received = 0;
    size_t bytes_to_receive = sizeof(weather_response_t);

    char *buffer = (char *)&resp;

    while (total_received < bytes_to_receive) {
        ssize_t bytes_rcvd = recv(my_socket, buffer + total_received, bytes_to_receive - total_received, 0);

        if (bytes_rcvd == 0) {
            printf("Connessione chiusa dal server prima di ricevere la struct completa.\n");
            closesocket(my_socket);
            clearwinsock();
            return -1;
        }
        if (bytes_rcvd < 0) {
            errorhandler("recv() fallita");
            closesocket(my_socket);
            clearwinsock();
            return -1;
        }

        total_received += bytes_rcvd;
    }
    resp.status = ntohl(resp.status);

    //conversione del tipo
    uint32_t raw_bytes;

    memcpy(&raw_bytes, &resp.value, sizeof(resp.value));

    raw_bytes = ntohl(raw_bytes);

    memcpy(&resp.value, &raw_bytes, sizeof(resp.value));

    //creazione messaggio di output
    switch(resp.status){
    case 0: if (req.type == 't') {
    	printf("Ricevuto risultato dal server ip %s. %s: Temperatura = %.1f°C\n", server_ip, req.city, resp.value);
    }
    else if (req.type == 'h') {
    	printf("Ricevuto risultato dal server ip %s. %s: Umidità = %.1f%%\n", server_ip, req.city, resp.value);
    }
    else if (req.type == 'w') {
    	printf("Ricevuto risultato dal server ip %s. %s: Vento = %.1fkm/h\n", server_ip, req.city, resp.value);
    }
    else if (req.type == 'p') {
    	printf("Ricevuto risultato dal server ip %s. %s: Pressione = %.1fhPa\n", server_ip, req.city, resp.value);
    }
    else {

    }
    break;
    case 1: errorhandler("Città non disponibile");
    break;
    case 2: errorhandler("Richiesta non valida");
    break;
    }

    //chiusura socket
    closesocket(my_socket);
    clearwinsock();

    return 0;
}
