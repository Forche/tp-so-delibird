#include "operavirus-commons.h"

#include<stdio.h>
#include<stdlib.h>
#include<commons/string.h>

int main(void)
{
	char* message = string_new();
	string_append(&message, "Odio a Dua Lipa");
	puts(message);
	return EXIT_SUCCESS;
}
