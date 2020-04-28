#include "operavirus-commons.h"

#include<stdio.h>
#include<stdlib.h>

int main(void)
{
	return EXIT_SUCCESS;
}

int connect_to(char* ip, char* port) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, port, &hints, &server_info);

	int client_socket = socket(server_info->ai_family,
			server_info->ai_socktype, server_info->ai_protocol);

	if (connect(client_socket, server_info->ai_addr, server_info->ai_addrlen)
			== -1)
		printf("error");

	freeaddrinfo(server_info);

	return client_socket;
}

void send_message(uint32_t client_socket, event_code event_code, uint32_t id, uint32_t correlative_id, t_buffer* buffer)
{
	t_message* message = malloc(sizeof(t_message));
	message->event_code = event_code;
	message->correlative_id = correlative_id;
	message->id = id;
	message->buffer = buffer;

	uint32_t bytes_to_send = buffer->size + sizeof(uint32_t) + sizeof(event_code) + sizeof(uint32_t) + sizeof(uint32_t);

	void* to_send = serialize_message(message, &(bytes_to_send));

	send(client_socket, to_send, bytes_to_send, 0);

	free(to_send);
	free(message->buffer->payload);
	free(message->buffer);
	free(message);
}

void* serialize_message(t_message* message, uint32_t* bytes_to_send)
{
	void* to_send = malloc(*bytes_to_send);
	int offset = 0;
	memcpy(to_send + offset, &(message->event_code), sizeof(event_code));
	offset += sizeof(event_code);
	memcpy(to_send + offset, &(message->id), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(to_send + offset, &(message->correlative_id), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(to_send + offset, &(message->buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(to_send + offset, message->buffer->payload, message->buffer->size);

	return to_send;
}
}
