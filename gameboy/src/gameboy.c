#include "gameboy.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

	int conexion;

	if (string_equals_ignore_case(argv[1], "BROKER")) {
		conexion = connect_to(BROKER_IP, BROKER_PORT);
	} else if (string_equals_ignore_case(argv[1], "TEAM")) {
		conexion = connect_to(TEAM_IP, TEAM_PORT);
	} else if (string_equals_ignore_case(argv[1], "GAMECARD")) {
		conexion = connect_to(GAMECARD_IP, GAMECARD_PORT);
	} else {
		return EXIT_FAILURE;
	}

	char* payload_content[argc - 3];

	get_payload_content(argc, argv, payload_content);

	event_code code = string_to_event_code(argv[2]);

	t_buffer* buffer = serialize_buffer(code, argc - 3, payload_content);

	if (code == APPEARED_POKEMON) {
		send_message((uint32_t) conexion, code, 0, payload_content[3], buffer);
	} else if (code == CAUGHT_POKEMON) {
		send_message((uint32_t) conexion, code, 0, payload_content[0], buffer);
	} else {
		send_message((uint32_t) conexion, code, 0, 0, buffer);
	}

	return EXIT_SUCCESS;
}

void get_payload_content(int argc, char* argv[], char* payload_content[]) {
	for (int i = 3; i < argc; i++) {
		payload_content[i - 3] = argv[i];
	}
}
