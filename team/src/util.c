#include "util.h"

char* remove_square_braquets(char* text) {
	return replace_word(replace_word(text, "[", ""), "]", "");
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

uint32_t* substract_from_dictionary(t_dictionary* dictionary, char* item) {
	if (dictionary_has_key(dictionary, item)) {
		uint32_t* quantity = dictionary_get(dictionary, item);
		*quantity -= 1;
		if(*quantity > 0) {
			dictionary_put(dictionary, item, quantity);
		} else {
			dictionary_remove(dictionary, item);
		}
		return quantity;
	} else {
		return NULL;
		//No existe, no se hace nada.
	}
}

t_list* get_dictionary_difference(t_dictionary* a, t_dictionary* b) {
	t_list* difference = list_create();
	void get_difference(char* key, uint32_t* value) {
		int q_diff;
		if(dictionary_has_key(b, key)) {
			uint32_t* q_b = dictionary_get(b, key);
			q_diff =  (*value) - (*q_b);
		} else {
			q_diff =  (*value);
		}

		int i;
		for(i = 0; i < q_diff; i++) {
			char* aux = malloc(strlen(key));
			strcpy(aux, key);
			list_add(difference, aux);
		}
	}
	dictionary_iterator(a, get_difference);
	return difference;
}

t_dictionary* get_dictionary_if_has_value(t_list* list, uint32_t index) {
	if(index < list_size(list)) {
		char* value = list_get(list, index);
		return generate_dictionary_by_string(value, "|");
	} else {
		return dictionary_create();
	}
}

t_list* generate_list(char** array_of_char) {
	t_list* list = list_create();
	uint32_t i;
	for(i = 0; array_of_char[i] != '\0'; i++) {
		list_add(list, array_of_char[i]);
	}
	return list;
}

t_dictionary* generate_dictionary_by_string(char* text, char* delimiter) {
	char** splited_text = string_split(text, delimiter);
	t_dictionary* dictionary = dictionary_create();

	uint32_t index = 0;
	while(splited_text[index] != '\0') {
		char* item = splited_text[index];
		uint32_t* quantity = add_to_dictionary(dictionary, item);
		index++;
		free(item);
	}
	free(splited_text);
	return dictionary;
}

uint32_t dictionary_add_values(t_dictionary *self) {
	int table_index;
	uint32_t value = 0;
	for (table_index = 0; table_index < self->table_max_size; table_index++) {
		t_hash_element *element = self->elements[table_index];

		while (element != NULL) {
			uint32_t* data = element->data;
			value += (*data);
			element = element->next;
		}
	}
	return value;
}

void list_remove_by_value(t_list* list, char* value) {
	uint32_t i;
	 for(i = 0; i< list_size(list); i++) {
		 char* value_list = list_get(list, i);
		 if(string_equals_ignore_case(value, value_list)) {
			 list_remove(list, i);
			 break;
		 }
	 }
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

void* get_from_dictionary_with_mutex(pthread_mutex_t mutex, t_dictionary* dictionary, void* key) {
	pthread_mutex_lock(&mutex);
	uint32_t value = dictionary_get(dictionary, key);
	pthread_mutex_unlock(&mutex);
	return value;
}
