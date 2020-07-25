#include "gameboy.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

	char* string = string_new();
	string_append(&string, "/home/utnso/tp-2020-1c-Operavirus/gameboy/");
	if(argc == 2) {
		string_append(&string, argv[1]);
	} else {
		string_append(&string, "gameboy.config");
	}

	uint32_t connection;
	t_config* config = config_create(string);
	logger = log_create("log_gameboy.log", "gameboy", true, LOG_LEVEL_INFO);

	char* BROKER_IP = config_get_string_value(config, "IP_BROKER");
	char* BROKER_PORT = config_get_string_value(config, "PORT_BROKER");
	char* TEAM_IP = config_get_string_value(config, "IP_TEAM");
	char* TEAM_PORT = config_get_string_value(config, "PORT_TEAM");
	char* GAMECARD_IP = config_get_string_value(config, "IP_GAMECARD");
	char* GAMECARD_PORT = config_get_string_value(config, "PORT_GAMECARD");
	char* ID = config_get_string_value(config, "ID");
	char* IP = config_get_string_value(config, "IP");
	char* PORT = config_get_string_value(config, "PORT");

	if (string_equals_ignore_case(argv[1], "BROKER")
			|| string_equals_ignore_case(argv[1], "SUSCRIPTOR")) {
		connection = connect_to(BROKER_IP, BROKER_PORT);
	} else if (string_equals_ignore_case(argv[1], "TEAM")) {
		connection = connect_to(TEAM_IP, TEAM_PORT);
	} else if (string_equals_ignore_case(argv[1], "GAMECARD")) {
		connection = connect_to(GAMECARD_IP, GAMECARD_PORT);
	} else {
		return EXIT_FAILURE;
	}

	if(connection) {
		log_info(logger, "Conectado a proceso %s", argv[1]);
	}

	char* payload_content[argc - 3];

	event_code code = string_to_event_code(argv[2]);

	uint32_t is_new_subscriptor = 0;

	if (string_equals_ignore_case(argv[1], "SUSCRIPTOR")) {
		code = NEW_SUBSCRIPTOR;
		is_new_subscriptor = 1;
	}

	get_payload_content(argc, argv, payload_content, is_new_subscriptor);

	t_buffer* buffer = serialize_buffer(code, argc - 3, payload_content, ID, IP,
			atoi(PORT));

	if (code == APPEARED_POKEMON
			&& string_equals_ignore_case(argv[1], "BROKER")) {
		send_message(connection, code, 0, atoi(payload_content[3]), buffer);
	} else if (code == CAUGHT_POKEMON) {
		send_message(connection, code, 0, atoi(payload_content[0]), buffer);
	} else if (code == NEW_POKEMON
			&& string_equals_ignore_case(argv[1], "GAMECARD")) {
		send_message(connection, code, atoi(payload_content[4]), 0, buffer);
	} else if (code == CATCH_POKEMON
			&& string_equals_ignore_case(argv[1], "GAMECARD")) {
		send_message(connection, code, atoi(payload_content[3]), 0, buffer);
	} else if (code == GET_POKEMON
			&& string_equals_ignore_case(argv[1], "GAMECARD")) {
		send_message(connection, code, atoi(payload_content[1]), 0, buffer);
	} else if (code == NEW_SUBSCRIPTOR) {

		listen(connection, SOMAXCONN);

		send_message(connection, code, 0, 0, buffer);
		log_info(logger, "Suscripto a cola de mensajes %s", argv[2]);
		still_connected = 1;
		pthread_t thread = create_thread_with_param(received_messages, connection, "received_messages");

		sleep(atoi(argv[3]));
		log_info(logger, "Finalizo el tiempo de conexion");
		close(connection);
		config_destroy(config);
		exit(0);
	} else {
		send_message(connection, code, 0, 0, buffer);
	}

	config_destroy(config);
	return EXIT_SUCCESS;
}

void received_messages(uint32_t* socket) {
	while (still_connected) {
		still_connected = process_message(&socket);
	}
}

int process_message(uint32_t* socket) {
	event_code code;
	int bytes = recv(*socket, &code, sizeof(event_code), MSG_WAITALL);
	if (bytes == -1 || bytes == 0) {
		return 0;
	} else {
		t_message* msg = receive_message(code, *socket);
		uint32_t size;
		t_message_received* message_received = malloc(sizeof(t_message_received));
		switch (code) {
		case LOCALIZED_POKEMON:
			msg->buffer->payload = deserialize_localized_pokemon_message(*socket, &size);
			log_info(logger, "Recibido LOCALIZED_POKEMON.");
			break;
		case CAUGHT_POKEMON:
			msg->buffer->payload = deserialize_caught_pokemon_message(*socket, &size);
			log_info(logger, "Recibido CAUGHT_POKEMON.");
			break;
		case APPEARED_POKEMON:
			msg->buffer->payload = deserialize_appeared_pokemon_message(*socket, &size);
			log_info(logger, "Recibido APPEARED_POKEMON.");
			break;
		case NEW_POKEMON:
			msg->buffer->payload = deserialize_new_pokemon_message(*socket, &size);
			log_info(logger, "Recibido NEW_POKEMON.");
			break;
		case CATCH_POKEMON:
			msg->buffer->payload = deserialize_catch_pokemon_message(*socket, &size);
			log_info(logger, "Recibido CATCH_POKEMON.");
			break;
		case GET_POKEMON:
			msg->buffer->payload = deserialize_get_pokemon_message(*socket, &size);
			log_info(logger, "Recibido GET_POKEMON.");
			break;
		}
		return 1;
	}

}

void wait_for_messages(uint32_t socket) {
	struct sockaddr_in client_addr;

	uint32_t addr_size = sizeof(struct sockaddr_in);

	uint32_t client_socket = accept(socket, (void*) &client_addr, &addr_size);

	pthread_create(&thread, NULL, (void*) process_message, &client_socket);
	pthread_detach(thread);
}

void get_payload_content(int argc, char* argv[], char* payload_content[],
		uint32_t is_new_subscriptor) {
	if (is_new_subscriptor) {
		for (int i = 2; i < argc; i++) {
			payload_content[i - 2] = argv[i];
		}
	} else {
		for (int i = 3; i < argc; i++) {
			payload_content[i - 3] = argv[i];
		}
	}
}

