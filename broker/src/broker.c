/*
 ============================================================================
 Name        : broker.c
 Author      : Operavirus
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "broker.h"

int main(void) {
	server_init();

	return EXIT_SUCCESS;
}

void server_init(void) {
	int sv_socket;

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(IP, PORT, &hints, &servinfo);

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sv_socket = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1)
			continue;

		if (bind(sv_socket, p->ai_addr, p->ai_addrlen) == -1) {
			close(sv_socket);
			continue;
		}
		break;
	}

	listen(sv_socket, SOMAXCONN);

	freeaddrinfo(servinfo);

	while (1)
		wait_for_client(sv_socket);
}

void wait_for_client(uint32_t sv_socket) {
	struct sockaddr_in client_addr;

	uint32_t addr_size = sizeof(struct sockaddr_in);

	uint32_t client_socket = accept(sv_socket, (void*) &client_addr,
			&addr_size);

	pthread_create(&thread, NULL, (void*) serve_client, &client_socket);
	pthread_detach(thread);

}

void serve_client(uint32_t* socket) {
	uint32_t event_code;
	if (recv(*socket, &event_code, sizeof(uint32_t), MSG_WAITALL) == -1)
		event_code = -1;
	process_request(event_code, *socket);
}

void process_request(uint32_t event_code, uint32_t client_fd) {
	t_buffer* msg;
	switch (event_code) {
	case NEW_POKEMON:
		msg = receive_new_pokemon_message(client_fd);
		//return_message(msg, size, client_fd);
		free(msg);
		break;
	case 0:
		pthread_exit(NULL);
	case -1:
		pthread_exit(NULL);
	}
}

t_buffer* receive_new_pokemon_message(uint32_t client_socket) {
	t_buffer* buffer;
	t_message* message = malloc(sizeof(t_message));
	message->event_code = NEW_POKEMON;

	recv(client_socket, &(message->id), sizeof(uint32_t), MSG_WAITALL);
	recv(client_socket, &(message->correlative_id), sizeof(uint32_t), MSG_WAITALL);
	buffer = malloc(sizeof(t_buffer));
	recv(client_socket, &(buffer->size), sizeof(uint32_t), MSG_WAITALL);

	message->buffer = buffer;

	t_new_pokemon* new_pokemon = malloc(sizeof(t_new_pokemon));
	recv(client_socket, &(new_pokemon->pokemon_len), sizeof(uint32_t), MSG_WAITALL);

	recv(client_socket, new_pokemon->pokemon, new_pokemon->pokemon_len, MSG_WAITALL);

	recv(client_socket, &(new_pokemon->pos_x), sizeof(uint32_t), MSG_WAITALL);
	recv(client_socket, &(new_pokemon->pos_y), sizeof(uint32_t), MSG_WAITALL);
	recv(client_socket, &(new_pokemon->count), sizeof(uint32_t), MSG_WAITALL);


	return buffer;
}

void* serialize_message(t_message* message, uint32_t bytes) {
	void* memory_position = malloc(bytes);
	uint32_t offset = 0;

	memcpy(memory_position + offset, &(message->event_code),
			sizeof(event_code));
	offset += sizeof(uint32_t);
	memcpy(memory_position + offset, &(message->id), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(memory_position + offset, &(message->correlative_id),
			sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(memory_position + offset, &(message->buffer->size),
			sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(memory_position + offset, message->buffer->payload,
			message->buffer->size);
	offset += message->buffer->size;

	return memory_position;
}

void return_message(void* payload, uint32_t size, uint32_t socket_cliente) {
//	t_message* message = malloc(sizeof(t_message));
//
//	message->codigo_operacion = MENSAJE;
//	message->buffer = malloc(sizeof(t_buffer));
//	message->buffer->size = size;
//	message->buffer->stream = malloc(paquete->buffer->size);
//	memcpy(paquete->buffer->stream, payload, paquete->buffer->size);
//
//	int bytes = paquete->buffer->size + 2*sizeof(int);
//
//	void* a_enviar = serializar_paquete(paquete, bytes);
//
//	send(socket_cliente, a_enviar, bytes, 0);
//
//	free(a_enviar);
//	free(paquete->buffer->stream);
//	free(paquete->buffer);
//	free(paquete);
}
