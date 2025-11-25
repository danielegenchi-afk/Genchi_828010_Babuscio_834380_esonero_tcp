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
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

#include "protocol.h"

#define NO_ERROR 0

void clearwinsock() {
#if defined WIN32
	WSACleanup();
#endif
}



int main(int argc, char *argv[]) {

	// TODO: Implement server logic
	srand(time(NULL));
	int port = SERVER_PORT;
	for (int i = 1; i < argc; i++){
		if (strcmp(argv[i], "-p") == 0) {
			if (i + 1 < argc) {
			port = atoi(argv[i+1]);
			i++;
			}

			if (port < 0) {
				printf("Numero di porta errato %s \n", argv[1]);
				system("pause");
				return 0;
			}
		}
	}



#if defined WIN32
	// Initialize Winsock
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (result != NO_ERROR) {
		printf("Error at WSAStartup()\n");
		return 0;
	}
#endif



	// CREAZIONE DELLA SOCKET
	int my_socket;
	my_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (my_socket < 0) {
	errorhandler("socket creation failed.\n");
	clearwinsock();
	printf("\n Premere un tasto per terminare...");
	system("pause");
	return -1;
	}

	// ASSEGNAZIONE DI UN INDIRIZZO ALLA SOCKET
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);





	// bind
	if (bind(my_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
	errorhandler("bind() failed.\n");
	closesocket(my_socket);
	clearwinsock();
	printf("\n Premere un tasto per terminare...");
	system("pause");
	return -1;
	}

	// listen;
	if (listen (my_socket, QUEUE_SIZE) < 0) {
	errorhandler("listen() failed.\n");
	closesocket(my_socket);
	clearwinsock();
	printf("\n Premere un tasto per terminare...");
	system("pause");
	return -1;
	}

	// TODO: Implement connection acceptance loop
	// while (1) {
	//     int client_socket = accept(...);
	//     // Handle client communication
	//     closesocket(client_socket);
	// }

	struct sockaddr_in client_addr; // struttura per l'indirizzo client
	int client_socket; // descrittore di socket per il client
	int client_len; // dimensione dell' indirizzo client
	printf("Waiting for a client to connect...");
	while (1) {
	client_len = sizeof(client_addr); // setta la dimensione dell'indirizzo client
	if ((client_socket = accept(my_socket, (struct sockaddr
	*)&client_addr, &client_len)) < 0) {
	errorhandler("accept() failed.\n");
	}


	weather_request_t req;
	weather_response_t res;

	res.status = 3;
	res.type = 'N';
	res.value = 0.0;

	const char *citta[] ={
							"BARI",
							"ROMA",
							"MILANO",
							"NAPOLI",
							"TORINO",
							"PALERMO",
							"GENOVA",
							"BOLOGNA",
							"FIRENZE",
							"VENEZIA"
						};

	//Ricezione dati dal client
	memset(&req, 0, sizeof(req));
	int bytes_rcvd = recv(client_socket, (const char*)&req, sizeof(req), 0);

	if (bytes_rcvd <= 0) {
	    errorhandler("Richiesta fallita\n");
	    closesocket(client_socket);
	    continue;
	}

	printf("Richiesta %c %s dal client ip %s\n", req.type, req.city, inet_ntoa(client_addr.sin_addr));


	//Creazione del messaggio di risposta
	//Verifico se la citta esiste
	int trovata=0;
	//Rendo la stringa maiuscola
	strmMaiusc(req.city);

	for(int i=0; i<10; i++){
		if(strcmp(req.city,citta[i]) == 0) {
			trovata = 1;
			break;
		}
	}

	if(trovata == 1){
		res.status = 0;
	}else{
		res.status = 1;
		conversione(&res);
		send(client_socket, (const char*)&res, sizeof(res), 0);
		closesocket(client_socket);
			continue;
	}


	//Verifica sul tipo
		switch(req.type){
	case 't': res.value = get_temperature();
	break;
	case 'h': res.value = get_humidity();
	break;
	case 'w': res.value = get_wind();
	break;
	case 'p': res.value = get_pressure();
	break;

	default: res.status=2, res.type='N', res.value=0.0;

	conversione(&res);
	send(client_socket, (const char*)&res, sizeof(res), 0);
			closesocket(client_socket);
			continue;
	break;
	}

		conversione(&res);
		//Creo la risposta per il client
		res.type = req.type;
		send(client_socket, (const char*)&res, sizeof(res), 0);
		closesocket(client_socket);
		continue;


	} //end while

	printf("Server terminated.\n");

	closesocket(my_socket);
	clearwinsock();
	system("pause");
	return 0;
} // main end


float get_temperature(void) {
    float min = -10.0;
    float max = 40.0;
    float n = min + (rand() / (float)RAND_MAX) * (max - min);
    return n;
}


float get_humidity(void){
	float min = 20.0;
	    float max = 100.0;
	    float n = min + (rand() / (float)RAND_MAX) * (max - min);
	    return n;
}

float get_wind(void){
		float min = 0.0;
	    float max = 100.0;
	    float n = min + (rand() / (float)RAND_MAX) * (max - min);
	    return n;
}

float get_pressure(void){
		float min = 950.0;
	    float max = 1050.0;
	    float n = min + (rand() / (float)RAND_MAX) * (max - min);
	    return n;
}

void strmMaiusc(char *s) {
    while (*s) {
        *s = toupper(*s);
        s++;
    }
}

void conversione(weather_response_t *res){
	uint32_t status_net = htonl(res->status);
	    memcpy(&(res->status), &status_net, sizeof(uint32_t));
	    uint32_t raw_float;

	    memcpy(&raw_float, &(res->value), sizeof(float));
	    raw_float = htonl(raw_float);
	    memcpy(&(res->value), &raw_float, sizeof(float));
}

void errorhandler(char *errorMessage) {
printf ("%s", errorMessage);
}
