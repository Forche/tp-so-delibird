#include "broker.h"

int main(void) {
	server_init();

	return EXIT_SUCCESS;
}

void server_init(void) {
	answered_messages = list_create(); // Answered messages list
	threads = list_create(); // Subscriptor threads list
	message_count = 0;
	queues_init();
	int sv_socket;
	t_config* config = config_create("broker.config");
	char* IP = config_get_string_value(config, "IP_BROKER");
	char* PORT = config_get_string_value(config, "PUERTO_BROKER");

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(IP, PORT, &hints, &servinfo);

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sv_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1)
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

uint32_t get_message_id() {
	message_count++;
	return message_count;
}

void queues_init() {
	queue_new_pokemon.messages = list_create();
	queue_new_pokemon.subscriptors = list_create();

	queue_appeared_pokemon.messages = list_create();
	queue_appeared_pokemon.subscriptors = list_create();

	queue_catch_pokemon.messages = list_create();
	queue_catch_pokemon.subscriptors = list_create();

	queue_caught_pokemon.messages = list_create();
	queue_caught_pokemon.subscriptors = list_create();

	queue_get_pokemon.messages = list_create();
	queue_get_pokemon.subscriptors = list_create();

	queue_localized_pokemon.messages = list_create();
	queue_localized_pokemon.subscriptors = list_create();
}

t_log* iniciar_logger(void) {
	return log_create("operavirus-broker.log", "operavirus-broker", true,
			LOG_LEVEL_INFO);
}

void wait_for_client(uint32_t sv_socket) {
	struct sockaddr_in client_addr;

	uint32_t addr_size = sizeof(struct sockaddr_in);

	uint32_t client_socket;

	if ((client_socket = accept(sv_socket, (void*) &client_addr, &addr_size))
			!= -1) {
		pthread_create(&thread, NULL, (void*) serve_client, &client_socket);
		list_add(threads, &thread);
		pthread_detach(thread);
	}
}

void serve_client(uint32_t* socket) {
	event_code code;
	if (recv(*socket, &code, sizeof(event_code), MSG_WAITALL) == -1)
		code = -1;

	process_request(code, *socket);
}

void process_request(uint32_t event_code, uint32_t client_socket) {
	t_message* msg = receive_message(event_code, client_socket);
	// Implement mutex, critical region:
	msg->id = get_message_id();
	// End of critical region
	uint32_t size;
	uint32_t i;
	switch (event_code) {
	case NEW_POKEMON:
		msg->buffer->payload = deserialize_new_pokemon_message(client_socket,
				&size);
		msg->buffer->size = size;

		return_message_id(client_socket, msg->id);


		for (i = 0; i < list_size(queue_new_pokemon.subscriptors); i++) {
			t_subscriptor* subscriptor = list_get(
					queue_new_pokemon.subscriptors, i);
			send_message(subscriptor->socket, event_code, msg->id,
					msg->correlative_id, msg->buffer);
		}
		break;
	case APPEARED_POKEMON:
		msg->buffer->payload = deserialize_appeared_pokemon_message(
				client_socket, &size);
		msg->buffer->size = size;

		return_message_id(client_socket, msg->id);

		for (i = 0; i < list_size(queue_appeared_pokemon.subscriptors); i++) {
			t_subscriptor* subscriptor = list_get(
					queue_appeared_pokemon.subscriptors, i);
			send_message(subscriptor->socket, event_code, msg->id,
					msg->correlative_id, msg->buffer);
		}
		break;
	case CATCH_POKEMON:
		msg->buffer->payload = deserialize_catch_pokemon_message(client_socket,
				&size);
		msg->buffer->size = size;
		t_log* logger = logger_init();
		log_info(logger, "Devolviendo id %d", msg->id);
		return_message_id(client_socket, msg->id);

		for (i = 0; i < list_size(queue_catch_pokemon.subscriptors); i++) {
			t_subscriptor* subscriptor = list_get(
					queue_catch_pokemon.subscriptors, i);
			send_message(subscriptor->socket, event_code, msg->id,
					msg->correlative_id, msg->buffer);
		}
		break;
	case CAUGHT_POKEMON:
		msg->buffer->payload = deserialize_caught_pokemon_message(client_socket,
				&size);
		msg->buffer->size = size;

		return_message_id(client_socket, msg->id);

		for (i = 0; i < list_size(queue_caught_pokemon.subscriptors); i++) {
			t_subscriptor* subscriptor = list_get(
					queue_caught_pokemon.subscriptors, i);
			send_message(subscriptor->socket, event_code, msg->id,
					msg->correlative_id, msg->buffer);
		}
		break;
	case GET_POKEMON:
		msg->buffer->payload = deserialize_get_pokemon_message(client_socket,
				&size);
		msg->buffer->size = size;

		for (i = 0; i < list_size(queue_get_pokemon.subscriptors); i++) {
			t_subscriptor* subscriptor = list_get(
					queue_get_pokemon.subscriptors, i);
			send_message(subscriptor->socket, event_code, msg->id,
					msg->correlative_id, msg->buffer);
		}
		break;
	case LOCALIZED_POKEMON:
		msg->buffer->payload = deserialize_localized_pokemon_message(
				client_socket, &size);
		msg->buffer->size = size;

		for (i = 0; i < list_size(queue_localized_pokemon.subscriptors); i++) {
			t_subscriptor* subscriptor = list_get(
					queue_localized_pokemon.subscriptors, i);
			send_message(subscriptor->socket, event_code, msg->id,
					msg->correlative_id, msg->buffer);
		}
		break;
	case NEW_SUBSCRIPTOR:
		process_new_subscription(client_socket);
		break;
	default:
		pthread_exit(NULL);
	}
}

void process_new_subscription(uint32_t socket) {
	uint32_t subscriptor_len;
	recv(socket, &(subscriptor_len), sizeof(uint32_t), MSG_WAITALL);

	char* subscriptor_id = malloc(subscriptor_len);
	recv(socket, subscriptor_id, subscriptor_len,
	MSG_WAITALL);

	event_code queue_type;
	recv(socket, &(queue_type), sizeof(event_code),
	MSG_WAITALL);

	uint32_t ip_len;
	recv(socket, &(ip_len), sizeof(uint32_t), MSG_WAITALL);

	t_subscription_petition* subcription_petition = malloc(
			subscriptor_len + ip_len + 4 * sizeof(uint32_t)
					+ sizeof(event_code));
	subcription_petition->subscriptor_len = subscriptor_len;
	subcription_petition->subscriptor_id = subscriptor_id;
	subcription_petition->ip_len = ip_len;

	subcription_petition->queue = queue_type;

	subcription_petition->ip = malloc(subcription_petition->ip_len);
	recv(socket, subcription_petition->ip, subcription_petition->ip_len,
	MSG_WAITALL);

	recv(socket, &(subcription_petition->port), sizeof(uint32_t), MSG_WAITALL);

	recv(socket, &(subcription_petition->duration), sizeof(uint32_t),
	MSG_WAITALL);

	t_subscriptor* subscriptor = malloc(
			sizeof(t_subscription_petition) + sizeof(uint32_t));
	subscriptor->subscriptor_info = subcription_petition;
	subscriptor->socket = socket;

	switch (subcription_petition->queue) {
	case NEW_POKEMON:
		list_add(queue_new_pokemon.subscriptors, subscriptor);
		break;
	case APPEARED_POKEMON:
		list_add(queue_appeared_pokemon.subscriptors, subscriptor);
		break;
	case CATCH_POKEMON:
		list_add(queue_catch_pokemon.subscriptors, subscriptor);
		break;
	case CAUGHT_POKEMON:
		list_add(queue_caught_pokemon.subscriptors, subscriptor);
		break;
	case GET_POKEMON:
		list_add(queue_get_pokemon.subscriptors, subscriptor);
		break;
	case LOCALIZED_POKEMON:
		list_add(queue_localized_pokemon.subscriptors, subscriptor);
		break;
	default:
		pthread_exit(NULL);
	}

	t_log* logger = logger_init();
	log_info(logger, "Nuevo suscriptor");
	log_info(logger, subcription_petition->subscriptor_id);
	log_destroy(logger);

	process_subscriptor(&(subscriptor->socket));
}

void process_subscriptor(uint32_t* socket) {
	//TODO: Add logic to send all messages in the queue of the new subscription
	//TODO: Add logic to subscribe for 'duration' seconds.
	event_code code;

	while (1) {
		if (recv(*socket, &code, sizeof(event_code), MSG_WAITALL) == -1)
			code = -1;

		process_request(code, *socket);
	}
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
