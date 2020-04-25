#include "gameboy.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {

	int conexion = connect_to(IP, PORT);

	char* mensajito = "Hola";
	send(conexion, mensajito, 5, 0);

	return EXIT_SUCCESS;
}
