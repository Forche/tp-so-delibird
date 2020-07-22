#include "gameboy.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

	uint32_t connection;
	t_config* config = config_create("gameboy.config");

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

		send_message((uint32_t) connection, code, 0, 0, buffer);

		while (1) {
			process_message(&connection);
		}
	} else {
		send_message(connection, code, 0, 0, buffer);
	}

	config_destroy(config);
	return EXIT_SUCCESS;
}

void process_message(uint32_t* socket) {
	event_code code;
	if (recv(*socket, &code, sizeof(event_code), MSG_WAITALL) == -1)
	{
		code = -1;
	}
	else
	{
		printf("Mensaje recibido. \n");
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
