#include "team.h"


void print_pokemons(char* key, int* value) {
	log_info(logger, "Pokemon: %s. Cantidad: %d", key, *value);
}

int main(void) {

	config = config_create("team.config");
	char* LOG_FILE = config_get_string_value(config, "LOG_FILE");

	pokemon_received_to_catch = list_create();
	trainers = list_create();
	matches = list_create();
	logger = log_create(LOG_FILE, "team", true, LOG_LEVEL_INFO);
	set_logger_thread(logger);
	being_caught_pokemons = dictionary_create();
	remaining_pokemons = dictionary_create();
	matched_deadlocks = list_create();
	to_deadlock = list_create();
	queue_deadlock = list_create();

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
	RETARDO_CICLO_CPU = config_get_int_value(config, "RETARDO_CICLO_CPU");
	ESTIMACION_INICIAL = config_get_double_value(config, "ESTIMACION_INICIAL");
	ALPHA = config_get_double_value(config, "ALPHA");
	ALGORITMO_PLANIFICACION = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	QUANTUM = config_get_int_value(config, "QUANTUM");


	global_objective = build_global_objective(OBJETIVOS_ENTRENADORES);
	caught_pokemons = build_global_objective(POKEMON_ENTRENADORES);
	build_remaining_pokemons();

	create_trainers(POSICIONES_ENTRENADORES, POKEMON_ENTRENADORES,OBJETIVOS_ENTRENADORES);
	create_planner();
	create_matcher();
	create_thread_open_socket();

	init_sem();

	send_get_pokemons();


	sem_init(&sem_all_pokemons_caught, 0, 0);

	pthread_t thread_appeared = create_thread_with_param(subscribe_to, APPEARED_POKEMON, "subscribe APPEARED_POKEMON");
	sleep(1);
	pthread_t thread_caught = create_thread_with_param(subscribe_to, CAUGHT_POKEMON, "subscribe CAUGHT_POKEMON");
	sleep(1);
	pthread_t thread_localized = create_thread_with_param(subscribe_to, LOCALIZED_POKEMON, "subscribe LOCALIZED_POKEMON");

	sem_wait(&sem_all_pokemons_caught);

	//No se necesitan mas los threads de suscripcion
	create_thread(planning_deadlock, "planning_deadlock");
	proceed_to_finish();

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
	log_info(logger, "Recibido LOCALIZED_POKEMON. Pokemon: %s. Cantidad: %d.",
			localized_pokemon->pokemon, localized_pokemon->positions_count);

	pthread_mutex_lock(&mutex_remaining_pokemons);
	uint32_t cant_catch = dictionary_get(remaining_pokemons, localized_pokemon->pokemon);
	pthread_mutex_unlock(&mutex_remaining_pokemons);

	if(cant_catch) {
		uint32_t pos_x;
		uint32_t pos_y = 1;
		t_list* localized_appeared_pokemon = list_create();
		for(pos_x = 0; pos_x < (localized_pokemon->positions_count * 2); pos_x = pos_x + 2) {
			t_appeared_pokemon* appeared_pokemon = malloc(sizeof(t_appeared_pokemon));
			appeared_pokemon->pokemon_len = localized_pokemon->pokemon_len;
			appeared_pokemon->pokemon = localized_pokemon->pokemon;
			appeared_pokemon->pos_x = localized_pokemon->positions[pos_x];
			appeared_pokemon->pos_y = localized_pokemon->positions[pos_y];
			list_add(localized_appeared_pokemon, appeared_pokemon);
			pos_y = pos_y + 2;
		}

		pthread_mutex_lock(&mutex_pokemon_received_to_catch);
		list_add_all(pokemon_received_to_catch, localized_appeared_pokemon);
		pthread_mutex_unlock(&mutex_pokemon_received_to_catch);
		uint32_t aux;
		for(aux = 0; aux < localized_pokemon->positions_count; aux++) {
			sem_post(&sem_appeared_pokemon);
		}
	}
}

void handle_caught(t_message* msg) {
	t_caught_pokemon* caught_pokemon = msg->buffer;
	log_info(logger, "Recibido CAUGHT_POKEMON. Resultado: %d. Correlative_id: %d", caught_pokemon->result, msg->correlative_id);
	t_trainer* trainer = obtener_trainer_mensaje(msg);
	if (trainer != NULL) {
		trainer->pcb_trainer->result_catch = caught_pokemon->result;
		pthread_mutex_unlock(&trainer->pcb_trainer->sem_caught);
	}
}

void handle_appeared(t_message* msg) {
	t_appeared_pokemon* appeared_pokemon = msg->buffer;
	log_info(logger, "Recibido APPEARED_POKEMON. Pokemon: %s. Posicion x: %d, y: %d",
			appeared_pokemon->pokemon, appeared_pokemon->pos_x, appeared_pokemon->pos_y);

	pthread_mutex_lock(&mutex_remaining_pokemons);
	uint32_t q_catch = dictionary_get(remaining_pokemons, appeared_pokemon->pokemon);
	pthread_mutex_unlock(&mutex_remaining_pokemons);

	if (q_catch) {
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
	sem_init(&sem_count_queue_deadlocks, 0, 0);
	pthread_mutex_init(&mutex_pokemon_received_to_catch, NULL);
	pthread_mutex_init(&mutex_trainers, NULL);
	pthread_mutex_init(&mutex_matches, NULL);
	pthread_mutex_init(&mutex_planning, NULL);
	pthread_mutex_init(&mutex_remaining_pokemons, NULL);
	pthread_mutex_init(&mutex_caught_pokemons, NULL);
	pthread_mutex_init(&mutex_being_caught_pokemons, NULL);
	pthread_mutex_init(&mutex_queue_deadlocks, NULL);
	pthread_mutex_init(&mutex_planning_deadlock, NULL);
}

void send_get_pokemons() {
	dictionary_iterator(remaining_pokemons, get_pokemon);
}

void get_pokemon(char* pokemon, uint32_t* cant) {
	create_thread_with_param(send_get, pokemon, "send_get");
	//FIXME Sacar sleep, hay sindrome de los tres chiflados
	sleep(1);
}

void send_get(char* pokemon) {
	uint32_t broker_connection = connect_to(IP_BROKER, PUERTO_BROKER);
	if(broker_connection == -1) {
		log_error(logger, "Error de comunicacion con el broker, realizando operacion get default");
	} else {
		t_get_pokemon* get_pokemon = malloc(strlen(pokemon) + sizeof(uint32_t));
		get_pokemon->pokemon_len = strlen(pokemon);
		get_pokemon->pokemon = pokemon;
		t_buffer* buffer = serialize_t_get_pokemon_message(get_pokemon);
		send_message(broker_connection, GET_POKEMON, NULL, NULL, buffer);
		close(broker_connection);
	}
}

t_dictionary* build_global_objective(char* objectives) {
	char* flat_objectives = replace_word(replace_word(objectives, "|", ","), "\"", "");
	return generate_dictionary_by_string(flat_objectives, ",");
}

void* build_remaining_pokemons() {
	dictionary_iterator(global_objective, add_remaining);
}

void* add_remaining(char* pokemon, uint32_t* cant_global) {
	if(dictionary_has_key(caught_pokemons, pokemon)) {
		uint32_t* cant_caught = dictionary_get(caught_pokemons, pokemon);
		uint32_t* cant_remaining = malloc(sizeof(uint32_t));
		*cant_remaining = (*cant_global) - (*cant_caught);
		if(*cant_remaining < 0) {
			exit(-777);
		} else if(*cant_remaining > 0) {
			dictionary_put(remaining_pokemons, pokemon, cant_remaining);
		} else {
			//Si es 0 ni es error, ni se agrega a los faltantes.
		}
	} else {
		dictionary_put(remaining_pokemons, pokemon, cant_global);
	}
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

	t_list* list_pokemons_by_trainer = generate_list(pokemons_by_trainer);
	t_list* list_objectives_by_trainer = generate_list(objectives_by_trainer);

	uint32_t i = 0;
	while(positions_by_trianer[i] != '\0') {
		pthread_t thread;

		char** positions = string_split(positions_by_trianer[i], "|");
		t_dictionary* pokemons = get_dictionary_if_has_value(list_pokemons_by_trainer, i);
		t_dictionary* objectives = get_dictionary_if_has_value(list_objectives_by_trainer, i);

		t_trainer* trainer = malloc(sizeof(t_trainer));
		trainer->name = i;
		trainer->pos_x = atoi(positions[0]);
		trainer->pos_y = atoi(positions[1]);
		trainer->objective = objectives;
		trainer->caught = pokemons;
		trainer->status = NEW;
		trainer->estimacion_anterior = ESTIMACION_INICIAL;
		trainer->real_anterior = ESTIMACION_INICIAL;
		trainer->pcb_trainer = malloc(sizeof(t_pcb_trainer));


		pthread_create(&thread, NULL, handle_trainer, trainer);
		list_add(trainers, trainer);
		i++;
	}

}

void subscribe_to(event_code code) {
	t_subscription_petition* suscription = malloc(sizeof(t_subscription_petition));
	suscription->ip_len = string_length(IP_TEAM) + 1;
	suscription->ip = IP_TEAM;
	suscription->queue = code;
	suscription->subscriptor_len = 12;
	suscription->subscriptor_id = "TEAM ROCKET";
	suscription->port = PUERTO_TEAM;
	suscription->duration = -1;

	t_buffer* buffer = serialize_t_new_subscriptor_message(suscription);

	uint32_t broker_connection = connect_to(IP_BROKER, PUERTO_BROKER);
	if(broker_connection == -1) {
		log_error(logger, "No se pudo conectar al broker, reintentando en %d seg", TIEMPO_RECONEXION);
//		sleep(TIEMPO_RECONEXION);
//		subscribe_to(code);
	} else {
		log_info(logger, "Conectada cola code %d al broker", code);
		send_message(broker_connection, NEW_SUBSCRIPTOR, NULL, NULL, buffer);

		while(1) {
			handle_event(&broker_connection);
		}
	}

}


//SACAR A TRAINER O RESOLVER DEADLOCK
void swap_pokemons(t_deadlock_matcher* deadlock_matcher) {
	t_trainer* trainer_1 = deadlock_matcher->trainer1;
	t_trainer* trainer_2 = deadlock_matcher->trainer2;
	char* pokemon_1 = deadlock_matcher->pokemon1;
	char* pokemon_2 = deadlock_matcher->pokemon2;
	log_info(logger, "Inicio intercambio entre %d y %d, pokemons %s y %s", trainer_1->name, trainer_2->name, pokemon_1, pokemon_2);

	move_to_position(trainer_1, trainer_2->pos_x, trainer_2->pos_y);
	sleep(RETARDO_CICLO_CPU * 5);
	substract_from_dictionary(trainer_1->caught, pokemon_1);
	substract_from_dictionary(trainer_2->caught, pokemon_2);
	add_to_dictionary(trainer_1->caught, pokemon_2);
	add_to_dictionary(trainer_2->caught, pokemon_1);

	log_info(logger, "Finalizado intercambio entre %d y %d, pokemons %s y %s", trainer_1->name, trainer_2->name, pokemon_1, pokemon_2);

	t_list* leftovers_trainer_1 = get_dictionary_difference(trainer_1->caught, trainer_1->objective);

	if(list_is_empty(leftovers_trainer_1)) {
		trainer_1->status = EXIT;
	} else {
		trainer_1->status = BLOCK;
	}

	t_list* leftovers_trainer_2 = get_dictionary_difference(trainer_2->caught, trainer_2->objective);

	if(list_is_empty(leftovers_trainer_2)) {
		trainer_2->status = EXIT;
	} else {
		trainer_2->status = BLOCK;
	}


	sem_post(&sem_deadlock_directo);
	pthread_mutex_unlock(&mutex_planning_deadlock);
}

