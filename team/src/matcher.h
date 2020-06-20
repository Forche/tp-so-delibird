#ifndef SRC_MATCHER_H_
#define SRC_MATCHER_H_

#include<commons/collections/list.h>
#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<operavirus-commons.h>
#include "team.h"

typedef struct {
	int32_t shortest_distance;
	t_trainer* closest_trainer;
	t_appeared_pokemon* closest_pokemon;
} t_match_pokemon_trainer;

void* match_pokemon_with_trainer();
t_match_pokemon_trainer* distance_to_pokemon(t_trainer* trainer, t_match_pokemon_trainer* closest);
t_match_pokemon_trainer* match_closest_trainer();
int get_distance(t_trainer* trainer, t_appeared_pokemon* pokemon);
t_trainer* get_closest_trainer(t_appeared_pokemon* appeared_pokemon);
bool not_exec(t_trainer* trainer);
uint32_t check_remaining_minus_vistima(char* pokemon);

#endif
