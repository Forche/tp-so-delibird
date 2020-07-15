/*
 * util.h
 *
 *  Created on: 15 may. 2020
 *      Author: utnso
 */

#ifndef SRC_UTIL_H_
#define SRC_UTIL_H_

#include<operavirus-commons.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/list.h>

char* remove_square_braquets(char* text);
t_dictionary* generate_dictionary_by_string(char* text, char* delimiter);
char *replace_word(const char *s, const char *oldW, const char *newW);
uint32_t* add_to_dictionary(t_dictionary* dictionary, char* item);
void* get_from_dictionary_with_mutex(pthread_mutex_t mutex, t_dictionary* dictionary, void* key);
void list_remove_by_value(t_list* list, char* value);
t_list* get_dictionary_difference(t_dictionary* a, t_dictionary* b);
uint32_t dictionary_add_values(t_dictionary *self);
t_dictionary* get_dictionary_if_has_value(t_list* list, uint32_t index);
t_list* generate_list(char** array_of_char);

#endif /* SRC_UTIL_H_ */
