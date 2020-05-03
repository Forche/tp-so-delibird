#include "operavirus-commons.h"

#include<stdio.h>
#include<stdlib.h>

int main(void) {
	return EXIT_SUCCESS;
}

t_log* logger_init(void) {
	return log_create("operavirus-commons.log", "operavirus-commons", true, LOG_LEVEL_INFO);
}

int connect_to(char* ip, char* port) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, port, &hints, &server_info);

	int client_socket = socket(server_info->ai_family, server_info->ai_socktype,
			server_info->ai_protocol);

	if (connect(client_socket, server_info->ai_addr, server_info->ai_addrlen)
			== -1)
		printf("error");

	freeaddrinfo(server_info);

	return client_socket;
}

void send_message(uint32_t client_socket, event_code event_code, uint32_t id,
		uint32_t correlative_id, t_buffer* buffer) {
	t_message* message = malloc(sizeof(t_message));
	message->event_code = event_code;
	message->correlative_id = correlative_id;
	message->id = id;
	message->buffer = buffer;

	uint32_t bytes_to_send = buffer->size + sizeof(uint32_t)
			+ sizeof(event_code) + sizeof(uint32_t) + sizeof(uint32_t);

	void* to_send = serialize_message(message, &(bytes_to_send));

	send(client_socket, to_send, bytes_to_send, 0);

	free(to_send);
	free(message->buffer->payload);
	free(message->buffer);
	free(message);
}

void* serialize_message(t_message* message, uint32_t* bytes_to_send) {
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

void validate_arg_count(uint32_t arg_count, uint32_t payload_args) {
	//TODO: Throw exception if arg_count != payload_args
}

t_buffer* serialize_buffer(event_code event_code, uint32_t arg_count,
		char* payload_content[]) {
	switch (event_code) {
	case NEW_POKEMON:
		validate_arg_count(arg_count, 4);
		return serialize_new_pokemon_message(payload_content);

	case APPEARED_POKEMON:
		validate_arg_count(arg_count, 3);
		return serialize_appeared_pokemon_message(payload_content);

	case CATCH_POKEMON:
		validate_arg_count(arg_count, 3);
		return serialize_catch_pokemon_message(payload_content);

	case CAUGHT_POKEMON:
		validate_arg_count(arg_count, 1);
		return serialize_caught_pokemon_message(payload_content);

	case GET_POKEMON:
		validate_arg_count(arg_count, 1);
		return serialize_get_pokemon_message(payload_content);

	case LOCALIZED_POKEMON:
		validate_arg_count(arg_count, 2 + atoi(payload_content[1]) * 2);
		return serialize_localized_pokemon_message(payload_content);

	default:
		return NULL;
	}

	return NULL;
}

t_buffer* serialize_new_pokemon_message(char* payload_content[]) {
	t_new_pokemon* new_pokemon_ptr = malloc(sizeof(t_new_pokemon));
	new_pokemon_ptr->pokemon_len = strlen(payload_content[0]) + 1;
	new_pokemon_ptr->pokemon = payload_content[0];
	new_pokemon_ptr->pos_x = atoi(payload_content[1]);
	new_pokemon_ptr->pos_y = atoi(payload_content[2]);
	new_pokemon_ptr->count = atoi(payload_content[3]);
	t_new_pokemon new_pokemon_msg = (*new_pokemon_ptr);

	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = sizeof(uint32_t) * 4 + new_pokemon_ptr->pokemon_len;

	void* payload = malloc(buffer->size);
	int offset = 0;

	memcpy(payload + offset, &new_pokemon_msg.pokemon_len, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, new_pokemon_msg.pokemon,
			new_pokemon_msg.pokemon_len);
	offset += new_pokemon_msg.pokemon_len;
	memcpy(payload + offset, &new_pokemon_msg.pos_x, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, &new_pokemon_msg.pos_y, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, &new_pokemon_msg.count, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	buffer->payload = payload;

	free(new_pokemon_ptr);

	return buffer;
}

t_buffer* serialize_appeared_pokemon_message(char* payload_content[]) {
	return NULL;
}

t_buffer* serialize_catch_pokemon_message(char* payload_content[]) {
	return NULL;
}

t_buffer* serialize_caught_pokemon_message(char* payload_content[]) {
	return NULL;
}

t_buffer* serialize_get_pokemon_message(char* payload_content[]) {
	return NULL;
}

t_buffer* serialize_localized_pokemon_message(char* payload_content[]) {
	return NULL;
}

t_message* receive_message(uint32_t event_code, uint32_t socket) {
	t_buffer* buffer;
	t_message* message = malloc(sizeof(t_message));
	message->event_code = event_code;

	recv(socket, &(message->id), sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &(message->correlative_id), sizeof(uint32_t),
			MSG_WAITALL);
	uint32_t size;
	recv(socket, &size, sizeof(uint32_t), MSG_WAITALL);
	buffer = malloc(sizeof(uint32_t) + size);
	buffer->size = size;
	message->buffer = buffer;

	return message;
}

t_new_pokemon* deserialize_new_pokemon_message(uint32_t socket) {
	uint32_t pokemon_len;
	recv(socket, &(pokemon_len), sizeof(uint32_t),
			MSG_WAITALL);

	t_new_pokemon* new_pokemon = malloc(4 * sizeof(uint32_t) + pokemon_len);
	new_pokemon->pokemon_len = pokemon_len;

	new_pokemon->pokemon = malloc(new_pokemon->pokemon_len);
	recv(socket, new_pokemon->pokemon, new_pokemon->pokemon_len,
			MSG_WAITALL);

	recv(socket, &(new_pokemon->pos_x), sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &(new_pokemon->pos_y), sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &(new_pokemon->count), sizeof(uint32_t), MSG_WAITALL);

	t_log* logger = logger_init();
	log_info(logger, new_pokemon->pokemon);
	log_destroy(logger);

	return new_pokemon;
}

t_appeared_pokemon* deserialize_appeared_pokemon_message(uint32_t socket) {
	return NULL;
}

t_catch_pokemon* deserialize_catch_pokemon_message(uint32_t socket) {
	return NULL;
}

t_caught_pokemon* deserialize_caught_pokemon_message(uint32_t socket) {
	return NULL;
}

t_get_pokemon* deserialize_get_pokemon_message(uint32_t socket) {
	return NULL;
}

t_localized_pokemon* deserialize_localized_pokemon_message(uint32_t socket) {
	return NULL;
}

event_code string_to_event_code(char* code) {
	if (string_equals_ignore_case(code, "NEW_POKEMON")) {
		return NEW_POKEMON;
	} else if (string_equals_ignore_case(code, "APPEARED_POKEMON")) {
		return APPEARED_POKEMON;
	} else if (string_equals_ignore_case(code, "CATCH_POKEMON")) {
		return CATCH_POKEMON;
	} else if (string_equals_ignore_case(code, "CAUGHT_POKEMON")) {
		return CAUGHT_POKEMON;
	} else if (string_equals_ignore_case(code, "GET_POKEMON")) {
		return GET_POKEMON;
	} else if (string_equals_ignore_case(code, "LOCALIZED_POKEMON")) {
		return LOCALIZED_POKEMON;
	} else {
		return -1;
	}
}
