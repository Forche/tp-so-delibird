#ifndef TEAM_H_
#define TEAM_H_

#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<operavirus-commons.h>
#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include "planner.h"
#include "matcher.h"
#include "trainer.h"
#include "util.h"

typedef struct {
	uint32_t quantity;
	uint32_t pokemon_len;
	char* pokemon;
} t_quantity_pokemon;

t_list* pokemon_received_to_catch;
t_list* trainers;
t_list* matches;

pthread_mutex_t* sem_existing_pokemon;
sem_t* sem_trainer_available;

t_dictionary* build_global_objective(char* objectives);
void handle_trainer(t_trainer* trainer);
void create_trainers(char* POSICIONES_ENTRENADORES, char* POKEMON_ENTRENADORES, char* OBJETIVOS_ENTRENADORES);
void create_planner();
void create_matcher();
#endif /* TEAM_H_ */
