#include "gameboy.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

	int conexion;

	if (string_equals_ignore_case(argv[0], "BROKER"))
	{
		conexion = connect_to(BROKER_IP, BROKER_PORT);
	}
	else if (string_equals_ignore_case(argv[0], "TEAM"))
	{
		conexion = connect_to(TEAM_IP, TEAM_PORT);
	}
	else if (string_equals_ignore_case(argv[0], "GAMECARD"))
	{
		conexion = connect_to(GAMECARD_IP, GAMECARD_PORT);
	}
	else
	{
		return EXIT_FAILURE;
	}

	char* payload_content[argc - 2];

	get_payload_content(argc, argv, payload_content);

	t_buffer* buffer = serialize_buffer(atoi(argv[1]), argc - 2, payload_content);
	send_message((uint32_t) conexion, (event_code) atoi(argv[1]), 0, 0, buffer);

	return EXIT_SUCCESS;
}

void get_payload_content(int argc, char* argv[], char* payload_content[]) {
	for (int i = 2; i < argc; i++) {
		payload_content[i - 2] = argv[i];
	}
}
