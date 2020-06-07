#include "team.h"

int main(void) {

	uint32_t conection;
	t_config* config = config_create("team.config");
	pokemon_received_to_catch = list_create();
	trainers = list_create();
	matches = list_create();
	logger = log_create("pruebitaTeam.log", "team", true, LOG_LEVEL_INFO);


	char* POSICIONES_ENTRENADORES = config_get_string_value(config, "POSICIONES_ENTRENADORES");
	char* POKEMON_ENTRENADORES = config_get_string_value(config, "POKEMON_ENTRENADORES");
	char* OBJETIVOS_ENTRENADORES = config_get_string_value(config, "OBJETIVOS_ENTRENADORES");
	char* IP_BROKER = config_get_string_value(config, "IP_BROKER");
	char* PUERTO_BROKER = config_get_string_value(config, "PUERTO_BROKER");
	POSICIONES_ENTRENADORES = remove_square_braquets(POSICIONES_ENTRENADORES);
	POKEMON_ENTRENADORES = remove_square_braquets(POKEMON_ENTRENADORES);
	OBJETIVOS_ENTRENADORES = remove_square_braquets(OBJETIVOS_ENTRENADORES);


	global_objective = build_global_objective(OBJETIVOS_ENTRENADORES);

	create_trainers(POSICIONES_ENTRENADORES, POKEMON_ENTRENADORES,OBJETIVOS_ENTRENADORES);
	create_planner();
	create_matcher();

	init_sem();

	conection = connect_to(IP_BROKER, PUERTO_BROKER);

	int sv_socket;
	char* IP = config_get_string_value(config, "IP");
	char* PORT = config_get_string_value(config, "PUERTO");

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(IP, PORT, &hints, &servinfo);

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sv_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1)
			continue;

		if (bind(sv_socket, p->ai_addr, p->ai_addrlen) == -1) {
			close(sv_socket);
			continue;
		}
		break;
	}

	listen(sv_socket, SOMAXCONN);

	freeaddrinfo(servinfo);

	while(1) {
		wait_for_appeared_pokemon(sv_socket);
	}

	return EXIT_SUCCESS;
}

void wait_for_appeared_pokemon(uint32_t sv_socket) {
	struct sockaddr_in client_addr;
	pthread_t thread;
	uint32_t addr_size = sizeof(struct sockaddr_in);

	uint32_t client_socket;

	if ((client_socket = accept(sv_socket, (void*) &client_addr, &addr_size))
			!= -1) {
		pthread_create(&thread, NULL, (void*) serve_client, &client_socket);
		pthread_detach(thread);
	}
}

void serve_client(uint32_t* socket) {
	event_code code;
	if (recv(*socket, &code, sizeof(event_code), MSG_WAITALL) == -1)
		code = -1;

	uint32_t size;
	t_message* msg = receive_message(code, *socket);
	t_appeared_pokemon* appeared_pokemon = deserialize_appeared_pokemon_message(*socket, &size);
	if(dictionary_get(global_objective, appeared_pokemon->pokemon)) {
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

}


t_dictionary* build_global_objective(char* objectives) {
	char* flat_objectives = replace_word(replace_word(objectives, "|", ","), "\"", "");
	return generate_dictionary_by_string(flat_objectives, ",");
}

void create_planner() {
	pthread_t thread;
	pthread_create(&thread, NULL, planning_catch, NULL);
}

void create_matcher() {
	pthread_t thread;
	pthread_create(&thread, NULL, match_pokemon_with_trainer, NULL);
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


		pthread_create(&thread, NULL, handle_trainer, trainer);
		list_add(trainers, trainer);
		i++;
	}

}



