/*
 * team.h
 *
 *  Created on: 4 may. 2020
 *      Author: utnso
 */

#ifndef TEAM_H_
#define TEAM_H_

#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<operavirus-commons.h>
#include<stdlib.h>

typedef struct {
	uint32_t quantity;
	uint32_t pokemon_len;
	char* pokemon;
} t_quantity_pokemon;

typedef struct {
	uint32_t posx;
	uint32_t posy;
	t_dictionary* objective;
	t_dictionary* caugth;
} t_coach;

char *replace_word(const char *s, const char *oldW,
                                 const char *newW);
void print_pokemons(char* key, int* value);
t_dictionary* build_global_objective(char* objectives);

#endif /* TEAM_H_ */
