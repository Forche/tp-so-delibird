/*
 ============================================================================
 Name        : gamecard.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "gamecard.h"


int main(void) {

	read_config();
	init_loggers();

	log_info(logger, "holis");

	shutdown_gamecard();

	return EXIT_SUCCESS;
}

void read_config() {
	config = config_create("/home/utnso/workspace/tp-2020-1c-Operavirus/gamecard/gamecard.config");

	TIEMPO_DE_REINTENTO_CONEXION = config_get_int_value(config, "TIEMPO_DE_REINTENTO_CONEXION");
	TIEMPO_DE_REINTENTO_OPERACION = config_get_int_value(config, "TIEMPO_DE_REINTENTO_OPERACION");
	TIEMPO_RETARDO_OPERACION = config_get_int_value(config, "TIEMPO_RETARDO_OPERACION");
	PUNTO_MONTAJE_TALLGRASS = config_get_string_value(config, "PUNTO_MONTAJE_TALLGRASS");
	IP_BROKER = config_get_string_value(config, "IP_BROKER");
	PUERTO_BROKER = config_get_string_value(config, "PUERTO_BROKER");

}

void init_loggers() {

	logger = log_create("/home/utnso/workspace/tp-2020-1c-Operavirus/gamecard/gamecard.log", "gamecard", true, LOG_LEVEL_INFO); //true porque escribe tambien la consola

}

void shutdown_gamecard() {
	log_destroy(logger);
}
