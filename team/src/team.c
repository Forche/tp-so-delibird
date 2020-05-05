#include "team.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {

	t_config* config = config_create("team.config");

	char* POSICIONES_ENTRENADORES = config_get_string_value(config, "POSICIONES_ENTRENADORES");
	char* POKEMON_ENTRENADORES = config_get_string_value(config, "POKEMON_ENTRENADORES");
	char* OBJETIVOS_ENTRENADORES = config_get_string_value(config, "OBJETIVOS_ENTRENADORES");
	char* IP_BROKER = config_get_string_value(config, "IP_BROKER");
	char* PUERTO_BROKER = config_get_string_value(config, "PUERTO_BROKER");


	t_dictionary* global_objective = build_global_objective(OBJETIVOS_ENTRENADORES);

	dictionary_iterator(global_objective, print_pokemons);

	return EXIT_SUCCESS;
}

t_dictionary* build_global_objective(char* objectives) {
	char* flat_objectives = replace_word(replace_word(objectives, "|", ","), "\"", "");
	printf("%s \n", flat_objectives);
	char** objectives_by_coach = string_split(flat_objectives, ",");
	t_dictionary* global_objective = dictionary_create();

	uint32_t i = 0;
	while(objectives_by_coach[i] != '\0') {
		char* pokemon = objectives_by_coach[i];
		uint32_t* quantity = dictionary_get(global_objective, pokemon);
		if(quantity == NULL) {
			quantity = malloc(sizeof(uint32_t));
			*quantity = 1;
		} else {
			*quantity += 1;
		}
		dictionary_put(global_objective, pokemon, quantity);
		i++;
	}

	return global_objective;
}

void print_pokemons(char* key, int* value) {
	printf("Pokemon: %s. Cantidad: %d \n", key, *value);
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
