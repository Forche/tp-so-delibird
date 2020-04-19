/*
 ============================================================================
 Name        : broker.c
 Author      : Operavirus
 Version     :
 Copyright   : 
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "broker.h"

int main(void) {
	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	t_message* message = malloc(sizeof(t_message));
	free(message);
	return EXIT_SUCCESS;
}
