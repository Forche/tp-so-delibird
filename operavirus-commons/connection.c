#include "connection.h"


void listener(char* ip, char* port, void* handle_event) {
	int sv_socket;
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	getaddrinfo(ip, port, &hints, &servinfo);
	int activate = 1;
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sv_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1)
			continue;
		
		setsockopt(sv_socket,SOL_SOCKET,SO_REUSEADDR,&activate,sizeof(activate));

		if (bind(sv_socket, p->ai_addr, p->ai_addrlen) == -1) {
			close(sv_socket);
			continue;
		}
		break;
	}
	listen(sv_socket, SOMAXCONN);
	freeaddrinfo(servinfo);
	while (1) {
		wait_for_message(sv_socket, handle_event);
	}
}

void wait_for_message(uint32_t sv_socket, void* handle_event) {
	struct sockaddr_in client_addr;
	pthread_t thread;
	uint32_t addr_size = sizeof(struct sockaddr_in);


	uint32_t* client_socket = malloc(sizeof(int));
	if ((*client_socket = accept(sv_socket, (void*) &client_addr, &addr_size))
			!= -1) {
		//log_info(logger, "Creo el client socket: %d adress socket: %d", *client_socket, client_socket);
		pthread_create(&thread, NULL, (void*) handle_event, client_socket);
		//log_info(logger, "Termina el thread client socket: %d adress socket: %d", *client_socket, client_socket);
		pthread_detach(thread);
	}

}

