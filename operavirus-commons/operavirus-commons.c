#include "operavirus-commons.h"

int main(void) {
	return EXIT_SUCCESS;
}

t_log* logger_init() {
	return log_create("operavirus.log", "operavirus", true,
			LOG_LEVEL_INFO);
}

uint32_t connect_to(char* ip, char* port) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, port, &hints, &server_info);

	uint32_t client_socket = socket(server_info->ai_family, server_info->ai_socktype,
			server_info->ai_protocol);

	if (connect(client_socket, server_info->ai_addr, server_info->ai_addrlen)
			== -1) {
		close(client_socket);
		client_socket = -1;
	}

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

t_subscription_petition* build_new_subscription(event_code code, char* my_ip, char* id, uint32_t my_port){
	t_subscription_petition* suscription = malloc(sizeof(t_subscription_petition));
	suscription->ip_len = string_length(my_ip) + 1;
	suscription->ip = my_ip;
	suscription->queue = code;
	suscription->subscriptor_len = string_length(id) + 1;
	suscription->subscriptor_id = id;
	suscription->port = my_port;
	suscription->duration = -1;

	return suscription;
}

void make_subscription_to(t_subscription_petition* suscription, char* broker_ip, char* broker_port,
		uint32_t reconnect_time, t_log* logger, void handle_event(uint32_t*)) {

	t_buffer* buffer = serialize_t_new_subscriptor_message(suscription);

	uint32_t broker_connection = connect_to(broker_ip, broker_port);
	if(broker_connection == -1) {
		log_error(logger, "No se pudo conectar al broker, reintentando suscripcion en %d seg", reconnect_time);
		sleep(reconnect_time);
		make_subscription_to(suscription, broker_ip, broker_port, reconnect_time, logger, handle_event);
	} else {
		log_info(logger, "Conectada cola code %d al broker", suscription->queue);
		send_message(broker_connection, NEW_SUBSCRIPTOR, NULL, NULL, buffer);

		while(1) {
			handle_event(&broker_connection);
		}
	}
}

void return_message_id(uint32_t client_socket, uint32_t id) {
	void* to_send = malloc(sizeof(uint32_t));
	memcpy(to_send, &(id), sizeof(uint32_t));
	send(client_socket, to_send, sizeof(uint32_t), 0);

	free(to_send);
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

}

t_buffer* serialize_buffer(event_code event_code, uint32_t arg_count,
	char* payload_content[], char* sender_id, char* sender_ip,
	uint32_t sender_port) {
	switch (event_code) {
	case NEW_POKEMON:
		validate_arg_count(arg_count, 4);
		return serialize_new_pokemon_message(payload_content);

	case APPEARED_POKEMON:
		validate_arg_count(arg_count, 4);
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

	case NEW_SUBSCRIPTOR:
		validate_arg_count(arg_count, 2);
		return serialize_new_subscriptor_message(payload_content, sender_id,
				sender_ip, sender_port);
	default:
		return NULL;
	}

	return NULL;
}

t_buffer* serialize_new_subscriptor_message(char* payload_content[],
		char* sender_id, char* sender_ip, uint32_t sender_port) {
	t_subscription_petition* subscription_petition_ptr = malloc(
			sizeof(t_subscription_petition));
	subscription_petition_ptr->subscriptor_len = strlen(sender_id) + 1;
	subscription_petition_ptr->subscriptor_id = sender_id;
	subscription_petition_ptr->queue = string_to_event_code(payload_content[0]);
	subscription_petition_ptr->ip_len = strlen(sender_ip) + 1;
	subscription_petition_ptr->ip = sender_ip;
	subscription_petition_ptr->port = sender_port;
	subscription_petition_ptr->duration = atoi(payload_content[1]);

	return serialize_t_new_subscriptor_message(subscription_petition_ptr);
}

t_buffer* serialize_t_new_subscriptor_message(t_subscription_petition* subscription_petition_ptr) {
	t_subscription_petition subscription_petition_msg = (*subscription_petition_ptr);
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = subscription_petition_ptr->subscriptor_len
			+ subscription_petition_ptr->ip_len + 4 * sizeof(uint32_t)
			+ sizeof(event_code);

	void* payload = malloc(buffer->size);
	int offset = 0;

	memcpy(payload + offset, &subscription_petition_msg.subscriptor_len,
			sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, subscription_petition_msg.subscriptor_id,
			subscription_petition_msg.subscriptor_len);
	offset += subscription_petition_msg.subscriptor_len;
	memcpy(payload + offset, &subscription_petition_msg.queue, sizeof(event_code));
	offset += sizeof(event_code);
	memcpy(payload + offset, &subscription_petition_msg.ip_len,
				sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, subscription_petition_msg.ip,
			subscription_petition_msg.ip_len);
	offset += subscription_petition_msg.ip_len;
	memcpy(payload + offset, &subscription_petition_msg.port, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, &subscription_petition_msg.duration, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	buffer->payload = payload;

	free(subscription_petition_ptr);

	return buffer;
}

t_buffer* serialize_message_received_message(char* payload_content[],
	char* sender_id, char* sender_ip, uint32_t received_message_id) {
	t_message_received* message_received_ptr = malloc(
			sizeof(t_message_received));
	message_received_ptr->subscriptor_len = strlen(sender_id) + 1;
	message_received_ptr->subscriptor_id = sender_id;
	message_received_ptr->message_type = string_to_event_code(payload_content[0]);
	message_received_ptr->received_message_id = received_message_id;

	return serialize_t_message_received_message(message_received_ptr);
}

t_buffer* serialize_t_message_received_message(t_message_received* message_received_ptr) {
	t_message_received message_received_msg = (*message_received_ptr);
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = message_received_ptr->subscriptor_len + 2 * sizeof(uint32_t)
			+ sizeof(event_code);

	void* payload = malloc(buffer->size);
	int offset = 0;

	memcpy(payload + offset, &message_received_msg.subscriptor_len,
			sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, message_received_msg.subscriptor_id,
			message_received_msg.subscriptor_len);
	offset += message_received_msg.subscriptor_len;
	memcpy(payload + offset, &message_received_msg.message_type, sizeof(event_code));
	offset += sizeof(event_code);
	memcpy(payload + offset, &message_received_msg.received_message_id,
				sizeof(uint32_t));

	buffer->payload = payload;

	free(message_received_ptr);

	return buffer;
}

t_buffer* serialize_new_pokemon_message(char* payload_content[]) {
	t_new_pokemon* new_pokemon_ptr = malloc(sizeof(t_new_pokemon));
	new_pokemon_ptr->pokemon_len = strlen(payload_content[0]) + 1;
	new_pokemon_ptr->pokemon = payload_content[0];
	new_pokemon_ptr->pos_x = atoi(payload_content[1]);
	new_pokemon_ptr->pos_y = atoi(payload_content[2]);
	new_pokemon_ptr->count = atoi(payload_content[3]);

	return serialize_t_new_pokemon_message(new_pokemon_ptr);
}

t_buffer* serialize_t_new_pokemon_message(t_new_pokemon* new_pokemon_ptr){
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
	t_appeared_pokemon* appeared_pokemon_ptr = malloc(
			sizeof(t_appeared_pokemon));
	appeared_pokemon_ptr->pokemon_len = strlen(payload_content[0]) + 1;
	appeared_pokemon_ptr->pokemon = payload_content[0];
	appeared_pokemon_ptr->pos_x = atoi(payload_content[1]);
	appeared_pokemon_ptr->pos_y = atoi(payload_content[2]);

	return serialize_t_appeared_pokemon_message(appeared_pokemon_ptr);
}

t_buffer* serialize_t_appeared_pokemon_message(t_appeared_pokemon* appeared_pokemon_ptr) {

	t_appeared_pokemon appeared_pokemon_msg = (*appeared_pokemon_ptr);

	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = sizeof(uint32_t) * 3 + appeared_pokemon_ptr->pokemon_len;

	void* payload = malloc(buffer->size);
	int offset = 0;

	memcpy(payload + offset, &appeared_pokemon_msg.pokemon_len,
			sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, appeared_pokemon_msg.pokemon,
			appeared_pokemon_msg.pokemon_len);
	offset += appeared_pokemon_msg.pokemon_len;
	memcpy(payload + offset, &appeared_pokemon_msg.pos_x, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, &appeared_pokemon_msg.pos_y, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	buffer->payload = payload;

	free(appeared_pokemon_ptr);

	return buffer;
}

t_buffer* serialize_catch_pokemon_message(char* payload_content[]) {
	t_catch_pokemon* catch_pokemon_ptr = malloc(sizeof(t_catch_pokemon));
	catch_pokemon_ptr->pokemon_len = strlen(payload_content[0]) + 1;
	catch_pokemon_ptr->pokemon = payload_content[0];
	catch_pokemon_ptr->pos_x = atoi(payload_content[1]);
	catch_pokemon_ptr->pos_y = atoi(payload_content[2]);

	return serialize_t_catch_pokemon_message(catch_pokemon_ptr);
}

t_buffer* serialize_t_catch_pokemon_message(t_catch_pokemon* catch_pokemon) {

	t_catch_pokemon catch_pokemon_msg = (*catch_pokemon);

	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = sizeof(uint32_t) * 3 + catch_pokemon->pokemon_len;

	void* payload = malloc(buffer->size);
	int offset = 0;

	memcpy(payload + offset, &catch_pokemon_msg.pokemon_len, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, catch_pokemon_msg.pokemon,
			catch_pokemon_msg.pokemon_len);
	offset += catch_pokemon_msg.pokemon_len;
	memcpy(payload + offset, &catch_pokemon_msg.pos_x, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, &catch_pokemon_msg.pos_y, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	buffer->payload = payload;

	free(catch_pokemon);

	return buffer;
}

t_buffer* serialize_caught_pokemon_message(char* payload_content[]) {
	t_caught_pokemon* caught_pokemon_ptr = malloc(sizeof(t_caught_pokemon));
	uint32_t status;
	if (string_equals_ignore_case(payload_content[1], "OK")) {
		status = 1;
	} else {
		status = 0;
	}

	caught_pokemon_ptr->result = status;

	return serialize_t_caught_pokemon_message(caught_pokemon_ptr);
}

t_buffer* serialize_t_caught_pokemon_message(t_caught_pokemon* caught_pokemon_ptr) {
	t_caught_pokemon caught_pokemon_msg = (*caught_pokemon_ptr);

	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = sizeof(uint32_t);

	void* payload = malloc(buffer->size);
	int offset = 0;

	memcpy(payload + offset, &caught_pokemon_msg.result, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	buffer->payload = payload;


	return buffer;
}

t_buffer* serialize_get_pokemon_message(char* payload_content[]) {
	t_get_pokemon* get_pokemon_ptr = malloc(sizeof(t_get_pokemon));
	get_pokemon_ptr->pokemon_len = strlen(payload_content[0]) + 1;
	get_pokemon_ptr->pokemon = payload_content[0];
	return serialize_t_get_pokemon_message(get_pokemon_ptr);
}

t_buffer* serialize_t_get_pokemon_message(t_get_pokemon* get_pokemon_ptr) {
	t_get_pokemon get_pokemon_msg = (*get_pokemon_ptr);
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = sizeof(uint32_t) + get_pokemon_ptr->pokemon_len;

	void* payload = malloc(buffer->size);
	int offset = 0;

	memcpy(payload + offset, &get_pokemon_msg.pokemon_len, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, get_pokemon_msg.pokemon,
			get_pokemon_msg.pokemon_len);
	offset += get_pokemon_msg.pokemon_len;

	buffer->payload = payload;

	free(get_pokemon_ptr);

	return buffer;
}

t_buffer* serialize_localized_pokemon_message(char* payload_content[]) {

	//No se llama desde el gameboy
	t_localized_pokemon* localized_pokemon_ptr = malloc(sizeof(t_appeared_pokemon));
	localized_pokemon_ptr->pokemon_len = strlen(payload_content[0]) + 1;
	localized_pokemon_ptr->pokemon = payload_content[0];
	localized_pokemon_ptr->positions_count = atoi(payload_content[1]);
	localized_pokemon_ptr->positions = atoi(payload_content[2]);

	return serialize_t_localized_pokemon_message(localized_pokemon_ptr);
}

t_buffer* serialize_t_localized_pokemon_message(t_localized_pokemon* localized_pokemon_ptr) {
	t_localized_pokemon localized_pokemon_msg = (*localized_pokemon_ptr);

	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = sizeof(uint32_t) * 2 + localized_pokemon_ptr->pokemon_len + 2 * localized_pokemon_ptr->positions_count;

	void* payload = malloc(buffer->size);
	int offset = 0;

	memcpy(payload + offset, &localized_pokemon_msg.pokemon_len, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, &localized_pokemon_msg.positions_count, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, localized_pokemon_msg.pokemon, localized_pokemon_msg.pokemon_len);
	offset += localized_pokemon_msg.pokemon_len;
	memcpy(payload + offset, &localized_pokemon_msg.positions, 2 * localized_pokemon_msg.positions_count);

	buffer->payload = payload;

	free(localized_pokemon_ptr);

	return buffer;
}

t_buffer* serialize_t_message_received(t_message_received* message_received) {
	t_message_received message_received_msg = (*message_received);

	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = 3 * sizeof(uint32_t) + message_received->subscriptor_len;

	void* payload = malloc(buffer->size);
	int offset = 0;

	memcpy(payload + offset, &message_received_msg.subscriptor_len, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, message_received_msg.subscriptor_id, message_received_msg.subscriptor_len);
	offset += message_received_msg.subscriptor_len;
	memcpy(payload + offset, &message_received_msg.message_type, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(payload + offset, &message_received_msg.received_message_id, sizeof(uint32_t));

	buffer->payload = payload;

	free(message_received);
	return buffer;
}

t_message* receive_message(uint32_t event_code, uint32_t socket) {
	uint32_t id;
	uint32_t correlative_id;
	uint32_t size;

	recv(socket, &id, sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &correlative_id, sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &size, sizeof(uint32_t), MSG_WAITALL);

	t_message* message = malloc(sizeof(t_message));
	message->id = id;
	message->correlative_id = correlative_id;
	message->event_code = event_code;
	message->buffer = malloc(sizeof(uint32_t) + size);
	message->buffer->size = size;

	return message;
}

t_new_pokemon* deserialize_new_pokemon_message(uint32_t socket, uint32_t* size) {
	uint32_t pokemon_len;
	recv(socket, &(pokemon_len), sizeof(uint32_t),
	MSG_WAITALL);

	*size = 4 * sizeof(uint32_t) + pokemon_len;
	t_new_pokemon* new_pokemon = malloc(*size);
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

t_appeared_pokemon* deserialize_appeared_pokemon_message(uint32_t socket,
		uint32_t* size) {
	uint32_t pokemon_len;
	recv(socket, &(pokemon_len), sizeof(uint32_t),
	MSG_WAITALL);

	*size = 3 * sizeof(uint32_t) + pokemon_len;
	t_appeared_pokemon* appeared_pokemon = malloc(*size);
	appeared_pokemon->pokemon_len = pokemon_len;

	appeared_pokemon->pokemon = malloc(appeared_pokemon->pokemon_len);
	recv(socket, appeared_pokemon->pokemon, appeared_pokemon->pokemon_len,
	MSG_WAITALL);

	recv(socket, &(appeared_pokemon->pos_x), sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &(appeared_pokemon->pos_y), sizeof(uint32_t), MSG_WAITALL);

	t_log* logger = logger_init();
	log_info(logger, appeared_pokemon->pokemon);
	log_destroy(logger);

	return appeared_pokemon;
}

t_catch_pokemon* deserialize_catch_pokemon_message(uint32_t socket,
		uint32_t* size) {
	uint32_t pokemon_len;
	recv(socket, &(pokemon_len), sizeof(uint32_t),
	MSG_WAITALL);

	*size = 3 * sizeof(uint32_t) + pokemon_len;
	t_catch_pokemon* catch_pokemon = malloc(*size);
	catch_pokemon->pokemon_len = pokemon_len;

	catch_pokemon->pokemon = malloc(catch_pokemon->pokemon_len);
	recv(socket, catch_pokemon->pokemon, catch_pokemon->pokemon_len,
	MSG_WAITALL);

	recv(socket, &(catch_pokemon->pos_x), sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &(catch_pokemon->pos_y), sizeof(uint32_t), MSG_WAITALL);

	t_log* logger = logger_init();
	log_info(logger, catch_pokemon->pokemon);
	log_destroy(logger);

	return catch_pokemon;
}

t_caught_pokemon* deserialize_caught_pokemon_message(uint32_t socket,
		uint32_t* size) {
	t_caught_pokemon* caught_pokemon = malloc(sizeof(uint32_t));
	recv(socket, &(caught_pokemon->result), sizeof(uint32_t), MSG_WAITALL);

	t_log* logger = logger_init();
	*size = 1;
	char* str_result = string_new();
	str_result= string_from_format("%d", caught_pokemon->result);
	log_info(logger, str_result);
	log_destroy(logger);
	free(str_result);

	return caught_pokemon;
}

t_get_pokemon* deserialize_get_pokemon_message(uint32_t socket, uint32_t* size) {
	uint32_t pokemon_len;
	recv(socket, &(pokemon_len), sizeof(uint32_t),
	MSG_WAITALL);

	*size = sizeof(uint32_t) + pokemon_len;
	t_get_pokemon* get_pokemon = malloc(*size);
	get_pokemon->pokemon_len = pokemon_len;

	get_pokemon->pokemon = malloc(get_pokemon->pokemon_len);
	recv(socket, get_pokemon->pokemon, get_pokemon->pokemon_len,
	MSG_WAITALL);

	t_log* logger = logger_init();
	log_info(logger, get_pokemon->pokemon);
	log_destroy(logger);

	return get_pokemon;
}

t_localized_pokemon* deserialize_localized_pokemon_message(uint32_t socket, uint32_t* size) {
	uint32_t pokemon_len;
	uint32_t count;
	recv(socket, &(pokemon_len), sizeof(uint32_t), MSG_WAITALL);
	recv(socket, &(count), sizeof(uint32_t), MSG_WAITALL);

	*size = 2 * sizeof(uint32_t) + pokemon_len + 2*count;
	t_localized_pokemon* localized_pokemon = malloc(*size);
	localized_pokemon->pokemon_len = pokemon_len;
	localized_pokemon->positions_count = count;
	localized_pokemon->pokemon = malloc(localized_pokemon->pokemon_len);
	recv(socket, localized_pokemon->pokemon, localized_pokemon->pokemon_len, MSG_WAITALL);
	recv(socket, localized_pokemon->positions, 2*count, MSG_WAITALL);

	t_log* logger = logger_init();
	log_info(logger, localized_pokemon->pokemon);
	log_destroy(logger);

	return localized_pokemon;
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
	} else if (string_equals_ignore_case(code, "SUSCRIPTOR")) {
		return NEW_SUBSCRIPTOR;
	} else {
		return -1;
	}
}

char* event_code_to_string(event_code code) {
	switch (code) {
	case LOCALIZED_POKEMON:
		return "LOCALIZED_POKEMON";
	case CAUGHT_POKEMON:
		return "CAUGHT_POKEMON";
	case APPEARED_POKEMON:
		return "APPEARED_POKEMON";
	case NEW_POKEMON:
		return "NEW_POKEMON";
	case CATCH_POKEMON:
		return "CATCH_POKEMON";
	case GET_POKEMON:
		return "GET_POKEMON";
	}
}
