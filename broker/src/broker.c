#include "broker.h"

int main(void) {
	server_init();

	return EXIT_SUCCESS;
}

void server_init(void) {
	answered_messages = list_create(); // Answered messages list
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
	queue_new_pokemon->messages = list_create();
	queue_new_pokemon->subscriptor_sockets = list_create();

	queue_appeared_pokemon->messages = list_create();
	queue_appeared_pokemon->subscriptor_sockets = list_create();

	queue_catch_pokemon->messages = list_create();
	queue_catch_pokemon->subscriptor_sockets = list_create();

	queue_caught_pokemon->messages = list_create();
	queue_caught_pokemon->subscriptor_sockets = list_create();

	queue_get_pokemon->messages = list_create();
	queue_get_pokemon->subscriptor_sockets = list_create();

	queue_localized_pokemon->messages = list_create();
	queue_localized_pokemon->subscriptor_sockets = list_create();
}

t_log* iniciar_logger(void) {
	return log_create("pruebita.log", "broker", true, LOG_LEVEL_INFO);
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
	event_code code;
	if (recv(*socket, &code, sizeof(event_code), MSG_WAITALL) == -1)
		code = -1;

	process_request(code, *socket);
}

void process_request(uint32_t event_code, uint32_t client_socket) {
	t_message* msg = receive_message(event_code, client_socket);
	msg->id = get_message_id();
	uint32_t size;

	switch (event_code) {
	case NEW_POKEMON:
			msg->buffer->payload = deserialize_new_pokemon_message(client_socket, &size);
			msg->buffer->size = size;
			//TODO: Return message_id to message sender

			uint32_t i;
			for (i = 0; i < list_size(queue_new_pokemon.subscriptor_sockets); i++)
			{
				uint32_t socket = list_get(queue_new_pokemon.subscriptor_sockets, i);
				send_message(socket, event_code, msg->id, msg->correlative_id, msg->buffer);
			}

			break;
	case APPEARED_POKEMON:
			msg->buffer->payload = deserialize_appeared_pokemon_message(client_socket, &size);
			msg->buffer->size = size;
			break;
	case CATCH_POKEMON:
			msg->buffer->payload = deserialize_catch_pokemon_message(client_socket, &size);
			msg->buffer->size = size;
			break;
	case CAUGHT_POKEMON:
			msg->buffer->payload = deserialize_caught_pokemon_message(client_socket, &size);
			msg->buffer->size = size;
			break;
	case GET_POKEMON:
			msg->buffer->payload = deserialize_get_pokemon_message(client_socket, &size);
			msg->buffer->size = size;
			break;
	case LOCALIZED_POKEMON:
			msg->buffer->payload = deserialize_localized_pokemon_message(client_socket, &size);
			msg->buffer->size = size;
			break;
	case 0:
		pthread_exit(NULL);
	case -1:
		pthread_exit(NULL);
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
