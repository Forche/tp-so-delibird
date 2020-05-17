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
#include<semaphore.h>
#include "planner.h"
#include "trainer.h"
#include "util.h"

typedef struct {
	uint32_t quantity;
	uint32_t pokemon_len;
	char* pokemon;
} t_quantity_pokemon;

t_dictionary* build_global_objective(char* objectives);
void handle_trainer(t_trainer* trainer);
void create_trainers(char* POSICIONES_ENTRENADORES, char* POKEMON_ENTRENADORES, char* OBJETIVOS_ENTRENADORES, t_list* trainers);
void create_planner(t_list* pokemon_received_to_catch);
#endif /* TEAM_H_ */
