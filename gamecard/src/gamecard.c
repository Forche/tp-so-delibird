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

	sem_init(&kkkk, 0, 0);
	create_listener_thread();
	create_subscription_threads();

	shutdown_gamecard();

	sem_wait(&kkkk);

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
	IP_GAMECARD = config_get_string_value(config, "IP");
	PUERTO_GAMECARD = config_get_string_value(config, "PUERTO");

}

void init_loggers() {

	logger = log_create("/home/utnso/workspace/tp-2020-1c-Operavirus/gamecard/gamecard.log", "gamecard", true, LOG_LEVEL_INFO); //true porque escribe tambien la consola

}

void shutdown_gamecard() {
	log_destroy(logger);
}

void create_subscription_threads() {
	pthread_t thread_new = create_thread_with_param(subscribe_to, NEW_POKEMON, "subscribe NEW_POKEMON");
	sleep(2);
	pthread_t thread_catch = create_thread_with_param(subscribe_to, CATCH_POKEMON, "subscribe CATCH_POKEMON");
	sleep(2);
	pthread_t thread_get = create_thread_with_param(subscribe_to, GET_POKEMON, "subscribe GET_POKEMON");

}

void subscribe_to(event_code code) {
	t_subscription_petition* new_subscription = build_new_subscription(code, IP_GAMECARD, "Game Card", PUERTO_GAMECARD);
	make_subscription_to(new_subscription, IP_BROKER, PUERTO_BROKER, TIEMPO_DE_REINTENTO_CONEXION, logger, handle_event);
}


void create_listener_thread() {
	create_thread(gamecard_listener, "match_pokemon_with_trainer");
}

void gamecard_listener() {
	listener(IP_GAMECARD, PUERTO_GAMECARD, handle_event);
}


void handle_event(uint32_t* socket) {
	event_code code;
	if (recv(*socket, &code, sizeof(event_code), MSG_WAITALL) == -1)
		code = -1;
	t_message* msg = receive_message(code, *socket);
	uint32_t size;
	switch (code) {
	case NEW_POKEMON:
		msg->buffer = deserialize_new_pokemon_message(*socket, &size);
		//create_thread_with_param(handle_localized, msg, "handle_localized");

		break;
	case CATCH_POKEMON:
		msg->buffer = deserialize_catch_pokemon_message(*socket, &size);
		//create_thread_with_param(handle_caught, msg, "handle_caught");

		break;
	case GET_POKEMON:
		msg->buffer = deserialize_get_pokemon_message(*socket, &size);
		//create_thread_with_param(handle_appeared, msg, "handle_appeared");
		break;
	}

}
