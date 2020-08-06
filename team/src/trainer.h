/*
 * trainer.h
 *
 *  Created on: 15 may. 2020
 *      Author: utnso
 */

#ifndef SRC_TRAINER_H_
#define SRC_TRAINER_H_

#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<commons/string.h>
#include<operavirus-commons.h>
#include"team.h"

typedef struct {
	t_trainer* trainer;
	t_list* leftovers;
	t_list* remaining;
} t_to_deadlock;

t_list* to_deadlock;

void handle_trainer(t_trainer* trainer);
void change_status_to(t_trainer* trainer, status status);
void catch_pokemon(t_trainer* trainer, t_appeared_pokemon* appeared_pokemon);
void receive_message_id(t_trainer* trainer);
void find_candidate_to_swap(t_list* remaining, t_list* leftovers, t_trainer* trainer);
t_to_deadlock* get_leftover_from_to_deadlock_by_pokemon(char* pokemon);
char* get_pokemon_he_needs(t_list* leftovers, t_list* remaining);
void validate_deadlock(t_trainer* trainer);
bool is_trainer_full(t_trainer* trainer);
uint32_t dictionary_add_values(t_dictionary *self);
void list_remove_by_value(t_list* list, char* value);
void trainer_catch_pokemon(t_trainer* trainer);
void handle_catch(t_trainer* trainer);
void wait_catch(t_trainer* trainer, bool lock_sem_caught);

#endif /* SRC_TRAINER_H_ */
