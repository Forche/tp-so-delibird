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
	char* IP_BROKER = config_get_string_value(config, "IP_BROKER");
	char* PUERTO_BROKER = config_get_string_value(config, "PUERTO_BROKER");
	IP_TEAM = config_get_string_value(config, "IP");
	PUERTO_TEAM = config_get_string_value(config, "PUERTO");
	POSICIONES_ENTRENADORES = remove_square_braquets(POSICIONES_ENTRENADORES);
	POKEMON_ENTRENADORES = remove_square_braquets(POKEMON_ENTRENADORES);
	OBJETIVOS_ENTRENADORES = remove_square_braquets(OBJETIVOS_ENTRENADORES);


	global_objective = build_global_objective(OBJETIVOS_ENTRENADORES);

	create_trainers(POSICIONES_ENTRENADORES, POKEMON_ENTRENADORES,OBJETIVOS_ENTRENADORES);
	create_planner();
	create_matcher();

	init_sem();

	broker_connection = connect_to(IP_BROKER, PUERTO_BROKER);


	listener(IP_TEAM, PUERTO_TEAM, handle_event);

	return EXIT_SUCCESS;
}


void handle_event(uint32_t* socket) {
	event_code code;
	if (recv(*socket, &code, sizeof(event_code), MSG_WAITALL) == -1)
		code = -1;

	t_message* msg = receive_message(code, *socket);
	uint32_t size;
	switch (code) {
	case LOCALIZED_POKEMON:;
		t_localized_pokemon* localized_pokemon = deserialize_localized_pokemon_message(*socket, &size);
		msg->buffer = localized_pokemon;

		//TODO Iterar localized por la cantidad de pokemons para crear structs de appeared_pokemon
		// y el funcionamiento es el mismo que en appeared
		break;
	case CAUGHT_POKEMON:;
		t_caught_pokemon* caught_pokemon = deserialize_caught_pokemon_message(*socket, &size);
		msg->buffer = caught_pokemon;

		t_trainer* trainer = list_get(trainers, 0);//obtener_trainer_mensaje(msg);
		trainer->pcb_trainer->result_catch = caught_pokemon->result;
		pthread_mutex_unlock(&trainer->pcb_trainer->sem_caught);

		break;
	case APPEARED_POKEMON:;
		t_appeared_pokemon* appeared_pokemon = deserialize_appeared_pokemon_message(*socket, &size);
		msg->buffer = appeared_pokemon;
		if(dictionary_get(global_objective, appeared_pokemon->pokemon)) {
			pthread_mutex_lock(&mutex_pokemon_received_to_catch);
			list_add(pokemon_received_to_catch, appeared_pokemon);
			pthread_mutex_unlock(&mutex_pokemon_received_to_catch);
			sem_post(&sem_appeared_pokemon);
		}
		break;
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



