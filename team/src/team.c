#include "team.h"

int main(void) {

	uint32_t conection;
	t_config* config = config_create("team.config");

	char* POSICIONES_ENTRENADORES = config_get_string_value(config, "POSICIONES_ENTRENADORES");
	char* POKEMON_ENTRENADORES = config_get_string_value(config, "POKEMON_ENTRENADORES");
	char* OBJETIVOS_ENTRENADORES = config_get_string_value(config, "OBJETIVOS_ENTRENADORES");
	char* IP_BROKER = config_get_string_value(config, "IP_BROKER");
	char* PUERTO_BROKER = config_get_string_value(config, "PUERTO_BROKER");


	t_dictionary* global_objective = build_global_objective(OBJETIVOS_ENTRENADORES);

	create_threads_coaches(POSICIONES_ENTRENADORES, POKEMON_ENTRENADORES,OBJETIVOS_ENTRENADORES);


	conection = connect_to(IP_BROKER, PUERTO_BROKER);

	while(1) {
		uint32_t size;
		t_appeared_pokemon* appeared_pokemon = deserialize_appeared_pokemon_message(conection, &size);
		if(dictionary_get(global_objective, appeared_pokemon->pokemon)) {
			t_coach* closest_coach = get_closest_coach(appeared_pokemon->pos_x, appeared_pokemon->pos_y);
			change_status_to(closest_coach, EXEC);
			move_to_position(closest_coach, appeared_pokemon->pos_x, appeared_pokemon->pos_y);
			catch_pokemon(closest_coach, appeared_pokemon, conection);
			handle_post_catch(closest_coach);
		}
	}

	return EXIT_SUCCESS;
}

void change_status_to(t_coach* coach, status status) {
	coach->status = status;
}

void move_to_position(t_coach* coach, uint32_t pos_x, uint32_t pos_y) {
	coach->pos_x = pos_x;
	coach->pos_y = pos_y;
}

void catch_pokemon(t_coach coach, t_appeared_pokemon* appeared_pokemon, uint32_t conection) {
	t_catch_pokemon* pokemon_to_catch = malloc(sizeof(t_catch_pokemon));
	pokemon_to_catch->pos_x = appeared_pokemon->pos_x;
	pokemon_to_catch->pos_y = appeared_pokemon->pos_y;
	pokemon_to_catch->pokemon_len = appeared_pokemon->pokemon_len;
	pokemon_to_catch->pokemon = appeared_pokemon->pokemon;

	t_buffer* buffer = serialize_t_catch_pokemon_message(pokemon_to_catch);

	send_message(conection, CATCH_POKEMON, NULL, NULL, buffer);
}

t_dictionary* build_global_objective(char* objectives) {
	objectives = replace_word(objectives, "[", "");
	objectives = replace_word(objectives, "]", "");
	char* flat_objectives = replace_word(replace_word(objectives, "|", ","), "\"", "");
	return generate_dictionary_by_string(flat_objectives, ",");
}

void create_threads_coaches(char* POSICIONES_ENTRENADORES, char* POKEMON_ENTRENADORES, char* OBJETIVOS_ENTRENADORES) {
	POSICIONES_ENTRENADORES = replace_word(POSICIONES_ENTRENADORES, "[", "");
	POSICIONES_ENTRENADORES = replace_word(POSICIONES_ENTRENADORES, "]", "");
	POKEMON_ENTRENADORES = replace_word(POKEMON_ENTRENADORES, "[", "");
	POKEMON_ENTRENADORES = replace_word(POKEMON_ENTRENADORES, "]", "");
	OBJETIVOS_ENTRENADORES = replace_word(OBJETIVOS_ENTRENADORES, "[", "");
	OBJETIVOS_ENTRENADORES = replace_word(OBJETIVOS_ENTRENADORES, "]", "");
	char** positions_by_coach = string_split(POSICIONES_ENTRENADORES, ",");
	char** pokemons_by_coach = string_split(POKEMON_ENTRENADORES, ",");
	char** objectives_by_coach = string_split(OBJETIVOS_ENTRENADORES, ",");

	uint32_t i = 0;
	while(positions_by_coach[i] != '\0') {
		pthread_t thread;

		char** positions = string_split(positions_by_coach[i], "|");
		t_dictionary* pokemons = generate_dictionary_by_string(pokemons_by_coach[i], "|");
		t_dictionary* objectives = generate_dictionary_by_string(objectives_by_coach[i], "|");

		t_coach* coach = malloc(sizeof(t_coach));
		coach->pos_x = atoi(positions[0]);
		coach->pos_y = atoi(positions[1]);
		coach->objective = objectives;
		coach->caught = pokemons;
		coach->status = NEW;


		pthread_create(&thread, NULL, handle_coach, coach);
		pthread_join(thread, NULL);
		i++;
	}

}

void handle_coach(t_coach* coach) {

	printf("Hilo: %u \n", pthread_self());
	printf("Posicion X: %zu \n", coach->pos_x);
	printf("Posicion Y: %zu \n", coach->pos_y);
	dictionary_iterator(coach->caught, print_pokemons);
	dictionary_iterator(coach->objective, print_pokemons);
	while(1) {

	}
}


void print_pokemons(char* key, int* value) {
	printf("Pokemon: %s. Cantidad: %d \n", key, *value);
}

t_dictionary* generate_dictionary_by_string(char* text, char* delimiter) {
	char** splited_text = string_split(text, delimiter);
	t_dictionary* dictonary = dictionary_create();

	uint32_t index = 0;
	while(splited_text[index] != '\0') {
		char* item = splited_text[index];
		uint32_t* quantity = dictionary_get(dictonary, item);
		if(quantity == NULL) {
			quantity = malloc(sizeof(uint32_t));
			*quantity = 1;
		} else {
			*quantity += 1;
		}
		dictionary_put(dictonary, item, quantity);
		index++;
	}
	return dictonary;
}


// Function to replace a string with another
// string
char *replace_word(const char *s, const char *oldW,
                                 const char *newW)
{
    char *result;
    int i, cnt = 0;
    int newWlen = strlen(newW);
    int oldWlen = strlen(oldW);

    // Counting the number of times old word
    // occur in the string
    for (i = 0; s[i] != '\0'; i++)
    {
        if (strstr(&s[i], oldW) == &s[i])
        {
            cnt++;

            // Jumping to index after the old word.
            i += oldWlen - 1;
        }
    }

    // Making new string of enough length
    result = (char *)malloc(i + cnt * (newWlen - oldWlen) + 1);

    i = 0;
    while (*s)
    {
        // compare the substring with the result
        if (strstr(s, oldW) == s)
        {
            strcpy(&result[i], newW);
            i += newWlen;
            s += oldWlen;
        }
        else
            result[i++] = *s++;
    }

    result[i] = '\0';
    return result;
}
