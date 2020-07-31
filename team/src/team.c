#include "team.h"


void print_pokemons(char* key, int* value) {
	log_info(logger, "Pokemon: %s. Cantidad: %d", key, *value);
}

int main(int argc, char* argv[]) {

	char* string = string_new();
	string_append(&string, "/home/utnso/tp-2020-1c-Operavirus/team/");
	if(argc == 2) {
		string_append(&string, argv[1]);
	} else {
		string_append(&string, "team.config");
	}

	config = config_create(string);
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

	ID = config_get_string_value(config, "ID");
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
	q_ciclos_cpu_totales = 0;
	q_cambios_contexto_totales = 0;


	global_objective = build_global_objective(OBJETIVOS_ENTRENADORES);
	caught_pokemons = build_global_objective(POKEMON_ENTRENADORES);
	build_remaining_pokemons();

	create_trainers(POSICIONES_ENTRENADORES, POKEMON_ENTRENADORES,OBJETIVOS_ENTRENADORES);
	pthread_t thread_planner = create_planner();
	pthread_t thread_matcher = create_matcher();
	pthread_t thread_listener = create_thread_open_socket();

	init_sem();

	send_get_pokemons();


	sem_init(&sem_all_pokemons_caught, 0, 0);

	pthread_t thread_appeared = create_thread_with_param(subscribe_to, APPEARED_POKEMON, "subscribe APPEARED_POKEMON");
	sleep(1);
	pthread_t thread_caught = create_thread_with_param(subscribe_to, CAUGHT_POKEMON, "subscribe CAUGHT_POKEMON");
	sleep(1);
	pthread_t thread_localized = create_thread_with_param(subscribe_to, LOCALIZED_POKEMON, "subscribe LOCALIZED_POKEMON");

	sem_wait(&sem_all_pokemons_caught);

//	pthread_exit(&thread_appeared);
//	pthread_exit(&thread_caught);
//	pthread_exit(&thread_localized);
//	pthread_exit(&thread_planner);
//	pthread_exit(&thread_matcher);
//	pthread_exit(&thread_listener);

	proceed_to_finish();

	return EXIT_SUCCESS;
}

int handle_event(uint32_t* socket) {
	event_code code;
	int bytes = recv(*socket, &code, sizeof(event_code), MSG_WAITALL);
	if (bytes == -1 || bytes == 0) {
		return 0;
	}
		t_message* msg = receive_message(code, *socket);
		uint32_t size;
		t_message_received* message_received = malloc(sizeof(t_message_received));
		switch (code) {
		case LOCALIZED_POKEMON:
			msg->buffer->payload = deserialize_localized_pokemon_message(*socket, &size);
			create_thread_with_param(handle_localized, msg, "handle_localized");
			message_received->message_type = LOCALIZED_POKEMON;
			break;
		case CAUGHT_POKEMON:
			msg->buffer->payload = deserialize_caught_pokemon_message(*socket, &size);
			create_thread_with_param(handle_caught, msg, "handle_caught");
			message_received->message_type = CAUGHT_POKEMON;
			break;
		case APPEARED_POKEMON:
			msg->buffer->payload = deserialize_appeared_pokemon_message(*socket, &size);
			create_thread_with_param(handle_appeared, msg, "handle_appeared");
			message_received->message_type = APPEARED_POKEMON;
			break;
		}
		message_received->received_message_id = msg->id;
		message_received->subscriptor_len = string_length(ID) + 1;
		message_received->subscriptor_id = ID;

		send_message_received_to_broker(message_received, msg->id, msg->correlative_id);
		return 1;
}

void send_message_received_to_broker(t_message_received* message_received, uint32_t id, uint32_t correlative_id){
	t_buffer* buffer_received = serialize_t_message_received(message_received);
	uint32_t connection = connect_to(IP_BROKER,PUERTO_BROKER);
	if(connection == -1) {
			log_error(logger, "No se pudo enviar el ACK al broker. No se pudo conectar al broker");
		} else {
			log_info(logger, "Se envio el ACK al broker");
		}

	send_message(connection, MESSAGE_RECEIVED, id, correlative_id, buffer_received);

}

void handle_localized(t_message* msg) {
	t_localized_pokemon* localized_pokemon = msg->buffer->payload;
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
	t_caught_pokemon* caught_pokemon = msg->buffer->payload;
	log_info(logger, "Recibido CAUGHT_POKEMON. Resultado: %d. Correlative_id: %d", caught_pokemon->result, msg->correlative_id);
	t_trainer* trainer = obtener_trainer_mensaje(msg);
	if (trainer != NULL) {
		trainer->pcb_trainer->result_catch = caught_pokemon->result;
		pthread_mutex_unlock(&trainer->pcb_trainer->sem_caught);
	}
}

void handle_appeared(t_message* msg) {
	t_appeared_pokemon* appeared_pokemon = msg->buffer->payload;
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
	pthread_mutex_init(&mutex_matched_deadlocks, NULL);
	pthread_mutex_init(&mutex_q_ciclos_cpu_totales, NULL);
}

void send_get_pokemons() {
	dictionary_iterator(remaining_pokemons, get_pokemon);
}

void get_pokemon(char* pokemon, uint32_t* cant) {
	create_thread_with_param(send_get, pokemon, "send_get");
	sleep(1);
}

void send_get(char* pokemon) {
	uint32_t broker_connection = connect_to(IP_BROKER, PUERTO_BROKER);
	if(broker_connection == -1) {
		log_error(logger, "Error de comunicacion con el broker, realizando operacion get default");
	} else {
		uint32_t length_pokemon = strlen(pokemon) + 1;
		t_get_pokemon* get_pokemon = malloc(length_pokemon + sizeof(uint32_t));
		get_pokemon->pokemon_len = length_pokemon;
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

pthread_t create_planner() {
	return create_thread(planning_catch, "planning_catch");
}

pthread_t create_matcher() {
	return  create_thread(match_pokemon_with_trainer, "match_pokemon_with_trainer");
}

pthread_t create_thread_open_socket() {
	return create_thread(team_listener, "team_listener");
}

void team_listener() {
	listener(IP_TEAM, PUERTO_TEAM, handle_event);
}

void create_trainers(char* POSICIONES_ENTRENADORES, char* POKEMON_ENTRENADORES, char* OBJETIVOS_ENTRENADORES) {
	char** positions_by_trainer = string_split(POSICIONES_ENTRENADORES, ",");
	char** pokemons_by_trainer = string_split(POKEMON_ENTRENADORES, ",");
	char** objectives_by_trainer = string_split(OBJETIVOS_ENTRENADORES, ",");

	t_list* list_pokemons_by_trainer = generate_list(pokemons_by_trainer);
	t_list* list_objectives_by_trainer = generate_list(objectives_by_trainer);

	uint32_t i = 0;
	while(positions_by_trainer[i] != '\0') {
		pthread_t thread;

		char** positions = string_split(positions_by_trainer[i], "|");
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

		free(positions_by_trainer[i]);
		i++;
		free(positions[0]);
		free(positions[1]);
		free(positions);
	}

	list_iterate(list_pokemons_by_trainer, free);
	list_iterate(list_objectives_by_trainer, free);
	free(list_pokemons_by_trainer);
	free(list_objectives_by_trainer);
	free(positions_by_trainer);
	free(pokemons_by_trainer);
	free(objectives_by_trainer);

}

void subscribe_to(event_code code) {
	t_subscription_petition* suscription = malloc(sizeof(t_subscription_petition));
	suscription->ip_len = string_length(IP_TEAM) + 1;
	suscription->ip = IP_TEAM;
	suscription->queue = code;
	suscription->subscriptor_len = string_length(ID) + 1;
	suscription->subscriptor_id = ID;
	suscription->port = PUERTO_TEAM;
	suscription->duration = -1;

	t_buffer* buffer = serialize_t_new_subscriptor_message(suscription);

	uint32_t broker_connection = connect_to(IP_BROKER, PUERTO_BROKER);
	if(broker_connection == -1) {
		log_error(logger, "No se pudo conectar al broker, reintentando en %d seg", TIEMPO_RECONEXION);
		sleep(TIEMPO_RECONEXION);
		subscribe_to(code);
	} else {
		log_info(logger, "Conectada cola code %s al broker", event_code_to_string(code));
		send_message(broker_connection, NEW_SUBSCRIPTOR, NULL, NULL, buffer);
		int still_connected = 1;
		while(still_connected) {
			still_connected = handle_event(&broker_connection);
		}
		log_error(logger, "Se desconecto del broker");
		close(broker_connection);
	}

}


//SACAR A TRAINER O RESOLVER DEADLOCK
void swap_pokemons(t_deadlock_matcher* deadlock_matcher) {
	t_trainer* trainer_1 = deadlock_matcher->trainer1;
	t_trainer* trainer_2 = deadlock_matcher->trainer2;
	char* pokemon_1 = deadlock_matcher->pokemon1;
	char* pokemon_2 = deadlock_matcher->pokemon2;
	log_info(logger, "Inicio intercambio entre %d y %d, pokemons %s y %s", trainer_1->name, trainer_2->name, pokemon_1, pokemon_2);

	move_to_position(trainer_1, trainer_2->pos_x, trainer_2->pos_y, deadlock_matcher, true);
	exchange_pokemons(deadlock_matcher, true);

	log_info(logger, "Finalizado intercambio entre %d y %d, pokemons %s y %s", trainer_1->name, trainer_2->name, pokemon_1, pokemon_2);

	validate_state_trainer(trainer_1);
	validate_state_trainer(trainer_2);

	count_sem_reverse_direct_deadlock = count_sem_reverse_direct_deadlock + 1;
	if(count_sem_reverse_direct_deadlock == list_size(matched_deadlocks)) {
		pthread_mutex_unlock(&mutex_direct_deadlocks);
	}
	trainer_1->pcb_trainer->quantum = 0;
	trainer_1->real_anterior = trainer_1->estimacion_anterior - trainer_1->estimacion_actual;
	trainer_1->sjf_calculado = false;
	pthread_mutex_unlock(&mutex_planning_deadlock);
}

void exchange_pokemons(t_deadlock_matcher* deadlock_matcher, bool with_validate_desalojo) {
	t_trainer* trainer_1 = deadlock_matcher->trainer1;
	t_trainer* trainer_2 = deadlock_matcher->trainer2;
	char* pokemon_1 = deadlock_matcher->pokemon1;
	char* pokemon_2 = deadlock_matcher->pokemon2;
	uint32_t i;
	for(i = 0;i < 4;i++) {
		sleep(RETARDO_CICLO_CPU);
		log_trace(logger, "Un ciclo de intercambio");
		increment_q_ciclos_cpu(trainer_1);
		if(with_validate_desalojo) {
			trainer_must_go_on(trainer_1, deadlock_matcher);
		}
	}
	log_trace(logger, "Ultimo ciclo de intercambio");
	increment_q_ciclos_cpu(trainer_1);
	substract_from_dictionary(trainer_1->caught, pokemon_1);
	substract_from_dictionary(trainer_2->caught, pokemon_2);
	add_to_dictionary(trainer_1->caught, pokemon_2);
	add_to_dictionary(trainer_2->caught, pokemon_1);
}

void validate_state_trainer(t_trainer* trainer) {
	t_list* leftovers_trainer = get_dictionary_difference(trainer->caught, trainer->objective);
	if(list_is_empty(leftovers_trainer)) {
		trainer->status = EXIT;
	} else {
		trainer->status = BLOCK;
		pthread_mutex_unlock(&trainer->pcb_trainer->sem_deadlock);
	}
}

void increment_q_ciclos_cpu(t_trainer* trainer) {
	trainer->q_ciclos_cpu = trainer->q_ciclos_cpu + 1;
	pthread_mutex_lock(&mutex_q_ciclos_cpu_totales);
	q_ciclos_cpu_totales = q_ciclos_cpu_totales + 1;
	pthread_mutex_unlock(&mutex_q_ciclos_cpu_totales);
}

