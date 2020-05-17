#include "team.h"

int main(void) {

	uint32_t conection;
	t_config* config = config_create("team.config");
	t_list* pokemon_received_to_catch = list_create();
	t_list* trainers = list_create();

	char* POSICIONES_ENTRENADORES = config_get_string_value(config, "POSICIONES_ENTRENADORES");
	char* POKEMON_ENTRENADORES = config_get_string_value(config, "POKEMON_ENTRENADORES");
	char* OBJETIVOS_ENTRENADORES = config_get_string_value(config, "OBJETIVOS_ENTRENADORES");
	char* IP_BROKER = config_get_string_value(config, "IP_BROKER");
	char* PUERTO_BROKER = config_get_string_value(config, "PUERTO_BROKER");

	t_dictionary* global_objective = build_global_objective(OBJETIVOS_ENTRENADORES);

	create_trainers(POSICIONES_ENTRENADORES, POKEMON_ENTRENADORES,OBJETIVOS_ENTRENADORES, trainers);

	create_planner(pokemon_received_to_catch);

	conection = connect_to(IP_BROKER, PUERTO_BROKER);

	while(1) {
		uint32_t size;
		t_appeared_pokemon* appeared_pokemon = deserialize_appeared_pokemon_message(conection, &size);
		if(dictionary_get(global_objective, appeared_pokemon->pokemon)) {
			list_add(pokemon_received_to_catch, appeared_pokemon);
		}
	}

	return EXIT_SUCCESS;
}


t_dictionary* build_global_objective(char* objectives) {
	remove_square_braquets(objectives);
	char* flat_objectives = replace_word(replace_word(objectives, "|", ","), "\"", "");
	return generate_dictionary_by_string(flat_objectives, ",");
}

void create_planner(t_list* pokemon_received_to_catch) {
	pthread_t thread;
	pthread_create(&thread, NULL, planning_catch, pokemon_received_to_catch);
}

void create_trainers(char* POSICIONES_ENTRENADORES, char* POKEMON_ENTRENADORES, char* OBJETIVOS_ENTRENADORES,
		t_list* trainers) {
	remove_square_braquets(POSICIONES_ENTRENADORES);
	remove_square_braquets(POKEMON_ENTRENADORES);
	remove_square_braquets(OBJETIVOS_ENTRENADORES);

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



