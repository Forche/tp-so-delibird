#include "gameboy.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

	int connection;
	t_config* config = config_create("gameboy.config");

	char* BROKER_IP = config_get_string_value(config, "IP_BROKER");
	char* BROKER_PORT = config_get_string_value(config, "PUERTO_BROKER");
	char* TEAM_IP = config_get_string_value(config, "IP_TEAM");
	char* TEAM_PORT = config_get_string_value(config, "PUERTO_TEAM");
	char* GAMECARD_IP = config_get_string_value(config, "IP_GAMECARD");
	char* GAMECARD_PORT = config_get_string_value(config, "PUERTO_GAMECARD");
	char* ID = config_get_string_value(config, "ID");
	char* IP = config_get_string_value(config, "IP");
	char* PORT = config_get_string_value(config, "PORT");

	if (string_equals_ignore_case(argv[1], "BROKER")) {
		connection = connect_to(BROKER_IP, BROKER_PORT);
	} else if (string_equals_ignore_case(argv[1], "TEAM")) {
		connection = connect_to(TEAM_IP, TEAM_PORT);
	} else if (string_equals_ignore_case(argv[1], "GAMECARD")) {
		connection = connect_to(GAMECARD_IP, GAMECARD_PORT);
	} else {
		return EXIT_FAILURE;
	}

	char* payload_content[argc - 3];

	get_payload_content(argc, argv, payload_content);

	event_code code = string_to_event_code(argv[2]);

	t_buffer* buffer = serialize_buffer(code, argc - 3, payload_content, ID, IP, atoi(PORT));

	if (code == APPEARED_POKEMON && string_equals_ignore_case(argv[1], "BROKER")) {
		send_message((uint32_t) connection, code, 0, atoi(payload_content[3]), buffer);
	} else if (code == CAUGHT_POKEMON) {
		send_message((uint32_t) connection, code, 0, atoi(payload_content[0]), buffer);
	} else if (code == NEW_POKEMON && string_equals_ignore_case(argv[1], "GAMECARD")) {
		send_message((uint32_t) connection, code, atoi(payload_content[4]), 0, buffer);
	} else if (code == CATCH_POKEMON && string_equals_ignore_case(argv[1], "GAMECARD")) {
		send_message((uint32_t) connection, code, atoi(payload_content[3]), 0, buffer);
	} else if (code == GET_POKEMON && string_equals_ignore_case(argv[1], "GAMECARD")) {
		send_message((uint32_t) connection, code, atoi(payload_content[1]), 0, buffer);
	} else if (code == NEW_SUBSCRIPTOR && string_equals_ignore_case(argv[1], "BROKER")) {

		uint32_t gb_socket;
		char* IP = config_get_string_value(config, "IP");
		char* PORT = config_get_string_value(config, "PORT");

		struct addrinfo hints, *info, *p;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		getaddrinfo(IP, PORT, &hints, &info);

		for (p = info; p != NULL; p = p->ai_next) {
			if ((gb_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
					== -1)
				continue;

			if (bind(gb_socket, p->ai_addr, p->ai_addrlen) == -1) {
				close(gb_socket);
				continue;
			}
			break;
		}

		listen(gb_socket, SOMAXCONN);

		freeaddrinfo(info);

		send_message((uint32_t) connection, code, 0, 0, buffer);

		while (1)
			wait_for_messages(gb_socket);

	}
	else
	{
		send_message((uint32_t) connection, code, 0, 0, buffer);
	}

	return EXIT_SUCCESS;
}

void process_message(uint32_t* socket) {
	event_code code;
	if (recv(*socket, &code, sizeof(event_code), MSG_WAITALL) == -1)
		code = -1;

	//TODO: Log received message
}


void wait_for_messages(uint32_t socket) {
	struct sockaddr_in client_addr;

	uint32_t addr_size = sizeof(struct sockaddr_in);

	uint32_t client_socket = accept(socket, (void*) &client_addr,
			&addr_size);

	pthread_create(&thread, NULL, (void*) process_message, &client_socket);
	pthread_detach(thread);
}



void get_payload_content(int argc, char* argv[], char* payload_content[]) {
	for (int i = 3; i < argc; i++) {
		payload_content[i - 3] = argv[i];
	}
}
