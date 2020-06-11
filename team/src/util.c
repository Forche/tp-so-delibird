#include "util.h"


pthread_t create_thread(void* function_thread, char* name) {
    pthread_t thread;
    if (pthread_create(&thread, 0, function_thread, NULL) !=0) {
        log_error(logger, strcat("Error al crear el hilo:", name));
    }
    if (pthread_detach(thread) != 0) {
        log_error(logger, strcat("Error al detachar el hilo:", name));
    }
    return thread;
}

void set_logger_thread(t_log* log) {
	logger = log;
}


char* remove_square_braquets(char* text) {
	return replace_word(replace_word(text, "[", ""), "]", "");
}

void print_pokemons(char* key, int* value) {
	printf("Pokemon: %s. Cantidad: %d \n", key, *value);
}

uint32_t* add_to_dictionary(t_dictionary* dictionary, char* item) {
	uint32_t* quantity = dictionary_get(dictionary, item);
	if (quantity == NULL) {
		quantity = malloc(sizeof(uint32_t));
		*quantity = 1;
	} else {
		*quantity += 1;
	}
	dictionary_put(dictionary, item, quantity);
	return quantity;
}

t_dictionary* generate_dictionary_by_string(char* text, char* delimiter) {
	char** splited_text = string_split(text, delimiter);
	t_dictionary* dictionary = dictionary_create();

	uint32_t index = 0;
	while(splited_text[index] != '\0') {
		char* item = splited_text[index];
		uint32_t* quantity = add_to_dictionary(dictionary, item);
		index++;
	}
	return dictionary;
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
