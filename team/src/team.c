#include "team.h"


int main(void) {

	config = config_create("team.config");
	pokemon_received_to_catch = list_create();
	trainers = list_create();
	matches = list_create();
	logger = log_create("pruebitaTeam.log", "team", true, LOG_LEVEL_INFO);
	set_logger_thread(logger);


	char* POSICIONES_ENTRENADORES = config_get_string_value(config, "POSICIONES_ENTRENADORES");
	char* POKEMON_ENTRENADORES = config_get_string_value(config, "POKEMON_ENTRENADORES");
	char* OBJETIVOS_ENTRENADORES = config_get_string_value(config, "OBJETIVOS_ENTRENADORES");
	IP_BROKER = config_get_string_value(config, "IP_BROKER");
	PUERTO_BROKER = config_get_string_value(config, "PUERTO_BROKER");
	IP_TEAM = config_get_string_value(config, "IP");
	PUERTO_TEAM = config_get_string_value(config, "PUERTO");
	POSICIONES_ENTRENADORES = remove_square_braquets(POSICIONES_ENTRENADORES);
	POKEMON_ENTRENADORES = remove_square_braquets(POKEMON_ENTRENADORES);
	OBJETIVOS_ENTRENADORES = remove_square_braquets(OBJETIVOS_ENTRENADORES);
	TIEMPO_RECONEXION = config_get_int_value(config, "TIEMPO_RECONEXION");


	global_objective = build_global_objective(OBJETIVOS_ENTRENADORES);

	create_trainers(POSICIONES_ENTRENADORES, POKEMON_ENTRENADORES,OBJETIVOS_ENTRENADORES);
	create_planner();
	create_matcher();
	create_thread_open_socket();

	init_sem();

	send_get_pokemons();


	sem_t sem_objetive_completed;
	sem_init(&sem_objetive_completed, 0, 0);

	pthread_t thread_appeared = create_thread_with_param(subscribe_to, APPEARED_POKEMON, "subscribe APPEARED_POKEMON");
	sleep(1);
	pthread_t thread_caught = create_thread_with_param(subscribe_to, CAUGHT_POKEMON, "subscribe CAUGHT_POKEMON");
	sleep(1);
	pthread_t thread_localized = create_thread_with_param(subscribe_to, LOCALIZED_POKEMON, "subscribe LOCALIZED_POKEMON");

	sem_wait(&sem_objetive_completed);

	return EXIT_SUCCESS;
}

void handle_event(uint32_t* socket) {
	event_code code;
	if (recv(*socket, &code, sizeof(event_code), MSG_WAITALL) == -1)
		code = -1;
	t_message* msg = receive_message(code, *socket);
	uint32_t size;
	switch (code) {
	case LOCALIZED_POKEMON:
		msg->buffer = deserialize_localized_pokemon_message(*socket, &size);
		create_thread_with_param(handle_localized, msg, "handle_localized");

		break;
	case CAUGHT_POKEMON:
		msg->buffer = deserialize_caught_pokemon_message(*socket, &size);
		create_thread_with_param(handle_caught, msg, "handle_caught");

		break;
	case APPEARED_POKEMON:
		msg->buffer = deserialize_appeared_pokemon_message(*socket, &size);
		create_thread_with_param(handle_appeared, msg, "handle_appeared");
		break;
	}

}

void handle_localized(t_message* msg) {
	t_localized_pokemon* localized_pokemon = msg->buffer;
	if(dictionary_get(global_objective, localized_pokemon->pokemon)) {
		uint32_t pos_x;
		uint32_t pos_y = 1;
		t_list* localized_appeared_pokemon = list_create();
		for(pos_x = 0; pos_x < (localized_pokemon->positions_count * 2); pos_x = pos_x + 2 ) {
			t_appeared_pokemon* appeared_pokemon = malloc(sizeof(t_appeared_pokemon));
			appeared_pokemon->pokemon_len = localized_pokemon->pokemon_len;
			appeared_pokemon->pokemon = localized_pokemon->pokemon;
			appeared_pokemon->pos_x = localized_pokemon->positions[pos_x];
			appeared_pokemon->pos_y = localized_pokemon->positions[pos_y];
			list_add(localized_appeared_pokemon, appeared_pokemon);
			pos_y = pos_y + 2;
		}

		//TODO MANEJAR COLA DE VICTIMAS EN MATCHER
		pthread_mutex_lock(&mutex_pokemon_received_to_catch);
		list_add_all(pokemon_received_to_catch, localized_appeared_pokemon);
		pthread_mutex_unlock(&mutex_pokemon_received_to_catch);
		sem_post(&sem_appeared_pokemon);
	}
}

void handle_caught(t_message* msg) {
	t_caught_pokemon* caught_pokemon = msg->buffer;
	t_trainer* trainer = obtener_trainer_mensaje(msg);
	if (trainer != NULL) {
		trainer->pcb_trainer->result_catch = caught_pokemon->result;
		pthread_mutex_unlock(&trainer->pcb_trainer->sem_caught);
	}
}

void handle_appeared(t_message* msg) {
	t_appeared_pokemon* appeared_pokemon = msg->buffer;
	if (dictionary_get(global_objective, appeared_pokemon->pokemon)) {
		pthread_mutex_lock(&mutex_pokemon_received_to_catch);
		list_add(pokemon_received_to_catch, appeared_pokemon);
		pthread_mutex_unlock(&mutex_pokemon_received_to_catch);
		sem_post(&sem_appeared_pokemon);
	}
}

void init_sem() {
	sem_init(&sem_trainer_available, 0, list_size(trainers));
	sem_init(&sem_appeared_pokemon, 0, 0);
	sem_init(&sem_count_matches, 0, 0);
	pthread_mutex_init(&mutex_pokemon_received_to_catch, NULL);
	pthread_mutex_init(&mutex_trainers, NULL);
	pthread_mutex_init(&mutex_matches, NULL);
	pthread_mutex_init(&mutex_planning, NULL);
	pthread_mutex_init(&mutex_receiving_id, NULL);

}

void send_get_pokemons() {
	dictionary_iterator(global_objective, get_pokemon);
}

void get_pokemon(char* pokemon, uint32_t* cant) {
	create_thread_with_param(send_get, pokemon, "send_get");
	//FIXME Sacar sleep, hay sindrome de los tres chiflados
	sleep(1);
}

void send_get(char* pokemon) {
	t_get_pokemon* get_pokemon = malloc(strlen(pokemon) + sizeof(uint32_t));
	get_pokemon->pokemon_len = strlen(pokemon);
	get_pokemon->pokemon = pokemon;
	log_info(logger, pokemon);
	t_buffer* buffer = serialize_t_get_pokemon_message(get_pokemon);
	uint32_t broker_connection = connect_to(IP_BROKER, PUERTO_BROKER);
	if(broker_connection == -1) {
		log_error(logger, "Error de comunicacion con el broker, realizando operacion default");
	} else {
		send_message(broker_connection, GET_POKEMON, NULL, NULL, buffer);
		close(broker_connection);
	}
}

t_dictionary* build_global_objective(char* objectives) {
	char* flat_objectives = replace_word(replace_word(objectives, "|", ","), "\"", "");
	return generate_dictionary_by_string(flat_objectives, ",");
}

void create_planner() {
	create_thread(planning_catch, "planning_catch");
}

void create_matcher() {
	create_thread(match_pokemon_with_trainer, "match_pokemon_with_trainer");
}

void create_thread_open_socket() {
	create_thread(team_listener, "match_pokemon_with_trainer");
}

void team_listener() {
	listener(IP_TEAM, PUERTO_TEAM, handle_event);
}

void create_trainers(char* POSICIONES_ENTRENADORES, char* POKEMON_ENTRENADORES, char* OBJETIVOS_ENTRENADORES) {
	char** positions_by_trianer = string_split(POSICIONES_ENTRENADORES, ",");
	char** pokemons_by_trainer = string_split(POKEMON_ENTRENADORES, ",");
	char** objectives_by_trainer = string_split(OBJETIVOS_ENTRENADORES, ",");

	uint32_t i = 0;
	while(positions_by_trianer[i] != '\0') {
		pthread_t thread;

		char** positions = string_split(positions_by_trianer[i], "|");
		t_dictionary* pokemons = generate_dictionary_by_string(pokemons_by_trainer[i], "|");
		t_dictionary* objectives = generate_dictionary_by_string(objectives_by_trainer[i], "|");

		t_trainer* trainer = malloc(sizeof(t_trainer));
		trainer->pos_x = atoi(positions[0]);
		trainer->pos_y = atoi(positions[1]);
		trainer->objective = objectives;
		trainer->caught = pokemons;
		trainer->status = NEW;
		trainer->pcb_trainer = malloc(sizeof(t_pcb_trainer));


		pthread_create(&thread, NULL, handle_trainer, trainer);
		list_add(trainers, trainer);
		i++;
	}

}

void subscribe_to(event_code code) {
	t_subscription_petition* suscription_appeared = malloc(sizeof(t_subscription_petition));
	suscription_appeared->ip_len = string_length(IP_TEAM) + 1;
	suscription_appeared->ip = IP_TEAM;
	suscription_appeared->queue = code;
	suscription_appeared->subscriptor_len = 12;
	suscription_appeared->subscriptor_id = "TEAM ROCKET";
	suscription_appeared->port = PUERTO_TEAM;
	suscription_appeared->duration = -1;

	t_buffer* buffer = serialize_t_new_subscriptor_message(suscription_appeared);

	uint32_t broker_connection = connect_to(IP_BROKER, PUERTO_BROKER);
	if(broker_connection == -1) {
		log_error(logger, "No se pudo conectar al broker, reintentando en %d seg", TIEMPO_RECONEXION);
		sleep(TIEMPO_RECONEXION);
		subscribe_to(code);
	} else {
		send_message(broker_connection, NEW_SUBSCRIPTOR, NULL, NULL, buffer);

		while(1) {
			handle_event(&broker_connection);
		}
	}

}



