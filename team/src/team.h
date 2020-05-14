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
#include<stdio.h>
#include<pthread.h>

typedef struct {
	uint32_t quantity;
	uint32_t pokemon_len;
	char* pokemon;
} t_quantity_pokemon;

typedef enum
{
	NEW = 1,
	READY = 2,
	EXEC = 3,
	BLOCK = 4,
	EXIT = 5,
} status;

typedef struct {
	uint32_t pos_x;
	uint32_t pos_y;
	t_dictionary* objective;
	t_dictionary* caught;
	status status;
} t_coach;

char *replace_word(const char *s, const char *oldW,
                                 const char *newW);
void print_pokemons(char* key, int* value);
t_dictionary* build_global_objective(char* objectives);
void handle_coach(t_coach* coach);
void create_threads_coaches(char* POSICIONES_ENTRENADORES, char* POKEMON_ENTRENADORES, char* OBJETIVOS_ENTRENADORES);
t_dictionary* generate_dictionary_by_string(char* text, char* delimiter);

#endif /* TEAM_H_ */
