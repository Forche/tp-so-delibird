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

#endif /* SRC_UTIL_H_ */
